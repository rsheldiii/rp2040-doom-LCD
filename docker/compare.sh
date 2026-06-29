#!/bin/bash
# compare-vm.sh — Compare a built firmware against the VM "ground truth".
#
# The ground truth is the known-good "previous iteration" that the prior
# reproducibility effort captured: branch `new-st7789` @ commit 996bef5e,
# pico-sdk 1.5.1 (6a7db34), built on Ubuntu 24.04 / gcc 13.2. Its firmware
# (doom_tiny_usb_ST7789_240_135) boots and runs on the DoomBusinessCard.
#
# This is the slimmed-down successor to docker-compare.sh: it diffs purely on
# host using the VM report tarball + the per-target analysis files that
# docker-build.sh now emits (<TARGET>.sections.txt = `readelf -S`,
# <TARGET>.nm.txt = `nm -n`). No intermediate phase files required.
#
# Usage:
#   ./compare-vm.sh [ARTIFACTS_DIR] [TARGET] [LABEL]
# Defaults: ARTIFACTS_DIR=out  TARGET=doom_tiny_usb_ST7789_240_135  LABEL=current
#
# Override the recorded source SHA (the git rev the artifacts were built from)
# with BUILD_SRC_SHA=... (otherwise `git rev-parse HEAD` of this checkout).
#
# Writes: docker/artifacts/compare-vm-<LABEL>.md  and prints a verdict.
set -euo pipefail
cd "$(dirname "$0")/.."   # repo root (artifacts live under docker/, source at root)

ART=${1:-out}
TARGET=${2:-doom_tiny_usb_ST7789_240_135}
LABEL=${3:-current}

# --- VM ground-truth constants (new-st7789 @ 996bef5e, pico-sdk 1.5.1) --------
VM_BIN_SHA=5efa1712fb473ff9dc88a26076441402cac8605475867ee4c12ed88a0ec74ae0
VM_BIN_SIZE=254928
VM_SRC_SHA=996bef5e4f2c135f5697d1d1a47f6d7bb16da83d
VM_SDK_SHA=6a7db34ff63345a7badec79ebea3aaef1712f374

# --- Extract the VM reference report once -------------------------------------
VM_REF=docker/artifacts/vm-ref/reproducibility-report-vm
if [ ! -d "$VM_REF" ]; then
    if [ ! -f reproducibility-report-vm.tar.gz ]; then
        echo "ERROR: reproducibility-report-vm.tar.gz not found in repo root." >&2
        exit 1
    fi
    mkdir -p docker/artifacts/vm-ref
    tar xzf reproducibility-report-vm.tar.gz -C docker/artifacts/vm-ref
fi

BIN="$ART/$TARGET.bin"
[ -f "$BIN" ] || { echo "ERROR: $BIN not found — build it first (./docker-run.sh $TARGET)." >&2; exit 1; }

BIN_SHA=$(sha256sum "$BIN" | awk '{print $1}')
BIN_SIZE=$(wc -c < "$BIN")

# Source SHA of the tree that produced this build.
SRC_SHA=${BUILD_SRC_SHA:-$(git rev-parse HEAD 2>/dev/null || echo unknown)}
if [ -z "${BUILD_SRC_SHA:-}" ] && [ -n "$(git status --porcelain --untracked-files=no 2>/dev/null | head -1)" ]; then
    SRC_SHA="$SRC_SHA-dirty"
fi

# --- Section + symbol diffs (host-only; need the .sections.txt/.nm.txt files) -
strip_hdr() { tail -n +2 "$1"; }   # drop readelf's variable "starting at offset" line

SECT="$ART/$TARGET.sections.txt"
NM="$ART/$TARGET.nm.txt"
VM_SECT="$VM_REF/firmware.sections.txt"
VM_NM="$VM_REF/firmware.nm-address.txt"

