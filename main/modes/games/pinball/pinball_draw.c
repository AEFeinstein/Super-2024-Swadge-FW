#include "hdw-tft.h"
#include "shapes.h"

#include "pinball_line.h"
#include "pinball_circle.h"
#include "pinball_rectangle.h"
#include "pinball_flipper.h"

/**
 * @brief TODO doc
 *
 * @param scene
 */
void jsSceneDraw(jsScene_t* scene)
{
    clearPxTft();

    // Lines
    for (int32_t i = 0; i < scene->numLines; i++)
    {
        pinballDrawLine(&scene->lines[i]);
    }

    // balls
    for (int32_t i = 0; i < scene->numBalls; i++)
    {
        vecFl_t* pos = &scene->balls[i].pos;
        drawCircleFilled(pos->x, pos->y, scene->balls[i].radius, c500);
    }

    // obstacles
    for (int32_t i = 0; i < scene->numObstacles; i++)
    {
        vecFl_t* pos = &scene->obstacles[i].pos;
        drawCircleFilled(pos->x, pos->y, scene->obstacles[i].radius, c131);
    }

    // flippers
    for (int32_t i = 0; i < scene->numFlippers; i++)
    {
        pinballDrawFlipper(&scene->flippers[i]);
    }

    // launchers
    for (int32_t i = 0; i < scene->numLaunchers; i++)
    {
        jsLauncher_t* l = &scene->launchers[i];
        drawRect(l->pos.x, l->pos.y, l->pos.x + l->width, l->pos.y + l->height, c330);
    }
}