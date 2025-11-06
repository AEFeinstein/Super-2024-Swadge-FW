#include "textEntry.h"
#include "macros.h"

//==============================================================================
// Defines
//==============================================================================

// Font
#define NUM_KEYBOARD_SYMBOLS ('~' - ' ' + 2) // Used to iterate over the characters to get widest character.

// Keyboard
#define KB_LINES     5
#define RETURN_WIDTH 16 // Width of return symbol

// Graphics
#define CORNER_MARGIN    20 // Margin to clear the rounded corners
#define SHADOWBOX_MARGIN 4  // Margin around shadowboxes
#define KEY_SPACING      5  // Space between keys

// Timers
#define BLINK_RATE 500000 // Time in Us before blinker toggles

//==============================================================================
// Consts
//==============================================================================

// Uppercase character list
// See controlChar_t for definitions of hex values
static const char keyboard_upper[] = "\
~!@#$%^&*()_+\x03\x05\
\x09QWERTYUIOP{}|\x05\
\002ASDFGHJKL:\"\x0a\x05\
\x01ZXCVBNM<>?\x01\x05\
\x20";

// Lowercase character list
// See controlChar_t for definitions of hex values
static const char keyboard_lower[] = "\
`1234567890-=\x03\x05\
\x09qwertyuiop[]\\\x05\
\002asdfghjkl;\'\x0a\x05\
\x01zxcvbnm,./\x01\x05\
\x20";

static const char* const modeStrs[] = {
    "Typing: ", "Upper", "Lower", "Caps", "Proper Nouns",
};

static const uint8_t lengthPerLine[] = {14, 14, 13, 12, 1};
static const char okText[]           = "OK";

//==============================================================================
// Enums
//==============================================================================

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

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    // Text Array
    char* text;
    font_t* font;

    // Settings
    const textEntrySettings_t* tes;
    uint8_t widestChar; // Used to set the full width of the keyboard based on the
                        // widest character in the set

    // State
    int selX, selY;
    keyModifier_t kMod;
    char selChar;

    // Cursor blink
    int64_t blinkTimer;
    bool blinkState;
} textEntry_t;

//==============================================================================
// Internal function declarations
//==============================================================================

// Helpers
static void _textEntrySetWideChar(font_t* newFont);

// Draw major elements
static int16_t _drawStr(int16_t endH, int64_t elapsedUs);
static int16_t _drawKeyboard(void);
static void _drawCursor(int64_t eUs, int16_t pos, int16_t h);
static void _drawTypeMode(void);
static void _drawPrompt(int16_t hPos);

// Special characters
static void _drawCaps(int16_t x, int16_t y, uint8_t color);
static void _drawShift(int16_t x, int16_t y, uint8_t color);
static void _drawBackspace(int16_t x, int16_t y, uint8_t color);
static int16_t _drawSpacebar(int16_t x, int16_t y, uint8_t color);
static void _drawTab(int16_t x, int16_t y, uint8_t color);
static int _drawEnter(int16_t x, int16_t y, uint8_t color);

//==============================================================================
// Variables
//==============================================================================

textEntry_t* te;

//==============================================================================
// Functions
//==============================================================================

void textEntryInit(const textEntrySettings_t* settings, char* entryText, font_t* font)
{
    // Load memory
    te       = (textEntry_t*)heap_caps_calloc(1, sizeof(textEntry_t), MALLOC_CAP_8BIT);
    te->tes  = settings;
    te->text = entryText;
    te->font = font;

    // Set the widest character
    _textEntrySetWideChar(te->font);

    // Move to enter by default
    te->selX = 12;
    te->selY = 2;

    // Set defaults
    te->kMod       = te->tes->startKMod;
    te->blinkState = true;
}

void textEntryDeinit()
{
    heap_caps_free(te);
}

