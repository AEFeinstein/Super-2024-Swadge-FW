#include "pinball_circle.h"
#include "pinball_rectangle.h"

/**
 * @brief TODO doc
 *
 * @param tableData
 * @param scene
 * @return uint32_t
 */
uint32_t readCircleFromFile(uint8_t* tableData, pbScene_t* scene)
{
    pbCircle_t* circle = &scene->circles[scene->numCircles++];

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
 * @param scene
 */
void pbBallSimulate(pbBall_t* ball, int32_t elapsedUs, float dt, pbScene_t* scene)
{
    if (ball->scoopTimer <= 0)
    {
        ball->vel = addVecFl2d(ball->vel, mulVecFl2d(scene->gravity, dt));
        ball->pos = addVecFl2d(ball->pos, mulVecFl2d(ball->vel, dt));
    }
    else
    {
        ball->scoopTimer -= elapsedUs;

        if (ball->scoopTimer <= 0)
        {
            // Respawn in the launch tube
            for (int32_t pIdx = 0; pIdx < scene->numPoints; pIdx++)
            {
                pbPoint_t* point = &scene->points[pIdx];
                if (PB_BALL_SPAWN == point->type)
                {
                    ball->pos = point->pos;
                    break;
                }
            }

            pbOpenLaunchTube(scene, true);

            // Give the ball initial velocity
            ball->vel.x = 0;
            ball->vel.y = MAX_LAUNCHER_VELOCITY;
        }
    }
}

/**
 * @brief TODO
 *
 * @param circle
 * @param elapsedUs
 */
void pbCircleTimer(pbCircle_t* circle, int32_t elapsedUs)
{
    if (circle->litTimer > 0)
    {
        circle->litTimer -= elapsedUs;
    }
}
