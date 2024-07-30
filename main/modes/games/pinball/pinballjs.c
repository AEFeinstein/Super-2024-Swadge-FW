#include <stdbool.h>
#include <math.h>
#include <float.h>

#include "hdw-tft.h"
#include "geometryFl.h"
#include "shapes.h"
#include "macros.h"
#include "vectorFl2d.h"
#include "pinballjs.h"

static vecFl_t closestPointOnSegment(vecFl_t p, vecFl_t a, vecFl_t b);
static void jsBallInit(jsBall_t* ball, float radius, float mass, vecFl_t pos, vecFl_t vel, float restitution);
static void jsBallSimulate(jsBall_t* ball, float dt, vecFl_t gravity);
static void jsObstacleInit(jsObstacle_t* obstacle, float radius, vecFl_t pos, float pushVel);
static void jsFlipperInit(jsFlipper_t* flipper, float radius, vecFl_t pos, float length, float restAngle,
                          float maxRotation, float angularVelocity);
static void jsFlipperSimulate(jsFlipper_t* flipper, float dt);
static vecFl_t jsFlipperGetTip(jsFlipper_t* flipper);
static void handleBallBallCollision(jsBall_t* ball1, jsBall_t* ball2);
static void handleBallObstacleCollision(jsScene_t* scene, jsBall_t* ball, jsObstacle_t* obstacle);
static void handleBallFlipperCollision(jsBall_t* ball, jsFlipper_t* flipper);
static void handleBallWallCollision(jsBall_t* ball, jsLine_t* walls, int32_t numWalls, jsLine_t* straightBumpers,
                                    int32_t numStraightBumpers);
static void jsLauncherInit(jsLauncher_t* launcher, float x, float y, float w, float h);
static void jsLauncherSimulate(jsLauncher_t* launcher, jsBall_t* balls, int32_t numBalls, float dt);

static const jsLine_t constWalls[] = {
    {.p1 = {.x = 95, .y = 190}, .p2 = {.x = 0, .y = 116}},    {.p1 = {.x = 184, .y = 190}, .p2 = {.x = 261, .y = 129}},
    {.p1 = {.x = 0, .y = 0}, .p2 = {.x = 279, .y = 0}},       {.p1 = {.x = 0, .y = 239}, .p2 = {.x = 0, .y = 0}},
    {.p1 = {.x = 279, .y = 0}, .p2 = {.x = 279, .y = 239}},   {.p1 = {.x = 0, .y = 239}, .p2 = {.x = 279, .y = 239}},
    {.p1 = {.x = 0, .y = 116}, .p2 = {.x = 3, .y = 97}},      {.p1 = {.x = 3, .y = 97}, .p2 = {.x = 9, .y = 79}},
    {.p1 = {.x = 9, .y = 79}, .p2 = {.x = 20, .y = 58}},      {.p1 = {.x = 20, .y = 58}, .p2 = {.x = 37, .y = 37}},
    {.p1 = {.x = 37, .y = 37}, .p2 = {.x = 62, .y = 18}},     {.p1 = {.x = 62, .y = 18}, .p2 = {.x = 78, .y = 11}},
    {.p1 = {.x = 78, .y = 11}, .p2 = {.x = 96, .y = 5}},      {.p1 = {.x = 96, .y = 5}, .p2 = {.x = 116, .y = 1}},
    {.p1 = {.x = 116, .y = 1}, .p2 = {.x = 140, .y = 0}},     {.p1 = {.x = 140, .y = 0}, .p2 = {.x = 163, .y = 1}},
    {.p1 = {.x = 163, .y = 1}, .p2 = {.x = 183, .y = 5}},     {.p1 = {.x = 183, .y = 5}, .p2 = {.x = 201, .y = 11}},
    {.p1 = {.x = 201, .y = 11}, .p2 = {.x = 217, .y = 18}},   {.p1 = {.x = 217, .y = 18}, .p2 = {.x = 242, .y = 37}},
    {.p1 = {.x = 242, .y = 37}, .p2 = {.x = 259, .y = 58}},   {.p1 = {.x = 259, .y = 58}, .p2 = {.x = 270, .y = 79}},
    {.p1 = {.x = 270, .y = 79}, .p2 = {.x = 276, .y = 97}},   {.p1 = {.x = 276, .y = 97}, .p2 = {.x = 279, .y = 116}},
    {.p1 = {.x = 261, .y = 129}, .p2 = {.x = 261, .y = 239}}, {.p1 = {.x = 219, .y = 143}, .p2 = {.x = 190, .y = 163}},
    {.p1 = {.x = 219, .y = 107}, .p2 = {.x = 219, .y = 143}}, {.p1 = {.x = 60, .y = 143}, .p2 = {.x = 89, .y = 163}},
    {.p1 = {.x = 60, .y = 107}, .p2 = {.x = 60, .y = 143}},
};

