#ifdef __cplusplus
extern "C" {
#endif

#ifndef __SCREEN_SHARED__
#define __SCREEN_SHARED__

#include "pico.h"
#include "i_video.h"


#define DOOM_WIDTH SCREENWIDTH
#define DOOM_HEIGHT SCREENHEIGHT

// here be some chicanery to avoid floats
#ifndef DOWNSAMPLING_FACTOR_OUT_OF_100
#define DOWNSAMPLING_FACTOR_OUT_OF_100 243
#endif

#define DOWNSAMPLED_WIDTH ((SCREENWIDTH * 100 / DOWNSAMPLING_FACTOR_OUT_OF_100))
#define DOWNSAMPLED_HEIGHT ((SCREENHEIGHT * 100 / DOWNSAMPLING_FACTOR_OUT_OF_100))

#define DOWNSAMPLING_OFFSET_WIDTH ((SCREENWIDTH - (SCREENWIDTH * 100 / DOWNSAMPLING_FACTOR_OUT_OF_100)) >> 1)
#define DOWNSAMPLING_OFFSET_HEIGHT ((SCREENHEIGHT - (SCREENHEIGHT * 100 / DOWNSAMPLING_FACTOR_OUT_OF_100)) >> 1)

void clearDownsampleBuffers(void);

uint16_t colorToGreyscale(uint16_t pixel);

// Generic non-blocking per-line DMA push, shared by every MIPI-DCS screen
// driver (ST7789, ST7735, ...).  The DMA engine lives in the HAL and is
// controller-agnostic; this just byte-swaps one downsampled row into a
// ping-pong buffer and kicks off its transfer, so the caller can downsample the
// NEXT row while this one is on the wire.  Sized per-target via DOWNSAMPLED_WIDTH.
void screenBlitDMAInit(void);                                              // once, at screen init
void screenBlitLineDMA(uint16_t offset_x, int scanline, uint16_t *line);   // per scanline
void screenBlitDMAFlush(void);                                             // once, at frame start

void areaAverageHandleDownsampling(uint16_t *src, int scanline, void (*callback)(uint16_t *, int));
void areaAverageHandleFrameStart(void);

void nearestNeighborHandleDownsampling(uint16_t *src, int scanline, void (*callback)(uint16_t *, int));
void nearestNeighborHandleFrameStart(void);

void blockAverageHandleDownsampling(uint16_t *src, int scanline, void (*callback)(uint16_t *, int));
void blockAverageHandleFrameStart(void);

#endif

#ifdef __cplusplus
}
#endif