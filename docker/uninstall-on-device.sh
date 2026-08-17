#!/usr/bin/env bash
#
# Uninstall conboard from the Orange Pi (run on the board, as root). Removes the
# systemd units (including RETIRED ones from older installs), the udev rule, and the
# /conboard program tree. By default it PRESERVES the rules-library data
# (/conboard/backend/data); pass --purge to remove that too.
#
#   sudo ./uninstall-on-device.sh            # remove program + services, keep data
#   sudo ./uninstall-on-device.sh --purge    # also delete the rules DB + all data
set -euo pipefail

if [ "$EUID" -ne 0 ]; then
    echo "Please run as root (sudo ./uninstall-on-device.sh)" >&2
    exit 1
fi

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

PURGE=0
[ "${1:-}" = "--purge" ] && PURGE=1

# Every unit conboard has ever shipped, current + retired (frontend = old python UI;
# backend covers both the retired python and the current C++ one).
UNITS=(usb-otg usb-gadget-dhcp launcher dispatcher backend hmi frontend conboard-firewall)

echo "== stopping + disabling services =="
for u in "${UNITS[@]}"; do
    systemctl stop    "${u}.service" 2>/dev/null || true
    systemctl disable "${u}.service" 2>/dev/null || true
done

echo "== removing systemd unit files =="
for u in "${UNITS[@]}"; do
    rm -f "/etc/systemd/system/${u}.service"
done
# Also remove the launcher's auto-generated per-device units (one per matched
# device). Nothing else tracks these, so without this they linger forever and a
# stale one breaks the next install's handler for that device.
for f in /etc/systemd/system/*.service; do
    [ -e "$f" ] || continue
    if grep -q "auto generated service file" "$f" 2>/dev/null; then
        u="$(basename "$f")"
        systemctl stop "$u" 2>/dev/null || true
        systemctl disable "$u" 2>/dev/null || true
        rm -f "$f"
        echo "   removed auto-generated $u"
    fi
done
systemctl daemon-reload
systemctl reset-failed 2>/dev/null || true

echo "== removing udev rule + reloading =="
rm -f /etc/udev/rules.d/100-usb.rules
udevadm control --reload-rules 2>/dev/null || true

echo "== reopening the firewall (conboard-firewall.service is gone) =="
# The allowlist existed only to protect conboard's own services; leaving INPUT
# locked down to ssh+80 after conboard itself is removed would be a surprise for
# whatever the board is used for next.
command -v iptables >/dev/null 2>&1 && { iptables -P INPUT ACCEPT; iptables -F INPUT; } || true

echo "== removing nginx site + reloading =="
if [ -f /etc/nginx/conf.d/conboard.conf ]; then
    rm -f /etc/nginx/conf.d/conboard.conf
    command -v nginx >/dev/null 2>&1 && { nginx -t 2>/dev/null && systemctl reload nginx 2>/dev/null || true; }
fi
# The generated web-login (install-on-device.sh) is host config, not app data --
# kept across a plain uninstall (so a re-install doesn't hand out a new,
# unannounced password) but wiped on --purge like everything else.
if [ "$PURGE" -eq 1 ]; then
    rm -f /etc/nginx/.htpasswd-conboard /etc/conboard-web-password.txt /usr/local/bin/conboard-password
fi

# Remove a stale global libcommon entry from older installs (pre-bundled-lib scheme),
# which used to install libcommon.so into /usr/local/lib + ldconfig.
if [ -f /usr/local/lib/libcommon.so ]; then
    echo "== removing legacy /usr/local/lib/libcommon.so =="
    rm -f /usr/local/lib/libcommon.so
    ldconfig 2>/dev/null || true
fi
rm -f /etc/ld.so.conf.d/conboard.conf 2>/dev/null || true

# Tear down any live USB gadget so the port is released (best-effort). Run BEFORE
# `rm -rf /conboard` below, while scripts/usb-gadget-stop.sh (bundled in this same
# artifact tree, see docker/package.sh) is still on disk -- it's the real teardown
# (unbind UDC, rm the configfs symlinks, rmdir functions/configs/strings/g1 itself),
# not just an unbind. A plain UDC-unbind alone left the g1 configfs tree behind,
# which is what previously made --purge not a true clean slate.
if [ -d /sys/kernel/config/usb_gadget/g1 ]; then
    echo "== tearing down USB gadget g1 =="
    if [ -x "$HERE/scripts/usb-gadget-stop.sh" ]; then
        "$HERE/scripts/usb-gadget-stop.sh" || true
    else
        echo "  WARNING: scripts/usb-gadget-stop.sh missing -- falling back to a UDC-only unbind." >&2
        echo "" > /sys/kernel/config/usb_gadget/g1/UDC 2>/dev/null || true
    fi
fi

echo "== removing program tree =="
if [ "$PURGE" -eq 1 ]; then
    rm -rf /conboard
    echo "   removed /conboard entirely (--purge: rules DB + data deleted)."
else
    # Preserve /conboard/backend/data (the rules library).
    if [ -d /conboard ]; then
        find /conboard -mindepth 1 -maxdepth 1 ! -name backend -exec rm -rf {} +
        if [ -d /conboard/backend ]; then
            find /conboard/backend -mindepth 1 -maxdepth 1 ! -name data -exec rm -rf {} +
        fi
        echo "   removed /conboard but KEPT /conboard/backend/data (use --purge to delete)."
    fi
fi

echo
echo "Done. conboard services + program removed."
if [ "$PURGE" -eq 0 ]; then
    echo "Rules-library data kept at /conboard/backend/data (re-install restores it)."
fi