static const jsLine_t constStraightBumpers[] = {
    {.p1 = {.x = 190, .y = 163}, .p2 = {.x = 219, .y = 107}},
    {.p1 = {.x = 89, .y = 163}, .p2 = {.x = 60, .y = 107}},
};

// physics scene -------------------------------------------------------

/**
 * @brief Initialize a ball
 *
 * @param ball
 * @param radius
 * @param mass
 * @param pos
 * @param vel
 * @param restitution
 */
static void jsBallInit(jsBall_t* ball, float radius, float mass, vecFl_t pos, vecFl_t vel, float restitution)
{
    ball->radius      = radius;
    ball->mass        = mass;
    ball->pos         = pos;
    ball->vel         = vel;
    ball->restitution = restitution;
}

/**
 * @brief Simulate a ball's motion
 *
 * @param ball
 * @param dt
 * @param gravity
 */
static void jsBallSimulate(jsBall_t* ball, float dt, vecFl_t gravity)
{
    ball->vel = addVecFl2d(ball->vel, mulVecFl2d(gravity, dt));
    ball->pos = addVecFl2d(ball->pos, mulVecFl2d(ball->vel, dt));
}

/**
 * @brief TODO doc
 *
 * @param obstacle
 * @param radius
 * @param pos
 * @param pushVel
 */
static void jsObstacleInit(jsObstacle_t* obstacle, float radius, vecFl_t pos, float pushVel)
{
    obstacle->radius  = radius;
    obstacle->pos     = pos;
    obstacle->pushVel = pushVel;
}

/**
 * @brief TODO doc
 *
 * @param flipper
 * @param radius
 * @param pos
 * @param length
 * @param restAngle
 * @param maxRotation
 * @param angularVelocity
 */
static void jsFlipperInit(jsFlipper_t* flipper, float radius, vecFl_t pos, float length, float restAngle,
                          float maxRotation, float angularVelocity)
{
    // fixed
    flipper->radius          = radius;
    flipper->pos             = pos;
    flipper->length          = length;
    flipper->restAngle       = restAngle;
    flipper->maxRotation     = ABS(maxRotation);
    flipper->sign            = (maxRotation >= 0) ? -1 : 1;
    flipper->angularVelocity = angularVelocity;
    // changing
    flipper->rotation               = 0;
    flipper->currentAngularVelocity = 0;
    flipper->buttonHeld             = false;
}

/**
 * @brief TODO doc
 *
 * @param flipper
 * @param dt
 */
static void jsFlipperSimulate(jsFlipper_t* flipper, float dt)
{
    float prevRotation = flipper->rotation;

    if (flipper->buttonHeld)
    {
        flipper->rotation = flipper->rotation + dt * flipper->angularVelocity;
        if (flipper->rotation > flipper->maxRotation)
        {
            flipper->rotation = flipper->maxRotation;
        }
    }
    else
    {
        flipper->rotation = flipper->rotation - dt * flipper->angularVelocity;
        if (flipper->rotation < 0)
        {
            flipper->rotation = 0;
        }
    }
    flipper->currentAngularVelocity = flipper->sign * (flipper->rotation - prevRotation) / dt;
}

/**
 * @brief TODO doc
 *
 * @param flipper
 * @return vecFl_t
 */
static vecFl_t jsFlipperGetTip(jsFlipper_t* flipper)
{
    float angle = flipper->restAngle + flipper->sign * flipper->rotation;
    vecFl_t dir = {.x = cosf(angle), .y = sinf(angle)};
    return addVecFl2d(flipper->pos, mulVecFl2d(dir, flipper->length));
}

/**
 * @brief TODO doc
 *
 * @param launcher
 * @param x
 * @param y
 * @param w
 * @param h
 */
static void jsLauncherInit(jsLauncher_t* launcher, float x, float y, float w, float h)
{
    launcher->pos.x      = x;
    launcher->pos.y      = y;
    launcher->width      = w;
    launcher->height     = h;
    launcher->buttonHeld = false;
    launcher->impulse    = 0;
}

/**
 * @brief TODO doc
 *
 * @param launcher
 * @param balls
 * @param numBalls
 * @param dt
 */
