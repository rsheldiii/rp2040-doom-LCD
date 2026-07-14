/*

MIT License

Copyright (c) 2019-2022 Mika Tuupola

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

-cut-

This file is part of the Raspberry Pi Pico MIPI DCS backend for the HAGL
graphics library: https://github.com/tuupola/hagl_pico_mipi

SPDX-License-Identifier: MIT

*/

#include <stdio.h>
#include <stdlib.h>
// #include <string.h>
// #include <stdatomic.h>

#include <hardware/spi.h>
#include <hardware/dma.h>
#include <hardware/gpio.h>
#include <hardware/clocks.h>

#include "mipi_dcs.h"
#include "mipi_display.h"

static int dma_channel;

static void mipi_display_write_command(const uint8_t command)
{
    /* Set DC low to denote incoming command. */
    gpio_put(MIPI_DISPLAY_PIN_DC, 0);

    /* Set CS low to reserve the SPI bus. */
    gpio_put(MIPI_DISPLAY_PIN_CS, 0);

    spi_write_blocking(MIPI_DISPLAY_SPI_PORT, &command, 1);

    /* Set CS high to ignore any traffic on SPI bus. */
    gpio_put(MIPI_DISPLAY_PIN_CS, 1);
}

static void mipi_display_write_data(const uint8_t *data, size_t length)
{
    size_t sent = 0;

    if (0 == length) {
        return;
    };

    /* Set DC high to denote incoming data. */
    gpio_put(MIPI_DISPLAY_PIN_DC, 1);

    /* Set CS low to reserve the SPI bus. */
    gpio_put(MIPI_DISPLAY_PIN_CS, 0);

    spi_write_blocking(MIPI_DISPLAY_SPI_PORT, data, length);

    /* Set CS high to ignore any traffic on SPI bus. */
    gpio_put(MIPI_DISPLAY_PIN_CS, 1);
}

static void mipi_display_write_data_dma(const uint8_t *buffer, size_t length)
{
    if (0 == length) {
        return;
    };

    /* Set DC high to denote incoming data. */
    gpio_put(MIPI_DISPLAY_PIN_DC, 1);

    /* Set CS low to reserve the SPI bus. */
    gpio_put(MIPI_DISPLAY_PIN_CS, 0);

    dma_channel_wait_for_finish_blocking(dma_channel);
    dma_channel_set_trans_count(dma_channel, length, false);
    dma_channel_set_read_addr(dma_channel, buffer, true);
}

void mipi_display_dma_init(void)
{


    dma_channel = dma_claim_unused_channel(true);
    dma_channel_config channel_config = dma_channel_get_default_config(dma_channel);
    channel_config_set_transfer_data_size(&channel_config, DMA_SIZE_8);
    channel_config_set_read_increment(&channel_config, true);    // walk the source line
    channel_config_set_write_increment(&channel_config, false);  // fixed SPI data register
    if (spi0 == MIPI_DISPLAY_SPI_PORT) {
        channel_config_set_dreq(&channel_config, DREQ_SPI0_TX);
    } else {
        channel_config_set_dreq(&channel_config, DREQ_SPI1_TX);
    }
    dma_channel_set_config(dma_channel, &channel_config, false);
    dma_channel_set_write_addr(dma_channel, &spi_get_hw(MIPI_DISPLAY_SPI_PORT)->dr, false);
}

static void mipi_display_read_data(uint8_t *data, size_t length)
{
    if (0 == length) {
        return;
    };
}

static void mipi_display_set_address(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
    uint8_t command;
    uint8_t data[4];
    static uint16_t prev_x1, prev_x2, prev_y1, prev_y2;

    x1 = x1 + MIPI_DISPLAY_OFFSET_X;
    y1 = y1 + MIPI_DISPLAY_OFFSET_Y;
    x2 = x2 + MIPI_DISPLAY_OFFSET_X;
    y2 = y2 + MIPI_DISPLAY_OFFSET_Y;

    /* Change column address only if it has changed. */
    if ((prev_x1 != x1 || prev_x2 != x2)) {
        mipi_display_write_command(MIPI_DCS_SET_COLUMN_ADDRESS);
        data[0] = x1 >> 8;
        data[1] = x1 & 0xff;
        data[2] = x2 >> 8;
        data[3] = x2 & 0xff;
        mipi_display_write_data(data, 4);

        prev_x1 = x1;
        prev_x2 = x2;
    }

    /* Change page address only if it has changed. */
    if ((prev_y1 != y1 || prev_y2 != y2)) {
        mipi_display_write_command(MIPI_DCS_SET_PAGE_ADDRESS);
        data[0] = y1 >> 8;
        data[1] = y1 & 0xff;
        data[2] = y2 >> 8;
        data[3] = y2 & 0xff;
        mipi_display_write_data(data, 4);

        prev_y1 = y1;
        prev_y2 = y2;
    }

    mipi_display_write_command(MIPI_DCS_WRITE_MEMORY_START);
}

