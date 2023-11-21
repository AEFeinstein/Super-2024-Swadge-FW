//==============================================================================
// Includes
//==============================================================================

#include "touchTextEntry.h"

#include <string.h>
#include <malloc.h>

#include "font.h"
#include "hdw-btn.h"
#include "hdw-tft.h"
#include "touchUtils.h"
#include "fill.h"
#include "shapes.h"
#include "macros.h"

//==============================================================================
// Macros
//==============================================================================

/// @brief Macro to check if a character is between two bounds
#define BTWN(chr, l, u) (l <= chr && chr <= u)
/// @brief Macro to check if a character is an uppercase letter
#define IS_UPPER(chr) BTWN(chr, 'A', 'Z')
/// @brief Macro to check if a character is a lowercase letter
#define IS_LOWER(chr) BTWN(chr, 'a', 'z')
/// @brief Macro to check if a character is a decimal digit
#define IS_NUMBER(chr) BTWN(chr, '0', '9')
/// @brief Macro to check if a character is a symbol
#define IS_SYMBOL(chr) (BTWN(chr, '!', '/') || BTWN(chr, ':', '@') || BTWN(chr, '[', '`') || BTWN(chr, '{', '~'))
/// @brief Macro to check if a character is whitespace
#define IS_WHITESPACE(chr) (chr == '\n' || chr == ' ' || chr == '\t')
/// @brief Macro to check if a character is printable
#define IS_PRINTABLE(chr) BTWN(chr, ' ', '~')

/// @brief Macro to match a character against any mask value
#define MATCH_CHAR(chr, mask)                                            \
    ((((mask & ENTRY_UPPERCASE) == ENTRY_UPPERCASE) && IS_UPPER(chr))    \
     || (((mask & ENTRY_LOWERCASE) == ENTRY_LOWERCASE) && IS_LOWER(chr)) \
     || (((mask & ENTRY_NUMBERS) == ENTRY_NUMBERS) && IS_NUMBER(chr))    \
     || (((mask & ENTRY_SYMBOLS) == ENTRY_SYMBOLS) && IS_SYMBOL(chr))    \
     || (((mask & ENTRY_WHITESPACE) == ENTRY_WHITESPACE) && IS_WHITESPACE(chr)))

//==============================================================================
// Defines
//==============================================================================

/// @brief The number of degrees of spin required to advance the character
#define SPIN_DEG_PER_CHAR 12

/// @brief The time of each blink half cycle (on-to-off or off-to-on)
#define BLINK_TIME 400000
/// @brief The time a button must be held before it starts repeating
#define REPEAT_DELAY 500000
/// @brief The time between each repeated button press
#define REPEAT_TIME 100000

#define REPEATABLE_BUTTONS (PB_UP | PB_DOWN | PB_LEFT | PB_RIGHT | PB_B | PB_A)

//==============================================================================
// Function Prototypes
//==============================================================================

static char prevChar(char cur, textEntryCharMask_t mask);
static char nextChar(char cur, textEntryCharMask_t mask);
static void checkBuffer(textEntry_t* entry);

static void cursorLeft(textEntry_t* entry);
static void cursorRight(textEntry_t* entry);
static void updateChar(textEntry_t* entry);
static void insertChar(textEntry_t* entry);
static void deleteChar(textEntry_t* entry);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Return the previous character from the given one, respecting the given mask and wrapping around
 *
 * @param cur The character to return the previous character for
 * @param mask The mask of acceptable character types
 * @return char The previous matching character
 */
static char prevChar(char cur, textEntryCharMask_t mask)
{
    do
    {
        if (cur <= ' ' || cur > '~')
        {
            cur = '~';
        }
        else
        {
            cur--;
        }
    } while (!MATCH_CHAR(cur, mask));

    return cur;
}

/**
 * @brief Returns the next character from the given one, respecting the given mask and wrapping around
 *
 * @param cur The character to return the next character for
 * @param mask The mask of acceptable character types
 * @return char The next matching character
 */
static char nextChar(char cur, textEntryCharMask_t mask)
{
    do
    {
        if (cur < ' ' || cur >= '~')
        {
            cur = ' ';
        }
        else
        {
            cur++;
        }
    } while (!MATCH_CHAR(cur, mask));

    return cur;
}

/**
 * @brief Ensure that the text entry has sufficient buffer space for at least one additional character,
 * expanding the buffer if necessary
 *
 * @param entry The text entry to check
 */
