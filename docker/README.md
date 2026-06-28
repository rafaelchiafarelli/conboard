# Cross-building conboard with Docker buildx

This replaces "building locally is miserable" with one command that produces
ready-to-install artifacts for both Orange Pi boards:

| Board               | SoC   | Arch    | Docker platform | Artifact                  |
|---------------------|-------|---------|-----------------|---------------------------|
| Orange Pi Zero 3    | H618  | arm64   | `linux/arm64`   | `conboard-arm64.tar.gz`   |
| Orange Pi Zero      | H3    | armhf   | `linux/arm/v7`  | `conboard-armhf.tar.gz`   |

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
./build-cross.sh                       # both arches
PLATFORMS=linux/arm64 ./build-cross.sh # just the Zero 3
```

The four steps it prints:

1. **register QEMU** — `tonistiigi/binfmt --install` (one-time, needs `--privileged`).
2. **builder** — creates a `docker-container` buildx builder (multi-platform output).
3. **submodules** — `git submodule update --init` to fetch Crow.
4. **build** — `docker buildx build --platform … --output type=local,dest=dist`.

Output lands in `dist/<platform>/`:

```
dist/
├── linux_arm64/
│   ├── conboard/               # the unpacked tree (mirrors /conboard on the board)
│   └── conboard-arm64.tar.gz
└── linux_arm_v7/
    ├── conboard/
    └── conboard-armhf.tar.gz
```

## Eyeball it before shipping

Every artifact carries a `MANIFEST.txt` — arch, ELF type per binary, the
dispatcher's shared-library needs, and sha256 of every file:

```bash
tar xzO -f dist/linux_arm64/conboard-arm64.tar.gz conboard/MANIFEST.txt
```

The `binaries` section is the quick correctness check: every line for the arm64
artifact should say `ELF 64-bit … aarch64`, and the armhf one `ELF 32-bit … ARM`.

## Install on the board

Copy the tarball to the Pi, unpack, and run the bundled installer (it installs
runtime libs, drops the tree at `/conboard`, and enables the systemd units — no
compilation on the board):

```bash
scp dist/linux_arm64/conboard-arm64.tar.gz orangepi@<board-ip>:~
ssh orangepi@<board-ip>
tar xzf conboard-arm64.tar.gz
cd conboard
sudo ./install-on-device.sh
```

Then confirm the gadget can bind:

```bash
ls /sys/class/udc        # must be non-empty (e.g. 5100000.usb on the H618)
systemctl status usb-otg.service dispatcher.service launcher.service
```

> ⚠️ **Zero 3 hardware caveat:** the H618's USB-OTG controller is left *disabled*
> in the mainline board device tree, and the USB-C is documented as power input.
> If `ls /sys/class/udc` is empty, OTG isn't wired/enabled on this board and the
> gadget can't bind — see the note in the repo's MEMORY for the test procedure.

## What's in the artifact

- Binaries: `libcommon.so`, `launcher`, `dispatcher`, `conMIDI`, `conMouse`, `conKeyB`, `conJoyS`
- systemd units: `usb-otg`, `launcher`, `dispatcher`, `frontend`
- `100-usb.rules` + `event_handler.sh` (udev hotplug)
- `scripts/` (USB gadget scripts), `boards/` (device JSON), `dispatcher` `config.json`
- `install-on-device.sh`, `MANIFEST.txt`

The python frontend (`backend/`) is not built here; set up its venv per
`scripts/install.sh` `install_frontend`.
