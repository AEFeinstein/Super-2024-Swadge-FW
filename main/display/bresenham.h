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

void plotLineScaled(int x0, int y0, int x1, int y1, paletteColor_t col, int dashWidth, int xTr, int yTr, int xScale,
                    int yScale);
void plotLine(int x0, int y0, int x1, int y1, paletteColor_t col, int dashWidth);
void plotRect(int x0, int y0, int x1, int y1, paletteColor_t col);
void plotRectScaled(int x0, int y0, int x1, int y1, paletteColor_t col, int xTr, int yTr, int xScale, int yScale);
void plotEllipse(int xm, int ym, int a, int b, paletteColor_t col);
void plotEllipseScaled(int xm, int ym, int a, int b, paletteColor_t col, int xTr, int yTr, int xScale, int yScale);
void plotOptimizedEllipse(int xm, int ym, int a, int b, paletteColor_t col);
void plotCircle(int xm, int ym, int r, paletteColor_t col);
void plotCircleScaled(int xm, int ym, int r, paletteColor_t col, int xTr, int yTr, int xScale, int yScale);
void plotCircleQuadrants(int xm, int ym, int r, bool q1, bool q2, bool q3, bool q4, paletteColor_t col);
void plotCircleFilled(int xm, int ym, int r, paletteColor_t col);
void plotCircleFilledScaled(int xm, int ym, int r, paletteColor_t col, int xTr, int yTr, int xScale, int yScale);
void plotEllipseRect(int x0, int y0, int x1, int y1, paletteColor_t col);
void plotEllipseRectScaled(int x0, int y0, int x1, int y1, paletteColor_t col, int xTr, int yTr, int xScale,
                           int yScale);
void plotQuadBezierSeg(int x0, int y0, int x1, int y1, int x2, int y2, paletteColor_t col);
void plotQuadBezierSegScaled(int x0, int y0, int x1, int y1, int x2, int y2, paletteColor_t col, int xTr, int yTr,
                             int xScale, int yScale);
void plotQuadBezier(int x0, int y0, int x1, int y1, int x2, int y2, paletteColor_t col);
void plotQuadBezierScaled(int x0, int y0, int x1, int y1, int x2, int y2, paletteColor_t col, int xTr, int yTr,
                          int xScale, int yScale);
void plotQuadRationalBezierSeg(int x0, int y0, int x1, int y1, int x2, int y2, float w, paletteColor_t col);
void plotQuadRationalBezier(int x0, int y0, int x1, int y1, int x2, int y2, float w, paletteColor_t col);
void plotRotatedEllipse(int x, int y, int a, int b, float angle, paletteColor_t col);
void plotRotatedEllipseRect(int x0, int y0, int x1, int y1, long zd, paletteColor_t col);
void plotCubicBezierSeg(int x0, int y0, float x1, float y1, float x2, float y2, int x3, int y3, paletteColor_t col);
void plotCubicBezierSegScaled(int x0, int y0, float x1, float y1, float x2, float y2, int x3, int y3,
                              paletteColor_t col, int xTr, int yTr, int xScale, int yScale);
void plotCubicBezier(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, paletteColor_t col);
void plotCubicBezierScaled(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, paletteColor_t col, int xTr,
                           int yTr, int xScale, int yScale);
void plotQuadSpline(int n, int x[], int y[], paletteColor_t col);
void plotCubicSpline(int n, int x[], int y[], paletteColor_t col);

#endif /* SRC_BRESENHAM_H_ */
