# HW-test runbook — evdev stack (conJoyS / conKeyB / conMouse)

The evdev input stack (shared `EvdevDevice` on `DeviceEngine`, plus the three thin
handlers `conJoyS` / `conKeyB` / `conMouse`) is **built and cross-compiled but never
run on hardware.** This is the step-by-step to prove it on the Orange Pi Zero 3, so
`feature/evdev-matcher` can then merge to `main`.

> Runs on the board + a host PC connected to the Zero 3's USB-C (the gadget acts as a
> USB keyboard on that host). Keep a text editor focused on the host — the gamepad
> "types" into it. Nothing here needs the network.

---

## 0. Deploy the build

The tarball is already cross-built (`dist/zero3/conboard-zero3.tar.gz`, aarch64,
links `libcommon` via `$ORIGIN`). Only rebuild if code changed since:
`./build-cross.sh zero3`.

```bash
# on the host
scp dist/zero3/conboard-zero3.tar.gz  <user>@<zero3>:~
# on the board
tar xzf conboard-zero3.tar.gz
cd conboard-zero3            # (or whatever the tarball unpacks to)
sudo ./install-on-device.sh
```

Bring up the USB gadget (HID keyboard on the host). Confirm the UDC enumerates:
```bash
cat /sys/class/udc/*/state        # expect: configured   (host has enumerated us)
```
If not `configured`, fix the gadget first (see `conboard-zero3-otg-verdict`); the
evdev test is meaningless until the host sees the keyboard.

---

## 1. Single Xbox pad → conJoyS

### 1a. Confirm the pad's USB IDs match the profile
`boards/Xbox360.json` matches on `ID_VENDOR_ID=045e` + `ID_MODEL_ID=028e` (Xbox 360
**wired**). Verify your actual pad:
```bash
lsusb | grep -i xbox        # or:  lsusb -d 045e:
```
- `045e:028e` → matches as-is. Good.
- Anything else (Xbox One `02dd`/`02ea`, Series `0b12`, a wireless receiver, a clone)
  → edit `ID_MODEL_ID` in `boards/Xbox360.json` to the real PID, redeploy the file, or
  the launcher won't spawn `conJoyS`.

