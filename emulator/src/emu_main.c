//==============================================================================
// Includes
//==============================================================================

#include <unistd.h>
#ifdef __linux__
    #include <signal.h>
    #include <execinfo.h>
#endif
#include <time.h>

#include <esp_system.h>
#include <esp_timer.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

#include "hdw-tft.h"
#include "hdw-tft_emu.h"
#include "hdw-led.h"
#include "hdw-led_emu.h"
#include "hdw-bzr.h"
#include "hdw-btn.h"
#include "hdw-btn_emu.h"
#include "hdw-accel_emu.h"
#include "swadge2024.h"
#include "macros.h"
#include "trigonometry.h"

// Make it so we don't need to include any other C files in our build.
#define CNFG_IMPLEMENTATION
#define CNFGOGL
#include "rawdraw_sf.h"

// Useful if you're trying to find the code for a key/button
// #define DEBUG_INPUTS

#define EMU_EXTENSIONS

#include "emu_args.h"
#include "emu_ext.h"

#include "emu_main.h"

//==============================================================================
// Defines
//==============================================================================

#define MIN_LED_WIDTH 64
#define BG_COLOR      0x191919FF // This color isn't part of the palette
#define DIV_WIDTH     1
#define DIV_HEIGHT    1
#define DIV_COLOR     0x808080FF

//==============================================================================
// Macros
//==============================================================================

/**
 * @brief Macro to use once in each function to setup the loop, before using ::EMU_CB_LOOP()
 */
#define EMU_CB_SETUP \
    int cbCount;     \
    const emuCallback_t** cbList = getEmuCallbacks(&cbCount);

#define EMU_CB_LOOP_BARE    for (int i = 0; i < cbCount; i++)
#define EMU_CB_HAS_FN(cbFn) (cbList[i]->cbFn != NULL)
#define EMU_CB_NAME         (cbList[i]->name)

/**
 * @brief Macro to be used as a for-loop replacement for calling a particular callback
 *
 * This macro must only be used after ::EMU_CB_SETUP has been used in the current scope.
 * Use this macro as though it were \c for(;;) initializer -- with braces. The body will
 * only operate on ::emuCallback_t for which \c cbFn is not NULL.
 *
 * Example:
 * \code{.c}
 * void doCallbacks(void)
 * {
 *     EMU_CB_SETUP
 *     EMU_CB_LOOP(fnPreFrameCb)
 *     {
 *         EMU_CB(fnPreFrameCb);
 *     }
 * }
 * \endcode
 */
#define EMU_CB_LOOP(cbFn) \
    EMU_CB_LOOP_BARE      \
    if (cbEnabled[i] && EMU_CB_HAS_FN(cbFn))

/**
 * @brief Macro used to call \c cbFn on the current callback with any args inside of an ::EMU_CB_LOOP loop.
 */
#define EMU_CB(cbFn, ...) cbList[i]->cbFn(__VA_ARGS__)

//==============================================================================
// Structs
//==============================================================================

/**
 * @brief Struct representing a sub-pane in the main emulator window
 *
 */
typedef struct
{
    uint32_t paneW; ///< Width of the pane, or 0 if there is no pane
    uint32_t paneH; ///< Height of the pane, or 0 if there is no pane
    uint32_t paneX; ///< X offset of the pane
    uint32_t paneY; ///< Y offset of the pane
} emuPane_t;

/**
 * @brief Stores a pane-set's minimum area requirement and number of panes for convenience
 *
 */
typedef struct
{
    /**
     * @brief The minimum size of this pane's variable dimension (height for top/bottom, width for left/right)
     */
    uint32_t min;

    /**
     * @brief Stores the total number of sub-panes that were requested within this pane
     */
    uint32_t count;
} emuPaneInfo_t;

//==============================================================================
// Variables
//==============================================================================

static bool isRunning = true;

#ifdef EMU_EXTENSIONS

/// @brief Stores any panes associated with callbacks
static emuPane_t emuPanes[16] = {0};

/// @brief Stores whether each callback is enabled
static bool cbEnabled[16] = {false};
#endif

/// @brief Stores the size and location of the left LED panel, if any
static emuPane_t ledPaneLeft = {0};

/// @brief Stores the size and location of the right LED panel, if any
static emuPane_t ledPaneRight = {0};

//==============================================================================
// Function Prototypes
//==============================================================================

