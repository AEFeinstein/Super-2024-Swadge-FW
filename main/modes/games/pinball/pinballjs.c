#include <stdbool.h>
#include <math.h>

#include "hdw-tft.h"
#include "shapes.h"
#include "macros.h"
#include "vectorFl2d.h"
#include "pinballjs.h"

static vecFl_t closestPointOnSegment(vecFl_t p, vecFl_t a, vecFl_t b);
static void jsBallInit(jsBall_t* ball, float radius, float mass, vecFl_t pos, vecFl_t vel, float restitution);
static void jsBallSimulate(jsBall_t* ball, float dt, vecFl_t gravity);
static void jsObstacleInit(jsObstacle_t* obstacle, float radius, vecFl_t pos, float pushVel);
static void jsFlipperInit(jsFlipper_t* flipper, float radius, vecFl_t pos, float length, float restAngle,
                          float maxRotation, float angularVelocity, float restitution);
static void jsFlipperSimulate(jsFlipper_t* flipper, float dt);
static vecFl_t jsFlipperGetTip(jsFlipper_t* flipper);
static void handleBallBallCollision(jsBall_t* ball1, jsBall_t* ball2);
static void handleBallObstacleCollision(jsScene_t* scene, jsBall_t* ball, jsObstacle_t* obstacle);
static void handleBallFlipperCollision(jsBall_t* ball, jsFlipper_t* flipper);
static void handleBallBorderCollision(jsBall_t* ball, vecFl_t* border, int32_t numBorders);
static int32_t cX(jsScene_t* scene, vecFl_t pos);
static int32_t cY(jsScene_t* scene, vecFl_t pos);

// ----------------------------------------------------------------------

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
    // ab.subtractVectors(b, a);
    vecFl_t ab = subVecFl2d(b, a);
    // var t = ab.dot(ab);
    float t = dotVecFl2d(ab, ab);

    // if (t == 0.0)
    //     return a.clone();
    if (t == 0.0f)
    {
        return a;
    }

    // t = Math.max(0.0, Math.min(1.0, (p.dot(ab) - a.dot(ab)) / t));
    t = (dotVecFl2d(p, ab) - dotVecFl2d(a, ab)) / t;
    if (t > 1)
    {
        t = 1;
    }
    else if (t < 0)
    {
        t = 0;
    }

    // var closest = a.clone();
    // return closest.add(ab, t);
    return addVecFl2d(a, mulVecFl2d(ab, t));
}

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
 * @param restitution
 */
static void jsFlipperInit(jsFlipper_t* flipper, float radius, vecFl_t pos, float length, float restAngle,
                          float maxRotation, float angularVelocity, float restitution)
{
    // fixed
    flipper->radius          = radius;
    flipper->pos             = pos;
    flipper->length          = length;
    flipper->restAngle       = restAngle;
    flipper->maxRotation     = ABS(maxRotation);
    flipper->sign            = (maxRotation >= 0) ? 1 : -1;
    flipper->angularVelocity = angularVelocity;
    // changing
    flipper->rotation               = 0;
    flipper->currentAngularVelocity = 0;
    flipper->touchIdentifier        = -1;
}

/**
 * @brief TODO doc
 *
 * @param flipper
 * @param dt
 */
static void jsFlipperSimulate(jsFlipper_t* flipper, float dt)
{
    // var prevRotation = this.rotation;
    float prevRotation = flipper->rotation;
    // var pressed = this.touchIdentifier >= 0;
    bool pressed = flipper->touchIdentifier >= 0;

    // if (pressed)
    if (pressed)
    {
        // this.rotation = Math.min(this.rotation + dt * this.angularVelocity,
        //     this.maxRotation);
        flipper->rotation = flipper->rotation + dt * flipper->angularVelocity;
        if (flipper->rotation > flipper->maxRotation)
        {
            flipper->rotation = flipper->maxRotation;
        }
    }
    // else
    else
    {
        // this.rotation = Math.max(this.rotation - dt * this.angularVelocity,
        //     0.0);
        flipper->rotation = flipper->rotation - dt * flipper->angularVelocity;
        if (flipper->rotation < 0)
        {
            flipper->rotation = 0;
        }
    }
    // this.currentAngularVelocity = this.sign * (this.rotation - prevRotation) / dt;
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
    // var angle = this.restAngle + this.sign * this.rotation;
    float angle = flipper->restAngle + flipper->sign * flipper->rotation;
    // var dir = new Vector2(Math.cos(angle), Math.sin(angle));
    vecFl_t dir = {.x = cosf(angle), .y = sinf(angle)};
    // var tip = this.pos.clone();
    // return tip.add(dir, this.length);
    return addVecFl2d(flipper->pos, mulVecFl2d(dir, flipper->length));
}

