/**
 * @file touchTest.c
 * @author dylwhich (dylan@whichard.com)
 * @brief A test mode to view touchpad data
 * @date 2023-08-19
 */

//==============================================================================
// Includes
//==============================================================================

#include "swadge2024.h"
#include "hdw-btn.h"
#include "touchTest.h"
#include "esp_log.h"
#include "trigonometry.h"
#include "shapes.h"
#include "fill.h"
#include "linked_list.h"
#include "font.h"
#include "touchUtils.h"

//==============================================================================
// Structs
//==============================================================================

/// @brief The struct that holds all the state for the touchpad test mode
typedef struct
{
    font_t ibm; ///< The font used to display text

    uint16_t btnState; ///< The button state

    bool touch; ///< Whether or not the touchpad is currently touched

    int32_t angle;     ///< The latest touchpad angle
    int32_t radius;    ///< The latest touchpad radius
    int32_t intensity; ///< The latest touchpad intensity

    touchSpinState_t spin; ///< Struct to keep track of the spin state
} touchTest_t;

//==============================================================================
// Function Prototypes
//==============================================================================

static void touchTestMainLoop(int64_t elapsedUs);
static void touchTestEnterMode(void);
static void touchTestExitMode(void);

static void touchTestReset(void);
static void touchTestHandleInput(void);

static void touchTestBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);
static void touchDrawCircle(const char* label, int16_t x, int16_t y, int16_t r, int16_t segs, bool center,
                            touchJoystick_t val);
static void touchDrawVector(int16_t x, int16_t y, int16_t r);
static void touchTestDraw(void);

//==============================================================================
// Strings
//==============================================================================

static const char touchTestName[] = "Touch Test";

//==============================================================================
// Variables
//==============================================================================

/// The Swadge mode for touchTest
swadgeMode_t touchTestMode = {
    .modeName                 = touchTestName,
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = false,
    .usesAccelerometer        = false,
    .usesThermometer          = false,
    .fnEnterMode              = touchTestEnterMode,
    .fnExitMode               = touchTestExitMode,
    .fnMainLoop               = touchTestMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = touchTestBackgroundDrawCallback,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL,
};

/// All state information for the Touchpad Test mode.
touchTest_t* touchTest = NULL;

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Enter Touchpad Test mode, allocate required memory, and initialize required variables
 *
 */
static void touchTestEnterMode(void)
{
    // Allocate and clear all memory for this mode. All the variables are contained in a single struct for convenience.
    // calloc() is used instead of malloc() because calloc() also initializes the allocated memory to zeros.
    touchTest = calloc(1, sizeof(touchTest_t));

    // Load a font
    loadFont("ibm_vga8.font", &touchTest->ibm, false);
}

/**
 * This function is called when the mode is exited. It deinitializes variables and frees all memory.
 */
static void touchTestExitMode(void)
{
    // Free the font
    freeFont(&touchTest->ibm);
    free(touchTest);
}

/**
 * @brief This function is called periodically and frequently. It will both read inputs and draw the screen.
 *
 * @param elapsedUs The time that has elapsed since the last call to this function, in microseconds
 */
static void touchTestMainLoop(int64_t elapsedUs)
{
    // Process button events
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        // Save the button state
        touchTest->btnState = evt.state;

        // Check if the pause button was pressed
        if (evt.down && (PB_START == evt.button))
        {
            touchTestReset();
        }
    }

    // Get all the touchpad things
    touchTestHandleInput();

    // Draw the field
    touchTestDraw();
}

/**
 * @brief Perform the touchpad readings and store them
 */
static void touchTestHandleInput(void)
{
    // Just call all the options for getting touchpad readings for demonstration purposes
    touchTest->touch = getTouchAngleRadius(&touchTest->angle, &touchTest->radius, &touchTest->intensity);

    if (touchTest->touch)
    {
        getTouchSpins(&touchTest->spin, touchTest->angle, touchTest->radius);
    }
    else
    {
        touchTest->spin.startSet = false;
    }
}

/**
 * @brief Reset the touch test mode variables
 */
static void touchTestReset(void)
{
    touchTest->angle     = 0;
    touchTest->radius    = 0;
    touchTest->intensity = 0;

    touchTest->spin.startSet = false;
}

/**
 * This function is called when the display driver wishes to update a
 * section of the display.
 *
 * @param x the x coordinate that should be updated
 * @param y the x coordinate that should be updated
 * @param w the width of the rectangle to be updated
 * @param h the height of the rectangle to be updated
 * @param up update number
 * @param numUp update number denominator
 */
static void touchTestBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
    // Use TURBO drawing mode to draw individual pixels fast
    SETUP_FOR_TURBO();

    // Blank the display
    for (int16_t yp = y; yp < y + h; yp++)
    {
        for (int16_t xp = x; xp < x + w; xp++)
        {
            TURBO_SET_PIXEL(xp, yp, c000);
        }
    }
}

