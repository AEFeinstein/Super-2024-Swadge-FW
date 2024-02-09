//==============================================================================
// Includes
//==============================================================================

#include "pinball_physics.h"
#include "pinball_zones.h"

//==============================================================================
// Function Declarations
//==============================================================================

void calculateBallZones(pinball_t* p);
void checkBallBallCollisions(pinball_t* p);
void checkBallStaticCollision(pinball_t* p);
void moveBalls(pinball_t* p);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief TODO
 *
 * @param p
 */
void updatePinballPhysicsFrame(pinball_t* p)
{
    // Recalculate which zone each ball is in
    calculateBallZones(p);

    // If there are multiple balls
    if (1 < p->balls.length)
    {
        // Check for ball-ball collisions
        checkBallBallCollisions(p);
    }

    // Check for collisions between balls and static objects
    checkBallStaticCollision(p);

    // Move balls along new vectors
    moveBalls(p);
}

/**
 * @brief TODO
 *
 * @param p
 */
void calculateBallZones(pinball_t* p)
{
    // For each ball, find which zones it is in
    node_t* ballNode = p->balls.first;
    while (ballNode != NULL)
    {
        pbCircle_t* ball = (pbCircle_t*)ballNode->val;

        // Figure out which zones the ball is in
        ball->zoneMask = pinZoneCircle(p, *ball);
        // Iterate
        ballNode = ballNode->next;
    }
}

/**
 * @brief TODO
 *
 * @param p
 */
void checkBallBallCollisions(pinball_t* p)
{
    // For each ball, check collisions with other balls
    node_t* ballNode = p->balls.first;
    while (ballNode != NULL)
    {
        pbCircle_t* ball      = ballNode->val;
        node_t* otherBallNode = ballNode->next;
        while (otherBallNode != NULL)
        {
            pbCircle_t* otherBall = otherBallNode->val;
            // Balls are touching each other
            if ((NULL != ball->touching) && (ball->touching == otherBall->touching))
            {
                // check if they're not touching anymore
                if ((0 == (ball->zoneMask & otherBall->zoneMask))
                    || !circleCircleIntersection(intCircle(*ball), intCircle(*otherBall)))
                {
                    // Not in the same zone or not touching, so set to NULL
                    ball->touching      = NULL;
                    otherBall->touching = NULL;
                }
            }
            else if ((ball->zoneMask & otherBall->zoneMask)
                     && circleCircleIntersection(intCircle(*ball), intCircle(*otherBall)))
            {
                // Math for the first ball
                vec_q24_8 v1         = ball->vel;
                vec_q24_8 x1         = ball->pos;
                vec_q24_8 v2         = otherBall->vel;
                vec_q24_8 x2         = otherBall->pos;
                vec_q24_8 x1_x2      = fpvSub(x1, x2);
                vec_q24_8 v1_v2      = fpvSub(v1, v2);
                q24_8 xSqMag         = fpvSqMag(x1_x2);
                vec_q24_8 ballNewVel = ball->vel;
                if (xSqMag > 0)
                {
                    ballNewVel = fpvSub(v1, fpvMulSc(x1_x2, DIV_FX(fpvDot(v1_v2, x1_x2), xSqMag)));
                }

                // Flip everything for the other ball
                v1     = otherBall->vel;
                x1     = otherBall->pos;
                v2     = ball->vel;
                x2     = ball->pos;
                x1_x2  = fpvSub(x1, x2);
                v1_v2  = fpvSub(v1, v2);
                xSqMag = fpvSqMag(x1_x2);
                if (xSqMag > 0)
                {
                    otherBall->vel = fpvSub(v1, fpvMulSc(x1_x2, DIV_FX(fpvDot(v1_v2, x1_x2), fpvSqMag(x1_x2))));
                }

                // Set the new velocity for the first ball after finding the second's
                ball->vel = ballNewVel;

                // The balls are touching each other
                ball->touching      = otherBall;
                otherBall->touching = ball;
            }
            // Iterate
            otherBallNode = otherBallNode->next;
        }

        // Iterate
        ballNode = ballNode->next;
    }
}

/**
 * @brief TODO
 *
 * @param p
 */
