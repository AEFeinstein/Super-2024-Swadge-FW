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

#define HOURGLASS_FRAMES   12
#define PAUSE_FLASH_SPEED  1000000
#define PAUSE_FLASH_SHOW   600000
#define EXPIRE_FLASH_SPEED 250000
#define EXPIRE_FLASH_SHOW  125000
#define REPEAT_DELAY       500000
#define REPEAT_TIME        150000

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
    wsg_t dpadWsg;
    wsg_t aWsg;
    wsg_t bWsg;

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

    bool holdingArrow;
    buttonBit_t heldArrow;
    int64_t repeatTimer;
} timerMode_t;

//==============================================================================
// Function Prototypes
//==============================================================================

static void timerEnterMode(void);
static void timerExitMode(void);
static void timerMainLoop(int64_t elapsedUs);
static void incTime(void);
static void decTime(void);

//==============================================================================
// Strings
//==============================================================================

static const char timerName[]              = "Timer";
static const char minutesSecondsFmt[]      = "%02" PRIu8 ":%02" PRIu8 ".%03" PRIu16;
static const char hoursMinutesSecondsFmt[] = "%" PRIu64 ":%02" PRIu8 ":%02" PRIu8 ".%03" PRIu16;

static const char startStr[] = "Start";
static const char pauseStr[] = "Stop";
static const char resetStr[] = "Reset";

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

    loadFont("ibm_vga8.font", &timerData->textFont, false);
    loadFont("seven_segment.font", &timerData->numberFont, false);
    loadWsg("button_up.wsg", &timerData->dpadWsg, false);
    loadWsg("button_a.wsg", &timerData->aWsg, false);
    loadWsg("button_b.wsg", &timerData->bWsg, false);

    // 100FPS? Sure?
    setFrameRateUs(1000000 / 100);

    // Default to 30s
    timerData->countdownTime = 30 * 1000000;
    timerData->timerState    = STOPPED;
}

static void timerExitMode(void)
{
    freeFont(&timerData->textFont);
    freeFont(&timerData->numberFont);

    freeWsg(&timerData->dpadWsg);
    freeWsg(&timerData->aWsg);
    freeWsg(&timerData->bWsg);

    free(timerData);
    timerData = NULL;
}

