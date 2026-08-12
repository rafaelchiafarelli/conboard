# conboard — local screen/buttons/encoders UI handoff (2026-08-10/11)

New, **separate** workstream from the console-fixes milestone below — a small
SPI TFT + 2 push buttons + 2 rotary encoders (each with its own pushbutton),
wired directly to the Orange Pi. Deliberately independent of the rest of the
codebase (see the plan doc from the session that started this): no
`libcommon.so`, no `DeviceEngine`, no udev/launcher path, and it gets **all**
domain data over the backend's REST/JSON API — no local business logic.

Scope was phases 1-2 of a 5-phase plan (dependencies + a screen-size-adaptable
base component layer); phases 3-5 (visual theming to match the console,
WiFi/activation/radio screens, deeper backend integration) are not started.

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
- Phases 3-5 not started: visual theme matching `frontend/console/src/
  index.css`'s palette, the actual WiFi/activation/radio screens, and which
  physical control does what (nav scheme) are all open.

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
| `CONHMI_REST_PSWD_HASH` | `1bf812ac18b80d4a5ea4d51e6bfb7f58` | matches backend's compile-time hash |
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
investigation's salvageable bits (DJTech-4-Mix.csv, boards/blender.cmds, DevInspector).
Tagged `milestone-2026-08-10`. Pushed to `origin/main`. Full history is normalized to
one git identity (`rafael.chiafarelli@gmail.com`) — a mixed-identity repo was the
original trigger for that cleanup; a `backup/blender-investigation-2026-08-10` branch
holds the raw pre-cleanup local work in case anything in it is ever worth a second look.

