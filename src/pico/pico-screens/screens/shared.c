#include "shared.h"
#include "stdlib.h"

// what (downsampled) scanline we are processing right now
static uint8_t current_downsampled_scanline = 0;
static uint16_t downsampled_row[DOWNSAMPLED_WIDTH] = {0}; // I have no idea why I can't do (int)DOWNSAMPLED_WIDTH

const uint8_t DOWNSAMPLING_FACTOR = DOWNSAMPLING_FACTOR_OUT_OF_100 / 100;

uint16_t ceiling(uint16_t dividend,uint16_t divisor) {
    return dividend/divisor + (dividend % divisor != 0);
}
 
void clearDownsampleBuffers() {
  memset(downsampled_row, 0, sizeof(downsampled_row[0]) * DOWNSAMPLED_WIDTH);
}

uint16_t colorToGreyscale(uint16_t pixel) {
    uint8_t r = pixel & 0b1111100000000000 >> 11;
    uint8_t g = pixel & 0b0000011111100000 >> 5;
    uint8_t b = pixel & 0b0000000000011111;

    // I reached these numbers by complete brute force. every formula I tried was way too dark... not sure why
    uint16_t greyscale = (32 * r + 34 * g + 12 * b) >> 1;
    if (greyscale < 255) return greyscale;
    return 255;
}

// TODO use
uint16_t separateSumAndAveragePixels(uint16_t *pixels, uint8_t length, uint8_t weight) {
    uint16_t r_sum = 0, g_sum = 0, b_sum = 0;
    for(uint8_t pixel = 0; pixel < length; pixel++) {
        r_sum += ((pixels[pixel] & 0b1111100000000000) >> 11);
        g_sum += ((pixels[pixel] & 0b0000011111100000) >> 5);
        b_sum += (pixels[pixel] & 0b0000000000011111);
    }

    uint16_t downsampled_pixel;
    downsampled_pixel  =   (b_sum / weight)        & 0b0000000000011111;
    downsampled_pixel |= (((g_sum / weight) << 5)  & 0b0000011111100000);
    downsampled_pixel |= (((r_sum / weight) << 11) & 0b1111100000000000);

    return downsampled_pixel;
}

// TODO this picks up trash on the very last run - on a [100] array at 98 we'll check 98 99 100 101
uint16_t areaAverageDownsamplePixelGroup(uint16_t *src, uint16_t end) {
    uint16_t total_weight = 0;
    uint16_t r_sum = 0, g_sum = 0, b_sum = 0;

    for(uint8_t pixel = 0; pixel < end; pixel++) {
        uint16_t position = pixel * 100;
        uint16_t middle_position = end * 100 >> 1; // / 2

        uint16_t weight = end * 100 - abs(middle_position - position);

        total_weight += weight;

        r_sum += ((src[pixel] & 0b1111100000000000) >> 11) * weight;
        g_sum += ((src[pixel] & 0b0000011111100000) >> 5) * weight;
        b_sum += (src[pixel] & 0b0000000000011111) * weight;
    }

    uint16_t downsampled_pixel;
    downsampled_pixel  =   (b_sum / total_weight)        & 0b0000000000011111;
    downsampled_pixel |= (((g_sum / total_weight) << 5)  & 0b0000011111100000);
    downsampled_pixel |= (((r_sum / total_weight) << 11) & 0b1111100000000000);

    return downsampled_pixel;
}

// static uint16_t scanline_total_weight = 0;

void areaAverageDownsampleLine(uint16_t *src, uint16_t *dest, uint8_t weight) {
    for (uint16_t x = 0; x < DOWNSAMPLED_WIDTH; x++) {
        uint16_t start = (x * DOWNSAMPLING_FACTOR_OUT_OF_100 / 100);
        // effectively last_downsample_line but for columns
        uint16_t end = ((x + 1) * DOWNSAMPLING_FACTOR_OUT_OF_100 - 1) / 100;
        uint8_t range = end - start;

        uint16_t downsampled_pixel = areaAverageDownsamplePixelGroup(&src[start], range);

        uint8_t r = ((downsampled_pixel & 0b1111100000000000) >> 11) ;
        uint8_t g = ((downsampled_pixel & 0b0000011111100000) >> 5) ;
        uint8_t b = ( downsampled_pixel & 0b0000000000011111);

        uint16_t averaged_pixel;
        averaged_pixel  =   (b / weight)       & 0b0000000000011111;
        averaged_pixel |= (((g / weight) << 5)  & 0b0000011111100000);
        averaged_pixel |= (((r / weight) << 11) & 0b1111100000000000);
        
        dest[x] = downsampled_pixel;
    }
}

