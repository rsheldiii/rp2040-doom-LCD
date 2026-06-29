#!/bin/bash
# Build rp2040-doom-LCD firmware from the current working tree, in Docker.
#
#   ./docker/run.sh                               # default: all screen targets
#   ./docker/run.sh doom_tiny_usb_ST7789_240_135  # build specific target(s)
#   TARGETS="a b" ./docker/run.sh                 # ...or select via env
#   PICO_BOARD=vgaboard BUILD_TYPE=MinSizeRel ./docker/run.sh
#
# Writes to <repo>/out/: firmware (.uf2/.elf/.bin/.elf.map) for each target, plus
# a drag-and-drop .uf2 for every WAD (*.whx) in the repo root, at WAD_ADDR
# (default 0x10042000).
#
# Note: the USB targets need the tinyusb submodule on the host first:
#   git submodule update --init 3rdparty/tinyusb
set -euo pipefail
cd "$(dirname "$0")"                 # docker/  (build context: holds Dockerfile + build.sh)
REPO_ROOT="$(cd .. && pwd)"          # repo root = the source tree to build

IMAGE=${IMAGE:-rp2040-doom-lcd-builder}
OUT=${OUT:-"$REPO_ROOT/out"}
mkdir -p "$OUT"

echo "==> Building image '$IMAGE'..."
docker build -t "$IMAGE" .

echo "==> Building firmware (targets: ${*:-${TARGETS:-all screen targets}})..."
docker run --rm \
    -v "$REPO_ROOT:/src:ro" \
    -v "$OUT:/out" \
    -e PICO_BOARD="${PICO_BOARD:-vgaboard}" \
    -e BUILD_TYPE="${BUILD_TYPE:-MinSizeRel}" \
    -e TARGETS="${TARGETS:-}" \
    -e WAD_ADDR="${WAD_ADDR:-0x10042000}" \
    "$IMAGE" "$@"

echo "==> Firmware written to: $OUT"
