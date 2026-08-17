# conboard — ethernet-gadget access, SHELVED (2026-08-16)

Investigated adding a USB-ethernet gadget (RNDIS+ECM) for driver-free network
access to the console. **Blocked on hardware, not a to-do**: the Zero 3's
musb-hdrc USB controller only has endpoint budget for 4 IN + 2 OUT total, which
the current gadget (ACM+HID+mass-storage) already uses in full — adding a
network function means *dropping* HID or mass-storage from that config, not
adding on top. Confirmed live via configfs bind tests (`unable to autoconfigure
all endpoints`, kernel `-524`) before any code was proposed for landing. All
changes reverted; board back to its known-good state (`configured`,
ACM+HID+mass-storage). Full writeup in [../NOTES.md](../NOTES.md).

Next step, if picked back up: retarget a board with a dwc2/dwc3-class USB
controller (more endpoint headroom than musb-hdrc) instead of the Zero 3.

---

# conboard — console bug hunt handoff (2026-08-14)

Session driven by the user actually running the console against the real board
(`192.168.7.4`) and reporting what broke, live. Full detail (root causes, what was
ruled out, exact repro commands) is in [../NOTES.md](../NOTES.md) under "Console bug
hunt against real hardware" and the two "OPEN" sections right after it — this is
just the short version so the next session knows where to start.

**Fixed + deployed this session** (all rebuilt/redeployed to `192.168.7.4` and
re-verified, not just typechecked):
- WirelessKB/WirelessMouse were never added to the console's DB-seeding fixtures
  (`frontend/console/src/fixtures/boards.ts`) even though their realtime templates
  shipped back on 2026-08-11 — invisible + un-addable in the console despite working
  hardware. Fixed.
- A real harpia codegen bug: the generated protobuf field for the trigger's
  `interval` is actually named `erval` on the wire (`backend/generated/proto/
  protofiles/trigger_*.proto`). Sending `interval` 400s silently — this is why
  WirelessKB kept failing to seed even after the fixtures fix (its hold-mode rule
  carries `interval`). Fixed in `frontend/console/src/api/{harpia,map}.ts`.
- Live monitor decoded raw `[type,code,value]`/MIDI triples into readable labels
  (was showing meaningless raw numbers).
- Live monitor was silently truncating event text (CSS ellipsis, no wrap) — fixed.
- Two `window.alert()` calls (copy/delete failure) replaced with an in-app toast.

**RESOLVED (2026-08-15) — mouse/keyboard rule output not firing.** Root cause:
`jsonParser::parseIO()` only parsed the evdev trigger (`code`/`mode` → `evtrig`) for
`"type":"joystick"`; the `mouse`/`keyboard` case branches only parsed output-shaped
fields, so `evtrig.mode` stayed `ev_nomode` and `evmatch::matches()` returned false
for every keyboard/mouse rule, always. Proven live before the fix (inotify watch on
`/dev/input/event0`+`event1`+`/dev/hidg0`): 1306 real input events, 0 HID writes.
Fixed by sharing the trigger-parsing helper across joystick/keyboard/mouse instead of
joystick-only (`LowLevel/Common/src/jsonParser.cpp`). Hardware-reverified after
rebuild/redeploy: 138 real input events → 612 `/dev/hidg0` writes, user-confirmed the
expected text appeared on the connected host. Full writeup in
[../NOTES.md](../NOTES.md).

