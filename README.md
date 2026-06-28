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
    * detection of witch device belongs to witch json and witch .service
    * Keyboard and Mouse are standard to the PC
    * All devices can send Keyboard and/or mouse.
    * a websocket will launch any user event to the client
    * operation mode for all devices, no limit of operating modes
    * standarized language for all input devices

* midi device (IO) supported
    * Input/Output MIDI 
    * Memory kept for the current working mode
    * Event for push down and for pull up key
    * Delay in action to send to the PC
    * Multiple output to the device types (blink, normal, etc)

* keyboard device (IO) supported
    * keys are read 

* joystick device (IO) supported

    
# What is Missing?

* generalized behavior 
    * missing installing the system as an ethernet port (it would sinplify access by users)
    * detection of inserted device does not filter by device type (MIDI, Keyboard, Joystick or HID,). Always push this event to the launcher witch in turn, creates a dummy device, even if this device is a usb hub, or something that does not have any actions.
    * lauch of a service is very heavily dependent on the user configuration witch is very dangerous, user should not have that power. 
    * detection of the device is very heavily dependent on the user configuration witch is very dangerous, user should not have that power.
    * no UI yet. only a websocket 
    * no security planned yet, needs a firewall or something that could prevent outside access to the communications. 
    * backend in python-flask is deprecated, but it makes more sense since has a lot of features that would speed up the process
* midi device
    * SysEx commands are not working
    * multiple commands and multiple actions can overrun the system.
* keyboard device 
    * not even working properly
* joystick device
    * not even started.

    

    