static void mipi_display_spi_master_init()
{
    

    gpio_set_function(MIPI_DISPLAY_PIN_DC, GPIO_FUNC_SIO);
    gpio_set_dir(MIPI_DISPLAY_PIN_DC, GPIO_OUT);

    gpio_set_function(MIPI_DISPLAY_PIN_CS, GPIO_FUNC_SIO);
    gpio_set_dir(MIPI_DISPLAY_PIN_CS, GPIO_OUT);

    gpio_set_function(MIPI_DISPLAY_PIN_CLK,  GPIO_FUNC_SPI);
    gpio_set_function(MIPI_DISPLAY_PIN_MOSI, GPIO_FUNC_SPI);

    if (MIPI_DISPLAY_PIN_MISO > 0) {
        gpio_set_function(MIPI_DISPLAY_PIN_MISO, GPIO_FUNC_SPI);
    }

    /* Set CS high to ignore any traffic on SPI bus. */
    gpio_put(MIPI_DISPLAY_PIN_CS, 1);

    spi_init(MIPI_DISPLAY_SPI_PORT, MIPI_DISPLAY_SPI_CLOCK_SPEED_HZ);

    uint32_t baud = spi_set_baudrate(MIPI_DISPLAY_SPI_PORT, MIPI_DISPLAY_SPI_CLOCK_SPEED_HZ);
    uint32_t peri = clock_get_hz(clk_peri);
    uint32_t sys = clock_get_hz(clk_sys);
    
    
    
}

void mipi_display_init()
{
#ifdef HAGL_HAL_USE_SINGLE_BUFFER
    
#endif /* HAGL_HAL_USE_SINGLE_BUFFER */

#ifdef HAGL_HAL_USE_DOUBLE_BUFFER
    
#endif /* HAGL_HAL_USE_DOUBLE_BUFFER */

#ifdef HAGL_HAL_USE_TRIPLE_BUFFER
    
#endif /* HAGL_HAL_USE_DOUBLE_BUFFER */

    /* Init the spi driver. */
    mipi_display_spi_master_init();
    sleep_ms(100);

    /* Reset the display. */
    if (MIPI_DISPLAY_PIN_RST >= 0) {
        gpio_set_function(MIPI_DISPLAY_PIN_RST, GPIO_FUNC_SIO);
        gpio_set_dir(MIPI_DISPLAY_PIN_RST, GPIO_OUT);

        gpio_put(MIPI_DISPLAY_PIN_RST, 0);
        sleep_ms(100);
        gpio_put(MIPI_DISPLAY_PIN_RST, 1);
        sleep_ms(100);
    }

    /* Send minimal init commands. */
    mipi_display_write_command(MIPI_DCS_SOFT_RESET);
    sleep_ms(200);

    mipi_display_write_command(MIPI_DCS_SET_ADDRESS_MODE);
    mipi_display_write_data(&(uint8_t){MIPI_DISPLAY_ADDRESS_MODE}, 1);

    mipi_display_write_command(MIPI_DCS_SET_PIXEL_FORMAT);
    mipi_display_write_data(&(uint8_t){MIPI_DISPLAY_PIXEL_FORMAT}, 1);

#ifdef MIPI_DISPLAY_INVERT
    mipi_display_write_command(MIPI_DCS_ENTER_INVERT_MODE);
    
#else
    mipi_display_write_command(MIPI_DCS_EXIT_INVERT_MODE);
#endif

    mipi_display_write_command(MIPI_DCS_EXIT_SLEEP_MODE);
    sleep_ms(200);

    mipi_display_write_command(MIPI_DCS_SET_DISPLAY_ON);
    sleep_ms(200);

    /* Enable backlight */
    if (MIPI_DISPLAY_PIN_BL > 0) {
        gpio_set_function(MIPI_DISPLAY_PIN_BL, GPIO_FUNC_SIO);
        gpio_set_dir(MIPI_DISPLAY_PIN_BL, GPIO_OUT);

        gpio_put(MIPI_DISPLAY_PIN_BL, 1);
    }

    /* Set the default viewport to full screen. */
    mipi_display_set_address(0, 0, MIPI_DISPLAY_WIDTH - 1, MIPI_DISPLAY_HEIGHT - 1);

#ifdef HAGL_HAS_HAL_BACK_BUFFER
#ifdef HAGL_HAL_USE_DMA
    mipi_display_dma_init();
#endif /* HAGL_HAL_USE_DMA */
#endif /* HAGL_HAS_HAL_BACK_BUFFER */
}

