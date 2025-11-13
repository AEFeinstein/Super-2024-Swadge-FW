#include "textEntry.h"

//==============================================================================
// Internal function declarations
//==============================================================================

/**
 * @brief Updated text entry with more options
 *
 * @param endH Where the keyboard ends
 * @param elapsedUs How many ms have elapsed since last time function was called
 * @return int16_t Top of textbox for next element
 */
static int16_t _drawStr(int16_t endH, int64_t elapsedUs);

/**
 * @brief Draws the cursor at the end of the line
 *
 * @param eUs used to calculate if cursor should toggle on or off.
 * @param end The end of the line, used to calculate position.
 * @param h Height value to start at
 */
static void _drawCursor(int64_t eUs, int16_t pos, int16_t h);

/**
 * @brief Draws the keyboard
 *
 * @return int16_t top of keyboard
 */
static int16_t _drawKeyboard(void);

/**
 * @brief Draws the custom caps lock character
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
 * @return int16_t width of bar
 */
static int16_t _drawSpacebar(int16_t x, int16_t y, uint8_t color);

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
 * @return int  Width of the symbol for selection box drawing
 */
static int _drawEnter(int16_t x, int16_t y, uint8_t color);

/**
 * @brief Draws some text indicating the typing mode
 *
 */
static void _drawTypeMode(void);

/**
 * @brief Draws the text prompt as long as it exists.
 *
 * @param hPos Top edge of the previous textbox
 */
static void _drawPrompt(int16_t hPos);

//==============================================================================
// Variables
//==============================================================================

// Text entry
static int texLen;
static char* texString;
static bool multi;
static keyModifier_t keyMod;
static int8_t selX;
static int8_t selY;
static char selChar;
static char promptString[32];

// Graphical
bgMode_t backgroundMode;
static uint8_t textColor;
static uint8_t emphasisColor;
static uint8_t bgColor;
static bool useShadowboxes;
static uint8_t shadowboxColor;
static bool enterNewStyle;
static bool capsNewStyle;
static uint64_t cursorTimer;
static bool cursorToggle;
static uint8_t wideChar;

// Resources
static wsg_t* bgImage;
static font_t* activeFont;

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

static const uint8_t lengthPerLine[] = {14, 14, 13, 12, 1};

//==============================================================================
// Functions
//==============================================================================

void textEntryInit(font_t* useFont, int max_len, char* buffer)
{
    // Set incoming data
    texLen     = max_len;
    texString  = buffer;
    activeFont = useFont;
    wideChar   = activeFont->chars[MAX_WIDTH_CHAR].width;

    // Initialize necessary variables
    multi        = false;
    selX         = 1;
    selY         = 1;
    keyMod       = SHIFT;
    texString[0] = 0;
    cursorTimer  = 0;
    cursorToggle = true;
    strcpy(promptString, "");

    // Initialize default colors and BG mode
    backgroundMode = COLOR_BG;
    bgColor        = c000;
    textColor      = c555;
    emphasisColor  = c555;
    useShadowboxes = false;
    enterNewStyle  = true;
    capsNewStyle   = true;
}

bool textEntryDraw(int64_t elapsedUs)
{
    // If we're done, return false
    if (keyMod == SPECIAL_DONE)
    {
        return false;
    }
    // Background
    switch (backgroundMode)
    {
        case COLOR_BG:
            fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, bgColor);
            break;
        case WSG_BG:
            drawWsg(bgImage, 0, 0, false, false, 0);
            break;
        case CLEAR_BG:
            // Do nothing! Rely on BG being drawing each cycle by something else, or the keyboard will start
            // ghosting
            break;
        default:
            fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);
    }
    // Draw an indicator for the current key modifier
    _drawTypeMode();
    // Draw the rest of the owl
    _drawPrompt(_drawStr(_drawKeyboard(), elapsedUs));
    return true;
}

