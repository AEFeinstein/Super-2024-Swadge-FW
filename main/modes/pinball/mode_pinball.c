//==============================================================================
// Includes
//==============================================================================

#include "mode_pinball.h"

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    q24_8 r;
    vec_q24_8 pos;
    vec_q24_8 vel;
    bool isFixed;
} pbCircle_t;

typedef struct
{
    pbCircle_t ball;
    pbCircle_t bumper;
} pinball_t;

//==============================================================================
// Function Prototypes
//==============================================================================

static void pinEnterMode(void);
static void pinExitMode(void);
static void pinMainLoop(int64_t elapsedUs);
static void pinBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);

bool pinCirclesCollide(pbCircle_t* a, pbCircle_t* b);

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

    pb->ball.pos.x = TO_FX(10);
    pb->ball.pos.y = TO_FX((TFT_HEIGHT / 2) - 29);
    pb->ball.r     = TO_FX(10);

    pb->ball.vel.x = 5;
    pb->ball.vel.y = 0;

    pb->bumper.pos.x = TO_FX(TFT_WIDTH / 2);
    pb->bumper.pos.y = TO_FX(TFT_HEIGHT / 2);
    pb->bumper.r     = TO_FX(20);
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
    if (pinCirclesCollide(&pb->ball, &pb->bumper))
    {
        // Reflect the velocity vector along the normal between the two radii
        // See http://www.sunshine2k.de/articles/coding/vectorreflection/vectorreflection.html
        vec_q24_8 reflVec = fpvNorm(fpvSub(pb->ball.pos, pb->bumper.pos));
        pb->ball.vel      = fpvSub(pb->ball.vel, fpvMulSc(reflVec, (2 * fpvDot(pb->ball.vel, reflVec))));
    }

    // Move the ball
    pb->ball.pos.x += (pb->ball.vel.x * elapsedMs);
    pb->ball.pos.y += (pb->ball.vel.y * elapsedMs);

    // Draw Stuff
    clearPxTft();
    drawCircle(FROM_FX(pb->ball.pos.x), FROM_FX(pb->ball.pos.y), FROM_FX(pb->ball.r), c005);
    drawCircleFilled(FROM_FX(pb->bumper.pos.x), FROM_FX(pb->bumper.pos.y), FROM_FX(pb->bumper.r), c500);
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
 * @brief Return true if the two given circles intersect at all
 *
 * @param a A circle
 * @param b The other circle
 * @return true if they intersect, false if they do not
 */
bool pinCirclesCollide(pbCircle_t* a, pbCircle_t* b)
{
    q24_8 delX      = SUB_FX(a->pos.x, b->pos.x);
    q24_8 delY      = SUB_FX(a->pos.y, b->pos.y);
    q24_8 sqDist    = ((delX * delX) + (delY * delY)) >> FRAC_BITS;
    q24_8 sumRadius = ADD_FX(a->r, b->r);
    q24_8 sqRadius  = MUL_FX(sumRadius, sumRadius);
    return sqDist < sqRadius;
}
