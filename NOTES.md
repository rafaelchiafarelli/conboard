# Direction & scope notes

## Build/packaging wishlist — DONE
* ~~dist folder should have a "how-to-install" with the main commands~~ → `dist/<board>/HOW-TO-INSTALL.txt`
* ~~every dist folder should have a surname saying what board they should go in~~ → `dist/<board>/BOARD.txt`
* ~~adding another build destination should be visible and easy to understand~~ → `docker/boards.conf`
    * ~~list of available compatible boards~~ → `./build-cross.sh list`
    * ~~enable each build separately~~ → `./build-cross.sh <board-id>`

## Generalize input devices beyond MIDI — DONE
The MIDI input→rule→output engine was generalized so joystick/keyboard/mouse (all
evdev) reuse it:
* shared, pure matchers: `midiMap` (MIDI) + `evMatch` (all evdev), unit-tested.
* `holdGen`: synthesizes hold/long-press for non-autorepeating gamepads (pure).
* `DeviceEngine`: shared output/coms/mode engine; `condev::runDevice`: shared main.
* `conJoyS` / `conKeyB` / `conMouse`: all three evdev handlers built on the shared
  `EvdevDevice` engine; `conMIDI` also migrated onto `DeviceEngine`.
* MIDI trigger matching gained **operation modes** (`normal` / `trigger_higher` /
  `trigger_lower` / `spot` / `blink`), plumbed end-to-end: firmware matcher → harpia
  schema → console rule editor.

