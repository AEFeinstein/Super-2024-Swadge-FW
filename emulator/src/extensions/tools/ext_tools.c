#include "ext_tools.h"
#include "emu_ext.h"
#include "esp_timer.h"
#include "hdw-btn.h"
#include "hdw-btn_emu.h"
#include "hdw-imu.h"
#include "hdw-imu_emu.h"
#include "macros.h"
#include "emu_main.h"
#include "ext_replay.h"
#include "hdw-tft.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>
#include <unistd.h>

#include "hdw-tft_emu.h"

//==============================================================================
// Defines
//==============================================================================

//==============================================================================
// Enums
//==============================================================================

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
} toolsExt_t;

//==============================================================================
// Function Prototypes
//==============================================================================

static bool toolsInit(emuArgs_t* emuArgs);
static int32_t toolsKeyCb(uint32_t keycode, bool down);

static const char* getScreenshotName(char* buffer, size_t maxlen);

//==============================================================================
// Variables
//==============================================================================

emuExtension_t toolsEmuExtension = {
    .name            = "tools",
    .fnInitCb        = toolsInit,
    .fnPreFrameCb    = NULL,
    .fnPostFrameCb   = NULL,
    .fnKeyCb         = toolsKeyCb,
    .fnMouseMoveCb   = NULL,
    .fnMouseButtonCb = NULL,
    .fnRenderCb      = NULL,
};

toolsExt_t tools = {0};

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initialize the tools extension
 *
 * @param emuArgs
 * @return true
 */
static bool toolsInit(emuArgs_t* emuArgs)
{
    return true;
}

static int32_t toolsKeyCb(uint32_t keycode, bool down)
{
    static bool ctrl = false;
    static bool alt = false;
    static bool shift = false;

    if (keycode == EMU_KEY_CTRL)
    {
        ctrl = down;
        // don't consume
        return 0;
    }
    else if (keycode == EMU_KEY_ALT)
    {
        alt = down;
        // don't consume
        return 0;
    }
    else if (keycode == EMU_KEY_SHIFT)
    {
        shift = down;
        // don't consume
        return 0;
    }
    else if (keycode == EMU_KEY_F12)
    {
        static bool released = true;

        if (down)
        {
            if (released)
            {
                released = false;

                // Take a screenshot with an auto-generated filename
                takeScreenshot(NULL);

                // Consume
                return -1;
            }
        }
        else
        {
            released = true;
        }
    }
    else if (keycode == EMU_KEY_F8)
    {
        static int touchState = -1;

        if (touchState == -1)
        {
            touchState = emulatorArgs.emulateTouch ? 1 : 0;
        }

        if (!down)
        {
            if (touchState == 1)
            {
                if (disableExtension("touch"))
                {
                    touchState = 0;
                }
            }
            else
            {
                if (enableExtension("touch"))
                {
                    touchState = 1;
                }
            }

            return -1;
        }
    }
    else if (keycode == EMU_KEY_F9)
    {
        static int ledState = -1;
        if (ledState == -1)
        {
            ledState = emulatorArgs.hideLeds ? 0 : 1;
        }

        if (!down)
        {
            if (ledState == 1)
            {
                emulatorArgs.hideLeds = true;
                if (disableExtension("leds"))
                {
                    ledState = 0;
                }
            } else {
                emulatorArgs.hideLeds = false;
                if (enableExtension("leds"))
                {
                    ledState = 1;
                }
            }

            return -1;
        }
    }

    return 0;
}

/**
 * @brief Write a timestamp-based filename into the given buffer, formatted as "<prefix><timestamp>.<ext>"
 *
 * @param dst The buffer to write the timestamp into
 * @param n The maximum number of characters to write into dst
 * @param prefix The filename prefix to write before the timestamp
 * @param ext The file extension to write after the timestamp and a dot
 * @return const char* A pointer to the beginning of dst
 */
const char* getTimestampFilename(char* dst, size_t n, const char* prefix, const char* ext)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    // Turns out time_t doesn't printf well, so stick it in something that does
    uint64_t timeSec    = (uint64_t)ts.tv_sec;
    uint64_t timeMillis = (uint64_t)ts.tv_nsec / 1000000;

    uint32_t tries = 0;
    do
    {
        snprintf(dst, n, "%s%" PRIu64 "%03" PRIu64 ".%s", prefix, timeSec, timeMillis, ext);
        // Increment millis by one in case the file already exists
        timeMillis++;

        // Might as well handle the edge cases to avoid weird stuff
        if (timeMillis >= 1000000)
        {
            timeMillis %= 1000000;
            timeSec++;
        }

        // If the file exists, keep trying, up to 5 times, then just give up and overwrite it?
    } while (0 == access(dst, R_OK) && ++tries < 5);

    return dst;
}

static const char* getScreenshotName(char* buffer, size_t maxlen)
{
    return getTimestampFilename(buffer, maxlen, "screenshot-", "png");
}

/**
 * @brief Takes a screenshot and writes it to a PNG file with the given filename
 *
 * @param name The filename to write to. If NULL or empty, a timestamp-based filename will be used instead
 * @return true If the screenshot was successfully written
 * @return false If there was an error writing a screenshot
 */
bool takeScreenshot(const char* name)
{
    uint16_t width, height;
    uint32_t* bitmap = getDisplayBitmap(&width, &height);

    // We need to swap some channels for PNG output
    uint32_t converted[width * height];
    for (int row = 0; row < height; row++)
    {
        for (int col = 0; col < width; col++)
        {
            uint8_t r = (bitmap[row * width + col] >> 8) & 0xFF;
            uint8_t g = (bitmap[row * width + col] >> 16) & 0xFF;
            uint8_t b = (bitmap[row * width + col] >> 24) & 0xFF;
            uint8_t a = 0xFF;

            uint8_t* out = (uint8_t*)(&converted[row * width + col]);

            *(out++) = b;
            *(out++) = g;
            *(out++) = r;
            *(out++) = a;
        }
    }

    char buf[64];
    if (!name || !*name)
    {
        name = getScreenshotName(buf, sizeof(buf) - 1);
    }

    printf("Saving screenshot %s\n", name);

    // Notify the replay ext of the screenshot
    recordScreenshotTaken(name);

    // Add full transparency for the rounded corners
    plotRoundedCorners(converted, width, height, (width / TFT_WIDTH) * 40, 0x000000);

    return 0 != stbi_write_png(name, width, height, 4, converted, width * sizeof(uint32_t));
}
