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

echo "== registering libcommon.so with the dynamic linker =="
# The conboard binaries link libcommon.so (which lives next to them under
# LowLevel/Common/build). The binaries also carry an $ORIGIN-relative RUNPATH,
# but install it into a standard lib dir + ldconfig as well, so resolution never
# depends on RUNPATH or on how/where the binary is launched.
install -m 644 /conboard/LowLevel/Common/build/libcommon.so /usr/local/lib/
ldconfig

echo "== installing systemd units =="
install -m 644 /conboard/LowLevel/assets/usb-otg.service            /etc/systemd/system/
install -m 644 /conboard/LowLevel/assets/launcher.service           /etc/systemd/system/
install -m 644 /conboard/LowLevel/dispatcher/assets/dispatcher.service /etc/systemd/system/
systemctl daemon-reload

echo "== installing udev rule + event handler =="
install -m 644 /conboard/LowLevel/assets/100-usb.rules /etc/udev/rules.d/
# 100-usb.rules runs /conboard/event_handler.sh on hotplug; it must live at that
# exact path (the rsync only puts it under LowLevel/assets/). Without this, USB
# add/remove events never reach the launcher and no device service is created.
install -m 755 /conboard/LowLevel/assets/event_handler.sh /conboard/event_handler.sh
# Reload rules only here — do NOT trigger yet: a replayed 'add' would spawn
# handlers before usb-otg + dispatcher are up, so they couldn't register with the
# dispatcher or reach /dev/hidgN. The coldplug replay happens after those start.
udevadm control --reload-rules || true

echo "== enabling services (start on boot) =="
systemctl enable usb-otg.service dispatcher.service launcher.service

# Start now — but NOT with `enable --now`. launcher.service is a Type=oneshot
# that scans devices and can call `systemctl restart` for a matched device;
# starting it inside the same systemd transaction that `--now` opens deadlocks
# the two jobs (the installer hangs). Start as plain, separate jobs instead.
echo "== starting services =="
systemctl start usb-otg.service       # brings up the USB gadget now
systemctl start dispatcher.service

# Coldplug at install time: handle devices ALREADY attached now, without a reboot.
# launcher.service does exactly this at boot (replay 'add' uevents once the
# dispatcher + gadget are up). Replay it here too, now that both are started, so a
# fresh install picks up an attached controller without a manual unplug/replug.
# Run the trigger directly (not `systemctl start launcher.service`) to avoid any
# oneshot-in-transaction nesting; udevd re-runs 100-usb.rules -> event_handler.sh
# -> launcher, spawning the same identity-named handler as a live hotplug.
echo "== coldplug: replaying attached USB devices =="
udevadm trigger --action=add --subsystem-match=usb || true
udevadm settle || true

echo
echo "Done. Attached devices are handled now; the launcher also runs at boot and on hotplug (udev)."
echo "Quick checks:"
echo "  ls /sys/class/udc            # must be non-empty for USB gadget to bind"
echo "  systemctl status usb-otg.service dispatcher.service launcher.service"
