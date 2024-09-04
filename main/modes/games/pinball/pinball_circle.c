#include "pinball_circle.h"

/**
 * @brief TODO doc
 *
 * @param tableData
 * @param scene
 * @return uint32_t
 */
uint32_t readCircleFromFile(uint8_t* tableData, jsScene_t* scene)
{
    jsCircle_t* circle = &scene->circles[scene->numCircles++];

    uint32_t dIdx   = 0;
    circle->id      = readInt16(tableData, &dIdx);
    circle->groupId = readInt8(tableData, &dIdx);
    circle->group   = addToGroup(scene, circle, circle->groupId);
    circle->pos.x   = readInt16(tableData, &dIdx);
    circle->pos.y   = readInt16(tableData, &dIdx);
    circle->radius  = readInt8(tableData, &dIdx);
    circle->type    = readInt8(tableData, &dIdx);
    circle->pushVel = readInt8(tableData, &dIdx);

    return dIdx;
}

/**
 * @brief Simulate a ball's motion
 *
 * @param ball
 * @param dt
 * @param gravity
 */
void jsBallSimulate(jsBall_t* ball, float dt, vecFl_t gravity)
{
    ball->vel = addVecFl2d(ball->vel, mulVecFl2d(gravity, dt));
    ball->pos = addVecFl2d(ball->pos, mulVecFl2d(ball->vel, dt));
}

/**
 * @brief TODO
 *
 * @param circle
 * @param elapsedUs
 */
void jsCircleTimer(jsCircle_t* circle, int32_t elapsedUs)
{
    if (circle->litTimer > 0)
    {
        circle->litTimer -= elapsedUs;
    }
}
