# conboard
Control Surface based on the Orange Pi.
Most of the usb-otg ware used from https://github.com/dpavlin/usb-otg
It is a good project and you definetly should check it out.

## Boards

conboard runs on Linux SBCs with working USB-OTG / USB-gadget support. Current
target is the **Orange Pi Zero 3** (Allwinner H618, arm64); the original Orange
Pi Zero (H3, armhf) is also supported. Supported build targets are listed in
[`docker/boards.conf`](docker/boards.conf) — adding a board is a one-line edit.

## Build (cross-compile — no local toolchain needed)

Requires only Docker with `buildx`. The build runs the C++ (`LowLevel`) for the
target architecture inside an emulated container, so you don't install a cross
toolchain.

```bash
./build-cross.sh list      # show supported boards
./build-cross.sh zero3     # build one board  -> dist/zero3/
./build-cross.sh           # build all boards
```

Each `dist/<board>/` holds the installable `conboard-<board>.tar.gz`, a
`BOARD.txt` (which board it's for), a `HOW-TO-INSTALL.txt` (the on-device
commands), and a `MANIFEST.txt`. Full walkthrough: [docker/README.md](docker/README.md).

## Test

Unit tests are pure-logic with synthetic inputs (no hardware):

```bash
./run-tests.sh                 # all, natively
./run-tests.sh joystick        # one suite
./run-tests.sh --qemu zero3    # same tests on the target arch, under emulation
```

See [tests/README.md](tests/README.md).

## Install on the board

Copy `dist/<board>/conboard-<board>.tar.gz` to the device, unpack, and run
`sudo ./install-on-device.sh` (no compilation on the board). The exact commands
are in that board's `HOW-TO-INSTALL.txt`.

## Diagnosing attached devices

`tools/devprobe` classifies attached USB/input devices from Linux kernel ABIs
(not VID/PID) — handy when bringing up a new controller. See
[tools/devprobe/README.md](tools/devprobe/README.md).

> Direction & scope notes live in [NOTES.md](NOTES.md).

# What is done?


* generalized behavior
    * device installed as a keyboard, a mouse, a joystick and a PenDrive (bootable storage)
    * detection of inserted device with udev
    * launch of a service identified with json or a dummy service with a json constructed with udev variables
    * detection of which device belongs to which json and which .service
    * **device detection by kernel ABIs** (evdev / USB class), not VID/PID; VID/PID is identity-only (`tools/devprobe`)
    * **hubs / non-actionable devices are skipped** instead of getting a dummy handler (`isActionableInterfaces`); a device that matches a known profile is always handled even if vendor-specific (e.g. Xbox)
    * Keyboard and Mouse are standard to the PC
    * All devices can send Keyboard and/or mouse.
    * a websocket will launch any user event to the client
    * operation mode for all devices, no limit of operating modes
    * **standardized rule language**: MIDI triggers (`b0/b1/b2` + mode) and evdev triggers (symbolic `BTN_*`/`KEY_*`/`ABS_*`/`REL_*` + mode) map to a shared `out[]` of keyboard/mouse/midi actions
    * **shared device engine**: `DeviceEngine` (output queue + zmq coms + modes) and `condev::runDevice` (per-device main) factored into `LowLevel/Common`; each handler is its own process
    * **unit-test harness** (doctest, pure logic / synthetic inputs): 55 cases across classifier, usb, joystick, midi, json, launch, filter, evmatch, holdgen

* midi device (IO) supported — HARDWARE-VERIFIED
    * Input/Output MIDI 
    * Memory kept for the current working mode
    * Event for push down and for pull up key
    * Delay in action to send to the PC
    * Multiple output to the device types (blink, normal, etc)

* joystick / keyboard / mouse (input) — BUILT, pending hardware test
    * one shared `EvdevDevice` engine (on `DeviceEngine`) serves `conJoyS` / `conKeyB` / `conMouse`; a new device is a ~45-line main
    * pure `evMatch` matcher (press / release / hold / hold_once / higher / lower / spot), symbolic triggers (`BTN_*`/`KEY_*`/`ABS_*`/`REL_*`)
    * `holdGen` synthesizes all hold events uniformly (rule-timed repeat + long-press); kernel autorepeat is dropped so behaviour is identical across device types
    * self-discovers its `/dev/input` node; per-device separation by physical port or trustworthy serial; handles device moves between ports
    * starter joystick profile: `boards/Xbox360.json` (keyboard/mouse profiles are per-device, add like that one)

    
# What is Missing?

* generalized behavior 
    * missing installing the system as an ethernet port (it would simplify access by users)
    * launch/detection of a service is heavily dependent on user configuration, which is dangerous — the user should not have that power
    * no UI yet. only a websocket 
    * no security planned yet, needs a firewall or something that could prevent outside access to the communications. 
    * backend in python-flask is deprecated, but it makes more sense since it has a lot of features that would speed up the process
* midi device
    * SysEx commands are not working
    * multiple commands and multiple actions can overrun the system (pre-existing 10-slot reporting-queue overflow, `STACKED_IO_MSG`)
    * not yet migrated onto the shared `DeviceEngine` (still uses its own orchestration)
    * identical-MIDI separation not done — it's ALSA (binds by name), so it needs ALSA-card binding rather than the evdev per-port mechanism
* joystick / keyboard / mouse (input)
    * built + unit-tested + compiles, but **not yet exercised on real hardware**
    * keyboard / mouse need per-device json profiles (none shipped yet — add like `boards/Xbox360.json`)

    

    

