#!/usr/bin/env bash
#
# Assemble the installable conboard artifact for the architecture currently
# being built. Runs INSIDE the Docker builder stage (see docker/Dockerfile).
#
#   usage: package.sh <outdir>
#
# Produces:
#   <outdir>/conboard/                 tree mirroring the on-device /conboard
#   <outdir>/conboard-<arch>.tar.gz    the same tree, tarred for transfer
#
# The tree deliberately mirrors the paths the systemd units reference
# (e.g. /conboard/LowLevel/launcher/build/launcher) so install-on-device.sh is
# just a copy + enable, with no compilation on the board.
set -euo pipefail

OUT="${1:?usage: package.sh <outdir>}"
SRC="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ARCH="$(dpkg --print-architecture)"          # arm64 | armhf
STAGE="$OUT/conboard"

# Board labelling (passed from build-cross.sh via docker/boards.conf).
BOARD_ID="${BOARD_ID:-unknown}"
BOARD_DESC="${BOARD_DESC:-unknown board}"
BOARD_OTG="${BOARD_OTG:-unknown}"
TARBALL="conboard-${BOARD_ID}.tar.gz"

mkdir -p "$STAGE"

# install -D creates parent dirs and sets mode in one shot.
copy() { install -D -m "$1" "$SRC/$2" "$STAGE/$2"; }

# --- compiled binaries (paths must match the systemd ExecStart lines) --------
copy 644 LowLevel/Common/build/libcommon.so
copy 755 LowLevel/launcher/build/launcher
copy 755 LowLevel/dispatcher/build/dispatcher
copy 755 LowLevel/MIDI/build/conMIDI
copy 755 LowLevel/Mouse/build/conMouse
copy 755 LowLevel/KeyBoard/build/conKeyB
copy 755 LowLevel/Joystick/build/conJoyS

# diagnostic tool (run on the board to classify attached devices)
copy 755 tools/devprobe/build/devprobe

# management API backend (Crow REST + gRPC over harpia SOCI/SQLite DAOs). Installed
# at backend/conboard_backend to match backend.service ExecStart.
install -D -m 755 "$SRC/backend/build/conboard_backend" "$STAGE/backend/conboard_backend"

# --- systemd units -----------------------------------------------------------
copy 644 LowLevel/assets/usb-otg.service
copy 644 LowLevel/assets/launcher.service
copy 644 LowLevel/dispatcher/assets/dispatcher.service
copy 644 backend/assets/backend.service

# --- udev rule + hotplug handler ---------------------------------------------
copy 644 LowLevel/assets/100-usb.rules
copy 755 LowLevel/assets/event_handler.sh

# --- runtime config ----------------------------------------------------------
copy 644 LowLevel/dispatcher/assets/config.json

# --- gadget scripts + board definitions --------------------------------------
mkdir -p "$STAGE/scripts" "$STAGE/boards"
cp "$SRC"/scripts/*.sh "$STAGE/scripts/"
cp "$SRC"/boards/*.json "$STAGE/boards/" 2>/dev/null || true

# --- console UI: built SPA bundle + nginx site -------------------------------
# nginx serves the static bundle from /conboard/frontend and proxies the API/WS
# (see backend/assets/interface.conf). The bundle is built by the Dockerfile's
# `frontend` stage and copied to frontend/console/dist before this runs.
if [ -d "$SRC/frontend/console/dist" ]; then
    mkdir -p "$STAGE/frontend"
    cp -r "$SRC/frontend/console/dist/." "$STAGE/frontend/"
else
    echo "WARN: frontend/console/dist missing -- console UI will NOT be packaged" >&2
fi
copy 644 backend/assets/interface.conf

# --- bundle the runtime shared-library closure -------------------------------
# So the board needs NO `apt install`: gather every non-core .so that the shipped
# binaries depend on (transitively, via ldd) into /conboard/lib, then stamp an
# absolute RUNPATH=/conboard/lib onto every conboard ELF (binaries AND the bundled
# libs, so the loader resolves inter-library deps too -- RUNPATH is not transitive).
# We do NOT bundle the glibc core (loader + libc/libm/libdl/libpthread/librt/
# libresolv): it is ABI-stable and present on every target, and bundling it risks a
# loader/glibc mismatch. We DO bundle libstdc++/libgcc_s (GCC runtime versions vary).
# Uses absolute RUNPATH (install path is always /conboard) rather than a global
# ldconfig entry, so conboard's libs never shadow the system's for other programs.
echo "== bundling runtime shared libraries into /conboard/lib =="
BUNDLE="$STAGE/lib"
mkdir -p "$BUNDLE"
ELVES=(
    "$STAGE/LowLevel/Common/build/libcommon.so"
    "$STAGE/LowLevel/launcher/build/launcher"
    "$STAGE/LowLevel/dispatcher/build/dispatcher"
    "$STAGE/LowLevel/MIDI/build/conMIDI"
    "$STAGE/LowLevel/Mouse/build/conMouse"
    "$STAGE/LowLevel/KeyBoard/build/conKeyB"
    "$STAGE/LowLevel/Joystick/build/conJoyS"
    "$STAGE/tools/devprobe/build/devprobe"
    "$STAGE/backend/conboard_backend"
)
is_core_soname() {  # true for libs guaranteed present on the target (do not bundle)
    case "$1" in
        ld-linux*|libc.so.*|libm.so.*|libdl.so.*|libpthread.so.*|librt.so.*|libresolv.so.*) return 0 ;;
        *) return 1 ;;
    esac
}
# Copy each resolved dependency under its SONAME (the left-hand ldd name), so the
# binary's DT_NEEDED matches; ldd is recursive, so this captures the full closure.
for e in "${ELVES[@]}"; do
    [ -f "$e" ] || continue
    ldd "$e" 2>/dev/null | while read -r soname arrow path _rest; do
        [ "$arrow" = "=>" ] || continue          # skip vdso + the loader line
        [ -e "$path" ] || continue
        is_core_soname "$soname" && continue
        [ -e "$BUNDLE/$soname" ] || cp -L "$path" "$BUNDLE/$soname"
    done
done
# libcommon is part of the bundle too (its old sibling path is kept for the units).
cp -L "$STAGE/LowLevel/Common/build/libcommon.so" "$BUNDLE/libcommon.so" 2>/dev/null || true
chmod 644 "$BUNDLE"/*.so* 2>/dev/null || true
# Stamp RUNPATH on binaries and on the bundled libs alike.
for e in "${ELVES[@]}"; do
    [ -f "$e" ] && patchelf --set-rpath /conboard/lib "$e" 2>/dev/null || true
done
for so in "$BUNDLE"/*.so*; do
    [ -f "$so" ] && patchelf --set-rpath /conboard/lib "$so" 2>/dev/null || true
done
echo "   bundled $(ls -1 "$BUNDLE" | wc -l) libraries ($(du -sh "$BUNDLE" | cut -f1))"

# --- on-device installer + uninstaller (ship inside the artifact) ------------
install -D -m 755 "$SRC/docker/install-on-device.sh"   "$STAGE/install-on-device.sh"
install -D -m 755 "$SRC/docker/uninstall-on-device.sh" "$STAGE/uninstall-on-device.sh"

# --- BOARD.txt: the "surname" -- which board this artifact is for ------------
cat > "$STAGE/BOARD.txt" <<EOF
board-id : ${BOARD_ID}
goes in  : ${BOARD_DESC}
arch     : ${ARCH}
usb-otg  : ${BOARD_OTG}
EOF

# --- HOW-TO-INSTALL.txt: the main commands -----------------------------------
cat > "$STAGE/HOW-TO-INSTALL.txt" <<EOF
conboard — install on: ${BOARD_DESC} (${ARCH})

1. Copy the tarball to the board:
     scp ${TARBALL} <user>@<board-ip>:~

2. On the board, unpack and run the installer (no compilation, no apt on the board --
   every runtime library is bundled under ./lib):
     tar xzf ${TARBALL}
     cd conboard
     sudo ./install-on-device.sh

3. Verify the USB gadget can bind + the backend is up:
     cat /sys/class/udc/*/state          # want: configured (when plugged into a host PC)
     systemctl status usb-otg.service dispatcher.service launcher.service backend.service
     curl -s localhost:8080/healthz      # backend management API -> "ok"