/**
 * @brief TODO doc
 *
 * @param scene
 */
void jsSceneInit(jsScene_t* scene)
{
    // drawing setup -------------------------------------------------------
    float flipperHeight = 1.7f;
    scene->cScale       = TFT_HEIGHT / flipperHeight;
    float offset        = 0.02;

    scene->gravity.x = 0;
    scene->gravity.y = -3;
    scene->dt        = 1 / 60.0f;
    scene->score     = 0;
    scene->paused    = true;

    // borders
    vecFl_t borders[] = {
        {.x = 0.74f, .y = 0.25f},
        {.x = 1.0f - offset, .y = 0.4f},
        {.x = 1.0f - offset, .y = flipperHeight - offset},
        {.x = offset, .y = flipperHeight - offset},
        {.x = offset, .y = 0.4f},
        {.x = 0.26f, .y = 0.25f},
        {.x = 0.26f, .y = 0.0f},
        {.x = 0.74f, .y = 0.0f},
    };

    scene->numBorders = 0;
    for (int32_t wIdx = 0; wIdx < ARRAY_SIZE(borders); wIdx++)
    {
        scene->border[scene->numBorders++] = borders[wIdx];
    }

    // balls

    float radius = 0.03f;
    vecFl_t pos  = {.x = 0.92f, .y = 0.5f};
    vecFl_t vel  = {.x = -0.2f, .y = 3.5f};
    jsBallInit(&scene->balls[scene->numBalls++], radius, M_PI * radius * radius, pos, vel, 0.2f);

    pos.x = 0.08f;
    pos.y = 0.5f;
    vel.x = 0.2f;
    vel.y = 3.5f;
    jsBallInit(&scene->balls[scene->numBalls++], radius, M_PI * radius * radius, pos, vel, 0.2f);

    // obstacles

    pos.x = 0.25f;
    pos.y = 0.6f;
    jsObstacleInit(&scene->obstacles[scene->numObstacles++], 0.1f, pos, 2.0f);
    pos.x = 0.75f;
    pos.y = 0.5f;
    jsObstacleInit(&scene->obstacles[scene->numObstacles++], 0.1f, pos, 2.0f);
    pos.x = 0.7f;
    pos.y = 1.0f;
    jsObstacleInit(&scene->obstacles[scene->numObstacles++], 0.12f, pos, 2.0f);
    pos.x = 0.2f;
    pos.y = 1.2f;
    jsObstacleInit(&scene->obstacles[scene->numObstacles++], 0.1f, pos, 2.0f);

    // flippers

    radius                = 0.03f;
    float length          = 0.2f;
    float maxRotation     = 1.0f;
    float restAngle       = 0.5f;
    float angularVelocity = 10.0f;
    float restitution     = 0.0f;

    vecFl_t pos1 = {.x = 0.26f, .y = 0.22f};
    vecFl_t pos2 = {.x = 0.74f, .y = 0.22f};

    jsFlipperInit(&scene->flippers[scene->numFlippers++], radius, pos1, length, -restAngle, maxRotation,
                  angularVelocity, restitution);
    jsFlipperInit(&scene->flippers[scene->numFlippers++], radius, pos2, length, M_PI + restAngle, -maxRotation,
                  angularVelocity, restitution);
}

// --- collision handling -------------------------------------------------------

/**
 * @brief TODO doc
 *
 * @param ball1
 * @param ball2
 */
