#ifndef _PAINT_UI_H_
#define _PAINT_UI_H_

#include <stdint.h>
#include <stdbool.h>

#include "palette.h"

#include "paint_type.h"
#include "paint_common.h"

void restoreCursorPixels(void);

void plotCursor(void);
void paintRenderCursor(void);

void drawColorBox(uint16_t xOffset, uint16_t yOffset, uint16_t w, uint16_t h, paletteColor_t col, bool selected,
                  paletteColor_t topBorder, paletteColor_t bottomBorder);
void paintRenderToolbar(paintArtist_t* artist, paintCanvas_t* canvas, paintDraw_t* paintState,
                        const brush_t* firstBrush, const brush_t* lastBrush);
uint16_t paintRenderGradientBox(paintCanvas_t* canvas, char channel, paletteColor_t col, uint16_t x, uint16_t y,
                                uint16_t barW, uint16_t h, bool selected);
void paintRenderColorPicker(paintArtist_t* artist, paintCanvas_t* canvas, paintDraw_t* paintState);
void paintRenderAll(void);

void paintClearCanvas(const paintCanvas_t* canvas, paletteColor_t bgColor);

bool paintGenerateCursorSprite(wsg_t* sprite, const paintCanvas_t* canvas, uint8_t size);
void paintFreeCursorSprite(wsg_t* sprite);

void initCursor(paintCursor_t* cursor, paintCanvas_t* canvas, const wsg_t* sprite);
void deinitCursor(paintCursor_t* cursor);
void setCursorSprite(paintCursor_t* cursor, paintCanvas_t* canvas, const wsg_t* sprite);
void setCursorOffset(paintCursor_t* cursor, int16_t x, int16_t y);
void undrawCursor(paintCursor_t* cursor, paintCanvas_t* canvas);
void hideCursor(paintCursor_t* cursor, paintCanvas_t* canvas);
bool showCursor(paintCursor_t* cursor, paintCanvas_t* canvas);
bool drawCursor(paintCursor_t* cursor, paintCanvas_t* canvas);
void moveCursorRelative(paintCursor_t* cursor, paintCanvas_t* canvas, int16_t xDiff, int16_t yDiff);
void moveCursorAbsolute(paintCursor_t* cursor, paintCanvas_t* canvas, uint16_t x, uint16_t y);

#endif