4. Classify attached devices (diagnostic):
     sudo /conboard/tools/devprobe/build/devprobe

To remove conboard (keeps the rules DB; add --purge to delete it too):
     sudo ./uninstall-on-device.sh

USB-OTG status for this board: ${BOARD_OTG}
EOF

# --- manifest: the eyeballable summary of what got built ---------------------
MANIFEST="$STAGE/MANIFEST.txt"
{
    echo "conboard prebuilt artifact"
    echo "board        : ${BOARD_ID} (${BOARD_DESC})"
    echo "architecture : $ARCH"
    echo
    echo "== binaries (ELF arch should match '$ARCH') =="
    for rel in \
        LowLevel/Common/build/libcommon.so \
        LowLevel/launcher/build/launcher \
        LowLevel/dispatcher/build/dispatcher \
        LowLevel/MIDI/build/conMIDI \
        LowLevel/Mouse/build/conMouse \
        LowLevel/KeyBoard/build/conKeyB \
        LowLevel/Joystick/build/conJoyS \
        tools/devprobe/build/devprobe \
        backend/conboard_backend ; do
        file -b "$STAGE/$rel" | sed "s|^|$rel: |"
    done
    echo
    echo "== bundled runtime libraries (/conboard/lib; no apt needed on the board) =="
    ( cd "$BUNDLE" && ls -1 ) | sed 's|^|  |'
    echo
    echo "== dispatcher shared-library needs (RUNPATH resolves to /conboard/lib) =="
    objdump -p "$STAGE/LowLevel/dispatcher/build/dispatcher" 2>/dev/null \
        | awk '/NEEDED|RUNPATH|RPATH/ {print "  " $1 " " $2}' || echo "  (objdump unavailable)"
    echo "== backend shared-library needs =="
    objdump -p "$STAGE/backend/conboard_backend" 2>/dev/null \
        | awk '/NEEDED|RUNPATH|RPATH/ {print "  " $1 " " $2}' || echo "  (objdump unavailable)"
    echo
    echo "== sha256 =="
    ( cd "$STAGE" && find . -type f ! -name MANIFEST.txt -print0 | sort -z | xargs -0 sha256sum )
} > "$MANIFEST"

# --- tarball -----------------------------------------------------------------
( cd "$OUT" && tar czf "$TARBALL" conboard )

# Surface BOARD.txt + HOW-TO-INSTALL.txt at the dist-folder root too, so they're
# visible without unpacking the tarball (NOTES.md: "dist folder should have...").
cp "$STAGE/BOARD.txt" "$STAGE/HOW-TO-INSTALL.txt" "$OUT/"

echo "packaged ${BOARD_ID} (${ARCH}) -> $OUT/${TARBALL}"
cat "$MANIFEST"
