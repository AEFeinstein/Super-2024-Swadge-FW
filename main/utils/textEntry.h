/*! \file textEntry.h
 *
 * \section textEntry_design Design Philosophy
 *
 * TextEntry serves as a method to create strings for a multitude of cases. It should be easy to see, easy to customize
 * and as intuitive to use as possible. Several customization options are available.
 *
 * \section textEntry_usage Usage
 *
 * Text entry in initialize with textEntryInit(), where the settings, string pointer and the desired font are
 * provided. By default, a blank, black background is used with white text and red as the emphasis color.
 *
 * textEntryInit() can be called once during initialization or just before the text entry is required, but cannot be
 * inside the loop or it will not function.
 *
 * The text entry is re-drawn from scratch every cycle.
 *
 * Settings:
 * - textPrompt: The text prompt at the top of the screen
 * - maxLen: The maximum length that the string may build to
 * - startKMod: There are several capitalization modes. This sets the initial mode. The options are TE_NO_SHIFT,
TE_SHIFT, TE_CAPS_LOCK, and TE_PROPER_NOUN
 * - useMultiLine: Uses a larger text box for longer strings
 * - useNewCapsStyle: Use the second capslock style
 * - useOKEnterStyle: Use 'OK' instead of the return symbol
 * - blink: If the cursor should blink
 * - textColor: Color of most of the text in the UI
 * - emphasisColor: Color of certain UI elements to help users
 * - bgColor: Color that the background is drawn, set to cTransparent to not draw anything
 * - shadowboxColor: Color of shadowboxes drawn to allow text to stand out. Set to cTransparent to not draw
 *
 * After all of the initialization is done, use textEntryDraw() to draw the current text entry box and textEntryInput()
 * to send inputs to the tent entry. textEntryInput() returns false until enter/okay is selected.
 *
 * The controls are as follows:
 * Directions: navigate the keyboard
 * A: Select currently highlighted symbol
 * B: Backspace
 * Start: Move cursor to the enter key
 *
 * \section textEntry_example Example
 *
 * \code{.c}
 // Settings
 static const textEntrySettings_t teSettings = {
    .textPrompt      = textSamplePrompt,
    .maxLen          = MAX_TEXT_LEN,
    .startKMod       = TE_PROPER_NOUN,
    .useMultiLine    = true,
    .useNewCapsStyle = true,
    .useOKEnterStyle = true,
    .blink           = true,
    .textColor       = c454,
    .emphasisColor   = c511,
    .bgColor         = c000,
    .shadowboxColor  = c222,
};

// Init
textEntryInit(&teSettings, promptText, font);

main loop(int64_t elapsedUs)
{
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        done = textEntryInput(evt);
    }
    if (done)
    {
        // Text entry is done, createdString contains the text and can be used elsewhere
    }
    // Draw backgrounds if no solid color is picked
    textEntryDraw(elapsedUs);
}

// Deinit
textEntryDeinit();
 * \endcode
 */

#pragma once

//==============================================================================
// Includes
//==============================================================================

#include "swadge2024.h"

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    TE_NO_SHIFT,    ///< Start on lowercase, no auto-capitalization
    TE_SHIFT,       ///< Capitalize the first character only
    TE_CAPS_LOCK,   ///< Default to all caps
    TE_PROPER_NOUN, ///< automatically the first letter after a space
    TE_SPECIAL_DONE ///< DO NOT USE! Indicates to system that it is done
} keyModifier_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    // Text options
    const char* textPrompt;  ///< Prompt for the player. Set to NULL to not draw.
    int maxLen;              ///< The max length of the string
    keyModifier_t startKMod; ///< Sets default capitalization mode
    bool useMultiLine;       ///< Use multiple lines
    bool useOKEnterStyle;    ///< Use the "OK" instead of the arrow
    bool useNewCapsStyle;    ///< Use new style instead of old style
    bool blink;              ///< If the cursor should blink

    // Colors
    paletteColor_t textColor;      ///< Color of the text to use.
    paletteColor_t emphasisColor;  ///< Color of the emphasis, used to highlight certain parts of the UI.
    paletteColor_t bgColor;        ///< Color of the background. Set to `cTransparent` to not draw a background
    paletteColor_t shadowboxColor; ///< Color of the shadowboxes. Set to `cTransparent` to not draw shadowboxes
} textEntrySettings_t;

//==============================================================================
// Function declarations
//==============================================================================

void textEntryInit(const textEntrySettings_t* settings, char* entryText, font_t* font);

/**
 * @brief Deletes the memory used by the text entry system
 *
 */
void textEntryDeinit(void);

/**
 * @brief Handle button input for text entry
 *
 * @param evt button event to run
 * @return false if text entry is still ongoing
 * @return true if the enter key was pressed and text entry is done
 */
bool textEntryInput(buttonEvt_t evt);

/**
 * @brief Draws the text entry screen
 *
 * @param elapsedUs TIme for cursor blink
 * @return true If text entry is still being executed
 * @return false If text entry is done
 */
bool textEntryDraw(int64_t elapsedUs);

/**
 * @brief Resets the textEntry object
 *
 */
void textEntrySoftReset(void);