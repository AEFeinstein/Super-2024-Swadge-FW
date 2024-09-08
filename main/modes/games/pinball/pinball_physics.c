#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "pinball_line.h"
#include "pinball_circle.h"
#include "pinball_rectangle.h"
#include "pinball_flipper.h"
#include "pinball_triangle.h"
#include "pinball_physics.h"

static void handleBallBallCollision(jsBall_t* ball1, jsBall_t* ball2);
static void handleBallCircleCollision(jsScene_t* scene, jsBall_t* ball, jsCircle_t* circle);
static void handleBallFlipperCollision(jsBall_t* ball, jsFlipper_t* flipper);
static bool handleBallLineCollision(jsBall_t* ball, jsScene_t* scene);
static void handleBallLauncherCollision(jsLauncher_t* launcher, jsBall_t* ball, float dt);

/**
 * @brief TODO
 *
 * @param scene
 * @param elapsedUs
 */
void jsSimulate(jsScene_t* scene, int32_t elapsedUs)
{
    float elapsedUsFl = elapsedUs / 1000000.0f;

    for (int32_t i = 0; i < scene->numFlippers; i++)
    {
        jsFlipperSimulate(&scene->flippers[i], elapsedUsFl);
    }

    for (int32_t i = 0; i < scene->numLaunchers; i++)
    {
        jsLauncherSimulate(&scene->launchers[i], &scene->balls, elapsedUsFl);
    }

    node_t* bNode = scene->balls.first;
    while (bNode)
    {
        jsBall_t* ball = bNode->val;

        jsBallSimulate(ball, elapsedUs, elapsedUsFl, scene);

        node_t* bNode2 = bNode->next;
        while (bNode2)
        {
            jsBall_t* ball2 = bNode2->val;
            handleBallBallCollision(ball, ball2);
            bNode2 = bNode2->next;
        }

        for (int32_t cIdx = 0; cIdx < scene->numCircles; cIdx++)
        {
            handleBallCircleCollision(scene, ball, &scene->circles[cIdx]);
        }

        for (int32_t fIdx = 0; fIdx < scene->numFlippers; fIdx++)
        {
            handleBallFlipperCollision(ball, &scene->flippers[fIdx]);
        }

        for (int32_t lIdx = 0; lIdx < scene->numLaunchers; lIdx++)
        {
            handleBallLauncherCollision(&scene->launchers[lIdx], ball, elapsedUs);
        }

        // Collide ball with lines
        if (handleBallLineCollision(ball, scene))
        {
            // Iterate to the next ball node
            bNode = bNode->next;

            // Then remove the ball
            jsRemoveBall(ball, scene);
        }
        else
        {
            // Iterate to the next ball
            bNode = bNode->next;
        }
    }

    for (int32_t cIdx = 0; cIdx < scene->numCircles; cIdx++)
    {
        jsCircleTimer(&scene->circles[cIdx], elapsedUs);
    }

    for (int32_t tIdx = 0; tIdx < scene->numTriangles; tIdx++)
    {
        jsTriangleTimer(&scene->triangles[tIdx], elapsedUs);
    }

    for (int32_t lIdx = 0; lIdx < scene->numLines; lIdx++)
    {
        jsLineTimer(&scene->lines[lIdx], elapsedUs, scene);
    }
}

/**
 * @brief Find the closest point to point p on a line segment between a and b
 *
 * @param p A point
 * @param a One end of a line segment
 * @param b The other end of a line segment
 * @return A point on the line segment closest to p
 */
static vecFl_t closestPointOnSegment(vecFl_t p, vecFl_t a, vecFl_t b)
{
    vecFl_t ab = subVecFl2d(b, a);
    float t    = sqMagVecFl2d(ab);

    if (t == 0.0f)
    {
        return a;
    }

    t = (dotVecFl2d(p, ab) - dotVecFl2d(a, ab)) / t;
    if (t > 1)
    {
        t = 1;
    }
    else if (t < 0)
    {
        t = 0;
    }

    return addVecFl2d(a, mulVecFl2d(ab, t));
}

/**
 * @brief TODO doc
 *
 * @param ball1
 * @param ball2
 */
