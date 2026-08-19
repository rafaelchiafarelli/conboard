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

## RESOLVED — mouse/keyboard rule output not firing (found + fixed + hardware-verified 2026-08-15)
User report (2026-08-14): WirelessKB and WirelessMouse both register live events
(visible in the monitor, dispatcher heartbeats present), but pressing a key /
clicking a button produced **no output on the connected host** — even for the
simplest rules (`BTN_LEFT press → type "left click"`).

**Root cause, found by static read + live confirmation:** `jsonParser::parseIO()`
(`LowLevel/Common/src/jsonParser.cpp`) is reused to parse BOTH a rule's INPUT
trigger (`{"type":"mouse","code":"BTN_LEFT","mode":"press"}`) and its OUTPUT action
(`{"type":"keyboard","data":"left click",...}`), dispatching on the `"type"` string
alone. Only the `case devType::joystick` branch ever called
`evmatch::resolveSymbol()` to populate `evtrig` from `code`/`mode` — the `mouse` and
`keyboard` branches only parsed output-shaped fields (`dx`/`dy`/... , `data`/
`keyType`/...) and never touched `evtrig` at all. So for every keyboard/mouse rule,
`evtrig.mode` stayed at its default `ev_nomode`, and `evmatch::matches()`
(`LowLevel/Common/src/evMatch.cpp:94`) returned `false` unconditionally, for every
event, always. Joystick "worked" (structurally — never hardware-tested) purely by
being the first evdev consumer written; keyboard/mouse support was added later and
the trigger-parsing path was never extended to them.

**Proven live before the fix** (not just read from code): built a small inotify-based
watcher (no `strace`/`inotifywait` on-device, wrote one in ~30 lines of python3
ctypes) on `/dev/input/event0` (keyboard), `/dev/input/event1` (mouse), and
`/dev/hidg0` simultaneously. Result over one interaction window: **276 real keyboard
events + 1030 real mouse events read by the handlers, zero writes to `/dev/hidg0`.**
Confirmed the watcher itself works by manually writing a raw HID report to
`/dev/hidg0` first (immediate `MODIFY` event) before trusting the "zero events"
read on the real handlers.

**Fix**: extracted the trigger-parsing block (resolveSymbol + mode/threshold/holdMs)
into a shared `parseEvTrigger()` helper, called from all three evdev branches
(`joystick`/`keyboard`/`mouse`) instead of only `joystick`. Guarded on
`act.HasMember("code")` so it's a no-op when parseIO is called on an OUTPUT object
(which never has `code`).