static void checkBuffer(textEntry_t* entry)
{
    if (entry->cursor + 2 >= entry->size)
    {
        entry->size *= 2;
        entry->value = realloc(entry->value, entry->size);

        // Zero out the remainder of the memory
        for (char* chr = entry->value + strlen(entry->value); chr < entry->value + entry->size; ++chr)
        {
            *chr = '\0';
        }
    }
}

/**
 * @brief Move the text entry's cursor to the left by one character, if possible
 *
 * @param entry The text entry to move the cursor of
 */
static void cursorLeft(textEntry_t* entry)
{
    if (entry->cursor > 0)
    {
        entry->cursor--;

        if (entry->offset > 0)
        {
            entry->offset--;
        }

        entry->blinkTimer = BLINK_TIME;
        entry->blinkState = true;
    }

    // If we're in overtype and between chars, set the char at the cursor to that character
    if (entry->overtype && entry->value[entry->cursor])
    {
        entry->cur = entry->value[entry->cursor];

        if (!entry->pendingChar && (entry->maxLength == 0 || strlen(entry->value) < entry->maxLength))
        {
            entry->pendingChar = true;
        }
    }
}

/**
 * @brief Move the text entry's cursor to the right by one character, if possible
 *
 * @param entry The text entry to move the cursor of
 */
static void cursorRight(textEntry_t* entry)
{
    if (entry->heldButton != PB_RIGHT && (entry->maxLength == 0 || entry->cursor < entry->maxLength)
        && (entry->mask & ENTRY_WHITESPACE) == ENTRY_WHITESPACE && !entry->value[entry->cursor]
        && (entry->cursor == 0 || entry->value[entry->cursor-1] != ' '))
    {
        // Add a space, if allowed and at the end
        entry->value[entry->cursor] = ' ';
        entry->cursor++;
        entry->blinkTimer = BLINK_TIME;
        entry->blinkState = true;
    }
    else if (entry->cursor < entry->size && (entry->maxLength == 0 || entry->cursor < entry->maxLength)
             && entry->cursor < strlen(entry->value))
    {
        entry->cursor++;

        entry->blinkTimer = BLINK_TIME;
        entry->blinkState = true;
    }

    // If we're in overtype and between chars, set the char at the cursor to that character
    if (entry->overtype && entry->value[entry->cursor])
    {
        entry->cur = entry->value[entry->cursor];

        if (entry->maxLength != 0 && strlen(entry->value) >= entry->maxLength)
        {
            entry->pendingChar = false;
        }
        else if (!entry->pendingChar)
        {
            entry->pendingChar = true;
        }
    }
}

/**
 * @brief Replace the character at the text entry's cursor with the current pending character and advance the cursor by
 * one character
 *
 * @param entry The text entry to update the character for
 */
static void updateChar(textEntry_t* entry)
{
    if (entry->cursor < entry->maxLength)
    {
        entry->value[entry->cursor] = entry->cur;
        entry->cursor++;
    }
}

/**
 * @brief Insert the current pending character at the text entry cursor's position, shifting any later characters
 *
 * @param entry The text entry to insert the character in
 */
static void insertChar(textEntry_t* entry)
{
    // Don't allow inserting if we're at the length limit
    if (entry->maxLength != 0 && strlen(entry->value) >= entry->maxLength)
    {
        entry->pendingChar = false;
        return;
    }

    // Shift all the characters after the cursor back
    uint16_t n;
    for (n = strlen(entry->value); n > entry->cursor; --n)
    {
        entry->value[n + 1] = entry->value[n];
    }

    if (entry->cursor < strlen(entry->value))
    {
        entry->value[entry->cursor + 1] = entry->value[entry->cursor];
    }

    entry->value[entry->cursor] = entry->cur;
    entry->cursor++;

    if (entry->maxLength != 0 && strlen(entry->value) >= entry->maxLength)
    {
        entry->pendingChar = false;
    }

    entry->blinkTimer = BLINK_TIME;
    entry->blinkState = true;
}

/**
 * @brief Delete the character at the text entry's cursor, shifting any later characters
 *
 * @param entry The text entry to delete the character within
 */
static void deleteChar(textEntry_t* entry)
{
    if (entry->pendingChar)
    {
        entry->pendingChar = false;
    }
    else
    {
        if (entry->cursor > 0)
        {
            uint16_t n;
            for (n = entry->cursor - 1; n + 1 < strlen(entry->value); ++n)
            {
                entry->value[n] = entry->value[n + 1];
            }

            entry->value[n] = '\0';
            entry->cursor--;

            if (entry->offset > 0)
            {
                entry->offset--;
            }
        }
        entry->pendingChar = false;

        entry->blinkTimer = BLINK_TIME;
        entry->blinkState = true;
    }
}

