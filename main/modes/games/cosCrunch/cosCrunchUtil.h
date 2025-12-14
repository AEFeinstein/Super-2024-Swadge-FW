/*! \file cosCrunchUtil.h
 *
 * This file contains utilities for Cosplay Crunch microgames, mostly centered around tinting wsg images.
 */

#pragma once

#include "wsg.h"
#include "wsgPalette.h"

/// Palette lowlight color to be tinted in grayscale images. See tintPalette()
#define PALETTE_LOWLIGHT c111
/// Palette base color to be tinted in grayscale images. See tintPalette()
#define PALETTE_BASE c222
/// Palette alt base color to be tinted in grayscale images that need an extra color. See tintPalette()
#define PALETTE_BASE_ALT c333
/// Palette highlight color to be tinted in grayscale images. See tintPalette()
#define PALETTE_HIGHLIGHT c444

/// A set of colors used to tint a greyscale image
typedef struct
{
    paletteColor_t lowlight;
    paletteColor_t base;
    paletteColor_t highlight;
    paletteColor_t baseAlt;
} tintColor_t;

/**
 * @brief Modifies a palette to be used for tinting greyscale images. The resulting palette is intended to be used with
 * the `wsgPalette*` functions.
 *
 * @param palette The palette to be modified
 * @param tintColor The color set to apply to the grey tones of the palette
 */
void tintPalette(wsgPalette_t* palette, const tintColor_t* tintColor);

/**
 * @brief Draws a wsg image onto another wsg image.
 *
 * @param canvas A wsg to draw onto
 * @param wsg The image to draw to the canvas
 * @param x x position to draw the wsg
 * @param y y position to draw the wsg
 */
void drawToCanvas(wsg_t canvas, wsg_t wsg, uint16_t x, uint16_t y);

/**
 * @brief Draws a wsg image drawn in greyscale onto another wsg image, tinting the grayscale pixels. This function can
 * be used to tint a greyscale image in place.
 *
 * @param canvas A wsg to draw onto
 * @param wsg The image to draw to the canvas
 * @param x x position to draw the wsg
 * @param y y position to draw the wsg
 * @param rotationDeg
 * @param tintColor The color used to tint a greyscale wsg
 */
void drawToCanvasTint(wsg_t canvas, wsg_t wsg, int32_t x, int32_t y, int32_t rotationDeg, const tintColor_t* tintColor);

/**
 * @brief Draws a wsg image onto another wsg image without taking transparent pixels into account.
 *
 * @param canvas A wsg to draw onto
 * @param wsg The image to draw to the canvas
 * @param x x position to draw the wsg
 * @param y y position to draw the wsg
 */
void drawToCanvasTile(wsg_t canvas, wsg_t wsg, uint16_t x, uint16_t y);

/**
 * @brief Draws a box styled like a piece of paper for displaying text.
 *
 * @param x1 Left position of box
 * @param y1 Top position of box
 * @param x2 Right position of box
 * @param y2 Bottom position of box
 * @param fold Pointer to a top-right corner image. You probably want \ref CC_MENU_FOLD_WSG
 */
void drawMessageBox(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, wsg_t fold);
