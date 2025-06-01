#include "ccmgBreakTime.h"
#include "cosCrunch.h"

#include <stddef.h>

static void ccmgBreakTimeInitMicrogame(void);
static void ccmgBreakTimeDestroyMicrogame(void);
static void ccmgBreakTimeMainLoop(int64_t elapsedUs, uint64_t timeRemainingUs, cosCrunchMicrogameState state,
                                  buttonEvt_t buttonEvts[], uint8_t buttonEvtCount);
static bool ccmgBreakTimeTimeout(void);

static const char ccmgBreakTimeVerb[]       = "Break Time";
static const char ccmgBreakTimeSuccessMsg[] = "Calm enhanced";
cosCrunchMicrogame_t ccmgBreakTime          = {
             .verb               = ccmgBreakTimeVerb,
             .successMsg         = ccmgBreakTimeSuccessMsg,
             .failureMsg         = NULL,
             .timeoutUs          = 3000000,
             .fnInitMicrogame    = ccmgBreakTimeInitMicrogame,
             .fnDestroyMicrogame = ccmgBreakTimeDestroyMicrogame,
             .fnMainLoop         = ccmgBreakTimeMainLoop,
             .fnMicrogameTimeout = ccmgBreakTimeTimeout,
};

#define MUG_WIDTH        65
#define MUG_LIP_WIDTH    4
#define STEAM_WSG_WIDTH  MUG_WIDTH
#define STEAM_WSG_HEIGHT 67
#define STEAM_MAX_WIDTH  15

typedef struct
{
    bool buttonPressed;

    /// Center of the steam line coming out of the mug
    int16_t steamX;
    int16_t steamWidth;
    bool steamDitherEvenPixels;

    struct
    {
        wsg_t mug;
        wsg_t mugLiquid;
        wsg_t mugSpilled;
        wsg_t mugSpilledLiquid;
        wsg_t spill;
        wsg_t steam;
    } wsg;
    paletteColor_t steamPixels[STEAM_WSG_WIDTH * STEAM_WSG_HEIGHT];

    tintColor_t liquidTintColor;
    wsgPalette_t liquidTintPalette;
} ccmgBreakTime_t;
ccmgBreakTime_t* ccmgbt = NULL;

static void ccmgBreakTimeInitMicrogame(void)
{
    ccmgbt = heap_caps_calloc(1, sizeof(ccmgBreakTime_t), MALLOC_CAP_8BIT);

    ccmgbt->steamX                = STEAM_WSG_WIDTH / 2;
    ccmgbt->steamWidth            = 1;
    ccmgbt->steamDitherEvenPixels = false;

    loadWsg(CC_MUG_WSG, &ccmgbt->wsg.mug, false);
    loadWsg(CC_MUG_LIQUID_WSG, &ccmgbt->wsg.mugLiquid, false);
    loadWsg(CC_MUG_SPILLED_WSG, &ccmgbt->wsg.mugSpilled, false);
    loadWsg(CC_MUG_SPILLED_LIQUID_WSG, &ccmgbt->wsg.mugSpilledLiquid, false);
    loadWsg(CC_SPILL_WSG, &ccmgbt->wsg.spill, false);

    ccmgbt->wsg.steam.w  = STEAM_WSG_WIDTH;
    ccmgbt->wsg.steam.h  = STEAM_WSG_HEIGHT;
    ccmgbt->wsg.steam.px = ccmgbt->steamPixels;
    for (uint32_t i = 0; i < ccmgbt->wsg.steam.w * ccmgbt->wsg.steam.h; i++)
    {
        ccmgbt->steamPixels[i] = cTransparent;
    }

    int8_t drinkType = esp_random() % 3;
    if (drinkType == 0)
    {
        // Coffee
        ccmgbt->liquidTintColor.base      = c210;
        ccmgbt->liquidTintColor.highlight = c310;
    }
    else if (drinkType == 1)
    {
        // Black tea
        ccmgbt->liquidTintColor.base      = c320;
        ccmgbt->liquidTintColor.highlight = c431;
    }
    else
    {
        // Green tea
        ccmgbt->liquidTintColor.base      = c331;
        ccmgbt->liquidTintColor.highlight = c441;
    }

    wsgPaletteReset(&ccmgbt->liquidTintPalette);
    tintPalette(&ccmgbt->liquidTintPalette, &ccmgbt->liquidTintColor);
}

