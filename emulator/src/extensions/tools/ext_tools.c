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
#include "emu_args.h"
#include "ext_modes.h"
#include "emu_utils.h"
#include "emu_console_cmds.h"

#if defined(__clang__) || (defined(__GNUC__) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ > 5))))
    #pragma GCC diagnostic push
#endif
#ifdef __GNUC__
    #pragma GCC diagnostic ignored "-Wcast-qual"
    #pragma GCC diagnostic ignored "-Wmissing-prototypes"
#endif

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#if defined(__clang__) || (defined(__GNUC__) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ > 5))))
    #pragma GCC diagnostic pop
#endif

#include "gifenc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>

#include "hdw-tft_emu.h"
#include "esp_timer_emu.h"
#include "swadge2024.h"

//==============================================================================
// Function Prototypes
//==============================================================================

static bool toolsInit(emuArgs_t* emuArgs);
static int32_t toolsKeyCb(uint32_t keycode, bool down, modKey_t modifiers);
static void toolsPreFrame(uint64_t frame);
static void toolsPostFrame(uint64_t frame);
static void toolsRenderCb(uint32_t winW, uint32_t winH, const emuPane_t* panes, uint8_t numPanes);
static void handleConsoleCommand(const char* command);
static void makeTransparent(uint8_t* framebuffer);

static const char* getScreenshotName(char* buffer, size_t maxlen);

//==============================================================================
// Variables
//==============================================================================

emuExtension_t toolsEmuExtension = {
    .name            = "tools",
    .fnInitCb        = toolsInit,
    .fnPreFrameCb    = toolsPreFrame,
    .fnPostFrameCb   = toolsPostFrame,
    .fnKeyCb         = toolsKeyCb,
    .fnMouseMoveCb   = NULL,
    .fnMouseButtonCb = NULL,
    .fnRenderCb      = toolsRenderCb,
};

static bool useFakeTime       = false;
static uint64_t fakeTime      = 0;
static uint64_t fakeFrameTime = 0;

static bool recordScreen           = false;
static char recordingFilename[256] = {0};

static bool pauseNextFrame = false;

static bool showFps  = false;
static int fpsPaneId = -1;
static int64_t frameStartTime;
static int64_t frameTimes[120] = {0};
static const int frameTimeSize = sizeof(frameTimes) / sizeof(int64_t);
static int frameStartIndex     = 0;
static int frameEndIndex       = 0;

static int64_t lastFrameTime = 0;
static float lastFps         = 0.0;

static bool showConsole         = false;
static int consolePaneId        = -1;
static char consoleBuffer[1024] = {0};
static char* consolePtr         = consoleBuffer;

static char consoleOutput[1024] = {0};

//==============================================================================
// Functions
//==============================================================================

static bool toolsInit(emuArgs_t* emuArgs)
{
    if (emuArgs->fakeTime)
    {
        emuSetUseRealTime(false);
        useFakeTime = true;

        //
        fakeTime = 1;

        // Calculate the frame time time in us from the FPS value
        fakeFrameTime = (uint64_t)(1000000.0 / emulatorArgs.fakeFps);

        printf("Using fake frame rate of %.1fFPS -- %" PRIu64 "us per frame\n", emuArgs->fakeFps, fakeFrameTime);
    }

    if (emuArgs->showFps)
    {
        fpsPaneId      = requestPane(&toolsEmuExtension, PANE_BOTTOM, 30, 30);
        showFps        = true;
        frameStartTime = esp_timer_get_time();
    }

    return true;
}

