#!/bin/bash
set -euo pipefail

# ---------------------------------------------------------------------------
# General-purpose Docker build of the rp2040-doom-LCD working tree.
#
#   Source : bind-mounted read-only at /src  (your current checkout, edits and all)
#   Output : firmware written to /out         (.uf2 / .elf / .bin / .elf.map)
#
# Usage (normally invoked via ./docker-run.sh):
#   docker run --rm -v "$PWD:/src:ro" -v "$PWD/out:/out" \
#       rp2040-doom-lcd-builder [TARGET ...]
#
# With no TARGET argument it builds this board's firmware
# (doom_tiny_usb_ST7735_128_128).  Pass one or more CMake target names to
# build something else, e.g. doom_tiny_usb_ST7789_240_135.
#
# Environment overrides:
#   PICO_BOARD  (default vgaboard)
#   BUILD_TYPE  (default MinSizeRel)
#   JOBS        (default: all cores)
# ---------------------------------------------------------------------------

SRC=${SRC:-/src}
OUT=${OUT:-/out}
BUILD_DIR=${BUILD_DIR:-/build/rp2040-build}
PICO_BOARD=${PICO_BOARD:-vgaboard}
BUILD_TYPE=${BUILD_TYPE:-MinSizeRel}
JOBS=${JOBS:-$(nproc)}

# Targets to build.  Priority: CLI args > $TARGETS env > all screen targets.
if [ "$#" -gt 0 ]; then
    TARGETS=("$@")
elif [ -n "${TARGETS:-}" ]; then
    # shellcheck disable=SC2206
    TARGETS=($TARGETS)
else
    TARGETS=(
        doom_tiny_usb_SSD1306_70_40
        doom_tiny_usb_SSD1306_70_40_i2c
        doom_tiny_usb_LILYGO_TTGO
        doom_tiny_usb_ST7789_240_135
        doom_tiny_usb_ST7735_128_128
    )
fi

mkdir -p "$OUT"

# The USB targets need the tinyusb submodule.  Because /src is mounted
# read-only, it must already be checked out on the host:
#   git submodule update --init 3rdparty/tinyusb
if [ ! -e "$SRC/3rdparty/tinyusb/src/tusb.h" ]; then
    echo "ERROR: $SRC/3rdparty/tinyusb is not checked out." >&2
    echo "       Run this on the host first, then re-run the build:" >&2
    echo "         git submodule update --init 3rdparty/tinyusb" >&2
    exit 1
fi

echo "==> Configuring (PICO_BOARD=$PICO_BOARD, BUILD_TYPE=$BUILD_TYPE)"
cmake -S "$SRC" -B "$BUILD_DIR" \
    -G "Unix Makefiles" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DPICO_BOARD="$PICO_BOARD" \
    -DPICO_SDK_PATH="$PICO_SDK_PATH" \
    -DPICO_EXTRAS_PATH="$PICO_EXTRAS_PATH"

echo "==> Building: ${TARGETS[*]}  (-j$JOBS)"
make -C "$BUILD_DIR" "${TARGETS[@]}" -j"$JOBS"

echo "==> Collecting firmware to $OUT"
for T in "${TARGETS[@]}"; do
    for ext in uf2 elf bin elf.map; do
        f=$(find "$BUILD_DIR" -name "${T}.${ext}" | head -1)
        [ -n "$f" ] && cp -v "$f" "$OUT/"
    done
    # Emit analysis dumps consumed by compare-vm.sh (readelf -S / nm -n).
    elf=$(find "$BUILD_DIR" -name "${T}.elf" | head -1)
    if [ -n "$elf" ]; then
        arm-none-eabi-readelf -S "$elf" > "$OUT/${T}.sections.txt" 2>/dev/null || true
        arm-none-eabi-nm -n     "$elf" > "$OUT/${T}.nm.txt"       2>/dev/null || true
    fi
done

# Convert every WAD (*.whx in the source root, next to doom1.whx) into a
# drag-and-drop UF2 at the firmware's WAD offset, so a host without picotool
# can flash it onto the RPI-RP2 BOOTSEL drive.
WAD_ADDR=${WAD_ADDR:-0x10042000}
shopt -s nullglob
whx_files=("$SRC"/*.whx)
shopt -u nullglob
if [ "${#whx_files[@]}" -gt 0 ]; then
    echo "==> Converting WAD(s) to UF2 at $WAD_ADDR"
    for whx in "${whx_files[@]}"; do
        base=$(basename "$whx" .whx)
        python3 - "$whx" "$OUT/$base.uf2" "$WAD_ADDR" <<'PY'
import struct, sys
inp, out, addr = sys.argv[1], sys.argv[2], int(sys.argv[3], 16)
data = open(inp, "rb").read(); PS = 256
blocks = (len(data) + PS - 1) // PS
with open(out, "wb") as f:
    for i in range(blocks):
        c = data[i*PS:(i+1)*PS]; c += b"\x00" * (PS - len(c))
        f.write(struct.pack("<IIIIIIII", 0x0A324655, 0x9E5D5157, 0x2000,
                addr + i*PS, PS, i, blocks, 0xE48BFF56)   # 0xE48BFF56 = RP2040
                + c + b"\x00" * (476 - PS) + struct.pack("<I", 0x0AB16F30))
print(f"    {inp.rsplit('/',1)[-1]} -> {out.rsplit('/',1)[-1]}  ({blocks} blocks @ 0x{addr:08x})")
PY
    done
fi

echo "==> Done. Artifacts in $OUT:"
ls -la "$OUT"
