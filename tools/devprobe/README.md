# devprobe — portable device classifier

A standalone diagnostic that prints the **type** and **capabilities** of attached
devices using only Linux kernel ABIs. It's the first step of conboard's
environment-agnostic device detection (see [`../../NOTES.md`](../../NOTES.md)).

The classification logic lives in the shared, board-agnostic detector
[`LowLevel/Common/deviceDetect.{hpp,cpp}`](../../LowLevel/Common/include/deviceDetect.hpp)
that the **launcher also uses** — so this tool and the launcher's runtime
detection can never disagree. `devprobe` is just the human-readable front-end.

## Why it exists

The launcher's old pain was trying to infer device type/capability from USB
**VID/PID** — which is impossible, because VID/PID is an *identity* number, not a
capability description. The detector instead reads what the kernel already
parsed from the device's descriptors:

- **USB interface class** from sysfs (`/sys/bus/usb/devices/*/bInterfaceClass`) —
  e.g. HID keyboard/mouse, Audio/MIDIStreaming (MIDI), mass storage, hub.
- **evdev** (`/dev/input/event*`) via `EVIOCGBIT` ioctls — classifies
  keyboard / mouse / joystick and counts keys/axes.

None of this is board- or distro-specific, so the same logic works on the Orange
Pi Zero 3, a Raspberry Pi, or any Linux box. VID/PID is left for *identity* only
(matching a custom profile in `boards/*.json`), never for capability.

## Build

No dependencies — compile directly (it pulls in the shared classifier source),
even on the board itself:

```bash
g++ -std=c++17 -O2 -I../../LowLevel/Common/include \
    src/devprobe.cpp ../../LowLevel/Common/src/deviceDetect.cpp -o devprobe
```

Or via CMake (`cmake -S . -B build && cmake --build build`). It's also built by
the cross-build and shipped in the artifact under `tools/devprobe/`.

## Run

```bash
sudo ./devprobe                    # scan USB interfaces + all /dev/input/event*
sudo ./devprobe /dev/input/event3  # probe a specific evdev node
```

`sudo` is needed to open the `/dev/input/event*` nodes.

## Scope / roadmap

This increment covers the USB descriptor view (incl. MIDI at the interface-class
level) and the evdev view. Planned next: ALSA rawmidi/seq enumeration (MIDI port
names and in/out counts), then wiring the classifier into the launcher so udev
triggers on the classified subsystems (`input`, `sound`) instead of raw `usb`.