bool textEntryInput(uint8_t down, uint8_t button)
{
    // If this was a release, just return true
    if (!down)
    {
        return true;
    }

    // If text entry is done, return false
    if (keyMod == SPECIAL_DONE)
    {
        return false;
    }

    // Handle the button
    switch (button)
    {
        case PB_A:
        {
            // User selected this key
            int stringLen = strlen(texString);
            switch (selChar)
            {
                case KEY_ENTER:
                {
                    // Enter key was pressed, so text entry is done
                    keyMod = SPECIAL_DONE;
                    return false;
                }
                case KEY_SHIFT:
                {
                    if (keyMod == SHIFT)
                    {
                        keyMod = PROPER_NOUN;
                    }
                    else if (keyMod == PROPER_NOUN)
                    {
                        keyMod = NO_SHIFT;
                    }
                    else
                    {
                        keyMod = SHIFT;
                    }
                    break;
                }
                case KEY_CAPS_LOCK:
                {
                    if (CAPS_LOCK == keyMod)
                    {
                        keyMod = NO_SHIFT;
                    }
                    else
                    {
                        keyMod = CAPS_LOCK;
                    }
                    break;
                }
                case KEY_BACKSPACE:
                {
                    // If there is any text, delete the last char
                    if (stringLen > 0)
                    {
                        texString[stringLen - 1] = 0;
                    }
                    break;
                }
                case KEY_TAB:
                {
                    selChar = KEY_SPACE;
                    // Intentional fallthrough
                }
                    __attribute__((fallthrough));
                default:
                {
                    // If there is still space, add the selected char
                    if (stringLen < texLen - 1)
                    {
                        texString[stringLen]     = selChar;
                        texString[stringLen + 1] = 0;

                        // Clear shift if it as active
                        if (keyMod == SHIFT)
                        {
                            keyMod = NO_SHIFT;
                        }
                    }
                    break;
                }
            }
            break;
        }
        case PB_B:
        {
            int stringLen = strlen(texString);
            // If there is any text, delete the last char
            if (stringLen > 0)
            {
                texString[stringLen - 1] = 0;
            }
            break;
        }
        case PB_LEFT:
        {
            // Move cursor
            selX--;
            break;
        }
        case PB_DOWN:
        {
            // Move cursor
            selY++;
            break;
        }
        case PB_RIGHT:
        {
            // Move cursor
            selX++;
            break;
        }
        case PB_UP:
        {
            // Move cursor
            selY--;
            break;
        }
        case PB_SELECT:
        {
            // Rotate the keyMod from NO_SHIFT -> SHIFT -> PROPER_NOUN -> CAPS LOCK, and back
            if (NO_SHIFT == keyMod)
            {
                keyMod = SHIFT;
            }
            else if (SHIFT == keyMod)
            {
                keyMod = CAPS_LOCK;
            }
            else if (CAPS_LOCK == keyMod)
            {
                keyMod = PROPER_NOUN;
            }
            else
            {
                keyMod = NO_SHIFT;
            }
            break;
        }
        case PB_START:
        {
            // Move cursor to enter
            selX = ENTER_X;
            selY = ENTER_Y;
        }
        default:
        {
            break;
        }
    }

    // Make sure the cursor is in bounds, wrap around if necessary
    if (selY < 0)
    {
        selY = KB_LINES - 1;
    }
    else if (selY >= KB_LINES)
    {
        selY = 0;
    }

    // Make sure the cursor is in bounds, wrap around if necessary
    if (selX < 0)
    {
        selX = lengthPerLine[selY] - 1;
    }
    else if (selX >= lengthPerLine[selY])
    {
        selX = 0;
    }

    // All done, still entering text
    return true;
}

// Setters

void textEntrySetFont(font_t* newFont)
{
    activeFont = newFont;
    wideChar   = activeFont->chars[MAX_WIDTH_CHAR].width;
}

void textEntrySetBgWsg(wsg_t* BG)
{
    backgroundMode = WSG_BG;
    bgImage        = BG;
}

void textEntrySetBGColor(uint8_t color)
{
    backgroundMode = COLOR_BG;
    bgColor        = color;
}

void textEntrySetBGTransparent()
{
    backgroundMode = CLEAR_BG;
}

void textEntrySetTextColor(uint8_t color, bool emphasis)
{
    textColor = color;
    if (emphasis)
    {
        emphasisColor = color;
    }
}

void textEntrySetEmphasisColor(uint8_t color)
{
    emphasisColor = color;
}

void textEntrySetShadowboxColor(bool active, uint8_t color)
{
    useShadowboxes = active;
    shadowboxColor = color;
}

void textEntrySetNewEnterStyle(bool newStyle)
{
    enterNewStyle = newStyle;
}

void textEntrySetNewCapsStyle(bool newStyle)
{
    capsNewStyle = newStyle;
}

void textEntrySetMultiline(bool multiline)
{
    multi = multiline;
}

void textEntrySoftReset()
{
    keyMod = NO_SHIFT;
}

void textEntrySetPrompt(char* prompt)
{
    strcpy(promptString, prompt);
}

void textEntrySetCapMode()
{
    keyMod = CAPS_LOCK;
}

