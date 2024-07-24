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
void checkBallFixedCollision(pinball_t* p);
void sweepCheckFlippers(pinball_t* p);

void moveBalls(pinball_t* p);

void checkBallsNotTouching(pinball_t* p);
void setBallTouching(pbTouchRef_t* ballTouching, const void* obj, pbShapeType_t type);
bool ballIsTouching(pbTouchRef_t* ballTouching, const void* obj);

void checkBallsAtRest(pinball_t* p);

void moveBallBackFromLine(pbCircle_t* ball, pbLine_t* line, vecFl_t* collisionVec);
void moveBallBackFromCircle(pbCircle_t* ball, pbCircle_t* fixed);

vecFl_t dampenBounce(vecFl_t velocity, vecFl_t normRefl);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Update a single physics frame by moving and colliding all objects
 *
 * @param p The entire game state
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

    // Check for collisions between balls and fixed objects
    checkBallFixedCollision(p);

    // Check if balls are actually at rest
    checkBallsAtRest(p);

    // Clear references to balls touching things after moving
    checkBallsNotTouching(p);
}

/**
 * @brief Check for collisions between moving balls and other moving balls
 *
 * @param p The entire game state
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
            vecFl_t centerToCenter;
            if ((ball->zoneMask & otherBall->zoneMask)                                       // In the same zone
                && circleCircleFlIntersection(ball->c, otherBall->c, NULL, &centerToCenter)) // and intersecting
            {
                // Move balls backwards equally from the midpoint to not clip
                float halfDistM     = (ball->c.radius + otherBall->c.radius + EPSILON) / 2.0f;
                vecFl_t midwayPoint = divVecFl2d(addVecFl2d(ball->c.pos, otherBall->c.pos), 2.0f);
                vecFl_t vecFromMid  = mulVecFl2d(normVecFl2d(centerToCenter), halfDistM);

                // Move both balls
                ball->c.pos      = addVecFl2d(midwayPoint, vecFromMid);
                otherBall->c.pos = subVecFl2d(midwayPoint, vecFromMid);

                // If the balls aren't touching yet, adjust velocities (bounce)
                if (!ballIsTouching(p->ballsTouching[bIdx], otherBall))
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
                        otherBall->vel = subVecFl2d(v1, mulVecFl2d(x1_x2, (dotVecFl2d(v1_v2, x1_x2) / xSqMag)));
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
 * @brief Check for collisions between moving balls and fixed circles
 *
 * @param ball The moving ball
 * @param circle The fixed circle
 * @param touchRef The touch references for the moving ball
 * @return true if the ball actively bounced off the circle this frame, false otherwise
 */
bool checkBallPbCircleCollision(pbCircle_t* ball, pbCircle_t* circle, pbTouchRef_t* touchRef)
{
    // Assume no bounce this frame
    bool bounced = false;

    // Check for a collision
    vecFl_t collisionVec;
    if ((ball->zoneMask & circle->zoneMask)                                     // In the same zone
        && circleCircleFlIntersection(ball->c, circle->c, NULL, &collisionVec)) // and intersecting
    {
        // If the ball isn't already touching the circle
        if (!ballIsTouching(touchRef, circle))
        {
            // Bounced on a circle
            ball->bounce = true;
            bounced      = true;

            // Find the normalized vector along the collision direction to reflect across
            vecFl_t reflVec = normVecFl2d(collisionVec);
            // Reflect the velocity vector along the normal between the two radii
            // See http://www.sunshine2k.de/articles/coding/vectorreflection/vectorreflection.html
            ball->vel = subVecFl2d(ball->vel, mulVecFl2d(reflVec, (2 * dotVecFl2d(ball->vel, reflVec))));
            // Mark this circle as being touched to not double-bounce
            setBallTouching(touchRef, circle, PIN_CIRCLE);
        }

        // Move ball back to not clip into the circle
        moveBallBackFromCircle(ball, circle);
    }
    return bounced;
}

/**
 * @brief Check for collisions between moving balls and fixed lines
 *
 * @param ball The moving ball
 * @param line The fixed line
 * @param touchRef The touch references for the moving ball
 * @return true if the ball actively bounced off the circle this frame, false otherwise
 */