#ifdef __linux__
void init_crashSignals(void);
void signalHandler_crash(int signum, siginfo_t* si, void* vcontext);
#endif

static void drawBitmapPixel(uint32_t* bitmapDisplay, int w, int h, int x, int y, uint32_t col);
static void plotRoundedCorners(uint32_t* bitmapDisplay, int w, int h, int r, uint32_t col);
static void plotLeds(led_t* leds, int numLeds, emuPane_t* leftPane, emuPane_t* rightPane);

static void calculatePaneInfo(emuPaneInfo_t* paneInfos);
static void layoutWindow(int32_t winW, int32_t winH, int32_t screenW, int32_t screenH, emuPane_t* screenPane,
                         uint8_t* screenMult);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Parse and handle command line arguments
 *
 * @param argc The number of command line arguments
 * @param argv An array of null-terminated string arguments
 */
void handleArgs(int argc, char** argv)
{
    // Parse the arguments and check for errors
    if (!emuParseArgs(argc, argv))
    {
        // Error parsing, set isRunning to false
        isRunning = false;
    }
}

/**
 * @brief Main function to run the emulator
 *
 * @param argc The number of command line arguments
 * @param argv An array of null-terminated string arguments
 * @return 0 if the emulator exited normally, nonzero if there was an error
 */
int main(int argc, char** argv)
{
#ifdef __linux__
    init_crashSignals();
#endif

    handleArgs(argc, argv);

    if (!isRunning)
    {
        // For one reason or another, we're done after parsing args, so quit now
        return 0;
    }

#ifdef EMU_EXTENSIONS
    // Call any init callbacks we may have and pass them the parsed command-line arguments
    // We also determine which extensions are enabled here, which is important for laying out the window properly
    EMU_CB_SETUP
    EMU_CB_LOOP_BARE
    {
        // Mark an extension as enabled if it either has no init callback, or its init callback returns true
        if (EMU_CB_HAS_FN(fnInitCb))
        {
            printf("Initializing extension %s...\n", EMU_CB_NAME);
            cbEnabled[i] = EMU_CB(fnInitCb, &emulatorArgs);
            printf("Extension %s %s.\n", EMU_CB_NAME, cbEnabled[i] ? "initialized" : "disabled");
        }
        else
        {
            cbEnabled[i] = true;
            printf("Extension %s enabled\n", EMU_CB_NAME);
        }
    }
#endif

    // First initialize rawdraw
    // Screen-specific configurations
    // Save window dimensions from the last loop
    if (emulatorArgs.fullscreen)
    {
        CNFGSetupFullscreen("Swadge 2024 Simulator", 0);
    }
    else
    {
        // Get all the pane info to see how much space we need aside from the simulated TFT screen
        emuPaneInfo_t paneInfos[5] = {0};
        calculatePaneInfo(paneInfos);

        // Add the screen size to the minimum pane sizes to get our window size
        CNFGSetup("Swadge 2024 Simulator", (TFT_WIDTH)*2 + paneInfos[PANE_LEFT].min + paneInfos[PANE_RIGHT].min,
                  (TFT_HEIGHT)*2 + paneInfos[PANE_TOP].min + paneInfos[PANE_BOTTOM].min);
    }

    // We won't call the pre-frame callback for the very first frame
    // This is because everything isn't initialized and there would have to be emu-specific code to do so
    // post-initialization So, this is fine, we get one frame of peace before the emulator can start messing with stuff.

    // This is the 'main' that gets called when the ESP boots. It does not return
    app_main();
}

/**
 * @brief This is called from app_main() once each loop. This is effectively the emulator's main loop which handles key
 * inputs, drawing to the screen (both TFT and LEDs), and checking timers.
 */
