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