/**
 * @brief
 *
 * @param x
 * @param y
 * @param r
 * @param segs
 */
static void touchDrawCircle(const char* label, int16_t x, int16_t y, int16_t r, int16_t segs, bool center,
                            touchJoystick_t val)
{
    drawText(&touchTest->ibm, c555, label, x - textWidth(&touchTest->ibm, label) / 2,
             y - r - touchTest->ibm.height - 5);

    // Draw outer circle
    drawCircle(x, y, r, c222);

    int16_t centerR = center ? 10 : 0;
    int16_t offset  = 360 - (360 / segs) / 2;

    // Draw the segment lines
    for (uint8_t sector = 0; sector < segs; sector++)
    {
        int16_t angle = (offset + (360 * sector / segs)) % 360;
        drawLineFast(x + getCos1024(angle) * centerR / 1024, y + getSin1024(angle) * centerR / 1024,
                     x + getCos1024(angle) * r / 1024, y + getSin1024(angle) * r / 1024, c222);
    }

    // Draw center circle?
    if (center)
    {
        drawCircle(x, y, centerR - 1, c222);
    }

    if (touchTest->touch)
    {
        int16_t angle = 0;
        int16_t fillR = r / 2;

        switch (val)
        {
            case TB_CENTER:
                angle = 0;
                fillR = 0;
                break;

            case TB_RIGHT:
                angle = 0;
                break;

            case TB_UP | TB_RIGHT:
                angle = 45;
                break;

            case TB_UP:
                angle = 90;
                break;

            case TB_UP | TB_LEFT:
                angle = 135;
                break;

            case TB_LEFT:
                angle = 180;
                break;

            case TB_DOWN | TB_LEFT:
                angle = 225;
                break;

            case TB_DOWN:
                angle = 270;
                break;

            case TB_DOWN | TB_RIGHT:
                angle = 315;
                break;
        }

        // Fill in the segment
        floodFill(x + getCos1024(angle) * fillR / 1024, y - getSin1024(angle) * fillR / 1024, c555, x - r - 1,
                  y - r - 1, x + r + 1, y + r + 1);
    }
}

static void touchDrawVector(int16_t x, int16_t y, int16_t r)
{
    // Draw a circle with tick marks at each 45-degree interval
    int16_t textW = textWidth(&touchTest->ibm, "Raw");
    drawText(&touchTest->ibm, c555, "Raw", x - textW / 2, y - r - touchTest->ibm.height - 5);
    drawCircle(x, y, r, c222);

    int16_t startR     = r * 6 / 7;
    int16_t diagStartR = r * 4 / 7;
    int16_t diagR      = (r + 1) * 2 / 3;
    drawLineFast(x + startR, y, x + r, y, c222);
    drawLineFast(x, y - startR, x, y - r, c222);
    drawLineFast(x - startR, y, x - r, y, c222);
    drawLineFast(x, y + startR, x, y + r, c222);

    drawLineFast(x + diagStartR, y - diagStartR, x + diagR, y - diagR, c222);
    drawLineFast(x - diagStartR, y - diagStartR, x - diagR, y - diagR, c222);
    drawLineFast(x - diagStartR, y + diagStartR, x - diagR, y + diagR, c222);
    drawLineFast(x + diagStartR, y + diagStartR, x + diagR, y + diagR, c222);

    // Draw a line indicating the analog touch vector
    drawLine(x, y, x + getCos1024(touchTest->angle) * touchTest->radius / 1024 * startR / 1024,
             y - getSin1024(touchTest->angle) * touchTest->radius / 1024 * startR / 1024,
             touchTest->touch ? c555 : c333, 0);
    drawCircleFilled(x + getCos1024(touchTest->angle) * touchTest->radius / 1024 * startR / 1024,
                     y - getSin1024(touchTest->angle) * touchTest->radius / 1024 * startR / 1024, 3,
                     touchTest->touch ? c500 : c333);
}

/**
 * @brief Draw the text and graphs, etc.
 */
