#include "st7789_240_135.h"

// lifted from hagl_hal_single.c
static void put_pixel(int16_t x0, int16_t y0, color_t color)
{
    mipi_display_write(x0, y0, 1, 1, (uint8_t *) &color);
}

static void blit(int16_t x0, int16_t y0, uint16_t width, uint16_t height, uint16_t *src)
{
    mipi_display_write(x0, y0, width, height, (uint8_t *) src);
}

void st7789_240_135_initScreen(void) {
    mipi_display_init();
    screenBlitDMAInit();          // enable the shared non-blocking per-line DMA blit
    // sleep_ms(3000);

    uint16_t data[] = {
        0b0000000000000000
    };

    // for(uint8_t y = 0; y < 80; y++) {
    //     blit(x,y,1,1, data);
    // }

    for (uint8_t x = 0; x < LCD_WIDTH; x++) {
        for(uint8_t y = 0; y < LCD_HEIGHT; y++) {
            blit(x, y, 1, 1, data);
        }
    }
    // sleep_ms(3000);
}

void st7789_240_135_handleFrameStart(uint8_t frame) {
    // Make sure the previous frame's last DMA row has fully drained before we
    // start reusing buffers / issuing a new frame.
    screenBlitDMAFlush();
    nearestNeighborHandleFrameStart();
}

void st7789_240_135_blit(uint16_t *downsampled_line, int scanline) {
    // Shared non-blocking path: byte-swap into a ping-pong buffer and DMA the
    // row so the next row is downsampled while this one is on the wire.
    screenBlitLineDMA(SCREEN_WIDTH_OFFSET, scanline, downsampled_line);
}

void st7789_240_135_handleScanline(uint16_t *line, int scanline) {
    nearestNeighborHandleDownsampling(line, scanline, st7789_240_135_blit);
}