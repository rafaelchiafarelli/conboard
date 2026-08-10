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

## Next
* HARDWARE TEST the evdev stack (conJoyS/conKeyB/conMouse) on a board — built and
  unit-tested, never exercised on real hardware. See `docs/HW-TEST-evdev.md`.
* dispatcher: emit the `HB,<uuid>,<devname>` heartbeat/roster frame the console's
  live view wants (`INTERFACE.md` O5, `NEEDS ACK`) and settle the HTTP port
  inconsistency (O1).
* fix `uninstall-on-device.sh --purge` (reported not fully reliable).
* raise/relieve the 10-slot reporting-queue overflow (`STACKED_IO_MSG`) when rule traffic is heavy.
* longer term: ethernet-gadget access, the local power-password login (design in
  `backend/README.md`, never implemented), security/firewall.