size_t mipi_display_write(uint16_t x1, uint16_t y1, uint16_t w, uint16_t h, uint8_t *buffer)
{
    if (0 == w || 0 == h) {
        return 0;
    }

    int32_t x2 = x1 + w - 1;
    int32_t y2 = y1 + h - 1;
    uint32_t size = w * h;

#ifdef HAGL_HAL_USE_SINGLE_BUFFER
    mipi_display_set_address(x1, y1, x2, y2);
    mipi_display_write_data(buffer, size * DISPLAY_DEPTH / 8);
#endif /* HAGL_HAL_SINGLE_BUFFER */

#ifdef HAGL_HAS_HAL_BACK_BUFFER
    mipi_display_set_address(x1, y1, x2, y2);
#ifdef HAGL_HAL_USE_DMA
    mipi_display_write_data_dma(buffer, size * DISPLAY_DEPTH / 8);
#else
    mipi_display_write_data(buffer, size * DISPLAY_DEPTH / 8);
#endif /* HAGL_HAL_USE_DMA */
#endif /* HAGL_HAS_HAL_BACK_BUFFER */
    /* This should also include the bytes for writing the commands. */
    return size * DISPLAY_DEPTH / 8;
}

/*
 * Non-blocking per-line DMA blit used by the LCD scanline push.
 *
 * Kicks off this row's SPI transfer via DMA and returns immediately, so the
 * caller can downsample/convert the NEXT row while this one is on the wire.
 * The previous row's transfer is drained here at the start -- and note that a
 * DMA "finish" only means the TX FIFO was filled, so we must also wait for the
 * SPI shift register to empty (spi_is_busy) before we touch CS/DC or send the
 * next window-address commands, or they would corrupt the tail of the pixels.
 *
 * CS is left asserted while the DMA runs; the next call (or the flush below)
 * closes it.  Requires mipi_display_dma_init() to have been called.
 */
void mipi_display_blit_dma(uint16_t x1, uint16_t y1, uint16_t w, uint16_t h, const uint8_t *buffer)
{
    size_t length = (size_t) w * h * DISPLAY_DEPTH / 8;
    if (0 == length) {
        return;
    }

    /* Drain the previous transfer completely before disturbing the bus. */
    dma_channel_wait_for_finish_blocking(dma_channel);
    while (spi_is_busy(MIPI_DISPLAY_SPI_PORT)) {
        tight_loop_contents();
    }
    gpio_put(MIPI_DISPLAY_PIN_CS, 1);   /* close the previous data transaction */

    /* Set the target window (blocking 8-bit command writes; bus is idle now). */
    mipi_display_set_address(x1, y1, x1 + w - 1, y1 + h - 1);

    /* Start this row's pixel DMA and return; CS stays low for the transfer. */
    gpio_put(MIPI_DISPLAY_PIN_DC, 1);
    gpio_put(MIPI_DISPLAY_PIN_CS, 0);
    dma_channel_set_trans_count(dma_channel, length, false);
    dma_channel_set_read_addr(dma_channel, buffer, true);   /* triggers */
}

/* Block until the last DMA blit has fully drained and release CS. */
void mipi_display_blit_dma_flush(void)
{
    dma_channel_wait_for_finish_blocking(dma_channel);
    while (spi_is_busy(MIPI_DISPLAY_SPI_PORT)) {
        tight_loop_contents();
    }
    gpio_put(MIPI_DISPLAY_PIN_CS, 1);
}

/* TODO: This most likely does not work with dma atm. */
void mipi_display_ioctl(const uint8_t command, uint8_t *data, size_t size)
{
    switch (command) {
        case MIPI_DCS_GET_COMPRESSION_MODE:
        case MIPI_DCS_GET_DISPLAY_ID:
        case MIPI_DCS_GET_RED_CHANNEL:
        case MIPI_DCS_GET_GREEN_CHANNEL:
        case MIPI_DCS_GET_BLUE_CHANNEL:
        case MIPI_DCS_GET_DISPLAY_STATUS:
        case MIPI_DCS_GET_POWER_MODE:
        case MIPI_DCS_GET_ADDRESS_MODE:
        case MIPI_DCS_GET_PIXEL_FORMAT:
        case MIPI_DCS_GET_DISPLAY_MODE:
        case MIPI_DCS_GET_SIGNAL_MODE:
        case MIPI_DCS_GET_DIAGNOSTIC_RESULT:
        case MIPI_DCS_GET_SCANLINE:
        case MIPI_DCS_GET_DISPLAY_BRIGHTNESS:
        case MIPI_DCS_GET_CONTROL_DISPLAY:
        case MIPI_DCS_GET_POWER_SAVE:
        case MIPI_DCS_READ_DDB_START:
        case MIPI_DCS_READ_DDB_CONTINUE:
            mipi_display_write_command(command);
            mipi_display_read_data(data, size);
            break;
        default:
            mipi_display_write_command(command);
            mipi_display_write_data(data, size);
    }
}

void mipi_display_close()
{
    spi_deinit(MIPI_DISPLAY_SPI_PORT);
}