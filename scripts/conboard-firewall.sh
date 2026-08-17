#!/usr/bin/env bash
#
# Minimal INPUT-chain firewall for a conboard appliance. Run at boot by
# conboard-firewall.service. Idempotent (safe to re-run): rebuilds the same
# rule set from scratch every time rather than layering onto whatever was
# there before.
#
# Rationale: with the dispatcher now bound to 127.0.0.1 (see
# LowLevel/dispatcher/src/main.cpp) and the backend already defaulting there
# (CONBOARD_HTTP_HOST), the only service meant to be reachable from the
# network is nginx on :80 -- everything else conboard runs is loopback-only
# and proxied through it. This just makes that boundary explicit at the
# packet-filter level too, in case a future change forgets that assumption.
#
# Deliberately conservative about lockout: this only ever manipulates the
# INPUT chain (never OUTPUT/FORWARD, never touches other services' rules
# beyond that), and the "deny by default" behavior is a trailing DROP *rule*,
# not the chain's default *policy* -- so if this script dies partway through
# (missing iptables, a typo, whatever), the failure mode is "no extra rules
# yet" (fails open), not "everything blocked including SSH" (fails closed).
set -euo pipefail

command -v iptables >/dev/null 2>&1 || { echo "conboard-firewall: iptables not found, skipping." >&2; exit 0; }

# SSH port: respect sshd_config if it's been changed from the default, so this
# never locks out an admin who already moved SSH off :22.
SSH_PORT="$(awk '/^[Pp]ort[ \t]+[0-9]+/{print $2; exit}' /etc/ssh/sshd_config 2>/dev/null || true)"
SSH_PORT="${SSH_PORT:-22}"

iptables -P INPUT ACCEPT      # reset to a known-safe state before rebuilding
iptables -F INPUT

iptables -A INPUT -i lo -j ACCEPT
iptables -A INPUT -m conntrack --ctstate ESTABLISHED,RELATED -j ACCEPT
iptables -A INPUT -p tcp --dport "$SSH_PORT" -j ACCEPT   # remote admin access
iptables -A INPUT -p tcp --dport 80 -j ACCEPT             # the console (nginx)
iptables -A INPUT -p icmp -j ACCEPT                       # ping, for diagnostics
# DHCP for the USB-ethernet gadget (usb-gadget-dhcp.service, only runs at all
# when usb-composite-all.sh's endpoint-budget probe found room for it -- see
# NOTES.md "Ethernet-gadget access"): a DHCPDISCOVER arrives on udp/67 from a
# host with no address yet, so it can't be matched by an established/related
# rule -- needs its own ACCEPT. Scoped to usb0 only, so this can't be used to
# reach the DHCP server over WiFi/ethernet. The -i match is by name, not by an
# interface that must already exist -- this firewall script runs before
# usb-otg.service brings usb0 up (see install-on-device.sh), so the rule has
# to be in place ahead of the interface, not conditional on it.
iptables -A INPUT -i usb0 -p udp --dport 67 -j ACCEPT
iptables -A INPUT -j DROP                                 # everything else

echo "conboard-firewall: applied (ssh=$SSH_PORT, http=80, else dropped)."
