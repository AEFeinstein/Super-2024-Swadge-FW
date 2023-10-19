//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <esp_timer.h>

#include "modeTimer.h"
#include "swadge2024.h"
#include "hdw-btn.h"
#include "font.h"
#include "wsg.h"

//==============================================================================
// Defines
//==============================================================================

#define HOURGLASS_FRAMES 12
#define PAUSE_FLASH_SPEED 500000
#define EXPIRE_FLASH_SPEED 250000

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    STOPPED = 0,
    PAUSED,
    RUNNING,
    EXPIRED,
} timerState_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    font_t textFont;
    font_t numberFont;
    wsg_t hourglassFrames[HOURGLASS_FRAMES];

    /// @brief Current timer state
    timerState_t timerState;

    /// @brief If true, counts up (stopwatch), otherwise counts down (timer)
    bool stopwatch;

    /// @brief The amount of time being counted down from
    int64_t countdownTime;

    /// @brief Any time that was accumulated before the timer was resumed
    int64_t accumulatedDuration;

    /// @brief The actual time the timer was started
    int64_t startTime;
} timerMode_t;

//==============================================================================
// Function Prototypes
//==============================================================================

static void timerEnterMode(void);
static void timerExitMode(void);
static void timerMainLoop(int64_t elapsedUs);

//==============================================================================
// Strings
//==============================================================================

static const char timerName[] = "Timer";
static const char hourglassFrameFmt[] = "hourglass_%02" PRIu8 ".wsg";
static const char minutesSecondsFmt[] = "%" PRIu8 ":%02" PRIu8 ".";
static const char hoursMinutesSecondsFmt[] = "%" PRIu64 ":%02" PRIu8 ":%02" PRIu8 ".";
static const char millisFmt[] = "%03" PRIu16;
//static const char minutesSecondsBg[] = "88:88.";
//static const char millisBg[] = "888";

//==============================================================================
// Variables
//==============================================================================

swadgeMode_t timerMode = {
    .modeName                 = timerName,
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = false,
    .usesAccelerometer        = true,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = timerEnterMode,
    .fnExitMode               = timerExitMode,
    .fnMainLoop               = timerMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = NULL,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL,
};

static timerMode_t* timerData = NULL;

//==============================================================================
// Functions
//==============================================================================

static void timerEnterMode(void)
{
    timerData = calloc(1, sizeof(timerMode_t));

    for (uint8_t i = 0; i < HOURGLASS_FRAMES; i++)
    {
        char wsgName[18];
        snprintf(wsgName, sizeof(wsgName), hourglassFrameFmt, i);
        loadWsg(wsgName, &timerData->hourglassFrames[i], false);
    }

    loadFont("ibm_vga8.font", &timerData->textFont, false);
    loadFont("seven_segment.font", &timerData->numberFont, false);

    // 100FPS? Sure?
    setFrameRateUs(1000000 / 100);

    // Default to 30s
    timerData->countdownTime = 30 * 1000000;
    timerData->timerState = STOPPED;
}

static void timerExitMode(void)
{
    for (uint8_t i = 0; i < HOURGLASS_FRAMES; i++)
    {
        freeWsg(&timerData->hourglassFrames[i]);
    }

    freeFont(&timerData->textFont);
    freeFont(&timerData->numberFont);

    free(timerData);
    timerData = NULL;
}

static void timerMainLoop(int64_t elapsedUs)
{
    int64_t now = esp_timer_get_time();
    if (timerData->timerState == RUNNING
        && (!timerData->stopwatch && timerData->accumulatedDuration + (now - timerData->startTime) >= timerData->countdownTime))
    {
        timerData->accumulatedDuration = timerData->countdownTime;
        timerData->timerState = EXPIRED;
    }

    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        if (evt.down && evt.button == PB_A)
        {
            // A pressed, pause/unpause/start
            switch (timerData->timerState)
            {
                case STOPPED:
                    timerData->accumulatedDuration = 0;
                    // Intentional fall-through
                case PAUSED:
                {
                    timerData->startTime = now;
                    timerData->timerState = RUNNING;
                    break;
                }

                case RUNNING:
                {
                    timerData->accumulatedDuration += (now - timerData->startTime);
                    timerData->timerState = PAUSED;
                    break;
                }

                case EXPIRED:
                {
                    timerData->timerState = STOPPED;
                    break;
                }
            }
        }
        else if (evt.down && evt.button == PB_B)
        {
            // B pressed, pause/stop/reset
            // TODO
            switch (timerData->timerState)
            {
                case PAUSED:
                {
                    timerData->timerState = STOPPED;
                    timerData->accumulatedDuration = 0;
                    break;
                }

                case RUNNING:
                {
                    timerData->accumulatedDuration += (now - timerData->startTime);
                    timerData->timerState = PAUSED;
                    break;
                }

                case STOPPED:
                case EXPIRED:
                break;
            }
        }
        else if (evt.down && (evt.button == PB_LEFT || evt.button == PB_RIGHT))
        {
            if (timerData->timerState == STOPPED)
            {
                timerData->stopwatch = !timerData->stopwatch;
            }
        }
        else if (evt.down && (evt.button == PB_UP || evt.button == PB_DOWN))
        {
            if (timerData->timerState == STOPPED)
            {
                if (!timerData->stopwatch)
                {
                    timerData->countdownTime += (evt.button == PB_UP) ? 30000000 : -30000000;
                }
            }
        }
    }

    clearPxTft();

    if (timerData->stopwatch)
    {
        drawText(&timerData->textFont, c050, "Stopwatch", 40, 5);
    }
    else
    {
        drawText(&timerData->textFont, c050, "Timer", 40, 5);
    }

    int64_t remaining = timerData->stopwatch
                        ? timerData->accumulatedDuration
                        : timerData->countdownTime - timerData->accumulatedDuration;

    if (timerData->timerState == RUNNING)
    {
        remaining += (timerData->stopwatch ? 1 : -1) * (now - timerData->startTime);
    }

    uint16_t remainingMillis = (remaining / 1000) % 1000;
    uint8_t remainingSecs = (remaining / 1000000) % 60;
    uint8_t remainingMins = (remaining / (60 * 1000000)) % 60;
    // Might as well...
    uint64_t remainingHrs = remaining / 3600000000;

    char buffer[32];
    if (remainingHrs > 0)
    {
        snprintf(buffer, sizeof(buffer), hoursMinutesSecondsFmt,
                 remainingHrs, remainingMins, remainingSecs);
    }
    else
    {
        snprintf(buffer, sizeof(buffer), minutesSecondsFmt,
                 remainingMins, remainingSecs);
    }

    bool blink = (timerData->timerState == PAUSED && 0 == (now / PAUSE_FLASH_SPEED) % 2)
                 || (timerData->timerState == EXPIRED && 0 == (now / EXPIRE_FLASH_SPEED) % 2);

    if (!blink)
    {
        uint16_t textX = 20;
        uint16_t textY = (TFT_HEIGHT - timerData->numberFont.height) / 2;
        //drawText(&timerData->numberFont, c222, minutesSecondsBg, textX, textY);
        textX = drawText(&timerData->numberFont, c050, buffer, textX, textY);

        if (remainingHrs == 0)
        {
            //drawText(&timerData->numberFont, c222, millisBg, textX, textY);
            snprintf(buffer, sizeof(buffer), millisFmt, remainingMillis);

            drawText(&timerData->numberFont, c030, buffer, textX, textY);
        }
    }
}