bool textEntryInput(buttonEvt_t evt)
{
    if (!evt.down)
    {
        // If this was a release, just return true
        return false;
    }
    else if (te->kMod == TE_SPECIAL_DONE)
    {
        // If text entry is done, return false
        return true;
    }

    // Handle the button
    switch (evt.button)
    {
        case PB_A:
        {
            // User selected this key
            int stringLen = strlen(te->text);
            switch (te->selChar)
            {
                case KEY_ENTER:
                {
                    // Enter key was pressed, so text entry is done
                    te->kMod = TE_SPECIAL_DONE;
                    return true;
                }
                case KEY_SHIFT:
                {
                    if (te->kMod == TE_SHIFT)
                    {
                        te->kMod = TE_PROPER_NOUN;
                    }
                    else if (te->kMod == TE_PROPER_NOUN)
                    {
                        te->kMod = TE_NO_SHIFT;
                    }
                    else
                    {
                        te->kMod = TE_SHIFT;
                    }
                    break;
                }
                case KEY_CAPS_LOCK:
                {
                    if (te->kMod == TE_CAPS_LOCK)
                    {
                        te->kMod = TE_NO_SHIFT;
                    }
                    else
                    {
                        te->kMod = TE_CAPS_LOCK;
                    }
                    break;
                }
                case KEY_BACKSPACE:
                {
                    // If there is any text, delete the last char
                    if (stringLen > 0)
                    {
                        te->text[stringLen - 1] = 0;
                    }
                    break;
                }
                case KEY_TAB:
                {
                    te->selChar = KEY_SPACE;
                    // Intentional fallthrough
                }
                    __attribute__((fallthrough));
                default:
                {
                    // If there is still space, add the selected char
                    if (stringLen < te->tes->maxLen - 1)
                    {
                        te->text[stringLen]     = te->selChar;
                        te->text[stringLen + 1] = 0;

                        // Clear shift if it as active
                        if (te->kMod == TE_SHIFT)
                        {
                            te->kMod = TE_NO_SHIFT;
                        }
                    }
                    break;
                }
            }
            break;
        }
        case PB_B:
        {
            int stringLen = strlen(te->text);
            // If there is any text, delete the last char
            if (stringLen > 0)
            {
                te->text[stringLen - 1] = 0;
            }
            break;
        }
        case PB_LEFT:
        {
            // Move cursor
            te->selX--;
            break;
        }
        case PB_RIGHT:
        {
            // Move cursor
            te->selX++;
            break;
        }
        case PB_DOWN:
        {
            te->selY++;
            if (te->selY >= KB_LINES)
            {
                te->selY = 0;
            }
            break;
        }
        case PB_UP:
        {
            te->selY--;
            if (te->selY < 0)
            {
                te->selY = KB_LINES - 1;
            }
            break;
        }
        case PB_START:
        {
            // Rotate the keyMod from NO_SHIFT -> SHIFT -> PROPER_NOUN -> CAPS LOCK, and back
            te->kMod++;
            if (te->kMod == TE_SPECIAL_DONE)
            {
                te->kMod = TE_NO_SHIFT;
            }
        }
        default:
        {
            break;
        }
    }
    // These are external to ensure the spacebar will work
    if (te->selX >= lengthPerLine[te->selY])
    {
        te->selX = 0;
    }
    else if (te->selX < 0)
    {
        te->selX = lengthPerLine[te->selY] - 1;
    }

    // Still entering text
    return false;
}

bool textEntryDraw(int64_t elapsedUs)
{
    // If we're done, return false
    if (te->kMod == TE_SPECIAL_DONE)
    {
        return false;
    }
    // Background
    if (te->tes->bgColor != cTransparent)
    {
        fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, te->tes->bgColor);
    }
    // Draw an indicator for the current key modifier
    _drawTypeMode();
    // Draw the rest of the owl
    _drawPrompt(_drawStr(_drawKeyboard(), elapsedUs));

    return true;
}

void textEntrySoftReset()
{
    te->kMod = te->tes->startKMod;
}

//==============================================================================
// Static functions
//==============================================================================

// Helpers
static void _textEntrySetWideChar(font_t* newFont)
{
    te->widestChar = 0;
    for (int idx = 0; idx < NUM_KEYBOARD_SYMBOLS; idx++)
    {
        if (te->font->chars[idx].width > te->widestChar)
        {
            te->widestChar = te->font->chars[idx].width;
        }
    }
}

// Draw major elements
/**
 * @brief Updated text entry with more options
 *
 * @param endH Where the keyboard ends
 * @param elapsedUs How many ms have elapsed since last time function was called
 * @return int16_t Top of textbox for next element
 */