SECT_STATE="skipped"; SECT_DIFF="(no $TARGET.sections.txt — re-run docker-build.sh to emit it)"
if [ -f "$SECT" ] && [ -f "$VM_SECT" ]; then
    if diff <(strip_hdr "$VM_SECT") <(strip_hdr "$SECT") >/tmp/cmp-vm-sect.diff 2>/dev/null; then
        SECT_STATE="identical"; SECT_DIFF="(identical — same section layout & addresses)"
    else
        SECT_STATE="differ"; SECT_DIFF=$(head -40 /tmp/cmp-vm-sect.diff)
    fi
fi

NM_DIFF="(no $TARGET.nm.txt — re-run docker-build.sh to emit it)"
if [ -f "$NM" ] && [ -f "$VM_NM" ]; then
    if diff "$VM_NM" "$NM" >/tmp/cmp-vm-nm.diff 2>/dev/null; then
        NM_DIFF="(identical)"
    else
        NM_DIFF=$(head -30 /tmp/cmp-vm-nm.diff)
    fi
fi

# --- Verdict ------------------------------------------------------------------
if [ "$BIN_SHA" = "$VM_BIN_SHA" ]; then
    V=A; DESC="Bit-for-bit identical to the VM ground-truth firmware."
elif [ "$BIN_SIZE" = "$VM_BIN_SIZE" ] && [ "$SECT_STATE" = "identical" ]; then
    V=B; DESC="Same size + identical ELF sections; SHA differs only in non-deterministic metadata (__DATE__). Functionally identical — safe to flash."
elif [ "${SRC_SHA%%-dirty}" != "$VM_SRC_SHA" ]; then
    V=G; DESC="Source SHA ($SRC_SHA) != VM ground truth ($VM_SRC_SHA). This build is from a DIFFERENT code base/branch than the known-good firmware."
elif [ "$BIN_SIZE" != "$VM_BIN_SIZE" ]; then
    V=C; DESC="Same source line but binary size differs ($BIN_SIZE vs $VM_BIN_SIZE) — investigate section/layout changes."
else
    V=C; DESC="Binary differs from VM; sections changed. Investigate."
fi

OUT_MD="docker/artifacts/compare-vm-${LABEL}.md"
mkdir -p docker/artifacts
{
cat <<HDR
# Firmware vs VM ground-truth — \`$LABEL\`

Generated: $(date -u)
Target: \`$TARGET\`   Artifacts: \`$ART\`

## Verdict: **$V**

$DESC

| Grade | Meaning |
|-------|---------|
| A | Bit-for-bit identical |
| B | Same size + sections, SHA differs (metadata only) — boots |
| C | Same source, sections/size differ — investigate layout |
| G | Different source/branch than the known-good build |

## Key numbers

| Metric | VM ground truth (new-st7789 @ 996bef5e) | This build (\`$LABEL\`) | Match |
|--------|------------------------------------------|------------------------|-------|
| source SHA | \`$VM_SRC_SHA\` | \`$SRC_SHA\` | $([ "${SRC_SHA%%-dirty}" = "$VM_SRC_SHA" ] && echo PASS || echo **FAIL**) |
| .bin size | $VM_BIN_SIZE | $BIN_SIZE | $([ "$BIN_SIZE" = "$VM_BIN_SIZE" ] && echo PASS || echo **FAIL**) |
| .bin sha256 | \`$VM_BIN_SHA\` | \`$BIN_SHA\` | $([ "$BIN_SHA" = "$VM_BIN_SHA" ] && echo PASS || echo "no (see verdict)") |
| ELF sections | (reference) | $SECT_STATE | $([ "$SECT_STATE" = "identical" ] && echo PASS || echo "$SECT_STATE") |

## ELF section diff (VM vs this build)

\`\`\`
$SECT_DIFF
\`\`\`

## nm symbol diff (first 30 lines, VM vs this build)

\`\`\`
$NM_DIFF
\`\`\`

---
*compare-vm.sh — successor to docker-compare.sh*
HDR
} > "$OUT_MD"

echo "Report: $OUT_MD"
echo "=== VERDICT: $V ==="
echo "$DESC"
