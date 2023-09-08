/**
 * @file pushy.c
 * @author Brycey92
 * @brief A port of Socks' Pushy Kawaii 2
 * @date 2023-09-07
 */

//==============================================================================
// Includes
//==============================================================================

//#include "esp_random.h"
#include "pushy.h"

//==============================================================================
// Defines
//==============================================================================

#define NUM_DIGITS 8

//==============================================================================
// Enums
//==============================================================================

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    font_t logbook;       ///< The font used in the menu and game

    uint32_t score;        ///< The score for the game

    int32_t lastSaveUs;
    uint16_t btnState;    ///< The button state

    int32_t ledFadeTimer; ///< The timer to fade LEDs
} pushy_t;

//==============================================================================
// Function Prototypes
//==============================================================================

static void pushyMainLoop(int64_t elapsedUs);
static void pushyEnterMode(void);
static void pushyExitMode(void);

static void pushyFadeLeds(int64_t elapsedUs);

static void pushyBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);

//==============================================================================
// Strings
//==============================================================================

/* Design Pattern!
 * These strings are all declared 'const' because they do not change, so that they are placed in ROM, not RAM.
 * Lengths are not explicitly given so the compiler can figure it out.
 */

static const char pushyName[] = "Pushy";

//==============================================================================
// Variables
//==============================================================================

/// The Swadge mode for Pushy
swadgeMode_t pushyMode = {
    .modeName                 = pushyName,
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = false,
    .usesAccelerometer        = false,
    .usesThermometer          = false,
    .fnEnterMode              = pushyEnterMode,
    .fnExitMode               = pushyExitMode,
    .fnMainLoop               = pushyMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = pushyBackgroundDrawCallback,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL,
};

/// All state information for the Pushy mode. This whole struct is calloc()'d and free()'d so that Pushy is only
/// using memory while it is being played
pushy_t* pushy = NULL;

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Enter Pushy mode, allocate required memory, and initialize required variables
 *
 */
static void pushyEnterMode(void)
{
    // Allocate and clear all memory for this mode. All the variables are contained in a single struct for convenience.
    // calloc() is used instead of malloc() because calloc() also initializes the allocated memory to zeros.
    pushy = calloc(1, sizeof(pushy_t));

    // Load a font
    loadFont("logbook.font", &pushy->logbook, false);
}

/**
 * This function is called when the mode is exited. It deinitializes variables and frees all memory.
 */
static void pushyExitMode(void)
{
    // Free the font
    freeFont(&pushy->logbook);
    // Free everything else
    free(pushy);
}

/**
 * @brief This function is called periodically and frequently. It will either draw the menu or play the game, depending
 * on which screen is currently being displayed
 *
 * @param elapsedUs The time that has elapsed since the last call to this function, in microseconds
 */
static void pushyMainLoop(int64_t elapsedUs)
{
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        // Save the button state
        pushy->btnState = evt.state;

        // Check if the pause button was pressed
        if (evt.down && (PB_A == evt.button))
        {
            pushy->score++;
        }
    }
}

/**
 * @brief Fade the LEDs at a consistent rate over time
 *
 * @param elapsedUs The time that has elapsed since the last call to this function, in microseconds
 */
static void pushyFadeLeds(int64_t elapsedUs)
{
    // This timer fades out LEDs. The fade is checked every 10ms
    // The pattern of incrementing a variable by elapsedUs, then decrementing it when it accumulates
    pushy->ledFadeTimer += elapsedUs;
    while (pushy->ledFadeTimer >= 10000)
    {
        pushy->ledFadeTimer -= 10000;

        // Fade left LED channels independently
        // TODO: rotate RGB and rename this function to match this new purpose
    }
}

/**
 * This function is called when the display driver wishes to update a
 * section of the display.
 *
 * @param disp The display to draw to
 * @param x the x coordinate that should be updated
 * @param y the x coordinate that should be updated
 * @param w the width of the rectangle to be updated
 * @param h the height of the rectangle to be updated
 * @param up update number
 * @param numUp update number denominator
 */
static void pushyBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
    // Use TURBO drawing mode to draw individual pixels fast
    SETUP_FOR_TURBO();

    // Draw a grid
    for (int16_t yp = y; yp < y + h; yp++)
    {
        for (int16_t xp = x; xp < x + w; xp++)
        {
            if ((0 == xp % 40) || (0 == yp % 40))
            {
                TURBO_SET_PIXEL(xp, yp, c110);
            }
            else
            {
                TURBO_SET_PIXEL(xp, yp, c000);
            }
        }
    }
}
