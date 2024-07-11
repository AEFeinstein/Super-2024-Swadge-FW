/*! \file touchTextEntry.h
 *
 * \section touchTextEntry_design Design Philosophy
 *
 * The text entry utility provides a minimal interface for editing a single line of text of unlimited length.
 * The entered text is kept in an internal buffer that automatically grows if needed. The string is returned via
 * a callback, and should be copied to an external buffer with strncpy() or strdup() for later access.
 *
 * Characters can be selected using either touchpad spins or up and down on the D-pad. Once selected, a character
 * can be confirmed and entered with the A button. The B button erases the previous charater, and the left and
 * right arrows move the cursor within the text. The right arrow will also create one space if used at the end
 * of the text. The pause button confirms the text entry and calls the callback.
 *
 * \section touchTextEntry_usage Usage
 *
 * The text entry was made as generic as possible to enable using it in various modes. The only thing it draws
 * to the screen is the text and, optionally, a box around the text area's bounds. When the text entry is being
 * used, the mode should pass all button input to the text entry until the callback is called. The text entry
 * provides support for allowing certain sets of characters individually, or for combined multiple classes by
 *  performing a logical OR of values in textEntryCharMask_t. The classes that can be used are:
 *
 * - Uppercase Letters
 * - Lowercase Letters
 * - Numbers
 * - Whitespace
 * - Symbols, which includes everything not covered by the other categories
 *
 *
 * \section touchTextEntry_example Example
 *
 * \code{.c}
 *
 * typedef struct {
 *     textEntry_t* textEntry;
 *     font_t font;
 *     char* textData;
 * } modeData_t;
 *
 * static modeData_t* modeData;
 *
 * void textEntryCb(const char* text, void* data)
 * {
 *     ESP_LOGI("Text", "Text entered was %s!", text);
 *     modeData_t* mode = (modeData_t*) data;
 *
 *     if (mode->textData != NULL)
 *     {
 *         free(mode->textData);
 *     }
 *     mode->textData = strdup(text);
 * }
 *
 * void setup(void)
 * {
 *     modeData = calloc(1, sizeof(modeData_t));
 *     // Setup function
 *     uint16_t entryW = 160;
 *     uint16_t entryH = 16;
 *     uint16_t entryX = (TFT_WIDTH - entryW) / 2;
 *     uint16_t entryY = (TFT_HEIGHT - entryH) / 2;
 *
 *     loadFont("ibm_vga8.font", &modeData->font);
 *
 *     modeData->textEntry = initTextEntry(entryX, entryY, entryW, entryH, &font, ENTRY_ALPHANUM, textEntryCb);
 *
 *     // If needed, a data pointer can be set that will be passed back to the callback
 *     textEntrySetData(textEntry, &modeData);
 * }
 *
 * void cleanUp(void)
 * {
 *     freeTextEntry(textEntry);
 *     freeFont(&font);
 *
 *     if (modeData->textData != NULL)
 *     {
 *         free(modeData->textData);
 *     }
 * }
 *
 * // Main loop
 * void mainLoop(int64_t elapsedUs)
 * {
 *     // Handle buttons for text entry
 *     buttonEvt_t evt;
 *     while (checkButtonQueueWrapper(&evt))
 *     {
 *         textEntryButton(textEntry, &evt);
 *     }
 *
 *     // Handle touchpad, button repeating, and other logic
 *     textEntryMainLoop(textEntry, elapsedUs);
 *
 *     // Draw the text entry with a white background, black text, and a box around the text itself
 *     drawTextEntry(textEntry, c000, c555, true);
 * }
 *
 * \endcode
 *
 */
#ifndef _TOUCH_TEXT_ENTRY_H_
#define _TOUCH_TEXT_ENTRY_H_

#include <stdint.h>
#include "font.h"
#include "hdw-btn.h"
#include "touchUtils.h"

/**
 * @brief Bitmask enum for specifying which classes of characters may be entered.
 *
 * Any of these values may be combined with a bitwise OR to allow multiple classes.
 *
 */
