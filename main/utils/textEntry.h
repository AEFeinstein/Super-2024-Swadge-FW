/*! \file textEntry.h
 *
 * \section textEntry_design Design Philosophy
 *
 * TextEntry serves as a method to create strings for a multitude of cases. It should be easy to see, easy to customize
 * and as intuitive to use as possible. Several customization options are available.
 *
 * \section textEntry_usage Usage
 *
 * Text entry in initialize with textEntryInit(), where the font used, max string length, and string pointer are
 * provided. By default, a blank, black background is used with white text and no emphasis color.
 *
 * Once initalized, additional commands can be run to customize the text entry screen:
 * - textEntrySetFont(font_t* newFont): A new font to use for the keyboard
 * - textEntrySetBgWsg(wsg_t *wsg): Sets a provided image to the background and sets teh background mode to use the WSG.
 * - textEntrySetBGColor(uint8_t color): Sets the background to a solid color
 * - textEntrySetBGTransparent(): Sets the background to be transparent
 * - textEntrySetTextColor(uint8_t color, bool emphasis): Sets the text color, and optionally sets the emphasis color.
 * - textEntrySetEmphasisColor(uint8_t color): Sets the emphasis color.
 * - textEntrySetShadowboxColor(bool active, uint8_t color): Sets the color of the shadowboxes used to contrast the
 *                                                           background
 * - textEntrySetNewEnterStyle(bool newStyle): Sets the style to old (false) or new (true)
 * - textEntrySetNewCapsStyle(bool newStyle): Sets the style to old (false) or new (true)
 *
 * The text entry is re-drawn from scratch every cycle. The above commands can be run between cycles if desired, though
 * it is strongly discouraged to keep the text entry easy for the end user to navigate and utilize.
 *
 * textEntryInit() can be called once during initialization or just before the text entry is required, but cannot be
 * inside the loop or it will not function.
 *
 * After all of the initialization is done, use textEntryDraw() to draw the current text entry box and textEntryInput()
 * to send inputs to the tent entry. textEntryInput() returns true until enter/okay is selected.
 *
 * The controls are as follows:
 * Directions: navigate the keyboard
 * A button: Select currently highlighted symbol
 * B button: Backspace
 * Start button: Move cursor to the enter key
 *
 * \section textEntry_example Example
 *
 * \code{.c}
 * // Variables required
 * font fnt;
 * font fnt2;
 * int strLen = 32;
 * char createdString[strlen];
 * wsg bg_test;
 *
 * // Text Entry initialization
 * textEntryInit(&fnt, strLen, kbTest->typedText);
 *
 * // Examples: Each of the following would overwrite the others
 * // Blue background, gray text, red emphasis text, and dark gray shadowboxes
 * textEntrySetBGColor(c003);
 * textEntrySetTextColor(c444);
 * textEntrySetEmphasisColor(c500);
 * textEntrySetShadowboxColor(c111);
 *
 * // WSG background, pink shadowboxes, and a new font
 * textEntrySetBgWsg(&bg_test);
 * textEntrySetShadowboxColor(true, c433);
 * textEntrySetFont(&fnt2);
 *
 * // Transparent BG with caps lock and enter variations
 * textEntrySetBGTransparent();
 * textEntrySetNewCapsStyle(true);
 * textEntrySetNewEnterStyle(true);
 *
 * main loop(int64_t elapsedUs)
 * {
 *     buttonEvt_t evt = {0};
 *     while (checkButtonQueueWrapper(&evt))
 *     {
 *         done = !textEntryInput(evt.down, evt.button);
 *     }
 *     if (done)
 *     {
 *         // Text entry is done, createdString contains the text and can be used elsewhere
 *     }
 *     textEntryDraw(elapsedUs);
 * }
 * \endcode
 */

#pragma once

//==============================================================================
// Includes
//==============================================================================

#include "swadge2024.h"

//==============================================================================
// Defines
//==============================================================================

// Selection position
#define KB_LINES 5
#define ENTER_X  12
#define ENTER_Y  2

// Graphics
#define CORNER_MARGIN    20 // Margin to clear the rounded corners
#define SHADOWBOX_MARGIN 4  // Margin around shadowboxes
#define RETURN_WIDTH     16 // Width of return symbol
#define KEY_SPACING      5  // Space between keys
#define MAX_WIDTH_CHAR   32 // @ symbol, due to size in fonts tested.

// Timers
#define BLINK_RATE 500000 // Time in Us before blinker toggles

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
    KEY_CAPS_LOCK = 0x02,
    KEY_BACKSPACE = 0x03,
    KEY_SPACE     = 0x20,
    KEY_EOL       = 0x05,
    KEY_TAB       = 0x09,
    KEY_ENTER     = 0x0A,
} controlChar_t;

typedef enum
{
    WSG_BG,
    COLOR_BG,
    CLEAR_BG,
} bgMode_t;

//==============================================================================
// Function declarations
//==============================================================================

/**
 * @brief Initialize the text entry with prettier graphics
 *
 * @param useFont  The font to use
 * @param max_len  The length of buffer
 * @param buffer   A char* to store the entered text in
 */
void textEntryInit(font_t* useFont, int max_len, char* buffer);

/**
 * @brief Draw the text entry UI
 *
 * @return true if text entry is still being used
 *         false if text entry is finished
 */
bool textEntryDraw(int64_t elapsedUs);

/**
 * @brief Handle button input for text entry
 *
 * @param down   true if the button was pressed, false if it was released
 * @param button The button that was pressed
 * @return true if text entry is still ongoing
 *         false if the enter key was pressed and text entry is done
 */
bool textEntryInput(uint8_t down, uint8_t button);

// Setters

/**
 * @brief A new font to load into the text entry screen
 *
 * @param newFont pointer to a font file
 */
void textEntrySetFont(font_t* newFont);

/**
 * @brief Sets a background image and sets the BG Mode to use the WSG
 *
 * @param BG Pointer to Background image to set
 */
void textEntrySetBgWsg(wsg_t* BG);

/**
 * @brief Sets the background to use a solid color
 *
 * @param color Color to set the background
 */
void textEntrySetBGColor(uint8_t color);

/**
 * @brief Sets the background to be transparent
 *
 */
void textEntrySetBGTransparent(void);

/**
 * @brief Set the main text color for the keyboard and entry text
 *
 * @param color Color to set the text to.
 * @param emphasis Whether emphasis color should also be set to the same color
 */
void textEntrySetTextColor(uint8_t color, bool emphasis);

/**
 * @brief Set the emphasis color for the text
 *
 * @param color Color for the emphasis text
 */
void textEntrySetEmphasisColor(uint8_t color);

/**
 * @brief Sets the color of the shadowbox
 *
 * @param active Whether these boxes should be drawn
 * @param color Color for the shadowboxes
 */
void textEntrySetShadowboxColor(bool active, uint8_t color);

/**
 * @brief Sets the style of the Enter key
 *
 * @param newStyle if true, use the new style
 */
void textEntrySetNewEnterStyle(bool newStyle);

/**
 * @brief Sets the style of the caps lock key
 *
 * @param newStyle if true, use the new style
 */
void textEntrySetNewCapsStyle(bool newStyle);

/**
 * @brief Sets the text entry mode from single line to multi-line
 *
 * @param multiline True if using multi-line, false if not
 * @note cursor does not draw in multiline due to drawTextWordWrap not exposing the end position of the text.
 */
void textEntrySetMultiline(bool multiline);

/**
 * @brief Allow the text box to continue without a full reset
 *
 */
void textEntrySoftReset(void);