void textEntrySetNoShiftMode()
{
    keyMod = NO_SHIFT;
}

void textEntrySetShiftMode()
{
    keyMod = SHIFT;
}

void textEntrySetNounMode()
{
    keyMod = PROPER_NOUN;
}

// Drawing code

static int16_t _drawStr(int16_t endH, int64_t eUs)
{
    if (multi)
    {
        int16_t startX = CORNER_MARGIN;
        int16_t startY = CORNER_MARGIN;
        int16_t endX   = TFT_WIDTH - CORNER_MARGIN;
        int16_t endY   = endH - CORNER_MARGIN;
        if (useShadowboxes)
        {
            fillDisplayArea(startX - SHADOWBOX_MARGIN, startY - SHADOWBOX_MARGIN, endX + SHADOWBOX_MARGIN,
                            endY + SHADOWBOX_MARGIN, shadowboxColor);
        }
        drawTextWordWrap(activeFont, textColor, texString, &startX, &startY, endX, endY);
        return startY - SHADOWBOX_MARGIN;
    }
    else
    {
        int16_t hStart = endH - ((activeFont->height + SHADOWBOX_MARGIN) * 2);
        if (useShadowboxes)
        {
            fillDisplayArea(CORNER_MARGIN - SHADOWBOX_MARGIN, hStart - SHADOWBOX_MARGIN,
                            TFT_WIDTH - CORNER_MARGIN + SHADOWBOX_MARGIN,
                            hStart + activeFont->height + SHADOWBOX_MARGIN, shadowboxColor);
        }
        int16_t textLen = textWidth(activeFont, texString) + activeFont->chars[0].width;
        int16_t endPos  = drawText(activeFont, textColor, texString, (TFT_WIDTH - textLen) / 2, hStart);
        _drawCursor(eUs, endPos, hStart);
        return hStart - SHADOWBOX_MARGIN;
    }
}

static void _drawCursor(int64_t eUs, int16_t end, int16_t h)
{
    cursorTimer += eUs;
    if (BLINK_RATE < cursorTimer)
    {
        cursorToggle = !cursorToggle;
        cursorTimer  = 0;
    }
    if (cursorToggle)
    {
        drawLineFast(end + 1, h - 2, end + 1, h + activeFont->height + 3, emphasisColor);
    }
}

static int16_t _drawKeyboard()
{
    int16_t width          = wideChar + 2;
    int16_t StartX         = TFT_WIDTH / 2 - (lengthPerLine[0] * (width + KEY_SPACING)) / 2;
    int16_t keyboardHeight = KB_LINES * (activeFont->height + KEY_SPACING);
    int16_t StartY         = TFT_HEIGHT - (keyboardHeight + activeFont->height + (3 * SHADOWBOX_MARGIN));
    if (useShadowboxes)
    {
        fillDisplayArea(StartX - SHADOWBOX_MARGIN, StartY - SHADOWBOX_MARGIN, TFT_WIDTH - StartX + SHADOWBOX_MARGIN,
                        StartY + keyboardHeight, shadowboxColor);
    }
    int col = 0;
    int row = 0;
    char c;
    int stringLen = strlen(texString);
    const char* s;
    if (keyMod == NO_SHIFT || (keyMod == PROPER_NOUN && !(texString[stringLen - 1] == KEY_SPACE || stringLen == 0)))
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
            width    = activeFont->chars[32].width + 2;
            int posX = col * (width + KEY_SPACING) + StartX + row * (width / 2 + 1);
            int posY = row * (activeFont->height + KEY_SPACING) + StartY;
            // Draw the character, may be a control char
            switch (c)
            {
                case KEY_CAPS_LOCK:
                    if (keyMod == CAPS_LOCK)
                    {
                        _drawCaps(posX, posY, emphasisColor);
                    }
                    else
                    {
                        _drawCaps(posX, posY, textColor);
                    }
                    break;
                case KEY_SHIFT:
                    if (keyMod == PROPER_NOUN)
                    {
                        _drawShift(posX, posY, emphasisColor);
                    }
                    else
                    {
                        _drawShift(posX, posY, textColor);
                    }
                    break;
                case KEY_BACKSPACE:
                    _drawBackspace(posX, posY, textColor);
                    break;
                case KEY_SPACE:
                    width = _drawSpacebar(posX, posY, textColor);
                    break;
                case KEY_TAB:
                    _drawTab(posX, posY, textColor);
                    break;
                case KEY_ENTER:
                    width = _drawEnter(posX, posY, textColor);
                    break;
                default:
                {
                    // Just draw the char
                    char sts[2];
                    sts[0] = c;
                    sts[1] = '\0';
                    drawText(activeFont, textColor, sts, posX, posY);
                    break;
                }
            }
            if (col == selX && row == selY)
            {
                // Draw Box around selected item.
                drawRect(posX - 2, posY - 2, posX + width, posY + activeFont->height + 4, textColor);
                selChar = c;
            }
            col++;
        }
        s++;
    }
    return StartY;
}