void taskYIELD(void)
{
#ifdef EMU_EXTENSIONS
    // Count total frames, just for callback reasons
    static uint64_t frameNum = 0;

    // Call the post-frame callback
    EMU_CB_SETUP
    EMU_CB_LOOP(fnPostFrameCb)
    {
        EMU_CB(fnPostFrameCb, frameNum);
    }
#endif

    // Calculate time between calls
    static int64_t tLastCallUs = 0;
    int64_t tElapsedUs         = 0;
    if (0 == tLastCallUs)
    {
        tLastCallUs = esp_timer_get_time();
    }
    else
    {
        // Track the elapsed time between loop calls
        int64_t tNowUs = esp_timer_get_time();
        tElapsedUs     = tNowUs - tLastCallUs;
        tLastCallUs    = tNowUs;
    }

    // These are persistent!
    static short lastWindow_w = 0;
    static short lastWindow_h = 0;

    // Below: Support for pausing and unpausing the emulator
    // Keep track of whether we've called the pre-frame callbacks yet
    // bool preFrameCalled = false;
    // do {

    // Always handle inputs
    if (!CNFGHandleInput())
    {
        isRunning = false;
    }

    // If not running anymore, don't handle graphics
    // Must be checked after handling input, before graphics
    if (!isRunning)
    {
        deinitSystem();
        exit(0);
        return;
    }

    // Check things here which are called by interrupts or timers on the Swadge
    check_esp_timer(tElapsedUs);

    // Grey Background
    CNFGBGColor = BG_COLOR;
    CNFGClearFrame();

    // Get the current window dimensions
    short window_w, window_h;
    CNFGGetDimensions(&window_w, &window_h);
    static emuPane_t screenPane;

    // If the dimensions changed
    if ((lastWindow_h != window_h) || (lastWindow_w != window_w))
    {
        uint8_t screenMult;
        // Recalculate the window layout and get the settings for the screen
        layoutWindow(window_w, window_h, TFT_WIDTH, TFT_HEIGHT, &screenPane, &screenMult);

        // Set the multiplier
        setDisplayBitmapMultiplier(screenMult);

        // Save for the next loop
        lastWindow_w = window_w;
        lastWindow_h = window_h;
    }

    // Get the LED memory
    uint8_t numLeds;
    led_t* leds = getLedMemory(&numLeds);

    plotLeds(leds, numLeds, &ledPaneLeft, &ledPaneRight);

    // Draw dividing lines, if they're on-screen
    CNFGColor(DIV_COLOR);

    // Draw Left Divider
    if (screenPane.paneX > 0)
    {
        CNFGTackSegment(screenPane.paneX - 1, 0, screenPane.paneX - 1, window_h);
    }

    // Draw Right Divider
    if (screenPane.paneX + screenPane.paneW < window_w)
    {
        CNFGTackSegment(screenPane.paneX + screenPane.paneW, 0, screenPane.paneX + screenPane.paneW, window_h);
    }

    // Draw Top Divider
    if (screenPane.paneY > 0)
    {
        CNFGTackSegment(screenPane.paneX, screenPane.paneY - 1, screenPane.paneX + screenPane.paneW,
                        screenPane.paneY - 1);
    }

    // Draw Bottom Divider
    if (screenPane.paneY + screenPane.paneH < window_h)
    {
        CNFGTackSegment(screenPane.paneX, screenPane.paneY + screenPane.paneH, screenPane.paneX + screenPane.paneW,
                        screenPane.paneY + screenPane.paneH);
    }

    // Get the display memory
    uint16_t bitmapWidth, bitmapHeight;
    uint32_t* bitmapDisplay = getDisplayBitmap(&bitmapWidth, &bitmapHeight);

    if ((0 != bitmapWidth) && (0 != bitmapHeight) && (NULL != bitmapDisplay))
    {
#if defined(CONFIG_GC9307_240x280)
        plotRoundedCorners(bitmapDisplay, bitmapWidth, bitmapHeight, (bitmapWidth / TFT_WIDTH) * 40, BG_COLOR);
#endif
        // Update the display, centered
        CNFGBlitImage(bitmapDisplay, screenPane.paneX, screenPane.paneY, bitmapWidth, bitmapHeight);
    }

#ifdef EMU_EXTENSIONS
    // After the screen has been fully rendered, call all the render callbacks to render anything else
    EMU_CB_LOOP(fnRenderCb)
    {
        // Get the pane associated with this callback.
        // If there's no pane associated, the pane will just be all zeros
        emuPane_t* pane = emuPanes + i;
        if (cbEnabled[i] && pane->paneH > 0 && pane->paneW > 0)
        {
            EMU_CB(fnRenderCb, window_w, window_h, pane->paneW, pane->paneH, pane->paneX, pane->paneY);
        }
    }
#endif

    // Display the image and wait for time to display next frame.
    CNFGSwapBuffers();

    // Sleep for one ms
    static struct timespec tRemaining = {0};
    const struct timespec tSleep      = {
             .tv_sec  = 0 + tRemaining.tv_sec,
             .tv_nsec = 1000000 + tRemaining.tv_nsec,
    };
    nanosleep(&tSleep, &tRemaining);

#ifdef EMU_EXTENSIONS
    EMU_CB_LOOP(fnPreFrameCb)
    {
        EMU_CB(fnPreFrameCb, ++frameNum);
    }
#endif

    // Below: Support for pausing and unpausing the emulator
    // Note:  Remove the above EMU_CB_LOOP(fnPreFrameCb)... if uncommenting the below
    //     // Don't call the pre-frame callbacks until the emulator is unpaused.
    //     // And also, only call it once per frame
    //     // This means that the pre-frame callback gets called once (assuming the post-frame
    //     // callback didn't already pause) and then, if one of them pauses, they don't get called
    //     // again until after
    //     // be able to handle input as normal, which is good since that's the only way we'd
    //     if (!preFrameCalled && !emuTimerIsPaused())
    //     {
    //         preFrameCalled = true;
    //
    //         // Call the pre-frame callbacks just before we return to the swadge main loop
    //         // When fnPreFrameCb is first called, the system is always initialized
    //         // and this is the optimal time to inject button presses, pause, etc.
    //         EMU_CB_LOOP(fnPreFrameCb)
    //         {
    //             EMU_CB(fnPreFrameCb, ++frameNum);
    //         }
    //     }
    //
    //     // Set the elapsed micros to 0 so ESP timer tasks don't get called repeatedly if we pause
    //     // (if we just updated the time normally instead, this would be 0 anyway since time is paused, but this
    //     shortcuts that) tElapsedUs = 0;
    //
    //     // Make sure we stop if no longer running, but otherwise run until the emulator is unpaused
    //     // and the pre-frame callbacks have all been called
    // } while (isRunning && (!preFrameCalled || emuTimerIsPaused()));
}

