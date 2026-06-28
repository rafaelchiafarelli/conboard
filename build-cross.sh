#!/usr/bin/env bash
#
# One-command cross-build of conboard's C++ for the Orange Pi boards.
#
#   ./build-cross.sh                  # both: arm64 (Zero 3) + armhf (Zero)
#   PLATFORMS=linux/arm64 ./build-cross.sh   # just one
#
# Outputs per-arch artifacts under dist/:
#   dist/linux_arm64/conboard-arm64.tar.gz   (Orange Pi Zero 3 / H618)
#   dist/linux_arm_v7/conboard-armhf.tar.gz  (Orange Pi Zero  / H3)
#
# Requires Docker with buildx (ships with Docker 23+). Everything else --
# QEMU emulators, a container builder -- is set up automatically below.
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")"

PLATFORMS="${PLATFORMS:-linux/arm64,linux/arm/v7}"
DEST="${DEST:-dist}"
BUILDER="conboard-xbuild"

echo ">> [1/4] registering QEMU emulators (one-time, needs --privileged)"
docker run --privileged --rm tonistiigi/binfmt --install arm64,arm >/dev/null

echo ">> [2/4] ensuring buildx builder '$BUILDER' exists"
if ! docker buildx inspect "$BUILDER" >/dev/null 2>&1; then
    docker buildx create --name "$BUILDER" --driver docker-container --bootstrap >/dev/null
fi

echo ">> [3/4] initialising git submodules (Crow)"
git submodule update --init --recursive

echo ">> [4/4] building [$PLATFORMS] -> $DEST/"
docker buildx --builder "$BUILDER" build \
    --platform "$PLATFORMS" \
    --target artifact \
    --output "type=local,dest=$DEST" \
    -f docker/Dockerfile \
    .

echo
echo ">> done. artifacts:"
find "$DEST" -name 'conboard-*.tar.gz' -print 2>/dev/null
echo
echo "Inspect one without unpacking, e.g.:"
echo "  tar xzO -f $DEST/linux_arm64/conboard-arm64.tar.gz conboard/MANIFEST.txt"
