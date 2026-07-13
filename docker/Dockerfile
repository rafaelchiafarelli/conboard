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
ARG NODE_VERSION=22

# ---- frontend: build the console SPA (arch-independent) ----------------------
# The Vite bundle is static and platform-agnostic, so build it on the BUILD host
# (no QEMU) and copy the output into the artifact for nginx to serve on-device.
FROM --platform=$BUILDPLATFORM node:${NODE_VERSION}-alpine AS frontend
WORKDIR /app
COPY frontend/console/package.json frontend/console/package-lock.json ./
RUN npm ci
COPY frontend/console/ ./
RUN npm run build            # -> /app/dist (index.html + hashed assets)

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
#   ccache               -> caches compiled objects across builds (see below)
# backend/ (harpia-generated management API) additionally needs:
#   protobuf-compiler + libprotobuf-dev            -> message structs (.pb.cc)
#   protobuf-compiler-grpc + libgrpc++-dev         -> the generated gRPC services
#   libsoci-dev + libsoci-sqlite3-4.0 + libsqlite3-dev -> SOCI DAO on SQLite
#   patchelf             -> stamp an absolute RUNPATH so the bundled runtime .so's
#                           resolve on the board with NO apt install (see package.sh)
RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential cmake git pkg-config ca-certificates file ccache patchelf \
        libzmq3-dev libasound2-dev uuid-dev \
        libboost-dev libboost-system-dev libboost-date-time-dev \
        protobuf-compiler libprotobuf-dev protobuf-compiler-grpc libgrpc++-dev \
        libsoci-dev libsoci-sqlite3-4.0 libsqlite3-dev \
    && rm -rf /var/lib/apt/lists/*

# Make the compiler go through ccache so a *second* build only recompiles the
# files that actually changed. Debian's ccache shims cover gcc/g++ but NOT cc/c++
# (which CMake picks by default), so add those two; then put the shim dir first
# on PATH and point ccache at the path we mount as a persistent cache below.
RUN ln -sf /usr/bin/ccache /usr/lib/ccache/cc \
 && ln -sf /usr/bin/ccache /usr/lib/ccache/c++
ENV PATH="/usr/lib/ccache:${PATH}"
ENV CCACHE_DIR=/ccache

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
#   common -> joystick -> keyboard -> midi -> mouse -> launcher -> dispatcher -> devprobe
# The cache mount persists ccache's object cache across builds; ccache -s at the
# end prints the hit/miss stats so you can see the cache working.
RUN --mount=type=cache,target=/ccache \
    chmod +x LowLevel/scripts/compiler.sh \
             LowLevel/*/scripts/*.sh \
             tools/*/scripts/*.sh \
             docker/package.sh \
    && LowLevel/scripts/compiler.sh \
    && echo "==== ccache stats ====" && ccache -s

# Build the C++ backend (Crow REST + gRPC over the harpia-generated SOCI/SQLite
# DAOs). Consumes backend/generated/ (committed); no protoc-at-build needed as long
# as the image's protobuf matches the committed .pb.cc (bookworm 3.21.x == the
# generator image). package.sh bundles its runtime .so closure into the artifact.
#
# CAP PARALLELISM (do NOT use -j $(nproc)): the ~38 generated protobuf/gRPC TUs are
# large, and under QEMU emulation each g++ is very RAM-hungry. On a high-core host,
# -j $(nproc) launches dozens at once, exhausts memory, and emulated exec() starts
# failing with exit code 126 (observed on a many-core builder; an 8-core/32G box is
# fine). 4 concurrent is plenty under emulation, where per-file speed dominates.
RUN --mount=type=cache,target=/ccache \
    JOBS="$(nproc)"; if [ "$JOBS" -gt 4 ]; then JOBS=4; fi; \
    echo "backend build with -j $JOBS (capped for QEMU memory safety)"; \
    cmake -S backend -B backend/build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build backend/build -j "$JOBS" \
    && echo "==== backend ccache stats (-j $JOBS) ====" && ccache -s

# Built console SPA (from the native frontend stage) — package.sh copies it into
# the artifact at /conboard/frontend, where the bundled nginx site serves it.
COPY --from=frontend /app/dist /src/frontend/console/dist

# Assemble the installable artifact tree + tarball under /out.
RUN BOARD_ID="${BOARD_ID}" BOARD_DESC="${BOARD_DESC}" BOARD_OTG="${BOARD_OTG}" \
        docker/package.sh /out

# ---- artifact: a scratch image whose entire filesystem IS the output --------
# `buildx --output type=local` writes this per platform, e.g.
#   dist/linux_arm64/conboard-arm64.tar.gz
#   dist/linux_arm_v7/conboard-armhf.tar.gz
FROM scratch AS artifact
COPY --from=builder /out/ /
