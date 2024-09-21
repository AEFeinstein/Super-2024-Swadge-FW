/*! \file wsg.h
 *
 * \section wsg_design Design Philosophy
 *
 * WSG is a simple image format developed for Swadge sprites.
 * Each pixel in a WSG is from the web-safe palette of colors, with one extra value indicating transparent
 * (::paletteColor_t). The pixels are then compressed with <a
 * href="https://github.com/atomicobject/heatshrink">heatshrink compression</a>.
 *
 * WSGs are handled individually, not in a sheet.
 * The \c assets_preprocessor program will take PNG files and convert them to WSG.
 * The cmake build will take the generated WSG files and generate \c cnfs_image.c which is included in the firmware
 * Swadge. See \c assets_preprocessor in \c CMakeLists.txt.
 *
 * WSG files can be loaded from the filesystem with helper functions in fs_wsg.h.
 * Once loaded from the filesystem WSGs can be drawn to the display.
 *
 * \section wsg_usage Usage
 *
 * There are five ways to draw a WSG to the display each with varying complexity and speed
 * - drawWsg(): Draw a WSG to the display with transparency, rotation, and flipping over horizontal or vertical axes.
 * This is the slowest option.
 * - drawWsgSimple(): Draw a WSG to the display with transparency. This is the medium speed option and should be used if
 * the WSG is not rotated or flipped.
 * - drawWsgTile(): Draw a WSG to the display without transparency. Any transparent pixels will be an indeterminate
 * color. This is the fastest option, and best for background tiles or images.
 * - drawWsgSimpleScaled():  Draw a WSG to the display with transparency at a specified scale. Scales are integer
 * values, so 2x, 3x, 4x... are the valid options.
 * - drawWsgSimpleHalf(): Draw a WSG to the display with transparency at half the original resolution.
 *
 * \section wsg_example Example
 *
 * \code{.c}
 * // Declare and load an image
 * wsg_t king_donut;
 * loadWsg("kid0.wsg", &king_donut, true);
 *
 * // Draw the image to the display
 * drawWsg(&king_donut, 100, 100, false, false, 0);
 *
 * // Free the image
 * freeWsg(&king_donut);
 * \endcode
 */

#ifndef _WSG_H_
#define _WSG_H_

#include <stdint.h>
#include <palette.h>
#include <stdbool.h>

/**
 * @brief A sprite using paletteColor_t colors that can be drawn to the display
 */
typedef struct
{
    paletteColor_t* px; ///< The row-order array of pixels in the image
    uint16_t w;         ///< The width of the image
    uint16_t h;         ///< The height of the image
} wsg_t;

void rotatePixel(int16_t* x, int16_t* y, int16_t rotateDeg, int16_t width, int16_t height);
void drawWsg(const wsg_t* wsg, int16_t xOff, int16_t yOff, bool flipLR, bool flipUD, int16_t rotateDeg);
void drawWsgSimple(const wsg_t* wsg, int16_t xOff, int16_t yOff);
void drawWsgSimpleScaled(const wsg_t* wsg, int16_t xOff, int16_t yOff, int16_t xScale, int16_t yScale);
void drawWsgTile(const wsg_t* wsg, int32_t xOff, int32_t yOff);
void drawWsgSimpleHalf(const wsg_t* wsg, int16_t xOff, int16_t yOff);

#endif