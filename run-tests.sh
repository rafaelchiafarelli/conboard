#!/usr/bin/env bash
#
# Run conboard's unit tests (pure logic, synthetic inputs — no hardware).
#
#   ./run-tests.sh                       # all tests, natively (fast)
#   ./run-tests.sh joystick              # only the "joystick" suite
#   ./run-tests.sh joystick:hitTheAButton# only that case in that suite
#   ./run-tests.sh --qemu zero3          # all tests under emulation for zero3's arch
#   ./run-tests.sh --qemu zero3 joystick # filtered, under emulation
#
# Native mode needs a host g++/cmake. --qemu reuses the cross-build's QEMU to run
# the SAME tests on the target architecture (proves arch-correctness).
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")"

# --- parse args -------------------------------------------------------------
QEMU=0; BOARD=""; FILTER=""
if [ "${1:-}" = "--qemu" ]; then
    QEMU=1; BOARD="${2:-}"; FILTER="${3:-}"
    [ -z "$BOARD" ] && { echo "usage: ./run-tests.sh --qemu <board-id> [filter]" >&2; exit 1; }
else
    FILTER="${1:-}"
fi

# Translate a "suite[:case]" filter into doctest flags.
dt_args=()
if [ -n "$FILTER" ]; then
    suite="${FILTER%%:*}"
    case="${FILTER#*:}"
    [ -n "$suite" ] && dt_args+=("--test-suite=*${suite}*")
    [ "$case" != "$FILTER" ] && [ -n "$case" ] && dt_args+=("--test-case=*${case}*")
fi

# --- native -----------------------------------------------------------------
if [ "$QEMU" -eq 0 ]; then
    echo ">> building tests (native)"
    cmake -S tests -B build/tests >/dev/null
    cmake --build build/tests >/dev/null
    echo ">> running"
    exec ./build/tests/conboard_tests "${dt_args[@]}"
fi

# --- under QEMU for the board's arch ----------------------------------------
# Resolve the board's docker platform from the registry.
plat="$(awk -F'|' -v b="$BOARD" '
    $1 ~ "^[[:space:]]*"b"[[:space:]]*$" { gsub(/^[ \t]+|[ \t]+$/,"",$2); print $2 }
' docker/boards.conf)"
[ -z "$plat" ] && { echo "unknown board '$BOARD' (see ./build-cross.sh list)" >&2; exit 1; }

echo ">> registering QEMU emulators (one-time)"
docker run --privileged --rm tonistiigi/binfmt --install arm64,arm >/dev/null

echo ">> building + running tests under $plat (emulated)"
docker run --rm --platform "$plat" -v "$PWD":/src:ro -w /src debian:bookworm-slim bash -c "
    set -e
    apt-get update -qq >/dev/null && DEBIAN_FRONTEND=noninteractive \
        apt-get install -y -qq g++ cmake make >/dev/null
    cmake -S tests -B /tmp/tb >/dev/null && cmake --build /tmp/tb >/dev/null
    /tmp/tb/conboard_tests ${dt_args[*]}
"