### 1b. Plug it in and watch the launcher react
```bash
# watch the launcher/handler come up (in one shell)
journalctl -f | grep -Ei 'conboard|conJoyS|launcher|event_handler'
```
Plug the pad. Expected chain: udev fires → `event_handler.sh` → launcher matches
`Xbox360.json` → spawns a service that runs `conJoyS -x <profile> -d <usb-devpath>`.
Confirm it's alive:
```bash
systemctl list-units | grep -i xbox        # a "<DevName>-<identity>.service" is running
systemctl status 'Xbox360-*.service'
```
`conJoyS` self-discovers the `/dev/input/eventN` node from the profile's
`"input": "Microsoft X-Box 360 pad"` name (or the pad under `-d`'s port).

### 1c. Exercise the rules (mode 0 of Xbox360.json)
With a text editor focused on the **host**, press each control and confirm the output:

| Control | Trigger | Expected on host |
|---------|---------|------------------|
| A button | `BTN_SOUTH` press | types `a` |
| B button | `BTN_EAST` press | types `b` |
| X button | `BTN_WEST` press | types `x` |
| Y button | `BTN_NORTH` **release** | types `y` **when you let go** (not on press) |
| Left bumper | `BTN_TL` **hold** (interval 150ms) | repeats `l` while held |
| Right bumper | `BTN_TR` **hold_once** (delay 800ms) | types `right bumper long press` once, ~0.8s after press |
| Start | `BTN_START` press | types `sudo systemctl status ` |
| Right trigger | `ABS_RZ` **higher** (value 200) | types `r` when RT pressed past ~200 |

The three things this proves that unit tests can't: the real `/dev/input` reader, the
holdGen tick timing on a real gamepad (`hold` repeat + `hold_once` one-shot), and the
analog-trigger threshold (`ABS_RZ higher`).

**If nothing types:** check (a) the service is running (1b), (b) the gadget is
`configured` (step 0), (c) `conJoyS` opened a node — its log prints
`evdev: reading /dev/input/eventN` or `evdev: no usable node`. If "no usable node",
the profile `input` name didn't match `EVIOCGNAME`; get the real name with
`cat /proc/bus/input/devices` (look for the pad's `Name=`) and fix the profile.

---

## 2. Two pads → separation + move (identical VID/PID)

Tests that two pads reporting the same `045e:028e` stay separate, keyed by identity:
a **genuine** MS pad (trustworthy serial, e.g. `25A477C`) is **serial-keyed** (follows
it across ports); a **clone** (fake serial `FFFFFFF`/empty/all-0) is **port-keyed**
(separated by physical USB port).

1. Plug **both** pads. Expect **two** services with distinct identity-based names:
   ```bash
   systemctl list-units | grep -i xbox      # e.g. Xbox360-ser-25A477C.service + Xbox360-port-<token>.service
   ```
   Confirm each responds independently (both type when pressed).
2. **Move test:** unplug one, replug it into a **different** port.
   - Genuine (serial-keyed): same service identity should resume — it followed the pad.
   - Clone (port-keyed): a new port → a new `-port-<token>` service; per-device mode
     state resets (expected — a clone with a fake serial is indistinguishable from a
     different pad).
   Watch removal actually stop the right service:
   ```bash
   journalctl -f | grep -Ei 'stop_device|conJoyS|remove'
   ```
   (This exercises the `stop_device` rewrite — remove events carry only `DEVPATH`, so
   the service is found by the `-d <devpath>` in its `ExecStart`.)

---

## 3. Keyboard + mouse (conKeyB / conMouse)

Handlers are built but have **no profiles yet** (need real VID/PIDs). Templates are in
[`docs/profile-templates/`](profile-templates/). For each device:

1. Get its IDs and evdev name:
   ```bash
   lsusb                              # note VID:PID
   cat /proc/bus/input/devices        # note the Name= of the keyboard/mouse
   ```
2. Copy the template into `boards/`, fill in `ID_VENDOR_ID` / `ID_MODEL_ID` / `name` /
   `input`, and (re)deploy the file to `/conboard/boards/` on the board:
   ```bash
   cp docs/profile-templates/keyboard.json boards/MyKeyboard.json   # then edit
   cp docs/profile-templates/mouse.json    boards/MyMouse.json      # then edit
   ```
3. Plug the device → launcher spawns `conKeyB -x <profile> -d <port>` (or `conMouse`).
   With a host editor focused, confirm the template's starter rules fire (e.g. `KEY_A`
   press → types something; mouse `BTN_LEFT` press / `REL_X` motion → its mapped output).

> Note: mouse/keyboard **output** rules that emit `mouse`/`joystick` HID are currently
> no-ops (`oActions::oMouse`/`oJoystick` are empty stubs); keyboard output
> (`keyboard_send`) is the real, wired path. So map template rules to **keyboard**
> outputs to see something happen.

---

## 4. After it passes

Per the workflow rule (merge to `main` only after hardware confirmation):
```bash
git checkout main
git merge feature/evdev-matcher
```
Then the MIDI→DeviceEngine migration (`refactor/midi-deviceengine`) can rebase onto the
updated `main` and get its own MIDI hardware re-test before merging.

---

## Gotchas (each cost a debug cycle before — check if something's off)
- **`event_handler.sh` path:** the udev rule runs `/conboard/event_handler.sh`; the
  installer must place it there (rsync alone leaves it under `LowLevel/assets/`).
- **UDC name:** H618 is `musb-hdrc.5.auto` — auto-detected from `/sys/class/udc`, never
  hardcoded.
- **`$ORIGIN` RUNPATH:** cross-built binaries find `libcommon.so` relative to
  themselves; if a handler won't start with a linker error, check `ldd` and `ldconfig`.
- **Composite gadget:** `usb-composite-all.sh` must tear down an existing `g1` gadget
  before rebuilding, else "Device or resource busy".
- **Services that don't exit:** handlers are `Type=simple` daemons; don't `enable --now`
  a scan-and-exit oneshot.