void areaAverageHandleFrameStart() {
    clearDownsampleBuffers();
    current_downsampled_scanline = 0;
}

// TODO all broke cuz of switching to current_downsampled_scanline
void areaAverageHandleDownsampling(uint16_t *src, int scanline, void (*callback)(uint16_t *, int)) {
    uint8_t weight = scanline - ((current_downsampled_scanline * DOWNSAMPLING_FACTOR_OUT_OF_100) / 100) + 1 == 0 ? 2 : 4;
    areaAverageDownsampleLine(src, downsampled_row, weight);
    
    // this represents the last true scanline the current downsampling line needs to process
    const uint8_t last_downsample_line = ((current_downsampled_scanline + 1) * DOWNSAMPLING_FACTOR_OUT_OF_100 - 1) / 100;

     // if floor(current_downsampled_scanline * DOWNSAMPLING_FACTOR) == scanline
    if (last_downsample_line == scanline) {
        callback(downsampled_row, current_downsampled_scanline);
        clearDownsampleBuffers();
        current_downsampled_scanline++;

        // this represents the _first_ true scanline the _new_ downsampling line needs to process
        const uint8_t first_downsample_line = (current_downsampled_scanline * DOWNSAMPLING_FACTOR_OUT_OF_100) / 100;

        if (first_downsample_line == scanline) {
            areaAverageDownsampleLine(src, downsampled_row, 4);
        }
    }
}




// these could have the same signature, but we have no need of the end of the array
uint16_t nearestNeighborDownsamplePixelGroup(uint16_t *src) {
    return src[0]; // lol
}

void nearestNeighborDownsampleLine(uint16_t *src, uint16_t *dest) {
    for (uint16_t x = 0; x < DOWNSAMPLED_WIDTH; x++) {
      // Calculate the range of source pixels to average for this destination pixel
      // we don't have to calculate the actual window for nearest neighbor, which is nice
      uint16_t start = (uint16_t)((x * DOWNSAMPLING_FACTOR_OUT_OF_100) / 100);
      
      dest[x] = nearestNeighborDownsamplePixelGroup(&src[start]);
    }
}

void nearestNeighborHandleFrameStart() {
    clearDownsampleBuffers();
    current_downsampled_scanline = 0;
}

// TODO this chooses the last pixel, I want it to choose the first pixel
// TODO maybe this just returns whenever scanline == first_downsample_line
void nearestNeighborHandleDownsampling(uint16_t *src, int scanline, void (*callback)(uint16_t *, int)) {
    nearestNeighborDownsampleLine(src, downsampled_row); // TODO reject if already done

    // this represents the last true scanline the current downsampling line needs to process
    const uint8_t last_downsample_line = ((current_downsampled_scanline + 1) * DOWNSAMPLING_FACTOR_OUT_OF_100 - 1) / 100;

    // if floor(current_downsampled_scanline * DOWNSAMPLING_FACTOR) == scanline
    if (last_downsample_line == scanline) {
        callback(downsampled_row, current_downsampled_scanline);
        clearDownsampleBuffers();
        current_downsampled_scanline++;

        // this represents the _first_ true scanline the _new_ downsampling line needs to process
        const uint8_t first_downsample_line = (current_downsampled_scanline * DOWNSAMPLING_FACTOR_OUT_OF_100) / 100;

        if (first_downsample_line == scanline) {
            nearestNeighborDownsampleLine(src, downsampled_row);
        }
    } 
}


// Block Average (Box Sampling) Implementation
// This accumulates all pixels in a block and averages them equally