static void touchTestDraw(void)
{
    // Draw the line for X
    drawLine(20, TFT_HEIGHT - 15, TFT_WIDTH - 20, TFT_HEIGHT - 15, c555, 0);

    // Draw the line for Y
    drawLine(15, 20, 15, TFT_HEIGHT - 20, c555, 0);

    // And the marker, if there's a touch
    if (touchTest->touch)
    {
        int32_t cartX, cartY;
        getTouchCartesian(touchTest->angle, touchTest->radius, &cartX, &cartY);

        // And the marker for X
        drawCircleFilled(20 + (TFT_WIDTH - 40) * cartX / 1024, TFT_HEIGHT - 15, 5, c050);

        // And Y
        drawCircleFilled(15, TFT_HEIGHT - 20 - (TFT_HEIGHT - 40) * cartY / 1024, 5, c050);
    }

    touchDrawVector(60, TFT_HEIGHT / 4, 35);

    // Write the values
    char buffer[64];

    int16_t textW = textWidth(&touchTest->ibm, "Angle");
    int16_t textX = 60 - textW / 2;
    int16_t textY = TFT_HEIGHT / 4 + 35 + 15;

    // Draw "Angle" over the angle
    drawText(&touchTest->ibm, c555, "Angle", textX, textY);
    // Underline it
    drawLineFast(textX, textY + touchTest->ibm.height, textX + textW, textY + touchTest->ibm.height, c555);

    // Draw angle value on the next line, centered but right-aligned within its max size
    snprintf(buffer, sizeof(buffer) - 1, "%" PRId32, touchTest->angle);
    textW = textWidth(&touchTest->ibm, buffer);
    textX = 60 + textWidth(&touchTest->ibm, "222") / 2 - textW;
    textY += touchTest->ibm.height + 5;
    drawText(&touchTest->ibm, c555, buffer, textX, textY);
    // Draw a lil degree sign
    drawCircle(textX + textW + 2, textY, 1, c555);

    // Skip a line, draw the radius header
    textY += (touchTest->ibm.height + 2) * 2;
    textW = textWidth(&touchTest->ibm, "Radius");
    textX = 60 - textW / 2;
    drawText(&touchTest->ibm, c555, "Radius", 60 - textW / 2, textY);
    drawLineFast(textX, textY + touchTest->ibm.height, textX + textW, textY + touchTest->ibm.height, c555);

    // Draw the radius value
    snprintf(buffer, sizeof(buffer) - 1, "%" PRId32, touchTest->radius);
    textW = textWidth(&touchTest->ibm, buffer);
    textX = 60 + textWidth(&touchTest->ibm, "1222") / 2 - textW;
    textY += touchTest->ibm.height + 5;
    drawText(&touchTest->ibm, c555, buffer, textX, textY);

    // Skip a line, draw the intensity header
    textY += (touchTest->ibm.height + 2) * 2;
    textW = textWidth(&touchTest->ibm, "Intensity");
    textX = 60 - textW / 2;
    drawText(&touchTest->ibm, c555, "Intensity", 60 - textW / 2, textY);
    drawLineFast(textX, textY + touchTest->ibm.height, textX + textW, textY + touchTest->ibm.height, c555);

    // Draw the intensity value
    snprintf(buffer, sizeof(buffer) - 1, "%" PRId32, touchTest->touch ? touchTest->intensity : 0);
    textW = textWidth(&touchTest->ibm, buffer);
    textX = 60 + textWidth(&touchTest->ibm, "1222") / 2 - textW;
    textY += touchTest->ibm.height + 5;
    drawText(&touchTest->ibm, c555, buffer, textX, textY);

    if (touchTest->touch || touchTest->spin.startSet)
    {
        // Draw the spin text in the middle, right of "Angle"
        snprintf(buffer, sizeof(buffer) - 1, "Spins: %+" PRId32 "%c%" PRId32, touchTest->spin.spins,
                 (touchTest->spin.spins < 0 || touchTest->spin.remainder < 0) ? '-' : '+',
                 ABS(touchTest->spin.remainder));
        textW = textWidth(&touchTest->ibm, buffer);
        textX = TFT_WIDTH / 2 - 35;
        textY = TFT_HEIGHT / 4 + 35 + 15;
        drawText(&touchTest->ibm, c555, buffer, textX, textY);
        // Draw a degree sign
        drawCircle(textX + textW + 2, textY, 1, c555);
    }

    // Draw the 4-direction touchpad circle
    touchDrawCircle("4", TFT_WIDTH / 2, TFT_HEIGHT / 4, 35, 4, false,
                    getTouchJoystick(touchTest->angle, touchTest->radius, false, false));

    // Draw the 8-direction touchpad circle
    touchDrawCircle("8", TFT_WIDTH - 60, TFT_HEIGHT / 4, 35, 8, false,
                    getTouchJoystick(touchTest->angle, touchTest->radius, false, true));

    // Draw the 4-direction touchpad with center circle
    touchDrawCircle("4+Center", TFT_WIDTH / 2, TFT_HEIGHT - TFT_HEIGHT / 4, 35, 4, true,
                    getTouchJoystick(touchTest->angle, touchTest->radius, true, false));

    // Draw the 8-direction touchpad with center circle
    touchDrawCircle("8+Center", TFT_WIDTH - 60, TFT_HEIGHT - TFT_HEIGHT / 4, 35, 8, true,
                    getTouchJoystick(touchTest->angle, touchTest->radius, true, true));
}
