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

#ifndef _MIPI_DISPLAY_H
#define _MIPI_DISPLAY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

#include "hagl_hal.h"

void mipi_display_init();
size_t mipi_display_write(uint16_t x1, uint16_t y1, uint16_t w, uint16_t h, uint8_t *buffer);
void mipi_display_ioctl(uint8_t command, uint8_t *data, size_t size);
void mipi_display_close();

/* Non-blocking per-line DMA blit path (overlaps SPI transfer with CPU work). */
void mipi_display_dma_init(void);
void mipi_display_blit_dma(uint16_t x1, uint16_t y1, uint16_t w, uint16_t h, const uint8_t *buffer);
void mipi_display_blit_dma_flush(void);

#ifdef __cplusplus
}
#endif
#endif /* _MIPI_DISPLAY_H */