void checkBallStaticCollision(pinball_t* p)
{
    // For each ball, check collisions with static objects
    node_t* ballNode = p->balls.first;
    while (ballNode != NULL)
    {
        pbCircle_t* ball = (pbCircle_t*)ballNode->val;
        // if (ballZoneMask & p->bumper.zoneMask)
        // {
        //     // Check for collision
        //     if (circleCircleIntersection(intCircle(*ball), intCircle(p->bumper)))
        //     {
        //         // Reflect the velocity vector along the normal between the two radii
        //         // See http://www.sunshine2k.de/articles/coding/vectorreflection/vectorreflection.html
        //         vec_q24_8 centerToCenter = {
        //             .x = ball->pos.x - p->bumper.pos.x,
        //             .y = ball->pos.y - p->bumper.pos.y,
        //         };
        //         vec_q24_8 reflVec = fpvNorm(centerToCenter);
        //         ball->vel         = fpvSub(ball->vel, fpvMulSc(reflVec, (2 * fpvDot(ball->vel, reflVec))));
        //     }
        // }

        // Iterate over all walls
        node_t* wallNode = p->walls.first;
        while (wallNode != NULL)
        {
            // Check collision
            pbLine_t* wall = (pbLine_t*)wallNode->val;

            // If the ball is already touching this wall
            if (ball->touching == wall)
            {
                // Check to see if it's not touching anymore
                vec_t collisionVec;
                if (0 == (ball->zoneMask & wall->zoneMask))
                {
                    ball->touching = NULL;
                }
                else if (!circleLineIntersection(intCircle(*ball), intLine(*wall), &collisionVec))
                {
                    ball->touching = NULL;
                }
            }
            else // Ball is not touching this wall already
            {
                // Quick zone check
                if (ball->zoneMask & wall->zoneMask)
                {
                    // In the same zone, do a slower intersection check
                    vec_t collisionVec;
                    if (circleLineIntersection(intCircle(*ball), intLine(*wall), &collisionVec))
                    {
                        // Collision detected, do some physics
                        vec_q24_8 centerToCenter = {
                            .x = collisionVec.x,
                            .y = collisionVec.y,
                        };
                        vec_q24_8 reflVec = fpvNorm(centerToCenter);
                        ball->vel         = fpvSub(ball->vel, fpvMulSc(reflVec, (2 * fpvDot(ball->vel, reflVec))));

                        // Mark this wall as being touched
                        ball->touching = wall;

                        // Stop iterating
                        wallNode = NULL;
                    }
                }
            }

            // Iterate
            if (NULL != wallNode)
            {
                wallNode = wallNode->next;
            }
        }

        // Iterate
        ballNode = ballNode->next;
    }
}

/**
 * @brief TODO
 *
 * @param p
 */
void moveBalls(pinball_t* p)
{
    // For each ball
    node_t* ballNode = p->balls.first;
    while (ballNode != NULL)
    {
        pbCircle_t* ball = (pbCircle_t*)ballNode->val;

        // Move the ball
        ball->pos.x += (ball->vel.x);
        ball->pos.y += (ball->vel.y);

        // Iterate
        ballNode = ballNode->next;
    }
}

/**
 * @brief Helper function to 'cast' a pinball circle (q24_8 representation) to an integer circle
 *
 * @param pbc The circle to cast
 * @return The integer representation of the circle
 */
circle_t intCircle(pbCircle_t pbc)
{
    circle_t nc = {
        .radius = FROM_FX(pbc.radius),
        .pos.x  = FROM_FX(pbc.pos.x),
        .pos.y  = FROM_FX(pbc.pos.y),
    };
    return nc;
}

/**
 * @brief Helper function to 'cast' a pinball line (q24_8 representation) to an integer line
 *
 * @param pbl The line to cast
 * @return The integer representation of the line
 */
line_t intLine(pbLine_t pbl)
{
    line_t nl = {
        .p1.x = FROM_FX(pbl.p1.x),
        .p1.y = FROM_FX(pbl.p1.y),
        .p2.x = FROM_FX(pbl.p2.x),
        .p2.y = FROM_FX(pbl.p2.y),
    };
    return nl;
}

/**
 * @brief Helper function to 'cast' a pinball rectangle (q24_8 representation) to an integer rectangle
 *
 * @param pbr The rectangle to cast
 * @return The integer representation of the rectangle
 */

rectangle_t intRect(pbRect_t pbr)
{
    rectangle_t nr = {
        .pos.x  = FROM_FX(pbr.pos.x),
        .pos.y  = FROM_FX(pbr.pos.y),
        .width  = FROM_FX(pbr.width),
        .height = FROM_FX(pbr.height),
    };
    return nr;
}
