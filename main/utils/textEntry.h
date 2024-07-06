#pragma once

// TODO:
// Un-hard code values

//==============================================================================
// Includes
//==============================================================================

#include "swadge2024.h"

//==============================================================================
// Defines
//==============================================================================

// Default color for basic text entry
#define WHITE 215

// false = Thicker Shift, true = Thick arrow with box below
#define CAPS_NEW_STYLE true

// false = enter is texk "OK", true = "Return" style arrow
#define ENTER_STYLE true

// Selection position
#define KB_LINES 5
#define ENTER_X  12
#define ENTER_Y  2

// Graphics
#define MARGIN      32     // Margin from the edge of the screen
#define STR_H_START 64     // Distance from the top of the screen to start drawing
#define BLINK_RATE  250000 // Time in Us before blinker toggles

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    NO_SHIFT,
    SHIFT,
    CAPS_LOCK,
    SPECIAL_DONE
} keyModifier_t;

typedef enum
{
    KEY_SHIFT     = 0x01,
    KEY_CAPSLOCK  = 0x02,
    KEY_BACKSPACE = 0x03,
    KEY_SPACE     = 0x20,
    KEY_EOL       = 0x05,
    KEY_TAB       = 0x09,
    KEY_ENTER     = 0x0A,
} controlChar_t;

//==============================================================================
// Functions
//==============================================================================

/**
 * Initialize the text entry
 *
 * @param usefont The font to use, should be ibm_vga8
 * @param max_len The length of buffer
 * @param buffer  A char* to store the entered text in
 */
void textEntryStart(font_t* usefont, int max_len, char* buffer);

/**
 * Initialize the text entry with prettier graphics
 *
 * @param usefont  The font to use
 * @param max_len  The length of buffer
 * @param buffer   A char* to store the entered text in
 * @param BG       Background image to use
 */
void textEntryStartPretty(font_t* usefont, int max_len, char* buffer, wsg_t* BG);

/**
 * @brief Provided for backwards comaptibility. Will not blink cursor.
 *
 * @return true
 * @return false
 */
bool textEntryDraw(void);

/**
 * Draw the text entry UI
 *
 * @return true if text entry is still being used
 *         false if text entry is finished
 */
bool textEntryDrawBlink(int64_t elapsedUs);

/**
 * handle button input for text entry
 *
 * @param down   true if the button was pressed, false if it was released
 * @param button The button that was pressed
 * @return true if text entry is still ongoing
 *         false if the enter key was pressed and text entry is done
 */
bool textEntryInput(uint8_t down, uint8_t button);

// Setters
void textEntrySetTextColor(uint8_t col);
void textEntrySetEmphasisColor(uint8_t col);
void textEntrySetShadowboxColor(uint8_t col);
void textEntrySetBG(wsg_t* BG);

// Internal draw functions

/**
 * @brief Updated text entry with more options
 *
 * @param elaspedUs How many ms have elasped since last time function was called
 */
static void _drawStrPretty(int64_t elaspedUs);

/**
 * @brief Old, non-pretty keyboard routine. Provided for compatibility
 *
 * @param elaspedUs How many ms have elasped since last time function was called
 */
static void _drawStrSimple(int64_t elaspedUs);

/**
 * @brief Draws the curso at the end of the line
 *
 * @param eUs used to calculate if cursor should toggle on or off.
 * @param end The end of the line, used to calculate position.
 */
static void _drawCursor(int64_t eUs, int16_t pos);

/**
 * @brief Draws the keyboard
 *
 * @param pretty If the shadwobox should be drawn
 */
static void _drawKeyboard(bool pretty);

/**
 * @brief Draws the custom capslock character
 *
 * @param x     Starting x position
 * @param y     Starting y coordinate
 * @param color Color of the text
 */
static void _drawCaps(int16_t x, int16_t y, uint8_t color);

/**
 * @brief Draws the custom shift key character
 *
 * @param x     Starting x position
 * @param y     Starting y coordinate
 * @param color Color of the text
 */
static void _drawShift(int16_t x, int16_t y, uint8_t color);

/**
 * @brief Draws the custom backspace character
 *
 * @param x     Starting x position
 * @param y     Starting y coordinate
 * @param color Color of the text
 */
static void _drawBackspace(int16_t x, int16_t y, uint8_t color);

/**
 * @brief Draws the custom spacebar character
 *
 * @param x     Starting x position
 * @param y     Starting y coordinate
 * @param color Color of the text
 */
static void _drawSpacebar(int16_t x, int16_t y, uint8_t color);

/**
 * @brief Draws the custom tab character
 *
 * @param x     Starting x position
 * @param y     Starting y coordinate
 * @param color Color of the text
 */
static void _drawTab(int16_t x, int16_t y, uint8_t color);

/**
 * @brief Draws the custom enter kry character
 *
 * @param x     Starting x position
 * @param y     Starting y coordinate
 * @param color Color of the text
 *
 * @return int  Width of the symbol for selectiion box drawing
 */
static int _drawEnter(int16_t x, int16_t y, uint8_t color);

/**
 * @brief Draws some text indicationg the typing mode
 *
 * @param color Color to use for the text
 * @param pretty Whether to use the shadowbox
 */
static void _drawTypeMode(uint8_t color, bool pretty);