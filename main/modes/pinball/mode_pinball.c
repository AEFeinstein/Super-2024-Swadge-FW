//==============================================================================
// Includes
//==============================================================================

#include "mode_pinball.h"

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    circle_t c;
    vec_q24_8 vel;
} pbCircle_t;

typedef struct
{
    line_t l;
} pbLine_t;

typedef struct
{
    pbCircle_t ball;
    pbCircle_t bumper;
    pbLine_t wall;
} pinball_t;

//==============================================================================
// Function Prototypes
//==============================================================================

static void pinEnterMode(void);
static void pinExitMode(void);
static void pinMainLoop(int64_t elapsedUs);
static void pinBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);

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

    pb->ball.c.pos.x  = 10;
    pb->ball.c.pos.y  = (TFT_HEIGHT / 2) - 29;
    pb->ball.c.radius = 10;

    pb->ball.vel.x = 5;
    pb->ball.vel.y = 0;

    pb->bumper.c.pos.x  = TFT_WIDTH / 2;
    pb->bumper.c.pos.y  = TFT_HEIGHT / 2;
    pb->bumper.c.radius = 20;

    pb->wall.l.p1.x = 20;
    pb->wall.l.p1.y = 20;
    pb->wall.l.p2.x = 400;
    pb->wall.l.p2.y = 400;
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
    // Work in milliseconds, not microseconds
    uint32_t elapsedMs = elapsedUs / 1000;

    // Check for collision
    if (circleCircleIntersection(pb->ball.c, pb->bumper.c))
    {
        // Reflect the velocity vector along the normal between the two radii
        // See http://www.sunshine2k.de/articles/coding/vectorreflection/vectorreflection.html
        vec_q24_8 centerToCenter = {
            .x = pb->ball.c.pos.x - pb->bumper.c.pos.x,
            .y = pb->ball.c.pos.y - pb->bumper.c.pos.y,
        };
        vec_q24_8 reflVec = fpvNorm(centerToCenter);
        pb->ball.vel      = fpvSub(pb->ball.vel, fpvMulSc(reflVec, (2 * fpvDot(pb->ball.vel, reflVec))));
    }

    if (circleLineIntersection(pb->ball.c, pb->wall.l))
    {
        pb->ball.vel.x = 0;
        pb->ball.vel.y = 0;
    }

    // Move the ball
    pb->ball.c.pos.x += (pb->ball.vel.x * elapsedMs);
    pb->ball.c.pos.y += (pb->ball.vel.y * elapsedMs);

    // Draw Stuff
    clearPxTft();
    drawCircle(pb->ball.c.pos.x, pb->ball.c.pos.y, pb->ball.c.radius, c005);
    drawCircleFilled(pb->bumper.c.pos.x, pb->bumper.c.pos.y, pb->bumper.c.radius, c500);
    drawLineFast(pb->wall.l.p1.x, pb->wall.l.p1.y, pb->wall.l.p2.x, pb->wall.l.p2.y, c050);
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