## Backend + console — DONE (milestone `2026-08-10`)
A C++ management API (`backend/`, harpia-generated REST/gRPC over embedded SQLite)
and a React console (`frontend/console/`) replace the old python-flask stub
entirely — see [README.md § What's built](README.md#whats-built) for the feature
list and [docs/NEXT-SESSION.md](docs/NEXT-SESSION.md) for the live punch list.

## Local screen/buttons/encoders UI (`LowLevel/HMI/`) — IN PROGRESS
A small SPI TFT + 2 push buttons + 2 rotary encoders, wired directly to the board —
phases 1-3 of a 5-phase plan (deps + a screen-size-adaptable base component layer +
a custom dark/amber theme matching the console) are done; phases 4-5 (the actual
WiFi/activation/radio screens, deeper integration) are not started. Deliberately independent of the rest of
`LowLevel/`: own dependencies (LVGL, libgpiod, libcurl), no `libcommon`/
`DeviceEngine`/udev-launcher path, and it sources ALL domain data from new
read-only `backend/src/hmi.cpp` endpoints — no nmcli/system-state reads or other
business logic inside the UI module itself. Real SPI/GPIO bring-up (chip, transfer
mode, init sequence, hardware rotation) is hardware-confirmed on the dev board;
see [docs/NEXT-SESSION.md](docs/NEXT-SESSION.md) for the full handoff, the
hard-won hardware gotchas (esp. why plain `write(2)` to spidev silently no-ops on
this SoC), and the env var reference.

## Basic security hardening — DONE + HARDWARE-VERIFIED (2026-08-12)
Pre-release pass: nginx Basic Auth in front of the console (real per-install
password, generated into `/etc/conboard-web-password.txt` by `conboard-password
--reset` — `install-on-device.sh` calls this same helper for first-boot generation,
and it's left on `PATH` so `sudo conboard-password` / `sudo conboard-password --reset`
always work later too; losing the password is never a real lockout); the dispatcher's `/config`/`/iocommand`/`/screencommand`/`/ws` HTTP server — which had
**no credential check at all** and defaulted to binding all interfaces — now binds
loopback-only, reachable only through nginx's existing same-host proxy; a default-deny
INPUT firewall (`scripts/conboard-firewall.sh` + `conboard-firewall.service`, ssh + :80
allowed, everything else dropped, fails open not closed if it can't apply). None of
this touches the harpia-generated `X-User`/`X-Pswd` gate itself (that's a schema-hash,
not a secret, `backend/README.md`) — regenerating harpia's auth scheme was judged too
risky/out of scope for this pass. The real **power-password** design
(`backend/README.md`) is still unimplemented.

**Verified live on `192.168.7.4`**: confirmed the hole first — `curl
http://192.168.7.4:40080/config` returned the full dispatcher config, unauthenticated,
from an unrelated machine, before any fix was deployed. After deploying: that same
request now hangs/drops (firewall + loopback bind both independently block it);
`ss -tlnp` on the board shows the dispatcher moved from `0.0.0.0:40080` to
`127.0.0.1:40080`; the console at `:80` returns 401 with no credentials, 401 with
wrong ones, 200 with the right ones, and `/healthz` stays open; a raw websocket
upgrade through the authenticated `/websocket` proxy succeeded (`101 Switching
Protocols`) with real `HB,<uuid>,WirelessKB`/`WirelessMouse` heartbeat frames
streaming through, proving the live monitor still works end to end; `iptables -L
INPUT` shows the ssh+80-only allowlist active with the DROP counter incrementing
against the blocked-port probes.

**Two real bugs found only by actually running this on the board** (both fixed,
both re-verified):
- The generated `.htpasswd-conboard` was written `root:root`, but nginx workers run
  as `www-data` — every *authenticated* request 500'd (`open() ... Permission
  denied` in nginx's error log), while unauthenticated requests looked fine (401,
  no file read needed) and would have hidden this behind a false "it's working"
  signal from a shallow test. Fixed: `install-on-device.sh` now chowns the file to
  nginx's actual configured group (auto-detected from `nginx.conf`, defaulting to
  `www-data`), and re-asserts this on every install, not just first-generation.
- `uninstall-on-device.sh --purge` always exited `1` even on a fully successful run
  — classic `[ cond ] && echo ...` as the last line of a `set -e` script, where a
  false condition (`$PURGE -eq 0` when purging) makes the whole script's exit code
  1 regardless of everything above it having worked. Harmless interactively (easy
  to miss the exit code) but would break any automation/provisioning script that
  checks it. Fixed with a proper `if`.

Full purge cycle re-verified after both fixes: install → confirm live devices/`g1`
gadget bound → plain uninstall (data + web login kept, firewall reopened, `g1` fully
torn down not just UDC-unbound) → reinstall (same password, devices + rules DB back)
→ `--purge` (exit 0, `/conboard` + web login + `g1` all gone) → final reinstall to
leave the board in a working state.

## Console bug hunt against real hardware (2026-08-14)
Driven by the user actually using the console against `192.168.7.4` and reporting
what didn't work, live, in the same session:

* **WirelessKB invisible + un-addable — FOUND + FIXED.** `boards/WirelessKB.json` /
  `boards/WirelessMouse.json` (realtime templates, added 2026-08-11) were never
  mirrored into `frontend/console/src/fixtures/boards.ts` (the list that seeds the
  console's DB-backed device library on first load). Result: the launcher auto-
  matched the plugged-in wireless combo and it worked (events flowed), but it had no
  DB row (invisible in the rail) and was already `designated` (so also hidden from
  Add). Fixed by adding both to the fixtures file.
* **A real harpia codegen bug, found chasing the above — FIXED.** The `.harpia` spec
  declares a trigger field `interval`, but the generated protobuf field is actually
  named `erval` (`backend/generated/proto/protofiles/trigger_*.proto`, field 10) —
  looks like the generator stripped "int" out of "interval". The frontend was
  sending `interval`; protobuf JSON silently 400s on an unrecognized field, so
  *every* evdev rule using `mode: "hold"` (which carries `interval`) failed to
  create, with zero detail in the response or backend log. This is exactly why
  WirelessKB alone kept failing to seed even after the fixtures fix — its `KEY_ENTER
  hold` rule tripped it. Root-caused by bisecting field-by-field with raw `curl`
  POSTs against the live backend. Fixed in `api/harpia.ts`/`api/map.ts` (wire name is
  now `erval`, frontend model keeps calling it `interval`). Confirmed via the user's
  own browser auto-seeding `WirelessKB` correctly on next load.
* **Live monitor showed raw `[type,code,value]` triples — FIXED.** Meaningless
  without a lookup table (e.g. a left click was just `[1,272,1]`). Added a small
  decoder (`model/hid.ts`, mirrors the curated symbol table in
  `LowLevel/Common/src/evMatch.cpp`) plus reuse of the existing MIDI decoder
  (`model/midi.ts`) so the monitor now shows `BTN_LEFT press` / MIDI note names
  instead of raw numbers.
* **Live monitor truncation — FOUND + FIXED.** `.ev-human`/`.ev-dev` had
  `white-space: nowrap; overflow: hidden; text-overflow: ellipsis`, silently cutting
  decoded event text off with no visible indicator (user report: "not showing the
  full event", saw just a truncated UUID). Removed the truncation; rows now wrap
  instead of hiding content.
* **Two `window.alert()` calls replaced with a proper toast.** Copy/delete failures
  used to pop a native browser dialog, inconsistent with the rest of the app's
  styling; now a small dismissible banner styled from the same `--danger` tokens the
  crash screen uses.

All of the above rebuilt + redeployed to `192.168.7.4` and verified against the real
board multiple times this session (not just typechecked).

## OPEN — mouse/keyboard rule output not firing (2026-08-14, unresolved)
User report: WirelessKB and WirelessMouse both register live events (visible in the
monitor, dispatcher heartbeats present), but pressing a key / clicking a button
produces **no output on the connected host** — even for the simplest rules
(`BTN_LEFT press → type "left click"`).

**Ruled out this session:**
- The deployed profile is correct (`/conboard/boards/WirelessMouse.json` matches the
  repo template exactly after a fresh reinstall).
- The USB gadget is fully bound: `cat /sys/class/udc/*/state` → `configured`,
  `/dev/hidg0` exists with the right permissions.
- The HID write path itself works: manually wrote a raw boot-keyboard report for
  `letter_a` straight to `/dev/hidg0` over SSH (bypassing conMouse/conKeyB
  entirely) — **user confirmed "a" appeared** on the connected host. So the
  gadget→host link is healthy.
- Both `WirelessKB-port-4-1.service` and `WirelessMouse-port-4-1.service` show a
  valid, positive fd (`file: /dev/hidg0 fd:3`) at startup in the journal — not the
  "open() failed silently, fd stays -1" theory.

**Not yet confirmed — this is where to start next session:** whether the *matching*
even fires. `DeviceEngine::report()` sends every observed input event to the
dispatcher for the live monitor regardless of whether any rule matches it (that's
just visibility), which is a SEPARATE code path from the local
`evMatch::matches()` → `enqueue()` → `out_func()` → `oActions::keyboard_send()` chain
that actually produces the output. Seeing an event in the monitor does NOT prove a
rule matched. The monitor's new raw-event decoder (this session) should make it easy
to confirm the exact code names the dispatcher is receiving (`KEY_A press` /
`BTN_LEFT press` — or something else, which would point at a code mismatch instead)
— this check was requested but not completed before the session ended.

**A concrete lead worth checking, found reading the code (not yet proven):**
`DeviceEngine::enqueue()` (`LowLevel/Common/src/deviceEngine.cpp:67`) locks
`locking_mechanism` before touching `oQueue`/`send`, but `out_func()` (the executor
thread, line 104) reads and mutates both **without the same lock** — a genuine data
race on non-atomic state shared between two threads. Whether this actually explains
"enqueued output never gets executed" (vs. just being latent-but-currently-harmless
UB) is unconfirmed. Worth instrumenting or fixing regardless (make `send` atomic or
have `out_func()` take the lock) even if it isn't the root cause here.

## OPEN — live monitor layout, not investigated (2026-08-14)
User: "it is ugly" and the live monitor panel can't be resized. Not looked at this
session (deferred). Likely CSS/layout only (`.live-col`, `.monitor` in
`frontend/console/src/index.css`) — the panel is currently a fixed-width aside with
no resize handle.

## Next (pre-release cleanup, 2026-08-12)
* HARDWARE TEST `conJoyS` (joystick) — built + unit-tested, keyboard/mouse already
  hardware-verified (2026-08-11), no gamepad available yet. See `docs/HW-TEST-evdev.md`.
* load-test the reporting-queue overflow relief (`STACKED_IO_MSG`, raised 10→64 +
  drop-oldest, deployed 2026-08-12) against a real sustained burst — not yet exercised
  under load, only deployed.
* **mouse/keyboard output not firing** (above) — now the top item, blocks the core
  remap-a-device feature for two of four device kinds.
* longer term: ethernet-gadget access, the local power-password login (design in
  `backend/README.md`, never implemented).
