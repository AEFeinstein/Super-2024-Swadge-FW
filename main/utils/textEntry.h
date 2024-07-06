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
