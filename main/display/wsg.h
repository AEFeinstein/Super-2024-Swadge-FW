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
 * The \c spiffs_file_preprocessor program will take PNG files and convert them to WSG.
 * The cmake build will take the generated WSG files and generate a SPIFFS image with the files which is flashed to the
 * Swadge. See \c spiffs_create_partition_image in \c CMakeLists.txt.
 *
 * WSG files can be loaded from the SPIFFS filesystem with helper functions in spiffs_wsg.h.
 * Once loaded from the SPIFFS filesystem WSGs can be drawn to the display.
 *
 * \section wsg_usage Usage
 *
 * There are three ways to draw a WSG to the display each with varying complexity and speed
 * - drawWsg(): Draw a WSG to the display with transparency, rotation, and flipping over horizontal or vertical axes.
 * This is the slowest option.
 * - drawWsgSimple(): Draw a WSG to the display with transparency. This is the medium speed option and should be used if
 * the WSG is not rotated or flipped.
 * - drawWsgTile(): Draw a WSG to the display without transparency. Any transparent pixels will be an indeterminate
 * color. This is the fastest option, and best for background tiles or images.
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

typedef struct
{
    paletteColor_t* px; ///< The row-order array of pixels in the image
    uint16_t w;         ///< The width of the image
    uint16_t h;         ///< The height of the image
} wsg_t;

void drawWsg(const wsg_t* wsg, int16_t xOff, int16_t yOff, bool flipLR, bool flipUD, int16_t rotateDeg);

void drawWsgSimple(const wsg_t* wsg, int16_t xOff, int16_t yOff);

void drawWsgTile(const wsg_t* wsg, int32_t xOff, int32_t yOff);

#endif