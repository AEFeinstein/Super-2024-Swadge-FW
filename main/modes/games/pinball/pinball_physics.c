//==============================================================================
// Includes
//==============================================================================

#include <math.h>
#include "pinball_physics.h"
#include "pinball_zones.h"

//==============================================================================
// Function Declarations
//==============================================================================

bool checkBallPbCircleCollision(pbCircle_t* ball, pbCircle_t* circle, pbTouchRef_t* touchRef);
bool checkBallPbLineCollision(pbCircle_t* ball, pbLine_t* line, pbTouchRef_t* touchRef);

void checkBallBallCollisions(pinball_t* p);
void checkBallStaticCollision(pinball_t* p);
void sweepCheckFlippers(pinball_t* p);

void moveBalls(pinball_t* p);

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
    // Move balls along new vectors
    moveBalls(p);

    // Move flippers rotationally
    sweepCheckFlippers(p);

    // If there are multiple balls
    if (1 < p->numBalls)
    {
        // Check for ball-ball collisions
        checkBallBallCollisions(p);
    }

    // Check for collisions between balls and static objects
    checkBallStaticCollision(p);

    // Check if balls are actually at rest
    checkBallsAtRest(p);

    // Clear references to balls touching things after moving
    checkBallsNotTouching(p);
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
            if ((ball->zoneMask & otherBall->zoneMask)                                       // In the same zone
                && circleCircleFlIntersection(ball->c, otherBall->c, NULL, &centerToCenter)) // and intersecting
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
 * @param ball
 * @param circle
 * @param touchRef
 * @return true
 * @return false
 */
bool checkBallPbCircleCollision(pbCircle_t* ball, pbCircle_t* circle, pbTouchRef_t* touchRef)
{
    bool bounced = false;
    vecFl_t collisionVec;

    // Check for a collision
    if ((ball->zoneMask & circle->zoneMask)                                     // In the same zone
        && circleCircleFlIntersection(ball->c, circle->c, NULL, &collisionVec)) // and intersecting
    {
        // Find the normalized vector along the collision normal
        vecFl_t reflVec = normVecFl2d(collisionVec);

        // If the ball isn't already touching the circle
        if (PIN_NO_SHAPE == ballIsTouching(touchRef, circle))
        {
            // Bounced on a circle
            ball->bounce = true;
            bounced      = true;
            // Reflect the velocity vector along the normal between the two radii
            // See http://www.sunshine2k.de/articles/coding/vectorreflection/vectorreflection.html
            ball->vel = subVecFl2d(ball->vel, mulVecFl2d(reflVec, (2 * dotVecFl2d(ball->vel, reflVec))));
            // Lose some speed on the bounce
            ball->vel = mulVecFl2d(ball->vel, WALL_BOUNCINESS);
            // printf("%d,%.4f,%.4f\n", __LINE__, ball->vel.x, ball->vel.y);
            // Mark this circle as being touched to not double-bounce
            setBallTouching(touchRef, circle, PIN_CIRCLE);
        }

        // Move ball back to not clip into the circle
        ball->c.pos = addVecFl2d(circle->c.pos, mulVecFl2d(reflVec, ball->c.radius + circle->c.radius - EPSILON));
    }
    return bounced;
}

/**
 * @brief TODO
 *
 * @param ball
 * @param line
 * @param touchRef
 * @return true
 * @return false
 */
