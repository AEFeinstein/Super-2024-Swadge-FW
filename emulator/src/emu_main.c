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

#ifdef ENABLE_GCOV
    #include <gcov.h>
#endif

#include "hdw-tft.h"
#include "hdw-tft_emu.h"
#include "hdw-led.h"
#include "hdw-led_emu.h"
#include "hdw-bzr.h"
#include "hdw-btn.h"
#include "hdw-btn_emu.h"
#include "hdw-imu_emu.h"
#include "swadge2024.h"
#include "macros.h"
#include "trigonometry.h"

#include "hdw-esp-now.h"
#include "mainMenu.h"

// Make it so we don't need to include any other C files in our build.
#define CNFG_IMPLEMENTATION
#define CNFGOGL
#include "rawdraw_sf.h"

// Useful if you're trying to find the code for a key/button
// #define DEBUG_INPUTS

#include "emu_args.h"
#include "emu_ext.h"
#include "emu_main.h"

//==============================================================================
// Defines
//==============================================================================

#define BG_COLOR  0x191919FF // This color isn't parjt of the palette
#define DIV_COLOR 0x808080FF

//==============================================================================
// Variables
//==============================================================================

static bool isRunning = true;

//==============================================================================
// Function Prototypes
//==============================================================================

#ifdef __linux__
void init_crashSignals(void);
void signalHandler_crash(int signum, siginfo_t* si, void* vcontext);
#endif

static void drawBitmapPixel(uint32_t* bitmapDisplay, int w, int h, int x, int y, uint32_t col);
static void plotRoundedCorners(uint32_t* bitmapDisplay, int w, int h, int r, uint32_t col);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Quits the emulator
 *
 */
void emulatorQuit(void)
{
    isRunning = false;
}

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

    // Call any init callbacks we may have and pass them the parsed command-line arguments
    // We also determine which extensions are enabled here, which is important for laying out the window properly
    initExtensions(&emulatorArgs);

    if (!isRunning)
    {
        // One of the extension must have quit due to an error.
        return 0;
    }

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
        emuPaneMinimum_t paneMins[4] = {0};
        calculatePaneMinimums(paneMins);
        int32_t sidePanesW      = paneMins[PANE_LEFT].min + paneMins[PANE_RIGHT].min;
        int32_t topBottomPanesH = paneMins[PANE_TOP].min + paneMins[PANE_BOTTOM].min;
        int32_t winW            = (TFT_WIDTH) * 2 + sidePanesW;
        int32_t winH            = (TFT_HEIGHT) * 2 + topBottomPanesH;

        if (emulatorArgs.headless)
        {
            // If the window dimensions are negative, the window will still exist but not be displayed.
            // TODO does this work on all platforms?
            winW = -winW;
            winH = -winH;
        }

        // Add the screen size to the minimum pane sizes to get our window size
        CNFGSetup("Swadge 2024 Simulator", winW, winH);
    }

    // We won't call the pre-frame callback for the very first frame
    // This is because everything isn't initialized and there would have to be emu-specific code to do so
    // post-initialization So, this is fine, we get one frame of peace before the emulator can start messing with stuff.

    // This is a hack to make sure ESPNOW is always initialized on the emulator.
    // The real swadge initializes wifi on boot only if the mode requires it,
    // but the emulator doesn't actually reboot, so instead we just change the mode of the
    // main menu mode to force ESPNOW to always initialize when the emulator starts
    mainMenuMode.wifiMode = ESP_NOW;

    // This is the 'main' that gets called when the ESP boots. It does not return
    app_main();
}

/**
 * @brief This is called from app_main() once each loop. This is effectively the emulator's main loop which handles key
 * inputs, drawing to the screen (both TFT and LEDs), and checking timers.
 */
void taskYIELD(void)
{
    // Count total frames, just for callback reasons
    static uint64_t frameNum = 0;
    doExtPostFrameCb(frameNum);

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
        CNFGTearDown();

#ifdef ENABLE_GCOV
        __gcov_dump();
#endif

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
        layoutPanes(window_w, window_h, TFT_WIDTH, TFT_HEIGHT, &screenPane, &screenMult);

        // Set the multiplier
        setDisplayBitmapMultiplier(screenMult);

        // Save for the next loop
        lastWindow_w = window_w;
        lastWindow_h = window_h;
    }

    // Draw dividing lines, if they're on-screen
    CNFGColor(DIV_COLOR);

    emuPaneMinimum_t paneMins[4];
    calculatePaneMinimums(paneMins);

    // Draw Left Divider
    if (paneMins[PANE_LEFT].count > 0)
    {
        CNFGTackSegment(screenPane.paneX - 1, 0, screenPane.paneX - 1, window_h);
    }

    // Draw Right Divider
    if (paneMins[PANE_RIGHT].count > 0)
    {
        CNFGTackSegment(screenPane.paneX + screenPane.paneW, 0, screenPane.paneX + screenPane.paneW, window_h);
    }

    // Draw Top Divider
    if (paneMins[PANE_TOP].count > 0)
    {
        CNFGTackSegment(screenPane.paneX, screenPane.paneY - 1, screenPane.paneX + screenPane.paneW,
                        screenPane.paneY - 1);
    }

    // Draw Bottom Divider
    if (paneMins[PANE_BOTTOM].count > 0)
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

    // After the screen has been fully rendered, call all the render callbacks to render anything else
    doExtRenderCb(window_w, window_h);

    // Display the image and wait for time to display next frame.
    CNFGSwapBuffers();

    // Sleep for one ms
    static struct timespec tRemaining = {0};
    const struct timespec tSleep      = {
             .tv_sec  = 0 + tRemaining.tv_sec,
             .tv_nsec = 1000000 + tRemaining.tv_nsec,
    };
    nanosleep(&tSleep, &tRemaining);

    doExtPreFrameCb(++frameNum);

    // Below: Support for pausing and unpausing the emulator
    // Note:  Remove the above doExtPreFrameCb()... if uncommenting the below
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
    //         doExtPreFrameCb(++frameNum);
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

    keycode = doExtKeyCb(keycode, bDown);
    if (keycode < 0)
    {
        return;
    }

    // Assuming no callbacks canceled the key event earlier, handle it normally
    emulatorHandleKeys(keycode, bDown);

    // Keep track of when shift is held so we can exit on SHIFT + BACKSPACE
    // I would do CTRL+W, but rawdraw doesn't have a define for ctrl...
    static bool shiftDown = false;
    if (keycode == CNFG_KEY_SHIFT)
    {
        shiftDown = bDown;
    }

    // When in fullscreen, exit with Escape
    // And any time with Shift + Backspace
    if ((keycode == CNFG_KEY_ESCAPE && emulatorArgs.fullscreen) || (keycode == CNFG_KEY_BACKSPACE && shiftDown))
    {
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

    doExtMouseButtonCb(x, y, button, bDown);
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

    doExtMouseMoveCb(x, y, mask);
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
