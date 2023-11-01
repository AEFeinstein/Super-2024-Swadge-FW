#ifndef _PAINT_UTIL_H_
#define _PAINT_UTIL_H_

#include <stdbool.h>
#include <stdint.h>

#include "palette.h"
#include "wsg.h"

#include "paint_type.h"
#include "px_stack.h"

paletteColor_t getContrastingColor(paletteColor_t col);
paletteColor_t getContrastingColorBW(paletteColor_t col);

void colorReplaceWsg(wsg_t* wsg, paletteColor_t find, paletteColor_t replace);

// Extra drawing functions
bool paintDrawWsgTemp(const wsg_t* wsg, pxStack_t* saveTo, uint16_t x, uint16_t y, colorMapFn_t colorSwap);

void paintPlotSquareWave(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t waveLength, paletteColor_t col,
                         int xTr, int yTr, int xScale, int yScale);
void drawRectFilled(int x0, int y0, int x1, int y1, paletteColor_t col);
void drawRectFilledScaled(int x0, int y0, int x1, int y1, paletteColor_t col, int xTr, int yTr, int xScale, int yScale);
void paintColorReplace(paintCanvas_t* canvas, paletteColor_t search, paletteColor_t replace);

void setPxScaled(int x, int y, paletteColor_t col, int xTr, int yTr, int xScale, int yScale);

uint8_t paintGetMaxScale(uint16_t imgW, uint16_t imgH, uint16_t xMargin, uint16_t yMargin);

// void paintConvertPickPoints(const pxStack_t* pxStack, point_t* dest);
void paintConvertPickPointsScaled(const pxStack_t* pxStack, paintCanvas_t* canvas, point_t* dest);

uint16_t canvasToDispX(const paintCanvas_t* canvas, uint16_t x);
uint16_t canvasToDispY(const paintCanvas_t* canvas, uint16_t y);

void swap(uint8_t* a, uint8_t* b);

#endif