static void timerMainLoop(int64_t elapsedUs)
{
    int64_t now = esp_timer_get_time();
    if (timerData->timerState == RUNNING
        && (!timerData->stopwatch
            && timerData->accumulatedDuration + (now - timerData->startTime) >= timerData->countdownTime))
    {
        timerData->accumulatedDuration = timerData->countdownTime;
        timerData->timerState          = EXPIRED;
    }

    if (timerData->repeatTimer > elapsedUs)
    {
        timerData->repeatTimer -= elapsedUs;
    }
    else
    {
        if (timerData->holdingArrow)
        {
            if (timerData->heldArrow == PB_UP)
            {
                incTime();
            }
            else if (timerData->heldArrow == PB_DOWN)
            {
                decTime();
            }
            timerData->repeatTimer = REPEAT_TIME - (elapsedUs - timerData->repeatTimer);
        }
    }

    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        if (evt.down)
        {
            if (evt.button == PB_A)
            {
                // A pressed, pause/unpause/start
                switch (timerData->timerState)
                {
                    case STOPPED:
                        timerData->accumulatedDuration = 0;
                        // Intentional fall-through
                    case PAUSED:
                    {
                        timerData->startTime  = now;
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
            else if (evt.button == PB_B)
            {
                // B pressed, pause/stop/reset
                // TODO
                switch (timerData->timerState)
                {
                    case PAUSED:
                    {
                        timerData->timerState          = STOPPED;
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
            else if ((evt.button == PB_LEFT || evt.button == PB_RIGHT))
            {
                if (timerData->timerState == STOPPED)
                {
                    timerData->stopwatch = !timerData->stopwatch;
                }
            }
            else if ((evt.button == PB_UP || evt.button == PB_DOWN))
            {
                if (timerData->timerState == STOPPED)
                {
                    if (!timerData->stopwatch)
                    {
                        timerData->heldArrow    = evt.button;
                        timerData->holdingArrow = true;
                        timerData->repeatTimer  = REPEAT_DELAY;
                        if (evt.button == PB_DOWN)
                        {
                            decTime();
                        }
                        else
                        {
                            incTime();
                        }
                    }
                }
            }
        }
        else
        {
            if (evt.button == PB_UP || evt.button == PB_DOWN)
            {
                timerData->heldArrow    = evt.state & (PB_UP | PB_DOWN);
                timerData->holdingArrow = false;
                timerData->repeatTimer  = 0;
            }
        }
    }

    clearPxTft();

    uint16_t stopwatchW = textWidth(&timerData->textFont, "Stopwatch");
    uint16_t timerW     = textWidth(&timerData->textFont, "Timer");

    drawText(&timerData->textFont, timerData->stopwatch ? c050 : c333, "Stopwatch", 40, 5);
    drawText(&timerData->textFont, timerData->stopwatch ? c333 : c050, "Timer", TFT_WIDTH - 40 - timerW, 5);

    int16_t wsgOffset = (timerData->textFont.height - timerData->dpadWsg.h) / 2;

    const char* aAction = startStr;
    const char* bAction = resetStr;

    if (timerData->timerState == STOPPED)
    {
        aAction = startStr;
        bAction = resetStr;
        // Left arrow
        drawWsg(&timerData->dpadWsg, 40 + stopwatchW + 1, 5 + wsgOffset, false, false, 270);
        // Right arrow
        drawWsg(&timerData->dpadWsg, TFT_WIDTH - 40 - timerW - timerData->dpadWsg.w - 1, 5 + wsgOffset, false, false,
                90);
    }
    else if (timerData->timerState == PAUSED)
    {
        aAction = startStr;
        bAction = resetStr;
    }
    else if (timerData->timerState == RUNNING)
    {
        aAction = pauseStr;
        bAction = pauseStr;
    }

    drawWsgSimple(&timerData->aWsg, 5, TFT_HEIGHT - 60 + wsgOffset);
    drawText(&timerData->textFont, c050, aAction, 5 + timerData->aWsg.w + 2, TFT_HEIGHT - 60 + wsgOffset);

    drawWsgSimple(&timerData->bWsg, TFT_WIDTH / 2, TFT_HEIGHT - 60 + wsgOffset);
    drawText(&timerData->textFont, c050, bAction, TFT_WIDTH / 2 + timerData->aWsg.w + 2, TFT_HEIGHT - 60 + wsgOffset);

    if (!timerData->stopwatch && timerData->timerState == STOPPED)
    {
        // up
        drawWsg(&timerData->dpadWsg, 5, TFT_HEIGHT - 30 + wsgOffset, false, false, 0);
        // down
        drawWsg(&timerData->dpadWsg, 5 + 2 + timerData->dpadWsg.w, TFT_HEIGHT - 30 + wsgOffset, false, true, 0);
        drawText(&timerData->textFont, c050, "+/-30 Seconds", 5 + 2 * (timerData->aWsg.w + 2),
                 TFT_HEIGHT - 30 + wsgOffset);
    }

    int64_t remaining = timerData->stopwatch ? timerData->accumulatedDuration
                                             : timerData->countdownTime - timerData->accumulatedDuration;

    if (timerData->timerState == RUNNING)
    {
        remaining += (timerData->stopwatch ? 1 : -1) * (now - timerData->startTime);
    }

    uint16_t remainingMillis = (remaining / 1000) % 1000;
    uint8_t remainingSecs    = (remaining / 1000000) % 60;
    uint8_t remainingMins    = (remaining / (60 * 1000000)) % 60;
    // Might as well...
    uint64_t remainingHrs = remaining / 3600000000;

    char buffer[64];
    if (remainingHrs > 0)
    {
        snprintf(buffer, sizeof(buffer), hoursMinutesSecondsFmt, remainingHrs, remainingMins, remainingSecs,
                 remainingMillis);
    }
    else
    {
        snprintf(buffer, sizeof(buffer), minutesSecondsFmt, remainingMins, remainingSecs, remainingMillis);
    }

    bool blink = (timerData->timerState == PAUSED && ((now % PAUSE_FLASH_SPEED) > PAUSE_FLASH_SHOW))
                 || (timerData->timerState == EXPIRED && ((now % EXPIRE_FLASH_SPEED) > EXPIRE_FLASH_SHOW));

    if (!blink)
    {
        uint16_t textX = (TFT_WIDTH - textWidth(&timerData->numberFont, buffer)) / 2;
        uint16_t textY = (TFT_HEIGHT - timerData->numberFont.height) / 2;
        textX          = drawText(&timerData->numberFont, c050, buffer, textX, textY);
    }
}

static void incTime(void)
{
    timerData->countdownTime += 30000000;
}

static void decTime(void)
{
    if (timerData->countdownTime >= 30000000)
    {
        timerData->countdownTime -= 30000000;
    }
}