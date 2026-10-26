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
    CL_HOUR,
    CL_MINUTE,
    CL_SECOND,
    CL_MONTH,
    CL_DAY,
    CL_YEAR,
    CL_MAX_FIELDS,
} clockField_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    bool settingTime;
    clockField_t beingAdjusted;
    font_t clockFont;
    font_t noteFont;
    int32_t timeXoff;
    int32_t dateXoff;
    int32_t maxWidth;
    int32_t intOffsets[10];
    int32_t ySpacing;
    bool drawArrows;
    int32_t blinkTimer;
} swadgeClock_t;

//==============================================================================
// Function Prototypes
//==============================================================================

static void clockEnterMode(void);
static void clockExitMode(void);
static void clockMainLoop(int64_t elapsedUs);
static void clockBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);
static uint8_t drawClockChar(char c, int x, int y, paletteColor_t color);

//==============================================================================
// Const variables
//==============================================================================

static const char clockModeName[] = "Clock";
static const char timeSep         = ':';
static const char dateSep         = '/';
static const char setTimeStr[]    = "Press A to set the time";
static const char saveTimeStr[]   = "Press A to save the time";
static const char clockAM[]       = "AM";
static const char clockPM[]       = "PM";

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
 * @brief Initialize the clock by allocating resources and measuring the font and screen
 */
static void clockEnterMode(void)
{
    sc = heap_caps_calloc(1, sizeof(swadgeClock_t), MALLOC_CAP_8BIT);

    // Load fonts
    loadFont(PIX_ROMANA_FONT, &sc->clockFont, false);
    loadFont(MINISHCAP_FONT, &sc->noteFont, false);

    // Find the width of the largest integer
    sc->maxWidth = 0;
    for (int32_t i = 0; i < 10; i++)
    {
        uint8_t cWidth = sc->clockFont.chars['0' + i - ' '].width;
        sc->maxWidth   = MAX(sc->maxWidth, cWidth);
    }

    // Find the offset to center each integer
    for (int32_t i = 0; i < 10; i++)
    {
        uint8_t cWidth    = sc->clockFont.chars['0' + i - ' '].width;
        sc->intOffsets[i] = (sc->maxWidth - cWidth) / 2;
    }

    // Find the width of a time string and the X offset to center i
    int32_t timeSepW = sc->clockFont.chars[(int)(timeSep - ' ')].width;
    int32_t timeW    = (sc->maxWidth * 6) + (timeSepW * 2) + 8 + textWidth(&sc->clockFont, clockPM);
    sc->timeXoff     = (TFT_WIDTH - timeW) / 2;

    // Find the width of a date string and the X offset to center i
    int32_t dateSepW = sc->clockFont.chars[(int)(dateSep - ' ')].width;
    int32_t dateW    = (sc->maxWidth * 8) + (dateSepW * 2) + 9;
    sc->dateXoff     = (TFT_WIDTH - dateW) / 2;

    // Find the spacing between lines
    int32_t totalHeight = (2 * sc->clockFont.height) + sc->noteFont.height;
    int32_t blankHeight = TFT_HEIGHT - totalHeight;
    sc->ySpacing        = blankHeight / 4;
}

/**
 * @brief Exit the clock mode and free resources
 */
static void clockExitMode(void)
{
    freeFont(&sc->clockFont);
    freeFont(&sc->noteFont);
    heap_caps_free(sc);
}

/**
 * @brief Main loop which checks button inputs and draws to the screen
 *
 * @param elapsedUs The time elapsed since last call
 */
