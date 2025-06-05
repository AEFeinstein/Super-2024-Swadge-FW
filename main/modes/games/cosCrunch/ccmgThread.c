#include "cosCrunch.h"
#include "ccmgThread.h"

#include "esp_random.h"

static void ccmgThreadInitMicrogame(void);
static void ccmgThreadDestroyMicrogame(void);
static void ccmgThreadMainLoop(int64_t elapsedUs, uint64_t timeRemainingUs, cosCrunchMicrogameState state,
                               buttonEvt_t buttonEvts[], uint8_t buttonEvtCount);

#define NEEDLE_X 90
// These values need to line up with the needle image or the player will have a bad time
#define NEEDLE_EYE_HEIGHT   77
#define NEEDLE_EYE_Y_OFFSET 20

#define THREAD_START_X                    (TFT_WIDTH - 30)
#define THREAD_START_Y                    70
#define THREAD_X_SPEED_PX_PER_US          (55.f / 1000000)
#define THREAD_Y_ACCEL_MIN_PX_PER_US_SQ   (50.f / 1000000 / 1000000)
#define THREAD_Y_ACCEL_MAX_PX_PER_US_SQ   (90.f / 1000000 / 1000000)
#define THREAD_Y_ACCEL_CHANGE_TIME_MAX_US 170000
#define PLAYER_Y_ACCEL_PX_PER_US_SQ       (90.f / 1000000 / 1000000)

#define FINGERS_OFFSET_X 175
#define FINGERS_OFFSET_Y (-64)

tintColor_t const fingersTintColors[] = {
    // Light
    {c321, c432, c554, c543},
    // Dark
    {c100, c210, c543, c432},
};

static const char ccmgThreadVerb[]    = "Thread The Needle";
const cosCrunchMicrogame_t ccmgThread = {
    .verb                     = ccmgThreadVerb,
    .successMsg               = NULL,
    .failureMsg               = NULL,
    .timeoutUs                = 3100000,
    .fnInitMicrogame          = ccmgThreadInitMicrogame,
    .fnDestroyMicrogame       = ccmgThreadDestroyMicrogame,
    .fnMainLoop               = ccmgThreadMainLoop,
    .fnBackgroundDrawCallback = NULL,
    .fnMicrogameTimeout       = NULL,
};

typedef struct
{
    float threadX;
    float threadY;
    float threadYSpeed;
    float threadYAccel;
    int64_t threadYAccelChangeTimeUs;

    bool upPressed;
    bool downPressed;
    int64_t buttonHeldTimeUs;
    float threadYUserSpeed;

    struct
    {
        wsg_t thread;
        wsg_t fingers;
        wsg_t needleLeft;
        wsg_t needleRight;
    } wsg;

    const tintColor_t* threadTintColor;
    const tintColor_t* fingersTintColor;
} ccmgThread_t;
static ccmgThread_t* ccmgt = NULL;

static void ccmgThreadInitMicrogame(void)
{
    ccmgt = heap_caps_calloc(1, sizeof(ccmgThread_t), MALLOC_CAP_8BIT);

    ccmgt->threadX                  = THREAD_START_X;
    ccmgt->threadY                  = THREAD_START_Y;
    ccmgt->threadYAccelChangeTimeUs = esp_random() % THREAD_Y_ACCEL_CHANGE_TIME_MAX_US;

    loadWsg(CC_THREAD_WSG, &ccmgt->wsg.thread, false);
    loadWsg(CC_FINGERS_WSG, &ccmgt->wsg.fingers, false);
    loadWsg(CC_NEEDLE_LEFT_WSG, &ccmgt->wsg.needleLeft, false);
    loadWsg(CC_NEEDLE_RIGHT_WSG, &ccmgt->wsg.needleRight, false);

    ccmgt->threadTintColor  = cosCrunchMicrogameGetTintColor();
    ccmgt->fingersTintColor = &fingersTintColors[esp_random() % ARRAY_SIZE(fingersTintColors)];
}

static void ccmgThreadDestroyMicrogame(void)
{
    freeWsg(&ccmgt->wsg.thread);
    freeWsg(&ccmgt->wsg.fingers);
    freeWsg(&ccmgt->wsg.needleLeft);
    freeWsg(&ccmgt->wsg.needleRight);

    heap_caps_free(ccmgt);
}