/**
 * @brief Helper function to draw to a bitmap display
 *
 * @param bitmapDisplay The display to draw to
 * @param w The width of the display
 * @param h The height of the display
 * @param x The X coordinate to draw a pixel at
 * @param y The Y coordinate to draw a pixel at
 * @param col The color to draw the pixel
 */
static void drawBitmapPixel(uint32_t* bitmapDisplay, int w, int h, int x, int y, uint32_t col)
{
    if ((y * w) + x < (w * h))
    {
        bitmapDisplay[(y * w) + x] = col;
    }
}

/**
 * @brief Helper functions to draw rounded corners to the display
 *
 * @param bitmapDisplay The display to round the corners on
 * @param w The width of the display
 * @param h The height of the display
 * @param r The radius of the rounded corners
 * @param col The color to draw the rounded corners
 */
static void plotRoundedCorners(uint32_t* bitmapDisplay, int w, int h, int r, uint32_t col)
{
    int or = r;
    int x = -r, y = 0, err = 2 - 2 * r; /* bottom left to top right */
    do
    {
        for (int xLine = 0; xLine <= (or +x); xLine++)
        {
            drawBitmapPixel(bitmapDisplay, w, h, xLine, h - (or -y) - 1, col);         /* I.   Quadrant -x -y */
            drawBitmapPixel(bitmapDisplay, w, h, w - xLine - 1, h - (or -y) - 1, col); /* II.  Quadrant +x -y */
            drawBitmapPixel(bitmapDisplay, w, h, xLine, (or -y), col);                 /* III. Quadrant -x -y */
            drawBitmapPixel(bitmapDisplay, w, h, w - xLine - 1, (or -y), col);         /* IV.  Quadrant +x -y */
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

/**
 * @brief Plots the given LEDs within the given panes
 *
 * @param leds
 * @param numLeds
 * @param leftPane
 * @param rightPane
 */
static void plotLeds(led_t* leds, int numLeds, emuPane_t* leftPane, emuPane_t* rightPane)
{
    // Where LEDs are drawn, kinda
    // first value is the LED column (top-to-bottom(?))
    // second value is the row
    const int16_t ledOffsets[8][2] = {
        {1, 2}, {0, 3}, {0, 1}, {1, 0}, // Left side LEDs
        {2, 0}, {3, 1}, {3, 3}, {2, 2}, // Right side LEDs
    };

    // Draw simulated LEDs
    if (numLeds > 1 && NULL != leds && !emulatorArgs.hideLeds)
    {
        for (int i = 0; i < numLeds; i++)
        {
            emuPane_t* pane = (ledOffsets[i][0] < 2) ? leftPane : rightPane;

            int16_t ledH = pane->paneH / (numLeds / 2);
            int16_t ledW = pane->paneW / 2;

            int16_t xOffset = pane->paneX + (ledOffsets[i][0] % 2) * (ledW / 2);
            int16_t yOffset = ledOffsets[i][1] * ledH;

            CNFGColor((leds[i].r << 24) | (leds[i].g << 16) | (leds[i].b << 8) | 0xFF);
            CNFGTackRectangle(xOffset, yOffset, xOffset + ledW * 3 / 2, yOffset + ledH);
        }
    }
}

/**
 * @brief Helper function to calculate the minimum size needed for the display
 *
 * The results of the calculation will be written into \c paneInfos at the index
 * matching the value of ::emuCallback_t::paneLoc. \c paneInfos[0] will not be written
 * to since that corresponds to \c PANE_NONE.
 *
 * @param paneInfos A pointer to a 0-initialized array of at least 5 ::emuPaneInfo_t to be used as output.
 */
static void calculatePaneInfo(emuPaneInfo_t* paneInfos)
{
    // Handle the hardcoded LED panes
    if (!emulatorArgs.hideLeds)
    {
        paneInfos[PANE_LEFT].min = MIN_LED_WIDTH * 2;
        paneInfos[PANE_LEFT].count++;
        paneInfos[PANE_RIGHT].min = MIN_LED_WIDTH * 2;
        paneInfos[PANE_RIGHT].count++;
    }

#ifdef EMU_EXTENSIONS
    // Figure out the minimum required height/width of each pane side
    // For this we don't need to know anything about the
    EMU_CB_SETUP
    for (int i = 0; i < cbCount; i++)
    {
        /*if (!cbEnabled[i])
        {
            continue;
        }*/

        paneLocation_t paneLoc = cbList[i]->paneLocation;

        // make sure the callback wants a pane at all
        if (paneLoc != PANE_NONE && cbList[i]->minPaneW > 0 && cbList[i]->minPaneH > 0 && cbEnabled[i])
        {
            // There should be a pane associated with this callback
            switch (paneLoc)
            {
                case PANE_NONE:
                    // Do nothing, there's no pane, also this is "impossible"
                    break;

                case PANE_LEFT:
                case PANE_RIGHT:
                {
                    paneInfos[paneLoc].min = MAX(cbList[i]->minPaneW, paneInfos[paneLoc].min);
                    paneInfos[paneLoc].count++;
                    // minLeftPaneH += cbList[i]->minPaneH;
                    // minRightPaneH += cbList[i]->minPaneH;
                    break;
                }

                case PANE_TOP:
                case PANE_BOTTOM:
                {
                    paneInfos[paneLoc].min = MAX(cbList[i]->minPaneH, paneInfos[paneLoc].min);
                    paneInfos[paneLoc].count++;
                    // minTopPaneW += cbList[i]->minPaneW;
                    // minBottomPaneW += cbList[i]->minPaneH
                    break;
                }
            }
        }
    }
#endif
}

/**
 * @brief Calculates the position of all elements inside the main window and updates their locations and sizes.
 *
 * If winW and winH are negative,
 *
 * @param winW The total window width, in pixels. If <0,
 * @param winH The total window height, in pixels
 * @param screenW The actual width of the emulator screen, in pixels
 * @param screenH The actual height of the emulator screen, in pixels
 * @param screenPane A pointer to an ::emuPane_t to be updated with the screen location and dimensions
 * @param screenMult A pointer to an int to be updated with the screen scale multiplier
 */
static void layoutWindow(int32_t winW, int32_t winH, int32_t screenW, int32_t screenH, emuPane_t* screenPane,
                         uint8_t* screenMult)
{
    // The minimums will never change, so just calculate them once
    static bool paneInfosCalculated = false;

    static emuPaneInfo_t paneInfos[5] = {0};

    // We only need to calculate the pane infos once, since they only depend on the callbacks
    if (!paneInfosCalculated)
    {
        calculatePaneInfo(paneInfos);

        paneInfosCalculated = true;
    }

    // Figure out how much the screen should be scaled b
    uint8_t widthMult = (winW - paneInfos[PANE_LEFT].min - paneInfos[PANE_RIGHT].min) / screenW;
    if (0 == widthMult)
    {
        widthMult = 1;
    }

    uint8_t heightMult = (winH - paneInfos[PANE_TOP].min - paneInfos[PANE_BOTTOM].min) / screenH;
    if (0 == heightMult)
    {
        heightMult = 1;
    }

    // Set the scale to whichever dimension's multiplier was smallest
    *screenMult = MIN(widthMult, heightMult);

    // Update the screen pane size to the scaled size of the screen
    screenPane->paneW = screenW * (*screenMult);
    screenPane->paneH = screenH * (*screenMult);

    // These will hold the overall pane dimensions for easier logic
    emuPane_t winPanes[5] = {0};

    // The number of panes assigned in each area, for positioning subpanes
    uint8_t assigned[5] = {0};

    // Width/height of the dividers between the screen and each pane, if there are any
    uint32_t leftDivW   = DIV_WIDTH * (paneInfos[PANE_LEFT].count > 0);
    uint32_t rightDivW  = DIV_WIDTH * (paneInfos[PANE_RIGHT].count > 0);
    uint32_t topDivH    = DIV_HEIGHT * (paneInfos[PANE_TOP].count > 0);
    uint32_t bottomDivH = DIV_HEIGHT * (paneInfos[PANE_BOTTOM].count > 0);

    // Assign the remaining space to the left and right panes proportionally with their minimum sizes
    winPanes[PANE_LEFT].paneX = 0;
    winPanes[PANE_LEFT].paneY = 0;

    // Only set the pane dimensions if there are actually any panes, to avoid division-by-zero
    if (paneInfos[PANE_LEFT].count > 0)
    {
        winPanes[PANE_LEFT].paneW
            = MAX(0, (winW - rightDivW - leftDivW - screenPane->paneW) * (paneInfos[PANE_LEFT].min)
                         / (paneInfos[PANE_LEFT].min + paneInfos[PANE_RIGHT].min));
        winPanes[PANE_LEFT].paneH = winH;
    }

    // The screen will be just to the right of the left pane and its divider
    screenPane->paneX = winPanes[PANE_LEFT].paneW + leftDivW;

    // Assign whatever space is left to the right pane to account for rounding problems
    winPanes[PANE_RIGHT].paneX = screenPane->paneX + screenPane->paneW + rightDivW;
    winPanes[PANE_RIGHT].paneY = 0;
    if (paneInfos[PANE_RIGHT].count > 0)
    {
        winPanes[PANE_RIGHT].paneW
            = MAX(0, winW - (winPanes[PANE_LEFT].paneW + leftDivW + screenPane->paneW + rightDivW));
        winPanes[PANE_RIGHT].paneH = winH;
    }

    // Now do the horizontal panes, which have the same X and W as the screen
    winPanes[PANE_TOP].paneX = screenPane->paneX;
    winPanes[PANE_TOP].paneY = 0;
    if (paneInfos[PANE_TOP].count > 0)
    {
        winPanes[PANE_TOP].paneW = screenPane->paneW;
        // Assign the remaining space to the left and right panes proportionally with their minimum sizes
        winPanes[PANE_TOP].paneH = MAX(0, (winH - bottomDivH - topDivH - screenPane->paneH) * (paneInfos[PANE_TOP].min)
                                              / (paneInfos[PANE_TOP].min + paneInfos[PANE_BOTTOM].min));
    }

    // For the bottom one, flip things around just a bit so we can center the screen properly
    if (paneInfos[PANE_BOTTOM].count > 0)
    {
        winPanes[PANE_BOTTOM].paneW = screenPane->paneW;
        // Assign whatever space is left to the right pane to account for roundoff
        winPanes[PANE_BOTTOM].paneH
            = MAX(0, winH - (winPanes[PANE_TOP].paneH + topDivH + screenPane->paneH + bottomDivH));
    }

    // The screen will be just below the top pane and its divider, plus half of any extra space not used by the panes
    // (to center)
    screenPane->paneY
        = winPanes[PANE_TOP].paneH + topDivH
          + (winH - winPanes[PANE_TOP].paneH - winPanes[PANE_BOTTOM].paneH - screenPane->paneH - topDivH - bottomDivH)
                / 2;

    winPanes[PANE_BOTTOM].paneX = screenPane->paneX;
    winPanes[PANE_BOTTOM].paneY = screenPane->paneY + screenPane->paneH + bottomDivH;

///< Macro for calculating the offset of the current sub-pane within the overall pane
#define SUBPANE_OFFSET(side, hOrW) \
    ((paneInfos[side].count > 0) ? (assigned[side] * winPanes[side].pane##hOrW / paneInfos[side].count) : 0)

///< Macro for calculating the size of the currunt sub-pane within the overall pane
#define SUBPANE_SIZE(side, hOrW)                                                                                   \
    ((paneInfos[side].count > 0)                                                                                   \
         ? ((assigned[side] + 1) * winPanes[side].pane##hOrW / paneInfos[side].count - SUBPANE_OFFSET(side, hOrW)) \
         : 0)

    if (!emulatorArgs.hideLeds)
    {
        // Copy the base pane settings to the LED panes
        memcpy(&ledPaneLeft, winPanes + PANE_LEFT, sizeof(emuPane_t));
        memcpy(&ledPaneRight, winPanes + PANE_RIGHT, sizeof(emuPane_t));

        // We only need to change the heights if there are any other panes
        // Otherwise, the overall pane is already the correct dimensions for the LED pane
        if (paneInfos[PANE_LEFT].count > 1)
        {
            ledPaneLeft.paneY += SUBPANE_OFFSET(PANE_LEFT, H);
            ledPaneLeft.paneH = SUBPANE_SIZE(PANE_LEFT, H);
        }

        if (paneInfos[PANE_RIGHT].count > 1)
        {
            ledPaneRight.paneY += SUBPANE_OFFSET(PANE_RIGHT, H);
            ledPaneRight.paneH = SUBPANE_SIZE(PANE_RIGHT, H);
        }

        assigned[PANE_LEFT]++;
        assigned[PANE_RIGHT]++;
    }

    // One difference between the left/right and toplbottom sides that's now important:
    // Left/Right panes get the entire side
    // Top/Bottom panes only get the space under the screen...

    // Now, we actually apply all the dimensions we calculated to the panes
#ifdef EMU_EXTENSIONS
    EMU_CB_SETUP
    for (int i = 0; i < cbCount; i++)
    {
        paneLocation_t paneLoc = cbList[i]->paneLocation;
        if (paneLoc != PANE_NONE && cbList[i]->minPaneW > 0 && cbList[i]->minPaneH > 0)
        {
            ///< This is the enum for the location this pane is in -- TOP, BOTTOM, LEFT, RIGHT
            emuPane_t* cbPane = (emuPanes + i);

            // Copy the overall pane settings for the appropriate side onto the sub-pane for this callback
            memcpy(cbPane, (winPanes + paneLoc), sizeof(emuPane_t));

            // Now, we just
            switch (paneLoc)
            {
                case PANE_NONE:
                    // Do nothing, there's no pane, also this is "impossible"
                    break;

                case PANE_LEFT:
                case PANE_RIGHT:
                {
                    // Handle the left/right columns
                    // We just set the Y and Height
                    cbPane->paneY += SUBPANE_OFFSET(paneLoc, H);
                    cbPane->paneH = SUBPANE_SIZE(paneLoc, H);
                    break;
                }

                case PANE_TOP:
                case PANE_BOTTOM:
                {
                    cbPane->paneX += SUBPANE_OFFSET(paneLoc, W);
                    cbPane->paneW = SUBPANE_SIZE(paneLoc, W);
                    break;
                }
            }

            assigned[paneLoc]++;
        }
    }
#endif

#undef SUBPANE_OFFSET
#undef SUBPANE_SIZE
}

/**
 * This function must be provided for rawdraw. Key events are received here
 *
 * @param keycode The key code, a lowercase ascii char
 * @param bDown true if the key was pressed, false if it was released
 */
void HandleKey(int keycode, int bDown)
{
#ifdef DEBUG_INPUTS
    if (' ' <= keycode && keycode <= '~')
    {
        printf("HandleKey(keycode='%c', bDown=%s\n", keycode, bDown ? "true" : "false");
    }
    else
    {
        printf("HandleKey(keycode=%d, bDown=%s\n", keycode, bDown ? "true" : "false");
    }
#endif

    int32_t finalKey = keycode;

#ifdef EMU_EXTENSIONS
    EMU_CB_SETUP
    EMU_CB_LOOP(fnKeyCb)
    {
        int32_t newKey = EMU_CB(fnKeyCb, finalKey, bDown);

        // If the callback returns anything other than 0, we must handle it specially
        if (newKey != 0)
        {
            if (newKey < 0)
            {
                // The event was consumed by the handler, so stop processing now
                return;
            }
            else
            {
                // The key was replaced by the handler, so use that for any more callbacks
                finalKey = newKey;
            }
        }
    }
#endif

    // Assuming no callbacks canceled the key event earlier, handle it normally
    emulatorHandleKeys(finalKey, bDown);

    // When in fullscreen, exit with escape
    if (finalKey == CNFG_KEY_ESCAPE) // && emulatorArgs.fullscreen)
    {
        printf("Escape\n");
        isRunning = false;
        return;
    }
}

/**
 * @brief Handle mouse click events from rawdraw
 *
 * @param x The x coordinate of the mouse event
 * @param y The y coordinate of the mouse event
 * @param button The mouse button that was pressed or released
 * @param bDown true if the button was pressed, false if it was released
 */
void HandleButton(int x, int y, int button, int bDown)
{
#ifdef DEBUG_INPUTS
    printf("HandleButton(x=%d, y=%d, button=%x, bDown=%s\n", x, y, button, bDown ? "true" : "false");
#endif

#ifdef EMU_EXTENSIONS
    EMU_CB_SETUP
    EMU_CB_LOOP(fnMouseButtonCb)
    {
        // Convert button to its corresponding bitmask for simplicity and consistency
        if (EMU_CB(fnMouseButtonCb, x, y, 1 << (button - 1), bDown))
        {
            // Stop processing if the event was consumed
            return;
        }
    }
#endif
}

/**
 * @brief Handle mouse motion events from rawdraw
 *
 * @param x The x coordinate of the mouse event
 * @param y The y coordinate of the mouse event
 * @param mask A mask of mouse buttons that are currently held down
 */
void HandleMotion(int x, int y, int mask)
{
#ifdef DEBUG_INPUTS
    printf("HandleMotion(x=%d, y=%d, mask=%x\n", x, y, mask);
#endif

#ifdef EMU_EXTENSIONS
    EMU_CB_SETUP
    EMU_CB_LOOP(fnMouseMoveCb)
    {
        if (EMU_CB(fnMouseMoveCb, x, y, mask))
        {
            return;
        }
    }
#endif
}

/**
 * @brief Free memory on exit
 */
void HandleDestroy(void)
{
    isRunning = false;
    WARN_UNIMPLEMENTED();
}

#ifdef __linux__

/**
 * @brief Initialize a crash handler, only for Linux
 */
void init_crashSignals(void)
{
    const int sigs[] = {SIGSEGV, SIGBUS, SIGILL, SIGSYS, SIGABRT, SIGFPE, SIGIOT, SIGTRAP};
    for (int i = 0; i < sizeof(sigs) / sizeof(sigs[0]); i++)
    {
        struct sigaction action;
        memset(&action, 0, sizeof(struct sigaction));
        action.sa_flags     = SA_SIGINFO;
        action.sa_sigaction = signalHandler_crash;
        sigaction(sigs[i], &action, NULL);
    }
}

/**
 * @brief Print a backtrace when a crash is caught, only for Linux
 *
 * @param signum
 * @param si
 * @param vcontext
 */
void signalHandler_crash(int signum, siginfo_t* si, void* vcontext)
{
    char msg[128] = {'\0'};
    ssize_t result;

    char fname[64] = {0};
    sprintf(fname, "crash-%ld.txt", time(NULL));
    int dumpFileDescriptor
        = open(fname, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

    if (-1 != dumpFileDescriptor)
    {
        snprintf(msg, sizeof(msg), "Signal %d received!\nsigno: %d, errno %d\n, code %d\n", signum, si->si_signo,
                 si->si_errno, si->si_code);
        result = write(dumpFileDescriptor, msg, strnlen(msg, sizeof(msg)));
        (void)result;

        memset(msg, 0, sizeof(msg));
        for (int i = 0; i < __SI_PAD_SIZE; i++)
        {
            char tmp[8];
            snprintf(tmp, sizeof(tmp), "%02X", si->_sifields._pad[i]);
            tmp[sizeof(tmp) - 1] = '\0';
            strncat(msg, tmp, sizeof(msg) - strlen(msg) - 1);
        }
        strncat(msg, "\n", sizeof(msg) - strlen(msg) - 1);
        result = write(dumpFileDescriptor, msg, strnlen(msg, sizeof(msg)));
        (void)result;

        // Print backtrace
        void* array[128];
        size_t size = backtrace(array, (sizeof(array) / sizeof(array[0])));
        backtrace_symbols_fd(array, size, dumpFileDescriptor);
        close(dumpFileDescriptor);
    }

    // Exit
    _exit(1);
}
#endif
