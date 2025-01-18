/**
 * @file colorTest.c
 * @author dylwhich (dylan@whichard.com)
 * @brief A test mode for extra colors
 * @date 2025-01-17
 */

//==============================================================================
// Includes
//==============================================================================

#include "swadge2024.h"
#include "hdw-btn.h"
#include "colorTest.h"
#include "esp_log.h"
#include "trigonometry.h"
#include "shapes.h"
#include "fill.h"
#include "linked_list.h"
#include "font.h"
#include "touchUtils.h"
#include "palette.h"
#include "esp_random.h"

//==============================================================================
// Structs
//==============================================================================

/// @brief The struct that holds all the state for the touchpad test mode
typedef struct
{
    font_t ibm; ///< The font used to display text

    uint16_t btnState; ///< The button state

    int colIndex;
    uint16_t lastColor;
    uint8_t h;
    uint8_t s;
    uint8_t v;

    uint16_t palette[39];
    uint16_t lastPalette[39];
} colorTest_t;

//==============================================================================
// Function Prototypes
//==============================================================================

static void colorTestMainLoop(int64_t elapsedUs);
static void colorTestEnterMode(void);
static void colorTestExitMode(void);

static void colorTestReset(void);

static void colorTestBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);
static void colorTestDraw(void);
static void colorTestUpdateColors(void);

//==============================================================================
// Strings
//==============================================================================

static const char colorTestName[] = "Color Test";

//==============================================================================
// Variables
//==============================================================================

/// The Swadge mode for colorTest
swadgeMode_t colorTestMode = {
    .modeName                 = colorTestName,
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = false,
    .usesAccelerometer        = false,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = colorTestEnterMode,
    .fnExitMode               = colorTestExitMode,
    .fnMainLoop               = colorTestMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = colorTestBackgroundDrawCallback,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL,
};

/// All state information for the Touchpad Test mode.
colorTest_t* colorTest = NULL;

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Enter Touchpad Test mode, allocate required memory, and initialize required variables
 *
 */
static void colorTestEnterMode(void)
{
    // Allocate and clear all memory for this mode. All the variables are contained in a single struct for convenience.
    // heap_caps_calloc() is used instead of heap_caps_malloc() because heap_caps_calloc() also initializes the
    // allocated memory to zeros.
    colorTest = heap_caps_calloc(1, sizeof(colorTest_t), MALLOC_CAP_8BIT);

    // Load a font
    loadFont("ibm_vga8.font", &colorTest->ibm, false);

    setFrameRateUs(0);
}

/**
 * This function is called when the mode is exited. It deinitializes variables and frees all memory.
 */
static void colorTestExitMode(void)
{
    // Free the font
    freeFont(&colorTest->ibm);
    heap_caps_free(colorTest);
}

/**
 * @brief This function is called periodically and frequently. It will both read inputs and draw the screen.
 *
 * @param elapsedUs The time that has elapsed since the last call to this function, in microseconds
 */
static void colorTestMainLoop(int64_t elapsedUs)
{
    // Process button events
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        // Save the button state
        colorTest->btnState = evt.state;

        // Check if the pause button was pressed
        if (evt.down && (PB_START == evt.button))
        {
            colorTestReset();
        }
    }

    static int32_t timer = 0;
    timer -= elapsedUs;
    while (timer <= 0)
    {
        timer += 1250;
        colorTest->colIndex = (colorTest->colIndex + 1) % 39;
        //colorTest->palette[colorTest->colIndex] = esp_random() & 0xFFFF;
        //colorTest->palette[colorTest->colIndex] = colorTest->lastColor;
        //colorTest->lastColor += 2;

        uint32_t color24 = EHSVtoHEXhelper(colorTest->h, colorTest->s, colorTest->v, false);
        uint8_t r = ((color24 & 0xFF)) >> 3;
        uint8_t g = ((color24 >> 8) & 0xFF) >> 3;
        uint8_t b = ((color24 >> 16) & 0xFF) >> 3;
        uint16_t color16 = RGB_TO_16BIT_PALETTE(r, g, b);

        colorTest->palette[colorTest->colIndex] = color16;

        colorTest->s = 255;

        colorTest->v++;
        if (colorTest->v == 0)
        {
            colorTest->h++;
            /*if (colorTest->h == 0)
            {
                colorTest->s += 8;
            }*/
        }
    }

    colorTestUpdateColors();

    // Draw the field
    colorTestDraw();
}

/**
 * @brief Reset the touch test mode variables
 */
static void colorTestReset(void)
{
}

/**
 * This function is called when the display driver wishes to update a
 * section of the display.
 *
 * @param x the x coordinate that should be updated
 * @param y the y coordinate that should be updated
 * @param w the width of the rectangle to be updated
 * @param h the height of the rectangle to be updated
 * @param up update number
 * @param numUp update number denominator
 */
static void colorTestBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
    // Use TURBO drawing mode to draw individual pixels fast
    SETUP_FOR_TURBO();

    // Blank the display
    paletteColor_t col;
    for (int16_t yp = y; yp < y + h; yp++)
    {
        for (int16_t xp = x; xp < x + w; xp++)
        {
            // We want to print colors in blocks of 16x16 pixels...
            col = (yp / 15) * 16 + (xp / 15);
            TURBO_SET_PIXEL(xp, yp, col);
        }
    }
}

/**
 * @brief Draw the text and graphs, etc.
 */
static void colorTestDraw(void)
{
}

/**
 * @brief 
 * 
 */
static void colorTestUpdateColors(void)
{
    if (memcmp(colorTest->palette, colorTest->lastPalette, sizeof(colorTest->palette)))
    {
        extendPalette(colorTest->palette);
        memcpy(colorTest->lastPalette, colorTest->palette, sizeof(colorTest->palette));
    }
}