/**
 * @brief Allocate and return a new text entry. The text entry must be freed with freeTextEntry()
 *
 * @param x The X coordinate of the left edge of the text box
 * @param y The Y coordinate of the top of the text box
 * @param w The width of the text box, in pixels
 * @param length The maximum text length, or 0 for no limit
 * @param font The font to use when drawing the text entry
 * @param mask The mask of allowable character types, or 0 for all characters
 * @param cbFn The function to call once text entry is complete
 * @return textEntry_t*
 */
textEntry_t* initTextEntry(uint16_t x, uint16_t y, uint16_t w, uint16_t length, const font_t* font,
                           textEntryCharMask_t mask, textEntryCb cbFn)
{
    textEntry_t* entry = calloc(1, sizeof(textEntry_t));

    entry->minLength = 1;
    entry->maxLength = length;

    entry->x = x;
    entry->y = y;
    entry->w = w;

    entry->value = calloc(1, 32);
    entry->size  = 32;

    entry->cursor = 0;
    entry->font   = font;

    entry->mask = (mask != 0) ? mask : ENTRY_ALL;

    // Try to set the character to 'A', but respect the mask
    entry->cur         = nextChar('A' - 1, entry->mask);
    entry->pendingChar = true;

    entry->cbFn = cbFn;

    return entry;
}

/**
 * @brief Frees any memory associated with a text entry
 *
 * @param textEntry The text entry to be deallocated
 */
void freeTextEntry(textEntry_t* textEntry)
{
    free(textEntry->value);
    free(textEntry);
}

/**
 * @brief Sets the data pointer to be passed along to the callback function
 *
 * @param textEntry The text entry to update the data for
 * @param data The void pointer to be passed when the callback is called
 */
void textEntrySetData(textEntry_t* textEntry, void* data)
{
    textEntry->data = data;
}

/**
 * @brief Copy the given text into the text entry, replacing any text that was there and moving the cursor to the end of
 * the new text
 *
 * @param textEntry The text entry to update the text for
 * @param text The text to set as the text entry's value
 */
void textEntrySetText(textEntry_t* textEntry, const char* text)
{
    bool resized = false;
    while (textEntry->size < strlen(text) + 2)
    {
        resized = true;
        textEntry->size *= 2;
    }

    if (resized)
    {
        textEntry->value = realloc(textEntry->value, textEntry->size);
    }

    strncpy(textEntry->value, text, textEntry->size);

    // Zero out the remainder of the memory
    for (char* chr = textEntry->value + strlen(text); chr < textEntry->value + textEntry->size; ++chr)
    {
        *chr = '\0';
    }

    textEntry->offset      = 0;
    textEntry->cursor      = strlen(text);
    textEntry->pendingChar = false;
}

/**
 * @brief Update timer logic and handle touchpad for a text entry
 *
 * @param textEntry The text entry to update
 * @param elapsedUs The number of nanoseconds elapsed since the last call
 */
void textEntryMainLoop(textEntry_t* textEntry, int64_t elapsedUs)
{
    if (textEntry->heldButton)
    {
        if (textEntry->repeatTimer > elapsedUs)
        {
            textEntry->repeatTimer -= elapsedUs;
        }
        else
        {
            buttonEvt_t evt;
            evt.state  = textEntry->heldButton;
            evt.button = textEntry->heldButton;
            evt.down   = true;

            textEntryButton(textEntry, &evt);

            // Reset the timer and add in the remainder
            textEntry->repeatTimer = REPEAT_TIME - (elapsedUs - textEntry->repeatTimer);
        }
    }

    if (textEntry->blinkTimer > elapsedUs)
    {
        textEntry->blinkTimer -= elapsedUs;
    }
    else
    {
        textEntry->blinkState = !textEntry->blinkState;
        textEntry->blinkTimer = BLINK_TIME - (elapsedUs - textEntry->blinkTimer);
    }

    if (textEntry->maxLength == 0 || strlen(textEntry->value) < textEntry->maxLength
        || (textEntry->overtype && textEntry->cursor < strlen(textEntry->value)))
    {
        int32_t phi, r, intensity;
        if (getTouchJoystick(&phi, &r, &intensity))
        {
            textEntry->pendingChar = true;
            getTouchSpins(&textEntry->spinState, phi, r);

            if (!textEntry->spinCharStart)
            {
                if (textEntry->overtype && textEntry->cursor < strlen(textEntry->value))
                {
                    textEntry->cur = textEntry->value[textEntry->cursor];
                }
                textEntry->spinCharStart = textEntry->cur;
            }

            char chr = textEntry->spinCharStart;
            int16_t charChange
                = (textEntry->spinState.spins * 360 + textEntry->spinState.remainder) / SPIN_DEG_PER_CHAR;

            for (uint16_t i = 0; i < ABS(charChange); i++)
            {
                if (charChange < 0)
                {
                    chr = nextChar(chr, textEntry->mask);
                }
                else
                {
                    chr = prevChar(chr, textEntry->mask);
                }
            }

            if (chr != textEntry->cur)
            {
                textEntry->blinkState = true;
                textEntry->blinkTimer = BLINK_TIME;
            }

            textEntry->cur = chr;
        }
        else
        {
            textEntry->spinState.startSet = false;
            textEntry->spinCharStart      = 0;
        }
    }
}

