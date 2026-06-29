#include "lilygo_ttgo.h"

// lifted from hagl_hal_single.c
static void put_pixel(int16_t x0, int16_t y0, color_t color)
{
    mipi_display_write(x0, y0, 1, 1, (uint8_t *) &color);
}

static void blit(int16_t x0, int16_t y0, uint16_t width, uint16_t height, uint16_t *src)
{
    mipi_display_write(x0, y0, width, height, (uint8_t *) src);
}


void lilygo_ttgo_initScreen(void) {
    // turning on the backlight
    gpio_init(22);
    gpio_set_dir(22, GPIO_OUT);
    gpio_put(22, 1);

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
}

void lilygo_ttgo_handleFrameStart(uint8_t frame) {
    nearestNeighborHandleFrameStart();
}

void lilygo_ttgo_blit(uint16_t *downsampled_line, int scanline) {
    for (uint8_t x = 0; x < DOWNSAMPLED_WIDTH; x++) {
        uint16_t color = downsampled_line[x];
        // I... don't know why we would still have to do this... but it seems to work?
        downsampled_line[x] = (((color) << 8) & 0xFF00) | (((color) >> 8) & 0xFF);
        // put_pixel(x, scanline, downsampled_line[x]);
    }
    blit(SCREEN_WIDTH_OFFSET, scanline,DOWNSAMPLED_WIDTH, 1, downsampled_line);
}

void lilygo_ttgo_handleScanline(uint16_t *line, int scanline) {
    nearestNeighborHandleDownsampling(line, scanline, lilygo_ttgo_blit);
}