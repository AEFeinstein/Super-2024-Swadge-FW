#include <stdbool.h>
#include <math.h>
#include <float.h>

#include "hdw-tft.h"
#include "geometryFl.h"
#include "shapes.h"
#include "macros.h"
#include "vectorFl2d.h"
#include "heatshrink_helper.h"
#include "pinballjs.h"

static vecFl_t closestPointOnSegment(vecFl_t p, vecFl_t a, vecFl_t b);
static void jsBallInit(jsBall_t* ball, float radius, float mass, vecFl_t pos, vecFl_t vel, float restitution);
static void jsBallSimulate(jsBall_t* ball, float dt, vecFl_t gravity);
static void jsObstacleInit(jsObstacle_t* obstacle, float radius, vecFl_t pos, float pushVel);
static void jsFlipperInit(jsFlipper_t* flipper, float radius, vecFl_t pos, float length, bool facingRight,
                          float restAngle, float maxRotation, float angularVelocity);
static void jsFlipperSimulate(jsFlipper_t* flipper, float dt);
static vecFl_t jsFlipperGetTip(jsFlipper_t* flipper);
static void handleBallBallCollision(jsBall_t* ball1, jsBall_t* ball2);
static void handleBallObstacleCollision(jsScene_t* scene, jsBall_t* ball, jsObstacle_t* obstacle);
static void handleBallFlipperCollision(jsBall_t* ball, jsFlipper_t* flipper);
static void handleBallLineCollision(jsBall_t* ball, jsLine_t* lines, int32_t numLines);
static void jsLauncherInit(jsLauncher_t* launcher, float x, float y, float w, float h);
static void jsLauncherSimulate(jsLauncher_t* launcher, jsBall_t* balls, int32_t numBalls, float dt);

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
 * @param facingRight
 * @param restAngle
 * @param maxRotation
 * @param angularVelocity
 */
