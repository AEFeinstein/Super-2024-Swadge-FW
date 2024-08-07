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
    jsObstacle_t* obstacle = &scene->obstacles[scene->numObstacles++];

    uint32_t dIdx = 0;
    uint16_t id   = readInt16(tableData, &dIdx);
    uint8_t gIdx  = readInt8(tableData, &dIdx);
    vecFl_t pos;
    obstacle->pos.x   = readInt16(tableData, &dIdx);
    obstacle->pos.y   = readInt16(tableData, &dIdx);
    obstacle->radius  = readInt8(tableData, &dIdx);
    obstacle->pushVel = readInt8(tableData, &dIdx);

    // ball->radius      = radius;
    // ball->mass        = mass;
    // ball->pos         = pos;
    // ball->vel         = vel;
    // ball->restitution = restitution;

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
