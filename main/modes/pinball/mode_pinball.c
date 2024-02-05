//==============================================================================
// Includes
//==============================================================================

#include "mode_pinball.h"
#include <esp_random.h>

//==============================================================================
// Defines
//==============================================================================

#define US_PER_FRAME   16667
#define NUM_PARTITIONS 32

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    vec_q24_8 pos;
    vec_q24_8 vel; // Velocity is in pixels per frame (@ 60fps, so pixels per 16.7ms)
    q24_8 radius;
    int32_t zoneMask;
    paletteColor_t color;
    bool filled;
} pbCircle_t;

typedef struct
{
    vec_q24_8 p1;
    vec_q24_8 p2;
    int32_t zoneMask;
    paletteColor_t color;
} pbLine_t;

typedef struct
{
    vec_q24_8 pos; ///< The position the top left corner of the rectangle
    q24_8 width;   ///< The width of the rectangle
    q24_8 height;  ///< The height of the rectangle
    int32_t zoneMask;
    paletteColor_t color;
} pbRect_t;

typedef struct
{
    pbCircle_t ball;
    pbCircle_t bumper;
    pbLine_t wall;
    int32_t frameTimer;
    pbRect_t partitions[NUM_PARTITIONS];
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
rectangle_t intRect(pbRect_t pbr);

void drawPinCircle(pbCircle_t* c);
void drawPinLine(pbLine_t* l);
void drawPinRect(pbRect_t* r);

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

    // Make some test objects
    pb->ball.pos.x  = TO_FX(10);
    pb->ball.pos.y  = TO_FX((TFT_HEIGHT / 2) - 29);
    pb->ball.radius = TO_FX(10);
    pb->ball.vel.x  = TO_FX_FRAC(64 * US_PER_FRAME, 1000000);
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

    // Partition the space into a grid. Start with one big rectangle
    int32_t splitOffset      = (NUM_PARTITIONS >> 1);
    pb->partitions[0].pos.x  = 0;
    pb->partitions[0].pos.y  = 0;
    pb->partitions[0].width  = TO_FX(TFT_WIDTH);
    pb->partitions[0].height = TO_FX(TFT_HEIGHT);
    pb->partitions[0].color  = c505;

    // While more partitioning needs to happen
    while (splitOffset)
    {
        // Iterate over current partitions, back to front
        for (int32_t i = NUM_PARTITIONS - 1; i >= 0; i--)
        {
            // If this is a real partition
            if (0 < pb->partitions[i].height)
            {
                // Split it either vertically or horizontally, depending on which is larger
                if (pb->partitions[i].height > pb->partitions[i].width)
                {
                    // Split vertically
                    int32_t newHeight_1 = pb->partitions[i].height / 2;
                    int32_t newHeight_2 = pb->partitions[i].height - newHeight_1;

                    // Shrink the original partition
                    pb->partitions[i].height = newHeight_1;

                    // Create the new partition
                    pb->partitions[i + splitOffset].height = newHeight_2;
                    pb->partitions[i + splitOffset].pos.y  = pb->partitions[i].pos.y + pb->partitions[i].height;

                    pb->partitions[i + splitOffset].width = pb->partitions[i].width;
                    pb->partitions[i + splitOffset].pos.x = pb->partitions[i].pos.x;
                }
                else
                {
                    // Split horizontally
                    int32_t newWidth_1 = pb->partitions[i].width / 2;
                    int32_t newWidth_2 = pb->partitions[i].width - newWidth_1;

                    // Shrink the original partition
                    pb->partitions[i].width = newWidth_1;

                    // Create the new partition
                    pb->partitions[i + splitOffset].width = newWidth_2;
                    pb->partitions[i + splitOffset].pos.x = pb->partitions[i].pos.x + pb->partitions[i].width;

                    pb->partitions[i + splitOffset].height = pb->partitions[i].height;
                    pb->partitions[i + splitOffset].pos.y  = pb->partitions[i].pos.y;
                }

                // Give it a random color, just because
                pb->partitions[i + splitOffset].color = esp_random() % cTransparent;
            }
        }

        // Half the split offset
        splitOffset /= 2;
    }

    // Calculate zones for the wall, just as a test
    // TODO do this for all objects
    pb->wall.zoneMask = 0;
    for (int i = 0; i < NUM_PARTITIONS; i++)
    {
        rectangle_t zone = intRect(pb->partitions[i]);
        if (rectLineIntersection(zone, intLine(pb->wall)))
        {
            pb->wall.zoneMask |= (1 << i);
        }
    }
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
    while (pb->frameTimer >= US_PER_FRAME)
    {
        pb->frameTimer -= US_PER_FRAME;

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

        for (int32_t i = 0; i < NUM_PARTITIONS; i++)
        {
            drawPinRect(&pb->partitions[i]);
        }
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
 * @brief
 *
 * @param pbr
 * @return rectangle_t
 */
rectangle_t intRect(pbRect_t pbr)
{
    rectangle_t nr = {
        .pos.x  = FROM_FX(pbr.pos.x),
        .pos.y  = FROM_FX(pbr.pos.y),
        .width  = FROM_FX(pbr.width),
        .height = FROM_FX(pbr.height),
    };
    return nr;
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

/**
 * @brief
 *
 * @param r
 */
void drawPinRect(pbRect_t* r)
{
    drawRect(FROM_FX(r->pos.x), FROM_FX(r->pos.y), FROM_FX(ADD_FX(r->pos.x, r->width)),
             FROM_FX(ADD_FX(r->pos.y, r->height)), r->color);
}