See the top-level [README.md § What's built](../README.md#whats-built) for the feature
list. This file is the **live punch list** — what's known broken or unverified.

## Open bugs

- **`uninstall-on-device.sh --purge` is not fully reliable** (reported after installing
  this milestone on real hardware). Not yet root-caused — start by comparing what
  `--purge` actually removes (`docker/uninstall-on-device.sh`) against what
  `install-on-device.sh` and the launcher actually create (stale per-device
  `.service` units, `/conboard/backend/data` SQLite file, `boards/*.json`). Get a
  precise repro (what's left behind, on a board that had *what* installed) before
  changing anything.

## Proposed feature (not started, sized 2026-08-11): synthetic 1:1 keyboard rules on hotplug

Idea from the user: when a new keyboard is plugged in, auto-generate a full 1:1
rule set (every `KEY_*` press → the same key typed) and write it straight into the
rules DB — no HID-gadget/output-engine changes, purely populating
`board`/`mode`/`rule`/`trigger`/`output_action` via the existing harpia CRUD.
**Sized short** — deliberately scoped to skip the much bigger mouse/joystick HID
passthrough idea (see below) that prompted this.

Why it's short:
- The DB-writing side needs nothing new: harpia already generates full CRUD for
  `board`/`mode`/`rule`/`trigger`/`output_action`, exercised directly this session
  (`POST /api/v1/board` etc.) with no issues.
- The only new artifact is a static `KEY_*` → keyboard-output mapping table
  (~100 entries, one-time, mechanical — data, not engineering).
- Generating the rule set is then: for each `KEY_*` in the table, insert one
  `trigger` (press, that code) + one `output_action` (type keyboard, same key) +
  one `rule` linking them, under a `mode` under a `board` row for that device — all
  through calls the backend already exposes.

Open design question (needs a decision before writing code, not a difficulty
issue): **where does "a new keyboard was plugged in" get noticed and trigger
this?**
- The **launcher** (udev-triggered C++, `LowLevel/launcher/`) calls the backend
  REST API when it sees an unmatched keyboard-class device — closer to "fully
  automatic," but new territory: the launcher today only touches
  `boards/*.json` + systemd, never the DB (deliberately decoupled, see
  "Don't-relearn facts" below).
- The **console** does it explicitly — e.g. a "seed 1:1 rules" button in the
  existing Add-Device dialog, reusing the `GET /devices` inventory already there.
  Less new code, fits the current architecture cleanly, costs a click instead of
  being silent.

Either route also wants simple dedup (don't regenerate ~100 rows every time the
same dongle reconnects) — a lookup-before-insert keyed on VID/PID or serial.

**Related, explicitly NOT this feature — true mouse/joystick 1:1 HID passthrough**
(the bigger idea that came up first, deferred as out of scope for now): would need
real HID output, not just DB rows. Checked this session:
`oActions::mouse_fill_report`/`joystick_fill_report` (`LowLevel/Common/src/oActions.cpp`)
already build correct report bytes, but the dispatch functions that would call them,
`oMouse()`/`oJoystick()` (`LowLevel/Common/include/oActions.hpp`), are literal empty
stubs (`virtual void oMouse(mouseActions){}`) — never wired to anything. Bigger gap:
the USB gadget composite (`scripts/usb-composite-all.sh`) only declares **one** HID
interface, hardcoded as a keyboard (protocol=1, boot-keyboard report descriptor) — no
mouse/joystick HID function exists in the gadget at all yet. Sized medium (new gadget
HID function + wiring + real OTG-to-host-PC verification, which is untested territory
beyond keyboard), not attempted this session.

## Still needs a dispatcher-side change

- **O1 — HTTP port inconsistency. RESOLVED (2026-08-11).** Dispatcher now reads its
  HTTP port from `config.json` (`dispatcher::GetHTTPPort()`) instead of hardcoding
  `app.port(40080)`; `config.json`'s `http.port` was changed from `9080` to `40080` to
  match what nginx and every deployed board already use. Not yet rebuilt/redeployed to
  the board — do that before relying on it live.
- **O5 — heartbeat/roster frame. RESOLVED (2026-08-12).** Dispatcher now emits
  `HB,<uuid>,<devname>` ~1/s per live sender on `/ws`
  (`dispatcher::GetHeartbeats()` + `user_handler` in `LowLevel/dispatcher/src/
  {dispatcher,main}.cpp`), reusing the existing `devices`/`last_ping` maps — "live"
  means a ping within 5s. Verified via a full `./build-cross.sh zero3` (compiles
  clean). **Not yet rebuilt/redeployed to a real board or checked against the
  console's live view with an actual device** — do that before trusting the
  per-device liveness LEDs / device-name filtering in practice.

## Still needs on-board verification

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

  **Second bug found during the reinstall (NOT yet fixed — new, open):**
  `conKeyB`/`conMouse` (the shared `EvdevDevice`/`DeviceEngine` stack, so likely
  `conJoyS` too) don't shut down cleanly on SIGTERM. `install-on-device.sh`'s
  "stopping any running conboard services" step sent the normal stop signal, and both
  handlers hung for the full ~90s `systemd` `TimeoutStopSec` before being SIGKILLed
  (`journalctl`: `State 'stop-sigterm' timed out. Killing.`). This left both
  auto-generated units in a wedged `Loaded: error ... Device or resource busy` state
  needing manual `systemctl reset-failed` + `daemon-reload` + `start` to recover — a
  real end-user hitting this on a normal reinstall/upgrade would see handlers silently
  fail to come back. Likely cause: `EvdevDevice::Stop()` / `DeviceEngine::stopEngine()`
  join threads that are blocked on a synchronous `read()` (evdev fd) or a zmq call that
  doesn't check the `stop` atomic promptly — needs the in-thread read to be interrupted
  (e.g. close the fd first, or a poll/timeout loop) rather than relying on a blocking
  read to return on its own. Not root-caused or fixed this session — next session
  should start from `LowLevel/Common/src/evdevDevice.cpp` (`Stop()`) and
  `deviceEngine.cpp` (`stopEngine()`).
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
`POST /api/v1/undeploy` · `GET /api/v1/devices` · `GET /healthz` · `GET /ws` (backend
relay seam, unused). nginx serves the console at `/` and proxies `/websocket` →
dispatcher `:40080`. REST is credential-gated (`X-User: <entity>`, `X-Pswd: <hash>`);
hash = `1bf812ac18b80d4a5ea4d51e6bfb7f58` (bumped when `midi_mode` was added to the
trigger message; regen via `backend/generate.sh`).

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