static int32_t toolsKeyCb(uint32_t keycode, bool down, modKey_t modifiers)
{
    if (showConsole)
    {
        if (!down)
        {
            if (keycode == CNFG_KEY_F4)
            {
                showConsole = false;
                setPaneVisibility(&toolsEmuExtension, consolePaneId, false);
                emuTimerUnpause();
            }
            else if (keycode == CNFG_KEY_BACKSPACE)
            {
                if (consolePtr > consoleBuffer)
                {
                    *--consolePtr = '\0';
                }
            }
            else if (keycode == CNFG_KEY_ENTER)
            {
                // Handle console command
                handleConsoleCommand(consoleBuffer);

                consolePtr  = consoleBuffer;
                *consolePtr = '\0';
            }
            else if (' ' <= keycode && keycode <= '~')
            {
                if ((('A' <= keycode && keycode <= 'Z') || ('a' <= keycode && keycode <= 'z'))
                    && (modifiers & EMU_MOD_SHIFT))
                {
                    keycode ^= 32;
                }
                *consolePtr++ = (char)(keycode & 0x7F);
                *consolePtr   = '\0';
            }
        }

        // Consume all input until console is closed
        return -1;
    }
    else if (!down && keycode == CNFG_KEY_F4)
    {
        showConsole = true;
        /*if (consolePaneId == -1)
        {
            consolePaneId = requestPane(&toolsEmuExtension, PANE_TOP, 10, 32);
        }*/
        consolePtr     = consoleBuffer;
        *consolePtr    = '\0';
        *consoleOutput = '\0';

        setPaneVisibility(&toolsEmuExtension, consolePaneId, true);
        emuTimerPause();
    }

    if (keycode == CNFG_KEY_F12)
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
    else if (keycode == CNFG_KEY_F5)
    {
        // Toggle FPS counter
        if (!down)
        {
            showFps = !showFps;

            if (showFps && fpsPaneId == -1)
            {
                fpsPaneId = requestPane(&toolsEmuExtension, PANE_BOTTOM, 30, 30);
            }
            setPaneVisibility(&toolsEmuExtension, fpsPaneId, showFps);
        }
    }
    else if (keycode == CNFG_KEY_F9)
    {
        // Single-frame step
        if (!down && emuTimerIsPaused())
        {
            printf("Advancing one frame, time is now %" PRIu64 "\n", fakeTime);
            // Unpause for one frame
            emuTimerUnpause();
            pauseNextFrame = true;
        }
    }
    else if (keycode == CNFG_KEY_F10)
    {
        static bool pausing = false;

        if (down && !pausing)
        {
            pausing = true;
            if (emuTimerIsPaused())
            {
                printf("Unpausing\n");
                emuTimerUnpause();
            }
            else
            {
                printf("Pausing\n");
                emuTimerPause();
            }
        }
        else if (!down)
        {
            pausing = false;
        }
    }
    else if (keycode == CNFG_KEY_F11)
    {
        if (down)
        {
            if (recordScreen)
            {
                stopScreenRecording();
            }
            else
            {
                startScreenRecording(NULL);
            }
        }
    }
    else if (useFakeTime && keycode == CNFG_KEY_PAGE_UP) // && modifiers == EMU_MOD_CTRL)
    {
        // Speed up frames
        if (!down)
        {
            if (modifiers == EMU_MOD_SHIFT)
            {
                fakeFrameTime += 5000;
            }
            else if (modifiers == EMU_MOD_CTRL)
            {
                fakeFrameTime += 100000;
            }
            else
            {
                fakeFrameTime += 1000;
            }
            printf("Frame time set to %" PRIu64 "\n", fakeFrameTime);
        }
    }
    else if (useFakeTime && keycode == CNFG_KEY_PAGE_DOWN) // && modifiers == EMU_MOD_CTRL)
    {
        // Slow down frames
        if (!down)
        {
            if (modifiers == EMU_MOD_SHIFT && fakeFrameTime > 5000)
            {
                fakeFrameTime -= 5000;
            }
            else if (modifiers == EMU_MOD_CTRL && fakeFrameTime > 100000)
            {
                fakeFrameTime -= 100000;
            }
            else if (fakeFrameTime > 1000)
            {
                fakeFrameTime -= 1000;
            }
            printf("Frame time set to %" PRIu64 "\n", fakeFrameTime);
        }
    }
    else if (useFakeTime && keycode == CNFG_KEY_HOME)
    {
        fakeFrameTime = getFrameRateUs();
    }

    return 0;
}