static int16_t _drawStr(int16_t endH, int64_t eUs)
{
    if (te->tes->useMultiLine)
    {
        int16_t startX = CORNER_MARGIN;
        int16_t startY = CORNER_MARGIN;
        int16_t endX   = TFT_WIDTH - CORNER_MARGIN;
        int16_t endY   = endH - CORNER_MARGIN;
        if (te->tes->shadowboxColor != cTransparent)
        {
            fillDisplayArea(startX - SHADOWBOX_MARGIN, startY - SHADOWBOX_MARGIN, endX + SHADOWBOX_MARGIN,
                            endY + SHADOWBOX_MARGIN, te->tes->shadowboxColor);
        }
        drawTextWordWrap(te->font, te->tes->textColor, te->text, &startX, &startY, endX, endY);
        return startY - SHADOWBOX_MARGIN;
    }
    else
    {
        int16_t hStart = endH - ((te->font->height + SHADOWBOX_MARGIN) * 2);
        int16_t wStart = (TFT_WIDTH - (te->tes->maxLen * te->widestChar)) >> 1;
        if (te->tes->shadowboxColor != cTransparent)
        {
            fillDisplayArea(wStart, hStart - SHADOWBOX_MARGIN, wStart + (te->tes->maxLen * te->widestChar),
                            hStart + te->font->height + SHADOWBOX_MARGIN, te->tes->shadowboxColor);
        }
        int16_t textLen = textWidth(te->font, te->text) + te->font->chars[0].width;
        int16_t endPos  = drawText(te->font, te->tes->textColor, te->text, (TFT_WIDTH - textLen) / 2, hStart);
        _drawCursor(eUs, endPos, hStart);
        return hStart - SHADOWBOX_MARGIN;
    }
}

/**
 * @brief Draws the keyboard
 *
 * @return int16_t top of keyboard
 */
static int16_t _drawKeyboard()
{
    int16_t width          = te->widestChar + 2;
    int16_t StartX         = (TFT_WIDTH >> 1) - ((lengthPerLine[0] * (width + KEY_SPACING)) >> 1);
    int16_t keyboardHeight = KB_LINES * (te->font->height + KEY_SPACING);
    int16_t StartY         = TFT_HEIGHT - (keyboardHeight + te->font->height + (3 * SHADOWBOX_MARGIN));
    if (te->tes->shadowboxColor != cTransparent)
    {
        fillDisplayArea(StartX - SHADOWBOX_MARGIN, StartY - SHADOWBOX_MARGIN, TFT_WIDTH - StartX + SHADOWBOX_MARGIN,
                        StartY + keyboardHeight, te->tes->shadowboxColor);
    }
    int col = 0;
    int row = 0;
    char c;
    int stringLen = strlen(te->text);
    const char* s;
    if (te->kMod == TE_NO_SHIFT
        || (te->kMod == TE_PROPER_NOUN && !(te->text[stringLen - 1] == KEY_SPACE || stringLen == 0)))
    {
        s = keyboard_lower;
    }
    else
    {
        s = keyboard_upper;
    }
    while ((c = *s))
    {
        // EOL character hit, move to the next row
        if (c == KEY_EOL)
        {
            col = 0;
            row++;
        }
        else
        {
            // Recalculate each round because it is overwritten by spacial chars
            width    = te->font->chars[32].width + 2;
            int posX = col * (width + KEY_SPACING) + StartX + row * (width / 2 + 1);
            int posY = row * (te->font->height + KEY_SPACING) + StartY;
            // Draw the character, may be a control char
            switch (c)
            {
                case KEY_CAPS_LOCK:
                    if (te->kMod == TE_CAPS_LOCK)
                    {
                        _drawCaps(posX, posY, te->tes->emphasisColor);
                    }
                    else
                    {
                        _drawCaps(posX, posY, te->tes->textColor);
                    }
                    break;
                case KEY_SHIFT:
                    if (te->kMod == TE_PROPER_NOUN)
                    {
                        _drawShift(posX, posY, te->tes->emphasisColor);
                    }
                    else
                    {
                        _drawShift(posX, posY, te->tes->textColor);
                    }
                    break;
                case KEY_BACKSPACE:
                    _drawBackspace(posX, posY, te->tes->textColor);
                    break;
                case KEY_SPACE:
                    width = _drawSpacebar(posX, posY, te->tes->textColor);
                    break;
                case KEY_TAB:
                    _drawTab(posX, posY, te->tes->textColor);
                    break;
                case KEY_ENTER:
                    width = _drawEnter(posX, posY, te->tes->textColor);
                    break;
                default:
                {
                    // Just draw the char
                    char sts[2];
                    sts[0] = c;
                    sts[1] = '\0';
                    drawText(te->font, te->tes->textColor, sts, posX, posY);
                    break;
                }
            }
            if (col == te->selX && row == te->selY)
            {
                // Draw Box around selected item.
                drawRect(posX - 2, posY - 2, posX + width, posY + te->font->height + 4, te->tes->textColor);
                te->selChar = c;
            }
            col++;
        }
        s++;
    }
    return StartY;
}

/**
 * @brief Draws the cursor at the end of the line
 *
 * @param eUs used to calculate if cursor should toggle on or off.
 * @param end The end of the line, used to calculate position.
 * @param h Height value to start at
 */
