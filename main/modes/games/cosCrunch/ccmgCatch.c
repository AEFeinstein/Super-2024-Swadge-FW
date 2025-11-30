#include "ccmgCatch.h"

static void ccmgCatchInitMicrogame(void);
static void ccmgCatchDestroyMicrogame(void);
static void ccmgCatchMainLoop(int64_t elapsedUs, uint64_t timeRemainingUs, float timeScale,
                              cosCrunchMicrogameState state, buttonEvt_t buttonEvts[], uint8_t buttonEvtCount);
static void ccmgCatchBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);

static const char ccmgCatchVerb[]       = "Shop";
static const char ccmgCatchSuccessMsg[] = "Clearance discount";
const cosCrunchMicrogame_t ccmgCatch    = {
       .verb                     = ccmgCatchVerb,
       .successMsg               = ccmgCatchSuccessMsg,
       .failureMsg               = NULL,
       .timeoutUs                = 5500000,
       .fnInitMicrogame          = ccmgCatchInitMicrogame,
       .fnDestroyMicrogame       = ccmgCatchDestroyMicrogame,
       .fnMainLoop               = ccmgCatchMainLoop,
       .fnBackgroundDrawCallback = ccmgCatchBackgroundDrawCallback,
       .fnMicrogameTimeout       = NULL,
};

#define FABRIC_COUNT       3
#define FABRIC_SPAWN_MIN_X 20

#define LEG_WIDTH                8
#define PLAYER_FABRIC_MIN_X      31
#define PLAYER_FABRIC_Y          145
#define PLAYER_X_SPEED_PX_PER_US (180.f / 1000000)
#define FOOT_MAX_DIST            8

#define FABRIC_Y_ACCEL_PX_PER_US_SQ (95.f / 1000000 / 1000000)
#define FABRIC_SPAWN_DELAY_US       2500000

#define SIGN_Y_ACCEL_PX_PER_US_SQ (400.f / 1000000 / 1000000)
#define SIGN_Y_MAX                4

typedef struct
{
    uint16_t buttonState;

    /// Positions for all fabric images. The 0th item functions as the player's position.
    int16_t fabricPosX[FABRIC_COUNT];
    float fabricPosY[FABRIC_COUNT];
    uint8_t fabricsDropped;
    uint8_t fabricsCaught;
    float fabricYSpeed;

    int8_t leftFootXOffset;
    int8_t rightFootXOffset;
    bool footExtending;

    float signYSpeed;
    float signY;
    bool signBounced;

    const tintColor_t* tintColor;
    wsgPalette_t tintPalettes[FABRIC_COUNT];

    struct
    {
        wsg_t fabric;
        wsg_t shopper;
        wsg_t foot;
        wsg_t background;
        wsg_t closedSign;
    } wsg;
} ccmgCatch_t;
ccmgCatch_t* ccmgc = NULL;

static void ccmgCatchInitMicrogame()
{
    ccmgc = heap_caps_calloc(1, sizeof(ccmgCatch_t), MALLOC_CAP_8BIT);

    ccmgc->tintColor = cosCrunchMicrogameGetTintColor();
    wsgPaletteReset(&ccmgc->tintPalettes[0]);
    wsgPaletteReset(&ccmgc->tintPalettes[1]);
    wsgPaletteReset(&ccmgc->tintPalettes[2]);

    // Randomize order of tint palettes
    uint8_t indices[FABRIC_COUNT] = {0};
    for (uint8_t i = 0; i < FABRIC_COUNT; i++)
    {
        int candidate = esp_random() % FABRIC_COUNT;
        bool used     = false;
        for (uint8_t j = 0; j < i; j++)
        {
            if (indices[j] == candidate)
            {
                used = true;
            }
        }
        if (used)
        {
            i--;
        }
        else
        {
            indices[i] = candidate;
        }
    }
    ccmgc->tintPalettes[indices[0]].newColors[PALETTE_LOWLIGHT] = c000;
    ccmgc->tintPalettes[indices[0]].newColors[PALETTE_BASE]     = ccmgc->tintColor->lowlight;
    ccmgc->tintPalettes[indices[1]].newColors[PALETTE_LOWLIGHT] = ccmgc->tintColor->lowlight;
    ccmgc->tintPalettes[indices[1]].newColors[PALETTE_BASE]     = ccmgc->tintColor->base;
    ccmgc->tintPalettes[indices[2]].newColors[PALETTE_LOWLIGHT] = ccmgc->tintColor->base;
    ccmgc->tintPalettes[indices[2]].newColors[PALETTE_BASE]     = ccmgc->tintColor->highlight;

    loadWsg(CC_FABRIC_BUNDLE_WSG, &ccmgc->wsg.fabric, false);
    loadWsg(CC_FABRIC_SHOPPER_WSG, &ccmgc->wsg.shopper, false);
    loadWsg(CC_FABRIC_SHOPPER_FOOT_WSG, &ccmgc->wsg.foot, false);
    loadWsg(CC_FABRIC_STORE_BACKGROUND_WSG, &ccmgc->wsg.background, false);
    loadWsg(CC_FABRIC_CLOSED_SIGN_WSG, &ccmgc->wsg.closedSign, false);

    ccmgc->fabricPosX[0] = TFT_WIDTH / 2 - ccmgc->wsg.fabric.w / 2;
    ccmgc->fabricPosY[0] = PLAYER_FABRIC_Y;
    ccmgc->signY         = -ccmgc->wsg.closedSign.h;
}

