//==============================================================================
// Includes
//==============================================================================

#include "mode_pinball.h"

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    vec_q24_8 pos;
    vec_q24_8 vel; // Velocity is in pixels per frame (@ 60fps, so pixels per 16.7ms)
    q24_8 radius;
    paletteColor_t color;
    bool filled;
} pbCircle_t;

typedef struct
{
    vec_q24_8 p1;
    vec_q24_8 p2;
    paletteColor_t color;
} pbLine_t;

typedef struct
{
    pbCircle_t ball;
    pbCircle_t bumper;
    pbLine_t wall;
    int32_t frameTimer;
} pinball_t;

//==============================================================================
// Function Prototypes
//==============================================================================

static void pinEnterMode(void);
static void pinExitMode(void);
static void pinMainLoop(int64_t elapsedUs);
static void pinBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);

circle_t intCircle(pbCircle_t pbc);
line_t intLine(pbLine_t pbl);

void drawPinCircle(pbCircle_t* c);
void drawPinLine(pbLine_t* l);

//==============================================================================
// Variables
//==============================================================================

const char pinballName[] = "Pinball";

swadgeMode_t pinballMode = {
    .modeName                 = pinballName,
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = false,
    .usesAccelerometer        = false,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = pinEnterMode,
    .fnExitMode               = pinExitMode,
    .fnMainLoop               = pinMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = pinBackgroundDrawCallback,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL,
};

pinball_t* pb;

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Set up the pinball mode
 *
 * TODO literally everything
 */
static void pinEnterMode(void)
{
    pb = calloc(sizeof(pinball_t), 1);

    pb->ball.pos.x  = TO_FX(10);
    pb->ball.pos.y  = TO_FX((TFT_HEIGHT / 2) - 29);
    pb->ball.radius = TO_FX(10);
    pb->ball.vel.x  = TO_FX_FRAC(64 * 16667, 1000000);
    pb->ball.vel.y  = 0;
    pb->ball.color  = c500;
    pb->ball.filled = true;

    pb->bumper.pos.x  = TO_FX(240);
    pb->bumper.pos.y  = TO_FX(3 * TFT_HEIGHT / 4);
    pb->bumper.radius = TO_FX(20);
    pb->bumper.color  = c050;
    pb->bumper.filled = false;

    pb->wall.p1.x  = TO_FX(20);
    pb->wall.p1.y  = TO_FX(20);
    pb->wall.p2.x  = TO_FX(400);
    pb->wall.p2.y  = TO_FX(200);
    pb->wall.color = c005;
}

/**
 * @brief Tear down the pinball mode
 *
 */
static void pinExitMode(void)
{
    free(pb);
}

/**
 * @brief Run the pinball main loop
 *
 * @param elapsedUs The number of microseconds since this function was last called
 */
static void pinMainLoop(int64_t elapsedUs)
{
    pb->frameTimer += elapsedUs;
    while (pb->frameTimer >= 16667)
    {
        pb->frameTimer -= 16667;

        // Check for collision
        if (circleCircleIntersection(intCircle(pb->ball), intCircle(pb->bumper)))
        {
            // Reflect the velocity vector along the normal between the two radii
            // See http://www.sunshine2k.de/articles/coding/vectorreflection/vectorreflection.html
            vec_q24_8 centerToCenter = {
                .x = pb->ball.pos.x - pb->bumper.pos.x,
                .y = pb->ball.pos.y - pb->bumper.pos.y,
            };
            vec_q24_8 reflVec = fpvNorm(centerToCenter);
            pb->ball.vel      = fpvSub(pb->ball.vel, fpvMulSc(reflVec, (2 * fpvDot(pb->ball.vel, reflVec))));
        }

        vec_t collisionVec;
        static bool once = true; // TODO fix this to not multi-intersect
        if (once && circleLineIntersection(intCircle(pb->ball), intLine(pb->wall), &collisionVec))
        {
            once                     = false;
            vec_q24_8 centerToCenter = {
                .x = collisionVec.x,
                .y = collisionVec.y,
            };
            vec_q24_8 reflVec = fpvNorm(centerToCenter);
            pb->ball.vel      = fpvSub(pb->ball.vel, fpvMulSc(reflVec, (2 * fpvDot(pb->ball.vel, reflVec))));
        }

        // Move the ball
        pb->ball.pos.x += (pb->ball.vel.x);
        pb->ball.pos.y += (pb->ball.vel.y);

        // Draw Stuff
        clearPxTft();
        drawPinCircle(&pb->ball);
        drawPinCircle(&pb->bumper);
        drawPinLine(&pb->wall);
    }
}

/**
 * @brief Draw the background for the pinball game
 *
 * @param x the x coordinate that should be updated
 * @param y the x coordinate that should be updated
 * @param w the width of the rectangle to be updated
 * @param h the height of the rectangle to be updated
 * @param up update number
 * @param numUp update number denominator
 */
static void pinBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
    // No background draws, for now
}

/**
 * @brief TODO
 *
 * @param pbc
 * @return circle_t
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
 * @brief TODO
 *
 * @param pbl
 * @return line_t
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
 * @brief TODO
 *
 * @param c
 */
void drawPinCircle(pbCircle_t* c)
{
    if (c->filled)
    {
        drawCircleFilled(FROM_FX(c->pos.x), FROM_FX(c->pos.y), FROM_FX(c->radius), c->color);
    }
    else
    {
        drawCircle(FROM_FX(c->pos.x), FROM_FX(c->pos.y), FROM_FX(c->radius), c->color);
    }
}

/**
 * @brief TODO
 *
 * @param l
 */
void drawPinLine(pbLine_t* l)
{
    drawLineFast(FROM_FX(l->p1.x), FROM_FX(l->p1.y), FROM_FX(l->p2.x), FROM_FX(l->p2.y), l->color);
}