/*! \file shapes.h
 * \author Zingl Alois
 * \date 22.08.2016
 * \version 1.2
 *
 * \section shapes_design Design Philosophy
 *
 * The shape and curve drawing code is based on <a
 * href="https://github.com/zingl/Bresenham/blob/master/bresenham.c">Zingl Alois's bresenham.c</a>. More explanation
 * can be found <a href="https://zingl.github.io/bresenham.html">on their webpage</a>. An <a
 * href="http://members.chello.at/~easyfilter/bresenham.c">older version of bresenham.c</a> can be found with an
 * older description on <a href="http://members.chello.at/~easyfilter/bresenham.html">an older webpage</a>.
 *
 * Most, but not all, functions have \c Scaled and normal versions. \c Scaled functions can both scale and translate a
 * shape or curve before drawing, while normal functions draw one line widths. \c Scaled functions were originally
 * written for MFPaint to draw to a restricted area.
 *
 * Some functions, like drawTriangleOutlined() and drawLineFast() were written for the Swadge and not based on the
 * original bresenham.c.
 *
 * \section shapes_usage Usage
 *
 * initShapes() is called automatically before the Swadge mode is run. It should not be called from within a Swadge
 * Mode.
 *
 * Draw shapes and curves with the given functions. Each function has it's own description below that won't be copied
 * here.
 *
 * \section shapes_example Example
 *
 * \code{.c}
 * // Draw a blue line
 * drawLineFast(102, 92, 210, 200, c005);
 *
 * // Draw a red circle
 * drawCircle(200, 50, 20, c500);
 *
 * // Draw a green rectangle
 * drawRect(200, 150, 250, 220, c050);
 * \endcode
 */

#ifndef SRC_BRESENHAM_H_
#define SRC_BRESENHAM_H_

#include <stdbool.h>
#include "palette.h"

void drawLineFast(int16_t x0, int16_t y0, int16_t x1, int16_t y1, paletteColor_t color);
void drawLine(int x0, int y0, int x1, int y1, paletteColor_t col, int dashWidth);
void drawLineScaled(int x0, int y0, int x1, int y1, paletteColor_t col, int dashWidth, int xOrigin, int yOrigin,
                    int xScale, int yScale);
void drawRect(int x0, int y0, int x1, int y1, paletteColor_t col);
void drawRectScaled(int x0, int y0, int x1, int y1, paletteColor_t col, int xOrigin, int yOrigin, int xScale,
                    int yScale);
void drawTriangleOutlined(int16_t v0x, int16_t v0y, int16_t v1x, int16_t v1y, int16_t v2x, int16_t v2y,
                          paletteColor_t fillColor, paletteColor_t outlineColor);
void drawEllipse(int xm, int ym, int a, int b, paletteColor_t col);
void drawEllipseScaled(int xm, int ym, int a, int b, paletteColor_t col, int xOrigin, int yOrigin, int xScale,
                       int yScale);
void drawCircle(int xm, int ym, int r, paletteColor_t col);
void drawCircleScaled(int xm, int ym, int r, paletteColor_t col, int xOrigin, int yOrigin, int xScale, int yScale);
void drawCircleQuadrants(int xm, int ym, int r, bool q1, bool q2, bool q3, bool q4, paletteColor_t col);
void drawCircleFilled(int xm, int ym, int r, paletteColor_t col);
void drawCircleFilledScaled(int xm, int ym, int r, paletteColor_t col, int xOrigin, int yOrigin, int xScale,
                            int yScale);
void drawCircleOutline(int xm, int ym, int r, int stroke, paletteColor_t col);
void drawEllipseRect(int x0, int y0, int x1, int y1, paletteColor_t col);
void drawEllipseRectScaled(int x0, int y0, int x1, int y1, paletteColor_t col, int xOrigin, int yOrigin, int xScale,
                           int yScale);
void drawQuadBezierSeg(int x0, int y0, int x1, int y1, int x2, int y2, paletteColor_t col);
void drawQuadBezierSegScaled(int x0, int y0, int x1, int y1, int x2, int y2, paletteColor_t col, int xOrigin,
                             int yOrigin, int xScale, int yScale);
void drawQuadBezier(int x0, int y0, int x1, int y1, int x2, int y2, paletteColor_t col);
void drawQuadBezierScaled(int x0, int y0, int x1, int y1, int x2, int y2, paletteColor_t col, int xOrigin, int yOrigin,
                          int xScale, int yScale);
void drawQuadRationalBezierSeg(int x0, int y0, int x1, int y1, int x2, int y2, float w, paletteColor_t col);
void drawQuadRationalBezier(int x0, int y0, int x1, int y1, int x2, int y2, float w, paletteColor_t col);
void drawRotatedEllipse(int x, int y, int a, int b, float angle, paletteColor_t col);
void drawRotatedEllipseRect(int x0, int y0, int x1, int y1, long zd, paletteColor_t col);
void drawCubicBezierSeg(int x0, int y0, float x1, float y1, float x2, float y2, int x3, int y3, paletteColor_t col);
void drawCubicBezier(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, paletteColor_t col);
void drawCubicBezierScaled(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, paletteColor_t col,
                           int xOrigin, int yOrigin, int xScale, int yScale);
void drawQuadSpline(int n, int x[], int y[], paletteColor_t col);
void drawCubicSpline(int n, int x[], int y[], paletteColor_t col);

void initShapes(void);

#endif /* SRC_BRESENHAM_H_ */