static void ccmgCatchDestroyMicrogame()
{
    freeWsg(&ccmgc->wsg.fabric);
    freeWsg(&ccmgc->wsg.shopper);
    freeWsg(&ccmgc->wsg.foot);
    freeWsg(&ccmgc->wsg.background);
    freeWsg(&ccmgc->wsg.closedSign);

    heap_caps_free(ccmgc);
}

static void ccmgCatchMainLoop(int64_t elapsedUs, uint64_t timeRemainingUs, float timeScale,
                              cosCrunchMicrogameState state, buttonEvt_t buttonEvts[], uint8_t buttonEvtCount)
{
    if (buttonEvtCount > 0)
    {
        ccmgc->buttonState = buttonEvts[buttonEvtCount - 1].state;
    }

    int16_t oldFabricYPos = ccmgc->fabricPosY[ccmgc->fabricsDropped];
    if (state == CC_MG_PLAYING || state == CC_MG_DESPAIRING)
    {
        if (ccmgc->fabricsDropped > ccmgc->fabricsCaught)
        {
            ccmgc->fabricYSpeed += FABRIC_Y_ACCEL_PX_PER_US_SQ * elapsedUs;
            ccmgc->fabricPosY[ccmgc->fabricsDropped] += ccmgc->fabricYSpeed * elapsedUs;
        }
    }

    switch (state)
    {
        case CC_MG_GET_READY:
            // Do nothing
            break;

        case CC_MG_PLAYING:
        case CC_MG_CELEBRATING:
        {
            int16_t fabricsNeeded = (ccmgCatch.timeoutUs - timeRemainingUs) / FABRIC_SPAWN_DELAY_US + 1;
            if (fabricsNeeded > ccmgc->fabricsDropped && ccmgc->fabricsDropped < FABRIC_COUNT - 1)
            {
                ccmgc->fabricsDropped++;
                ccmgc->fabricPosX[ccmgc->fabricsDropped]
                    = (esp_random() % (TFT_WIDTH - ccmgc->wsg.fabric.w - FABRIC_SPAWN_MIN_X * 2)) + FABRIC_SPAWN_MIN_X;
                ccmgc->fabricPosY[ccmgc->fabricsDropped] = -ccmgc->wsg.fabric.h;
                ccmgc->fabricYSpeed                      = 0;
            }

            int8_t direction;
            if (ccmgc->buttonState & PB_LEFT)
            {
                direction = -1;
            }
            else if (ccmgc->buttonState & PB_RIGHT)
            {
                direction = 1;
            }
            else
            {
                direction = 0;
            }
            int16_t distance = direction * PLAYER_X_SPEED_PX_PER_US * elapsedUs;

            int16_t fabricStackTopY = ccmgc->fabricPosY[0] - ccmgc->wsg.fabric.h * ccmgc->fabricsDropped;
            if (oldFabricYPos <= fabricStackTopY && ccmgc->fabricPosY[ccmgc->fabricsDropped] > fabricStackTopY)
            {
                if (ccmgc->fabricPosX[ccmgc->fabricsDropped] + ccmgc->wsg.fabric.w
                        > ccmgc->fabricPosX[ccmgc->fabricsCaught]
                    && ccmgc->fabricPosX[ccmgc->fabricsDropped]
                           < ccmgc->fabricPosX[ccmgc->fabricsCaught] + ccmgc->wsg.fabric.w)
                {
                    ccmgc->fabricsCaught                    = ccmgc->fabricsDropped;
                    ccmgc->fabricPosY[ccmgc->fabricsCaught] = fabricStackTopY;
                    if (ccmgc->fabricsCaught == FABRIC_COUNT - 1)
                    {
                        cosCrunchMicrogameResult(true);
                    }

                    midiNoteOn(globalMidiPlayerGet(MIDI_SFX), 9, MARACAS, 0x7f);
                }
                else
                {
                    cosCrunchMicrogameResult(false);
                }
            }

            if (ccmgc->footExtending)
            {
                ccmgc->leftFootXOffset  = CLAMP(ccmgc->leftFootXOffset + distance, -FOOT_MAX_DIST, 0);
                ccmgc->rightFootXOffset = CLAMP(ccmgc->rightFootXOffset + distance, 0, FOOT_MAX_DIST);
                if ((direction == -1 && ccmgc->leftFootXOffset == -FOOT_MAX_DIST)
                    || (direction == 1 && ccmgc->rightFootXOffset == FOOT_MAX_DIST))
                {
                    ccmgc->footExtending = false;
                }
            }
            else
            {
                int16_t xMovement = distance;
                for (int i = 0; i <= ccmgc->fabricsCaught; i++)
                {
                    if (i == 0)
                    {
                        int16_t oldPosX = ccmgc->fabricPosX[i];
                        // Limit position of the bottom fabric only to restrict player movement
                        ccmgc->fabricPosX[i] = CLAMP(ccmgc->fabricPosX[i] + distance, PLAYER_FABRIC_MIN_X,
                                                     TFT_WIDTH - ccmgc->wsg.fabric.w - PLAYER_FABRIC_MIN_X);
                        xMovement            = ccmgc->fabricPosX[i] - oldPosX;
                    }
                    else
                    {
                        ccmgc->fabricPosX[i] += xMovement;
                    }
                }

                if (direction == -1)
                {
                    ccmgc->leftFootXOffset = CLAMP(ccmgc->leftFootXOffset - distance, -FOOT_MAX_DIST, 0);
                }
                else
                {
                    ccmgc->rightFootXOffset = CLAMP(ccmgc->rightFootXOffset - distance, 0, FOOT_MAX_DIST);
                }
                if ((direction == -1 && ccmgc->leftFootXOffset == 0)
                    || (direction == 1 && ccmgc->rightFootXOffset == 0))
                {
                    ccmgc->footExtending = true;
                }
            }
            break;
        }

        case CC_MG_DESPAIRING:
        {
            ccmgc->signYSpeed += SIGN_Y_ACCEL_PX_PER_US_SQ * elapsedUs;
            ccmgc->signY = ccmgc->signY + ccmgc->signYSpeed * elapsedUs;
            if (ccmgc->signY >= SIGN_Y_MAX)
            {
                ccmgc->signY = SIGN_Y_MAX;
                if (!ccmgc->signBounced)
                {
                    ccmgc->signBounced = true;
                    ccmgc->signYSpeed  = -ccmgc->signYSpeed / 3;
                }
            }
            drawWsgSimple(&ccmgc->wsg.closedSign, (TFT_WIDTH - ccmgc->wsg.closedSign.w) / 2, ccmgc->signY);
            break;
        }
    }

    for (int i = 0; i <= ccmgc->fabricsDropped; i++)
    {
        drawWsgPaletteSimple(&ccmgc->wsg.fabric, ccmgc->fabricPosX[i], ccmgc->fabricPosY[i], &ccmgc->tintPalettes[i]);

        if (i == 0)
        {
            // Draw player after the fabric they're holding but before other fabrics for proper layering
            drawWsgSimple(&ccmgc->wsg.shopper, ccmgc->fabricPosX[0] - 2, ccmgc->fabricPosY[0] - 35);
            for (int x = 0; x < LEG_WIDTH; x++)
            {
                paletteColor_t lineColor;
                if (x == 0 || x == LEG_WIDTH - 1)
                {
                    lineColor = c111;
                }
                else
                {
                    lineColor = c333;
                }
                // Left leg
                drawLineFast(ccmgc->fabricPosX[0] + ccmgc->wsg.fabric.w / 2 - x,
                             ccmgc->fabricPosY[0] + ccmgc->wsg.fabric.h,
                             ccmgc->fabricPosX[0] + ccmgc->wsg.fabric.w / 2 - x + ccmgc->leftFootXOffset,
                             ccmgc->fabricPosY[0] + ccmgc->wsg.fabric.h + 20, lineColor);
                // Right leg
                drawLineFast(ccmgc->fabricPosX[0] + ccmgc->wsg.fabric.w / 2 + 2 + x,
                             ccmgc->fabricPosY[0] + ccmgc->wsg.fabric.h,
                             ccmgc->fabricPosX[0] + ccmgc->wsg.fabric.w / 2 + 2 + x + ccmgc->rightFootXOffset,
                             ccmgc->fabricPosY[0] + ccmgc->wsg.fabric.h + 20, lineColor);
            }
            // Left foot
            drawWsgSimple(&ccmgc->wsg.foot,
                          ccmgc->fabricPosX[0] + ccmgc->wsg.fabric.w / 2 - 12 + ccmgc->leftFootXOffset,
                          ccmgc->fabricPosY[0] + ccmgc->wsg.fabric.h + 20);
            // Right foot
            drawWsg(&ccmgc->wsg.foot, ccmgc->fabricPosX[0] + ccmgc->wsg.fabric.w / 2 + 2 + ccmgc->rightFootXOffset,
                    ccmgc->fabricPosY[0] + ccmgc->wsg.fabric.h + 20, true, false, 0);
        }
    }
}

static void ccmgCatchBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
    memcpy(&getPxTftFramebuffer()[x + y * TFT_WIDTH], &ccmgc->wsg.background.px[x + y * TFT_WIDTH], w * h);
}