/**
 * @brief Handle a button event for a text entry to modify the text or confirm entry
 *
 * @param textEntry The text entry to handle the button event
 * @param evt A pointer to the button event to be handled
 */
void textEntryButton(textEntry_t* textEntry, const buttonEvt_t* evt)
{
    if (evt->down)
    {
        checkBuffer(textEntry);

        switch (evt->button)
        {
            case PB_UP:
            {
                if (textEntry->pendingChar)
                {
                    textEntry->cur = prevChar(textEntry->cur, textEntry->mask);
                }
                else if (textEntry->maxLength == 0 || strlen(textEntry->value) < textEntry->maxLength
                         || (textEntry->overtype && textEntry->cursor < strlen(textEntry->value)))
                {
                    textEntry->pendingChar = true;
                    if (textEntry->overtype)
                    {
                        textEntry->cur = textEntry->value[textEntry->cursor];
                    }
                }

                textEntry->blinkTimer = BLINK_TIME;
                textEntry->blinkState = true;
                break;
            }

            case PB_DOWN:
            {
                if (textEntry->pendingChar)
                {
                    textEntry->cur = nextChar(textEntry->cur, textEntry->mask);
                }
                else if (textEntry->maxLength == 0 || strlen(textEntry->value) < textEntry->maxLength
                         || (textEntry->overtype && textEntry->cursor < strlen(textEntry->value)))
                {
                    textEntry->pendingChar = true;
                    if (textEntry->overtype)
                    {
                        textEntry->cur = textEntry->value[textEntry->cursor];
                    }
                }

                textEntry->blinkTimer = BLINK_TIME;
                textEntry->blinkState = true;
                break;
            }

            case PB_LEFT:
            {
                // textEntry->cur = textEntry->value[textEntry->cursor];
                cursorLeft(textEntry);
                break;
            }

            case PB_RIGHT:
            {
                // textEntry->value[textEntry->cursor] = textEntry->cur;
                cursorRight(textEntry);
                break;
            }

            case PB_A:
            {
                if (textEntry->pendingChar)
                {
                    if (textEntry->overtype)
                    {
                        updateChar(textEntry);
                    }
                    else
                    {
                        insertChar(textEntry);
                    }
                }
                else
                {
                    // If we're in overtype and between chars, set the char at the cursor to that character
                    if (textEntry->overtype && textEntry->value[textEntry->cursor])
                    {
                        textEntry->cur = textEntry->value[textEntry->cursor];
                    }
                    textEntry->pendingChar = true;
                }
                break;
            }

            case PB_B:
            {
                deleteChar(textEntry);
                break;
            }

            case PB_START:
            case PB_SELECT:
                // Both handled on key up instead
                break;
        }

        if ((evt->button & REPEATABLE_BUTTONS) == evt->button)
        {
            if (textEntry->heldButton != evt->button)
            {
                textEntry->heldButton = evt->button;

                textEntry->repeatTimer = REPEAT_DELAY;
            }
        }
    }
    else
    {
        switch (evt->button)
        {
            case PB_UP:
            case PB_DOWN:
            case PB_LEFT:
            case PB_RIGHT:
            case PB_A:
            case PB_B:
            {
                if (textEntry->heldButton == evt->button)
                {
                    textEntry->heldButton = 0;
                }
                break;
            }

            case PB_SELECT:
            {
                textEntry->overtype = !textEntry->overtype;
                break;
            }

            case PB_START:
            {
                if (textEntry->cbFn && strlen(textEntry->value) >= textEntry->minLength)
                {
                    textEntry->cbFn(textEntry->value, textEntry->data);
                }
                break;
            }
        }
    }
}

