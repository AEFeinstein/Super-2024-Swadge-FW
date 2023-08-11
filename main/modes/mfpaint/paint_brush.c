#include "paint_brush.h"

#include <stdlib.h>
#include <math.h>

#include "paint_common.h"
#include "paint_util.h"
#include "bresenham.h"


void paintDrawSquarePen(paintCanvas_t* canvas, point_t* points, uint8_t numPoints, uint16_t size, paletteColor_t col)
{
    plotRectFilledScaled(canvas->disp, points[0].x - (size / 2), points[0].y - (size / 2), points[0].x + ((size + 1) / 2), points[0].y + ((size + 1) / 2), col, canvas->x, canvas->y, canvas->xScale, canvas->yScale);
}

void paintDrawCirclePen(paintCanvas_t* canvas, point_t* points, uint8_t numPoints, uint16_t size, paletteColor_t col)
{
    plotCircleFilledScaled(canvas->disp, points[0].x, points[0].y, size, col, canvas->x, canvas->y, canvas->xScale, canvas->yScale);

    // fill out the circle if it's very small
    if (size == 1)
    {
        setPxScaled(canvas->disp, points[0].x, points[0].y - 1, col, canvas->x, canvas->y, canvas->xScale, canvas->yScale);
        setPxScaled(canvas->disp, points[0].x, points[0].y + 1, col, canvas->x, canvas->y, canvas->xScale, canvas->yScale);
    }
}

void paintDrawLine(paintCanvas_t* canvas, point_t* points, uint8_t numPoints, uint16_t size, paletteColor_t col)
{
    for (int16_t x = -size / 2; x < (size + 1) / 2; x++)
    {
        for (int16_t y = -size / 2; y < (size + 1) / 2; y++)
        {
            plotLineScaled(canvas->disp, points[0].x + x, points[0].y + y, points[1].x + x, points[1].y + y, col, 0, canvas->x, canvas->y, canvas->xScale, canvas->yScale);
        }
    }
}

void paintDrawCurve(paintCanvas_t* canvas, point_t* points, uint8_t numPoints, uint16_t size, paletteColor_t col)
{
    for (int16_t x = -size / 2; x < (size + 1) / 2; x++)
    {
        for (int16_t y = -size / 2; y < (size + 1) / 2; y++)
        {
            plotCubicBezierScaled(canvas->disp, points[0].x + x, points[0].y + y, points[1].x + x, points[1].y + y, points[2].x + x, points[2].y + y, points[3].x + x, points[3].y + y, col, canvas->x, canvas->y, canvas->xScale, canvas->yScale);
        }
    }
}

void paintDrawRectangle(paintCanvas_t* canvas, point_t* points, uint8_t numPoints, uint16_t size, paletteColor_t col)
{
    uint16_t x0 = (points[0].x > points[1].x) ? points[1].x : points[0].x;
    uint16_t y0 = (points[0].y > points[1].y) ? points[1].y : points[0].y;
    uint16_t x1 = (points[0].x > points[1].x) ? points[0].x : points[1].x;
    uint16_t y1 = (points[0].y > points[1].y) ? points[0].y : points[1].y;

    point_t tmpPoints[2];
    // x0, y0 -> x0, y1
    tmpPoints[0].x = x0;
    tmpPoints[0].y = y0;
    tmpPoints[1].x = x0;
    tmpPoints[1].y = y1;
    paintDrawLine(canvas, tmpPoints, 2, size, col);

    // x0, y0 -> x1, y0
    tmpPoints[1].x = x1;
    tmpPoints[1].y = y0;
    paintDrawLine(canvas, tmpPoints, 2, size, col);

    // x0, y1 -> x1, y1
    tmpPoints[0].y = y1;
    tmpPoints[1].y = y1;
    paintDrawLine(canvas, tmpPoints, 2, size, col);

    // x1, y0 -> x1, y1
    tmpPoints[0].x = x1;
    tmpPoints[0].y = y0;
    paintDrawLine(canvas, tmpPoints, 2, size, col);
}

void paintDrawFilledRectangle(paintCanvas_t* canvas, point_t* points, uint8_t numPoints, uint16_t size, paletteColor_t col)
{
    uint16_t x0 = (points[0].x > points[1].x) ? points[1].x : points[0].x;
    uint16_t y0 = (points[0].y > points[1].y) ? points[1].y : points[0].y;
    uint16_t x1 = (points[0].x > points[1].x) ? points[0].x : points[1].x;
    uint16_t y1 = (points[0].y > points[1].y) ? points[0].y : points[1].y;

    // This function takes care of its own scaling because it's very easy and will save a lot of unnecessary draws
    plotRectFilledScaled(canvas->disp, x0, y0, x1 + 1, y1 + 1, col, canvas->x, canvas->y, canvas->xScale, canvas->yScale);
}