static void ccmgThreadMainLoop(int64_t elapsedUs, uint64_t timeRemainingUs, cosCrunchMicrogameState state,
                               buttonEvt_t buttonEvts[], uint8_t buttonEvtCount)
{
    switch (state)
    {
        case CC_MG_GET_READY:
            // Do nothing
            break;

        case CC_MG_PLAYING:
        {
            bool oldUpPressed   = ccmgt->upPressed;
            bool oldDownPressed = ccmgt->downPressed;
            for (uint8_t i = 0; i < buttonEvtCount; i++)
            {
                if (buttonEvts[i].button == PB_UP)
                {
                    ccmgt->upPressed = buttonEvts[i].down;
                }
                else if (buttonEvts[i].button == PB_DOWN)
                {
                    ccmgt->downPressed = buttonEvts[i].down;
                }
            }
            if (oldUpPressed != ccmgt->upPressed || oldDownPressed != ccmgt->downPressed)
            {
                ccmgt->buttonHeldTimeUs = 0;
            }
            ccmgt->buttonHeldTimeUs += elapsedUs;

            ccmgt->threadYAccelChangeTimeUs -= elapsedUs;
            if (ccmgt->threadYAccelChangeTimeUs <= 0)
            {
                ccmgt->threadYAccelChangeTimeUs = esp_random() % THREAD_Y_ACCEL_CHANGE_TIME_MAX_US;

                uint16_t rand = esp_random() % 100; // 0..99
                ccmgt->threadYAccel
                    = (THREAD_Y_ACCEL_MAX_PX_PER_US_SQ - THREAD_Y_ACCEL_MIN_PX_PER_US_SQ) * (rand / 99.f)
                      + THREAD_Y_ACCEL_MIN_PX_PER_US_SQ;
                if (rand % 2 == 0)
                {
                    ccmgt->threadYAccel *= -1;
                }
            }

            ccmgt->threadYSpeed += ccmgt->threadYAccel * elapsedUs;

            float effectiveYSpeed = ccmgt->threadYSpeed;
            if (ccmgt->upPressed)
            {
                effectiveYSpeed -= PLAYER_Y_ACCEL_PX_PER_US_SQ * ccmgt->buttonHeldTimeUs;
            }
            else if (ccmgt->downPressed)
            {
                effectiveYSpeed += PLAYER_Y_ACCEL_PX_PER_US_SQ * ccmgt->buttonHeldTimeUs;
            }
            ccmgt->threadY += effectiveYSpeed * elapsedUs;

            int16_t oldThreadX = (int16_t)ccmgt->threadX;
            ccmgt->threadX -= THREAD_X_SPEED_PX_PER_US * elapsedUs;

            uint16_t needleCenterX = NEEDLE_X + ccmgt->wsg.needleLeft.w;
            if (oldThreadX > needleCenterX && (int16_t)ccmgt->threadX <= needleCenterX)
            {
                // The thread image is drawn so the loose fibers fill the height and touch the top and bottom left
                // corners, so we can use the full thread image height to determine hits
                uint16_t needleEyeTop = TFT_HEIGHT - ccmgt->wsg.needleLeft.h + NEEDLE_EYE_Y_OFFSET;
                if ((int16_t)ccmgt->threadY > needleEyeTop
                    && (int16_t)ccmgt->threadY + ccmgt->wsg.thread.h < needleEyeTop + NEEDLE_EYE_HEIGHT)
                {
                    cosCrunchMicrogameResult(true);
                }
                else
                {
                    ccmgt->threadX = needleCenterX;
                    cosCrunchMicrogameResult(false);
                }
            }
            break;
        }

        case CC_MG_CELEBRATING:
            ccmgt->threadX -= THREAD_X_SPEED_PX_PER_US * elapsedUs;
            break;

        case CC_MG_DESPAIRING:
            // Do nothing
            break;
    }

    drawWsgSimple(&ccmgt->wsg.needleRight, NEEDLE_X + ccmgt->wsg.needleLeft.w, TFT_HEIGHT - ccmgt->wsg.needleRight.h);
    drawWsgPaletteSimple(&ccmgt->wsg.thread, ccmgt->threadX, ccmgt->threadY,
                         cosCrunchMicrogameGetWsgPalette(ccmgt->threadTintColor));
    drawWsgPaletteSimple(&ccmgt->wsg.fingers, ccmgt->threadX + FINGERS_OFFSET_X, ccmgt->threadY + FINGERS_OFFSET_Y,
                         cosCrunchMicrogameGetWsgPalette(ccmgt->fingersTintColor));
    drawWsgSimple(&ccmgt->wsg.needleLeft, NEEDLE_X, TFT_HEIGHT - ccmgt->wsg.needleLeft.h);
}
