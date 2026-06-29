# Direction & scope notes

## Build/packaging wishlist — DONE
* ~~dist folder should have a "how-to-install" with the main commands~~ → `dist/<board>/HOW-TO-INSTALL.txt`
* ~~every dist folder should have a surname saying what board they should go in~~ → `dist/<board>/BOARD.txt`
* ~~adding another build destination should be visible and easy to understand~~ → `docker/boards.conf`
    * ~~list of available compatible boards~~ → `./build-cross.sh list`
    * ~~enable each build separately~~ → `./build-cross.sh <board-id>`

## Current focus — generalize input devices beyond MIDI
The MIDI input→rule→output engine is being generalized so joystick/keyboard/mouse
(all evdev) reuse it. Decisions & state recorded in detail in internal memory
(`conboard-device-generalization`); summary:
* shared, pure matchers: `midiMap` (MIDI) + `evMatch` (all evdev), unit-tested.
* `holdGen`: synthesizes hold/long-press for non-autorepeating gamepads (pure).
* `DeviceEngine`: shared output/coms/mode engine; `condev::runDevice`: shared main.
* `conJoyS`: first device on the engine; `boards/Xbox360.json` profile; launcher wired.

## Next
* HARDWARE TEST conJoyS on the board (build → deploy → plug Xbox pad → keys type on host).
* migrate MIDI onto `DeviceEngine` (base now proven by conJoyS) — keep behavior identical.
* build `conKeyB` / `conMouse` evdev readers (matcher already supports them).
* raise/relieve the 10-slot reporting-queue overflow (`STACKED_IO_MSG`) when rule traffic is heavy.
* longer term: ethernet-gadget access, a real UI (flask backend exists), security/firewall.
