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

#pragma once

//==============================================================================
// Includes
//==============================================================================

#include "cnfs.h"

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
void canvasDraw(wsg_t* canvas, cnfsFileIdx_t image, int startX, int startY);

/**
 * @brief Draws a image to the canvas at the specified coordinates relative to the canvas and applies a palette
 *
 * @param canvas WSG to save changes to. Will work with any WSG, cannot be reverted
 * @param image New cnfsFileIdx_t to apply to the canvas
 * @param startX X position on canvas. Negative moves left, positive moves right. Pixels not on canvas are cropped.
 * @param startY Y position on canvas. Negative moves left, positive moves right. Pixels not on canvas are cropped.
 * @param pal Palette data to use to transform the image
 */
void canvasDrawPalette(wsg_t* canvas, cnfsFileIdx_t image, int startX, int startY, wsgPalette_t pal);

/**
 * @brief Draws a image to the canvas at the specified coordinates relative to the canvas. Allows flipping for
 * mirroring.
 *
 * @param canvas WSG to save changes to. Will work with any WSG, cannot be reverted
 * @param image New cnfsFileIdx_t to apply to the canvas
 * @param startX X position on canvas. Negative moves left, positive moves right. Pixels not on canvas are cropped.
 * @param startY Y position on canvas. Negative moves left, positive moves right. Pixels not on canvas are cropped.
 * @param flipX Flips applied image in the x direction
 * @param flipY Flips applied image in the Y direction
 */
void canvasDrawFlip(wsg_t* canvas, cnfsFileIdx_t image, int startX, int startY, bool flipX, bool flipY);

/**
 * @brief Draws a image to the canvas at the specified coordinates relative to the canvas and applies a palette. Allows
 * flipping for mirroring.
 *
 * @param canvas WSG to save changes to. Will work with any WSG, cannot be reverted
 * @param image New cnfsFileIdx_t to apply to the canvas
 * @param startX X position on canvas. Negative moves left, positive moves right. Pixels not on canvas are cropped.
 * @param startY Y position on canvas. Negative moves left, positive moves right. Pixels not on canvas are cropped.
 * @param flipX Flips applied image in the x direction
 * @param flipY Flips applied image in the Y direction
 * @param pal Palette data to use to transform the image
 */
void canvasDrawPaletteFlip(wsg_t* canvas, cnfsFileIdx_t image, int startX, int startY, bool flipX, bool flipY, wsgPalette_t pal);

/**
 * @brief Frees the canvas
 *
 * @param canvas Canvas to free. Ensure this is run when closing down the program
 */
void canvasFree(wsg_t* canvas);