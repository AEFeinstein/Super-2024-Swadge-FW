#pragma once

#include "wsg.h"
#include "cnfs.h"

void canvasBlankInit(wsg_t* canvas, int width, int height, paletteColor_t startColor, bool spiRam);

void canvasDraw(wsg_t* canvas, cnfsFileIdx_t splat, int startX, int startY);

void canvasDrawPalette(wsg_t* canvas, cnfsFileIdx_t splat, int startX, int startY);