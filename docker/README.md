# Cross-building conboard with Docker buildx

This replaces "building locally is miserable" with one command that produces a
ready-to-install artifact per **board**. Boards are declared in
[`boards.conf`](boards.conf) — adding a target is a one-line edit there. See the
current list with:

```bash
./build-cross.sh list
```

Each board maps to a Docker platform/arch; several boards can share an arch
(e.g. the Zero 3 and a 64-bit Raspberry Pi are both `arm64`), but each gets its
own clearly-labelled output folder.

## How it works

It's an **emulated native build**, not a cross-toolchain. `docker buildx` spins
up an arm64 (and an armhf) Debian container, QEMU emulates the CPU, so:

- `apt install` pulls correctly-arched libraries, and
- the stock `gcc`/`cmake` in the container *is* the target compiler.

That means [`LowLevel/scripts/compiler.sh`](../LowLevel/scripts/compiler.sh) runs
**unchanged** inside the container — the same recipe you'd run on the board, just
faster and reproducible. The trade-off is speed (emulation is slower than a real
cross-toolchain), which is fine for an occasional packaging build.

## Prerequisites

- Docker 23+ with `buildx` (run `docker buildx version` to confirm).
- That's it. The wrapper registers the QEMU emulators and creates the builder.

## Build

```bash
./build-cross.sh             # build ALL boards in boards.conf
./build-cross.sh zero3       # build one board
./build-cross.sh zero3 rpi64 # build several
./build-cross.sh list        # show the board registry
```

It registers the QEMU emulators (one-time, `--privileged`), ensures a
`docker-container` buildx builder, inits the Crow submodule, then builds each
selected board (`docker buildx build --platform … --output type=local`).

Output lands in `dist/<board-id>/`, one self-describing folder per board:

```
dist/
└── zero3/
    ├── conboard-zero3.tar.gz   # the installable artifact
    ├── BOARD.txt               # which board this is for ("surname") + arch + OTG status
    ├── HOW-TO-INSTALL.txt       # the main install commands, ready to copy/paste
    └── conboard/               # the unpacked tree (mirrors /conboard on the board) + MANIFEST.txt
```

## Eyeball it before shipping

- `BOARD.txt` tells you which board the folder is for at a glance.
- `HOW-TO-INSTALL.txt` has the exact install commands for that board.
- `MANIFEST.txt` (inside `conboard/`) lists the board, arch, ELF type per
  binary, the dispatcher's shared-library needs, and sha256 of every file:

```bash
cat dist/zero3/BOARD.txt
tar xzO -f dist/zero3/conboard-zero3.tar.gz conboard/MANIFEST.txt
```

The `binaries` section is the quick correctness check: every line for an arm64
board should say `ELF 64-bit … aarch64`, and an armhf one `ELF 32-bit … ARM`.

## Install on the board

`HOW-TO-INSTALL.txt` spells out the commands for the specific board; in general:

```bash
scp dist/zero3/conboard-zero3.tar.gz orangepi@<board-ip>:~
ssh orangepi@<board-ip>
tar xzf conboard-zero3.tar.gz
cd conboard
sudo ./install-on-device.sh
```

Then confirm the gadget can bind:

```bash
ls /sys/class/udc        # must be non-empty (e.g. 5100000.usb on the H618)
systemctl status usb-otg.service dispatcher.service launcher.service backend.service
curl -s localhost:8080/healthz   # backend management API, direct
curl -s localhost/healthz        # via nginx — same origin as the console UI
open http://<board-ip>/          # the console
```

> ⚠️ **Zero 3 hardware caveat:** the H618's USB-OTG controller is left *disabled*
> in the mainline board device tree, and the USB-C is documented as power input.
> If `ls /sys/class/udc` is empty, OTG isn't wired/enabled on this board and the
> gadget can't bind — see the note in the repo's MEMORY for the test procedure.

## What's in the artifact

- Binaries: `libcommon.so`, `launcher`, `dispatcher`, `conMIDI`, `conMouse`, `conKeyB`, `conJoyS`
- The C++ **backend** (`conboard_backend`) — management REST/gRPC API over an
  embedded SQLite rules library, built via a separate `cmake`/`ninja` pass in the
  same Dockerfile stage (see `docker/Dockerfile`'s `backend/` block). Its runtime
  `.so` closure is bundled into `lib/` alongside the device handlers' — the whole
  artifact needs **no `apt install` on the board**.
- The built **console** SPA (`frontend/console/`, Vite + React) — compiled in its own
  arch-independent Dockerfile stage (`FROM node:22-alpine AS frontend`, runs on the
  build host, not under QEMU) and copied into `frontend/` in the artifact.
- systemd units: `usb-otg`, `launcher`, `dispatcher`, `backend`
- nginx site (`backend/assets/interface.conf`) — serves the console SPA at `/` and
  reverse-proxies `/api/v1` + `/ws` to the backend and `/websocket` to the dispatcher.
  Installed only if `nginx` is already present on the board (`install-on-device.sh`
  warns and continues headless if not).
- `100-usb.rules` + `event_handler.sh` (udev hotplug)
- `scripts/` (USB gadget scripts), `boards/` (device JSON), `dispatcher` `config.json`
- `install-on-device.sh`, `MANIFEST.txt`