bool checkBallPbLineCollision(pbCircle_t* ball, pbLine_t* line, pbTouchRef_t* touchRef)
{
    // Assume no bounce
    bool bounced = false;

    // Check for a collision
    vecFl_t collisionVec;
    vecFl_t cpOnLine;
    if ((ball->zoneMask & line->zoneMask)                                              // In the same zone
        && circleLineFlIntersection(ball->c, line->l, true, &cpOnLine, &collisionVec)) // and intersecting
    {
        /* TODO this reflection can have bad results when colliding with the tip of a line.
         * The center-center vector can get weird if the ball moves fast and clips into the tip.
         * The solution is probably to binary-search-move the ball as far as it'll go without clipping
         */

        // Find the normalized vector along the collision direction
        vecFl_t reflVec = normVecFl2d(collisionVec);

        // If the ball isn't already touching the line
        if (!ballIsTouching(touchRef, line))
        {
            // Bounce it by reflecting across the collision normal
            ball->vel = subVecFl2d(ball->vel, mulVecFl2d(reflVec, (2 * dotVecFl2d(ball->vel, reflVec))));

            // Dampen it
            ball->vel = dampenBounce(ball->vel, reflVec);

            // Mark this line as being touched to not double-bounce
            setBallTouching(touchRef, line, PIN_LINE);

            // Bounced off a line
            ball->bounce = true;
            bounced      = true;
        }

        // TODO accommodate line tip collisions (points)

        // Move ball back to not clip into the bumper
        moveBallBackFromLine(ball, line, &reflVec);
    }
    return bounced;
}

/**
 * @brief Check for collisions between moving balls and all fixed objects
 *
 * @param p The entire game state
 */
