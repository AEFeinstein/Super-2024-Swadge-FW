//==============================================================================
// Includes
//==============================================================================

#include <time.h>
#include <sys/time.h>
#include "swadgeClock.h"

//==============================================================================
// enums
//==============================================================================

typedef enum
{
    CL_YEAR,
    CL_MONTH,
    CL_DAY,
    CL_HOUR,
    CL_MINUTE,
    CL_SECOND,
    CL_MAX_FIELDS,
} clockField_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    bool settingTime;
    clockField_t beingAdjusted;
} swadgeClock_t;

//==============================================================================
// Function Prototypes
//==============================================================================

static void clockEnterMode(void);
static void clockExitMode(void);
static void clockMainLoop(int64_t elapsedUs);
static void clockBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);

//==============================================================================
// Const variables
//==============================================================================

static const char clockModeName[] = "Clock";

//==============================================================================
// Variables
//==============================================================================

swadgeMode_t clockMode = {
    .modeName                 = clockModeName,
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = false,
    .usesAccelerometer        = false,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = clockEnterMode,
    .fnExitMode               = clockExitMode,
    .fnMainLoop               = clockMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = clockBackgroundDrawCallback,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL,
    .fnDacCb                  = NULL,
    .fnAddToSwadgePassPacket  = NULL,
    .trophyData               = NULL,
};

swadgeClock_t* sc;

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief TODO doc
 *
 */
static void clockEnterMode(void)
{
    sc = heap_caps_calloc(1, sizeof(swadgeClock_t), MALLOC_CAP_8BIT);
}

/**
 * @brief TODO doc
 *
 */
static void clockExitMode(void)
{
    heap_caps_free(sc);
}

/**
 * @brief TODO doc
 *
 * @param elapsedUs
 */
static void clockMainLoop(int64_t elapsedUs)
{
    // Get the current time
    struct timeval tv = {0};
    gettimeofday(&tv, NULL);
    struct tm* tmTime = localtime(&tv.tv_sec);

    // Handle button inputs, which may adjust the time
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        if (evt.down)
        {
            switch (evt.button)
            {
                case PB_UP:
                case PB_DOWN:
                {
                    if (sc->settingTime)
                    {
                        int32_t dir = evt.button == PB_UP ? 1 : -1;
                        switch (sc->beingAdjusted)
                        {
                            case CL_YEAR:
                            {
                                tmTime->tm_year += dir;
                                break;
                            }
                            case CL_MONTH:
                            {
                                tmTime->tm_mon += dir;
                                break;
                            }
                            case CL_DAY:
                            {
                                tmTime->tm_mday += dir;
                                break;
                            }
                            case CL_HOUR:
                            {
                                tmTime->tm_hour += dir;
                                break;
                            }
                            case CL_MINUTE:
                            {
                                tmTime->tm_min += dir;
                                break;
                            }
                            case CL_SECOND:
                            {
                                tmTime->tm_sec += dir;
                                break;
                            }
                            default:
                            {
                                break;
                            }
                        }

                        tv.tv_sec = mktime(tmTime);
                        settimeofday(&tv, NULL);
                    }
                    break;
                }
                case PB_LEFT:
                {
                    if (sc->settingTime)
                    {
                        if (CL_YEAR == sc->beingAdjusted)
                        {
                            sc->beingAdjusted = CL_SECOND;
                        }
                        else
                        {
                            sc->beingAdjusted--;
                        }
                    }
                    break;
                }
                case PB_RIGHT:
                {
                    if (sc->settingTime)
                    {
                        if (CL_SECOND == sc->beingAdjusted)
                        {
                            sc->beingAdjusted = CL_YEAR;
                        }
                        else
                        {
                            sc->beingAdjusted++;
                        }
                    }
                    break;
                }
                case PB_A:
                case PB_B:
                case PB_START:
                case PB_SELECT:
                {
                    if (sc->settingTime)
                    {
                        // TODO save time
                        sc->settingTime = false;
                    }
                    else
                    {
                        sc->settingTime = true;
                    }
                    break;
                }
            }
        }
    }

    if (tmTime != NULL)
    {
        char dateStr[64] = {0};
        snprintf(dateStr, sizeof(dateStr) - 1, "%02d/%02d/%04d", //
                 tmTime->tm_mon + 1, tmTime->tm_mday, tmTime->tm_year + 1900);

        char timeStr[64] = {0};
        snprintf(timeStr, sizeof(timeStr) - 1, "%02d:%02d:%02d", //
                 tmTime->tm_hour, tmTime->tm_min, tmTime->tm_sec);

        drawText(getSysFont(), c554, timeStr, 20, 20);
        drawText(getSysFont(), c554, dateStr, 20, 20 + getSysFont()->height + 2);

        if (sc->settingTime)
        {
            char dbgStr[64];
            snprintf(dbgStr, sizeof(dbgStr) - 1, "Setting %d", sc->beingAdjusted);
            drawText(getSysFont(), c554, dbgStr, 20, 20 + 2 * (getSysFont()->height + 2));
        }
    }
}

/**
 * @brief TODO doc
 *
 * @param x
 * @param y
 * @param w
 * @param h
 * @param up
 * @param upNum
 */
static void clockBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
    memset(&(getPxTftFramebuffer()[(y * TFT_WIDTH) + x]), c001, w * h);
}