**RESOLVED (2026-08-15, same day) — the `enqueue()`/`out_func()` lock gap.**
`DeviceEngine::out_func()` now takes `locking_mechanism` around the `oQueue`
front/pop check (kept scoped tightly, `executeOutput()` runs outside the lock so a
delayed output can't stall `enqueue()`). Rebuilt/redeployed/reverified live: 267
real input events → 628 `/dev/hidg0` writes, output still firing correctly.

**RESOLVED (2026-08-15) — live monitor layout ("it is ugly" + can't be resized).**
Two stacked bugs, both found live rather than by reading the CSS: a stale
fixed-width grid (`.feed-head`/`.ev-row` in `index.css`) whose columns no longer
fit the panel's current 280–768px range, which combined with the Aug-14
`min-width: 0` truncation fix to collapse the action column to ~0px and
ink-overflow its text into the UUID column — genuinely overlapping, not just
cramped; and a resize handle that technically worked but sat in CSS's spec-fixed
bottom-right corner, the wrong corner for a panel docked on the right edge (a user
spot-check on the real board caught this after the first fix only addressed
discoverability, not the corner itself). Fixed both, added a real drag handle on
the shared left edge with `cursor: ew-resize` feedback, rebuilt/redeployed to
`192.168.7.4` twice and user-confirmed live each time. Full writeup in
[../NOTES.md](../NOTES.md).

---

# conboard — local screen/buttons/encoders UI handoff (2026-08-10/11, phase 4a 2026-08-15)

New, **separate** workstream from the console-fixes milestone below — a small
SPI TFT + 2 push buttons + 2 rotary encoders (each with its own pushbutton),
wired directly to the Orange Pi. Deliberately independent of the rest of the
codebase (see the plan doc from the session that started this): no
`libcommon.so`, no `DeviceEngine`, no udev/launcher path, and it gets **all**
domain data over the backend's REST/JSON API — no local business logic.

Scope was phases 1-3 of a 5-phase plan (dependencies + a screen-size-adaptable
base component layer + a custom theme matching the console's palette), plus
now phase 4a (2026-08-15): the first real domain screen (WiFi list),
hardware-confirmed. Phase 4b (activation/radio screens) and phase 5
(encoder/button wiring, the real navigation scheme) are still not started.

## What's built

- **`LowLevel/HMI/`** (binary `conHMI`): LVGL (vendored submodule, pinned
  `v9.5.0`) + libgpiod + libcurl + vendored `nlohmann/json`. `PanelDriver`
  abstraction (`panel_null` for hardware-less dev/test, `panel_st7789` as the
  first real chip driver) + `ClippedPanel` (a working-box crop wrapper, see
  below) + `RotaryEncoder`/`PushButton` (libgpiod polling + pure, unit-tested
  quadrature-decode/debounce logic) + `RestClient` (libcurl + JSON — the
  **only** way this module learns anything) + `app_shell` (generic,
  resolution-adaptive LVGL nav/menu/value components) + `lvgl_glue` (wires it
  all into LVGL's display/indev model). One concrete demo screen: fetches and
  renders the console URL over HTTP, proving the REST-sourced-data path end
  to end.
- **`backend/src/hmi.cpp`** (additive, hand-written like `devices.cpp`):
  `GET /api/v1/hmi/{console-url,wifi/networks,activation,radio/stations}`,
  same `X-User`/`X-Pswd` gate as every other route. `console-url` (real,
  `getifaddrs()`) and `wifi/networks` (real, `nmcli -t -f SSID,SIGNAL,SECURITY
  dev wifi list`) work; `activation` and `radio/stations` are documented
  stubs (activation ties to the still-unimplemented power-password design in
  `backend/README.md`; radio's data source is still TBD). All read-only —
  a WiFi-connect action is a later, mutating addition.
- Build pipeline: `conHMI` wired into `LowLevel/scripts/compiler.sh` /
  `docker/package.sh` / install+uninstall scripts, `libgpiod-dev` +
  `libcurl4-openssl-dev` added to `docker/Dockerfile`. `hmi.service` ships
  but is **not** auto-enabled by `install-on-device.sh` (needs real wiring
  first) — enable manually with `systemctl enable --now hmi.service`.
- 83 doctest cases (was 77) — added `quadrature`, `debounce`, `clipped_panel`
  suites, all pure logic (no gpiod/curl/lvgl needed to run them).
- **Phase 3 (2026-08-12): custom LVGL theme matching the console.**
  `LowLevel/HMI/include/hmi_theme.hpp` + `src/hmi_theme.cpp` — a small
  `lv_theme_t` registered via LVGL's public theme API (`lv_theme_create` +
  `lv_theme_set_apply_cb`), with the palette copied 1:1 from
  `frontend/console/src/index.css`. Styles screens/root with the console's
  `--ground` bg / `--ink` text (inherits to child labels for free), gives list
  buttons a `--line` divider, and gives the encoder-focused item the
  `--accent-soft` bg + `--accent` text + left border treatment that mirrors
  the console's `.viewnav button.on` active-tab look. Wired in once, right
  after `lvgl_glue::createDisplay()` in `main.cpp`. Deliberately narrow: only
  covers the widget kinds `app_shell.cpp` actually builds today (screens,
  plain containers, `lv_list`/its buttons) — add cases to `themeApply()` as
  phase-4 screens introduce new widget kinds (arc, roller, ...). Verified
  visually with no hardware: `--panel null --dump` under qemu, rendered the
  PPM (confirmed both the plain demo screen and, via a temporary 3-item demo
  menu that was reverted before committing, the list/divider/focus styling).
  **Hardware-confirmed same day**: deployed the binary to `192.168.7.4`
  (swapped `/conboard/LowLevel/HMI/build/conHMI` directly, no full reinstall
  needed) and started `hmi.service` for the first time this session --
  `journalctl` shows a clean `conHMI: running (panel=st7789, ...)` with only
  the expected "no hardware?" warnings for the still-unwired encoders/buttons,
  and the dark theme background is confirmed visible by eye on the physical
  panel.
- **Phase 4a (2026-08-15): the WiFi list screen — DONE, hardware-confirmed.**
  New `LowLevel/HMI/include/wifi_screen.hpp` + `src/wifi_screen.cpp`: fetches
  `GET /hmi/wifi/networks`, renders one `appshell` menu row per network
  (`SSID  (signal%)`), with separate fallback `createInfoLabel` text for a
  failed fetch vs. an empty/no-networks result. `main.cpp` now pushes a small
  top-level menu ("Console URL", "WiFi") at startup instead of jumping
  straight into the old console-url demo, so both screens are reachable
  through one root — a minimal base for phase 4b's activation/radio entries
  to slot into later. Also bumped the compiled-in default font from
  Montserrat 14 to **Montserrat 18** (`lv_conf.h`) — 14 was unreadably small
  for a scrollable list of real network names on the physical panel.
  **Hardware-confirmed** on `192.168.7.4`: real nearby networks render
  correctly with good row spacing (confirmed via a temporary combined
  debug screen — console-url text as a header line above the WiFi list on
  one screen, since there's no encoder/button hardware yet to navigate the
  real top-level menu — reverted before committing, same pattern as phase
  3's temporary demo menu). User feedback: layout works but isn't polished
  yet ("not very pretty") — left for a future UI-design pass, not blocking.

## Hardware-confirmed facts (dev board: `rafael@192.168.7.4`, `orangepizero3`)

Found by diffing against a **known-working Python reference driver already on
that board** (`/home/rafael/workspace/bart-lobotomy`, esp. `ST7789/ST7789.py`
and `main.py`) after the first hardware test showed nothing on screen. Keep
using that repo as ground truth if this panel misbehaves again — it's the
only proof that ever worked on this exact unit.

- **Panel**: real ST7789, 240×320 (2" module), mounted **landscape** in its
  enclosure.
- **RESET = PC7 (gpiochip0 offset 71), DC = PC10 (gpiochip0 offset 74)** —
  now the compiled-in defaults in `main.cpp`.
- **SPI device is `/dev/spidev1.1`, not `spidev0.0`** (both exist on this
  board; only `.1.1` is wired to this panel) — mode **3** (not 0), **4 MHz**
  (not a generic ST7789 spec speed) — all now compiled-in defaults.
- **The critical one**: this SoC's SPI controller silently no-ops on plain
  `write(2)` to `/dev/spidevN.N`. Only a full `ioctl(SPI_IOC_MESSAGE)`
  transfer actually clocks data out — exactly what Python's `spidev.xfer2()`
  does under the hood. Symptom was "no error anywhere, panel just keeps
  showing whatever was on it before" — nothing crashed, nothing logged, the
  screen just silently never got the new frame. Fixed in
  `panel_st7789.cpp`'s `spiTransfer()`, chunked at 4096 bytes (matches the
  reference driver's chunk size — a single ioctl covering a whole flush can
  silently truncate past that on some kernel configs).
- **Init sequence**: the panel needed the full gamma/power/porch/VCOM trim
  block (`PORCTRL`/`GCTRL`/`VCOMS`/`LCMCTRL`/`VDVVRHEN`/`VRHS`/`VDVSET`/
  `FRCTR2`/`PWCTRL1`/`PVGAMCTRL`/`NVGAMCTRL`), copied byte-for-byte from the
  reference driver — a minimal SWRESET/SLPOUT/COLMOD/MADCTL/DISPON sequence
  (what shipped initially) was not enough. `COLMOD` data byte is `0x05`, not
  the more common `0x55`. **Do not "clean up" this sequence against a generic
  ST7789 datasheet without re-testing on this exact panel** — cheap clones
  vary enough that a "more correct" sequence can render worse than what's
  proven to work.
- **Rotation: FINALIZED (2026-08-11) at `CONHMI_PANEL_ROTATION=270`** (`0xA0`).
  `90` was tried first and looked flipped 180° on the actual enclosure; `270`
  confirmed correct by eye on hardware. Applied on the board via a
  `hmi.service.d` drop-in and now also the compiled-in default in
  `LowLevel/HMI/src/main.cpp` (so a fresh install needs no drop-in for this).
- **`/conboard` on this board was NOT a real install before this session** —
  `conboard_backend` was a 0-byte placeholder, `BOARD.txt`/`HOW-TO-INSTALL.txt`
  were empty, and both `backend.service` and `hmi.service` were `systemctl
  mask`ed (origin unknown). Unmasked + did a real `install-on-device.sh` run;
  both services are now genuinely active. If a future session finds masked
  units again, `systemctl unmask <unit>` before reinstalling — `install`
  writing through a `/dev/null`-symlinked unit path silently no-ops, which is
  probably why the earlier "install" produced empty files.

## Still open / not yet wired

- **Encoder/button GPIO lines are still placeholders** (never wired):
  `CONHMI_ENC1_A/B`=5/6, `CONHMI_ENC1_BTN`=12, `CONHMI_ENC2_A/B`=16/20,
  `CONHMI_ENC2_BTN`=21, `CONHMI_BTN1`=26, `CONHMI_BTN2`=19. `conHMI` starts
  fine without them (each logs "did not start (no hardware?)" and continues)
  — update these (same `systemctl edit hmi.service` drop-in pattern) once
  wired, then tell the assistant so the compiled-in defaults get updated too.
- **Working box not yet set** (`CONHMI_WORK_X_OFFSET`/`Y_OFFSET`/`WIDTH`/
  `HEIGHT`, all default to the full 320×240 landscape view). User's enclosure
  crops the visible area to something smaller/offset — needs real
  measurements. See `LowLevel/HMI/include/clipped_panel.hpp` for how it
  composes (wraps any `PanelDriver`, crops+offsets, everything above it in
  the stack — LVGL, AppShell — only ever sees the cropped size).
- Phase 4a (WiFi list) is done and hardware-confirmed — see "What's built"
  above. Phase 4b's activation screen is built (`activation_screen.cpp`),
  not yet hardware-confirmed. Radio screen still open.
- **Nav scheme (which physical control does what) — partially data-driven
  now.** New `hmi_binding` table (`backend/harpia/conboard.harpia`, separate
  from the rules-library domain — see its README's domain-shape table) maps
  one `hmi_control` (`hc_encoder1_ccw`/`cw`/`press`, same for encoder2,
  `hc_button1_press`, `hc_button2_press`) to one `hmi_nav_key`
  (`nk_next`/`prev`/`select`/`back`/`up`/`down`). Full CRUD at
  `/api/v1/hmi_binding` (harpia-generated, auth `X-User: hmi_binding` —
  different from the hand-written `hmi` routes' looser check). `conHMI`
  fetches it at startup (`fetchHmiBindings()` in `main.cpp`) and applies it
  to the two standalone buttons (falls back to the old hardcoded ESC/NEXT if
  a binding is missing, so an empty table doesn't regress real hardware).
  **Both encoders still use LVGL's native ENCODER indev semantics on real
  hardware** — their `hmi_binding` rows exist and are honored by
  `/simulate` (below) but not by the real GPIO path yet; that needs a
  custom keypad-style encoder adapter to replace LVGL's built-in one,
  a separate decision not made this session. Real navigation by hand still
  needs the encoder/button GPIO lines wired (see above).
- **New: dev-only `/simulate` HTTP endpoint, opt-in via `CONHMI_SIM_PORT`.**
  Lets any of the 8 `hmi_control` events be fired without real GPIO —
  `curl -d '{"control":"hc_button1_press"}' http://host:PORT/simulate`.
  Hand-rolled over POSIX sockets (`sim_server.cpp`), one route, no new
  dependency; queues the control name, the LVGL-owning thread drains and
  applies it via `lv_group_send_data()` once per `runLoop()` tick (LVGL
  itself isn't thread safe, so the accept thread never touches it
  directly). Verified end to end under QEMU this session: created
  `hmi_binding` rows via curl, started `conHMI --panel null` pointed at
  that backend, fired `/simulate` for a bound control (applied + logged),
  an unbound control (logged "no binding", no crash), and garbage JSON
  (400, no crash). Not yet tried on the real board.

## Persisting hardware config (durable across reinstalls)

**Never edit `/etc/systemd/system/hmi.service` directly** — `install-on-
device.sh` reinstalls it fresh (via `install -m 644`) on every run, silently
discarding direct edits. Use a drop-in:
```sh
sudo systemctl edit hmi.service
# paste under [Service]:
#   Environment=CONHMI_ENC1_A=<n>
#   Environment=CONHMI_WORK_X_OFFSET=<n>
#   ...
sudo systemctl restart hmi.service
```

## Env var reference (`LowLevel/HMI/src/main.cpp`)

| var | default | status |
|---|---|---|
| `CONHMI_REST_BASE` | `http://127.0.0.1:8080/api/v1` | fine as-is (same-host backend) |
| `CONHMI_REST_PSWD_HASH` | `9f20d5d43738774941f9898b22cf2cf2` | matches backend's compile-time hash |
| `CONHMI_SPI_DEVICE` | `/dev/spidev1.1` | **confirmed** |
| `CONHMI_PANEL_SPI_SPEED_HZ` | `4000000` | **confirmed** |
| `CONHMI_GPIO_CHIP` | `gpiochip0` | **confirmed** |
| `CONHMI_PANEL_RESET_LINE` | `71` (PC7) | **confirmed** |
| `CONHMI_PANEL_DC_LINE` | `74` (PC10) | **confirmed** |
| `CONHMI_PANEL_BL_LINE` | `-1` (none) | not needed on this panel (no BL pin used by the reference driver) |
| `CONHMI_PANEL_WIDTH` / `HEIGHT` | `240` / `320` | **confirmed** (physical panel, pre-rotation) |
| `CONHMI_PANEL_ROTATION` | `270` | **confirmed** (2026-08-11) |
| `CONHMI_WORK_X/Y_OFFSET`, `WIDTH`/`HEIGHT` | `0,0,320,240` (no crop) | **placeholder** — needs real enclosure measurements |
| `CONHMI_ENC1_A`/`B`/`BTN`, `CONHMI_ENC2_A`/`B`/`BTN`, `CONHMI_BTN1`/`BTN2` | see above | **placeholder** — not wired yet |
| `CONHMI_SIM_PORT` | unset (disabled) | dev-only; set to a port to enable the `/simulate` HTTP endpoint |

## How to build / deploy (this workstream)

```sh
./build-cross.sh zero3            # -> dist/zero3/conboard-zero3.tar.gz
scp dist/zero3/conboard-zero3.tar.gz rafael@192.168.7.4:~
ssh rafael@192.168.7.4
tar xzf conboard-zero3.tar.gz && cd conboard
sudo ./install-on-device.sh                  # does NOT auto-start hmi.service
sudo systemctl enable --now hmi.service
sudo journalctl -u hmi.service -f
```

---

# conboard — next-session handoff (milestone `2026-08-10`)

`main` = the full console-fixes stack (console-quickwins → deploy-regression →
device-inventory → live-monitor → console-fixes) merged in, plus a Blender-control
investigation's salvageable bits (`docs/reference/DJTech-4-Mix.csv`, `boards/blender.cmds`,
DevInspector).
Tagged `milestone-2026-08-10`. Pushed to `origin/main`. Full history is normalized to
one git identity (`rafael.chiafarelli@gmail.com`) — a mixed-identity repo was the
original trigger for that cleanup; a `backup/blender-investigation-2026-08-10` branch
holds the raw pre-cleanup local work in case anything in it is ever worth a second look.

See the top-level [README.md § What's built](../README.md#whats-built) for the feature
list. This file is the **live punch list** — what's known broken or unverified.

## Open bugs

- **`uninstall-on-device.sh --purge` unreliable — TWO BUGS FOUND, FIXED + HARDWARE-
  VERIFIED (2026-08-12).** Root-caused by comparing `docker/uninstall-on-device.sh`
  against what actually creates state (`install-on-device.sh`, the launcher,
  `scripts/usb-composite-all.sh`):
  1. The per-device handler SIGTERM-hang bug (see "Dispatcher devname corruption" /
     evdev fixes below) made `uninstall`'s `systemctl stop` loop over auto-generated
     per-device units slow/wedge-prone — already fixed as a side effect of that fix,
     same session.
  2. **New, independent bug**: the gadget teardown at the end of the script only did
     `echo '' > .../g1/UDC` (unbind) — it never removed the `g1` configfs tree
     (functions/configs/strings symlinks + dirs), unlike the real teardown in
     `scripts/usb-gadget-stop.sh` sitting right there in the same artifact. Fixed by
     calling that script (it's still on disk at that point in the script, before the
     later `rm -rf /conboard`) instead of reimplementing a partial version inline.

  **Verified on `192.168.7.4`**: full cycle — install (confirmed `g1` bound with
  functions/configs live) → plain uninstall (`g1` fully torn down: symlinks removed,
  every configfs dir `rmdir`'d down to `g1` itself gone, not just UDC-unbound; data +
  web login kept; firewall reopened) → reinstall (same password, both wireless
  devices and the rules DB came back) → `--purge` (`/conboard` + web login + `g1` all
  gone) → final reinstall to leave the board working.

  **Third bug found in the course of verifying this one**: `--purge` always exited
  `1` even on full success — `[ "$PURGE" -eq 0 ] && echo ...` as the script's last
  line, so a false condition (true when purging) made the whole script's exit code 1
  regardless of everything above it. Harmless interactively, would break any
  automation checking the exit code. Fixed with a proper `if`; re-verified exit code
  0 on the board.

- **Dispatcher devname corruption on registration. FIXED + HARDWARE-VERIFIED
  (2026-08-12).** `dispatcher::th_unique_number()` (`LowLevel/dispatcher/src/
  dispatcher.cpp`) had `memset(data,1024,0)` — arguments swapped (should be
  `memset(data,0,1024)`), so it zeroed zero bytes, a no-op — then built
  `l_devname` via `std::string((char *)message.data())`, assuming a NUL
  terminator the wire protocol never provides (device→dispatcher registration is
  raw, undelimited bytes per `INTERFACE.md` §2.1). When a shorter devname's ZMQ
  message reused memory that previously held a longer one, the string ctor read
  past the real content into stale bytes. **Caught live**: after a service
  restart, the `/ws` heartbeat roster showed `WirelessKBuse` instead of
  `WirelessKB` — literally the old `WirelessMouse` registration's tail
  (`"...use"`) glued onto the new, shorter name.

  Fixed by bounding the string construction with `message.size()` instead of an
  assumed terminator, and fixing the swapped `memset` args. Verified on
  `192.168.7.4`: cycled both handlers' registration several times via
  `systemctl restart`, including deliberately reproducing the exact
  Mouse-then-KB order that originally triggered the corruption — clean
  `WirelessKB`/`WirelessMouse` devnames on `/ws` every time.

## Synthetic 1:1 keyboard rules on hotplug — DONE + HARDWARE-VERIFIED (2026-08-15)

Idea sized 2026-08-11, built and verified 2026-08-15. Full writeup:
[next-sessions/04-synthetic-1to1-rules.md](next-sessions/04-synthetic-1to1-rules.md).
Short version: a "Seed a full 1:1 rule set" checkbox in the console's Add-Device
dialog (keyboard type only) generates all 105 `KEY_*` → keyboard-output rules via
the existing harpia CRUD (`frontend/console/src/model/oneToOneKeyboard.ts`),
riding the create+deploy pipeline unchanged. Required extending
`LowLevel/Common/src/evMatch.cpp`'s `kSymbols` from 52 to 105 entries so the new
symbols actually resolve at deploy/runtime (lives in `libcommon.so`, a shared
library every LowLevel binary links — not statically in each executable).
Hardware-verified on `192.168.7.4`: `KEY_MINUS` and `KEY_F1` (both newly
resolvable) fired correct HID output on the real wireless-keyboard dongle; board
restored to its pre-session state afterward.

**Related, explicitly NOT this feature — true mouse/joystick 1:1 HID passthrough**
(the bigger idea that came up first, deferred as out of scope): re-checked
2026-08-15, still accurate. Would need real HID output, not just DB rows.
`oActions::mouse_fill_report`/`joystick_fill_report` (`LowLevel/Common/src/
oActions.cpp`) already build correct report bytes, but the dispatch functions
that would call them, `oMouse()`/`oJoystick()` (`LowLevel/Common/include/
oActions.hpp`), are literal empty stubs (`virtual void oMouse(mouseActions){}`)
— never wired to anything. Bigger gap: the USB gadget composite (`scripts/
usb-composite-all.sh`) only declares **one** HID interface, hardcoded as a
keyboard (protocol=1, boot-keyboard report descriptor) — no mouse/joystick HID
function exists in the gadget at all yet. Sized medium (new gadget HID function +
wiring + real OTG-to-host-PC verification, untested territory beyond keyboard),
not attempted.

## Still needs a dispatcher-side change

- **O1 — HTTP port inconsistency. RESOLVED (2026-08-11).** Dispatcher now reads its
  HTTP port from `config.json` (`dispatcher::GetHTTPPort()`) instead of hardcoding
  `app.port(40080)`; `config.json`'s `http.port` was changed from `9080` to `40080` to
  match what nginx and every deployed board already use. Not yet rebuilt/redeployed to
  the board — do that before relying on it live.
- **O5 — heartbeat/roster frame. RESOLVED + HARDWARE-VERIFIED (2026-08-12).**
  Dispatcher now emits `HB,<uuid>,<devname>` ~1/s per live sender on `/ws`
  (`dispatcher::GetHeartbeats()` + `user_handler` in `LowLevel/dispatcher/src/
  {dispatcher,main}.cpp`), reusing the existing `devices`/`last_ping` maps — "live"
  means a ping within 5s. Deployed to `192.168.7.4` and confirmed by connecting
  directly to the dispatcher's `/ws` (port `40080`) with the real wireless
  keyboard+mouse combo (`conKeyB`/`conMouse`, boards `WirelessKB.json`/
  `WirelessMouse.json`) live: two distinct `HB,<uuid>,<devname>` lines streamed once
  a second, uuids matching the currently-registered devices, cadence as designed.
  Not yet cross-checked against the console's UI directly (only the raw `/ws`
  stream) — do that next if the per-device liveness LEDs need a visual confirm.

  **Further confirmed with real input** (same session, hands on the board): captured
  a 15s `/ws` window while physically typing on the wireless keyboard and moving the
  mouse. Every resulting action frame's uuid matched a uuid the HB roster was
  already advertising under the right devname -- keystrokes (evdev codes 16/17/30/31,
  press+release pairs) landed under the `WirelessKB` uuid, relative-motion reports
  (`[2,axis,delta]`) landed under the `WirelessMouse` uuid. So the uuid-to-devname
  mapping isn't just structurally correct, it's proven against live, distinguishable
  traffic from two simultaneous senders.

  **Reinstalling on this board re-triggered the known SIGTERM-hang bug** (see below)
  on `WirelessKB-port-4-1.service`/`WirelessMouse-port-4-1.service` — both ended up
  `failed`/wedged after `install-on-device.sh`'s stop step, same symptom as
  2026-08-11. Recovered manually (`systemctl reset-failed` + `daemon-reload` +
  `start` on both units) to continue testing. This is not a new bug, just another
  confirmed occurrence — still open, see below.

- **O2 — envelope version field. RESOLVED + HARDWARE-VERIFIED (2026-08-12).** Added a
  `v0` token to io/heartbeat (both ZMQ directions) and `/ws` output (action rows + `HB`
  frame). Full detail in `INTERFACE.md` §2/§5 O2 — worth reading there, not
  duplicated here, because the interesting part is two bugs found while verifying it,
  not the feature itself:
  - A heartbeat-reply bug in `dispatcher::th_heart_beat()`: queued commands were sent
    to devices as empty strings (map lookup happened after erasing the entry). Fixed.
  - **A real, previously-unknown bug**: the shared `explode()` parser (duplicated in
    `zmq_coms.cpp` and `dispatcher.cpp`) called `std::remove(...)` to strip spaces but
    never followed it with `erase()`, so every token after the first kept a leftover
    duplicated trailing character. Practical effect: **heartbeat-delivered
    `reload`/`file`/`outstop` commands have never actually worked** — the exact-string
    compares in `DeviceEngine::coms_handler()` never matched. Root-caused via an
    isolated reproduction outside the codebase, then fixed at the source (both
    `explode()` copies). Not yet separately re-verified live (a `reload`/`file` command
    delivered end-to-end through a real heartbeat round trip) — the fix is proven
    correct in isolation and the surrounding pipeline (uuid matching, `HB` frames) is
    hardware-verified, but the specific command-delivery path itself wasn't
    re-exercised live this session. Do that first if anything built on top of
    reload/file/outstop misbehaves.
  - Deploy pitfall hit twice this session: after the first `scp` + `tar xzf` +
    `install-on-device.sh` cycle, later redeploys only re-ran `install-on-device.sh`
    against the **already-extracted** `~/conboard` directory without re-extracting the
    freshly uploaded tarball first — so two rebuild-and-redeploy cycles silently
    installed a stale binary while looking successful (services restarted cleanly, just
    running old code). Caught by `strings <binary> | grep <known-new-string>` on the
    deployed binary. **Always re-run `tar xzf conboard-zero3.tar.gz` before
    `install-on-device.sh` on every redeploy, not just the first.**

- **O4 — reporting-queue overflow. RELIEVED + DEPLOYED (2026-08-12).** `STACKED_IO_MSG`
  `10`→`64`, drop-oldest eviction on overflow instead of drop-newest, rate-limited
  overflow log. Detail in `INTERFACE.md` §2.2/§5 O4. Deployed alongside O2; not
  separately load-tested against a real sustained burst (no easy way to generate one
  from the wireless keyboard/mouse combo on hand this session).

- **Dispatcher SIGABRT-on-stop. FOUND + FIXED + HARDWARE-VERIFIED (2026-08-12,
  same session).** Seen repeatedly earlier this session
  (`journalctl -u dispatcher.service` after every `install-on-device.sh` run):
  `terminate called after throwing an instance of 'std::system_error'` /
  `what(): Invalid argument`, `Main process exited, code=killed, status=6/ABRT`.
  Exactly the same class of bug already fixed in `zmq_coms::die()` for
  `conKeyB`/`conMouse` (double-`join()` on the same `std::thread`, see the
  2026-08-12 entry above): `main.cpp` calls `dsp.die()` explicitly before
  returning, then `~dispatcher()` calls `die()` again as the stack-allocated
  `dsp` goes out of scope, double-joining `hb`/`th_unuique_numb`/`io`. Fixed with
  the same `joinable()`-guard pattern in `dispatcher::die()`
  (`LowLevel/dispatcher/src/dispatcher.cpp`). Verified on `192.168.7.4`: two
  consecutive reinstalls against the fixed binary both show `Deactivated
  successfully` in the journal, no crash.

## Still needs on-board verification

- **Basic security hardening — DONE + HARDWARE-VERIFIED (2026-08-12).**
  Pre-release pass:
  1. nginx Basic Auth in front of the whole site (`/healthz` excepted) —
     `backend/assets/interface.conf` + a password generated by
     `install-on-device.sh` into `/etc/conboard-web-password.txt`.
  2. **Real bug found**: the dispatcher's `/config`, `/iocommand`, `/screencommand`,
     `/ws` HTTP server (`LowLevel/dispatcher/src/main.cpp`) had no credential check
     on any route, and Crow defaults to binding `0.0.0.0` when `.bindaddr()` is never
     called — so `POST /iocommand` (arbitrary device commands) was reachable from
     anywhere on the network with zero auth. Confirmed nothing outside this process
     calls those routes except the console's `/websocket`, which nginx already
     proxies to it over loopback (`frontend/console/src/model/events.ts`) — so
     `.bindaddr("127.0.0.1")` closes it with no functional change.
  3. A default-deny INPUT firewall (`scripts/conboard-firewall.sh` +
     `docker/assets/conboard-firewall.service`), ssh + :80 allowed, everything else
     dropped. Designed to fail open (a trailing DROP *rule*, not chain policy) so a
     broken firewall doesn't lock out SSH.

  **Verified on `192.168.7.4`**: reproduced the hole first (unauthenticated
  `curl board-ip:40080/config` returned the live dispatcher config from another
  machine on the LAN) before deploying anything. After: that request now
  hangs/drops, `ss -tlnp` confirms the dispatcher moved to `127.0.0.1:40080`,
  the console gives 401/401/200 for no-creds/wrong-creds/right-creds with
  `/healthz` staying open, and a raw websocket upgrade through the authenticated
  `/websocket` proxy succeeded with real `HB,<uuid>,devname` frames streaming —
  the live monitor still works end to end. SSH stayed reachable throughout.

  **A fourth real bug found only by testing on hardware**: the generated
  `.htpasswd-conboard` was written `root:root`, but nginx runs as `www-data` —
  every *authenticated* request 500'd (`open() ... Permission denied` in nginx's
  error log) while unauthenticated ones looked fine (401, no file read needed),
  which would have hidden this behind a shallow "looks like it's working" check.
  Fixed: `install-on-device.sh` now chowns the file to nginx's actual configured
  group (parsed from `nginx.conf`, default `www-data`) on every install.

  Full install/reinstall/uninstall/`--purge` cycle re-verified end to end (password
  persists across reinstall, everything's removed on `--purge`, firewall reopens on
  plain uninstall).

  **Follow-up, same day**: the user (who administers these boards directly, not just
  through this repo) asked for the password to be reliably retrievable so a lost
  terminal scrollback can never turn into a real lockout. Added `conboard-password`
  (`docker/assets/conboard-password.sh`, installed to `/usr/local/bin`) — `sudo
  conboard-password` shows the current login, `sudo conboard-password --reset`
  rotates it. `install-on-device.sh` now calls this same script for first-boot
  generation instead of duplicating the logic, so there's one code path. Each
  (re)generation also logs a breadcrumb via `logger -t conboard` (visible with
  `journalctl -t conboard`) — deliberately *without* the raw password, since journal
  read access can be broader than root on some distros; the secret itself only ever
  lives in the password file and whatever the command just printed. **Verified live
  on `192.168.7.4`**: `sudo conboard-password` returned the exact known password;
  running it without `sudo` failed cleanly instead of a stack trace; `--reset`
  rotated it and the change took effect immediately with no service restart (old
  password 401'd, new one 200'd right after); the journal breadcrumb showed up with
  no password in it; a plain uninstall kept the command + login (survives a
  reinstall), `--purge` removed all three files/binary together.

- **evdev hardware test — keyboard + mouse VERIFIED (2026-08-11)** on `192.168.7.4`
  with a real 2.4G wireless keyboard+mouse combo receiver (`4037:2804`). Real
  keystrokes and real mouse motion/clicks confirmed flowing end-to-end (hardware →
  kernel evdev → `conKeyB`/`conMouse` → `DeviceEngine::report()` → zmq → dispatcher →
  `/ws` live stream), cross-checked against a raw `/dev/input/eventN` capture running
  simultaneously. Starter profiles added: `boards/WirelessKB.json` /
  `boards/WirelessMouse.json` (closes the "no keyboard/mouse profiles shipped" gap).
  Joystick (`conJoyS`) still untested — no gamepad was available this session.
  Runbook: `docs/HW-TEST-evdev.md` (note: its §0/§1 host-PC-in-the-loop setup wasn't
  used — this test connected over the network instead and read the dispatcher's `/ws`
  stream directly, which turned out to be a simpler and equally conclusive way to prove
  the pipeline works).
  **Bug found + fixed along the way (deploy.cpp): FIXED + REDEPLOYED, live on
  `192.168.7.4`.** `backend/src/deploy.cpp`'s `tags_sig()` matched an existing
  on-device profile file by `header.identifier.tags` alone. A composite USB device
  (this receiver) exposes a keyboard interface and a mouse interface under the *same*
  `ID_VENDOR_ID`/`ID_MODEL_ID`, so deploying the mouse profile silently overwrote the
  keyboard profile's file — only one of the two ever existed on disk, and the launcher
  only ever spawned one handler correctly. Fixed by folding `DEVICE.type` into the
  signature. Rebuilt (`./build-cross.sh zero3`), reinstalled on `192.168.7.4`, and
  re-verified live: deploying keyboard then mouse now correctly produces two separate
  files (`WirelessKB.json` + `WirelessMouse.json`) with the right content in each.
  Same-VID/PID composite devices are common (dongles, combo receivers, multi-function
  controllers) — this wasn't a synthetic edge case.

  **Second bug found during the reinstall — ROOT-CAUSED + FIXED (2026-08-12):**
  `conKeyB`/`conMouse` (the shared `EvdevDevice`/`DeviceEngine` stack, so also
  `conJoyS`/`conMIDI`) didn't shut down cleanly on SIGTERM. Root cause:
  `zmq_coms`'s heartbeat/io REQ sockets did a fully blocking `recv()` with no
  timeout (`LowLevel/Common/src/zmq_coms.cpp`); `install-on-device.sh` stops the
  dispatcher *before* the per-device handler units, so a handler's next
  heartbeat/io round trip waited forever for a reply that would never come, and
  `stopEngine()` hung inside `thcoms->join()`/`io_thread->join()` until systemd's
  ~90s `TimeoutStopSec` gave up and SIGKILLed it — leaving the auto-generated unit
  wedged (`Loaded: error ... Device or resource busy`) needing manual
  `systemctl reset-failed` + `daemon-reload` + `start` to recover.

  Fixed with `ZMQ_RCVTIMEO` (1s) on the hb/io/un REQ sockets plus reconnecting on
  a fresh socket after a timeout (a REQ socket that timed out mid-reply is still
  "owed" a recv — `send()`-ing again on the same socket throws instead of
  working). That fix then unmasked a **second, previously-unreachable bug**:
  `DeviceEngine::stopEngine()` calls `com->die()` explicitly and then `delete
  com`, whose destructor calls `die()` again — joining the same `std::thread`
  twice, which is undefined behaviour and which libstdc++ turns into a
  `std::system_error("Invalid argument")` abort. It never fired before because
  the first `join()` always hung forever first, so this line was never reached.
  `zmq_coms::die()` now guards with `joinable()` so a second call is a no-op.

  Verified live on `192.168.7.4`: before either fix, reinstalling with the
  wireless keyboard+mouse handlers running took 3m19s (each unit hanging ~90s
  before SIGKILL, then wedged, needing manual recovery); with only the first fix,
  the reinstall was fast but the handler crashed via the double-join instead;
  with both fixes, a clean `systemctl stop` of both units completes in ~0.25s
  with `Deactivated successfully` in the journal — no hang, no crash — and the
  handlers restart and keep reporting real keyboard/mouse input correctly
  afterward.
- **Delete/undeploy round-trip. VERIFIED (2026-08-11)** on `192.168.7.4`: exercised the
  console's exact flow (`DELETE /board/<id>` then `POST /undeploy`) against a disposable
  synthetic board (unique fake `header.identifier.tags` so it could never match real
  hardware, plus a dummy systemd unit standing in for a handler). Result: DB row 204 →
  404, `boards/*.json` profile removed, systemd unit stopped+disabled+deleted, no
  residue left on the board. No bug found — this path works as designed.
- **DJ-Tech-4-Mix events reach the monitor** — the conMIDI open-retry fix
  (`LowLevel/Common/include/runDevice.hpp`) should make the handler recover from the
  transient ALSA-port-busy failure that used to leave it inert after a redeploy
  restart; confirm on a board. **Still pending (2026-08-11): hardware wasn't
  available this session — pinned for next time.**

## How to build / install / deploy / test

```sh
./build-cross.sh zero3            # -> dist/zero3/conboard-zero3.tar.gz

# on the board (no apt, no compile; bundled libs; installs nginx site if nginx present)
scp dist/zero3/conboard-zero3.tar.gz <board>:~
tar xzf conboard-zero3.tar.gz && cd conboard
sudo ./uninstall-on-device.sh --purge   # clean slate recommended after a schema/DB change
sudo ./install-on-device.sh

# verify
curl -s localhost:8080/healthz   # backend direct
curl -s localhost/healthz        # via nginx (same origin as UI)
open http://<board-ip>/          # the console
sudo journalctl -u backend -f    # NOTE: needs sudo (backend runs as root)
```

## Endpoints

`/api/v1/{board,mode,rule,trigger,output_action}[/<id>]` · `POST /api/v1/deploy` ·
`POST /api/v1/undeploy` · `GET /api/v1/devices` · `GET /healthz`. nginx serves the
console at `/` and proxies `/websocket` → dispatcher `:40080` directly (the backend's
own unimplemented `/ws` relay stub was removed 2026-08-13, see `backend/README.md`).
REST is credential-gated (`X-User: <entity>`, `X-Pswd: <hash>`);
hash = `9f20d5d43738774941f9898b22cf2cf2` (bumped when the `hmi_binding` table was
added; regen via `backend/generate.sh`).

## Don't-relearn facts

- **harpia is a black box.** Regenerate: `backend/generate.sh` (SQLite default;
  `HARPIA_DB_BACKEND=postgresql` for a future central host). Never hand-edit
  `backend/generated/`. `.harpia` authoring constraints (enums in root file, must import
  an `Include/` module, punctuation-plain ASCII comments) — see `backend/harpia/README.md`.
- **`backend/generated/` IS committed source, not build output — do not gitignore
  it.** It's regenerated wholesale (harpia cleans the output dir each run), so a
  half-regenerated tree (old-hash files still committed alongside new ones) causes
  duplicate-symbol link errors. If you regenerate, `git status` should show the old
  hash's files as deleted and the new hash's as added — commit both sides together.
- **Realtime path** runs from `/conboard/boards/*.json`, decoupled from the DB by design;
  the launcher matches a device to a profile by `header.identifier.tags`.
- **Build**: emulated arm64 via QEMU; backend build `-j` capped (`BACKEND_JOBS`,
  currently 8 — many-core hosts OOM the emulated protobuf/gRPC compiles at higher
  values → exit 126).
- **CRLF**: `.gitattributes` forces LF; a fresh Windows/WSL clone must renormalize
  (`git config core.autocrlf false && git rm --cached -r . && git reset --hard`).
- Frontend↔harpia JSON: camelCase fields, `ID<hash>` caller-assigned PK unique per table,
  enums as names, zero values omitted. Mapping in `frontend/console/src/api/{harpia,map,client}.ts`.