**Verified after the fix**: all 83 existing unit tests still pass (none of them
covered this path — the gap wasn't test-visible). Cross-built, redeployed via a real
`install-on-device.sh` reinstall on `192.168.7.4`, then re-ran the same live watch:
**51 keyboard events + 87 mouse events → 612 writes to `/dev/hidg0`**, user-confirmed
the expected text actually appeared on the connected host.

**Follow-up — FIXED + HARDWARE-VERIFIED (2026-08-15, same day).**
`DeviceEngine::enqueue()` (`LowLevel/Common/src/deviceEngine.cpp:67`) locked
`locking_mechanism` before pushing to `oQueue`, but `out_func()` (line 104) read/
popped the same `oQueue` **without** that lock — `send` itself is
`std::atomic_bool` so that part was safe, but `std::queue` is not thread-safe for
concurrent push/pop regardless. Not the cause of the bug above (matching never
fired, so nothing reached the queue concurrently under real load), but a genuine
latent race. Fixed by taking `locking_mechanism` around the front/pop check in
`out_func()` too, kept scoped tightly (no `executeOutput()` inside the lock) so a
slow/delayed output can't stall `enqueue()` from the reader thread. Rebuilt,
redeployed, reverified live: 267 real input events → 628 `/dev/hidg0` writes,
output still firing correctly with the fix in place.

**Regression coverage added (2026-08-15, session 1 of post-v1 roadmap):** added
keyboard press/hold, mouse button/wheel-axis, and combined input-trigger +
output-data cases to `tests/test_jsonparser.cpp`'s `TEST_SUITE("json")`, so this
exact class of bug (an evdev branch parsing output fields but never calling
`parseEvTrigger()`) fails a unit test immediately instead of shipping silently.
Full suite: 83 → 86 cases, 259 assertions, all green
(`./run-tests.sh json` / `./run-tests.sh`). Logic-only, no hardware needed.
`docs/next-sessions/01-regression-test.md` removed now that its task is done.

## CONFIRMED — reporting-queue overflow relief load-tested (2026-08-15)
`STACKED_IO_MSG` (10→64 + drop-oldest, deployed 2026-08-12, see `INTERFACE.md` §O4)
was load-tested against a real sustained burst instead of just shipped. Used
continuous mouse motion on `WirelessMouse-port-4-1.service` (physical hardware,
same board/session as the keyboard/mouse trigger-parsing fix) — deliberate
fast/sustained shaking produced a peak of ~65 dispatch attempts/sec for ~8s
(523 `IOwill wait` log lines in that window, `zmq_coms.cpp:96`), well above
idle heartbeat rate.

Result: **no overflow triggered** — `dispatch overflow (reporting queue full,
dropping oldest)` (`deviceEngine.cpp:58`) never logged, service stayed `active`
throughout and after. The synchronous ZMQ REQ/REP drain in `io_handler()` keeps
pace with realistic mouse burst rates, so `io_queue` (`zmq_coms.cpp:136`) never
reaches the 64-message ceiling under normal use: the relief works, and real
usage doesn't get close to needing it. Drop-oldest eviction behavior itself
(queue actually at capacity) remains unexercised — would need either an
artificial producer or a much heavier burst to actually fill 64 slots.

## RESOLVED — live monitor layout ("it is ugly" + can't be resized), hardware-verified (2026-08-15)
Two separate bugs stacked on top of each other, both found by testing live in a
browser rather than reading the CSS in isolation (a throwaway mock WebSocket
dispatcher + Playwright, driven against both the dev server and the real board,
confirmed each fix before it shipped).

**The actual "ugly" bug wasn't spacing — it was overlapping text.**
`frontend/console/src/index.css`'s `.feed-head, .ev-row` grid (`108px 128px 1fr
120px minmax(150px, 0.9fr)`, 500px+ of fixed columns) predates the panel's current
width (400px default, resizable 280–768px) — leftover from before the panel was
narrowed to a permanent side column. The Aug-14 truncation fix (`f7d7d41`) added
`min-width: 0` to `.ev-event` so its content could shrink, but with no space left
after the fixed columns, that `1fr` track collapsed to ~0px and its flex children
(the RAW badge + decoded event text) ink-overflowed straight into the UUID column's
own text — genuinely overlapping, illegible glyphs, confirmed by measuring the
actual overlapping DOM rects in a real browser, not just "looks cramped." Fixed by
switching every column to `minmax(min, max)` (columns shrink together instead of
one blowing out) and adding `min-width: 0` to the remaining leaf cells; dropped
`.live-col .feed-scroll { overflow-x: auto }` (it let rows scroll independently of
the non-scrolling header, which is how the header ended up silently clipped) in
favor of wrapping.

**Resize was never actually broken — the first fix addressed the wrong half.**
Live-dragging `.live-col`'s existing `resize: horizontal` in a real browser proved
the CSS property itself worked and held through a live event stream; the real
problem was discoverability, so the first pass added a small drawn grip in the
browser's native corner. **User spot-check on the real board found this
insufficient** — still felt unresizable, no cursor feedback on hover. Root cause:
CSS `resize`'s handle is spec-fixed to an element's bottom-right corner, always,
which for a panel docked on the *right* (bordered on its *left*) is the wrong
corner entirely — nothing lived on the edge users actually reach for. Fixed
(`frontend/console/src/App.tsx`) with a real JS-driven drag handle on the left edge
(`mousedown` starts a `window`-level `mousemove`/`mouseup` drag, same 280px/48vw
clamp as the CSS) and `cursor: ew-resize` visible across the whole edge; the native
corner resize stays as a secondary affordance.

**Verified**: `npm run typecheck` clean; live in a real browser against a mock
dispatcher (drag-resize holds through a live event stream, no overlap from the
panel's 280px floor through its 48vw ceiling, cursor confirmed `ew-resize` over the
new edge); redeployed to `192.168.7.4` twice (once per round, previous build backed
up on-device before each overwrite) and user-confirmed live both times — the second
specifically checking hover cursor + edge-drag. `docs/next-sessions/02-live-monitor-css.md`
removed now that its task is done.

## Synthetic 1:1 keyboard rules on hotplug — DONE + HARDWARE-VERIFIED (2026-08-15)
Idea sized 2026-08-11 (see `docs/NEXT-SESSION.md`), built and verified same-day
2026-08-15. A "Seed a full 1:1 rule set" checkbox in the console's Add-Device
dialog (keyboard type only) generates one press-triggered rule per key across the
standard keyboard layout (`frontend/console/src/model/oneToOneKeyboard.ts`, 105
`KEY_*` entries), riding the existing create+deploy pipeline unchanged — no new
backend/CRUD code, since a single nested `POST /api/v1/board` already cascades
`modes`/`rules`/`triggers`/`output_actions`.

Required extending `LowLevel/Common/src/evMatch.cpp`'s `kSymbols` from 52 to 105
`KEY_*` entries so the newly-generated symbols actually *resolve* at deploy/
runtime, not just get stored in the DB — the rules DB is only an authoring
library; the running engine reads `boards/*.json` on deploy and resolves each
trigger's `code` through `kSymbols` at load time, so an unresolved symbol
silently no-ops (same failure shape as the mouse/keyboard parsing bug above).
Along the way, discovered `kSymbols` actually lives in a **shared library**
(`/conboard/lib/libcommon.so`), not statically in each per-device executable —
`conKeyB` itself was byte-identical before/after the fix; only `libcommon.so`
changed. Useful to know for any future `LowLevel/Common/` fix: the deployable
unit is that one shared object.

**Hardware-verified on `192.168.7.4`**: cross-built via `./build-cross.sh zero3`,
backed up the board's existing `libcommon.so` and its `WirelessKB.json` test
fixture, deployed the rebuilt library + the actual 105-rule generated board JSON
via a direct `POST /api/v1/deploy` (same call the console makes). Watched the
dispatcher's live event feed while physically pressing keys on the real wireless
keyboard: `KEY_MINUS` and `KEY_F1` — both newly resolvable this session, unlike
`KEY_A` which already worked before the fix — produced correct HID output.
Board restored to its exact pre-session state afterward (fixture put back,
diffed byte-identical); kept the rebuilt `libcommon.so` since it's a strict
superset of the old symbol table.

Also browser-verified the checkbox itself with headless Chromium (Playwright) —
absent for non-keyboard types, present and correctly wired for keyboard, no
console errors. That required one-time WSL2 environment setup (native Node 20+
via `nvm`, since the OS-packaged Node is too old for current Playwright, plus
Playwright's Chromium + its apt deps), now captured in `.claude/skills/run/
SKILL.md` so it isn't rediscovered next session — includes a real gotcha hit
along the way: `node_modules` installed under Windows' `node.exe` (also on
`PATH` in this WSL setup) pulls the wrong-platform native Rollup binary and
breaks `vite`.

Deliberately excludes true mouse/joystick 1:1 HID **passthrough** (the bigger
idea that prompted this one) — checked 2026-08-15 and still accurate:
`oActions::mouse_fill_report`/`joystick_fill_report` already build correct
report bytes, but `oMouse()`/`oJoystick()` (`LowLevel/Common/include/
oActions.hpp`) are literal empty stubs, and the USB gadget composite
(`scripts/usb-composite-all.sh`) only declares one HID interface, hardcoded as
a keyboard — no mouse/joystick HID function exists in the gadget at all. Sized
medium (new gadget HID function + wiring + real OTG-to-host-PC verification),
not attempted. `docs/next-sessions/04-synthetic-1to1-rules.md` removed now
that its task is done.

## Ethernet-gadget access — blocked on zero3, RESUMED with an auto-fallback (2026-08-16)
Picked up the long-standing "install as an ethernet port" item (`README.md` "What
is Missing?", carried in this file's Next list below). Plan: dual-config USB
gadget, RNDIS (Windows) + ECM (Linux/macOS), added alongside the existing ACM
serial + HID keyboard + mass-storage functions, following the pattern already
scoped out in `docs/dev-snippets/rndis-ecm-adm.sh`. Branched
`feat/ethernet-gadget-ecm` off `main`, wired ECM into
`scripts/usb-composite-all.sh` (uncommenting the already-scaffolded `ecm.$N`
function link), added a static IP + a `dnsmasq` instance scoped to `usb0` +
a `udp/67` firewall allow (the original plan doc assumed no firewall change was
needed — wrong: a DHCPDISCOVER arrives from a host with no address yet, so it
can't match the existing established/related rule and needs its own ACCEPT).

**Hardware-verified blocker, live on `192.168.7.4` (Orange Pi Zero 3, Allwinner
H618, musb-hdrc USB controller):** adding ECM to the existing composite gadget
failed the UDC bind outright — kernel log showed `unable to autoconfigure all
endpoints` / `failed to start g1: -524`. Isolated by testing combinations
directly against configfs: ACM+ECM alone binds fine (ACM 2 IN + 1 OUT, ECM 2 IN
+ 1 OUT = 4 IN + 2 OUT total) — and that's exactly the same endpoint total the
*current* working gadget already uses (ACM 2 IN + 1 OUT, HID 1 IN, mass-storage
1 IN + 1 OUT = 4 IN + 2 OUT). This SoC's musb-hdrc controller has hardware
budget for exactly 4 IN + 2 OUT endpoints and no more. Adding HID on top of
ACM+ECM (a 5th IN endpoint) fails immediately. So on this board, any USB config
carrying the network function can only also carry ACM serial — never HID
keyboard passthrough or mass-storage at the same time. This is a silicon limit,
not a config/driver bug — not caught by the original plan doc's research because
neither `docs/dev-snippets/rndis-ecm-adm.sh` (a different board) nor the
half-wired `usb-composite-all.sh` scaffolding had ever exercised the endpoint
budget.

Presented the finding and two workaround shapes (split into two USB configs
with no shared functions, or drop mass-storage permanently to free budget for
HID+ECM+ACM together). Initial call was to shelve rather than redesign v1
around a keyboard/network tradeoff on this board — all code changes reverted
(board restored to the known-good ACM+HID+mass-storage gadget, confirmed
`configured` again), `docs/next-sessions/08-ethernet-gadget.md` dropped.

**Candidate boards for a future retarget** (researched 2026-08-16, not yet
hardware-verified — see Sources below for where each claim comes from):
- **Raspberry Pi Zero / Zero W / Zero 2 W** (BCM2835/2710/2837, `dwc2`
  controller, 8 usable endpoints) — best-documented exact match: the P4wnP1
  toolkit runs RNDIS/ECM + HID + mass-storage together on exactly this
  hardware. Pragmatic first pick if this gets retargeted.
- **Raspberry Pi 4** — same `dwc2` family via its USB-C OTG port. (Pi 5 uses a
  different chip, RP1, for USB — not confirmed either way.)
- **USB Armory Mk II** (NXP i.MX6ULZ, ChipIdea controller) — worth calling out
  specifically: `docs/dev-snippets/rndis-ecm-adm.sh`'s header traces back to
  `ckuethe/usbarmory/wiki/USB-Gadgets`, i.e. conboard's *own* reference script
  originates from this board. ChipIdea's stock `g_multi` gadget ships
  RNDIS+ACM+mass-storage by default; other reports show HID+mass-storage
  combos working too. No hard endpoint-count source found for it.
- **BeagleBone Black / AI** (TI AM335x, TI's own `musb` glue — not the same
  integration as Allwinner's) — HID, RNDIS, ACM, ECM, and mass-storage
  confirmed "demonstrated working properly in multiple composite arrangements."
- **Rockchip boards with a `dwc3` OTG controller** (RK3399/RK3568/RK3588-class
  — Rock Pi 4, several Radxa/Orange Pi/NanoPi Rockchip models) — confirmed 7
  IN + 6 OUT = 13 usable endpoints on RK3399 specifically. Massive headroom;
  would fit every function conboard ships simultaneously with room to spare.

**Avoid**: other Allwinner H-series boards (H3, H616, H618 — other Orange Pi
Zero variants) — same constrained `musb-hdrc` glue. An independent report on
the H3 specifically found it fails past 2 composite functions, same failure
shape as zero3. (Allwinner's own D1/D1s RISC-V chip reportedly has 10
endpoints, more headroom — but a different architecture, niche pick for
conboard's arm64/armhf build pipeline.)

Sources: [Raspberry Pi Zero / Windows 10 RNDIS composite gadget](https://gist.github.com/Gadgetoid/c52ee2e04f1cd1c0854c3e77360011e2) ·
[P4wnP1-O2](https://packetwanderer.com/posts/p4wnp1-o2/) ·
[Raspberry Pi Zero as Multiple USB Gadgets](https://irq5.io/2016/12/22/raspberry-pi-zero-as-multiple-usb-gadgets/) ·
[musb-hdrc: can't add more than 2 functions to composite gadget](https://www.spinics.net/lists/linux-usb/msg163414.html) ·
[USB Gadget/Configfs - linux-sunxi.org](https://linux-sunxi.org/USB_Gadget/Configfs) ·
[Linux kernel Multifunction Composite Gadget docs](https://docs.kernel.org/usb/gadget_multi.html) ·
[AM335x multifunction composite gadget docs](https://github.com/hvaibhav/am335x-linux/blob/master/Documentation/usb/gadget_multi.txt) ·
[Synopsys DesignWare Core SuperSpeed USB 3.0 Controller docs](https://docs.kernel.org/driver-api/usb/dwc3.html) ·
[dwc3 endpoint-direction fix patch (RK3399 IN/OUT counts)](https://www.spinics.net/lists/linux-usb/msg216597.html)

**RESUMED (2026-08-16, same day): auto-detecting fallback instead of a board
switch.** Rafael has a second, non-Zero Orange Pi to develop this on, but
rather than hand-picking function sets per board, `usb-composite-all.sh` now
*tries* the full gadget (ACM+ECM+HID+mass-storage) first and falls back to
today's reduced gadget (ACM+HID+mass-storage, no network) only if the UDC bind
fails — recognizing the endpoint-budget limitation live, at boot, on whatever
board it's installed on, rather than needing a static per-board table. See the
"USB gadget auto-fallback" entry further down for the implementation and
hardware verification (proven on zero3, which is guaranteed to hit the
fallback branch).

## HMI phase 4a merged + deployed, hardware-verified (2026-08-16)
Merged `feat/hmi-phase4a-and-1to1-rules` into `main` (both the synthetic 1:1
keyboard rules and the WiFi list screen were already hardware-verified in the
2026-08-15 session that produced this branch — see the sections above).
`docs/next-sessions/04-synthetic-1to1-rules.md` and `05-hmi-wifi-screen.md`
dropped as part of the merge, now that both are done.

Deployed the merged `main` to `192.168.7.4` (`./build-cross.sh zero3` →
`install-on-device.sh`, normal reinstall, not `--purge`) to confirm the branch's
other change — a new `hmi_binding` table (backend schema) — migrates cleanly
onto an existing on-device database. It does: `CREATE TABLE IF NOT EXISTS` is
purely additive, no existing table's shape changed, so a plain reinstall is
enough for this kind of schema change (worth checking case-by-case — a column
change or rename would need `--purge`, this didn't). Confirmed on-device: the
existing `WirelessMouse`/`WirelessKB` board rows survived untouched
(`GET /api/v1/board` still returns them), and the new `GET /api/v1/hmi_binding`
route works and already had 2 rows in it (left over from the branch's own
QEMU-side testing, harmless).

`hmi.service` restarted clean on the real ARM binary (not QEMU) with the new
code: encoder/button GPIO lines still correctly report "no hardware, continuing
without it" (physically unwired, as expected, see phase 4b below), and firing
`POST /simulate` with `{"control":"hc_button2_press"}` correctly resolved
through the real on-device `hmi_binding` table (`hc_button2_press -> nk_select`,
logged), no crash. This is a step up from the branch's own "verified end to end
under QEMU" claim — same code path, now proven on the real device/binary — but
it's still not a substitute for someone looking at the physical panel while
navigating, which needs either `/simulate` driving it live or the encoders/
buttons actually wired. Neither happened this session.

**Activation screen explicitly deferred, not abandoned**: the screen itself
(built this branch, QEMU-verified, now also crash-free on real hardware via
`/simulate`) renders the still-stubbed `GET /hmi/activation` fields — Rafael is
doing the real GUI/UX design for this screen separately, outside this repo
interaction, so no further code changes here until that direction exists. See
`docs/next-sessions/06-hmi-phase4b.md`, unchanged, for the rest of what's open
(radio screen data-source decision, encoder/button physical wiring, nav-scheme
decision).

**Workflow change, same day**: introduced a `dev` branch (off this merge
commit) as the integration target going forward, at Rafael's direction — this
deploy-verification writeup is the first thing landing there rather than
directly on `main`. Not yet clear whether `dev` periodically merges to `main`
or replaces it as the primary branch; ask before assuming either way next
session.

## FIXED, not hardware-verified — identical-MIDI-device separation (2026-08-16)
Plan doc (`docs/next-sessions/09-midi-identical-device-separation.md`) removed
now that Task items 1-3 are done; Task item 4 (hardware verification) remains
open and is tracked in the "Next" list below instead.

**A real launcher bug, worse than the doc's own guess.** The doc's "Start here"
step 1 asked whether the launcher even spawns two `conMIDI` processes for two
identical controllers today. Reading `LowLevel/launcher/src/main.cpp` (the
`isEvdev` gate on per-instance service naming) showed it does not:
`devType::midi` was explicitly excluded from per-instance naming (`joystick`/
`keyboard`/`mouse` only), with a comment claiming MIDI "is unaffected and keeps
the plain DevName." So two identical MIDI controllers produced the *same*
systemd service name — the second unit's udev connect event just
`systemctl restart`s the first unit's already-running service. **Only one
`conMIDI` process ever ran at all**, not "two processes racing for one ALSA
card" as the doc speculated.

**Fix, mirroring evdev's already-working mechanism end to end** (same shape as
`EvdevDevice::resolveNode()` / `condetect::nodeUnderUsbPath`, which already
solves this exact problem for keyboard/mouse/joystick):
- `LowLevel/launcher/src/main.cpp`: `devType::midi` added to the per-instance
  condition (renamed `isEvdev` -> `isPerInstance`), so MIDI now gets a
  serial-or-port-keyed service name + `-d <devpath>` on its `ExecStart`, exactly
  like the other three device kinds.
- `LowLevel/Common/include/deviceDetect.{hpp,cpp}`: new `alsaCardSysfsPath(int
  card)` — the ALSA analog of the sysfs-path resolution `probeInput()` already
  does for evdev nodes (`realpath("/sys/class/sound/cardN")`).
- New `LowLevel/Common/{include,src}/midiPortMatch.*` (`midiportmatch::
  pickPort`) — pure, ALSA-free, unit-tested (`tests/test_midiportmatch.cpp`, 6
  cases, suite `midiport`): prefers the candidate whose name matches AND whose
  sysfs path sits under the given USB devpath (reusing the already-tested
  `condetect::nodeUnderUsbPath`), falling back unconditionally to the old
  first-name-match so single-unit setups can't regress even if the devpath
  heuristic doesn't land on real hardware.
- `LowLevel/MIDI`: `raw_midi` gained `sysfsPath` (populated in `list_device()`);
  `MIDI::MIDI()` takes a new optional `usbDevpath` param and calls
  `midiportmatch::pickPort` instead of its old inline first-name-match loop;
  `main.cpp` gained `-d`/`--devpath` argv parsing (mirrors `conKeyB` exactly).
- Considered and rejected: wiring the existing-but-dead
  `header.identifier.executable.port` JSON field (the doc floated this as
  possibly simpler). Rejected because that field lives in the board profile
  JSON, which is the *same file* shared by every physical unit of a controller
  model — it structurally cannot hold a different value per physical instance,
  so it can't solve identical-unit disambiguation. Left untouched, still dead.

**Verified this session**: `./run-tests.sh` — 96 cases pass (was 90), including
the new `midiport` suite. `./build-cross.sh zero3` — launcher, `conMIDI`, and
`libcommon.so` all compile cleanly cross-compiled for zero3.

**NOT verified — explicitly, per the doc's own honesty requirement**: no two
identical MIDI controllers were available this session, and the only reachable
board (`192.168.7.4`) currently has **no MIDI hardware attached at all**
(`/proc/asound/cards` shows only onboard codecs) — so neither the dual-unit
separation (the actual feature) nor the single-unit regression path (today's
only tested configuration) has been checked live. Both remain open; do them
first thing next time MIDI hardware is available, per Task item 4 and Done
criteria in the plan doc.

## MIDI SysEx support — code-complete + unit-tested, NOT hardware-verified (2026-08-16)

Planned in `docs/next-sessions/08-midi-sysex.md`, built the same session. Every
layer of the previous MIDI pipeline hard-assumed a fixed 3-byte message
(`midiSignal`, `LowLevel/Common/include/actions.h`) — SysEx (`0xF0...0xF7`,
arbitrary length) didn't fit anywhere, and `MIDI::in_func()` (`LowLevel/MIDI/
src/midithread.cpp`) explicitly **discarded** any ALSA read over 4 bytes
(`if (err > sizeof(midiSignal)) continue;`), meaning real SysEx traffic was
silently eaten before this session, not just unmatched.

**Design**: exact-match only (no prefix/wildcard) — a rule's `sysex` field is
the full expected byte sequence, framing bytes included, as a lowercase hex
string with no separators. Chosen because the wire envelope's `explode()`
parser (`LowLevel/Common/src/zmq_coms.cpp`) splits on `;` and strips literal
spaces — MIDI data bytes 0–127 can otherwise collide with those delimiter
characters, and hex trivially avoids that.

**What changed**:
- `actions.h`: new `midi_sysex` mode, `midiActions.sysex` (`std::vector<uint8_t>`),
  `hexEncode`/`hexDecode` helpers, `ar_str()` emits `"SX:<hex>"` for SysEx
  instead of `[b0,b1,b2]`.
- `midiMap.{hpp,cpp}`: new `matchesSysex()` (exact byte-for-byte); `matches()`
  never matches a `midi_sysex` trigger and vice versa.
- `MIDI::in_func()`: restructured to accumulate a variable-length message
  across possibly-many `snd_rawmidi_read()` calls (a single read is capped at
  256 bytes; real dumps commonly exceed that) once a `0xF0` is seen, stopping
  at `0xF7` or a 64KB safety cap (a device that sends `0xF0` and never
  terminates can't grow memory unboundedly). New `MIDI::processSysex()`
  parallels the existing `processInput()` (report + match + enqueue).
  **v1 simplification, stated in code comments**: assumes no System Real-Time
  bytes (clock/start/stop) are interleaved mid-SysEx — real hardware
  occasionally does this; unhandled today, flagged as a known gap to revisit
  if hardware testing hits it.
- `jsonParser.cpp`: `"sysex"` hex field on a `{"type":"midi",...}` object is
  **authoritative over `"mode"`** when present — decodes and forces
  `midi_mode = midi_sysex` regardless of what `"mode"` says, so a
  missing/mismatched `"mode"` can't silently produce a dead 3-byte trigger
  (the same failure shape as the keyboard/mouse trigger-parsing bug fixed
  2026-08-15). Malformed hex (odd length, non-hex chars) decodes to an empty
  vector rather than crashing or reading out of bounds.
- `backend/harpia/conboard.harpia`: `mm_sysex` enum value, `optional string
  sysex` on both `trigger` and `output_action`. Regenerated
  (`backend/generate.sh`) — domain hash bumped `1bf812ac18b80d4a5ea4d51e6bfb7f58`
  → `b13f689a5b6f99919ddaf4d1cc7eb7ac`; every hand-written `backend/src/*.cpp`
  include + the frontend's `HASH` constant updated to match. Backend rebuilt
  clean in the `harpia-build` image (needed `libudev-dev` installed in that
  image — not present by default, install-and-retry worked, worth remembering
  for next regen).
- Console (`frontend/console/src/`): `model/rules.ts`/`model/midi.ts` gained
  the `'sysex'` mode + hex helpers (`isValidSysexHex`, `normalizeSysexHex`,
  `looksLikeFramedSysex`); `RuleEditor.tsx`'s MIDI trigger editor swaps to a
  hex-input field in SysEx mode, the output editor gained a "Send as SysEx"
  checkbox (output actions have no `mode` field, so presence of `sysex` itself
  is the signal, matching `jsonParser.cpp`'s authoritative-field approach);
  `api/harpia.ts`/`api/map.ts` thread `sysex` through both directions. Two
  small new CSS hooks (`field.wide`, `input.invalid`) reusing existing
  `--danger`/grid-span patterns — not a return to the live-monitor CSS work,
  just the minimum needed for a usable hex field.

**Verified this session**: `./run-tests.sh` — 99 cases pass (was 96, +3 new
`jsonparser` sysex cases + 4 new `midi` suite cases), including exact-match
edge cases (one-byte difference, missing terminator, extra trailing byte) and
malformed-hex handling. Backend rebuilds clean against the regenerated schema
(`harpia-build` image). `npm run typecheck` clean in `frontend/console/`.

**NOT verified — no real SysEx round trip on hardware yet.** Same honesty
requirement as every other unverified claim in this file: this is proven
correct in isolation (unit tests, clean builds) but not against a real MIDI
device. Do that first — a MIDI "Universal Device Inquiry"
(`F0 7E 7F 06 01 F7` → identity reply) is a good baseline test that doesn't
depend on a controller's undocumented custom SysEx vocabulary, per the
session doc.

## Next (pre-release cleanup, 2026-08-12)
* HARDWARE TEST `conJoyS` (joystick) — built + unit-tested, keyboard/mouse already
  hardware-verified (2026-08-11), no gamepad available yet (still true as of
  2026-08-15). See `docs/HW-TEST-evdev.md`.
* HARDWARE RECONFIRM `conMIDI` open-retry fix (recovering from a transient ALSA
  port-busy failure after a redeploy restart, `LowLevel/Common/include/runDevice.hpp`)
  against the DJ-Tech-4-Mix controller (`boards/Dj4Mix.json`) — written +
  unit-tested since 2026-08-11, no MIDI hardware available to confirm the
  redeploy-recovery path on real hardware yet (still true as of 2026-08-15).
* HARDWARE VERIFY identical-MIDI-device separation (above, 2026-08-16) — fixed +
  unit-tested, needs two identical MIDI controllers (or at least one, for the
  single-unit regression check) to confirm live.
* HARDWARE VERIFY MIDI SysEx support (above, 2026-08-16) — code-complete +
  unit-tested, needs a real SysEx round trip against connected MIDI hardware
  (a Universal Device Inquiry exchange is a good minimal test) to confirm live.
* longer term: the local power-password login (design in `backend/README.md`,
  never implemented); true mouse/joystick 1:1 HID passthrough (sized above,
  under "Synthetic 1:1 keyboard rules"). Ethernet-gadget access is back in
  progress (above) — hardware-blocked on zero3, resumed with an
  auto-detecting fallback.
