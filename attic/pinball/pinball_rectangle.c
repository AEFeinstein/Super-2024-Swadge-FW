#include <stdio.h>
#include "geometryFl.h"

#include "pinball_rectangle.h"

/**
 * @brief TODO doc
 *
 * @param tableData
 * @param scene
 * @return uint32_t
 */
uint32_t readRectangleFromFile(uint8_t* tableData, pbScene_t* scene)
{
    uint32_t dIdx          = 0;
    pbLauncher_t* launcher = &scene->launchers[scene->numLaunchers++];
    launcher->id           = readInt16(tableData, &dIdx);
    launcher->groupId      = readInt8(tableData, &dIdx);
    launcher->group        = addToGroup(scene, launcher, launcher->groupId);
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
 * @param dt
 */
void pbLauncherSimulate(pbLauncher_t* launcher, list_t* balls, float dt)
{
    if (launcher->buttonHeld)
    {
        launcher->impulse += (dt / 3);
        if (launcher->impulse > 0.99f)
        {
            launcher->impulse = 0.99f;
        }
    }
    else if (launcher->impulse)
    {
        rectangleFl_t r = {.pos = launcher->pos, .width = launcher->width, .height = launcher->height};
        // If touching a ball, transfer to a ball
        node_t* bNode = balls->first;
        while (bNode)
        {
            pbBall_t* ball = bNode->val;
            circleFl_t b   = {.pos = ball->pos, .radius = ball->radius};
            if (circleRectFlIntersection(b, r, NULL))
            {
                ball->vel.y = (MAX_LAUNCHER_VELOCITY * launcher->impulse);
            }

            bNode = bNode->next;
        }

        launcher->impulse = 0;
    }
}