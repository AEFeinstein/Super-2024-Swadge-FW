/*! \file bresenham.h
 * \author Zingl Alois
 * \date 22.08.2016
 * \version 1.2
 *
 * \section bresenham_design Design Philosophy
 *
 * TODO doxygen
 * This is based on <a href="https://github.com/zingl/Bresenham/blob/master/bresenham.c">Zingl Alois's bresenham.c</a>.
 * More explanation can be found <a href="https://zingl.github.io/bresenham.html">on their webpage</a>.
 *
 * \section bresenham_usage Usage
 *
 * TODO doxygen
 *
 * \section bresenham_example Example
 *
 * \code{.c}
 * TODO doxygen
 * \endcode
 */

#ifndef SRC_BRESENHAM_H_
#define SRC_BRESENHAM_H_

#include <stdbool.h>
#include "palette.h"

void drawLine(int x0, int y0, int x1, int y1, paletteColor_t col, int dashWidth, int xTr, int yTr, int xScale,
              int yScale);
void drawLineScaled(int x0, int y0, int x1, int y1, paletteColor_t col, int dashWidth, int xTr, int yTr, int xScale,
                    int yScale);
void drawLineFast(int16_t x0, int16_t y0, int16_t x1, int16_t y1, paletteColor_t color);

void drawRect(int x0, int y0, int x1, int y1, paletteColor_t col, int xTr, int yTr, int xScale, int yScale);
void drawRectScaled(int x0, int y0, int x1, int y1, paletteColor_t col, int xTr, int yTr, int xScale, int yScale);

void drawTriangleOutlined(int16_t v0x, int16_t v0y, int16_t v1x, int16_t v1y, int16_t v2x, int16_t v2y,
                          paletteColor_t colorA, paletteColor_t colorB);

void drawEllipse(int xm, int ym, int a, int b, paletteColor_t col, int xTr, int yTr, int xScale, int yScale);
void drawEllipseScaled(int xm, int ym, int a, int b, paletteColor_t col, int xTr, int yTr, int xScale, int yScale);

void drawCircle(int xm, int ym, int r, paletteColor_t col, int xTr, int yTr, int xScale, int yScale);
void drawCircleScaled(int xm, int ym, int r, paletteColor_t col, int xTr, int yTr, int xScale, int yScale);

void drawCircleFilled(int xm, int ym, int r, paletteColor_t col, int xTr, int yTr, int xScale, int yScale);
void drawCircleFilledScaled(int xm, int ym, int r, paletteColor_t col, int xTr, int yTr, int xScale, int yScale);

void drawEllipseRect(int x0, int y0, int x1, int y1, paletteColor_t col, int xTr, int yTr, int xScale, int yScale);
void drawEllipseRectScaled(int x0, int y0, int x1, int y1, paletteColor_t col, int xTr, int yTr, int xScale,
                           int yScale);

void drawQuadBezierSeg(int x0, int y0, int x1, int y1, int x2, int y2, paletteColor_t col, int xTr, int yTr, int xScale,
                       int yScale);
void drawQuadBezierSegScaled(int x0, int y0, int x1, int y1, int x2, int y2, paletteColor_t col, int xTr, int yTr,
                             int xScale, int yScale);

void drawQuadBezier(int x0, int y0, int x1, int y1, int x2, int y2, paletteColor_t col, int xTr, int yTr, int xScale,
                    int yScale);
void drawQuadBezierScaled(int x0, int y0, int x1, int y1, int x2, int y2, paletteColor_t col, int xTr, int yTr,
                          int xScale, int yScale);

void drawCubicBezierSeg(int x0, int y0, float x1, float y1, float x2, float y2, int x3, int y3, paletteColor_t col,
                        int xTr, int yTr, int xScale, int yScale);
void drawCubicBezierSegScaled(int x0, int y0, float x1, float y1, float x2, float y2, int x3, int y3,
                              paletteColor_t col, int xTr, int yTr, int xScale, int yScale);

void drawCubicBezier(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, paletteColor_t col, int xTr,
                     int yTr, int xScale, int yScale);
void drawCubicBezierScaled(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, paletteColor_t col, int xTr,
                           int yTr, int xScale, int yScale);

void drawQuadRationalBezierSeg(int x0, int y0, int x1, int y1, int x2, int y2, float w, paletteColor_t col);
void drawQuadRationalBezier(int x0, int y0, int x1, int y1, int x2, int y2, float w, paletteColor_t col);
void drawRotatedEllipse(int x, int y, int a, int b, float angle, paletteColor_t col);
void drawRotatedEllipseRect(int x0, int y0, int x1, int y1, long zd, paletteColor_t col);
void drawQuadSpline(int n, int x[], int y[], paletteColor_t col);
void drawCubicSpline(int n, int x[], int y[], paletteColor_t col);

void drawOptimizedEllipse(int xm, int ym, int a, int b, paletteColor_t col);
void drawCircleQuadrants(int xm, int ym, int r, bool q1, bool q2, bool q3, bool q4, paletteColor_t col);

#endif /* SRC_BRESENHAM_H_ */
