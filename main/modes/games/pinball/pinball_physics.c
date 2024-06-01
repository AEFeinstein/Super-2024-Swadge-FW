//==============================================================================
// Includes
//==============================================================================

#include <math.h>
#include "pinball_physics.h"
#include "pinball_zones.h"

#define EPSILON 0.001f

//==============================================================================
// Function Declarations
//==============================================================================

void calculateBallZones(pinball_t* p);
void checkBallBallCollisions(pinball_t* p);
void checkBallStaticCollision(pinball_t* p);
void checkBallFlipperCollision(pinball_t* p);
void moveBalls(pinball_t* p);
void moveFlippers(pinball_t* p);
void checkBallsNotTouching(pinball_t* p);
void setBallTouching(pbTouchRef_t* ballTouching, const void* obj, pbShapeType_t type);
pbShapeType_t ballIsTouching(pbTouchRef_t* ballTouching, const void* obj);
void checkBallsAtRest(pinball_t* p);

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

    // Check for collisions between balls and moving objects (flippers)
    checkBallFlipperCollision(p);

    // Check if balls are actually at rest
    checkBallsAtRest(p);

    // Move balls along new vectors
    moveBalls(p);

    // Move flippers rotationally
    moveFlippers(p);

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
            vecFl_t centerToCenter;
            // Check for a new collision
            if ((ball->zoneMask & otherBall->zoneMask)                                 // In the same zone
                && circleCircleFlIntersection(ball->c, otherBall->c, &centerToCenter)) // and intersecting
            {
                // Move balls backwards equally from the midpoint to not clip
                float halfDistM     = (ball->c.radius + otherBall->c.radius - EPSILON) / 2.0f;
                vecFl_t midwayPoint = divVecFl2d(addVecFl2d(ball->c.pos, otherBall->c.pos), 2.0f);
                vecFl_t vecFromMid  = mulVecFl2d(normVecFl2d(centerToCenter), halfDistM);

                // Move both balls
                ball->c.pos      = addVecFl2d(midwayPoint, vecFromMid);
                otherBall->c.pos = subVecFl2d(midwayPoint, vecFromMid);

                // If the balls aren't touching yet, adjust velocities (bounce)
                if (PIN_NO_SHAPE == ballIsTouching(p->ballsTouching[bIdx], otherBall))
                {
                    // Math for the first ball
                    vecFl_t v1         = ball->vel;
                    vecFl_t x1         = ball->c.pos;
                    vecFl_t v2         = otherBall->vel;
                    vecFl_t x2         = otherBall->c.pos;
                    vecFl_t x1_x2      = subVecFl2d(x1, x2);
                    vecFl_t v1_v2      = subVecFl2d(v1, v2);
                    float xSqMag       = sqMagVecFl2d(x1_x2);
                    vecFl_t ballNewVel = ball->vel;
                    if (xSqMag > 0)
                    {
                        ballNewVel = subVecFl2d(v1, mulVecFl2d(x1_x2, (dotVecFl2d(v1_v2, x1_x2) / xSqMag)));
                    }

                    // Flip everything for the other ball
                    v1     = otherBall->vel;
                    x1     = otherBall->c.pos;
                    v2     = ball->vel;
                    x2     = ball->c.pos;
                    x1_x2  = subVecFl2d(x1, x2);
                    v1_v2  = subVecFl2d(v1, v2);
                    xSqMag = sqMagVecFl2d(x1_x2);
                    if (xSqMag > 0)
                    {
                        otherBall->vel
                            = subVecFl2d(v1, mulVecFl2d(x1_x2, (dotVecFl2d(v1_v2, x1_x2) / sqMagVecFl2d(x1_x2))));
                    }

                    // Set the new velocity for the first ball after finding the second's
                    ball->vel = ballNewVel;

                    // The balls are touching each other
                    setBallTouching(p->ballsTouching[bIdx], otherBall, PIN_CIRCLE);
                    setBallTouching(p->ballsTouching[obIdx], ball, PIN_CIRCLE);

                    // Mark both balls as bounced
                    ball->bounce      = true;
                    otherBall->bounce = true;
                }
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

        // Iterate over all bumpers
        for (uint32_t uIdx = 0; uIdx < p->numBumpers; uIdx++)
        {
            pbCircle_t* bumper = &p->bumpers[uIdx];
            vecFl_t collisionVec;

            // Check for a collision
            if ((ball->zoneMask & bumper->zoneMask)                               // In the same zone
                && circleCircleFlIntersection(ball->c, bumper->c, &collisionVec)) // and intersecting
            {
                // Find the normalized vector along the collision normal
                vecFl_t centerToCenter = {
                    .x = collisionVec.x,
                    .y = collisionVec.y,
                };
                vecFl_t reflVec = normVecFl2d(centerToCenter);

                // If the ball isn't already touching the bumper
                if (PIN_NO_SHAPE == ballIsTouching(p->ballsTouching[bIdx], bumper))
                {
                    // Bounced on a bumper
                    ball->bounce = true;
                    // Reflect the velocity vector along the normal between the two radii
                    // See http://www.sunshine2k.de/articles/coding/vectorreflection/vectorreflection.html
                    ball->vel = subVecFl2d(ball->vel, mulVecFl2d(reflVec, (2 * dotVecFl2d(ball->vel, reflVec))));
                    // Lose some speed on the bounce
                    ball->vel = mulVecFl2d(ball->vel, 0.9f);
                    // Mark this bumper as being touched to not double-bounce
                    setBallTouching(p->ballsTouching[bIdx], bumper, PIN_CIRCLE);
                }

                // Move ball back to not clip into the bumper
                ball->c.pos
                    = addVecFl2d(bumper->c.pos, mulVecFl2d(reflVec, ball->c.radius + bumper->c.radius - EPSILON));
            }
        }

        // Iterate over all walls
        for (uint32_t wIdx = 0; wIdx < p->numWalls; wIdx++)
        {
            pbLine_t* wall = &p->walls[wIdx];
            vecFl_t collisionVec;
            vecFl_t cpOnLine;
            // Check for a collision
            if ((ball->zoneMask & wall->zoneMask)                                        // In the same zone
                && circleLineFlIntersection(ball->c, wall->l, &cpOnLine, &collisionVec)) // and intersecting
            {
                /* TODO this reflection can have bad results when colliding with the tip of a line.
                 * The center-center vector can get weird if the ball moves fast and clips into the tip.
                 * The solution is probably to binary-search-move the ball as far as it'll go without clipping
                 */

                // Find the normalized vector along the collision normal
                vecFl_t centerToCenter = {
                    .x = collisionVec.x,
                    .y = collisionVec.y,
                };
                vecFl_t reflVec = normVecFl2d(centerToCenter);

                // If the ball isn't already touching the wall
                if (PIN_NO_SHAPE == ballIsTouching(p->ballsTouching[bIdx], wall))
                {
                    // Bounce it by reflecting across the collision normal
                    ball->vel = subVecFl2d(ball->vel, mulVecFl2d(reflVec, (2 * dotVecFl2d(ball->vel, reflVec))));
                    // Lose some speed on the bounce
                    ball->vel = mulVecFl2d(ball->vel, 0.9f);
                    // Mark this wall as being touched to not double-bounce
                    setBallTouching(p->ballsTouching[bIdx], wall, PIN_LINE);

                    // Bounced off a wall
                    ball->bounce = true;
                }

                // Move ball back to not clip into the bumper
                ball->c.pos = addVecFl2d(cpOnLine, mulVecFl2d(reflVec, ball->c.radius - EPSILON));
            }
        }
    }
}

