//==============================================================================
// Includes
//==============================================================================

#include <time.h>
#include "swadgeClock.h"

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

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief TODO doc
 *
 */
static void clockEnterMode(void)
{
}

/**
 * @brief TODO doc
 *
 */
static void clockExitMode(void)
{
}

/**
 * @brief TODO doc
 *
 * @param elapsedUs
 */
static void clockMainLoop(int64_t elapsedUs)
{
    time_t secondsSinceEpoch = time(NULL);
    struct tm* dateTime      = gmtime(&secondsSinceEpoch);

    if (dateTime != NULL)
    {
        char dateStr[64] = {0};
        snprintf(dateStr, sizeof(dateStr) - 1, "%02d/%02d/%04d", //
                 dateTime->tm_mon + 1, dateTime->tm_mday, dateTime->tm_year + 1900);

        char timeStr[64] = {0};
        snprintf(timeStr, sizeof(timeStr) - 1, "%02d:%02d:%02d", //
                 dateTime->tm_hour, dateTime->tm_min, dateTime->tm_sec);

        drawText(getSysFont(), c554, timeStr, 20, 20);
        drawText(getSysFont(), c554, dateStr, 20, 20 + getSysFont()->height + 2);
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