void checkBallFixedCollision(pinball_t* p)
{
    // For each ball, check collisions with fixed objects
    for (uint32_t bIdx = 0; bIdx < p->numBalls; bIdx++)
    {
        // Convenience reference
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
 * @brief Check for collisions between moving balls and moving flippers
 *
 * @param p The entire game state
 */
void sweepCheckFlippers(pinball_t* p)
{
    // For each flipper
    for (uint32_t fIdx = 0; fIdx < p->numFlippers; fIdx++)
    {
        pbFlipper_t* flipper = &p->flippers[fIdx];

        // Check if the flipper is moving up or down
        float angularVel = -FLIPPER_DOWN_RADIANS_PER_FRAME;
        if (flipper->buttonHeld)
        {
            angularVel = FLIPPER_UP_RADIANS_PER_FRAME;
        }

        // Find the bounds for the flipper depending on the direction it's facing
        float lBound = (M_PI + M_PI_2) - FLIPPER_DOWN_ANGLE;
        float uBound = (M_PI + M_PI_2) + FLIPPER_UP_ANGLE;
        if (flipper->facingRight)
        {
            lBound = M_PI_2 - FLIPPER_UP_ANGLE;
            uBound = M_PI_2 + FLIPPER_DOWN_ANGLE;
            // Flip velocity if facing right
            angularVel *= -1;
        }

        // Flipper starts here
        float sweepStart = flipper->angle;
        // Flipper ends here, bounded
        float sweepEnd = flipper->angle + angularVel;
        sweepEnd       = CLAMP((sweepEnd), lBound, uBound);

        // Find sweep steps if in motion
        float sweepStep = 0;
        int numSteps    = 1;
        if (sweepStart != sweepEnd)
        {
            // Flipper in motion
            numSteps  = 8;
            sweepStep = (sweepEnd - sweepStart) / (float)numSteps;
        }

        // Move the flipper a little, then check for collisions
        for (int32_t step = 0; step < numSteps; step++)
        {
            if (0 != sweepStep)
            {
                // Sweep the flipper a little
                flipper->angle += sweepStep;
                updateFlipperPos(flipper);
            }

            // Normal collision checks
            // For each ball, check collisions with flippers objects
            for (uint32_t bIdx = 0; bIdx < p->numBalls; bIdx++)
            {
                // Convenience references
                pbCircle_t* ball       = &p->balls[bIdx];
                pbTouchRef_t* touchRef = p->ballsTouching[bIdx];

                // If the ball and the flipper are in the same zone
                if (ball->zoneMask & flipper->zoneMask)
                {
                    // Check if the ball is touching any part of the flipper
                    // TODO maybe don't check pivot circle? or line tips?
                    bool touching = false;
                    vecFl_t colPoint, colVec;
                    if (circleLineFlIntersection(ball->c, flipper->sideL.l, false, &colPoint, &colVec))
                    {
                        // Move ball back to not clip into the flipper
                        colVec = normVecFl2d(colVec);
                        moveBallBackFromLine(ball, &flipper->sideL, &colVec);
                        touching = true;
                    }
                    if (circleLineFlIntersection(ball->c, flipper->sideR.l, false, &colPoint, &colVec))
                    {
                        // Move ball back to not clip into the flipper
                        colVec = normVecFl2d(colVec);
                        moveBallBackFromLine(ball, &flipper->sideR, &colVec);
                        touching = true;
                    }
                    if (circleCircleFlIntersection(ball->c, flipper->cPivot.c, &colPoint, &colVec))
                    {
                        // Move ball back to not clip into the flipper
                        moveBallBackFromCircle(ball, &flipper->cPivot);
                        touching = true;
                    }
                    if (circleCircleFlIntersection(ball->c, flipper->cTip.c, &colPoint, &colVec))
                    {
                        // Move ball back to not clip into the flipper
                        moveBallBackFromCircle(ball, &flipper->cTip);
                        touching = true;
                    }

                    // If the ball is touching the flipper for the first time
                    if (touching && !ballIsTouching(touchRef, flipper))
                    {
                        // Mark them as in contact
                        setBallTouching(touchRef, flipper, PIN_FLIPPER);

                        // Bounce the ball
                        vecFl_t reflVec = normVecFl2d(colVec);
                        ball->vel = subVecFl2d(ball->vel, mulVecFl2d(reflVec, (2 * dotVecFl2d(ball->vel, reflVec))));

                        // Dampen the bounce
                        ball->vel = dampenBounce(normVecFl2d(ball->vel), reflVec);

                        // If the flipper is in motion
                        if (sweepStart != sweepEnd)
                        {
                            // Get the distance between the pivot and the ball
                            float dist = magVecFl2d(subVecFl2d(flipper->cPivot.c.pos, ball->c.pos));
                            // Convert angular velocity of the flipper to linear velocity at that point
                            // TODO this should probably be turned down
                            float impulseMag = (ABS(angularVel) * dist);

                            // Impart an impulse on the ball along the collision normal
                            ball->vel = addVecFl2d(ball->vel, mulVecFl2d(reflVec, impulseMag));
                        }
                    }
                }
            }
        }

        // Make sure the final angle is correct
        flipper->angle = sweepEnd;
    }
}

/**
 * @brief Dampen a bounce linearly proportional to the incident angle
 *
 * @param velocity The ball's velocity, to be normalized into a direction
 * @param normRefl The normalized vector begin reflected across
 * @return The dampened velocity
 */
vecFl_t dampenBounce(vecFl_t velocity, vecFl_t normRefl)
{
    // The dot product of the normalized direction with the normalized collision vector is how much Y velocity
    // component there is. If this value is closer to 0 the ball should slide down the line (i.e. multiplier
    // closer to 1). If the value is closer to 1, lose more speed (i.e. multiplier closer to 0)
    vecFl_t dir = normVecFl2d(velocity);
    float aDot = fabsf(dotVecFl2d(dir, normRefl));
    // printf("|[%f, %f] dot [%f, %f]| == %f\n", dir.x, dir.y, normRefl.x, normRefl.y, aDot);
    float bounceMult = 0.25f + 0.75f * (1 - aDot);
    // printf("%f\n", bounceMult);
    return mulVecFl2d(velocity, bounceMult);
}

/**
 * @brief Move all balls
 *
 * @param p The entire game state
 */
void moveBalls(pinball_t* p)
{
    // For each ball
    for (uint32_t bIdx = 0; bIdx < p->numBalls; bIdx++)
    {
        // Convenience reference
        pbCircle_t* ball = &p->balls[bIdx];

        // Acceleration changes velocity
        ball->vel = addVecFl2d(ball->vel, ball->accel);

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
 * @brief Check if a ball is no longer touching an object
 *
 * @param p The entire game state
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
 * @brief Set that a ball is touching an object. This is used to track the moment of collision versus prolonged contact
 *
 * @param ballTouching A list of touch references for the ball
 * @param obj The object to set as touching the ball
 * @param type The object's type
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
 * @brief Check if a ball is touching an object
 *
 * @param ballTouching A list of touch references for the ball
 * @param obj The object to set as touching the ball
 * @return true if the ball is touching the object, false otherwise
 */
bool ballIsTouching(pbTouchRef_t* ballTouching, const void* obj)
{
    for (uint32_t i = 0; i < MAX_NUM_TOUCHES; i++)
    {
        if (ballTouching->obj == obj)
        {
            return true;
        }
    }
    return false;
}

/**
 * @brief Check if balls are at rest and adjust acceleration accordingly
 *
 * TODO this seems to think balls are at rest erroneously...
 *
 * @param p The entire game state
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
            // And the ball is traveling downward
            if (ball->vel.y > 0)
            {
                // See how far the ball actually traveled
                float velM = sqMagVecFl2d(ball->vel);

                // If the ball is moving slowly
                if (velM < 1.0f)
                {
                    // And it didn't move as much as it should have
                    float posDeltaM = sqMagVecFl2d(subVecFl2d(ball->c.pos, ball->lastPos));
                    if ((velM - posDeltaM) > 0.01f)
                    {
                        // Stop the ball altogether to not accumulate velocity
                        ball->vel.x = 0;
                        ball->vel.y = 0;
                    }
                }
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
    // Make sure the angle is between 0 and (2 * M_PI)
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
    float sinA = sinf(f->angle);
    float cosA = cosf(f->angle);

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
}

/**
 * @brief Move a ball backwards from a collision with a fixed line so that they are not intersecting
 *
 * @param ball The ball to move backwards
 * @param line The fixed line
 * @param collisionNorm The collision normal vector
 */
void moveBallBackFromLine(pbCircle_t* ball, pbLine_t* line, vecFl_t* collisionNorm)
{
    // First create a copy of the line
    lineFl_t barrierLine = line->l;

    // Then find the normal vector to the barrier, pointed towards the ball
    vecFl_t barrierOffset = mulVecFl2d(*collisionNorm, ball->c.radius + EPSILON);

    // Translate the along the normal vector, the distance of the radius
    // This creates a line parallel to the wall where the ball's center could be
    barrierLine.p1 = addVecFl2d(barrierLine.p1, barrierOffset);
    barrierLine.p2 = addVecFl2d(barrierLine.p2, barrierOffset);

    // Create a line for the ball's motion
    lineFl_t ballLine = {
        .p1 = ball->c.pos,
        .p2 = addVecFl2d(ball->c.pos, ball->vel),
    };

    // Find the intersection between where the ball's center could be and the ball's trajectory.
    // Set the ball's position to that point
    ball->c.pos = infLineIntersectionPoint(barrierLine, ballLine);
}

/**
 * @brief Move a ball backwards from a collision with a fixed circle so that they are not intersecting
 *
 * @param ball The ball to move backwards
 * @param fixed The fixed circle
 */
void moveBallBackFromCircle(pbCircle_t* ball, pbCircle_t* fixed)
{
    // Create a barrier circle around the fixed that the ball's center can't pass through
    circleFl_t barrier = fixed->c;
    barrier.radius += ball->c.radius + EPSILON;

    // Create a line for the ball's motion
    lineFl_t ballLine = {
        .p1 = ball->c.pos,
        .p2 = addVecFl2d(ball->c.pos, ball->vel),
    };

    // Find the intersection between the ball's center's motion and the barrier circle
    vecFl_t intersection_1;
    vecFl_t intersection_2;
    switch (circleLineFlIntersectionPoints(barrier, ballLine, &intersection_1, &intersection_2))
    {
        default:
        case 0:
        {
            // No intersection?
            break;
        }
        case 1:
        {
            // Single point of intersection (tangent)
            ball->c.pos = intersection_1;
            break;
        }
        case 2:
        {
            // Two intersection points, use the one closer to ball->c.pos
            float diff1 = sqMagVecFl2d(subVecFl2d(ball->c.pos, intersection_1));
            float diff2 = sqMagVecFl2d(subVecFl2d(ball->c.pos, intersection_2));
            if (diff1 < diff2)
            {
                ball->c.pos = intersection_1;
            }
            else
            {
                ball->c.pos = intersection_2;
            }
        }
    }
}
