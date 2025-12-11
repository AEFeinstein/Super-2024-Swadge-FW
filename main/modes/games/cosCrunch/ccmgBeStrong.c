#include "ccmgBeStrong.h"
#include "cosCrunch.h"

static void ccmgBeStrongInitMicrogame(void);
static void ccmgBeStrongDestroyMicrogame(bool successful);
static void ccmgBeStrongMainLoop(int64_t elapsedUs, uint64_t timeRemainingUs, float timeScale,
                                 cosCrunchMicrogameState state, buttonEvt_t buttonEvts[], uint8_t buttonEvtCount);

static const char ccmgBeStrongVerb[]       = "Be Strong";
static const char ccmgBeStrongSuccessMsg[] = "I'm okay";
const cosCrunchMicrogame_t ccmgBeStrong    = {
       .verb                     = ccmgBeStrongVerb,
       .successMsg               = ccmgBeStrongSuccessMsg,
       .failureMsg               = NULL,
       .timeoutUs                = 3000000,
       .fnInitMicrogame          = ccmgBeStrongInitMicrogame,
       .fnDestroyMicrogame       = ccmgBeStrongDestroyMicrogame,
       .fnMainLoop               = ccmgBeStrongMainLoop,
       .fnBackgroundDrawCallback = NULL,
       .fnMicrogameTimeout       = NULL,
};

#define LEFT_TEAR_START_RADIUS 8
#define TEAR_MAX_RADIUS        22
#define TEAR_X_OFFSET          70
#define TEAR_GROWTH_PER_US     (8.f / 1000000)

#define TEAR_FALL_SHRINKAGE_PER_US           (40.f / 1000000)
#define TEAR_FALL_MIN_Y_SHRINKAGE_MULTIPLIER 6
#define TEAR_FALL_MAX_Y_SHRINKAGE_MULTIPLIER 9

#define TEAR_COLOR           c135
#define TEAR_HIGHLIGHT_COLOR c245

typedef struct
{
    float radius;
    float xPos;
    float yPos;
} tear_t;

typedef struct
{
    bool buttonPressed;

    tear_t leftTear;
    tear_t rightTear;

    uint16_t fallYShrinkageMultiplier;
    int64_t endElapsedUs;

    struct
    {
        wsg_t tearSplat;
    } wsg;
} ccmgBeStrong_t;
ccmgBeStrong_t* ccmgbs = NULL;

static void ccmgBeStrongInitMicrogame(void)
{
    ccmgbs = heap_caps_calloc(1, sizeof(ccmgBeStrong_t), MALLOC_CAP_8BIT);
    loadWsg(CC_TEAR_SPLAT_WSG, &ccmgbs->wsg.tearSplat, false);

    ccmgbs->leftTear.xPos   = TEAR_X_OFFSET;
    ccmgbs->rightTear.xPos  = TFT_WIDTH - TEAR_X_OFFSET;
    ccmgbs->leftTear.radius = LEFT_TEAR_START_RADIUS;
    ccmgbs->fallYShrinkageMultiplier
        = esp_random() % (TEAR_FALL_MAX_Y_SHRINKAGE_MULTIPLIER - TEAR_FALL_MIN_Y_SHRINKAGE_MULTIPLIER + 1)
          + TEAR_FALL_MIN_Y_SHRINKAGE_MULTIPLIER;

    // Vibraphone sounds like sniffling, but it's kind of annoying played repeatedly so drop the volume a bit
    globalMidiPlayerSetVolume(MIDI_SFX, 12);
}

static void ccmgBeStrongDestroyMicrogame(bool successful)
{
    if (!successful)
    {
        cosCrunchMicrogamePersistSplatter(ccmgbs->wsg.tearSplat, ccmgbs->leftTear.xPos - ccmgbs->wsg.tearSplat.w / 2,
                                          ccmgbs->leftTear.yPos - ccmgbs->wsg.tearSplat.h / 2);
        cosCrunchMicrogamePersistSplatter(ccmgbs->wsg.tearSplat, ccmgbs->rightTear.xPos - ccmgbs->wsg.tearSplat.w / 2,
                                          ccmgbs->rightTear.yPos - ccmgbs->wsg.tearSplat.h / 2);
    }

    freeWsg(&ccmgbs->wsg.tearSplat);
    heap_caps_free(ccmgbs);
}

