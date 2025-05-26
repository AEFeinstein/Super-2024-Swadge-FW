#include "ccmgSpray.h"
#include "cosCrunch.h"
#include "cosCrunchUtil.h"

static void ccmgSprayInitMicrogame(void);
static void ccmgSprayDestroyMicrogame(void);
static void ccmgSprayMainLoop(int64_t elapsedUs, uint64_t timeRemainingUs, cosCrunchMicrogameState state,
                              buttonEvt_t buttonEvts[], uint8_t buttonEvtCount);
static bool ccmgSprayMicrogameTimeout(void);

static const char ccmgSprayVerb[]       = "Spray";
static const char ccmgSpraySuccessMsg[] = "Nice job!";
static const char ccmgSprayFailureMsg[] = "Poor coverage";
cosCrunchMicrogame_t ccmgSpray          = {
             .verb               = ccmgSprayVerb,
             .successMsg         = ccmgSpraySuccessMsg,
             .failureMsg         = ccmgSprayFailureMsg,
             .timeoutUs          = 5000000,
             .fnInitMicrogame    = ccmgSprayInitMicrogame,
             .fnDestroyMicrogame = ccmgSprayDestroyMicrogame,
             .fnMainLoop         = ccmgSprayMainLoop,
             .fnMicrogameTimeout = ccmgSprayMicrogameTimeout,
};

typedef struct
{
    struct
    {
        wsg_t can;
        wsg_t canGraphics;
        wsg_t nozzle;
        wsg_t spray;
        wsg_t canvas;
    } wsg;
    paletteColor_t canvasPixels[TFT_WIDTH * TFT_HEIGHT];

    const tintColor_t* tintColor;

    rectangle_t targetArea;
    uint32_t paintTimeRemainingUs;

    vec_t sprayCenter;
    uint16_t sprayCenterOrigX;

    uint8_t sweepDirection;
    bool firstSweep;

    bool nozzlePressed;
} ccmgSpray_t;
ccmgSpray_t* ccmgs = NULL;

#define SPRAY_CENTER_START_X  160
#define SPRAY_CENTER_START_Y  50
#define SPRAY_START_XY_JITTER 15
#define SWEEP_RANGE_X         100
#define SWEEP_US_PER_SIN_DEG  5500
#define SWEEP_US_PER_Y_PX     10000

#define SPRAY_CENTER_TO_CAN_X_DIST 40
#define NOZZLE_TRAVEL_PX           2

#define TARGET_AREA_CENTER_X  (SPRAY_CENTER_START_X - SWEEP_RANGE_X / 2)
#define TARGET_AREA_CENTER_Y  (SPRAY_CENTER_START_Y + 40)
#define TARGET_AREA_WIDTH     65
#define TARGET_AREA_HEIGHT    65
#define TARGET_AREA_WH_JITTER 20

/// Amount of time the player can hold the button and paint the canvas per target area pixel
#define PAINT_US_PER_TARGET_PX 465

/// How much paint needs to be on the targetArea for the player to succeed
#define COVERAGE_GOAL_PERCENT 90