bool checkBallPbLineCollision(pbCircle_t* ball, pbLine_t* line, pbTouchRef_t* touchRef)
{
    bool bounced = false;
    vecFl_t collisionVec;
    vecFl_t cpOnLine;
    // Check for a collision
    if ((ball->zoneMask & line->zoneMask)                                              // In the same zone
        && circleLineFlIntersection(ball->c, line->l, true, &cpOnLine, &collisionVec)) // and intersecting
    {
        /* TODO this reflection can have bad results when colliding with the tip of a line.
         * The center-center vector can get weird if the ball moves fast and clips into the tip.
         * The solution is probably to binary-search-move the ball as far as it'll go without clipping
         */

        // Find the normalized vector along the collision normal
        vecFl_t reflVec = normVecFl2d(collisionVec);

        // If the ball isn't already touching the line
        if (PIN_NO_SHAPE == ballIsTouching(touchRef, line))
        {
            // Bounce it by reflecting across the collision normal
            ball->vel = subVecFl2d(ball->vel, mulVecFl2d(reflVec, (2 * dotVecFl2d(ball->vel, reflVec))));
            // Lose some speed on the bounce
            ball->vel = mulVecFl2d(ball->vel, WALL_BOUNCINESS);
            // printf("%d,%.4f,%.4f\n", __LINE__, ball->vel.x, ball->vel.y);
            // Mark this line as being touched to not double-bounce
            setBallTouching(touchRef, line, PIN_LINE);

            // Bounced off a line
            ball->bounce = true;
            bounced      = true;
        }

        // Move ball back to not clip into the bumper
        ball->c.pos = addVecFl2d(cpOnLine, mulVecFl2d(reflVec, ball->c.radius - EPSILON));
    }
    return bounced;
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
            checkBallPbCircleCollision(ball, &p->bumpers[uIdx], p->ballsTouching[bIdx]);
        }

        // Iterate over all walls
        for (uint32_t wIdx = 0; wIdx < p->numWalls; wIdx++)
        {
            checkBallPbLineCollision(ball, &p->walls[wIdx], p->ballsTouching[bIdx]);
        }
    }
}

/**
 * @brief TODO
 *
 * @param p
 */
