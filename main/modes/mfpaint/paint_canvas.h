#ifndef _PAINT_CANVAS_H_
#define _PAINT_CANVAS_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "palette.h"
#include "wsg.h"

// The number of colors in the palette and the max number of colors an image can be saved with
#define PAINT_MAX_COLORS 16

/// @brief Definition for a paintable screen region
typedef struct
{
    // The X and Y offset of the canvas's top-left pixel
    uint16_t x, y;

    // The canvas's width and height, in "canvas pixels"
    uint16_t w, h;

    // The X and Y scale of the canvas. Each "canvas pixel" will be drawn as [xScale x yScale]
    uint8_t xScale, yScale;

    paletteColor_t palette[PAINT_MAX_COLORS];

    // Whether this canvas is backed off-screen in a buffer
    bool buffered;
    // The buffer, if this canvas has one
    uint8_t* buffer;

} paintCanvas_t;

void paintBlitCanvas(const paintCanvas_t* canvas);
void paintSyncCanvas(paintCanvas_t* canvas);

size_t paintGetStoredSize(const paintCanvas_t* canvas);
size_t paintGetStoredSizeDim(uint16_t w, uint16_t h);
int8_t paintGetPaletteIndex(const paletteColor_t palette[16], paletteColor_t color);
void paintRebuildPalette(paletteColor_t palette[16], const paletteColor_t* img, uint16_t w, uint16_t h);

bool paintDeserialize(const paintCanvas_t* dest, const uint8_t* data, size_t offset, size_t count);
size_t paintSerializeWsg(uint8_t* dest, const wsg_t* wsg);
size_t paintSerializeWsgPalette(uint8_t* dest, const wsg_t* wsg, const paletteColor_t palette[16]);
size_t paintSerialize(uint8_t* dest, const paintCanvas_t* canvas, size_t offset, size_t count);

#endif
