# Session 8 — ethernet-gadget access (USB-C network interface to the board)

**Scope: medium. Needs the board and, critically, a Windows host to verify the
RNDIS side — the biggest risk in this session is Windows composite-gadget
enumeration, not the Linux-side plumbing.**

**Handoff note (2026-08-16): this doc was originally written, then
deprioritized in favor of two MIDI sessions (SysEx support + identical-MIDI-
device separation, both landed on branch `feat/midi-sysex`), then picked back
up for a dedicated fresh session. Nothing about this doc's research is stale —
`scripts/usb-composite-all.sh`, `scripts/usb-gadget-stop.sh`,
`docs/dev-snippets/rndis-ecm-adm.sh`, `docker/boards.conf`,
`scripts/conboard-firewall.sh`, and `backend/assets/interface.conf` are all
byte-identical to when this was first researched. `main` itself hasn't moved
this whole cycle (`e1ac765`) — there are two other unmerged, pushed branches
(`feat/hmi-phase4a-and-1to1-rules`, `feat/midi-sysex`) sitting independent of
this work, both pending their own hardware verification before merging. This
session should branch fresh off `main`, same as those did — no dependency
between them.**

## Why

Flagged as missing since the original `README.md` "What is Missing?" list:
*"missing installing the system as an ethernet port (it would simplify access
by users)"* — also carried in `NOTES.md`'s longer-term list. Today, reaching
the console for the first time requires the board already be on a network
(WiFi via `nmcli`, see `backend/src/hmi.cpp`'s `wifi/networks` endpoint) —
there's no zero-config path. Plugging the board into a host over USB-C and
getting a network interface for free (like a Pi Zero in "USB gadget mode")
removes that bootstrap step entirely: plug in, get an IP, open the console.

## Decision made 2026-08-16: dual config, RNDIS + ECM

Chose **dual config — RNDIS (Windows-friendly) + ECM (Linux/macOS-native),
host OS picks whichever it understands** — over RNDIS-only or ECM-only. Best
compatibility across all three OSes at the cost of the fiddliest Windows
enumeration path. This directly follows the pattern already scoped out in
`docs/dev-snippets/rndis-ecm-adm.sh` (an existing reference snippet, not
conboard's own code — a HackPi/wismna script kept in the repo for exactly
this feature).

## Start here — exact facts, no need to re-read the project

- **The ECM function already exists, half-wired, in `scripts/usb-composite-all.sh`**:
  `functions/ecm.$N` is created with `host_addr`/`dev_addr` set (lines 57,
  63–64), but it is **never linked into the config** — line 84,
  `#ln -s functions/ecm.$N configs/c.$C/`, is commented out. The `ifconfig
  $N ...` lines (98–99) that would bring the interface up are commented too.
  This is the closest thing to a half-finished feature in the codebase —
  don't rebuild the ECM function from scratch, just finish wiring what's
  there.
- **Reference for the RNDIS side**: `docs/dev-snippets/rndis-ecm-adm.sh`
  builds a real two-config gadget (`configs/c.1` = RNDIS with an `os_desc`
  Microsoft OS descriptor block for zero-driver Windows auto-install,
  `configs/c.2` = ECM) — read it, don't copy it blindly (it's an external
  script targeting a different board/idVendor, and it doesn't include HID/
  mass-storage/ACM at all). The `os_desc/interface.rndis/compatible_id` +
  `qw_sign = "MSFT100"` dance is the part worth lifting verbatim — that's
  what makes Windows install its inbox RNDIS driver without a `.inf` prompt.
