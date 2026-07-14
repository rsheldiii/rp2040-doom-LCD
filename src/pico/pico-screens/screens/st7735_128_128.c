#include "pico/stdlib.h"
#include "st7735_128_128.h"

// lifted from hagl_hal_single.c
static void put_pixel(int16_t x0, int16_t y0, color_t color)
{
    mipi_display_write(x0, y0, 1, 1, (uint8_t *) &color);
}

static void blit(int16_t x0, int16_t y0, uint16_t width, uint16_t height, uint16_t *src)
{
    mipi_display_write(x0, y0, width, height, (uint8_t *) src);
}


void st7735_128_128_initScreen(void) {

    mipi_display_init();
    screenBlitDMAInit();          // enable the shared non-blocking per-line DMA blit
    // sleep_ms(3000);

    uint16_t data[] = {
        0b0000000000000000
    };

    // for(uint8_t y = 0; y < 80; y++) {
    //     blit(x,y,1,1, data);
    // }

    for (uint8_t x = 0; x < 161; x++) {
        for(uint8_t y = 0; y < 80; y++) {
            blit(x, y, 1, 1, data);
        }
    }
    // gpio_init(16);
    // gpio_set_dir(16, GPIO_OUT);
    // gpio_put(16, 1);
    // sleep_ms(3000);
}

void st7735_128_128_handleFrameStart(uint8_t frame) {
    // gpio_put(16, 1);
    // Drain the previous frame's last DMA row before reusing buffers.
    screenBlitDMAFlush();
    nearestNeighborHandleFrameStart();
}

void st7735_128_128_blit(uint16_t *downsampled_line, int scanline) {
    // Shared non-blocking path: byte-swap into a ping-pong buffer and DMA the
    // row so the next row is downsampled while this one is on the wire.  Same
    // helper the ST7789 driver uses -- the MIPI-DCS blit is controller-agnostic.
    screenBlitLineDMA(SCREEN_WIDTH_OFFSET, scanline, downsampled_line);
}

void st7735_128_128_handleScanline(uint16_t *line, int scanline) {
    nearestNeighborHandleDownsampling(line, scanline, st7735_128_128_blit);
}