static void ccmgSprayInitMicrogame(void)
{
    ccmgs = heap_caps_calloc(1, sizeof(ccmgSpray_t), MALLOC_CAP_8BIT);

    ccmgs->targetArea.width     = TARGET_AREA_WIDTH + esp_random() % TARGET_AREA_WH_JITTER;
    ccmgs->targetArea.height    = TARGET_AREA_HEIGHT + esp_random() % TARGET_AREA_WH_JITTER;
    ccmgs->targetArea.pos.x     = TARGET_AREA_CENTER_X - ccmgs->targetArea.width / 2;
    ccmgs->targetArea.pos.y     = TARGET_AREA_CENTER_Y - ccmgs->targetArea.height / 2;
    ccmgs->paintTimeRemainingUs = ccmgs->targetArea.width * ccmgs->targetArea.height * PAINT_US_PER_TARGET_PX;

    ccmgs->sprayCenter.x    = SPRAY_CENTER_START_X + esp_random() % SPRAY_START_XY_JITTER * 2 - SPRAY_START_XY_JITTER;
    ccmgs->sprayCenter.y    = SPRAY_CENTER_START_Y + esp_random() % SPRAY_START_XY_JITTER * 2 - SPRAY_START_XY_JITTER;
    ccmgs->sprayCenterOrigX = ccmgs->sprayCenter.x;

    ccmgs->firstSweep = true;

    loadWsg(CC_SPRAY_CAN_WSG, &ccmgs->wsg.can, false);
    loadWsg(CC_SPRAY_CAN_GRAPHICS_WSG, &ccmgs->wsg.canGraphics, false);
    loadWsg(CC_SPRAY_NOZZLE_WSG, &ccmgs->wsg.nozzle, false);
    loadWsg(CC_SPRAY_WSG, &ccmgs->wsg.spray, false);

    ccmgs->wsg.canvas.w  = TFT_WIDTH;
    ccmgs->wsg.canvas.h  = TFT_HEIGHT;
    ccmgs->wsg.canvas.px = ccmgs->canvasPixels;
    for (uint32_t i = 0; i < ccmgs->wsg.canvas.w * ccmgs->wsg.canvas.h; i++)
    {
        ccmgs->canvasPixels[i] = cTransparent;
    }
    ccmgs->tintColor = cosCrunchMicrogameGetTintColor();
}

static void ccmgSprayDestroyMicrogame(void)
{
    for (uint16_t x = ccmgs->targetArea.pos.x; x < ccmgs->targetArea.pos.x + ccmgs->targetArea.width; x++)
    {
        for (uint16_t y = ccmgs->targetArea.pos.y; y < ccmgs->targetArea.pos.y + ccmgs->targetArea.height; y++)
        {
            ccmgs->wsg.canvas.px[y * ccmgs->wsg.canvas.w + x] = cTransparent;
        }
    }
    cosCrunchMicrogamePersistSplatter(ccmgs->wsg.canvas, 0, 0);

    freeWsg(&ccmgs->wsg.can);
    freeWsg(&ccmgs->wsg.canGraphics);
    freeWsg(&ccmgs->wsg.nozzle);
    freeWsg(&ccmgs->wsg.spray);
    heap_caps_free(ccmgs);
}

