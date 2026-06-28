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

# --- systemd units -----------------------------------------------------------
copy 644 LowLevel/assets/usb-otg.service
copy 644 LowLevel/assets/launcher.service
copy 644 LowLevel/dispatcher/assets/dispatcher.service
copy 644 backend/assets/frontend.service

# --- udev rule + hotplug handler ---------------------------------------------
copy 644 LowLevel/assets/100-usb.rules
copy 755 LowLevel/assets/event_handler.sh

# --- runtime config ----------------------------------------------------------
copy 644 LowLevel/dispatcher/assets/config.json

# --- gadget scripts + board definitions --------------------------------------
mkdir -p "$STAGE/scripts" "$STAGE/boards"
cp "$SRC"/scripts/*.sh "$STAGE/scripts/"
cp "$SRC"/boards/*.json "$STAGE/boards/" 2>/dev/null || true

# --- on-device installer (ships inside the artifact) -------------------------
install -D -m 755 "$SRC/docker/install-on-device.sh" "$STAGE/install-on-device.sh"

# --- manifest: the eyeballable summary of what got built ---------------------
MANIFEST="$STAGE/MANIFEST.txt"
{
    echo "conboard prebuilt artifact"
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
        LowLevel/Joystick/build/conJoyS ; do
        file -b "$STAGE/$rel" | sed "s|^|$rel: |"
    done
    echo
    echo "== dispatcher shared-library needs =="
    objdump -p "$STAGE/LowLevel/dispatcher/build/dispatcher" 2>/dev/null \
        | awk '/NEEDED/ {print "  " $2}' || echo "  (objdump unavailable)"
    echo
    echo "== sha256 =="
    ( cd "$STAGE" && find . -type f ! -name MANIFEST.txt -print0 | sort -z | xargs -0 sha256sum )
} > "$MANIFEST"

# --- tarball -----------------------------------------------------------------
( cd "$OUT" && tar czf "conboard-${ARCH}.tar.gz" conboard )

echo "packaged $ARCH -> $OUT/conboard-${ARCH}.tar.gz"
cat "$MANIFEST"
