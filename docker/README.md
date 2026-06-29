# Docker build harness

Reproducible RP2040 firmware builds in a pinned toolchain (pico-sdk 1.5.1 /
gcc-arm-none-eabi 13.2 on Ubuntu 24.04), plus a comparison tool that grades a
build against the known-good "ground truth" firmware.

## Build

```bash
./docker/run.sh                               # all screen targets
./docker/run.sh doom_tiny_usb_ST7789_240_135  # one target (this board)
TARGETS="a b" ./docker/run.sh                 # ...or via env
PICO_BOARD=vgaboard BUILD_TYPE=MinSizeRel ./docker/run.sh
```

Firmware lands in `../out/` (`.uf2/.elf/.bin/.elf.map` per target). Every `*.whx`
in the repo root is also converted to a drag-and-drop `.uf2` at `WAD_ADDR`
(default `0x10042000`). The USB targets need the tinyusb submodule first:
`git submodule update --init 3rdparty/tinyusb`.

## Compare a build to the ground truth

```bash
./docker/compare.sh out doom_tiny_usb_ST7789_240_135 my-label
```

Grades the build (A = bit-identical, B = same size/sections — boots, G =
different source) against `new-st7789 @ 996bef5e`. Needs
`reproducibility-report-vm.tar.gz` in the repo root (kept locally, gitignored);
it's extracted to `docker/artifacts/` along with the report it writes.

## Files
| File | Role |
|------|------|
| `Dockerfile` | builder image (pinned SDK + toolchain + picotool) |
| `build.sh` | in-container build (mounted source `/src` → `/out`) |
| `run.sh` | host entry: build image, run build, collect to `../out/` |
| `compare.sh` | grade a firmware against the VM ground truth |
| `artifacts/` | comparison reports + reference (gitignored) |