static void jsFlipperInit(jsFlipper_t* flipper, float radius, vecFl_t pos, float length, bool facingRight,
                          float restAngle, float maxRotation, float angularVelocity)
{
    // fixed
    flipper->radius      = radius;
    flipper->pos         = pos;
    flipper->length      = length;
    flipper->facingRight = facingRight;
    if (!facingRight)
    {
        restAngle   = M_PI - restAngle;
        maxRotation = -maxRotation;
    }
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

static inline uint8_t readInt8(uint8_t* data, uint32_t* idx)
{
    return data[(*idx)++];
}

static inline uint16_t readInt16(uint8_t* data, uint32_t* idx)
{
    int16_t ret = (data[*idx] << 8) | (data[(*idx) + 1]);
    (*idx) += 2;
    return ret;
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

    uint32_t decompressedSize = 0;
    uint8_t* tableData        = (uint8_t*)readHeatshrinkFile("pinball.raw", &decompressedSize, true);
    uint32_t dIdx             = 0;

    uint8_t numGroups = readInt8(tableData, &dIdx);
    // TODO alloc groups

    uint16_t linesInFile = readInt16(tableData, &dIdx);
    scene->numLines      = 0;
    for (uint16_t lIdx = 0; lIdx < linesInFile; lIdx++)
    {
        jsLine_t* line = &scene->lines[scene->numLines++];
        uint16_t id    = readInt16(tableData, &dIdx);
        uint8_t gIdx   = readInt8(tableData, &dIdx);
        line->p1.x     = readInt16(tableData, &dIdx);
        line->p1.y     = readInt16(tableData, &dIdx);
        line->p2.x     = readInt16(tableData, &dIdx);
        line->p2.y     = readInt16(tableData, &dIdx);
        line->type     = readInt8(tableData, &dIdx);
        line->pushVel  = readInt8(tableData, &dIdx);
        line->isSolid  = readInt8(tableData, &dIdx);
        line->isUp     = true;
    }

    uint16_t circlesInFile = readInt16(tableData, &dIdx);
    scene->numObstacles    = 0;
    for (uint16_t cIdx = 0; cIdx < circlesInFile; cIdx++)
    {
        uint16_t id  = readInt16(tableData, &dIdx);
        uint8_t gIdx = readInt8(tableData, &dIdx);
        vecFl_t pos;
        pos.x           = readInt16(tableData, &dIdx);
        pos.y           = readInt16(tableData, &dIdx);
        uint8_t radius  = readInt8(tableData, &dIdx);
        uint8_t pushVel = readInt8(tableData, &dIdx);
        jsObstacleInit(&scene->obstacles[scene->numObstacles++], radius, pos, pushVel);
    }

    uint16_t rectanglesInFile = readInt16(tableData, &dIdx);
    scene->numLaunchers       = 0;
    for (uint16_t rIdx = 0; rIdx < rectanglesInFile; rIdx++)
    {
        uint16_t id    = readInt16(tableData, &dIdx);
        uint8_t gIdx   = readInt8(tableData, &dIdx);
        int16_t x      = readInt16(tableData, &dIdx);
        int16_t y      = readInt16(tableData, &dIdx);
        int16_t width  = readInt16(tableData, &dIdx);
        int16_t height = readInt16(tableData, &dIdx);

        jsLauncherInit(&scene->launchers[scene->numLaunchers++], x, y, width, height);
    }

    uint16_t flippersInFile = readInt16(tableData, &dIdx);
    scene->numFlippers      = 0;
    for (uint16_t fIdx = 0; fIdx < flippersInFile; fIdx++)
    {
        vecFl_t pos;
        pos.x            = readInt16(tableData, &dIdx);
        pos.y            = readInt16(tableData, &dIdx);
        uint8_t radius   = readInt8(tableData, &dIdx);
        uint8_t length   = readInt8(tableData, &dIdx);
        bool facingRight = readInt8(tableData, &dIdx) != 0;

        float maxRotation     = 1.0f;
        float restAngle       = -0.5f;
        float angularVelocity = 10.0f;

        jsFlipperInit(&scene->flippers[scene->numFlippers++], radius, pos, length, facingRight, -restAngle, maxRotation,
                      angularVelocity);
    }

    free(tableData);

    // TODO load ball position from file
    float radius = 4.0f;
    vecFl_t pos  = {.x = 274.0f, .y = 234.0f};
    // vecFl_t pos     = {.x = 48.0f, .y = 140.0f};
    vecFl_t vel     = {.x = 0.0f, .y = 0.0f};
    scene->numBalls = 0;
    jsBallInit(&scene->balls[scene->numBalls++], radius, M_PI * radius * radius, pos, vel, 0.2f);

    // pos.x = 160.0f;
    // pos.y = 60.0f;
    // vel.x = 0.0f;
    // vel.y = -20.0f;
    // jsBallInit(&scene->balls[scene->numBalls++], radius, M_PI * radius * radius, pos, vel, 0.2f);
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

        handleBallLineCollision(ball, scene->lines, scene->numLines);
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

    // LInes
    for (int32_t i = 0; i < scene->numLines; i++)
    {
        vecFl_t* p1          = &scene->lines[i].p1;
        vecFl_t* p2          = &scene->lines[i].p2;
        paletteColor_t color = c555;
        switch (scene->lines[i].type)
        {
            case JS_WALL:
            {
                color = c555;
                break;
            }
            case JS_SLINGSHOT:
            {
                color = c500;
                break;
            }
            case JS_DROP_TARGET:
            {
                if (scene->lines[i].isUp)
                {
                    color = c050;
                }
                else
                {
                    color = c010;
                };
                break;
            }
            case JS_STANDUP_TARGET:
            {
                color = c004;
                break;
            }
            case JS_SPINNER:
            {
                color = c123;
                break;
            }
        }

        drawLineFast(p1->x, p1->y, p2->x, p2->y, color);
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
                for (int32_t fIdx = 0; fIdx < scene->numFlippers; fIdx++)
                {
                    if (scene->flippers[fIdx].facingRight)
                    {
                        scene->flippers[fIdx].buttonHeld = true;
                    }
                }
                break;
            }
            case PB_RIGHT:
            {
                for (int32_t fIdx = 0; fIdx < scene->numFlippers; fIdx++)
                {
                    if (!scene->flippers[fIdx].facingRight)
                    {
                        scene->flippers[fIdx].buttonHeld = true;
                    }
                }
                for (int32_t rIdx = 0; rIdx < scene->numLaunchers; rIdx++)
                {
                    scene->launchers[rIdx].buttonHeld = true;
                }
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
                for (int32_t fIdx = 0; fIdx < scene->numFlippers; fIdx++)
                {
                    if (scene->flippers[fIdx].facingRight)
                    {
                        scene->flippers[fIdx].buttonHeld = false;
                    }
                }
                break;
            }
            case PB_RIGHT:
            {
                for (int32_t fIdx = 0; fIdx < scene->numFlippers; fIdx++)
                {
                    if (!scene->flippers[fIdx].facingRight)
                    {
                        scene->flippers[fIdx].buttonHeld = false;
                    }
                }
                for (int32_t rIdx = 0; rIdx < scene->numLaunchers; rIdx++)
                {
                    scene->launchers[rIdx].buttonHeld = false;
                }
                break;
            }
            default:
            {
                break;
            }
        }
    }
}