static void toolsPreFrame(uint64_t frame)
{
    // handle fake clock
    if (useFakeTime)
    {
        emuSetEspTimerTime(fakeTime);
        fakeTime += fakeFrameTime;
    }

    if (pauseNextFrame)
    {
        emuTimerPause();
        pauseNextFrame = false;
    }

    // track the frame time in the buffer
    int64_t now               = esp_timer_get_time();
    frameTimes[frameEndIndex] = now - frameStartTime;
    frameStartTime            = now;

    // if the buffer is full, advance the start index
    if (((frameEndIndex + 1) % frameTimeSize) == frameStartIndex)
    {
        frameStartIndex = (frameStartIndex + 1) % frameTimeSize;
    }

    // advance the end index
    frameEndIndex = (frameEndIndex + 1) % frameTimeSize;

    // calculate the actual time
    int64_t totalLength = 0;
    int totalFrames     = 0;
    for (int i = frameStartIndex; i != frameEndIndex; i = (i + 1) % frameTimeSize)
    {
        totalLength += frameTimes[i];
        totalFrames++;
    }

    lastFrameTime = (totalLength) / (totalFrames);
    lastFps       = 1000000.0 * totalFrames / totalLength;
}

static void toolsPostFrame(uint64_t frame)
{
    static uint64_t index = 0;
    static bool setup     = false;
    static ge_GIF* gif    = NULL;

    // Cumulative time of all GIF frames
    static float gifTime = 0;
    // Cumulative real time since first frame, from swadge perspective
    static float realTime = 0;

    // The time of the last GIF frame
    static int64_t lastGifFrame   = 0;
    static uint32_t skippedFrames = 0;
    static uint32_t longFrames    = 0;

    if (recordScreen)
    {
        static const paletteColor_t* fb = NULL;
        if (!fb)
        {
            fb = getLastTftBitmap();
        }

        if (!setup)
        {
            if (!recordingFilename[0])
            {
                getTimestampFilename(recordingFilename, sizeof(recordingFilename) - 1, "screen-recording-", "gif");
            }

            uint8_t gifPalette[256 * 3];
            for (int i = 0; i < 256; i++)
            {
                uint8_t* r = &gifPalette[i * 3];
                uint8_t* g = &gifPalette[i * 3 + 1];
                uint8_t* b = &gifPalette[i * 3 + 2];

                if (i < cTransparent)
                {
                    uint32_t rgb = paletteToRGB((paletteColor_t)i);
                    *r           = (rgb >> 16) & 0xFF;
                    *g           = (rgb >> 8) & 0xFF;
                    *b           = (rgb & 0xFF);
                }
                else
                {
                    *r = 0;
                    *g = 0;
                    *b = 0;
                }
            }

            gif = ge_new_gif(recordingFilename, TFT_WIDTH, TFT_HEIGHT, gifPalette, 8, cTransparent, 0);
            if (!gif)
            {
                printf("ERR! ext_tools.c: Unable to write to file %s for recording.\n", recordingFilename);
                return;
            }

            index         = 0;
            gifTime       = 0;
            realTime      = 0;
            skippedFrames = 0;
            longFrames    = 0;
            lastGifFrame  = esp_timer_get_time();
            setup         = true;
        }

        int64_t frameNow       = esp_timer_get_time();
        int64_t elapsedFrameUs = frameNow - lastGifFrame;

        memcpy(gif->frame, fb, TFT_WIDTH * TFT_HEIGHT);

        // Make the corners of the image transparent
        makeTransparent(gif->frame);
        float desiredFramerate = elapsedFrameUs / 10000.0;
        uint16_t actualLength  = MAX(2, (int)(desiredFramerate));
        // Cumulative time error, relative to the minimum frame time
        // Use this to determine when to drop frames, as we cannot drop half a frame
        float frameError = (gifTime - realTime) / ((float)actualLength * 10.0);
        // Cumulative time error, relative to the minimum frame *STEP* time
        // Use this to determine when a frame is too short, since we can always ADD a single step
        float gifLengthError = (gifTime - realTime) / 10.0;

        if (index != 0)
        {
            realTime += elapsedFrameUs / 1000;
        }

        if (frameError <= 0.5)
        {
            if (index != 0)
            {
                if (gifLengthError <= -1)
                {
                    actualLength += (uint16_t)(-gifLengthError);
                    longFrames++;
                }

                gifTime += actualLength * 10.0;
            }
            ge_add_frame(gif, actualLength);

            // Only update the last frame time when we actually emit a frame!
            lastGifFrame = frameNow;

            // Count frames
            index++;
        }
        else
        {
            skippedFrames++;
        }
    }
    else if (setup)
    {
        ge_close_gif(gif);
        gif = NULL;

        printf("Done Recording! Wrote %" PRIu64 " frames to %s (dropped %" PRIu32 ", lengthened %" PRIu32 ")\n", index,
               recordingFilename, skippedFrames, longFrames);
        setup = false;
    }
}

