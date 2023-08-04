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

#include "emu_main.h"

//==============================================================================
// Defines
//==============================================================================

#define MIN_LED_WIDTH         64
#define MOTION_CONTROL_HEIGHT 128
#define MOTION_CONTROL_WIDTH  128
#define BG_COLOR              0x191919FF // This color isn't part of the palette
#define DIV_COLOR             0x808080FF

//==============================================================================
// Variables
//==============================================================================

#define ARG_DVORAK        "dvorak"
#define ARG_FULLSCREEN    "fullscreen"
#define ARG_HIDE_LEDS     "hide-leds"
#define ARG_MOTION        "motion"
#define ARG_MOTION_DRIFT  "motion-drift"
#define ARG_MOTION_JITTER "motion-jitter"
#define ARG_TOUCH         "touch"
#define ARG_HELP          "help"

static const char keyboardLayoutDvorak[] = ",OAENTRC&[{}(";

static const char helpUsage[]
    = "usage: %s [--fullscreen] [--hide-leds] [--dvorak] [--motion [--motion-jitter] [--motion-drift]] [--help]\n"
      "Emulates a swadge\n"
      "\n"
      "--" ARG_DVORAK "\t\tuse controls mapped for dvorak instead of qwerty\n"
      "--" ARG_FULLSCREEN "\t\topen in fullscreen mode\n"
      "--" ARG_HIDE_LEDS "\t\tdon't draw simulated LEDs on the sides of the window\n"
      "--" ARG_MOTION "\t\tsimulate accelerometer readings with virtual trackball\n"
      "--" ARG_MOTION_DRIFT "\tsimulate accelerometer readings drifting over time\n"
      "--" ARG_MOTION_JITTER " [MAX=3]\tsimulate accelerometer readings randomly varying slightly from the true value\n"
      "--" ARG_TOUCH "\t\tsimulate touch sensor readings with a virtual touch-pad\n"
      "--" ARG_HELP "\t\t\tdisplay this help message and exit\n";

emuArgs_t emulatorArgs = {
    .fullscreen    = false,
    .hideLeds      = false,
    .emulateMotion = false,
    .motionZ       = 0,
    .motionX       = 0,
};

static bool isRunning = true;

static bool dragging        = false;
static int32_t dragX        = 0;
static int32_t dragY        = 0;
static int16_t startMotionZ = 0;
static int16_t startMotionX = 0;

//==============================================================================
// Function Prototypes
//==============================================================================

#ifdef __linux__
void init_crashSignals(void);
void signalHandler_crash(int signum, siginfo_t* si, void* vcontext);
#endif

static void drawBitmapPixel(uint32_t* bitmapDisplay, int w, int h, int x, int y, uint32_t col);
static void plotRoundedCorners(uint32_t* bitmapDisplay, int w, int h, int r, uint32_t col);
static void plotMotionControl(int w, int h, int x, int y, int zRotation, int xRotation);

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
#define MATCH_ARG(argName) (!strncmp(arg, "--" argName, sizeof("--" argName)))
#define REQUIRE_PARAM(name)                                                             \
    if (param == NULL)                                                                  \
    {                                                                                   \
        fprintf(stderr, "Error: Missing required parameter for argument '%s'\n", name); \
        isRunning = false;                                                              \
        break;                                                                          \
    }                                                                                   \
    else                                                                                \
    {                                                                                   \
        valUsed = true;                                                                 \
    }

