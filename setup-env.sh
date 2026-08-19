#!/usr/bin/env bash
#
# One-shot dev-environment bootstrap for a fresh conboard clone (e.g. after
# rebuilding a WSL2 distro from scratch). Gets you to the point where:
#
#   ./run-tests.sh          works (native unit tests)
#   frontend/console        builds/dev-serves with the host Node
#   ./build-cross.sh        works (Docker cross-build for real board artifacts)
#
# Safe to re-run — every step checks current state before changing anything.
set -uo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")"

ok()   { echo "  [ok] $*"; }
warn() { echo "  [!!] $*"; }
info() { echo ">> $*"; }

FAILED=0

# --- git submodules (Crow, lvgl) --------------------------------------------
info "git submodules (Crow, lvgl)"
if git submodule update --init --recursive; then
    ok "submodules present"
else
    warn "submodule init failed — check network access to github.com"
    FAILED=1
fi

# --- native C++ toolchain (needed for ./run-tests.sh) -----------------------
info "native toolchain (g++, cmake) for ./run-tests.sh"
need_pkgs=()
command -v g++   >/dev/null 2>&1 || need_pkgs+=(build-essential)
command -v cmake >/dev/null 2>&1 || need_pkgs+=(cmake)
if [ "${#need_pkgs[@]}" -gt 0 ]; then
    info "installing: ${need_pkgs[*]} (sudo apt-get)"
    if sudo apt-get update -qq && sudo apt-get install -y "${need_pkgs[@]}"; then
        ok "installed ${need_pkgs[*]}"
    else
        warn "failed to install ${need_pkgs[*]} — install manually"
        FAILED=1
    fi
else
    ok "g++ $(g++ --version | head -1 | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -1), cmake $(cmake --version | head -1 | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -1)"
fi

# --- Node (frontend/console) -------------------------------------------------
info "Node (frontend/console)"
if command -v node >/dev/null 2>&1; then
    node_path="$(command -v node)"
    node_ver="$(node --version)"
    case "$node_path" in
        /mnt/c/*|/mnt/[a-z]/*)
            warn "node resolves to a Windows install ($node_path)."
            warn "npm install here would pull the wrong platform's Rollup binary (see frontend/README.md)."
            warn "install a Linux Node instead, e.g.: curl -o- https://raw.githubusercontent.com/nvm-sh/nvm/v0.40.1/install.sh | bash && nvm install 22"
            FAILED=1
            ;;
        *)
            ok "node $node_ver ($node_path)"
            ;;
    esac
else
    warn "node not found. Install a Linux Node (e.g. via nvm): https://github.com/nvm-sh/nvm"
    FAILED=1
fi

# --- Docker + buildx (./build-cross.sh) --------------------------------------
info "Docker + buildx (./build-cross.sh — cross-compiled board artifacts)"
if docker version >/dev/null 2>&1; then
    ok "docker reachable: $(docker version --format '{{.Server.Version}}' 2>/dev/null || echo '(daemon up)')"
    if docker buildx version >/dev/null 2>&1; then
        ok "buildx: $(docker buildx version)"
    else
        warn "docker found but buildx isn't — install the buildx plugin (bundled with current Docker Desktop)"
        FAILED=1
    fi
else
    warn "docker not reachable from this WSL distro."
    warn "Docker Desktop for Windows is installed but this distro isn't enabled for WSL integration."
    warn "Fix (one-time, on the Windows side): Docker Desktop -> Settings -> Resources -> WSL Integration"
    warn "  -> toggle this distro on -> Apply & Restart. Then re-run this script."
    FAILED=1
fi

# --- smoke test: native unit tests -------------------------------------------
if command -v g++ >/dev/null 2>&1 && command -v cmake >/dev/null 2>&1; then
    info "smoke test: ./run-tests.sh"
    if ./run-tests.sh >/tmp/conboard-run-tests.log 2>&1; then
        ok "native unit tests pass ($(grep -oE '[0-9]+ \| [0-9]+ passed' /tmp/conboard-run-tests.log | tail -1))"
    else
        warn "native unit tests failed — see /tmp/conboard-run-tests.log"
        FAILED=1
    fi
fi

echo
if [ "$FAILED" -eq 0 ]; then
    echo "== environment ready =="
    echo "  ./run-tests.sh              # native unit tests"
    echo "  ./build-cross.sh list        # cross-build a board artifact"
    echo "  cd frontend/console && npm install && npm run dev -- --host"
else
    echo "== environment NOT fully ready — see [!!] lines above =="
fi
exit "$FAILED"