static void clockMainLoop(int64_t elapsedUs)
{
    // Blink arrows
    RUN_TIMER_EVERY(sc->blinkTimer, 500000, elapsedUs, { sc->drawArrows = !sc->drawArrows; });

    // Get the current time
    struct timeval tv = {0};
    gettimeofday(&tv, NULL);

    time_t epochSec   = tv.tv_sec;
    struct tm* tmTime = localtime(&epochSec);

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
                        // Adjust the field up or down
                        int32_t dir = evt.button == PB_UP ? 1 : -1;

                        // Adjust the specific field
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

                        // Convert the adjusted time back to epoch
                        tv.tv_sec  = mktime(tmTime);
                        tv.tv_usec = 0;

                        // Make sure that it's a valid time
                        if (tv.tv_sec >= 0)
                        {
                            // settimeofday() doesn't work (or exist?) for certain OSs, so only call this in firmware
#if defined(__XTENSA__)
                            // Set the system time to the new time
                            settimeofday(&tv, NULL);
#endif
                        }
                    }
                    break;
                }
                case PB_LEFT:
                {
                    if (sc->settingTime)
                    {
                        // Move adjusted field left, with wraparound
                        if (CL_HOUR == sc->beingAdjusted)
                        {
                            sc->beingAdjusted = CL_YEAR;
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
                        // Move adjusted field right, with wraparound
                        if (CL_YEAR == sc->beingAdjusted)
                        {
                            sc->beingAdjusted = CL_HOUR;
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
                    // Any of these buttons toggle setting time
                    sc->settingTime = !sc->settingTime;
                    break;
                }
            }
        }
    }

    // Get the time again, potentially after buttons change it
    gettimeofday(&tv, NULL);
    epochSec = tv.tv_sec;
    tmTime   = localtime(&epochSec);

    // Set up where to start drawing
    int32_t xOff = sc->timeXoff;
    int32_t yOff = sc->ySpacing;

    // Pick AM/PM and adjust the hour
    const char* ampm = clockAM;
    if (tmTime->tm_hour > 12)
    {
        ampm = clockPM;
        tmTime->tm_hour -= 12;
    }

    // Convert the time integers into a char array
    char timeDigits[64];
    snprintf(timeDigits, sizeof(timeDigits) - 1, "%02d%02d%02d", //
             tmTime->tm_hour, tmTime->tm_min, tmTime->tm_sec);

    // Draw each character in the time with the separator
    for (int i = 0; i < 6; i++)
    {
        drawClockChar(timeDigits[i], xOff + sc->intOffsets[timeDigits[i] - '0'], yOff, c554);
        xOff += sc->maxWidth + 1;

        if ((i % 2) && (i < 5))
        {
            xOff += (drawClockChar(timeSep, xOff, yOff, c554) + 1);
        }
    }
    // Then draw AM or PM
    drawText(&sc->clockFont, c554, ampm, xOff, yOff);

    // Move to the next line
    yOff += (sc->clockFont.height + sc->ySpacing);
    xOff = sc->dateXoff;

    // Convert the date integers into a char array
    char dateDigits[64];
    snprintf(dateDigits, sizeof(dateDigits) - 1, "%02d%02d%04d", //
             1 + tmTime->tm_mon, tmTime->tm_mday, 1900 + tmTime->tm_year);

    // Draw each character in the date with the separator
    for (int i = 0; i < 8; i++)
    {
        drawClockChar(dateDigits[i], xOff + sc->intOffsets[dateDigits[i] - '0'], yOff, c554);
        xOff += sc->maxWidth + 1;

        if ((i % 2) && (i < 5))
        {
            xOff += (drawClockChar(dateSep, xOff, yOff, c554) + 1);
        }
    }

    // Move to the next line
    yOff += (sc->clockFont.height + sc->ySpacing);

    if (sc->settingTime)
    {
        // Draw string, centered
        int32_t tWidth = textWidth(&sc->noteFont, saveTimeStr);
        drawText(&sc->noteFont, c554, saveTimeStr, (TFT_WIDTH - tWidth) / 2, yOff);

        // Arrows blink
        if (sc->drawArrows)
        {
            const vec_t arrowIdxs[6] = {
                {.x = 64, .y = 21}, {.x = 113, .y = 21}, {.x = 161, .y = 21},
                {.x = 59, .y = 96}, {.x = 117, .y = 96}, {.x = 198, .y = 96},
            };

            // Draw top arrow
            int tx = arrowIdxs[sc->beingAdjusted].x;
            int ty = arrowIdxs[sc->beingAdjusted].y;
            drawTriangleOutlined(tx, ty, tx - 8, ty + 8, tx + 8, ty + 8, c445, c445);

            // Draw bottom arrow
            ty += 58;
            drawTriangleOutlined(tx, ty, tx - 8, ty - 8, tx + 8, ty - 8, c544, c544);
        }
    }
    else
    {
        // Draw string, centered
        int32_t tWidth = textWidth(&sc->noteFont, setTimeStr);
        drawText(&sc->noteFont, c554, setTimeStr, (TFT_WIDTH - tWidth) / 2, yOff);
    }
}

/**
 * @brief Background draw callback, just fill a color
 *
 * @param x The X offset to draw at
 * @param y The Y offset to draw at
 * @param w The width to draw
 * @param h The height to draw
 * @param up The current number of the update call
 * @param upNum The total number of update calls for this frame
 */
static void clockBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
    memset(&(getPxTftFramebuffer()[(y * TFT_WIDTH) + x]), c001, w * h);
}

/**
 * @brief Helper to draw a single char
 *
 * @param c The character to draw
 * @param x The X offset to draw at
 * @param y The Y offset to draw at
 * @param color The color to draw
 * @return The width of the drawn character
 */
static uint8_t drawClockChar(char c, int x, int y, paletteColor_t color)
{
    font_ch_t* ch = &sc->clockFont.chars[(int)(c - ' ')];
    drawChar(color, sc->clockFont.height, ch, x, y);
    return ch->width + 1;
}

/**
 * @brief Set the default time if system seconds are 0
 */
void setDefaultTime(void)
{
    // Get the current time
    struct timeval tv = {0};
    gettimeofday(&tv, NULL);

    if (0 == tv.tv_sec)
    {
        struct tm magTime = {
            .tm_year = 2027 - 1900,
            .tm_mon  = 1 - 1,
            .tm_mday = 7,
            .tm_hour = 0,
            .tm_min  = 0,
            .tm_sec  = 0,
        };
        tv.tv_sec = mktime(&magTime);
        // settimeofday() doesn't work (or exist?) for certain OSs, so only call this in firmware
#if defined(__XTENSA__)
        settimeofday(&tv, NULL);
#endif
    }
}