static void handleBallBallCollision(jsBall_t* ball1, jsBall_t* ball2)
{
    // var restitution = Math.min(ball1.restitution, ball2.restitution);
    float restitution = MIN(ball1->restitution, ball2->restitution);
    // var dir = new Vector2();
    // dir.subtractVectors(ball2.pos, ball1.pos);
    vecFl_t dir = subVecFl2d(ball2->pos, ball1->pos);
    // var d = dir.length();
    float d = magVecFl2d(dir);
    // if (d == 0.0 || d > ball1.radius + ball2.radius)
    if (0 == d || d > (ball1->radius + ball2->radius))
    {
        // return;
        return;
    }

    // dir.scale(1.0 / d);
    dir = divVecFl2d(dir, d);

    // var corr = (ball1.radius + ball2.radius - d) / 2.0;
    float corr = (ball1->radius + ball2->radius - d) / 2.0f;
    // ball1.pos.add(dir, -corr);
    ball1->pos = addVecFl2d(ball1->pos, mulVecFl2d(dir, -corr));
    // ball2.pos.add(dir, corr);
    ball2->pos = addVecFl2d(ball2->pos, mulVecFl2d(dir, corr));

    // var v1 = ball1.vel.dot(dir);
    float v1 = dotVecFl2d(ball1->vel, dir);
    // var v2 = ball2.vel.dot(dir);
    float v2 = dotVecFl2d(ball2->vel, dir);

    // var m1 = ball1.mass;
    float m1 = ball1->mass;
    // var m2 = ball2.mass;
    float m2 = ball2->mass;

    // var newV1 = (m1 * v1 + m2 * v2 - m2 * (v1 - v2) * restitution) / (m1 + m2);
    float newV1 = (m1 * v1 + m2 * v2 - m2 * (v1 - v2) * restitution) / (m1 + m2);
    // var newV2 = (m1 * v1 + m2 * v2 - m1 * (v2 - v1) * restitution) / (m1 + m2);
    float newV2 = (m1 * v1 + m2 * v2 - m1 * (v2 - v1) * restitution) / (m1 + m2);

    // ball1.vel.add(dir, newV1 - v1);
    ball1->vel = addVecFl2d(ball1->vel, mulVecFl2d(dir, newV1 - v1));
    // ball2.vel.add(dir, newV2 - v2);
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
    // var dir = new Vector2();
    // dir.subtractVectors(ball.pos, obstacle.pos);
    vecFl_t dir = subVecFl2d(ball->pos, obstacle->pos);
    // var d = dir.length();
    float d = magVecFl2d(dir);
    // if (d == 0.0 || d > ball.radius + obstacle.radius)
    if (d == 0.0 || d > (ball->radius + obstacle->radius))
    {
        // return;
        return;
    }

    // dir.scale(1.0 / d);
    dir = divVecFl2d(dir, d);

    // var corr = ball.radius + obstacle.radius - d;
    float corr = ball->radius + obstacle->radius - d;
    // ball.pos.add(dir, corr);
    ball->pos = addVecFl2d(ball->pos, mulVecFl2d(dir, corr));

    // var v = ball.vel.dot(dir);
    float v = dotVecFl2d(ball->vel, dir);
    // ball.vel.add(dir, obstacle.pushVel - v);
    ball->vel = addVecFl2d(ball->vel, mulVecFl2d(dir, obstacle->pushVel - v));

    // physicsScene.score++;
    scene->score++;
}

/**
 * @brief TODO doc
 *
 * @param ball
 * @param flipper
 */
static void handleBallFlipperCollision(jsBall_t* ball, jsFlipper_t* flipper)
{
    // var closest = closestPointOnSegment(ball.pos, flipper.pos, flipper.getTip());
    vecFl_t closest = closestPointOnSegment(ball->pos, flipper->pos, jsFlipperGetTip(flipper));
    // var dir = new Vector2();
    // dir.subtractVectors(ball.pos, closest);
    vecFl_t dir = subVecFl2d(ball->pos, closest);
    // var d = dir.length();
    float d = magVecFl2d(dir);
    // if (d == 0.0 || d > ball.radius + flipper.radius)
    if (d == 0.0 || d > ball->radius + flipper->radius)
        // return;
        return;

    // dir.scale(1.0 / d);
    dir = divVecFl2d(dir, d);

    // var corr = (ball.radius + flipper.radius - d);
    float corr = (ball->radius + flipper->radius - d);
    // ball.pos.add(dir, corr);
    ball->pos = addVecFl2d(ball->pos, mulVecFl2d(dir, corr));

    // update velocity

    // var radius = closest.clone();
    vecFl_t radius = closest;
    // radius.add(dir, flipper.radius);
    radius = addVecFl2d(radius, mulVecFl2d(dir, flipper->radius));
    // radius.subtract(flipper.pos);
    radius = subVecFl2d(radius, flipper->pos);
    // var surfaceVel = radius.perp();
    vecFl_t surfaceVel = perpendicularVecFl2d(radius);
    // surfaceVel.scale(flipper.currentAngularVelocity);
    surfaceVel = mulVecFl2d(surfaceVel, flipper->currentAngularVelocity);

    // var v = ball.vel.dot(dir);
    float v = dotVecFl2d(ball->vel, dir);
    // var vNew = surfaceVel.dot(dir);
    float vNew = dotVecFl2d(surfaceVel, dir);

    // ball.vel.add(dir, vNew - v);
    ball->vel = addVecFl2d(ball->vel, mulVecFl2d(dir, vNew - v));
}

