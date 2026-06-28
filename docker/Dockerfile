# Cross-compile conboard's C++ (LowLevel) for arm64 (Orange Pi Zero 3 / H618)
# AND armhf (Orange Pi Zero / H3) from this single Dockerfile.
#
# Strategy: emulated *native* build. `docker buildx` + QEMU runs an arm64/armhf
# Debian container per --platform, so `apt` installs correctly-arched libraries
# and the stock gcc IS the target compiler. That lets us reuse the existing
# LowLevel/scripts/compiler.sh verbatim -- no cross toolchain files, no sysroot
# wrangling. Slower than a true cross-toolchain, but dead simple and repeatable.
#
# Build with: ./build-cross.sh   (handles QEMU + buildx + artifact extraction)

ARG DEBIAN_VERSION=bookworm

# ---- builder: one instance per --platform, emulated via QEMU ----------------
FROM --platform=$TARGETPLATFORM debian:${DEBIAN_VERSION}-slim AS builder

ARG CPPZMQ_REF=v4.10.0
# Board metadata, passed by build-cross.sh from docker/boards.conf. Used only to
# label the artifact (BOARD.txt / HOW-TO-INSTALL.txt); does not affect the build.
ARG BOARD_ID=unknown
ARG BOARD_DESC="unknown board"
ARG BOARD_OTG=unknown
ENV DEBIAN_FRONTEND=noninteractive

# Toolchain + every conboard build/runtime dependency (cf. scripts/configure.sh):
#   libzmq3-dev          -> libzmq, the ZeroMQ C lib that cppzmq wraps
#   libasound2-dev       -> -lasound  (MIDI / launcher / keyboard / mouse / joystick)
#   uuid-dev             -> -luuid    (dispatcher)
#   libboost-{system,date-time}-dev -> Crow (HTTP in dispatcher) needs Boost
#   file                 -> manifest only (records ELF arch per binary)
# (libusb dropped: device detection now uses kernel sysfs/evdev, not libusb.)
RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential cmake git pkg-config ca-certificates file \
        libzmq3-dev libasound2-dev uuid-dev \
        libboost-dev libboost-system-dev libboost-date-time-dev \
    && rm -rf /var/lib/apt/lists/*

# cppzmq: header + CMake package config (the modules call find_package(cppzmq)).
# Mirror scripts/configure.sh: install upstream, then overlay the repo's own
# zmq.hpp -- the exact header conboard is known to build against.
COPY scripts/assets/zmq.hpp /tmp/zmq.hpp
RUN git clone --depth 1 --branch ${CPPZMQ_REF} https://github.com/zeromq/cppzmq.git /tmp/cppzmq \
    && cmake -S /tmp/cppzmq -B /tmp/cppzmq/build -DCPPZMQ_BUILD_TESTS=OFF \
    && cmake --install /tmp/cppzmq/build \
    && cp /tmp/zmq.hpp /usr/local/include/zmq.hpp \
    && cp /tmp/zmq.hpp /usr/include/zmq.hpp \
    && rm -rf /tmp/cppzmq /tmp/zmq.hpp

WORKDIR /src
# Sources (build/ and dist/ are kept out by .dockerignore, so the in-tree build
# dirs created below are always fresh for this architecture).
COPY . /src

# Crow (header-only HTTP, used by dispatcher via #include "crow.h") comes from
# the repo's own submodule so the version matches exactly. build-cross.sh runs
# `git submodule update --init` before the build; fail loudly if it didn't.
RUN if [ -f LowLevel/Common/Crow/include/crow.h ]; then \
        cp -r LowLevel/Common/Crow/include/* /usr/local/include/ ; \
    else \
        echo "ERROR: Crow submodule missing. Run: git submodule update --init --recursive" >&2 ; \
        exit 1 ; \
    fi

# Build everything via the existing recipe:
#   common -> joystick -> keyboard -> midi -> mouse -> launcher -> dispatcher
RUN chmod +x LowLevel/scripts/compiler.sh \
             LowLevel/*/scripts/*.sh \
             tools/*/scripts/*.sh \
             docker/package.sh \
    && LowLevel/scripts/compiler.sh

# Assemble the installable artifact tree + tarball under /out.
RUN BOARD_ID="${BOARD_ID}" BOARD_DESC="${BOARD_DESC}" BOARD_OTG="${BOARD_OTG}" \
        docker/package.sh /out

# ---- artifact: a scratch image whose entire filesystem IS the output --------
# `buildx --output type=local` writes this per platform, e.g.
#   dist/linux_arm64/conboard-arm64.tar.gz
#   dist/linux_arm_v7/conboard-armhf.tar.gz
FROM scratch AS artifact
COPY --from=builder /out/ /