static void toolsRenderCb(uint32_t winW, uint32_t winH, const emuPane_t* panes, uint8_t numPanes)
{
    for (int i = 0; i < numPanes; i++)
    {
        int paneId = panes[i].id;

        if (paneId == fpsPaneId)
        {
            const emuPane_t* fpsPane = &panes[i];
            CNFGColor(0xFFFFFFFF);
            char buf[64];
            snprintf(buf, sizeof(buf), "%.2f FPS", lastFps);

            int w, h;
            CNFGGetTextExtents(buf, &w, &h, 5);
            CNFGPenX = fpsPane->paneX + (fpsPane->paneW - w) / 2;
            CNFGPenY = fpsPane->paneY + (fpsPane->paneH - h) / 2;
            CNFGDrawText(buf, 5);
        }
    }

    if (showConsole)
    {
        // const emuPane_t* consolePane = &panes[i];
        char buf[1030];
        buf[0] = '>';
        buf[1] = ' ';
        strcpy(buf + 2, consoleBuffer);

        // Measure text (mostly for height here)
        int w, h;
        CNFGGetTextExtents(consoleBuffer, &w, &h, 5);

        // Transparent outline
        CNFGColor(0x00000000);

        // 50% opacity black background
        CNFGDialogColor = 0x0000007f;
        CNFGDrawBox(0, 0, winW, h + 5);

        // Draw text
        CNFGColor(0xFFFFFFFF);
        CNFGPenX = 5;
        CNFGPenY = 5;
        CNFGDrawText(buf, 5);

        if (*consoleOutput)
        {
            int startY = h + 5;

            CNFGGetTextExtents(consoleOutput, &w, &h, 5);

            // Transparent outline
            CNFGColor(0x00000000);

            // 50% opacity black background
            CNFGDialogColor = 0x0000007f;
            CNFGDrawBox(0, startY, winW, startY + h + 5);

            CNFGColor(0xFFFFFFFF);
            CNFGPenY = startY;
            CNFGPenX = 5;
            CNFGDrawText(consoleOutput, 5);
        }
    }
}