static void jsLauncherSimulate(jsLauncher_t* launcher, jsBall_t* balls, int32_t numBalls, float dt)
{
    if (launcher->buttonHeld)
    {
        launcher->impulse += (200.0f * dt);
    }
    else
    {
        rectangleFl_t r = {.pos = launcher->pos, .width = launcher->width, .height = launcher->height};
        // If touching a ball, transfer to a ball
        for (int32_t bIdx = 0; bIdx < numBalls; bIdx++)
        {
            circleFl_t b = {.pos = balls[bIdx].pos, .radius = balls[bIdx].radius};
            if (circleRectFlIntersection(b, r, NULL))
            {
                balls[bIdx].vel.y -= launcher->impulse;
            }
        }
        launcher->impulse = 0;
    }
}

/**
 * @brief TODO doc
 *
 * @param scene
 */
void jsSceneInit(jsScene_t* scene)
{
    scene->gravity.x = 0;
    scene->gravity.y = 30;
    scene->dt        = 1 / 60.0f;
    scene->score     = 0;
    scene->paused    = true;

    scene->numWalls = 0;
    for (int32_t wIdx = 0; wIdx < ARRAY_SIZE(constWalls); wIdx++)
    {
        scene->walls[scene->numWalls]           = constWalls[wIdx];
        scene->walls[scene->numWalls++].pushVel = 0;
    }
    // balls

    float radius = 4.0f;
    // vecFl_t pos  = {.x = 274.0f, .y = 234.0f};
    vecFl_t pos     = {.x = 120.0f, .y = 60.0f};
    vecFl_t vel     = {.x = 0.0f, .y = 0.0f};
    scene->numBalls = 0;
    jsBallInit(&scene->balls[scene->numBalls++], radius, M_PI * radius * radius, pos, vel, 0.2f);

    // pos.x = 160.0f;
    // pos.y = 60.0f;
    // vel.x = 0.0f;
    // vel.y = -20.0f;
    // jsBallInit(&scene->balls[scene->numBalls++], radius, M_PI * radius * radius, pos, vel, 0.2f);

    // obstacles

    scene->numStraightBumpers = 0;
    for (int32_t bIdx = 0; bIdx < ARRAY_SIZE(constStraightBumpers); bIdx++)
    {
        scene->straightBumpers[scene->numStraightBumpers]           = constStraightBumpers[bIdx];
        scene->straightBumpers[scene->numStraightBumpers++].pushVel = 80.0f;
    }

    pos.x               = 130.0f;
    pos.y               = 120.0f;
    scene->numObstacles = 0;
    jsObstacleInit(&scene->obstacles[scene->numObstacles++], 10.0f, pos, 40.0f);
#if 0
    pos.x = 0.75f;
    pos.y = 0.5f;
    jsObstacleInit(&scene->obstacles[scene->numObstacles++], 0.1f, pos, 2.0f);
    pos.x = 0.7f;
    pos.y = 1.0f;
    jsObstacleInit(&scene->obstacles[scene->numObstacles++], 0.12f, pos, 2.0f);
    pos.x = 0.2f;
    pos.y = 1.2f;
    jsObstacleInit(&scene->obstacles[scene->numObstacles++], 0.1f, pos, 2.0f);
#endif

    // flippers

    radius                = 5.0f;
    float length          = 40.0f;
    float maxRotation     = 1.0f;
    float restAngle       = -0.5f;
    float angularVelocity = 10.0f;

    vecFl_t pos1 = {.x = 94.0f, .y = 196.0f};
    vecFl_t pos2 = {.x = 185.0f, .y = 196.0f};

    scene->numFlippers = 0;
    jsFlipperInit(&scene->flippers[scene->numFlippers++], radius, pos1, length, -restAngle, maxRotation,
                  angularVelocity);
    jsFlipperInit(&scene->flippers[scene->numFlippers++], radius, pos2, length, M_PI + restAngle, -maxRotation,
                  angularVelocity);

    static const rectangleFl_t constLaunchers[] = {
        {.pos = {.x = 261, .y = 231}, .width = 19, .height = 9},
    };
    scene->numLaunchers = 0;
    for (int32_t lIdx = 0; lIdx < ARRAY_SIZE(constLaunchers); lIdx++)
    {
        jsLauncherInit(&scene->launchers[scene->numLaunchers++], constLaunchers[lIdx].pos.x, constLaunchers[lIdx].pos.y,
                       constLaunchers[lIdx].width, constLaunchers[lIdx].height);
    }
}

// --- collision handling -------------------------------------------------------

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
 * @param walls
 * @param numWalls
 * @param straightBumpers
 * @param numStraightBumpers
 */
