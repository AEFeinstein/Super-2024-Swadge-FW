#include "geometryFl.h"

#include "pinball_rectangle.h"

/**
 * @brief TODO doc
 *
 * @param tableData
 * @param scene
 * @return uint32_t
 */
uint32_t readRectangleFromFile(uint8_t* tableData, jsScene_t* scene)
{
    uint32_t dIdx          = 0;
    jsLauncher_t* launcher = &scene->launchers[scene->numLaunchers++];
    int16_t id             = readInt16(tableData, &dIdx);
    int8_t gIdx            = readInt8(tableData, &dIdx);
    launcher->pos.x        = readInt16(tableData, &dIdx);
    launcher->pos.y        = readInt16(tableData, &dIdx);
    launcher->width        = readInt16(tableData, &dIdx);
    launcher->height       = readInt16(tableData, &dIdx);
    launcher->buttonHeld   = false;
    launcher->impulse      = 0;
    return dIdx;
}

/**
 * @brief TODO doc
 *
 * @param launcher
 * @param balls
 * @param numBalls
 * @param dt
 */
void jsLauncherSimulate(jsLauncher_t* launcher, jsBall_t* balls, int32_t numBalls, float dt)
{
    if (launcher->buttonHeld)
    {
        launcher->impulse += (200.0f * dt);
    }
    else
    {
        rectangleFl_t r = {.pos = launcher->pos, .width = launcher->width, .height = launcher->height};
        // If touching a ball, transfer to a ball
        for (int32_t bIdx = 0; bIdx < numBalls; bIdx++)
        {
            circleFl_t b = {.pos = balls[bIdx].pos, .radius = balls[bIdx].radius};
            if (circleRectFlIntersection(b, r, NULL))
            {
                balls[bIdx].vel.y -= launcher->impulse;
            }
        }
        launcher->impulse = 0;
    }
}