#define INT_VALUE(name) _##name##Value
#define REQUIRE_INT(name)                                                                                \
    errno              = 0;                                                                              \
    int _##name##Value = strtol(param, NULL, 0);                                                         \
    if (errno != 0)                                                                                      \
    {                                                                                                    \
        fprintf(stderr, "Error: Invalid integer parameter value '%s' for argument '%s'\n", param, name); \
        isRunning = false;                                                                               \
        break;                                                                                           \
    }                                                                                                    \
    else                                                                                                 \
    {                                                                                                    \
        valUsed = true;                                                                                  \
    }

    for (int n = 1; n < argc; n++)
    {
        char* arg    = argv[n];
        char* param  = NULL;
        bool valUsed = false;

        // If there's another argument, and it doesn't start with a '-'
        if (n + 1 < argc && strncmp(argv[n + 1], "-", 1))
        {
            param = argv[n + 1];
        }

        if (MATCH_ARG(ARG_DVORAK))
        {
            emulatorSetKeyMap(keyboardLayoutDvorak);
        }
        else if (MATCH_ARG(ARG_FULLSCREEN))
        {
            emulatorArgs.fullscreen = true;
        }
        else if (MATCH_ARG(ARG_HIDE_LEDS))
        {
            emulatorArgs.hideLeds = true;
        }
        else if (MATCH_ARG(ARG_MOTION))
        {
            emulatorArgs.emulateMotion = true;
        }
        else if (MATCH_ARG(ARG_MOTION_DRIFT))
        {
            emulatorArgs.motionDrift = true;
        }
        else if (MATCH_ARG(ARG_MOTION_JITTER))
        {
            emulatorArgs.motionJitter = true;

            if (param != NULL)
            {
                REQUIRE_INT(ARG_MOTION_JITTER)
                emulatorArgs.motionJitterAmount = INT_VALUE(ARG_MOTION_JITTER);
            }
        }
        else if (MATCH_ARG(ARG_TOUCH))
        {
            emulatorArgs.emulateTouch = true;
        }
        else if (MATCH_ARG(ARG_HELP))
        {
            printf(helpUsage, *argv);
            isRunning = false;
            break;
        }
        else
        {
            fprintf(stderr, "Warning: Unrecognized argument '%s'\n", arg);
        }

        // If we used a parameter value here, don't try to parse it as an argument
        if (valUsed)
        {
            n++;
        }
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
        CNFGSetup("Swadge 2024 Simulator", (TFT_WIDTH * 2) + (emulatorArgs.hideLeds ? 0 : ((MIN_LED_WIDTH * 4) + 2)),
                  (TFT_HEIGHT * 2) + (emulatorArgs.emulateMotion ? MOTION_CONTROL_HEIGHT : 0));
    }

    // This is the 'main' that gets called when the ESP boots. It does not return
    app_main();
}

/**
 * @brief This is called from app_main() once each loop. This is effectively the emulator's main loop which handles key
 * inputs, drawing to the screen (both TFT and LEDs), and checking timers.
 */