static void _drawCursor(int64_t eUs, int16_t end, int16_t h)
{
    if (!te->tes->blink)
    {
        return;
    }
    RUN_TIMER_EVERY(te->blinkTimer, BLINK_RATE, eUs, te->blinkState = !te->blinkState;);
    if (te->blinkState)
    {
        drawLineFast(end + 1, h - 2, end + 1, h + te->font->height + 3, te->tes->emphasisColor);
    }
}

/**
 * @brief Draws some text indicating the typing mode
 *
 */
static void _drawTypeMode()
{
    static const char* text;
    bool useLine = false;
    switch (te->kMod)
    {
        case TE_SHIFT:
            useLine = true;
            text    = modeStrs[1];
            break;
        case TE_NO_SHIFT:
            text = modeStrs[2];
            break;
        case TE_CAPS_LOCK:
            useLine = true;
            text    = modeStrs[3];
            break;
        case TE_PROPER_NOUN:
            text = modeStrs[4];
        default:
            break;
    }
    int16_t typingWidth = textWidth(te->font, modeStrs[0]);
    int16_t width       = textWidth(te->font, text) + typingWidth;
    if (te->tes->shadowboxColor != cTransparent)
    {
        fillDisplayArea((TFT_WIDTH - width) / 2 - SHADOWBOX_MARGIN,
                        TFT_HEIGHT - (te->font->height + 4 + SHADOWBOX_MARGIN),
                        (TFT_WIDTH + width) / 2 + SHADOWBOX_MARGIN, TFT_HEIGHT, te->tes->shadowboxColor);
    }
    char buffer[32];
    snprintf(buffer, sizeof(buffer) - 1, "%s%s", modeStrs[0], text);
    drawText(te->font, te->tes->textColor, buffer, (TFT_WIDTH - width) / 2, TFT_HEIGHT - te->font->height - 4);
    if (useLine)
    {
        drawLineFast((TFT_WIDTH - width) / 2 + typingWidth, TFT_HEIGHT - 2, (TFT_WIDTH - width) / 2 + width,
                     TFT_HEIGHT - 2, te->tes->emphasisColor);
    }
}

/**
 * @brief Draws the text prompt as long as it exists.
 *
 * @param hPos Top edge of the previous textbox
 */
static void _drawPrompt(int16_t hPos)
{
    // Abort if the string doesn't contain text
    if (te->tes->textPrompt == NULL)
    {
        return;
    }
    // If using multilines, place prompt higher and to the side to keep it out of the corner.
    int16_t width = textWidth(te->font, te->tes->textPrompt);
    if (te->tes->useMultiLine)
    {
        if (te->tes->shadowboxColor != cTransparent)
        {
            fillDisplayArea(CORNER_MARGIN, 0, CORNER_MARGIN + width + (2 * SHADOWBOX_MARGIN),
                            te->font->height + (2 * SHADOWBOX_MARGIN), te->tes->shadowboxColor);
        }
        drawText(te->font, te->tes->emphasisColor, te->tes->textPrompt, CORNER_MARGIN + SHADOWBOX_MARGIN,
                 SHADOWBOX_MARGIN);
    }
    else
    {
        if (te->tes->shadowboxColor != cTransparent)
        {
            fillDisplayArea(
                (TFT_WIDTH - width) / 2 - SHADOWBOX_MARGIN, hPos - (te->font->height + (3 * SHADOWBOX_MARGIN)),
                (TFT_WIDTH + width) / 2 + (2 * SHADOWBOX_MARGIN), hPos - SHADOWBOX_MARGIN, te->tes->shadowboxColor);
        }
        drawText(te->font, te->tes->emphasisColor, te->tes->textPrompt, (TFT_WIDTH - width) / 2,
                 hPos - (te->font->height + (2 * SHADOWBOX_MARGIN)));
    }
}

// Special characters
/**
 * @brief Draws the custom caps lock character
 *
 * @param x     Starting x position
 * @param y     Starting y coordinate
 * @param color Color of the text
 */
static int16_t _drawSpacebar(int16_t x, int16_t y, uint8_t color)
{
    // Draw spacebar
    int16_t width = TFT_WIDTH - (2 * x);
    drawRect(x + 1, y + 1, x + width - 2, y + 3, color);
    return width;
}

/**
 * @brief Draws the custom shift key character
 *
 * @param x     Starting x position
 * @param y     Starting y coordinate
 * @param color Color of the text
 */
