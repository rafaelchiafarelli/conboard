# conboard — branch cleanup, mode management, keyboard-only output (2026-08-19)

Three threads this session, full writeups in [../NOTES.md](../NOTES.md):

**Branch consolidation.** ~20 remote branches audited by ancestry, not by
name — only `dev` + 2 feature branches were actually live, the other ~17
were a single dead chain forked from `main` back in 2022 (before the current
rewrite) plus one unrelated old prototype. Tag-archived and deleted; merged
the 2 live branches into `dev` (the MIDI one needed a real harpia schema
merge + regen, not just git). `main` stays the stable rollback point, `dev`
is where work happens now.

**Mode management.** Console could only ever have one mode per device, for
every device type — found live, walking through the console. Added `+ mode`
/ `🗑 Delete mode` controls (`App.tsx`) with guards so a device is never left
with zero modes or zero *active* modes, plus an optional customer-facing
`name` field on the `mode` entity (schema change, regen, domain hash bumped,
threaded through every place a mode is shown in the console). The
matching/mode-switch engine underneath was already generic across
keyboard/mouse/joystick/MIDI — this was purely a missing console control,
no `LowLevel/` changes needed. Cross-build clean (zero3), browser-verified
with Playwright; **not yet exercised against a real deployed backend** (see
`NOTES.md`'s "Next" list).

**Keyboard-only output.** Dropped mouse as an addable output-action type
(`RuleEditor.tsx`) — no HID mouse gadget exists and no fixture board used it.
Checked before touching MIDI output too: it's not "output to the PC" at all,
it's feedback to the MIDI device itself (Arduino Micro's own LED boot-chase),
and 203 real output blocks depend on it — left untouched.

---

# conboard — identical-MIDI-device separation (2026-08-16)

Session doc `docs/next-sessions/09-midi-identical-device-separation.md` removed
(Task items 1-3 done). Full writeup in [../NOTES.md](../NOTES.md) under "FIXED,
not hardware-verified — identical-MIDI-device separation" — short version: the
launcher never gave MIDI per-instance service naming (excluded MIDI in the
`isEvdev` check, so two identical controllers shared one systemd service and
only one `conMIDI` process ever ran), now fixed the same way evdev already
handles identical clones (USB-devpath binding). Code lands + unit-tested +
cross-compiles clean; **no MIDI hardware was reachable this session** (not even
a single unit), so neither the dual-unit separation nor the single-unit
regression path has been proven live yet — do that first with real hardware
(tracked in `NOTES.md`'s "Next" list).

---

# conboard — HMI phase 4a merged + deployed (2026-08-16)

`feat/hmi-phase4a-and-1to1-rules` merged into `main`, then built and deployed
to `192.168.7.4`. Both features on that branch (synthetic 1:1 keyboard rules,
HMI WiFi list screen) were already hardware-verified before merging — this
deploy's own new finding is that the branch's `hmi_binding` DB table migrated
onto the board's existing database with zero data loss (plain reinstall, no
`--purge` needed — additive `CREATE TABLE IF NOT EXISTS` only), and that
`hmi.service` runs the new binary cleanly on real hardware: encoders/buttons
still correctly report unwired, and a `/simulate`-driven button press resolved
correctly through the real `hmi_binding` table with no crash. Full writeup in
[../NOTES.md](../NOTES.md).

Activation screen (built on the same branch) is intentionally not being pushed
further here — Rafael is designing its real GUI/UX separately. Radio screen,
encoder/button physical wiring, and the nav-scheme decision are still open,
tracked in `docs/next-sessions/06-hmi-phase4b.md` (unchanged by this session).

A `dev` branch now exists (off this merge) as the integration target going
forward — this writeup is the first thing on it, not `main`.

---

# conboard — ethernet-gadget access, auto-fallback landed (2026-08-16)

Adding a USB-ethernet gadget (ECM now, RNDIS later) hit a hardware wall on the
Zero 3: its musb-hdrc USB controller only has endpoint budget for 4 IN + 2 OUT
total, which the current gadget (ACM+HID+mass-storage) already uses in full —
so a network function can only be added by *dropping* HID or mass-storage, not
adding on top. Confirmed live via configfs bind tests (`unable to autoconfigure
all endpoints`, kernel `-524`).

Rather than hand-picking function sets per board, `scripts/usb-composite-all.sh`
now tries the full gadget (ACM+ECM+HID+mass-storage) first and auto-falls-back
to today's reduced gadget (no network) if the UDC bind fails — recognizing the
endpoint-budget limit live rather than needing a static board table.
`usb-gadget-dhcp.service` (scoped `dnsmasq` on `usb0`) only starts when the
fallback script found room for the network function
(`ConditionPathExists=/run/conboard/usb-gadget-network`), and
`scripts/conboard-firewall.sh` now allows `udp/67` on `usb0` for DHCP.

**Hardware-verified on `192.168.7.4`**: full gadget correctly fails and falls
back, gadget reaches `configured` with the same ACM+HID+mass-storage shape as
before, `/run/conboard/usb-gadget-network` correctly absent, and
`usb-gadget-dhcp.service` reports a clean systemd "skipped" (not failed) via
its `ConditionPathExists`. Rest of the stack (dispatcher/backend/firewall)
confirmed still healthy after. **Not yet verified**: the *full* scenario
(network actually working end-to-end) — needs a board whose USB controller has
enough endpoint headroom to take the "full gadget" branch instead of falling
back. Rafael has a second, non-Zero Orange Pi lined up for that. Full writeup
+ candidate-board research in [../NOTES.md](../NOTES.md).

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
| `CONHMI_REST_PSWD_HASH` | `5a67e5f27cce34a1ec5ac267a70f5d87` | matches backend's compile-time hash |
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
