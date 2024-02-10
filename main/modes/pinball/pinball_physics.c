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
void checkBallsNotTouching(pinball_t* p);
void setBallTouching(pbTouchRef_t* ballTouching, void* obj, pbShapeType_t type);
pbShapeType_t ballIsTouching(pbTouchRef_t* ballTouching, void* obj);

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
    if (1 < p->numBalls)
    {
        // Check for ball-ball collisions
        checkBallBallCollisions(p);
    }

    // Check for collisions between balls and static objects
    checkBallStaticCollision(p);

    // Move balls along new vectors
    moveBalls(p);

    // Clear references to balls touching things after moving
    checkBallsNotTouching(p);
}

/**
 * @brief TODO
 *
 * @param p
 */
void calculateBallZones(pinball_t* p)
{
    // For each ball, find which zones it is in
    for (uint32_t bIdx = 0; bIdx < p->numBalls; bIdx++)
    {
        // Figure out which zones the ball is in
        p->balls[bIdx].zoneMask = pinZoneCircle(p, p->balls[bIdx]);
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
    for (uint32_t bIdx = 0; bIdx < p->numBalls; bIdx++)
    {
        pbCircle_t* ball = &p->balls[bIdx];
        for (uint32_t obIdx = bIdx + 1; obIdx < p->numBalls; obIdx++)
        {
            pbCircle_t* otherBall = &p->balls[obIdx];
            // Check for a new collision
            if ((ball->zoneMask & otherBall->zoneMask)                                // In the same zone
                && PIN_NO_SHAPE == ballIsTouching(p->ballsTouching[bIdx], otherBall)  // and not already touching
                && circleCircleIntersection(intCircle(*ball), intCircle(*otherBall))) // and intersecting
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
                setBallTouching(p->ballsTouching[bIdx], otherBall, PIN_CIRCLE);
                setBallTouching(p->ballsTouching[obIdx], ball, PIN_CIRCLE);
            }
        }
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
    for (uint32_t bIdx = 0; bIdx < p->numBalls; bIdx++)
    {
        // Reference and integer representation
        pbCircle_t* ball = &p->balls[bIdx];
        circle_t intBall = intCircle(*ball);

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
        for (uint32_t wIdx = 0; wIdx < p->numWalls; wIdx++)
        {
            pbLine_t* wall = &p->walls[wIdx];
            vec_t collisionVec;
            // Check for a collision
            if ((ball->zoneMask & wall->zoneMask)                                  // In the same zone
                && PIN_NO_SHAPE == ballIsTouching(p->ballsTouching[bIdx], wall)    // and not already touching
                && circleLineIntersection(intBall, intLine(*wall), &collisionVec)) // and intersecting
            {
                // Collision detected, do some physics
                vec_q24_8 centerToCenter = {
                    .x = collisionVec.x,
                    .y = collisionVec.y,
                };
                vec_q24_8 reflVec = fpvNorm(centerToCenter);
                ball->vel         = fpvSub(ball->vel, fpvMulSc(reflVec, (2 * fpvDot(ball->vel, reflVec))));

                // Mark this wall as being touched
                setBallTouching(p->ballsTouching[bIdx], wall, PIN_LINE);
            }
        }
    }
}

/**
 * @brief TODO
 *
 * @param p
 */
void moveBalls(pinball_t* p)
{
    // For each ball, check collisions with static objects
    for (uint32_t bIdx = 0; bIdx < p->numBalls; bIdx++)
    {
        pbCircle_t* ball = &p->balls[bIdx];

        // Move the ball
        ball->pos.x += (ball->vel.x);
        ball->pos.y += (ball->vel.y);
    }
}

/**
 * @brief TODO
 *
 * @param p
 */
void checkBallsNotTouching(pinball_t* p)
{
    // For each ball
    for (uint32_t bIdx = 0; bIdx < p->numBalls; bIdx++)
    {
        pbCircle_t* ball = &p->balls[bIdx];
        circle_t iBall   = intCircle(*ball);
        // For each thing it could be touching
        for (uint32_t tIdx = 0; tIdx < MAX_TOUCHES; tIdx++)
        {
            pbTouchRef_t* tr = &p->ballsTouching[bIdx][tIdx];
            // If it's touching a thing
            if (NULL != tr->obj)
            {
                switch (tr->type)
                {
                    case PIN_CIRCLE:
                    {
                        pbCircle_t* other = (pbCircle_t*)tr->obj;
                        if ((0 == (ball->zoneMask & other->zoneMask))               // Not in the same zone
                            || !circleCircleIntersection(iBall, intCircle(*other))) // or not touching
                        {
                            // Clear the reference
                            tr->obj  = NULL;
                            tr->type = PIN_NO_SHAPE;
                        }
                        break;
                    }
                    case PIN_LINE:
                    {
                        pbLine_t* other = (pbLine_t*)tr->obj;
                        vec_t collisionVec;
                        if ((0 == (ball->zoneMask & other->zoneMask))                          // Not in the same zone
                            || !circleLineIntersection(iBall, intLine(*other), &collisionVec)) // or not touching
                        {
                            // Clear the reference
                            tr->obj  = NULL;
                            tr->type = PIN_NO_SHAPE;
                        }
                        break;
                    }
                    case PIN_RECT:
                    {
                        pbRect_t* other = (pbRect_t*)tr->obj;
                        if ((0 == (ball->zoneMask & other->zoneMask))           // Not in the same zone
                            || !circleRectIntersection(iBall, intRect(*other))) // or not touching
                        {
                            // Clear the reference
                            tr->obj  = NULL;
                            tr->type = PIN_NO_SHAPE;
                        }
                        break;
                    }
                    default:
                    case PIN_NO_SHAPE:
                    {
                        // Not touching anything...
                        break;
                    }
                }
            }
        }
    }
}

/**
 * @brief TODO
 *
 * @param ballTouching
 * @param obj
 * @param type
 */
void setBallTouching(pbTouchRef_t* ballTouching, void* obj, pbShapeType_t type)
{
    for (uint32_t i = 0; i < MAX_TOUCHES; i++)
    {
        if (NULL == ballTouching->obj)
        {
            ballTouching->obj  = obj;
            ballTouching->type = type;
            return;
        }
    }
}

/**
 * @brief TODO
 *
 * @param ballTouching
 * @param obj
 * @return pbShapeType_t
 */
pbShapeType_t ballIsTouching(pbTouchRef_t* ballTouching, void* obj)
{
    for (uint32_t i = 0; i < MAX_TOUCHES; i++)
    {
        if (ballTouching->obj == obj)
        {
            return ballTouching->type;
        }
    }
    return PIN_NO_SHAPE;
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