static void _drawCaps(int16_t x, int16_t y, uint8_t color)
{
    if (capsNewStyle)
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

static void _drawBackspace(int16_t x, int16_t y, uint8_t color)
{
    // Draw backspace
    drawLineFast(x + 0, y + 5, x + 6, y + 5, color); // -
    drawLineFast(x + 1, y + 4, x + 3, y + 2, color); // /
    drawLineFast(x + 2, y + 4, x + 4, y + 2, color); // / (extra thickness)
    drawLineFast(x + 1, y + 6, x + 3, y + 8, color); /* \ */
    drawLineFast(x + 2, y + 6, x + 4, y + 8, color); // \ (extra thickness)
}

static int16_t _drawSpacebar(int16_t x, int16_t y, uint8_t color)
{
    // Draw spacebar
    int16_t width = TFT_WIDTH - (2 * x);
    drawRect(x + 1, y + 1, x + width - 2, y + 3, color);
    return width;
}

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

static int _drawEnter(int16_t x, int16_t y, uint8_t color)
{
    if (!enterNewStyle)
    {
        drawText(activeFont, textColor, "OK", x, y);
        return textWidth(activeFont, "OK") + 2;
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

static void _drawTypeMode()
{
    static char* text;
    bool useLine = false;
    switch (keyMod)
    {
        case SHIFT:
            useLine = true;
            text    = "Typing: Upper";
            break;
        case NO_SHIFT:
            text = "Typing: Lower";
            break;
        case CAPS_LOCK:
            useLine = true;
            text    = "Typing: Caps";
            break;
        case PROPER_NOUN:
            text = "Typing: Proper Noun";
        default:
            break;
    }
    int16_t width       = textWidth(activeFont, text);
    int16_t typingWidth = textWidth(activeFont, "Typing: ");
    if (useShadowboxes)
    {
        fillDisplayArea((TFT_WIDTH - width) / 2 - SHADOWBOX_MARGIN,
                        TFT_HEIGHT - (activeFont->height + 4 + SHADOWBOX_MARGIN),
                        (TFT_WIDTH + width) / 2 + SHADOWBOX_MARGIN, TFT_HEIGHT, shadowboxColor);
    }
    drawText(activeFont, textColor, text, (TFT_WIDTH - width) / 2, TFT_HEIGHT - activeFont->height - 4);
    if (useLine)
    {
        drawLineFast((TFT_WIDTH - width) / 2 + typingWidth, TFT_HEIGHT - 2, (TFT_WIDTH - width) / 2 + width,
                     TFT_HEIGHT - 2, emphasisColor);
    }
}

static void _drawPrompt(int16_t hPos)
{
    // Abort if the string doesn't contain text
    if (strlen(promptString) <= 1)
    {
        return;
    }
    // If using multilines, place prompt higher and to the side to keep it out of the corner.
    if (multi)
    {
        int16_t width = textWidth(activeFont, promptString);
        if (useShadowboxes)
        {
            fillDisplayArea(CORNER_MARGIN, 0, CORNER_MARGIN + width + (2 * SHADOWBOX_MARGIN),
                            activeFont->height + (2 * SHADOWBOX_MARGIN), shadowboxColor);
        }
        drawText(activeFont, emphasisColor, promptString, CORNER_MARGIN + SHADOWBOX_MARGIN, SHADOWBOX_MARGIN);
    }
    else
    {
        int16_t width = textWidth(activeFont, promptString);
        if (useShadowboxes)
        {
            fillDisplayArea((TFT_WIDTH - width) / 2 - SHADOWBOX_MARGIN,
                            hPos - (activeFont->height + (3 * SHADOWBOX_MARGIN)),
                            (TFT_WIDTH + width) / 2 + (2 * SHADOWBOX_MARGIN), hPos - SHADOWBOX_MARGIN, shadowboxColor);
        }
        drawText(activeFont, emphasisColor, promptString, (TFT_WIDTH - width) / 2,
                 hPos - (activeFont->height + (2 * SHADOWBOX_MARGIN)));
    }
}