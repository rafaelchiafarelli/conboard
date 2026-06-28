#!/usr/bin/env bash
#
# Install a prebuilt conboard artifact ON the Orange Pi (run this on the board,
# as root, from inside the unpacked artifact directory).
#
# Unlike scripts/install.sh this does NOT compile anything -- the binaries in
# this tree were already cross-built for this board's architecture. It only
# installs the runtime shared libraries, drops the tree at /conboard, and wires
# up the systemd units.
#
#   sudo ./install-on-device.sh
#
# The python frontend (backend/) still needs its venv set up separately; see
# scripts/install.sh install_frontend. This script handles the C++/USB layer.
set -euo pipefail

if [ "$EUID" -ne 0 ]; then
    echo "Please run as root (sudo ./install-on-device.sh)" >&2
    exit 1
fi

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "== checking architecture =="
TARGET_ARCH=$(dpkg --print-architecture)
BIN_ARCH=$(file -b "$HERE/LowLevel/dispatcher/build/dispatcher" 2>/dev/null || echo unknown)
echo "board: $TARGET_ARCH"
echo "binary: $BIN_ARCH"
case "$TARGET_ARCH:$BIN_ARCH" in
    arm64:*aarch64*|armhf:*ARM*EABI*|armhf:*ARM,*) : ;;
    *) echo "WARNING: artifact arch may not match this board -- continuing anyway." >&2 ;;
esac

echo "== installing runtime shared libraries =="
export DEBIAN_FRONTEND=noninteractive
apt-get update
# Runtime counterparts of the build-time -dev packages (see docker/Dockerfile).
# Boost is installed via -dev to dodge version-suffixed runtime package names.
apt-get install -y --no-install-recommends \
    libzmq5 libasound2 libuuid1 \
    libboost-system-dev libboost-date-time-dev

echo "== stopping any running conboard services =="
for unit in usb-otg launcher dispatcher frontend; do
    systemctl stop "${unit}.service" 2>/dev/null || true
done

echo "== copying program tree to /conboard =="
mkdir -p /conboard
# Copy everything except this installer and the manifest.
rsync -a --delete \
    --exclude install-on-device.sh --exclude MANIFEST.txt \
    "$HERE"/ /conboard/

echo "== installing systemd units =="
install -m 644 /conboard/LowLevel/assets/usb-otg.service            /etc/systemd/system/
install -m 644 /conboard/LowLevel/assets/launcher.service           /etc/systemd/system/
install -m 644 /conboard/LowLevel/dispatcher/assets/dispatcher.service /etc/systemd/system/
systemctl daemon-reload

echo "== installing udev rule + event handler =="
install -m 644 /conboard/LowLevel/assets/100-usb.rules /etc/udev/rules.d/
udevadm control --reload-rules && udevadm trigger || true

echo "== enabling + starting services =="
systemctl enable --now usb-otg.service
systemctl enable --now dispatcher.service
systemctl enable --now launcher.service

echo
echo "Done. Quick checks:"
echo "  ls /sys/class/udc            # must be non-empty for USB gadget to bind"
echo "  systemctl status usb-otg.service dispatcher.service launcher.service"