static void ccmgBreakTimeDestroyMicrogame(void)
{
    freeWsg(&ccmgbt->wsg.mug);
    freeWsg(&ccmgbt->wsg.spill);
    heap_caps_free(ccmgbt);
}

static void ccmgBreakTimeMainLoop(int64_t elapsedUs, uint64_t timeRemainingUs, cosCrunchMicrogameState state,
                                  buttonEvt_t buttonEvts[], uint8_t buttonEvtCount)
{
    if (state == CC_MG_PLAYING)
    {
        for (uint8_t i = 0; i < buttonEvtCount; i++)
        {
            if (buttonEvts[i].down)
            {
                ccmgbt->buttonPressed = true;
                cosCrunchMicrogameResult(false);

                drawToCanvasTint(ccmgbt->wsg.spill, ccmgbt->wsg.spill, 0, 0, 0, &ccmgbt->liquidTintColor);
                cosCrunchMicrogamePersistSplatter(ccmgbt->wsg.spill, 100 - 42, 134);
            }
        }
    }

    // Steam rises. This is 1 px per frame, so its speed is dependent on the frame rate.
    // As long as the frame time is somewhat consistent it looks fine.
    for (uint16_t y = 0; y < STEAM_WSG_HEIGHT; y++)
    {
        for (uint16_t x = 0; x < STEAM_WSG_WIDTH; x++)
        {
            if (y == STEAM_WSG_HEIGHT - 1)
            {
                ccmgbt->wsg.steam.px[y * STEAM_WSG_WIDTH + x] = cTransparent;
            }
            else
            {
                ccmgbt->wsg.steam.px[y * STEAM_WSG_WIDTH + x] = ccmgbt->wsg.steam.px[(y + 1) * STEAM_WSG_WIDTH + x];
            }
        }
    }

    switch (state)
    {
        case CC_MG_GET_READY:
        case CC_MG_PLAYING:
        case CC_MG_CELEBRATING:
            ccmgbt->steamWidth += esp_random() % 3 - 1;
            ccmgbt->steamWidth = CLAMP(ccmgbt->steamWidth, 1, STEAM_MAX_WIDTH);

            drawWsgSimple(&ccmgbt->wsg.mug, 100, 50);
            drawWsgPaletteSimple(&ccmgbt->wsg.mugLiquid, 100 + 3, 50 + 10, &ccmgbt->liquidTintPalette);
            break;
        case CC_MG_DESPAIRING:
            // Taper steam since the cup is gone
            ccmgbt->steamWidth -= esp_random() % 2;
            ccmgbt->steamWidth = MAX(ccmgbt->steamWidth, 0);

            drawWsgSimple(&ccmgbt->wsg.mugSpilled, 100, 50);
            drawWsgPaletteSimple(&ccmgbt->wsg.mugSpilledLiquid, 100 + 5, 50 + 88, &ccmgbt->liquidTintPalette);
            break;
    }

    ccmgbt->steamX += esp_random() % 3 - 1;
    ccmgbt->steamX = CLAMP(ccmgbt->steamX, ccmgbt->steamWidth / 2 + MUG_LIP_WIDTH,
                           STEAM_WSG_WIDTH - MUG_LIP_WIDTH - ccmgbt->steamWidth / 2);

    ccmgbt->steamDitherEvenPixels = !ccmgbt->steamDitherEvenPixels;
    int8_t x                      = -ccmgbt->steamWidth / 2;
    if ((ccmgbt->steamX + x) % 2 == 0)
    {
        if (!ccmgbt->steamDitherEvenPixels)
        {
            x++;
        }
    }
    else
    {
        if (ccmgbt->steamDitherEvenPixels)
        {
            x++;
        }
    }
    // Color every other pixel for dithering effect
    for (; x < ccmgbt->steamWidth / 2; x += 2)
    {
        ccmgbt->wsg.steam.px[(STEAM_WSG_HEIGHT - 1) * STEAM_WSG_WIDTH + ccmgbt->steamX + x] = c555;
    }
    drawWsgSimple(&ccmgbt->wsg.steam, 100, 0);
}

static bool ccmgBreakTimeTimeout(void)
{
    return !ccmgbt->buttonPressed;
}