static void handleBallBallCollision(jsBall_t* ball1, jsBall_t* ball2)
{
    float restitution = MIN(ball1->restitution, ball2->restitution);
    vecFl_t dir       = subVecFl2d(ball2->pos, ball1->pos);
    float d           = magVecFl2d(dir);
    if (0 == d || d > (ball1->radius + ball2->radius))
    {
        return;
    }

    dir = divVecFl2d(dir, d);

    float corr = (ball1->radius + ball2->radius - d) / 2.0f;
    ball1->pos = addVecFl2d(ball1->pos, mulVecFl2d(dir, -corr));
    ball2->pos = addVecFl2d(ball2->pos, mulVecFl2d(dir, corr));

    float v1 = dotVecFl2d(ball1->vel, dir);
    float v2 = dotVecFl2d(ball2->vel, dir);

    float m1 = ball1->mass;
    float m2 = ball2->mass;

    float newV1 = (m1 * v1 + m2 * v2 - m2 * (v1 - v2) * restitution) / (m1 + m2);
    float newV2 = (m1 * v1 + m2 * v2 - m1 * (v2 - v1) * restitution) / (m1 + m2);

    ball1->vel = addVecFl2d(ball1->vel, mulVecFl2d(dir, newV1 - v1));
    ball2->vel = addVecFl2d(ball2->vel, mulVecFl2d(dir, newV2 - v2));
}

/**
 * @brief TODO doc
 *
 * @param scene
 * @param ball
 * @param circle
 */
static void handleBallCircleCollision(jsScene_t* scene, jsBall_t* ball, jsCircle_t* circle)
{
    vecFl_t dir = subVecFl2d(ball->pos, circle->pos);
    float d     = magVecFl2d(dir);
    if (d == 0.0 || d > (ball->radius + circle->radius))
    {
        if (circle->id == scene->touchedLoopId)
        {
            scene->touchedLoopId = PIN_INVALID_ID;
        }
        return;
    }

    if (JS_BUMPER == circle->type)
    {
        // Normalize the direction
        dir = divVecFl2d(dir, d);

        // Move ball backwards to not clip
        float corr = ball->radius + circle->radius - d;
        ball->pos  = addVecFl2d(ball->pos, mulVecFl2d(dir, corr));

        // Adjust the velocity
        float v   = dotVecFl2d(ball->vel, dir);
        ball->vel = addVecFl2d(ball->vel, mulVecFl2d(dir, circle->pushVel - v));

        circle->litTimer = 250000;
    }
    else if (JS_ROLLOVER == circle->type)
    {
        if (circle->id != scene->touchedLoopId)
        {
            scene->touchedLoopId = circle->id;

            memmove(&scene->loopHistory[1], &scene->loopHistory[0],
                    sizeof(scene->loopHistory) - sizeof(scene->loopHistory[0]));
            scene->loopHistory[0] = circle->id;

            if (scene->loopHistory[0] + 1 == scene->loopHistory[1]
                && scene->loopHistory[1] + 1 == scene->loopHistory[2])
            {
                printf("Loop Counter Clockwise\n");
            }
            else if (scene->loopHistory[2] + 1 == scene->loopHistory[1]
                     && scene->loopHistory[1] + 1 == scene->loopHistory[0])
            {
                printf("Loop Clockwise\n");
            }
        }
        // Group two rollovers should close the launch tube
        // TODO hardcoding a group ID is gross
        if (5 == circle->groupId)
        {
            jsOpenLaunchTube(scene, false);
        }
    }
}

/**
 * @brief TODO doc
 *
 * @param ball
 * @param flipper
 */
static void handleBallFlipperCollision(jsBall_t* ball, jsFlipper_t* flipper)
{
    vecFl_t closest = closestPointOnSegment(ball->pos, flipper->pos, jsFlipperGetTip(flipper));
    vecFl_t dir     = subVecFl2d(ball->pos, closest);
    float d         = magVecFl2d(dir);
    if (d == 0.0 || d > ball->radius + flipper->radius)
    {
        return;
    }

    dir = divVecFl2d(dir, d);

    float corr = (ball->radius + flipper->radius - d);
    ball->pos  = addVecFl2d(ball->pos, mulVecFl2d(dir, corr));

    // update velocity

    vecFl_t radius     = closest;
    radius             = addVecFl2d(radius, mulVecFl2d(dir, flipper->radius));
    radius             = subVecFl2d(radius, flipper->pos);
    vecFl_t surfaceVel = perpendicularVecFl2d(radius);
    surfaceVel         = mulVecFl2d(surfaceVel, flipper->currentAngularVelocity);

    float v    = dotVecFl2d(ball->vel, dir);
    float vNew = dotVecFl2d(surfaceVel, dir);

    ball->vel = addVecFl2d(ball->vel, mulVecFl2d(dir, vNew - v));
}

/**
 * @brief TODO
 *
 * @param ball
 * @param line
 * @return true
 * @return false
 */
bool ballLineIntersection(jsBall_t* ball, jsLine_t* line)
{
    // Get the line segment from the list of walls
    vecFl_t a = line->p1;
    vecFl_t b = line->p2;
    // Get the closest point on the segment to the center of the ball
    vecFl_t c = closestPointOnSegment(ball->pos, a, b);
    // Find the distance between the center of the ball and the closest point on the line
    vecFl_t d  = subVecFl2d(ball->pos, c);
    float dist = magVecFl2d(d);
    // If the distance is less than the radius, and the distance is less
    // than the minimum distance, its the best collision
    return (dist < ball->radius);
}

