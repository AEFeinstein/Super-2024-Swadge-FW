/**
 * @file wsgCanvas.h
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief Provides a canvas to paint with low memory requirements
 * @version 1.0
 * @date 2025-09-02
 *
 * @copyright Copyright (c) 2025
 *
 */

/*! \file wsgCanvas.h
 *
 * \section wsgCanvas_overview Overview
 *
 * The WSG Canvas system is a simple way of combining wsgs together to create a single sprite. Why is this useful? Well,
 * it was initially designed for the swadgesonas, which would require loading a *lot* of individual WSGs and leaving
 * them loaded, which would be pretty memory intensive. Instead, this canvas system allows for a WSG to be created once,
 * and all the other WSGs applied over it are only loaded when the data is needed, and then immediately discarded. This
 * is a much more efficient way to store swadgesonas, and will allow many of them to be on-screen at once.
 *
 * \section wsgCanvas_usage Usage
 *
 * Step one is to load up a canvas. Use `canvasBlankInit()` to create a blank canvas on which to paint, or load a normal
 * WSG. Just note that any changes you make cannot be revered without re-initializing the WSG.
 *
 * Next, use canvasDraw() to put WSGs onto the canvas. YOu can rotate, flip in both x and y, and position the WSG
 * however you like on the canvas. Pixels outside of the boundaries of the canvas will be discarded, and transparent
 * pixels will leave the color underneath to show through, allowing for complex layering.
 *
 * Palettes are also supported, so things like custom hair colors can be applied without having to rely on a drawing
 * function further down the line.
 *
 * Two additional "simple" versions have been provided that perform slightly faster is rotation and flipping aren't
 * required but speed is.
 *
 * Lastly, call `canvasFree()` when done with the canvas, just like any other WSG.
 *
 * The canvas can be used like any WSG, so all wsg drawing functions work on it once it has been created.
 * 
 * \code {.c}
// Create structs
wsg_t canvas;

// Initialize canvas
canvasBlankInit(&canvas, 280, 240, c444, true);

// Draw to the canvas normally
canvasDrawSimple(&canvas, KID_0_WSG, 40, 48);

// Draw to the canvas with a palette
wsgPalette_t palette;
wsgPaletteReset(&palette);
canvasDrawPal(&canvas, KID_1_WSG, 160, 48, true, false, 15, palette);

// Clean up
canvasFree(&canvas);
 * \endcode
 */

#pragma once

//==============================================================================
// Includes
//==============================================================================

#include "wsg.h"
#include "wsgPalette.h"

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initializes a blank canvas to the dimensions and color provided
 *
 * @param canvas WSG to save the canvas too. Use a standard wsg_t and provide the pointer
 * @param width Width in pixels of the desired canvas
 * @param height Height in pixels of the desired canvas
 * @param startColor The initial color. Can be any paletteColor_t including cTransparent
 * @param spiRam Whether or not to load the pixel data into SPIRAM.
 */
void canvasBlankInit(wsg_t* canvas, int width, int height, paletteColor_t startColor, bool spiRam);

/**
 * @brief Draws a image to the canvas at the specified coordinates relative to the canvas.
 *
 * @param canvas WSG to save changes to. Will work with any WSG, cannot be reverted
 * @param image New cnfsFileIdx_t to apply to the canvas
 * @param startX X position on canvas. Negative moves left, positive moves right. Pixels not on canvas are cropped.
 * @param startY Y position on canvas. Negative moves left, positive moves right. Pixels not on canvas are cropped.
 */
void canvasDrawSimple(wsg_t* canvas, cnfsFileIdx_t image, int startX, int startY);

/**
 * @brief Draws a image to the canvas at the specified coordinates relative to the canvas and applies a palette
 *
 * @param canvas WSG to save changes to. Will work with any WSG, cannot be reverted
 * @param image New cnfsFileIdx_t to apply to the canvas
 * @param startX X position on canvas. Negative moves left, positive moves right. Pixels not on canvas are cropped.
 * @param startY Y position on canvas. Negative moves left, positive moves right. Pixels not on canvas are cropped.
 * @param pal Palette data to use to transform the image
 */
void canvasDrawSimplePal(wsg_t* canvas, cnfsFileIdx_t image, int startX, int startY, wsgPalette_t pal);

/**
 * @brief Draws an image to the canvas at a specified angle.
 *
 * @param canvas WSG to save changes to. Will work with any WSG, cannot be reverted
 * @param image New cnfsFileIdx_t to apply to the canvas
 * @param startX X position on canvas. Negative moves left, positive moves right. Pixels not on canvas are cropped.
 * @param startY Y position on canvas. Negative moves left, positive moves right. Pixels not on canvas are cropped.
 * @param flipX Flips applied image in the x direction
 * @param flipY Flips applied image in the Y direction
 * @param rotateDeg Angle to draw the image at
 */
void canvasDraw(wsg_t* canvas, cnfsFileIdx_t image, int startX, int startY, bool flipX, bool flipY, int32_t rotateDeg);

/**
 * @brief Draws an image to the canvas at a specified angle.
 *
 * @param canvas WSG to save changes to. Will work with any WSG, cannot be reverted
 * @param image New cnfsFileIdx_t to apply to the canvas
 * @param startX X position on canvas. Negative moves left, positive moves right. Pixels not on canvas are cropped.
 * @param startY Y position on canvas. Negative moves left, positive moves right. Pixels not on canvas are cropped.
 * @param flipX Flips applied image in the x direction
 * @param flipY Flips applied image in the Y direction
 * @param rotateDeg Angle to draw the image at
 * @param pal Palette data to use to transform the image
 */
void canvasDrawPal(wsg_t* canvas, cnfsFileIdx_t image, int startX, int startY, bool flipX, bool flipY,
                   int32_t rotateDeg, wsgPalette_t pal);

/**
 * @brief Frees the canvas
 *
 * @param canvas Canvas to free. Ensure this is run when closing down the program
 */
void canvasFree(wsg_t* canvas);