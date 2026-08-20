#!/usr/bin/env bash
#
# Install a prebuilt conboard artifact ON the Orange Pi (run this on the board,
# as root, from inside the unpacked artifact directory).
#
# Unlike scripts/install.sh this does NOT compile anything AND does NOT apt-install
# anything -- the binaries were cross-built for this board's architecture and every
# runtime shared library they need is bundled under ./lib (resolved via an absolute
# RUNPATH=/conboard/lib stamped at build time). It just drops the tree at /conboard
# and wires up the systemd units.
#
#   sudo ./install-on-device.sh
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

echo "== stopping any running conboard services =="
# Includes retired units (frontend, old backend) so a re-install cleans them up.
for unit in usb-otg usb-gadget-dhcp launcher dispatcher backend hmi frontend conboard-firewall; do
    systemctl stop "${unit}.service" 2>/dev/null || true
done

echo "== removing stale auto-generated per-device services =="
# The launcher writes one unit per matched device (Description="auto generated
# service file."). These are NOT part of the artifact, so nothing else cleans them,
# and a lingering/stale one makes the launcher `systemctl restart` an OLD config
# instead of writing a fresh unit (a dead handler after re-install -- e.g. a MIDI
# device that stops sending). Wipe them so every install regenerates them clean.
for f in /etc/systemd/system/*.service; do
    [ -e "$f" ] || continue
    if grep -q "auto generated service file" "$f" 2>/dev/null; then
        u="$(basename "$f")"
        systemctl stop "$u" 2>/dev/null || true
        systemctl disable "$u" 2>/dev/null || true
        rm -f "$f"
        echo "   removed stale $u"
    fi
done
systemctl daemon-reload 2>/dev/null || true

echo "== copying program tree to /conboard =="
mkdir -p /conboard
# Copy everything except the installer/uninstaller and the manifest. Preserve the
# rules-library data dir if it already exists (do not --delete it).
rsync -a --delete \
    --exclude install-on-device.sh --exclude uninstall-on-device.sh \
    --exclude MANIFEST.txt --exclude backend/data \
    "$HERE"/ /conboard/
mkdir -p /conboard/backend/data

echo "== runtime libraries: bundled under /conboard/lib (no apt install) =="
# Nothing to install: the binaries carry RUNPATH=/conboard/lib and ./lib holds the
# full non-core .so closure (libstdc++, libzmq, libasound, libuuid, boost, soci,
# protobuf, grpc, sqlite, ...). libcommon.so is in there too. We deliberately do NOT
# add /conboard/lib to the global ld.so path, so these never shadow the system libs.
ls /conboard/lib >/dev/null 2>&1 || echo "  WARNING: /conboard/lib missing from artifact!"

echo "== installing systemd units =="
install -m 644 /conboard/LowLevel/assets/usb-otg.service               /etc/systemd/system/
install -m 644 /conboard/LowLevel/assets/launcher.service              /etc/systemd/system/
install -m 644 /conboard/LowLevel/dispatcher/assets/dispatcher.service /etc/systemd/system/
install -m 644 /conboard/backend/assets/backend.service                /etc/systemd/system/
install -m 644 /conboard/LowLevel/HMI/assets/hmi.service               /etc/systemd/system/
install -m 644 /conboard/docker/assets/conboard-firewall.service       /etc/systemd/system/
if command -v dnsmasq >/dev/null 2>&1; then
    install -m 644 /conboard/docker/assets/usb-gadget-dhcp.service /etc/systemd/system/
else
    echo "  WARNING: dnsmasq not found -- the USB-ethernet gadget (usb0), if this board's USB controller has room for it, won't hand out an IP. Install dnsmasq-base (NOT the 'dnsmasq' metapackage -- that one auto-enables its own system-wide DNS resolver you don't want here) and re-run this script." >&2
fi
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

echo "== installing conboard-password (retrieve/reset the web login any time) =="
# On PATH regardless of where the artifact was unpacked, so "I forgot/lost the
# password" is always `sudo conboard-password --reset`, never a real lockout.
install -m 755 /conboard/docker/assets/conboard-password.sh /usr/local/bin/conboard-password

echo "== web login: generating/keeping a per-install password =="
# nginx's Basic Auth (backend/assets/interface.conf) is the real perimeter secret --
# the app-level X-User/X-Pswd the backend also checks is a fixed value baked into
# harpia-generated code (effectively public, see backend/README.md), not a real
# credential. Generate a random password on first install (via the same
# conboard-password helper a customer would use to reset one later, so there's one
# code path); a re-install (without --purge, see uninstall-on-device.sh) keeps the
# existing one so re-running this script doesn't lock the customer out with a new,
# unannounced password. Either way, re-assert perms every run -- a stale perm from
# an older/buggy install gets fixed by a plain re-install too.
HTPASSWD_FILE=/etc/nginx/.htpasswd-conboard
if [ ! -f "$HTPASSWD_FILE" ]; then
    conboard-password --reset || echo "  WARNING: see above -- nginx will refuse to serve the site (auth_basic_user_file missing) until this is fixed (sudo conboard-password --reset)." >&2
else
    echo "  keeping existing login (sudo conboard-password to see it, sudo conboard-password --reset to change it)."
    NGINX_USER="$(awk '/^[ \t]*user[ \t]+/{print $2; exit}' /etc/nginx/nginx.conf 2>/dev/null | tr -d ';')"
    NGINX_USER="${NGINX_USER:-www-data}"
    NGINX_GROUP="$(id -gn "$NGINX_USER" 2>/dev/null || echo "$NGINX_USER")"
    chown "root:$NGINX_GROUP" "$HTPASSWD_FILE" 2>/dev/null || true
    chmod 640 "$HTPASSWD_FILE"
fi

echo "== installing nginx site (serves the console UI, proxies API + websockets) =="
# The console SPA lives at /conboard/frontend (from the artifact); nginx serves it
# and proxies /api/v1 + /ws to the backend. We do NOT apt-install nginx (see header);
# if it's absent, warn and continue -- the backend still runs headless.
if command -v nginx >/dev/null 2>&1; then
    install -m 644 /conboard/backend/assets/interface.conf /etc/nginx/conf.d/conboard.conf
    # Drop the distro default site if it owns :80 (Debian/Armbian layout).
    rm -f /etc/nginx/sites-enabled/default 2>/dev/null || true
    if nginx -t 2>/dev/null; then
        systemctl reload nginx 2>/dev/null || systemctl restart nginx 2>/dev/null || true
        systemctl enable nginx 2>/dev/null || true
    else
        echo "  WARNING: 'nginx -t' failed -- left nginx untouched. Check /etc/nginx/conf.d/conboard.conf." >&2
    fi
else
    echo "  WARNING: nginx not found -- console UI won't be served. Install nginx and re-run this script." >&2
fi

echo "== enabling services (start on boot) =="
systemctl enable usb-otg.service dispatcher.service launcher.service backend.service conboard-firewall.service
[ -f /etc/systemd/system/usb-gadget-dhcp.service ] && systemctl enable usb-gadget-dhcp.service
# hmi.service (the screen/buttons/encoders UI) is installed but deliberately NOT
# enabled/started here: it needs the physical SPI panel + GPIO wiring to init()
# successfully, which most boards running this installer today don't have yet.
# Once the hardware is wired: `systemctl enable --now hmi.service`.

# Start now — but NOT with `enable --now`. launcher.service is a Type=oneshot
# that scans devices and can call `systemctl restart` for a matched device;
# starting it inside the same systemd transaction that `--now` opens deadlocks
# the two jobs (the installer hangs). Start as plain, separate jobs instead.
echo "== starting services =="
systemctl start conboard-firewall.service   # before opening anything up to the network
systemctl start usb-otg.service       # brings up the USB gadget now (network function best-effort, see usb-gadget-dhcp.service)
[ -f /etc/systemd/system/usb-gadget-dhcp.service ] && systemctl start usb-gadget-dhcp.service
systemctl start dispatcher.service
systemctl start backend.service       # management API (REST/gRPC) over SQLite

# Coldplug at install time: handle devices ALREADY attached now, without a reboot.
# launcher.service does exactly this at boot (replay 'add' uevents once the
# dispatcher + gadget are up). Replay it here too, now that both are started, so a
# fresh install picks up an attached controller without a manual unplug/replug.
echo "== coldplug: replaying attached USB devices =="
udevadm trigger --action=add --subsystem-match=usb || true
udevadm settle || true

echo
echo "Done. Attached devices are handled now; the launcher also runs at boot and on hotplug (udev)."
echo "Quick checks:"
echo "  ls /sys/class/udc            # must be non-empty for USB gadget to bind"
echo "  systemctl status usb-otg.service dispatcher.service launcher.service backend.service"
echo "  systemctl status usb-gadget-dhcp.service     # DHCP for usb0, if this board's USB controller has endpoint room for the network function"
echo "  iptables -L INPUT -n              # confirm the firewall allowlist (ssh/80 only)"
echo "  curl -s localhost:8080/healthz   # backend management API (direct)"
echo "  curl -s localhost/healthz        # via nginx (same origin as the UI)"
echo "  open http://<board-ip>/          # the conboard console UI (will prompt for the web login)"
echo "  sudo conboard-password            # show the web login (--reset to change it)"
echo "  systemctl enable --now hmi.service   # once the SPI screen/buttons/encoders are wired"