void taskYIELD(void)
{
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
    static int16_t led_w      = MIN_LED_WIDTH;

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

    // If the dimensions changed
    if ((lastWindow_h != window_h) || (lastWindow_w != window_w))
    {
        // Figure out how much the TFT should be scaled by
        uint8_t widthMult = (window_w - (emulatorArgs.hideLeds ? 0 : ((4 * MIN_LED_WIDTH) - 2))) / TFT_WIDTH;
        if (0 == widthMult)
        {
            widthMult = 1;
        }
        uint8_t heightMult = (window_h - (emulatorArgs.emulateMotion ? MOTION_CONTROL_HEIGHT : 0)) / TFT_HEIGHT;
        if (0 == heightMult)
        {
            heightMult = 1;
        }
        uint8_t screenMult = MIN(widthMult, heightMult);

        // LEDs take up the rest of the horizontal space
        led_w = emulatorArgs.hideLeds ? 0 : (window_w - 2 - (screenMult * TFT_WIDTH)) / 4;

        // Set the multiplier
        setDisplayBitmapMultiplier(screenMult);

        // Save for the next loop
        lastWindow_w = window_w;
        lastWindow_h = window_h;
    }

    // Get the LED memory
    uint8_t numLeds;
    led_t* leds = getLedMemory(&numLeds);

    // Where LEDs are drawn, kinda
    const int16_t ledOffsets[8][2] = {
        {1, 2}, {0, 3}, {0, 1}, {1, 0}, {2, 0}, {3, 1}, {3, 3}, {2, 2},
    };

    // Draw simulated LEDs
    if (numLeds > 1 && NULL != leds && !emulatorArgs.hideLeds)
    {
        short led_h = window_h / (numLeds / 2);
        for (int i = 0; i < numLeds; i++)
        {
            CNFGColor((leds[i].r << 24) | (leds[i].g << 16) | (leds[i].b << 8) | 0xFF);

            int16_t xOffset = 0;
            if (ledOffsets[i][0] < 2)
            {
                xOffset = ledOffsets[i][0] * (led_w / 2);
            }
            else
            {
                xOffset = window_w - led_w - ((4 - ledOffsets[i][0]) * (led_w / 2));
            }

            int16_t yOffset = ledOffsets[i][1] * led_h;

            // Draw the LED
            CNFGTackRectangle(xOffset, yOffset, xOffset + (led_w * 3) / 2, yOffset + led_h);
        }
    }

    // Draw dividing lines
    if (!emulatorArgs.hideLeds)
    {
        CNFGColor(DIV_COLOR);
        CNFGTackSegment(led_w * 2, 0, led_w * 2, window_h);
        CNFGTackSegment(window_w - (led_w * 2), 0, window_w - (led_w * 2), window_h);
    }

    if (emulatorArgs.emulateMotion)
    {
        CNFGColor(DIV_COLOR);
        CNFGTackSegment(emulatorArgs.hideLeds ? 0 : (led_w * 2), window_h - MOTION_CONTROL_HEIGHT,
                        emulatorArgs.hideLeds ? window_w : window_w - (led_w * 2), window_h - MOTION_CONTROL_HEIGHT);
        plotMotionControl(MOTION_CONTROL_WIDTH, MOTION_CONTROL_HEIGHT, (window_w - MOTION_CONTROL_WIDTH) / 2,
                          window_h - MOTION_CONTROL_HEIGHT, emulatorArgs.motionZ, emulatorArgs.motionX);
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
        CNFGBlitImage(bitmapDisplay, emulatorArgs.hideLeds ? ((window_w - bitmapWidth) / 2) : ((led_w * 2) + 1),
                      emulatorArgs.emulateMotion ? (window_h - MOTION_CONTROL_HEIGHT - bitmapHeight) / 2
                                                 : (window_h - bitmapHeight) / 2,
                      bitmapWidth, bitmapHeight);
    }

    // Display the image and wait for time to display next frame.
    CNFGSwapBuffers();

    // Sleep for one ms
    static struct timespec tRemaining = {0};
    const struct timespec tSleep      = {
             .tv_sec  = 0 + tRemaining.tv_sec,
             .tv_nsec = 1000000 + tRemaining.tv_nsec,
    };
    nanosleep(&tSleep, &tRemaining);
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
 * @brief Helper function to render an indicator for the simulated acceleration vector
 *
 * The X axis lies within the plane of the swadge's circuit board, parallel with the line
 * passing through the Select and Start buttons, with +X being towards the A and B buttons
 * on the right and -X being towards the D-pad to the left.
 * The Y axis lies within the plane of the swadge's circuit board, perpendicular to the
 * X axis, with +Y being towards the lanyard hole at the top of the swadge, and with -Y
 * being towards the microphone at the bottom of the swadge.
 * The Z axis is perpendicular to the plane of the swadge's circuit board, with +Z being
 * on the front side, and -Z being the back side.
 *
 * @param w The width of the indicator, in pixels
 * @param h The height of the indicator, in pixels
 * @param x The X location of the top-left of the indicator
 * @param y The Y location of the top-left of the indicator
 * @param zRotation The clockwise rotation about the +Z (vertical) axis, in degrees
 * @param xRotation The clockwise rotation about the +X (horizontal) axis, in degrees
 */
static void plotMotionControl(int w, int h, int x, int y, int zRotation, int xRotation)
{
#define CALC_CIRCLE_POLY(buf, tris, xo, yo, r)                           \
    do                                                                   \
    {                                                                    \
        for (int i = 0; i < (tris); i++)                                 \
        {                                                                \
            buf[i].x = (xo) + getCos1024(i * 360 / (tris)) * (r) / 1024; \
            buf[i].y = (yo) + getSin1024(i * 360 / (tris)) * (r) / 1024; \
        }                                                                \
    } while (false)

    RDPoint circle[72];
    CALC_CIRCLE_POLY(circle, 72, x + w / 2, y + h / 2, w / 2);

    CNFGColor(0xFFFFFFFF);
    CNFGTackPoly(circle, 72);
#undef TRIS

    // The Z axis, which by default would be just a dot in the middle of the circle, is pushed from the center towards
    // the bottom with a +X rotation, and towards the top with a -X rotation Then, with a Z rotation, the position of
    // the Z dot simply follows the rotation So, to draw the +Z dot...
    // 1. Get the distance from the middle of the circle, which is based on the X rotation so zPosRadius = (h/2) *
    // sin(xRotation) / 1024
    // 2. Get the X and Y from the rotation of that, which is zPosX = (x + h/2) + zPosRadius * cos(zRotation) / 1024,
    // zPosY = zPosradius * sin(zRotation) / 1024
    // 3. Note that these values are for when xRotation <= 90 || xRotation >= 270; for the other cases, invert and swap
    // cos and sin, making zPos = (x + w / 2) - zPosRadius * sin(zRotation) / 1024, zPosY = (y + h / 2) - zPosRadius *
    // cos(zRotation) / 1024
    uint16_t zPosRadius = (h / 2) * sin(xRotation) / 1024;

    uint16_t zOffX = zPosRadius * cos(zRotation) / 1024;
    uint16_t zOffY = zPosRadius * sin(zRotation) / 1024;

    bool zPosVisible = (xRotation <= 90 || xRotation >= 270);
    uint16_t zPosX   = x + (w / 2) + zPosVisible ? zOffX : -zOffY;
    uint16_t zPosY   = y + (h / 2) + zPosVisible ? zOffY : -zOffX;

    // Draw smaller circle with less precision
    CALC_CIRCLE_POLY(circle, 4, zPosX, zPosY, 3);

    CNFGColor(0xFF0000FF);
    CNFGTackPoly(circle, 4);
    CNFGTackPixel(zPosX, zPosY);

    // printf("the Z axis dot is at %d, %d relative to %d, %d\n", zOffX, zOffY, x + (w / 2), y + (h / 2));
}

/**
 * This function must be provided for rawdraw. Key events are received here
 *
 * @param keycode The key code, a lowercase ascii char
 * @param bDown true if the key was pressed, false if it was released
 */
void HandleKey(int keycode, int bDown)
{
    emulatorHandleKeys(keycode, bDown);
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
    WARN_UNIMPLEMENTED();
    if (button == 1)
    {
        // Left-mouse click
        if (bDown)
        {
            if (emulatorArgs.emulateMotion)
            {
                // Get the current window dimensions
                short window_w, window_h;
                CNFGGetDimensions(&window_w, &window_h);

                int left   = (window_w - MOTION_CONTROL_WIDTH) / 2;
                int top    = window_h - MOTION_CONTROL_HEIGHT;
                int right  = left + MOTION_CONTROL_WIDTH;
                int bottom = window_h;

                if (left <= x && x <= right && top <= y && y <= bottom)
                {
                    printf("dragStart(%d,%d)\n", x, y);
                    dragX        = x;
                    dragY        = y;
                    startMotionZ = emulatorArgs.motionZ;
                    startMotionX = emulatorArgs.motionX;
                    dragging     = true;
                }
            }
        }
        else
        {
            if (dragging)
            {
                printf("dragEnd(%d,%d, %d,%d)\n", x, y, dragX, dragY);

                dragging = false;
            }
        }
    }
    else if (button == 4)
    {
        // Right-mouse click
        if (bDown)
        {
            emulatorArgs.motionX = 0;
            emulatorArgs.motionZ = 0;
        }
    }
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
    // left-click drag
    if (mask == 1 && dragging)
    {
        int diffX = (x - dragX) / 2;
        int diffY = (y - dragY) / 2;

        if (diffX < 5 && diffX > -5)
        {
            diffX = 0;
        }

        if (diffY < 5 && diffY > -5)
        {
            diffY = 0;
        }

        emulatorSetAccelerometerRotation(
            256, (emulatorArgs.motionX + diffX > 0 ? diffX % 360 : 360 - abs(diffX) % 360) % 360,
            (emulatorArgs.motionZ + diffY > 0 ? diffY % 360 : 360 - abs(diffY) % 360) % 360);
    }
    else if (mask != 0)
    {
        // 0x01 == LEFT_CLICK
        // 0x02 == MIDDLE_CLICK
        // 0x03 == RIGHT_CLICK
        // 0x08 == SCROLL_UP
        // 0x10 == SCROLL_DOWN
        // 0x20 == SCROLL_LEFT
        // 0x40 == SCROLL_RIGHT

        // printf("mask=%x, %d, %d\n", mask, x, y);
    }
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
