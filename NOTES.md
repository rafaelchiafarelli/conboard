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

## Next (pre-release cleanup, 2026-08-12)
* HARDWARE TEST `conJoyS` (joystick) — built + unit-tested, keyboard/mouse already
  hardware-verified (2026-08-11), no gamepad available yet (still true as of
  2026-08-15). See `docs/HW-TEST-evdev.md`.
* HARDWARE RECONFIRM `conMIDI` open-retry fix (recovering from a transient ALSA
  port-busy failure after a redeploy restart, `LowLevel/Common/include/runDevice.hpp`)
  against the DJ-Tech-4-Mix controller (`boards/Dj4Mix.json`) — written +
  unit-tested since 2026-08-11, no MIDI hardware available to confirm the
  redeploy-recovery path on real hardware yet (still true as of 2026-08-15).
* **mouse/keyboard output not firing** (above) — now the top item, blocks the core
  remap-a-device feature for two of four device kinds.
* longer term: ethernet-gadget access, the local power-password login (design in
  `backend/README.md`, never implemented).