void sweepCheckFlippers(pinball_t* p)
{
    // For each flipper
    for (uint32_t fIdx = 0; fIdx < p->numFlippers; fIdx++)
    {
        pbFlipper_t* flipper = &p->flippers[fIdx];

        // Assume no motion
        float sweepStart = flipper->angle;
        float sweepStep  = 0.0f;
        float sweepEnd   = flipper->angle;

        // Check if the flipper should be moving based on the button state
        if (flipper->buttonHeld)
        {
            if (flipper->facingRight && flipper->angle > (M_PI_2 - FLIPPER_UP_ANGLE))
            {
                sweepEnd = flipper->angle - FLIPPER_UP_DEGREES_PER_FRAME;
                if (sweepEnd < M_PI_2 - FLIPPER_UP_ANGLE)
                {
                    sweepEnd = M_PI_2 - FLIPPER_UP_ANGLE;
                }
            }
            else if (!flipper->facingRight && flipper->angle < (M_PI + M_PI_2) + FLIPPER_UP_ANGLE)
            {
                sweepEnd = flipper->angle + FLIPPER_UP_DEGREES_PER_FRAME;
                if (sweepEnd > (M_PI + M_PI_2) + FLIPPER_UP_ANGLE)
                {
                    sweepEnd = (M_PI + M_PI_2) + FLIPPER_UP_ANGLE;
                }
            }
        }
        else
        {
            if (flipper->facingRight && flipper->angle < (M_PI_2 + FLIPPER_DOWN_ANGLE))
            {
                sweepEnd = flipper->angle + FLIPPER_DOWN_DEGREES_PER_FRAME;
                if (sweepEnd > (M_PI_2 + FLIPPER_DOWN_ANGLE))
                {
                    sweepEnd = (M_PI_2 + FLIPPER_DOWN_ANGLE);
                }
            }
            else if (!flipper->facingRight && flipper->angle > (M_PI + M_PI_2) - FLIPPER_DOWN_ANGLE)
            {
                sweepEnd = flipper->angle - FLIPPER_DOWN_DEGREES_PER_FRAME;
                if (sweepEnd < ((M_PI + M_PI_2) - FLIPPER_DOWN_ANGLE))
                {
                    sweepEnd = ((M_PI + M_PI_2) - FLIPPER_DOWN_ANGLE);
                }
            }
        }

        // The flipper is moving if the sweep start and end are different
        flipper->moving = (sweepStart != sweepEnd);

        // Figure out the number and size of the steps if the flipper is moving or not
        int32_t numSteps = (flipper->moving) ? 8 : 1;
        sweepStep        = (sweepEnd - sweepStart) / (float)numSteps;

        // Move the flipper a little, then check for collisions
        for (int32_t step = 0; step < numSteps; step++)
        {
            // Sweep the flipper a little
            flipper->angle += sweepStep;
            updateFlipperPos(p, flipper);

            // Normal collision checks
            // For each ball, check collisions with flippers objects
            for (uint32_t bIdx = 0; bIdx < p->numBalls; bIdx++)
            {
                // Reference and integer representation
                pbCircle_t* ball       = &p->balls[bIdx];
                pbTouchRef_t* touchRef = p->ballsTouching[bIdx];

                // Check if the ball is touching any part of the flipper
                bool touching = false;
                vecFl_t colPoint, colVec;
                if (circleLineFlIntersection(ball->c, flipper->sideL.l, false, &colPoint, &colVec))
                {
                    // Move ball back to not clip into the flipper
                    ball->c.pos = addVecFl2d(colPoint, mulVecFl2d(normVecFl2d(colVec), ball->c.radius - EPSILON));
                    touching    = true;
                }
                if (circleLineFlIntersection(ball->c, flipper->sideR.l, false, &colPoint, &colVec))
                {
                    // Move ball back to not clip into the flipper
                    ball->c.pos = addVecFl2d(colPoint, mulVecFl2d(normVecFl2d(colVec), ball->c.radius - EPSILON));
                    touching    = true;
                }
                if (circleCircleFlIntersection(ball->c, flipper->cPivot.c, &colPoint, &colVec))
                {
                    // Move ball back to not clip into the flipper
                    ball->c.pos = addVecFl2d(colPoint, mulVecFl2d(normVecFl2d(colVec), ball->c.radius - EPSILON));
                    touching    = true;
                }
                if (circleCircleFlIntersection(ball->c, flipper->cTip.c, &colPoint, &colVec))
                {
                    // Move ball back to not clip into the flipper
                    ball->c.pos = addVecFl2d(colPoint, mulVecFl2d(normVecFl2d(colVec), ball->c.radius - EPSILON));
                    touching    = true;
                }

                // If the ball is touching the flipper for the first time
                if (touching && (PIN_NO_SHAPE == ballIsTouching(touchRef, flipper)))
                {
                    // Mark them as in contact
                    setBallTouching(touchRef, flipper, PIN_FLIPPER);

                    // Bounce the ball
                    vecFl_t reflVec = normVecFl2d(colVec);
                    ball->vel       = subVecFl2d(ball->vel, mulVecFl2d(reflVec, (2 * dotVecFl2d(ball->vel, reflVec))));

                    // If the flipper is in motion
                    if (flipper->moving)
                    {
                        // Impart an impulse on the ball
                        vecFl_t impulse;
                        if (flipper->facingRight)
                        {
                            impulse.x = flipper->sideL.l.p2.y - flipper->sideL.l.p1.y;
                            impulse.y = flipper->sideL.l.p1.x - flipper->sideL.l.p2.x;
                        }
                        else
                        {
                            impulse.x = flipper->sideL.l.p1.y - flipper->sideL.l.p2.y;
                            impulse.y = flipper->sideL.l.p2.x - flipper->sideL.l.p1.x;
                        }
                        ball->vel = addVecFl2d(ball->vel, mulVecFl2d(normVecFl2d(impulse), 2.0f));
                        // printf("%d,%.4f,%.4f\n", __LINE__, ball->vel.x, ball->vel.y);
                    }
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
void moveBalls(pinball_t* p)
{
    // For each ball, check collisions with static objects
    for (uint32_t bIdx = 0; bIdx < p->numBalls; bIdx++)
    {
        pbCircle_t* ball = &p->balls[bIdx];

        // Acceleration changes velocity
        // TODO adjust gravity vector when on top of a line
        ball->vel = addVecFl2d(ball->vel, ball->accel);
        // printf("%d,%.4f,%.4f\n", __LINE__, ball->vel.x, ball->vel.y);

        // Save the last position to check if the ball is at rest
        ball->lastPos = ball->c.pos;

        // Move the ball
        ball->c.pos.x += (ball->vel.x);
        ball->c.pos.y += (ball->vel.y);

        // Update zone mask
        // TODO update this after nudging ball too?
        ball->zoneMask = pinZoneCircle(p, *ball);
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
                        if ((0 == (ball->zoneMask & other->zoneMask))                      // Not in the same zone
                            || !circleCircleFlIntersection(ball->c, other->c, NULL, NULL)) // or not touching
                        {
                            setNotTouching = true;
                        }
                        break;
                    }
                    case PIN_LINE:
                    {
                        const pbLine_t* other = (const pbLine_t*)tr->obj;
                        if ((0 == (ball->zoneMask & other->zoneMask))                          // Not in the same zone
                            || !circleLineFlIntersection(ball->c, other->l, true, NULL, NULL)) // or not touching
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
                        if ((0 == (ball->zoneMask & flipper->zoneMask)) // Not in the same zone
                            || !(circleCircleFlIntersection(ball->c, flipper->cPivot.c, NULL, NULL) // or not touching
                                 || circleCircleFlIntersection(ball->c, flipper->cTip.c, NULL, NULL)
                                 || circleLineFlIntersection(ball->c, flipper->sideL.l, false, NULL, NULL)
                                 || circleLineFlIntersection(ball->c, flipper->sideR.l, false, NULL, NULL)))
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
        // And the ball is traveling downward
        if (false == ball->bounce && ball->vel.y > 0)
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
void updateFlipperPos(pinball_t* p, pbFlipper_t* f)
{
    // Make sure the angle is between 0 and 360
    while (f->angle < 0)
    {
        f->angle += (2 * M_PI);
    }
    while (f->angle >= (2 * M_PI))
    {
        f->angle -= (2 * M_PI);
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
            .x = f->cPivot.c.radius,
            .y = 0,
        },
        {
            // Top point of the right side
            .x = f->cTip.c.radius,
            .y = -f->length,
        },
        {
            // Bottom point of the left side
            .x = -f->cPivot.c.radius,
            .y = 0,
        },
        {
            // Top point of the left side
            .x = -f->cTip.c.radius,
            .y = -f->length,
        },
    };

    // This is where to write the rotated points
    vecFl_t* dests[] = {
        &f->cTip.c.pos, &f->sideR.l.p1, &f->sideR.l.p2, &f->sideL.l.p1, &f->sideL.l.p2,
    };

    // Get the trig values for all rotations, just once
    float sinA = sin(f->angle);
    float cosA = cos(f->angle);

    // For each point
    for (int32_t idx = 0; idx < ARRAY_SIZE(points); idx++)
    {
        // Rotate the point
        float oldX = points[idx].x;
        float oldY = points[idx].y;
        float newX = (oldX * cosA) - (oldY * sinA);
        float newY = (oldX * sinA) + (oldY * cosA);

        // Translate relative to the pivot point
        dests[idx]->x = f->cPivot.c.pos.x + newX;
        dests[idx]->y = f->cPivot.c.pos.y + newY;
    }

    // Update zones
    f->cPivot.zoneMask = pinZoneCircle(p, f->cPivot);
    f->cTip.zoneMask   = pinZoneCircle(p, f->cTip);
    f->sideL.zoneMask  = pinZoneLine(p, f->sideL);
    f->sideR.zoneMask  = pinZoneLine(p, f->sideR);

    // The flipper's zone is all zones combined
    f->zoneMask = (f->cPivot.zoneMask | f->cTip.zoneMask | f->sideL.zoneMask | f->sideR.zoneMask);
}
