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
    nearestNeighborHandleFrameStart();
}

void st7789_240_135_blit(uint16_t *downsampled_line, int scanline) {
    for (uint8_t x = 0; x < DOWNSAMPLED_WIDTH; x++) {
        uint16_t color = downsampled_line[x];
        // st7735 expects least significant byte first, but the normal msb / lsb order in each byte
        downsampled_line[x] = (((color) << 8) & 0xFF00) | (((color) >> 8) & 0xFF);
        // put_pixel(x, scanline, downsampled_line[x]);
    }
    blit(SCREEN_WIDTH_OFFSET, scanline,DOWNSAMPLED_WIDTH, 1, downsampled_line);
}

void st7789_240_135_handleScanline(uint16_t *line, int scanline) {
    nearestNeighborHandleDownsampling(line, scanline, st7789_240_135_blit);
}