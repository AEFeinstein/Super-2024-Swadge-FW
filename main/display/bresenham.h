/*
 * bresenham.h
 *
 *  Created on: Mar 3, 2019
 *      Author: Adam Feinstein
 */

#ifndef SRC_BRESENHAM_H_
#define SRC_BRESENHAM_H_

#include "palette.h"

typedef int (*translateFn_t)(int);

void drawLineScaled(int x0, int y0, int x1, int y1, paletteColor_t col, int dashWidth, int xTr, int yTr, int xScale,
                    int yScale);
void drawLine(int x0, int y0, int x1, int y1, paletteColor_t col, int dashWidth);
void drawRect(int x0, int y0, int x1, int y1, paletteColor_t col);
void drawRectScaled(int x0, int y0, int x1, int y1, paletteColor_t col, int xTr, int yTr, int xScale, int yScale);
void drawEllipse(int xm, int ym, int a, int b, paletteColor_t col);
void drawEllipseScaled(int xm, int ym, int a, int b, paletteColor_t col, int xTr, int yTr, int xScale, int yScale);
void drawOptimizedEllipse(int xm, int ym, int a, int b, paletteColor_t col);
void drawCircle(int xm, int ym, int r, paletteColor_t col);
void drawCircleScaled(int xm, int ym, int r, paletteColor_t col, int xTr, int yTr, int xScale, int yScale);
void drawCircleQuadrants(int xm, int ym, int r, bool q1, bool q2, bool q3, bool q4, paletteColor_t col);
void drawCircleFilled(int xm, int ym, int r, paletteColor_t col);
void drawCircleFilledScaled(int xm, int ym, int r, paletteColor_t col, int xTr, int yTr, int xScale, int yScale);
void drawEllipseRect(int x0, int y0, int x1, int y1, paletteColor_t col);
void drawEllipseRectScaled(int x0, int y0, int x1, int y1, paletteColor_t col, int xTr, int yTr, int xScale,
                           int yScale);
void drawQuadBezierSeg(int x0, int y0, int x1, int y1, int x2, int y2, paletteColor_t col);
void drawQuadBezierSegScaled(int x0, int y0, int x1, int y1, int x2, int y2, paletteColor_t col, int xTr, int yTr,
                             int xScale, int yScale);
void drawQuadBezier(int x0, int y0, int x1, int y1, int x2, int y2, paletteColor_t col);
void drawQuadBezierScaled(int x0, int y0, int x1, int y1, int x2, int y2, paletteColor_t col, int xTr, int yTr,
                          int xScale, int yScale);
void drawQuadRationalBezierSeg(int x0, int y0, int x1, int y1, int x2, int y2, float w, paletteColor_t col);
void drawQuadRationalBezier(int x0, int y0, int x1, int y1, int x2, int y2, float w, paletteColor_t col);
void drawRotatedEllipse(int x, int y, int a, int b, float angle, paletteColor_t col);
void drawRotatedEllipseRect(int x0, int y0, int x1, int y1, long zd, paletteColor_t col);
void drawCubicBezierSeg(int x0, int y0, float x1, float y1, float x2, float y2, int x3, int y3, paletteColor_t col);
void drawCubicBezierSegScaled(int x0, int y0, float x1, float y1, float x2, float y2, int x3, int y3,
                              paletteColor_t col, int xTr, int yTr, int xScale, int yScale);
void drawCubicBezier(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, paletteColor_t col);
void drawCubicBezierScaled(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, paletteColor_t col, int xTr,
                           int yTr, int xScale, int yScale);
void drawQuadSpline(int n, int x[], int y[], paletteColor_t col);
void drawCubicSpline(int n, int x[], int y[], paletteColor_t col);

#endif /* SRC_BRESENHAM_H_ */