static void drawTeardrops()
{
    for (int tearIndex = 0; tearIndex <= 1; tearIndex++)
    {
        tear_t* tear;
        if (tearIndex == 0)
        {
            tear = &ccmgbs->leftTear;
        }
        else
        {
            tear = &ccmgbs->rightTear;
        }

        if (tear->radius > 0)
        {
            drawTriangleOutlined(tear->xPos, tear->yPos, tear->xPos - tear->radius + 1,
                                 tear->yPos + tear->radius * 4 - 3, tear->xPos + tear->radius,
                                 tear->yPos + tear->radius * 4 - 3, TEAR_COLOR, TEAR_COLOR);
            drawCircleFilled(tear->xPos, tear->yPos + tear->radius * 4 - 3, tear->radius, TEAR_COLOR);
            if (tear->radius > 4)
            {
                drawLineFast(tear->xPos - tear->radius + 3, tear->yPos + tear->radius * 4 - 5,
                             tear->xPos - tear->radius * .75 + 3, tear->yPos + tear->radius * 3 - 3,
                             TEAR_HIGHLIGHT_COLOR);
            }
        }
    }
}

static void ccmgBeStrongMainLoop(int64_t elapsedUs, uint64_t timeRemainingUs, float timeScale,
                                 cosCrunchMicrogameState state, buttonEvt_t buttonEvts[], uint8_t buttonEvtCount)
{
    switch (state)
    {
        case CC_MG_GET_READY:
        case CC_MG_PLAYING:
        {
            ccmgbs->leftTear.radius += TEAR_GROWTH_PER_US * elapsedUs;
            ccmgbs->rightTear.radius += TEAR_GROWTH_PER_US * elapsedUs;

            if (state == CC_MG_PLAYING)
            {
                for (uint8_t i = 0; i < buttonEvtCount; i++)
                {
                    if (buttonEvts[i].button == PB_A && buttonEvts[i].down)
                    {
                        ccmgbs->leftTear.radius  = MAX(ccmgbs->leftTear.radius - 3 * timeScale, 0);
                        ccmgbs->rightTear.radius = MAX(ccmgbs->rightTear.radius - 3 * timeScale, 0);
                        midiNoteOn(globalMidiPlayerGet(MIDI_SFX), 9, VIBRASLAP, 0x7f);
                    }
                }
            }

            if (ccmgbs->leftTear.radius == 0)
            {
                cosCrunchMicrogameResult(true);
            }
            else if (ccmgbs->leftTear.radius >= TEAR_MAX_RADIUS)
            {
                cosCrunchMicrogameResult(false);
            }

            drawTeardrops();

            break;
        }

        case CC_MG_CELEBRATING:
            // Do nothing
            break;

        case CC_MG_DESPAIRING:
        {
            ccmgbs->endElapsedUs += elapsedUs;

            if (ccmgbs->leftTear.radius > 1)
            {
                ccmgbs->leftTear.radius -= TEAR_FALL_SHRINKAGE_PER_US * elapsedUs;
                ccmgbs->leftTear.yPos += TEAR_FALL_SHRINKAGE_PER_US * ccmgbs->fallYShrinkageMultiplier * elapsedUs;
            }
            else
            {
                if (ccmgbs->leftTear.radius > 0)
                {
                    ccmgbs->leftTear.radius = 0;
                    globalMidiPlayerSetVolume(MIDI_SFX, 13);
                    midiNoteOn(globalMidiPlayerGet(MIDI_SFX), 9, MARACAS, 0x7f);
                }
                drawWsgSimple(&ccmgbs->wsg.tearSplat, ccmgbs->leftTear.xPos - ccmgbs->wsg.tearSplat.w / 2,
                              ccmgbs->leftTear.yPos - ccmgbs->wsg.tearSplat.h / 2);
            }

            if (ccmgbs->endElapsedUs >= 500000)
            {
                if (ccmgbs->rightTear.radius > 1)
                {
                    ccmgbs->rightTear.radius -= TEAR_FALL_SHRINKAGE_PER_US * elapsedUs;
                    ccmgbs->rightTear.yPos += TEAR_FALL_SHRINKAGE_PER_US * ccmgbs->fallYShrinkageMultiplier * elapsedUs;
                }
                else
                {
                    if (ccmgbs->rightTear.radius > 0)
                    {
                        ccmgbs->rightTear.radius = 0;
                        // Volume was already reset when left tear hit
                        midiNoteOn(globalMidiPlayerGet(MIDI_SFX), 9, MARACAS, 0x7f);
                    }
                    drawWsgSimple(&ccmgbs->wsg.tearSplat, ccmgbs->rightTear.xPos - ccmgbs->wsg.tearSplat.w / 2,
                                  ccmgbs->rightTear.yPos - ccmgbs->wsg.tearSplat.h / 2);
                }
            }
            else
            {
                ccmgbs->rightTear.radius += TEAR_GROWTH_PER_US * elapsedUs;
            }

            drawTeardrops();

            break;
        }
    }
}