typedef enum
{
    /// @brief Mask for allowing uppercase alphabet characters
    ENTRY_UPPERCASE = 0x01,
    /// @brief Mask for allowing lowercase alphabet characters
    ENTRY_LOWERCASE = 0x02,
    /// @brief Mask for allowing numeric characters
    ENTRY_NUMBERS = 0x04,
    /// @brief Mask for allowing symbol characters (printable, non-alphanumeric, and non-whitespace)
    ENTRY_SYMBOLS = 0x08,
    /// @brief Mask for allowing whitespace characters (space)
    ENTRY_WHITESPACE = 0x10,
} textEntryCharMask_t;

/// @brief Mask for allowing uppercase and lowercase alphabet
#define ENTRY_ALPHA (ENTRY_UPPERCASE | ENTRY_LOWERCASE)
/// @brief Mask for allowing uppercase and lowercase alphabet, and numbers
#define ENTRY_ALPHANUM (ENTRY_ALPHA | ENTRY_NUMBERS)
/// @brief Mask for allowing any characters except whitespace
#define ENTRY_WORD (ENTRY_UPPERCASE | ENTRY_LOWERCASE | ENTRY_NUMBERS | ENTRY_SYMBOLS)
/// @brief Mask for allowing all printable characters
#define ENTRY_ALL (ENTRY_UPPERCASE | ENTRY_LOWERCASE | ENTRY_NUMBERS | ENTRY_SYMBOLS | ENTRY_WHITESPACE)

/**
 * @brief A callback which is called once text entry is complete.
 * @param text The final text value. This value should be copied if it will be saved.
 * @param data The void pointer passed to textEntrySetData(), or NULL if no data was set
 *
 */
typedef void (*textEntryCb)(const char* text, void* data);

/**
 * @brief Struct to store all state for a text entry
 *
 */
typedef struct
{
    //// Main Data

    /// @brief The buffer holding the actual entered text value. Can be accessed as a string
    char* value;

    /// @brief The total size of the dynamic text buffer.
    uint16_t size;

    /// @brief The position of the cursor
    uint16_t cursor;

    /// @brief Whether the cursor will operate in overtype mode instead of insert mode
    bool overtype;

    /// @brief Whether a to-be-entered character should be shown
    bool pendingChar;

    /// @brief The current character pending addition at the cursor, when pendingChar is true
    char cur;

    /// @brief Boolean value to keep track of whether the blinking cursor is shown
    bool blinkState;

    /// @brief The number of nanoseconds remaining before the blink state changes
    int64_t blinkTimer;

    /// @brief The minimum length of text that will be accepted
    uint16_t minLength;

    /// @brief The maximum length of text allowed, or 0 for no limit
    uint16_t maxLength;

    /// @brief The mask of character types that are allowed
    textEntryCharMask_t mask;

    /// @brief Struct to track the state of the touchpad for spins
    touchSpinState_t spinState;

    /// @brief The selected character at the start of the touchpad spin
    char spinCharStart;

    /// @brief The button that is currently being held, or 0 if none
    buttonBit_t heldButton;

    /// @brief The number of nanoseconds remaining before the held button repeats
    int64_t repeatTimer;

    /// @brief The first character to be drawn on-screen, if the whole value doesn't fit
    uint16_t offset;

    /// @brief The X position of the text entry box
    uint16_t x;

    /// @brief The Y position of the text entry box
    uint16_t y;

    /// @brief The width of the text entry box
    uint16_t w;

    /// @brief The font to use for text entry
    const font_t* font;

    /// @brief A pointer to be passed back into the callback
    void* data;

    /// @brief The function to call when the text entry is completed
    textEntryCb cbFn;
} textEntry_t;

textEntry_t* initTextEntry(uint16_t x, uint16_t y, uint16_t w, uint16_t length, const font_t* font,
                           textEntryCharMask_t mask, textEntryCb cbFn);
void freeTextEntry(textEntry_t* textEntry);

void textEntrySetData(textEntry_t* textEntry, void* data);
void textEntrySetText(textEntry_t* textEntry, const char* text);
void textEntryMainLoop(textEntry_t* textEntry, int64_t elapsedUs);
void textEntryButton(textEntry_t* textEntry, const buttonEvt_t* evt);
void drawTextEntry(textEntry_t* textEntry, paletteColor_t fg, paletteColor_t bg, bool drawBox);

#endif