/**
 * @brief Draw a text entry box
 *
 * @param textEntry The text entry to draw
 * @param fg The color to use for drawing text and "more" dots
 * @param bg The color to use for drawing inverted text and, if drawBox is enabled, the background
 * @param drawBox Whether to fill the text entry box area's background
 */
void drawTextEntry(textEntry_t* textEntry, paletteColor_t fg, paletteColor_t bg, bool drawBox)
{
    uint16_t letterSpacing = 2;
    uint16_t x             = textEntry->x + 5;
    uint16_t y             = textEntry->y;

    if (drawBox)
    {
        fillDisplayArea(textEntry->x - 2, textEntry->y - 2, textEntry->x + textEntry->w + 2,
                        textEntry->y + textEntry->font->height + 3, bg);
    }

    // This is where we start drawing, by default at the beginning of the string, sensibly
    const char* startChr = textEntry->value + textEntry->offset;
    uint16_t curWidth    = (textEntry->pendingChar && IS_PRINTABLE(textEntry->cur))
                               ? (letterSpacing + textEntry->font->chars[textEntry->cur - ' '].width)
                               : 0;

    while (textEntry->offset < textEntry->cursor
           && textWidth(textEntry->font, startChr) + letterSpacing * (MAX(1, strlen(startChr)) - 1) + curWidth
                  >= MIN(TFT_WIDTH, textEntry->x + textEntry->w - 5))
    {
        textEntry->offset++;
        startChr++;
    }

    // Draw a ... at the beginning
    if (textEntry->offset > 0)
    {
        fillDisplayArea(textEntry->x - 1, textEntry->y + textEntry->font->height / 2 - 1, textEntry->x + 1,
                        textEntry->y + textEntry->font->height / 2 + 1, fg);
        fillDisplayArea(textEntry->x + 2, textEntry->y + textEntry->font->height / 2 - 1, textEntry->x + 4,
                        textEntry->y + textEntry->font->height / 2 + 1, fg);
    }

    // TODO handle shifting the display over if the text doesn't fit
    for (const char* chr = startChr; chr <= textEntry->value + strlen(textEntry->value); ++chr)
    {
        bool invert      = false;
        bool drawPending = false;

        if (textEntry->cursor == (chr - textEntry->value))
        {
            drawPending = (textEntry->maxLength == 0 || strlen(textEntry->value) < textEntry->maxLength
                           || (textEntry->overtype && textEntry->cursor < strlen(textEntry->value)))
                          && textEntry->pendingChar && IS_PRINTABLE(textEntry->cur);
            if (textEntry->overtype)
            {
                // Overtype: Highlight this character, draw pending char instead if enabled
                invert = true;
            }
            else
            {
                // Insert: Draw a cursor before the char
                if (textEntry->blinkState)
                {
                    drawLineFast(x - 1, y - 1, x - 1, y + textEntry->font->height + 1, fg);
                }
            }
        }

        if (!IS_PRINTABLE((drawPending ? textEntry->cur : *chr))
            || x + textEntry->font->chars[(drawPending ? textEntry->cur : *chr) - ' '].width
                   >= textEntry->x + textEntry->w - 5)
        {
            if (IS_PRINTABLE((drawPending ? textEntry->cur : *chr)))
            {
                // Overflow!
                fillDisplayArea(x, textEntry->y + textEntry->font->height / 2 - 1, x + 2,
                                textEntry->y + textEntry->font->height / 2 + 1, fg);
                fillDisplayArea(x + 3, textEntry->y + textEntry->font->height / 2 - 1, x + 5,
                                textEntry->y + textEntry->font->height / 2 + 1, fg);
            }
            break;
        }

        if (invert && IS_PRINTABLE((drawPending ? textEntry->cur : *chr)))
        {
            fillDisplayArea(x - 1, y - 1,
                            x + textEntry->font->chars[(drawPending ? textEntry->cur : *chr) - ' '].width
                                + letterSpacing - 1,
                            y + textEntry->font->height + 1, fg);
        }

        if (drawPending)
        {
            if (textEntry->blinkState)
            {
                drawChar(invert ? bg : fg, textEntry->font->height, &textEntry->font->chars[textEntry->cur - ' '], x,
                         y);
            }
            x += textEntry->font->chars[textEntry->cur - ' '].width + letterSpacing;
            invert = false;
        }

        if (*chr && IS_PRINTABLE(*chr) && (!drawPending || !textEntry->overtype))
        {
            drawChar(invert ? bg : fg, textEntry->font->height, &textEntry->font->chars[*chr - ' '], x, y);

            x += textEntry->font->chars[*chr - ' '].width + letterSpacing;
        }

        drawPending = false;
    }
}