static void ccmgSprayMainLoop(int64_t elapsedUs, uint64_t timeRemainingUs, cosCrunchMicrogameState state,
                              buttonEvt_t buttonEvts[], uint8_t buttonEvtCount)
{
    for (uint8_t i = 0; i < buttonEvtCount; i++)
    {
        if (buttonEvts[i].button == PB_A)
        {
            ccmgs->nozzlePressed = buttonEvts[i].down;
        }
    }

    if (state == CC_MG_PLAYING)
    {
        uint64_t totalElapsedUs = ccmgSpray.timeoutUs - timeRemainingUs;
        // xOffset is a range of 0..SWEEP_RANGE_X timed to a sine wave
        uint16_t xOffset
            = (getSin1024((totalElapsedUs / SWEEP_US_PER_SIN_DEG + 270) % 360) + 1024) * SWEEP_RANGE_X / 2048;
        ccmgs->sprayCenter.x = ccmgs->sprayCenterOrigX - xOffset;
        if (xOffset < 5 || xOffset > SWEEP_RANGE_X - 5)
        {
            if (!ccmgs->firstSweep)
            {
                ccmgs->sprayCenter.y += elapsedUs / SWEEP_US_PER_Y_PX;
            }
        }
        else
        {
            ccmgs->firstSweep = false;
        }
    }

    uint16_t canX    = ccmgs->sprayCenter.x + SPRAY_CENTER_TO_CAN_X_DIST;
    uint16_t canY    = ccmgs->sprayCenter.y + ccmgs->wsg.nozzle.h / 2;
    uint16_t nozzleY = canY - ccmgs->wsg.nozzle.h;
    if (ccmgs->nozzlePressed)
    {
        nozzleY += NOZZLE_TRAVEL_PX;
    }

    // White rectangle, its shadow, and the paint already sprayed
    drawRectFilled(ccmgs->targetArea.pos.x + 1, ccmgs->targetArea.pos.y + 1,
                   ccmgs->targetArea.pos.x + ccmgs->targetArea.width + 2,
                   ccmgs->targetArea.pos.y + ccmgs->targetArea.height + 2, c222); // c231
    drawRectFilled(ccmgs->targetArea.pos.x, ccmgs->targetArea.pos.y, ccmgs->targetArea.pos.x + ccmgs->targetArea.width,
                   ccmgs->targetArea.pos.y + ccmgs->targetArea.height, c555);
    drawWsgSimple(&ccmgs->wsg.canvas, 0, 0);

    if (state == CC_MG_PLAYING)
    {
        if (ccmgs->nozzlePressed && ccmgs->paintTimeRemainingUs > 0)
        {
            ccmgs->paintTimeRemainingUs = MAX(ccmgs->paintTimeRemainingUs - elapsedUs, 0);

            // The actual spray decal, drawn to another wsg to track where we've painted already
            uint16_t rotation = esp_random() % 360;
            drawToCanvasTint(ccmgs->wsg.canvas, ccmgs->wsg.spray, ccmgs->sprayCenter.x - ccmgs->wsg.spray.w / 2,
                             ccmgs->sprayCenter.y - ccmgs->wsg.spray.h / 2, rotation, ccmgs->tintColor);

            // Lines that sound like "fssshhhhh"
            drawLine(canX + ccmgs->wsg.can.w / 2 - 4, ccmgs->sprayCenter.y - 2, ccmgs->sprayCenter.x + 2,
                     ccmgs->sprayCenter.y - (ccmgs->wsg.spray.h / 2 - 1), ccmgs->tintColor->lowlight, 6);
            drawLine(canX + ccmgs->wsg.can.w / 2 - 4, ccmgs->sprayCenter.y + 2, ccmgs->sprayCenter.x + 2,
                     ccmgs->sprayCenter.y + (ccmgs->wsg.spray.h / 2 - 1), ccmgs->tintColor->lowlight, 6);
        }
    }

    if (state == CC_MG_GET_READY || state == CC_MG_PLAYING)
    {
        // Circle to show where the paint will end up
        drawCircle(ccmgs->sprayCenter.x, ccmgs->sprayCenter.y, ccmgs->wsg.spray.w / 2 + 1, ccmgs->tintColor->lowlight);
    }

    drawWsgSimple(&ccmgs->wsg.nozzle, canX + ccmgs->wsg.can.w / 2 - ccmgs->wsg.nozzle.w / 2, nozzleY);
    drawWsgSimple(&ccmgs->wsg.can, canX, canY);
    drawWsgPaletteSimple(&ccmgs->wsg.canGraphics, canX, canY + 23, cosCrunchMicrogameGetWsgPalette(ccmgs->tintColor));
}

static bool ccmgSprayMicrogameTimeout()
{
    uint32_t filledPixels = 0, missedPixels = 0;
    for (uint16_t x = ccmgs->targetArea.pos.x; x < ccmgs->targetArea.pos.x + ccmgs->targetArea.width; x++)
    {
        for (uint16_t y = ccmgs->targetArea.pos.y; y < ccmgs->targetArea.pos.y + ccmgs->targetArea.height; y++)
        {
            if (ccmgs->wsg.canvas.px[y * ccmgs->wsg.canvas.w + x] == cTransparent)
            {
                missedPixels++;
            }
            else
            {
                filledPixels++;
            }
        }
    }
    float fillPercentage = filledPixels / (float)(filledPixels + missedPixels) * 100;
    return fillPercentage >= COVERAGE_GOAL_PERCENT;
}
