//==============================================================================
// Includes
//==============================================================================

#include <math.h>
#include "pinball_physics.h"
#include "pinball_zones.h"

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
void setBallTouchingAccel(pbTouchRef_t* ballTouching, const void* obj, pbShapeType_t type, vecFl_t additiveAccel);
pbShapeType_t ballIsTouching(pbTouchRef_t* ballTouching, const void* obj);

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
            // Check for a new collision
            if ((ball->zoneMask & otherBall->zoneMask)                               // In the same zone
                && PIN_NO_SHAPE == ballIsTouching(p->ballsTouching[bIdx], otherBall) // and not already touching
                && circleCircleFlIntersection(ball->c, otherBall->c, NULL))          // and intersecting
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
                && PIN_NO_SHAPE == ballIsTouching(p->ballsTouching[bIdx], bumper) // and not already touching
                && circleCircleFlIntersection(ball->c, bumper->c, &collisionVec)) // and intersecting
            {
                // Reflect the velocity vector along the normal between the two radii
                // See http://www.sunshine2k.de/articles/coding/vectorreflection/vectorreflection.html
                vecFl_t centerToCenter = {
                    .x = collisionVec.x,
                    .y = collisionVec.y,
                };
                vecFl_t reflVec = normVecFl2d(centerToCenter);
                ball->vel       = subVecFl2d(ball->vel, mulVecFl2d(reflVec, (2 * dotVecFl2d(ball->vel, reflVec))));

                // Mark this wall as being touched
                setBallTouching(p->ballsTouching[bIdx], bumper, PIN_CIRCLE);
            }
        }

        // Iterate over all walls
        for (uint32_t wIdx = 0; wIdx < p->numWalls; wIdx++)
        {
            pbLine_t* wall = &p->walls[wIdx];
            vecFl_t collisionVec;
            // Check for a collision
            if ((ball->zoneMask & wall->zoneMask)                               // In the same zone
                && PIN_NO_SHAPE == ballIsTouching(p->ballsTouching[bIdx], wall) // and not already touching
                && circleLineFlIntersection(ball->c, wall->l, &collisionVec))   // and intersecting
            {
                /* TODO this reflection can have bad results when colliding with the tip of a line.
                 * The center-center vector can get weird if the ball moves fast and clips into the tip.
                 * The solution is probably to binary-search-move the ball as far as it'll go without clipping
                 */

                // Collision detected, do some physics
                vecFl_t centerToCenter = {
                    .x = collisionVec.x,
                    .y = collisionVec.y,
                };
                vecFl_t reflVec = normVecFl2d(centerToCenter);
                ball->vel       = subVecFl2d(ball->vel, mulVecFl2d(reflVec, (2 * dotVecFl2d(ball->vel, reflVec))));

                // The force that may be statically applied if the ball and line stay in contact
                vecFl_t accelDelta = {
                    .x = 0,
                    .y = 0,
                };
                // If the line is below the circle
                if (collisionVec.y < 0)
                {
                    // Check if the reflection is close enough to the slope to stick to the line
                    // TODO care about momentum?
                    vecFl_t wallSlope;
                    if (wall->l.p1.y < wall->l.p2.y)
                    {
                        wallSlope.y = (wall->l.p2.y - wall->l.p1.y);
                        wallSlope.x = (wall->l.p2.x - wall->l.p1.x);
                    }
                    else
                    {
                        wallSlope.y = (wall->l.p1.y - wall->l.p2.y);
                        wallSlope.x = (wall->l.p1.x - wall->l.p2.x);
                    }

                    // Find the angle between the reflected velocity and wall
                    float velocityMag  = magVecFl2d(ball->vel);
                    float angleBetween = acosf(dotVecFl2d(ball->vel, wallSlope) / (velocityMag * wall->length));

                    // vecFl_t testA = {.x = 0, .y = -1,};
                    // for(int i = 0; i < 256; i++)
                    // {
                    //     vecFl_t testB = {.x = 0, .y = -1,};
                    //     testB = rotateVecFl2d(testB, (i * 2 * M_PIf) / 256.0f);
                    //     float res = acosf(dotVecFl2d(testA, testB));
                    //     printf("(%0.3f, %0.3f) and (%0.3f, %0.3f) -> %0.3f\n", testA.x, testA.y, testB.x, testB.y,
                    //     res);
                    // }
                    // exit(0);

                    // If the angle is small
                    // TODO pick something less arbitrary?
                    if (angleBetween < 0.6f)
                    {
                        // Have the ball ride the line

                        // Project velocity onto wall slope (a small amount is lost)
                        ball->vel = mulVecFl2d(wallSlope,
                                               (dotVecFl2d(ball->vel, wallSlope) / dotVecFl2d(wallSlope, wallSlope)));

                        // This is the force applied by the slope
                        float sinTh = wallSlope.y / wall->length;
                        float cosTh = wallSlope.x / wall->length;
                        // On a slope of angle th, the normal force to the slope is g*sin(th) and the acceleration
                        // straight down the slope is g*sin(th) Doing more trig to get the axis aligned vectors, the Y
                        // acceleration is g*sin(th)*sin(th) and the X acceleration is g*cos(th)*sin(th) The -1 in the Y
                        // acceleration is to counteract existing gravity, since this is summed
                        accelDelta.x = PINBALL_GRAVITY * sinTh * cosTh;
                        accelDelta.y = PINBALL_GRAVITY * (sinTh * sinTh - 1);

                        // Add the force to the ball's acceleration
                        ball->accel = addVecFl2d(ball->accel, accelDelta);
                    }
                }

                // Mark this wall as being touched, with acceleration
                setBallTouchingAccel(p->ballsTouching[bIdx], wall, PIN_LINE, accelDelta);
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

            // Check for a collision
            if ((ball->zoneMask & flipper->zoneMask)                               // In the same zone
                && PIN_NO_SHAPE == ballIsTouching(p->ballsTouching[bIdx], flipper) // and not already touching
                // And collides with a constituent part
                && (circleCircleFlIntersection(ball->c, flipper->cPivot, &collisionVec)
                    || circleCircleFlIntersection(ball->c, flipper->cTip, &collisionVec)
                    || circleLineFlIntersection(ball->c, flipper->sideL, &collisionVec)
                    || circleLineFlIntersection(ball->c, flipper->sideR, &collisionVec)))
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
                        vecFl_t collisionVec;
                        if ((0 == (ball->zoneMask & other->zoneMask))                       // Not in the same zone
                            || !circleLineFlIntersection(ball->c, other->l, &collisionVec)) // or not touching
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
                                && circleLineFlIntersection(ball->c, flipper->sideL, NULL)
                                && circleLineFlIntersection(ball->c, flipper->sideR, NULL)))
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
                    // Remove this object's acceleration from the ball
                    ball->accel = subVecFl2d(ball->accel, tr->additiveAccel);
                    tr->additiveAccel.x = 0;
                    tr->additiveAccel.y = 0;
                    // Clear the reference
                    tr->obj             = NULL;
                    tr->type            = PIN_NO_SHAPE;
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
    vecFl_t additiveAccel = {
        .x = 0,
        .y = 0,
    };
    setBallTouchingAccel(ballTouching, obj, type, additiveAccel);
}

/**
 * @brief TODO
 *
 * @param ballTouching
 * @param obj
 * @param type
 * @param additiveAccel
 */
void setBallTouchingAccel(pbTouchRef_t* ballTouching, const void* obj, pbShapeType_t type, vecFl_t additiveAccel)
{
    for (uint32_t i = 0; i < MAX_NUM_TOUCHES; i++)
    {
        if (NULL == ballTouching->obj)
        {
            ballTouching->obj           = obj;
            ballTouching->type          = type;
            ballTouching->additiveAccel = additiveAccel;
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
