#include "hdw-tft.h"
#include "shapes.h"

#include "pinball_draw.h"
#include "pinball_line.h"
#include "pinball_circle.h"
#include "pinball_rectangle.h"
#include "pinball_flipper.h"

/**
 * @brief TODO doc
 *
 * @param scene
 */
void jsAdjustCamera(jsScene_t* scene)
{
    // Find the ball lowest on the table
    float lowestBallX = 0;
    float lowestBallY = 0;
    for (int32_t bIdx = 0; bIdx < scene->numBalls; bIdx++)
    {
        if (scene->balls[bIdx].pos.y > lowestBallY)
        {
            lowestBallX = scene->balls[bIdx].pos.x;
            lowestBallY = scene->balls[bIdx].pos.y;
        }
    }

    // Adjust the lowest ball's position to screen coordinates
    lowestBallY -= scene->cameraOffset.y;

#define PIN_CAMERA_BOUND_UPPER ((TFT_HEIGHT) / 4)
#define PIN_CAMERA_BOUND_LOWER ((3 * TFT_HEIGHT) / 4)

    // If the lowest ball is lower than the boundary
    if (lowestBallY > PIN_CAMERA_BOUND_LOWER)
    {
        // Pan the camera down
        if (scene->cameraOffset.y < scene->tableDim.y - TFT_HEIGHT)
        {
            scene->cameraOffset.y += (lowestBallY - PIN_CAMERA_BOUND_LOWER);
        }
    }
    // If the lowest ball is higher than the other boundary
    else if (lowestBallY < PIN_CAMERA_BOUND_UPPER)
    {
        // Pan the camera up
        if (scene->cameraOffset.y > 0)
        {
            scene->cameraOffset.y -= (PIN_CAMERA_BOUND_UPPER - lowestBallY);
        }
    }

    // Pan in the X direction to view the launch tube
    int16_t xEnd = lowestBallX + PINBALL_RADIUS + 40;
    if (xEnd > TFT_WIDTH)
    {
        scene->cameraOffset.x = xEnd - TFT_WIDTH;
    }
    else
    {
        scene->cameraOffset.x = 0;
    }
}

/**
 * @brief TODO doc
 *
 * @param scene
 */
void jsSceneDraw(jsScene_t* scene)
{
    clearPxTft();

    // Triangle indicators
    for (int32_t i = 0; i < scene->numTriangles; i++)
    {
        jsTriangle_t* tri = &scene->triangles[i];
        drawTriangleOutlined(tri->p1.x - scene->cameraOffset.x, tri->p1.y - scene->cameraOffset.y, //
                             tri->p2.x - scene->cameraOffset.x, tri->p2.y - scene->cameraOffset.y, //
                             tri->p3.x - scene->cameraOffset.x, tri->p3.y - scene->cameraOffset.y, //
                             tri->isOn ? c550 : cTransparent, c220);
    }

    // Lines
    for (int32_t i = 0; i < scene->numLines; i++)
    {
        pinballDrawLine(&scene->lines[i], &scene->cameraOffset);
    }

    // balls
    for (int32_t i = 0; i < scene->numBalls; i++)
    {
        vecFl_t* pos = &scene->balls[i].pos;
        drawCircleFilled(pos->x - scene->cameraOffset.x, pos->y - scene->cameraOffset.y, scene->balls[i].radius, c500);
    }

    // circles
    for (int32_t i = 0; i < scene->numCircles; i++)
    {
        if (JS_BUMPER == scene->circles[i].type)
        {
            vecFl_t* pos = &scene->circles[i].pos;
            drawCircleFilled(pos->x - scene->cameraOffset.x, pos->y - scene->cameraOffset.y, scene->circles[i].radius,
                             c131);
        }
    }

    // flippers
    for (int32_t i = 0; i < scene->numFlippers; i++)
    {
        pinballDrawFlipper(&scene->flippers[i], &scene->cameraOffset);
    }

    // launchers
    for (int32_t i = 0; i < scene->numLaunchers; i++)
    {
        jsLauncher_t* l = &scene->launchers[i];
        int compression = l->height * l->impulse;
        vec_t offsetPos = {
            .x = l->pos.x - scene->cameraOffset.x,
            .y = l->pos.y + compression - scene->cameraOffset.y,
        };
        drawRect(offsetPos.x, offsetPos.y, offsetPos.x + l->width, offsetPos.y + l->height - compression, c330);
    }
}