void paintDrawCircle(paintCanvas_t* canvas, point_t* points, uint8_t numPoints, uint16_t size, paletteColor_t col)
{
    uint16_t dX = abs(points[0].x - points[1].x);
    uint16_t dY = abs(points[0].y - points[1].y);
    uint16_t r = (uint16_t)(sqrt(dX*dX+dY*dY) + 0.5);

    for (int16_t x = -size / 2; x < (size + 1) / 2; x++)
    {
        for (int16_t y = -size / 2; y < (size + 1) / 2; y++)
        {
            plotCircleScaled(canvas->disp, points[0].x + x, points[0].y + y, r, col, canvas->x, canvas->y, canvas->xScale, canvas->yScale);
        }
    }
}

void paintDrawFilledCircle(paintCanvas_t* canvas, point_t* points, uint8_t numPoints, uint16_t size, paletteColor_t col)
{
    uint16_t dX = abs(points[0].x - points[1].x);
    uint16_t dY = abs(points[0].y - points[1].y);
    uint16_t r = (uint16_t)(sqrt(dX*dX+dY*dY) + 0.5);

    plotCircleFilledScaled(canvas->disp, points[0].x, points[0].y, r, col, canvas->x, canvas->y, canvas->xScale, canvas->yScale);
}

void paintDrawEllipse(paintCanvas_t* canvas, point_t* points, uint8_t numPoints, uint16_t size, paletteColor_t col)
{
    uint16_t x0 = (points[0].x > points[1].x) ? points[1].x : points[0].x;
    uint16_t y0 = (points[0].y > points[1].y) ? points[1].y : points[0].y;
    uint16_t x1 = (points[0].x > points[1].x) ? points[0].x : points[1].x;
    uint16_t y1 = (points[0].y > points[1].y) ? points[0].y : points[1].y;

    pxStack_t tmpPxs;
    // TODO maybe we shouldn't use a heap-allocated pxStack here
    initPxStack(&tmpPxs);

    for (int16_t x = 0; x < size; x++)
    {
        for (int16_t y = 0; y < size; y++)
        {
            if (x * 2 >= (x1 - x0 + 1) || y * 2 >= (y1 - y0 + 1))
            {
                // don't draw a ridiculously large ellipse
                continue;
            }
            // for some reason, plotting an ellipse also plots 2 extra points outside of the ellipse
            // let's just work around that
            pushPxScaled(&tmpPxs, canvas->disp, x0 + x + ((x1 - x) - (x0 + x) + 1) / 2, y0 - 2 + y, canvas->x, canvas->y, canvas->xScale, canvas->yScale);
            pushPxScaled(&tmpPxs, canvas->disp, x0 + x + ((x1 - x) - (x0 + x) + 1) / 2, y1 + 2 - y, canvas->x, canvas->y, canvas->xScale, canvas->yScale);

            plotEllipseRectScaled(canvas->disp, x0 + x, y0 + y, x1 - x, y1 - y, col, canvas->x, canvas->y, canvas->xScale, canvas->yScale);

            while (popPxScaled(&tmpPxs, canvas->disp, canvas->xScale, canvas->yScale));
        }
    }
    freePxStack(&tmpPxs);
}

void paintDrawPolygon(paintCanvas_t* canvas, point_t* points, uint8_t numPoints, uint16_t size, paletteColor_t col)
{
    for (uint8_t i = 0; i < numPoints - 1; i++)
    {
        paintDrawLine(canvas, points + i, 2, size, col);
    }
}

void paintDrawSquareWave(paintCanvas_t* canvas, point_t* points, uint8_t numPoints, uint16_t size, paletteColor_t col)
{
    paintPlotSquareWave(canvas->disp, points[0].x, points[0].y, points[1].x, points[1].y, size, col, canvas->x, canvas->y, canvas->xScale, canvas->yScale);
}

void paintDrawPaintBucket(paintCanvas_t* canvas, point_t* points, uint8_t numPoints, uint16_t size, paletteColor_t col)
{
    floodFill(canvas->disp, canvas->x + canvas->xScale * points[0].x, canvas->y + canvas->yScale * points[0].y, col, canvas->x, canvas->y, canvas->x + canvas->xScale * canvas->w, canvas->y + canvas->yScale * canvas->h);
}