/**
 * @brief TODO doc
 *
 * @param ball
 * @param lines
 * @param true if the ball should be deleted, false if not
 */
static bool handleBallLineCollision(jsBall_t* ball, jsScene_t* scene)
{
    // find closest segment;
    vecFl_t ballToClosest;
    vecFl_t ab;
    vecFl_t normal;
    float minDist   = FLT_MAX;
    jsLine_t* cLine = NULL;

    // For each segment of the wall
    for (int32_t i = 0; i < scene->numLines; i++)
    {
        jsLine_t* line = &scene->lines[i];

        if (line->isUp)
        {
            // Get the line segment from the list of walls
            vecFl_t a = line->p1;
            vecFl_t b = line->p2;
            // Get the closest point on the segment to the center of the ball
            vecFl_t c = closestPointOnSegment(ball->pos, a, b);
            // Find the distance between the center of the ball and the closest point on the line
            vecFl_t d  = subVecFl2d(ball->pos, c);
            float dist = magVecFl2d(d);
            // If the distance is less than the radius, and the distance is less
            // than the minimum distance, its the best collision
            if ((dist < ball->radius) && (dist < minDist))
            {
                minDist       = dist;
                ballToClosest = d;
                ab            = subVecFl2d(b, a);
                normal        = perpendicularVecFl2d(ab);
                cLine         = line;
            }
        }
    }

    // Check if there were any collisions
    if (NULL == cLine)
    {
        return false;
    }

    // push out to not clip
    if (0 == minDist)
    {
        ballToClosest = normal;
        minDist       = magVecFl2d(normal);
    }
    ballToClosest = divVecFl2d(ballToClosest, minDist);
    ball->pos     = addVecFl2d(ball->pos, mulVecFl2d(ballToClosest, ball->radius - minDist)); // TODO epsilon here?

    float v = dotVecFl2d(ball->vel, ballToClosest);
    if (cLine->pushVel)
    {
        // Adjust the velocity
        ball->vel = addVecFl2d(ball->vel, mulVecFl2d(ballToClosest, cLine->pushVel - v));
    }
    else
    {
        // update velocity
        float vNew = ABS(v) * ball->restitution; // TODO care about wall's restitution?
        ball->vel  = addVecFl2d(ball->vel, mulVecFl2d(ballToClosest, vNew - v));
    }

    switch (cLine->type)
    {
        default:
        case JS_WALL:
        case JS_SPINNER:
        case JS_LAUNCH_DOOR:
        {
            break;
        }
        case JS_SLINGSHOT:
        case JS_STANDUP_TARGET:
        {
            cLine->litTimer = 250000;
            break;
        }
        case JS_DROP_TARGET:
        {
            cLine->isUp = false;

            // Check if all targets in the group are hit
            bool someLineUp = false;
            list_t* group   = cLine->group;
            node_t* node    = group->first;
            while (NULL != node)
            {
                jsLine_t* groupLine = node->val;
                if (groupLine->isUp)
                {
                    someLineUp = true;
                    break;
                }
                node = node->next;
            }

            // If all lines are down
            if (!someLineUp)
            {
                // Reset them
                // TODO start a timer for this? Make sure a ball isn't touching the line before resetting?
                node = group->first;
                while (NULL != node)
                {
                    // Start a timer to reset the target
                    ((jsLine_t*)node->val)->resetTimer = 3000000;
                    node                               = node->next;
                }
            }
            break;
        }
        case JS_SCOOP:
        {
            // Count the scoop
            scene->scoopCount++;
            printf("Ball %" PRId32 " locked\n", scene->scoopCount);
            if (3 == scene->scoopCount)
            {
                printf("Multiball!!!\n");
                jsStartMultiball(scene);
            }
            ball->scoopTimer = 2000000;
            break;
        }
        case JS_BALL_LOST:
        {
            return true;
        }
    }
    return false;
}

/**
 * @brief TODO
 *
 * @param launcher
 * @param balls
 * @param dt
 */
static void handleBallLauncherCollision(jsLauncher_t* launcher, jsBall_t* ball, float dt)
{
    if (ball->vel.y >= 0)
    {
        // Get the compressed Y level
        float posY = launcher->pos.y + (launcher->impulse * launcher->height);

        // Check Y
        if ((ball->pos.y + ball->radius > posY) && (ball->pos.y - ball->radius < posY))
        {
            // Check X
            if ((ball->pos.x > launcher->pos.x) && (ball->pos.x < launcher->pos.x + launcher->width))
            {
                // Collision, set the position to be slightly touching
                ball->pos.y = posY - ball->radius + 0.1f;
                // Bounce a little
                ball->vel = mulVecFl2d(ball->vel, -0.3f);
            }
        }
    }
}