/**
 * @brief TODO doc
 *
 * @param ball
 * @param border
 * @param numBorders
 */
static void handleBallBorderCollision(jsBall_t* ball, vecFl_t* border, int32_t numBorders)
{
    // if (border.length < 3)
    //     return;
    if (numBorders < 3)
    {
        return;
    }

    // find closest segment;

    // var d = new Vector2();
    vecFl_t d;
    // var closest = new Vector2();
    vecFl_t closest;
    // var ab = new Vector2();
    vecFl_t ab;
    // var normal;
    vecFl_t normal;

    // var minDist = 0.0;
    float minDist = 0;

    // for (var i = 0; i < border.length; i++) {
    for (int32_t i = 0; i < numBorders; i++)
    {
        // var a = border[i];
        vecFl_t a = border[i];
        // var b = border[(i + 1) % border.length];
        vecFl_t b = border[(i + 1) % numBorders];
        // var c = closestPointOnSegment(ball.pos, a, b);
        vecFl_t c = closestPointOnSegment(ball->pos, a, b);
        // d.subtractVectors(ball.pos, c);
        d = subVecFl2d(ball->pos, c);
        // var dist = d.length();
        float dist = magVecFl2d(d);
        // if (i == 0 || dist < minDist) {
        if (i == 0 || dist < minDist)
        {
            // minDist = dist;
            minDist = dist;
            // closest.set(c);
            closest = c;
            // ab.subtractVectors(b, a);
            ab = subVecFl2d(b, a);
            // normal = ab.perp();
            normal = perpendicularVecFl2d(ab);
        }
    }

    // push out
    // d.subtractVectors(ball.pos, closest);
    d = subVecFl2d(ball->pos, closest);
    // var dist = d.length();
    float dist = magVecFl2d(d);
    // if (dist == 0.0) {
    if (0 == dist)
    {
        // d.set(normal);
        d = normal;
        // dist = normal.length();
        dist = magVecFl2d(normal);
    }
    // d.scale(1.0 / dist);
    d = divVecFl2d(d, dist);

    // if (d.dot(normal) >= 0.0) {
    if (dotVecFl2d(d, normal) >= 0)
    {
        // if (dist > ball.radius)
        if (dist > ball->radius)
        {
            // return;
            return;
        }
        // ball.pos.add(d, ball.radius - dist);
        ball->pos = addVecFl2d(ball->pos, mulVecFl2d(d, ball->radius - dist));
    }
    // else
    else
    {
        // ball.pos.add(d, -(dist + ball.radius));
        ball->pos = addVecFl2d(ball->pos, mulVecFl2d(d, -(dist + ball->radius)));
    }

    // update velocity
    // var v = ball.vel.dot(d);
    float v = dotVecFl2d(ball->vel, d);
    // var vNew = Math.abs(v) * ball.restitution;
    float vNew = ABS(v) * ball->restitution;

    // ball.vel.add(d, vNew - v);
    ball->vel = addVecFl2d(ball->vel, mulVecFl2d(d, vNew - v));
}

// simulation -------------------------------------------------------