/**
 * @brief TODO
 *
 * @param p
 */
void checkBallFlipperCollision(pinball_t* p)
{
    // For each ball, check collisions with flippers objects
    for (uint32_t bIdx = 0; bIdx < p->numBalls; bIdx++)
    {
        // Reference and integer representation
        pbCircle_t* ball = &p->balls[bIdx];

        // Iterate over all flippers
        for (uint32_t fIdx = 0; fIdx < p->numFlippers; fIdx++)
        {
            const pbFlipper_t* flipper = &p->flippers[fIdx];
            vecFl_t collisionVec;
            vecFl_t cpOnLine;

            // Check for a collision
            if ((ball->zoneMask & flipper->zoneMask)                               // In the same zone
                && PIN_NO_SHAPE == ballIsTouching(p->ballsTouching[bIdx], flipper) // and not already touching
                // And collides with a constituent part
                && (circleCircleFlIntersection(ball->c, flipper->cPivot, &collisionVec)
                    || circleCircleFlIntersection(ball->c, flipper->cTip, &collisionVec)
                    || circleLineFlIntersection(ball->c, flipper->sideL, &cpOnLine, &collisionVec)
                    || circleLineFlIntersection(ball->c, flipper->sideR, &cpOnLine, &collisionVec)))
            {
                // TODO account for flipper velocity
                // Reflect the velocity vector along the normal
                vecFl_t centerToCenter = {
                    .x = collisionVec.x,
                    .y = collisionVec.y,
                };
                vecFl_t reflVec = normVecFl2d(centerToCenter);
                ball->vel       = subVecFl2d(ball->vel, mulVecFl2d(reflVec, (2 * dotVecFl2d(ball->vel, reflVec))));

                // Mark this flipper as being touched
                setBallTouching(p->ballsTouching[bIdx], flipper, PIN_FLIPPER);
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

        // Acceleration changes velocity
        // TODO adjust gravity vector when on top of a line
        ball->vel = addVecFl2d(ball->vel, ball->accel);

        // Save the last position to check if the ball is at rest
        ball->lastPos = ball->c.pos;

        // Move the ball
        ball->c.pos.x += (ball->vel.x);
        ball->c.pos.y += (ball->vel.y);
    }
}

/**
 * @brief TODO
 *
 * @param p
 */
void moveFlippers(pinball_t* p)
{
    // For each flipper
    for (uint32_t fIdx = 0; fIdx < p->numFlippers; fIdx++)
    {
        pbFlipper_t* flipper = &p->flippers[fIdx];

        // TODO use angular velocity (aVelocity)?

        bool posChanged = false;
        if (flipper->buttonHeld)
        {
            if (flipper->facingRight && flipper->angle > (90 - FLIPPER_UP_ANGLE))
            {
                flipper->angle -= FLIPPER_UP_DEGREES_PER_FRAME;
                if (flipper->angle < 90 - FLIPPER_UP_ANGLE)
                {
                    flipper->angle = 90 - FLIPPER_UP_ANGLE;
                }
                posChanged = true;
            }
            else if (!flipper->facingRight && flipper->angle < 270 + FLIPPER_UP_ANGLE)
            {
                flipper->angle += FLIPPER_UP_DEGREES_PER_FRAME;
                if (flipper->angle > 270 + FLIPPER_UP_ANGLE)
                {
                    flipper->angle = 270 + FLIPPER_UP_ANGLE;
                }
                posChanged = true;
            }
        }
        else
        {
            if (flipper->facingRight && flipper->angle < (90 + FLIPPER_DOWN_ANGLE))
            {
                flipper->angle += FLIPPER_DOWN_DEGREES_PER_FRAME;
                if (flipper->angle > (90 + FLIPPER_DOWN_ANGLE))
                {
                    flipper->angle = (90 + FLIPPER_DOWN_ANGLE);
                }
                posChanged = true;
            }
            else if (!flipper->facingRight && flipper->angle > 270 - FLIPPER_DOWN_ANGLE)
            {
                flipper->angle -= FLIPPER_DOWN_DEGREES_PER_FRAME;
                if (flipper->angle < (270 - FLIPPER_DOWN_ANGLE))
                {
                    flipper->angle = (270 - FLIPPER_DOWN_ANGLE);
                }
                posChanged = true;
            }
        }

        if (posChanged)
        {
            updateFlipperPos(flipper);
        }
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
        // For each thing it could be touching
        for (uint32_t tIdx = 0; tIdx < MAX_NUM_TOUCHES; tIdx++)
        {
            pbTouchRef_t* tr = &p->ballsTouching[bIdx][tIdx];
            // If it's touching a thing
            if (NULL != tr->obj)
            {
                bool setNotTouching = false;
                switch (tr->type)
                {
                    case PIN_CIRCLE:
                    {
                        const pbCircle_t* other = (const pbCircle_t*)tr->obj;
                        if ((0 == (ball->zoneMask & other->zoneMask))                // Not in the same zone
                            || !circleCircleFlIntersection(ball->c, other->c, NULL)) // or not touching
                        {
                            setNotTouching = true;
                        }
                        break;
                    }
                    case PIN_LINE:
                    {
                        const pbLine_t* other = (const pbLine_t*)tr->obj;
                        if ((0 == (ball->zoneMask & other->zoneMask))                    // Not in the same zone
                            || !circleLineFlIntersection(ball->c, other->l, NULL, NULL)) // or not touching
                        {
                            setNotTouching = true;
                        }
                        break;
                    }
                    case PIN_RECT:
                    {
                        const pbRect_t* other = (const pbRect_t*)tr->obj;
                        if ((0 == (ball->zoneMask & other->zoneMask))              // Not in the same zone
                            || !circleRectFlIntersection(ball->c, other->r, NULL)) // or not touching
                        {
                            setNotTouching = true;
                        }
                        break;
                    }
                    case PIN_FLIPPER:
                    {
                        const pbFlipper_t* flipper = (const pbFlipper_t*)tr->obj;
                        if ((0 == (ball->zoneMask & flipper->zoneMask))                    // Not in the same zone
                            || (circleCircleFlIntersection(ball->c, flipper->cPivot, NULL) // or not touching
                                && circleCircleFlIntersection(ball->c, flipper->cTip, NULL)
                                && circleLineFlIntersection(ball->c, flipper->sideL, NULL, NULL)
                                && circleLineFlIntersection(ball->c, flipper->sideR, NULL, NULL)))
                        {
                            setNotTouching = true;
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

                // If the object is no longer touching
                if (setNotTouching)
                {
                    // Clear the reference
                    tr->obj  = NULL;
                    tr->type = PIN_NO_SHAPE;
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
void setBallTouching(pbTouchRef_t* ballTouching, const void* obj, pbShapeType_t type)
{
    for (uint32_t i = 0; i < MAX_NUM_TOUCHES; i++)
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
pbShapeType_t ballIsTouching(pbTouchRef_t* ballTouching, const void* obj)
{
    for (uint32_t i = 0; i < MAX_NUM_TOUCHES; i++)
    {
        if (ballTouching->obj == obj)
        {
            return ballTouching->type;
        }
    }
    return PIN_NO_SHAPE;
}

/**
 * @brief TODO
 *
 * @param p
 */
void checkBallsAtRest(pinball_t* p)
{
    // For each ball
    for (uint32_t bIdx = 0; bIdx < p->numBalls; bIdx++)
    {
        pbCircle_t* ball = &p->balls[bIdx];

        // If the ball didn't bounce this frame (which can adjust position to not clip)
        if (false == ball->bounce)
        {
            // See how far the ball actually traveled
            float posDeltaM = sqMagVecFl2d(subVecFl2d(ball->c.pos, ball->lastPos));
            float velM      = sqMagVecFl2d(ball->vel);

            // If the ball didn't move as much as it should have
            if ((velM - posDeltaM) > 0.01f)
            {
                // Stop the ball altogether to not accumulate velocity
                ball->vel.x = 0;
                ball->vel.y = 0;
            }
        }
        else
        {
            // Clear the bounce flag
            ball->bounce = false;
        }
    }
}

/**
 * @brief Update the position of a flipper's tip circle and line walls depending on the flipper's angle. The pivot
 * circle never changes.
 *
 * @param f The flipper to update
 */
void updateFlipperPos(pbFlipper_t* f)
{
    // Make sure the angle is between 0 and 360
    while (f->angle < 0)
    {
        f->angle += 360;
    }
    while (f->angle > 359)
    {
        f->angle -= 360;
    }

    // This is the set of points to rotate
    vecFl_t points[] = {
        {
            // Center of the tip of the flipper
            .x = 0,
            .y = -f->length,
        },
        {
            // Bottom point of the right side
            .x = f->cPivot.radius,
            .y = 0,
        },
        {
            // Top point of the right side
            .x = f->cTip.radius,
            .y = -f->length,
        },
        {
            // Bottom point of the left side
            .x = -f->cPivot.radius,
            .y = 0,
        },
        {
            // Top point of the left side
            .x = -f->cTip.radius,
            .y = -f->length,
        },
    };

    // This is where to write the rotated points
    vecFl_t* dests[] = {
        &f->cTip.pos, &f->sideR.p1, &f->sideR.p2, &f->sideL.p1, &f->sideL.p2,
    };

    // Get the trig values for all rotations, just once
    int16_t sin = getSin1024(f->angle);
    int16_t cos = getCos1024(f->angle);

    // For each point
    for (int32_t idx = 0; idx < ARRAY_SIZE(points); idx++)
    {
        // Rotate the point
        int32_t oldX = points[idx].x;
        int32_t oldY = points[idx].y;
        int32_t newX = (oldX * cos) - (oldY * sin);
        int32_t newY = (oldX * sin) + (oldY * cos);

        // Do some rounding for newX before scaling down
        if (newX >= 0)
        {
            newX += 512;
        }
        else
        {
            newX -= 512;
        }

        // Do some rounding for newY before scaling down
        if (newY >= 0)
        {
            newY += 512;
        }
        else
        {
            newY -= 512;
        }

        // Return to integer space and translate relative to the pivot point
        dests[idx]->x = f->cPivot.pos.x + (newX / 1024);
        dests[idx]->y = f->cPivot.pos.y + (newY / 1024);
    }
}
