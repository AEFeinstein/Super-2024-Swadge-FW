#include "pinball_line.h"
#include "pinball_circle.h"
#include "pinball_rectangle.h"
#include "pinball_flipper.h"

static void handleBallBallCollision(jsBall_t* ball1, jsBall_t* ball2);
static void handleBallObstacleCollision(jsScene_t* scene, jsBall_t* ball, jsObstacle_t* obstacle);
static void handleBallFlipperCollision(jsBall_t* ball, jsFlipper_t* flipper);
static void handleBallLineCollision(jsBall_t* ball, jsLine_t* lines, int32_t numLines);

/**
 * @brief TODO doc
 *
 * @param scene
 */
void jsSimulate(jsScene_t* scene)
{
    for (int32_t i = 0; i < scene->numFlippers; i++)
    {
        jsFlipperSimulate(&scene->flippers[i], scene->dt);
    }

    for (int32_t i = 0; i < scene->numBalls; i++)
    {
        jsBall_t* ball = &scene->balls[i];
        jsBallSimulate(ball, scene->dt, scene->gravity);

        for (int32_t j = i + 1; j < scene->numBalls; j++)
        {
            jsBall_t* ball2 = &scene->balls[j];
            handleBallBallCollision(ball, ball2);
        }

        for (int32_t j = 0; j < scene->numObstacles; j++)
        {
            handleBallObstacleCollision(scene, ball, &scene->obstacles[j]);
        }

        for (int32_t j = 0; j < scene->numFlippers; j++)
        {
            handleBallFlipperCollision(ball, &scene->flippers[j]);
        }

        handleBallLineCollision(ball, scene->lines, scene->numLines);
    }

    for (int32_t i = 0; i < scene->numLaunchers; i++)
    {
        jsLauncherSimulate(&scene->launchers[i], scene->balls, scene->numBalls, scene->dt);
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
 * @param obstacle
 */
static void handleBallObstacleCollision(jsScene_t* scene, jsBall_t* ball, jsObstacle_t* obstacle)
{
    vecFl_t dir = subVecFl2d(ball->pos, obstacle->pos);
    float d     = magVecFl2d(dir);
    if (d == 0.0 || d > (ball->radius + obstacle->radius))
    {
        return;
    }

    // Normalize the direction
    dir = divVecFl2d(dir, d);

    // Move ball backwards to not clip
    float corr = ball->radius + obstacle->radius - d;
    ball->pos  = addVecFl2d(ball->pos, mulVecFl2d(dir, corr));

    // Adjust the velocity
    float v   = dotVecFl2d(ball->vel, dir);
    ball->vel = addVecFl2d(ball->vel, mulVecFl2d(dir, obstacle->pushVel - v));
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
 * @brief TODO doc
 *
 * @param ball
 * @param lines
 * @param numLines
 */
static void handleBallLineCollision(jsBall_t* ball, jsLine_t* lines, int32_t numLines)
{
    // find closest segment;
    vecFl_t ballToClosest;
    vecFl_t ab;
    vecFl_t normal;
    float minDist  = FLT_MAX;
    jsLine_t* line = NULL;

    // For each segment of the wall
    for (int32_t i = 0; i < numLines; i++)
    {
        if (lines[i].isSolid)
        {
            // Get the line segment from the list of walls
            vecFl_t a = lines[i].p1;
            vecFl_t b = lines[i].p2;
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
                line          = &lines[i];
            }
        }
    }

    // Check if there were any collisions
    if (NULL == line)
    {
        return;
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
    if (line->pushVel)
    {
        // Adjust the velocity
        ball->vel = addVecFl2d(ball->vel, mulVecFl2d(ballToClosest, line->pushVel - v));
    }
    else
    {
        // update velocity
        float vNew = ABS(v) * ball->restitution; // TODO care about wall's restitution?
        ball->vel  = addVecFl2d(ball->vel, mulVecFl2d(ballToClosest, vNew - v));
    }

    if (JS_DROP_TARGET == line->type)
    {
        line->isUp    = false;
        line->isSolid = false;
        // TODO check if all targets in the group are hit
    }
}
