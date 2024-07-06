#pragma once

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
#define ENTER_STYLE false

#define KB_LINES 5
#define ENTER_X  12
#define ENTER_Y  2

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
 * @param tbColor  Color used as a contrast to the text color
 * @param txtColor Color used for the text
 */
void textEntryStartPretty(font_t* usefont, int max_len, char* buffer, wsg_t BG, uint8_t tbColor, uint8_t txtColor);

/**
 * Draw the text entry UI
 *
 * @return true if text entry is still being used
 *         false if text entry is finished
 */
bool textEntryDraw(int64_t elapsedUs);

/**
 * handle button input for text entry
 *
 * @param down   true if the button was pressed, false if it was released
 * @param button The button that was pressed
 * @return true if text entry is still ongoing
 *         false if the enter key was pressed and text entry is done
 */
bool textEntryInput(uint8_t down, uint8_t button);

// Internal draw functions

/**
 * @brief Updated text entry with more options
 * 
 * @param text_h    How far down the screen the text starts
 * @param elaspedUs How many ms have elasped since last time function was called
 */
static void _drawStrPretty(int8_t text_h, int64_t elaspedUs);

/**
 * @brief Old, non-pretty keyboard routine. Provided for compatibility
 * 
 * @param text_h How far down the screen the text starts
 */
static void _drawStrSimple(uint8_t text_h);

/**
 * @brief Draws the keyboard
 * 
 */
static void _drawKeyboard(void);

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
 */
static void _drawEnter(int16_t x, int16_t y, uint8_t color);