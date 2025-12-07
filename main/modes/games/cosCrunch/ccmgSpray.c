#include "ccmgSpray.h"
#include "cosCrunch.h"
#include "cosCrunchUtil.h"

static void ccmgSprayInitMicrogame(void);
static void ccmgSprayDestroyMicrogame(bool successful);
static void ccmgSprayMainLoop(int64_t elapsedUs, uint64_t timeRemainingUs, float timeScale,
                              cosCrunchMicrogameState state, buttonEvt_t buttonEvts[], uint8_t buttonEvtCount);
static bool ccmgSprayMicrogameTimeout(void);

static const char ccmgSprayVerb[]       = "Spray";
static const char ccmgSpraySuccessMsg[] = "Smooth coat";
static const char ccmgSprayFailureMsg[] = "Poor coverage";
const cosCrunchMicrogame_t ccmgSpray    = {
       .verb                     = ccmgSprayVerb,
       .successMsg               = ccmgSpraySuccessMsg,
       .failureMsg               = ccmgSprayFailureMsg,
       .timeoutUs                = 5000000,
       .fnInitMicrogame          = ccmgSprayInitMicrogame,
       .fnDestroyMicrogame       = ccmgSprayDestroyMicrogame,
       .fnMainLoop               = ccmgSprayMainLoop,
       .fnBackgroundDrawCallback = NULL,
       .fnMicrogameTimeout       = ccmgSprayMicrogameTimeout,
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

    const tintColor_t* tintColor;

    rectangle_t targetArea;
    vec_t canvasPos;
    uint32_t paintTimeRemainingUs;

    vec_t sprayCenter;
    uint16_t sprayCenterOrigX;
    uint16_t sprayCenterOrigY;

    uint16_t oldXOffset;
    int64_t yOffsetElapsedUs;
    uint8_t sweepCount;

    bool nozzlePressed;
    cosCrunchMicrogameState previousState;
} ccmgSpray_t;
ccmgSpray_t* ccmgs = NULL;

#define SPRAY_CENTER_START_X  160
#define SPRAY_CENTER_START_Y  50
#define SPRAY_START_XY_JITTER 15
#define SWEEP_RANGE_X         100
#define SWEEP_CURVE_RANGE_X   5
#define SWEEP_US_PER_SIN_DEG  5500
#define SWEEP_US_PER_Y_PX     13700
#define SWEEP_HEIGHT          21
#define SWEEP_COUNT           6

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

    ccmgs->targetArea.width  = TARGET_AREA_WIDTH + esp_random() % TARGET_AREA_WH_JITTER;
    ccmgs->targetArea.height = TARGET_AREA_HEIGHT + esp_random() % TARGET_AREA_WH_JITTER;
    // Width and height both need to be odd so the dithered shadow doesn't look weird
    ccmgs->targetArea.width += ccmgs->targetArea.width % 2 - 1;
    ccmgs->targetArea.height += ccmgs->targetArea.height % 2 - 1;

    ccmgs->targetArea.pos.x     = TARGET_AREA_CENTER_X - ccmgs->targetArea.width / 2;
    ccmgs->targetArea.pos.y     = TARGET_AREA_CENTER_Y - ccmgs->targetArea.height / 2;
    ccmgs->paintTimeRemainingUs = ccmgs->targetArea.width * ccmgs->targetArea.height * PAINT_US_PER_TARGET_PX;

    ccmgs->sprayCenter.x    = SPRAY_CENTER_START_X + esp_random() % SPRAY_START_XY_JITTER * 2 - SPRAY_START_XY_JITTER;
    ccmgs->sprayCenter.y    = SPRAY_CENTER_START_Y + esp_random() % SPRAY_START_XY_JITTER * 2 - SPRAY_START_XY_JITTER;
    ccmgs->sprayCenterOrigX = ccmgs->sprayCenter.x;
    ccmgs->sprayCenterOrigY = ccmgs->sprayCenter.y;

    loadWsg(CC_SPRAY_CAN_WSG, &ccmgs->wsg.can, false);
    loadWsg(CC_SPRAY_CAN_GRAPHICS_WSG, &ccmgs->wsg.canGraphics, false);
    loadWsg(CC_SPRAY_NOZZLE_WSG, &ccmgs->wsg.nozzle, false);
    loadWsg(CC_SPRAY_WSG, &ccmgs->wsg.spray, false);

    ccmgs->wsg.canvas.w = SWEEP_RANGE_X + ccmgs->wsg.spray.w;
    ccmgs->wsg.canvas.h = SWEEP_HEIGHT * SWEEP_COUNT;
    ccmgs->canvasPos.x  = ccmgs->sprayCenter.x - ccmgs->wsg.canvas.w + ccmgs->wsg.spray.w / 2;
    ccmgs->canvasPos.y  = ccmgs->sprayCenter.y - ccmgs->wsg.spray.h / 2;

    ccmgs->wsg.canvas.px = (paletteColor_t*)heap_caps_malloc_tag(
        sizeof(paletteColor_t) * ccmgs->wsg.canvas.w * ccmgs->wsg.canvas.h, MALLOC_CAP_8BIT, "wsg");
    for (uint32_t i = 0; i < ccmgs->wsg.canvas.w * ccmgs->wsg.canvas.h; i++)
    {
        ccmgs->wsg.canvas.px[i] = cTransparent;
    }
    ccmgs->tintColor = cosCrunchMicrogameGetTintColor();

    ccmgs->previousState = CC_MG_GET_READY;

    globalMidiPlayerGet(MIDI_SFX)->channels[0].timbre.type = NOISE;
}