void jsSimulate(jsScene_t* scene)
{
    // for (var i = 0; i < physicsScene.flippers.length; i++)
    //     physicsScene.flippers[i].simulate(physicsScene.dt);
    for (int32_t i = 0; i < scene->numFlippers; i++)
    {
        jsFlipperSimulate(&scene->flippers[i], scene->dt);
    }

    // for (var i = 0; i < physicsScene.balls.length; i++)
    for (int32_t i = 0; i < scene->numBalls; i++)
    {
        // var ball = physicsScene.balls[i];
        jsBall_t* ball = &scene->balls[i];
        // ball.simulate(physicsScene.dt, physicsScene.gravity);
        jsBallSimulate(ball, scene->dt, scene->gravity);

        // for (var j = i + 1; j < physicsScene.balls.length; j++) {
        for (int32_t j = i + 1; j < scene->numBalls; j++)
        {
            // var ball2 = physicsScene.balls[j];
            jsBall_t* ball2 = &scene->balls[j];
            // handleBallBallCollision(ball, ball2, physicsScene.restitution);
            handleBallBallCollision(ball, ball2);
        }

        // for (var j = 0; j < physicsScene.obstacles.length; j++)
        //     handleBallObstacleCollision(ball, physicsScene.obstacles[j]);
        for (int32_t j = 0; j < scene->numObstacles; j++)
        {
            handleBallObstacleCollision(scene, ball, &scene->obstacles[j]);
        }

        // for (var j = 0; j < physicsScene.flippers.length; j++)
        //     handleBallFlipperCollision(ball, physicsScene.flippers[j]);
        for (int32_t j = 0; j < scene->numFlippers; j++)
        {
            handleBallFlipperCollision(ball, &scene->flippers[j]);
        }

        // handleBallBorderCollision(ball, physicsScene.border);
        handleBallBorderCollision(ball, scene->border, scene->numBorders);
    }
}

// draw -------------------------------------------------------

/**
 * @brief TODO doc
 *
 * @param scene
 * @param pos
 * @return int32_t
 */
static int32_t cX(jsScene_t* scene, vecFl_t pos)
{
    return (int32_t)(pos.x * scene->cScale);
}

/**
 * @brief TODO doc
 *
 * @param scene
 * @param pos
 * @return int32_t
 */
static int32_t cY(jsScene_t* scene, vecFl_t pos)
{
    return (int32_t)(TFT_HEIGHT - pos.y * scene->cScale);
}

/**
 * @brief TODO doc
 *
 * @param scene
 */
void jsSceneDraw(jsScene_t* scene)
{
    // c.clearRect(0, 0, TFT_WIDTH, TFT_HEIGHT);
    clearPxTft();

    // border
    if (scene->numBorders >= 2)
    {
        for (int32_t i = 0; i < scene->numBorders; i++)
        {
            vecFl_t p1 = scene->border[i];
            vecFl_t p2 = scene->border[(i + 1) % scene->numBorders];
            drawLineFast(cX(scene, p1), cY(scene, p1), cX(scene, p2), cY(scene, p2), c555);
        }
    }

    // balls
    for (int32_t i = 0; i < scene->numBalls; i++)
    {
        vecFl_t pos = scene->balls[i].pos;
        drawCircleFilled(cX(scene, pos), cY(scene, pos), scene->balls[i].radius * scene->cScale, c222);
    }

    // obstacles
    for (int32_t i = 0; i < scene->numObstacles; i++)
    {
        vecFl_t pos = scene->obstacles[i].pos;
        drawCircleFilled(cX(scene, pos), cY(scene, pos), scene->obstacles[i].radius * scene->cScale, c511);
    }

    // flippers
    for (int32_t i = 0; i < scene->numFlippers; i++)
    {
        jsFlipper_t* flipper = &scene->flippers[i];
        vecFl_t pos          = flipper->pos;
        drawCircleFilled(cX(scene, pos), cY(scene, pos), flipper->radius * scene->cScale, c115);
        pos = jsFlipperGetTip(flipper);
        drawCircleFilled(cX(scene, pos), cY(scene, pos), flipper->radius * scene->cScale, c115);
        // TODO raw lines between tip and base
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
                scene->flippers[0].touchIdentifier = 0;
                break;
            }
            case PB_RIGHT:
            {
                scene->flippers[1].touchIdentifier = 0;
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
                scene->flippers[0].touchIdentifier = -1;
                break;
            }
            case PB_RIGHT:
            {
                scene->flippers[1].touchIdentifier = -1;
                break;
            }
            default:
            {
                break;
            }
        }
    }
}