- **Current composite gadget shape** (`scripts/usb-composite-all.sh`, config
  `c.1` today): `acm.gs0` (serial) + `mass_storage.usb0` + `hid.usb0`
  (keyboard-only HID, see the mouse/joystick-passthrough gap in `NOTES.md`),
  bound to the device class `0xEF/0x02/0x01` (Interface Association
  Descriptor) specifically so Windows enumerates the composite device
  correctly for the ACM function — a hard-won fix already in the script,
  don't remove it. **Open question to resolve early**: whether HID/
  mass_storage/ACM should be linked into *both* new configs (a configfs
  function instance can be symlinked into more than one config — verify this
  actually works as expected on this kernel before assuming it) so the
  keyboard/mouse passthrough and serial console work regardless of which
  network config the host negotiates, or whether that's overengineering for
  v1 and they should just stay in whichever config is more likely to be
  picked (probably RNDIS, since Windows can't use ECM at all).
- **Launched by**: `LowLevel/assets/usb-otg.service`
  (`ExecStart=/conboard/scripts/usb-composite-all.sh`), enabled + started by
  `install-on-device.sh`. Torn down by `scripts/usb-gadget-stop.sh`, called
  from `docker/uninstall-on-device.sh` — that teardown script is generic
  (`ls -d functions/* | xargs rmdir`, `ls -d configs/* | xargs rmdir`) so it
  should handle new function/config names without changes, but verify on
  first real test rather than assuming.
- **No DHCP-server config exists yet for a gadget interface** — nothing in
  the repo sets up `usb0`'s IP or hands the host a lease. This is new work,
  not a half-built piece like the ECM function. Common pattern for USB
  gadget-ethernet (same as Pi Zero tutorials): static IP on the board's side
  of `usb0` (e.g. `10.55.0.1/24`), a `dnsmasq` instance scoped **only** to
  `usb0` (`--interface=usb0 --bind-interfaces`, one-address DHCP range) so it
  can't leak onto the board's real WiFi/ethernet uplink and start handing out
  leases there.
- **Firewall needs no changes.** `scripts/conboard-firewall.sh` allows
  ssh/:80 with no `-i <iface>` restriction — any interface, including a
  future `usb0`, is already covered. Confirmed by reading the script, not
  yet tested against a real `usb0` interface.
- **nginx needs no changes.** `backend/assets/interface.conf` listens on
  `0.0.0.0:80 default_server` — already reachable over any interface with an
  IP, `usb0` included.
- **Test board**: `zero3` (Orange Pi Zero 3, H618) — the only board marked
  "verified working" for USB-OTG in `docker/boards.conf`, and the one with
  hardware in hand. Don't worry about `zero`/`rpi64`/`rpi32` this session.
- **A Windows host is the actual RNDIS-enumeration test.** Plugging the
  board's USB-C into any Windows machine you have access to is what proves
  (or disproves) the driver-free-install claim — no special lab setup beyond
  that needed.

## Task

1. **Land ECM first, alone, and prove the whole path end-to-end on Linux**
   before touching RNDIS — it's lower-risk and the function scaffolding
   already exists:
   - Uncomment/finish the `ecm.$N` link into a config.
   - Bring `usb0` up with a static IP on the board side.
   - Add the scoped `dnsmasq` (or equivalent) instance, wired into the
     service lifecycle (own systemd unit, or folded into `usb-otg.service`'s
     script — match existing project conventions of one small unit per
     concern).
   - Verify from a Linux/macOS host: plug in, interface appears, DHCP lease
     received, `curl` the console over the new IP.
2. **Add the RNDIS config alongside it**, following
   `docs/dev-snippets/rndis-ecm-adm.sh`'s `os_desc`/MSFT100 pattern for
   driver-free Windows install. Resolve the "share HID/ACM/mass-storage
   across both configs, or not" question from above during this step, once
   you can actually test both paths.
3. **Hardware-verify on the Windows host**: plug in, confirm Windows
   auto-installs the RNDIS adapter with no manual driver step, confirm the
   existing HID passthrough (keyboard) and ACM serial still enumerate
   correctly alongside the network function — this is the step most likely
   to surface a real bug (composite descriptor conflicts, IAD class
   confusion between the existing ACM fix and the new RNDIS OS descriptor).
4. **Teardown verification**: run a full uninstall/reinstall cycle
   (`uninstall-on-device.sh` → `install-on-device.sh`) with the gadget
   active, confirm `usb-gadget-stop.sh` cleans up the new functions/configs
   and the DHCP daemon stops, same discipline as every other hardware
   session in this project (`docs/NEXT-SESSION.md` has several examples of
   this exact cycle catching real teardown bugs).
5. Update `README.md`'s "What is Missing?" list and `NOTES.md` once this
   lands — don't leave the "missing ethernet-gadget access" line stale.

## Done criteria

- Plugging the board into a fresh host (no WiFi configured yet) via USB-C
  gets that host a network interface + IP with no manual driver install on
  Windows, and the console is reachable at the board's `usb0` IP.
- Same proven on at least one non-Windows host (Linux or macOS) via ECM.
- Existing HID keyboard passthrough and ACM serial still work with the
  gadget's composite descriptor changed — regression-checked, not assumed.
- Full uninstall/reinstall cycle clean (no wedged gadget state, no leaked
  `dnsmasq` process).
- `README.md` / `NOTES.md` updated to reflect the feature landing.