static void ccmgSprayDestroyMicrogame(bool successful)
{
    for (uint16_t x = ccmgs->targetArea.pos.x; x < ccmgs->targetArea.pos.x + ccmgs->targetArea.width; x++)
    {
        for (uint16_t y = ccmgs->targetArea.pos.y; y < ccmgs->targetArea.pos.y + ccmgs->targetArea.height; y++)
        {
            ccmgs->wsg.canvas.px[(y - ccmgs->canvasPos.y) * ccmgs->wsg.canvas.w + (x - ccmgs->canvasPos.x)]
                = cTransparent;
        }
    }
    cosCrunchMicrogamePersistSplatter(ccmgs->wsg.canvas, ccmgs->canvasPos.x, ccmgs->canvasPos.y);

    freeWsg(&ccmgs->wsg.can);
    freeWsg(&ccmgs->wsg.canGraphics);
    freeWsg(&ccmgs->wsg.nozzle);
    freeWsg(&ccmgs->wsg.spray);
    freeWsg(&ccmgs->wsg.canvas);
    heap_caps_free(ccmgs);
}

static void ccmgSprayMainLoop(int64_t elapsedUs, uint64_t timeRemainingUs, float timeScale,
                              cosCrunchMicrogameState state, buttonEvt_t buttonEvts[], uint8_t buttonEvtCount)
{
    bool previousNozzlePressed = ccmgs->nozzlePressed;
    for (uint8_t i = 0; i < buttonEvtCount; i++)
    {
        if (buttonEvts[i].button == PB_A)
        {
            ccmgs->nozzlePressed = buttonEvts[i].down;
        }
    }

    if (state == CC_MG_PLAYING)
    {
        if (ccmgs->nozzlePressed && ccmgs->paintTimeRemainingUs > 0
            && (ccmgs->previousState == CC_MG_GET_READY || !previousNozzlePressed))
        {
            midiNoteOn(globalMidiPlayerGet(MIDI_SFX), 0, 60, 0x7f);
        }

        uint64_t totalElapsedUs = ccmgSpray.timeoutUs - timeRemainingUs;

        // xOffset is a range of 0..SWEEP_RANGE_X timed to a sine wave
        uint16_t xOffset
            = (getSin1024((totalElapsedUs / SWEEP_US_PER_SIN_DEG + 270) % 360) + 1024) * SWEEP_RANGE_X / 2048;

        if ((ccmgs->oldXOffset > SWEEP_RANGE_X - SWEEP_CURVE_RANGE_X && xOffset <= SWEEP_RANGE_X - SWEEP_CURVE_RANGE_X)
            || (ccmgs->oldXOffset < SWEEP_CURVE_RANGE_X && xOffset >= SWEEP_CURVE_RANGE_X))
        {
            ccmgs->sweepCount++;
        }
        ccmgs->sprayCenter.x = ccmgs->sprayCenterOrigX - xOffset;
        ccmgs->sprayCenter.y = ccmgs->sprayCenterOrigY + MAX(ccmgs->sweepCount - 1, 0) * SWEEP_HEIGHT;

        if (ccmgs->sweepCount > 0 && (xOffset < SWEEP_CURVE_RANGE_X || xOffset > SWEEP_RANGE_X - SWEEP_CURVE_RANGE_X))
        {
            ccmgs->yOffsetElapsedUs += elapsedUs;
            ccmgs->sprayCenter.y += ccmgs->yOffsetElapsedUs / SWEEP_US_PER_Y_PX;
        }
        else
        {
            ccmgs->yOffsetElapsedUs = 0;
        }

        ccmgs->oldXOffset = xOffset;
    }

    uint16_t canX    = ccmgs->sprayCenter.x + SPRAY_CENTER_TO_CAN_X_DIST;
    uint16_t canY    = ccmgs->sprayCenter.y + ccmgs->wsg.nozzle.h / 2;
    uint16_t nozzleY = canY - ccmgs->wsg.nozzle.h;
    if (ccmgs->nozzlePressed)
    {
        nozzleY += NOZZLE_TRAVEL_PX;
    }

    // White rectangle
    drawRectFilled(ccmgs->targetArea.pos.x, ccmgs->targetArea.pos.y, ccmgs->targetArea.pos.x + ccmgs->targetArea.width,
                   ccmgs->targetArea.pos.y + ccmgs->targetArea.height, c555);

    // Dotted lines to make a dithered shadow
    drawLine(ccmgs->targetArea.pos.x + 1, ccmgs->targetArea.pos.y + ccmgs->targetArea.height,
             ccmgs->targetArea.pos.x + 1 + ccmgs->targetArea.width, ccmgs->targetArea.pos.y + ccmgs->targetArea.height,
             c111, 1);
    drawLine(ccmgs->targetArea.pos.x + 2, ccmgs->targetArea.pos.y + ccmgs->targetArea.height + 1,
             ccmgs->targetArea.pos.x + 2 + ccmgs->targetArea.width,
             ccmgs->targetArea.pos.y + ccmgs->targetArea.height + 1, c111, 1);
    drawLine(ccmgs->targetArea.pos.x + ccmgs->targetArea.width, ccmgs->targetArea.pos.y + 1,
             ccmgs->targetArea.pos.x + ccmgs->targetArea.width, ccmgs->targetArea.pos.y + 1 + ccmgs->targetArea.height,
             c111, 1);
    drawLine(ccmgs->targetArea.pos.x + ccmgs->targetArea.width + 1, ccmgs->targetArea.pos.y + 2,
             ccmgs->targetArea.pos.x + ccmgs->targetArea.width + 1,
             ccmgs->targetArea.pos.y + 2 + ccmgs->targetArea.height, c111, 1);

    // Paint that's already been sprayed
    drawWsgSimple(&ccmgs->wsg.canvas, ccmgs->canvasPos.x, ccmgs->canvasPos.y);

    if (state == CC_MG_PLAYING && ccmgs->nozzlePressed && ccmgs->paintTimeRemainingUs > 0)
    {
        ccmgs->paintTimeRemainingUs = MAX(ccmgs->paintTimeRemainingUs - elapsedUs, 0);

        // The actual spray decal, drawn to another wsg to track where we've painted already
        uint16_t rotation = esp_random() % 360;
        drawToCanvasTint(
            ccmgs->wsg.canvas, ccmgs->wsg.spray, ccmgs->sprayCenter.x - ccmgs->wsg.spray.w / 2 - ccmgs->canvasPos.x,
            ccmgs->sprayCenter.y - ccmgs->wsg.spray.h / 2 - ccmgs->canvasPos.y, rotation, ccmgs->tintColor);

        // Lines that sound like "fssshhhhh"
        drawLine(canX + ccmgs->wsg.can.w / 2 - 4, ccmgs->sprayCenter.y - 2, ccmgs->sprayCenter.x + 2,
                 ccmgs->sprayCenter.y - (ccmgs->wsg.spray.h / 2 - 1), ccmgs->tintColor->lowlight, 6);
        drawLine(canX + ccmgs->wsg.can.w / 2 - 4, ccmgs->sprayCenter.y + 2, ccmgs->sprayCenter.x + 2,
                 ccmgs->sprayCenter.y + (ccmgs->wsg.spray.h / 2 - 1), ccmgs->tintColor->lowlight, 6);
    }
    else
    {
        midiAllNotesOff(globalMidiPlayerGet(MIDI_SFX), 0);
    }

    if (state == CC_MG_GET_READY || state == CC_MG_PLAYING)
    {
        // Circle to show where the paint will end up
        drawCircle(ccmgs->sprayCenter.x, ccmgs->sprayCenter.y, ccmgs->wsg.spray.w / 2 + 1, ccmgs->tintColor->lowlight);
    }

    drawWsgSimple(&ccmgs->wsg.nozzle, canX + ccmgs->wsg.can.w / 2 - ccmgs->wsg.nozzle.w / 2, nozzleY);
    drawWsgSimple(&ccmgs->wsg.can, canX, canY);
    drawWsgPaletteSimple(&ccmgs->wsg.canGraphics, canX, canY + 23, cosCrunchMicrogameGetWsgPalette(ccmgs->tintColor));

    ccmgs->previousState = state;
}

static bool ccmgSprayMicrogameTimeout()
{
    uint32_t filledPixels = 0, missedPixels = 0;
    for (uint16_t x = ccmgs->targetArea.pos.x; x < ccmgs->targetArea.pos.x + ccmgs->targetArea.width; x++)
    {
        for (uint16_t y = ccmgs->targetArea.pos.y; y < ccmgs->targetArea.pos.y + ccmgs->targetArea.height; y++)
        {
            if (ccmgs->wsg.canvas.px[(y - ccmgs->canvasPos.y) * ccmgs->wsg.canvas.w + (x - ccmgs->canvasPos.x)]
                == cTransparent)
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