static void handleBallWallCollision(jsBall_t* ball, jsLine_t* walls, int32_t numWalls, jsLine_t* straightBumpers,
                                    int32_t numStraightBumpers)
{
    // find closest segment;
    vecFl_t ballToClosest;
    vecFl_t ab;
    vecFl_t normal;
    float minDist = FLT_MAX;
    float pushVel = 0;

    // For each segment of the wall
    for (int32_t i = 0; i < numWalls; i++)
    {
        // Get the line segment from the list of walls
        vecFl_t a = walls[i].p1;
        vecFl_t b = walls[i].p2;
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
            pushVel       = walls[i].pushVel;
        }
    }

    for (int32_t i = 0; i < numStraightBumpers; i++)
    {
        // Get the line segment from the list of walls
        vecFl_t a = straightBumpers[i].p1;
        vecFl_t b = straightBumpers[i].p2;
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
            pushVel       = straightBumpers[i].pushVel;
        }
    }

    // Check if there were any collisions
    if (FLT_MAX == minDist)
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
    if (pushVel)
    {
        // Adjust the velocity
        ball->vel = addVecFl2d(ball->vel, mulVecFl2d(ballToClosest, pushVel - v));
    }
    else
    {
        // update velocity
        float vNew = ABS(v) * ball->restitution; // TODO care about wall's restitution?
        ball->vel  = addVecFl2d(ball->vel, mulVecFl2d(ballToClosest, vNew - v));
    }
}

// simulation -------------------------------------------------------

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

        handleBallWallCollision(ball, scene->walls, scene->numWalls, scene->straightBumpers, scene->numStraightBumpers);
    }

    for (int32_t i = 0; i < scene->numLaunchers; i++)
    {
        jsLauncherSimulate(&scene->launchers[i], scene->balls, scene->numBalls, scene->dt);
    }
}

// draw -------------------------------------------------------

/**
 * @brief TODO doc
 *
 * @param scene
 */
void jsSceneDraw(jsScene_t* scene)
{
    clearPxTft();

    // wall
    for (int32_t i = 0; i < scene->numWalls; i++)
    {
        vecFl_t* p1 = &scene->walls[i].p1;
        vecFl_t* p2 = &scene->walls[i].p2;
        drawLineFast(p1->x, p1->y, p2->x, p2->y, c555);
    }

    // balls
    for (int32_t i = 0; i < scene->numBalls; i++)
    {
        vecFl_t* pos = &scene->balls[i].pos;
        drawCircleFilled(pos->x, pos->y, scene->balls[i].radius, c500);
    }

    // obstacles
    for (int32_t i = 0; i < scene->numObstacles; i++)
    {
        vecFl_t* pos = &scene->obstacles[i].pos;
        drawCircleFilled(pos->x, pos->y, scene->obstacles[i].radius, c131);
    }

    for (int32_t i = 0; i < scene->numWalls; i++)
    {
        vecFl_t* p1 = &scene->straightBumpers[i].p1;
        vecFl_t* p2 = &scene->straightBumpers[i].p2;
        drawLineFast(p1->x, p1->y, p2->x, p2->y, c500);
    }

    // flippers
    for (int32_t i = 0; i < scene->numFlippers; i++)
    {
        jsFlipper_t* flipper = &scene->flippers[i];
        vecFl_t pos          = flipper->pos;
        drawCircleFilled(pos.x, pos.y, flipper->radius, c115);
        vecFl_t tip = jsFlipperGetTip(flipper);
        drawCircleFilled(tip.x, tip.y, flipper->radius, c115);
        drawLineFast(pos.x, pos.y + flipper->radius, tip.x, tip.y + flipper->radius, c115);
        drawLineFast(pos.x, pos.y - flipper->radius, tip.x, tip.y - flipper->radius, c115);
    }

    // launchers
    for (int32_t i = 0; i < scene->numLaunchers; i++)
    {
        jsLauncher_t* l = &scene->launchers[i];
        drawRect(l->pos.x, l->pos.y, l->pos.x + l->width, l->pos.y + l->height, c330);
    }
}

// ------------------------ user interaction ---------------------------

/**
 * @brief TODO doc
 *
 * @param scene
 * @param event
 */
void jsButtonPressed(jsScene_t* scene, buttonEvt_t* event)
{
    if (event->down)
    {
        switch (event->button)
        {
            case PB_LEFT:
            {
                scene->flippers[0].buttonHeld = true;
                break;
            }
            case PB_RIGHT:
            {
                scene->flippers[1].buttonHeld  = true;
                scene->launchers[0].buttonHeld = true;
                break;
            }
            default:
            {
                break;
            }
        }
    }
    else
    {
        switch (event->button)
        {
            case PB_LEFT:
            {
                scene->flippers[0].buttonHeld = false;
                break;
            }
            case PB_RIGHT:
            {
                scene->flippers[1].buttonHeld  = false;
                scene->launchers[0].buttonHeld = false;
                break;
            }
            default:
            {
                break;
            }
        }
    }
}
