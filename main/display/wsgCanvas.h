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

void canvasBlankInit(wsg_t* canvas, int width, int height, paletteColor_t startColor, bool spiRam);

void canvasDraw(wsg_t* canvas, cnfsFileIdx_t image, int startX, int startY);

void canvasDrawPalette(wsg_t* canvas, cnfsFileIdx_t image, int startX, int startY, wsgPalette_t pal);