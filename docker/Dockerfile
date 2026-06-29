# General-purpose build image for rp2040-doom-LCD.
#
# Builds your current working tree into RP2040 firmware (.uf2).  The source is
# supplied at run time as a read-only bind mount (see docker-run.sh), so this
# image is source-agnostic and only needs rebuilding when the toolchain/SDK
# pins below change.
#
#   ./docker-run.sh                               # build the ST7735 board target
#   ./docker-run.sh doom_tiny_usb_ST7789_240_135  # build a different target
#
# (The original byte-for-byte reproducibility flow lives in docker-entrypoint.sh
#  / docker-compare.sh and is no longer the image default.)
FROM ubuntu:24.04
ENV DEBIAN_FRONTEND=noninteractive

# ARM toolchain + build deps.  Versions are intentionally unpinned so the image
# keeps building as Ubuntu's archive moves on.  Known-good set from the original
# reproducible build: gcc-arm-none-eabi 13.2.rel1, binutils 2.42, newlib 4.4.
RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    git \
    cmake \
    make \
    python3 \
    build-essential \
    libusb-1.0-0-dev \
    pkg-config \
    gcc-arm-none-eabi \
    binutils-arm-none-eabi \
    libnewlib-arm-none-eabi \
    libstdc++-arm-none-eabi-newlib \
    && rm -rf /var/lib/apt/lists/*

# Pico SDK + extras, pinned to SHAs known to build this tree.
RUN git clone https://github.com/raspberrypi/pico-sdk /opt/pico-sdk \
    && git -C /opt/pico-sdk checkout 6a7db34ff63345a7badec79ebea3aaef1712f374 \
    && git -C /opt/pico-sdk submodule update --init --recursive --depth=1

RUN git clone https://github.com/raspberrypi/pico-extras /opt/pico-extras \
    && git -C /opt/pico-extras checkout ed98c7acb694757715ede81c044a7404e1762499 \
    && git -C /opt/pico-extras submodule update --init --recursive --depth=1

# picotool (handy for inspecting / flashing firmware).  It needs pico-sdk >= 2.0
# to BUILD, so use a throwaway 2.0.0 SDK for the build then discard it.
RUN git clone --depth 1 --branch 2.0.0 \
        https://github.com/raspberrypi/pico-sdk /tmp/pico-sdk-picotool \
    && git -C /tmp/pico-sdk-picotool submodule update --init --recursive --depth=1 \
    && git clone --depth 1 --branch 2.0.0 \
        https://github.com/raspberrypi/picotool /tmp/picotool \
    && cmake -S /tmp/picotool -B /tmp/picotool/build \
        -DPICO_SDK_PATH=/tmp/pico-sdk-picotool \
    && make -C /tmp/picotool/build -j"$(nproc)" \
    && cp /tmp/picotool/build/picotool /usr/local/bin/picotool \
    && rm -rf /tmp/picotool /tmp/pico-sdk-picotool

ENV PICO_SDK_PATH=/opt/pico-sdk
ENV PICO_EXTRAS_PATH=/opt/pico-extras

COPY build.sh /usr/local/bin/docker-build.sh
RUN chmod +x /usr/local/bin/docker-build.sh
ENTRYPOINT ["/usr/local/bin/docker-build.sh"]