static void _drawBackspace(int16_t x, int16_t y, uint8_t color)
{
    // Draw backspace
    drawLineFast(x + 0, y + 5, x + 6, y + 5, color); // -
    drawLineFast(x + 1, y + 4, x + 3, y + 2, color); // /
    drawLineFast(x + 2, y + 4, x + 4, y + 2, color); // / (extra thickness)
    drawLineFast(x + 1, y + 6, x + 3, y + 8, color); /* \ */
    drawLineFast(x + 2, y + 6, x + 4, y + 8, color); // \ (extra thickness)
}

/**
 * @brief Draws the custom backspace character
 *
 * @param x     Starting x position
 * @param y     Starting y coordinate
 * @param color Color of the text
 */
static void _drawShift(int16_t x, int16_t y, uint8_t color)
{
    // Draw shift/caps lock
    drawLineFast(x + 1, y + 9, x + 5, y + 9, color); // -
    drawLineFast(x + 3, y + 2, x + 3, y + 9, color); // |
    drawLineFast(x + 0, y + 5, x + 2, y + 3, color); // /
    drawLineFast(x + 0, y + 6, x + 2, y + 4, color); // / (extra thickness)
    drawLineFast(x + 4, y + 3, x + 6, y + 5, color); /* \ */
    drawLineFast(x + 4, y + 4, x + 6, y + 6, color); // \ (extra thickness)
}

/**
 * @brief Draws the custom spacebar character
 *
 * @param x     Starting x position
 * @param y     Starting y coordinate
 * @param color Color of the text
 * @return int16_t width of bar
 */
static void _drawTab(int16_t x, int16_t y, uint8_t color)
{
    // Draw tab
    drawLineFast(x + 0, y + 2, x + 0, y + 8, color); // |
    drawLineFast(x + 0, y + 5, x + 6, y + 5, color); // -
    drawLineFast(x + 3, y + 2, x + 5, y + 4, color); // \ (not a multiline comment)
    drawLineFast(x + 2, y + 2, x + 4, y + 4, color); // \ (extra thickness)
    drawLineFast(x + 3, y + 8, x + 5, y + 6, color); // /
    drawLineFast(x + 2, y + 8, x + 4, y + 6, color); // / (extra thickness)
}

/**
 * @brief Draws the custom tab character
 *
 * @param x     Starting x position
 * @param y     Starting y coordinate
 * @param color Color of the text
 */
static void _drawCaps(int16_t x, int16_t y, uint8_t color)
{
    if (te->tes->useNewCapsStyle)
    {
        drawRect(x + 2, y + 8, x + 5, y + 10, color);    // box
        drawLineFast(x + 3, y + 0, x + 3, y + 6, color); // |
        drawLineFast(x + 2, y + 2, x + 2, y + 6, color); // | (extra thickness left)
        drawLineFast(x + 4, y + 2, x + 4, y + 6, color); // | (extra thickness right)
        drawLineFast(x + 0, y + 3, x + 2, y + 1, color); // /
        drawLineFast(x + 0, y + 4, x + 2, y + 2, color); // / (extra thickness)
        drawLineFast(x + 4, y + 1, x + 6, y + 3, color); /* \ */
        drawLineFast(x + 4, y + 2, x + 6, y + 4, color); // \ (extra thickness)
    }
    else
    {
        // Draw caps lock extra arrow body thickness
        drawLineFast(x + 2, y + 4, x + 2, y + 9, color); // | (extra thickness left)
        drawLineFast(x + 4, y + 4, x + 4, y + 9, color); // | (extra thickness right)
        _drawShift(x, y, color);
    }
}

/**
 * @brief Draws the custom enter kry character
 *
 * @param x     Starting x position
 * @param y     Starting y coordinate
 * @param color Color of the text
 * @return int  Width of the symbol for selection box drawing
 */
static int _drawEnter(int16_t x, int16_t y, uint8_t color)
{
    if (!te->tes->useOKEnterStyle)
    {
        drawText(te->font, te->tes->textColor, okText, x, y);
        return textWidth(te->font, okText) + 2;
    }
    else
    {
        // Draw return symbol
        drawLineFast(x + 0, y + 5, x + 12, y + 5, color);  // -
        drawLineFast(x + 1, y + 4, x + 3, y + 2, color);   // /
        drawLineFast(x + 2, y + 4, x + 4, y + 2, color);   // / (extra thickness)
        drawLineFast(x + 1, y + 6, x + 3, y + 8, color);   /* \ */
        drawLineFast(x + 2, y + 6, x + 4, y + 8, color);   // \ (extra thickness)
        drawLineFast(x + 12, y + 0, x + 12, y + 5, color); // |
        return RETURN_WIDTH;
    }
}