static void handleConsoleCommand(const char* command)
{
    char tmpBuffer[sizeof(consoleBuffer)];
    const char* cur = command;
    char* out       = tmpBuffer;
    char* argStart  = out;

    // Divide the command at spaces and store a pointer to the start of each one
    const char* values[64];
    int argCount = 0;

    while (out < (tmpBuffer + sizeof(tmpBuffer)) && argCount < (sizeof(values) / sizeof(*values)))
    {
        if (*cur == ' ' || *cur == '\0')
        {
            // Next arg
            *out++ = '\0';

            values[argCount++] = argStart;
            argStart           = (out);

            if (*cur)
            {
                cur++;
            }
            else
            {
                break;
            }
        }
        else
        {
            // Just copy
            *out++ = *cur++;
        }
    }
    *out = '\0';

    if (out == tmpBuffer)
    {
        return;
    }

    printf("Got %d args:\n", argCount);
    for (int i = 0; i < argCount; i++)
    {
        printf(" - %s\n", values[i]);
    }

    if (argCount > 0)
    {
        for (const consoleCommand_t* action = getConsoleCommands();
             action < (getConsoleCommands() + consoleCommandCount()); action++)
        {
            if (!strcmp(action->name, values[0]))
            {
                int outputLen = action->cb(values + 1, argCount - 1, consoleOutput);

                consoleOutput[outputLen] = '\0';
                return;
            }
        }

        snprintf(consoleOutput, sizeof(consoleOutput), "Unknown command: %s", values[0]);
    }
    else
    {
        consoleOutput[0] = '\0';
    }
}

// This is copy-pasted a lot from plotRoundedCorners() but oh well it's pretty different
static void makeTransparent(uint8_t* framebuffer)
{
    int r  = 40;
    int or = r;
    int x = -r, y = 0, err = 2 - 2 * r; /* bottom left to top right */
    do
    {
        for (int xLine = 0; xLine <= (or +x); xLine++)
        {
            framebuffer[(TFT_HEIGHT - (or -y) - 1) * TFT_WIDTH + (xLine)] = cTransparent; /* I.   Quadrant -x -y */
            framebuffer[(TFT_HEIGHT - (or -y) - 1) * TFT_WIDTH + (TFT_WIDTH - xLine - 1)]
                = cTransparent;                                                        /* II.  Quadrant +x -y */
            framebuffer[(or -y) * TFT_WIDTH + (xLine)]                 = cTransparent; /* III. Quadrant -x -y */
            framebuffer[(or -y) * TFT_WIDTH + (TFT_WIDTH - xLine - 1)] = cTransparent; /* IV.  Quadrant +x -y */
        }

        r = err;
        if (r <= y)
        {
            err += ++y * 2 + 1; /* e_xy+e_y < 0 */
        }
        if (r > x || err > y) /* e_xy+e_x > 0 or no 2nd y-step */
        {
            err += ++x * 2 + 1; /* -> x-step now */
        }
    } while (x < 0);
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
    uint32_t* bitmap    = getDisplayBitmap(&width, &height);
    bool timerWasPaused = emuTimerIsPaused();

    if (!timerWasPaused)
    {
        emuTimerPause();
    }

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
    else if (strlen(name) <= 4 || strncmp(name + strlen(name) - 4, ".png", 4))
    {
        snprintf(buf, sizeof(buf), "%s.png", name);
        name = buf;
    }

    printf("Saving screenshot %s\n", name);

    // Notify the replay ext of the screenshot
    recordScreenshotTaken(name);

    // Add full transparency for the rounded corners
    plotRoundedCorners(converted, width, height, (width / TFT_WIDTH) * 40, 0x000000);

    int res = stbi_write_png(name, width, height, 4, converted, width * sizeof(uint32_t));

    if (!timerWasPaused)
    {
        emuTimerUnpause();
    }

    return 0 != res;
}

void startScreenRecording(const char* name)
{
    if (name && *name)
    {
        if (strlen(name) <= 4 || strncmp(name + strlen(name) - 4, ".gif", 4))
        {
            snprintf(recordingFilename, sizeof(recordingFilename), "%s.gif", name);
        }
        else
        {
            strncpy(recordingFilename, name, sizeof(recordingFilename));
            recordingFilename[sizeof(recordingFilename) - 1] = '\0';
        }
    }
    else
    {
        getTimestampFilename(recordingFilename, sizeof(recordingFilename), "screen-recording-", "gif");
    }

    recordScreen = true;
}

void stopScreenRecording(void)
{
    recordScreen = false;
}

bool isScreenRecording(void)
{
    return recordScreen;
}