// We need separate accumulation buffers for RGB channels to avoid overflow
static uint16_t block_r_accum[DOWNSAMPLED_WIDTH] = {0};
static uint16_t block_g_accum[DOWNSAMPLED_WIDTH] = {0};
static uint16_t block_b_accum[DOWNSAMPLED_WIDTH] = {0};
static uint8_t block_scanline_count = 0;

// Note: Bit shift optimization removed since we now support fractional downsampling

void clearBlockAverageBuffers() {
    memset(block_r_accum, 0, sizeof(block_r_accum[0]) * DOWNSAMPLED_WIDTH);
    memset(block_g_accum, 0, sizeof(block_g_accum[0]) * DOWNSAMPLED_WIDTH);
    memset(block_b_accum, 0, sizeof(block_b_accum[0]) * DOWNSAMPLED_WIDTH);
    block_scanline_count = 0;
}

void blockAverageAccumulateLine(uint16_t *src) {
    // Process each downsampled pixel using fractional downsampling like downsample_scanline
    for (uint16_t x = 0; x < DOWNSAMPLED_WIDTH; x++) {
        // Calculate the range of source pixels for this destination pixel
        uint16_t start = (x * DOWNSAMPLING_FACTOR_OUT_OF_100) / 100;
        uint16_t end = ((x + 1) * DOWNSAMPLING_FACTOR_OUT_OF_100) / 100;
        
        uint16_t r_sum = 0, g_sum = 0, b_sum = 0;
        
        // Accumulate all pixels in this horizontal range
        for (uint16_t px = start; px < end; px++) {
            uint16_t pixel = src[px];
            // Extract RGB components (avoiding redundant masking after shift)
            r_sum += pixel >> 11;
            g_sum += (pixel >> 5) & 0x3F;  // 6 bits for green
            b_sum += pixel & 0x1F;          // 5 bits for blue
        }
        
        // Add to accumulator
        block_r_accum[x] += r_sum;
        block_g_accum[x] += g_sum;
        block_b_accum[x] += b_sum;
    }
    block_scanline_count++;
}

void blockAverageOutputLine(uint16_t *dest) {
    // Calculate total pixels accumulated per downsampled pixel
    // For each downsampled pixel, we need to know how many source pixels contributed
    for (uint16_t x = 0; x < DOWNSAMPLED_WIDTH; x++) {
        // Calculate the range of source pixels for this destination pixel
        uint16_t start = (x * DOWNSAMPLING_FACTOR_OUT_OF_100) / 100;
        uint16_t end = ((x + 1) * DOWNSAMPLING_FACTOR_OUT_OF_100) / 100;
        uint16_t pixels_per_x = end - start;
        if (pixels_per_x == 0) pixels_per_x = 1; // Avoid division by zero
        
        // Total pixels for this downsampled pixel = horizontal_pixels * vertical_scanlines
        uint16_t total_pixels = pixels_per_x * block_scanline_count;
        
        // Average the accumulated values
        uint8_t r = block_r_accum[x] / total_pixels;
        uint8_t g = block_g_accum[x] / total_pixels;
        uint8_t b = block_b_accum[x] / total_pixels;
        
        // Reconstruct pixel
        dest[x] = (r << 11) | (g << 5) | b;
    }
}

void blockAverageHandleFrameStart() {
    clearDownsampleBuffers();
    clearBlockAverageBuffers();
    current_downsampled_scanline = 0;
}

void blockAverageHandleDownsampling(uint16_t *src, int scanline, void (*callback)(uint16_t *, int)) {
    // Accumulate this scanline
    blockAverageAccumulateLine(src);
    
    // Check if this is the last scanline for the current downsampled row using fractional calculation
    const uint16_t last_downsample_line = ((current_downsampled_scanline + 1) * DOWNSAMPLING_FACTOR_OUT_OF_100) / 100 - 1;
    
    if (last_downsample_line == scanline) {
        // Output the averaged downsampled row
        blockAverageOutputLine(downsampled_row);
        callback(downsampled_row, current_downsampled_scanline);
        
        // Clear buffers for next downsampled row
        clearBlockAverageBuffers();
        current_downsampled_scanline++;
    }
}