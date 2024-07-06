#include "textEntry.h"

/*============================================================================
 * Internal draw function declarations
 *==========================================================================*/

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

/*============================================================================
 * Variables
 *==========================================================================*/

// Text entry
static int texLen;
static char* texString;
static keyModifier_t keyMod;
static int8_t selx;
static int8_t sely;
static char selChar;

// Graphical
static bool prettyGraphics;
static uint8_t textColor;
static uint8_t empahsisColor;
static uint8_t textBoxColor;
static uint64_t cursorTimer;
static bool cursorToggle;

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

static const uint8_t lengthperline[] = {14, 14, 13, 12, 1};

//==============================================================================
// Functions
//==============================================================================

void textEntryStart(font_t* usefont, int max_len, char* buffer)
{
    texLen         = max_len;
    texString      = buffer;
    selx           = 1;
    sely           = 1;
    keyMod         = NO_SHIFT;
    texString[0]   = 0;
    cursorTimer    = 0;
    cursorToggle   = true;
    activeFont     = usefont;
    textColor      = WHITE;
    empahsisColor  = WHITE;
    prettyGraphics = false;
}

void textEntryStartPretty(font_t* usefont, int max_len, char* buffer, wsg_t* BG)
{
    texLen         = max_len;
    texString      = buffer;
    selx           = 1;
    sely           = 1;
    keyMod         = NO_SHIFT;
    texString[0]   = 0;
    cursorTimer    = 0;
    cursorToggle   = true;
    activeFont     = usefont;
    prettyGraphics = true;
    bgImage        = BG;
}

bool textEntryDrawBlink(int64_t elapsedUs)
{
    // If we're done, return false
    if (keyMod == SPECIAL_DONE)
    {
        return false;
    }
    if (prettyGraphics)
    {
        // Background
        drawWsg(bgImage, 0, 0, false, false, 0);
        // Draw the currently typed string
        _drawStrPretty(elapsedUs);
    }
    else
    {
        // Background
        fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);
        // Draw the currently typed string
        _drawStrSimple(elapsedUs);
    }
    // Draw an indicator for the current key modifier
    _drawTypeMode(textColor, prettyGraphics);
    // Draw the keyboard
    _drawKeyboard(prettyGraphics);
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
                    if (SHIFT == keyMod)
                    {
                        keyMod = NO_SHIFT;
                    }
                    else
                    {
                        keyMod = SHIFT;
                    }
                    break;
                }
                case KEY_CAPSLOCK:
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
            selx--;
            break;
        }
        case PB_DOWN:
        {
            // Move cursor
            sely++;
            break;
        }
        case PB_RIGHT:
        {
            // Move cursor
            selx++;
            break;
        }
        case PB_UP:
        {
            // Move cursor
            sely--;
            break;
        }
        case PB_SELECT:
        {
            // Rotate the keyMod from NO_SHIFT -> SHIFT -> CAPS LOCK, and back
            if (NO_SHIFT == keyMod)
            {
                keyMod = SHIFT;
            }
            else if (SHIFT == keyMod)
            {
                keyMod = CAPS_LOCK;
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
            selx = ENTER_X;
            sely = ENTER_Y;
        }
        default:
        {
            break;
        }
    }

    // Make sure the cursor is in bounds, wrap around if necessary
    if (sely < 0)
    {
        sely = KB_LINES - 1;
    }
    else if (sely >= KB_LINES)
    {
        sely = 0;
    }

    // Make sure the cursor is in bounds, wrap around if necessary
    if (selx < 0)
    {
        selx = lengthperline[sely] - 1;
    }
    else if (selx >= lengthperline[sely])
    {
        selx = 0;
    }

    // All done, still entering text
    return true;
}

// Setters

void textEntrySetTextColor(uint8_t col)
{
    textColor = col;
    if (empahsisColor == 0)
    {
        empahsisColor = col;
    }
}

void textEntrySetEmphasisColor(uint8_t col)
{
    empahsisColor = col;
}

void textEntrySetShadowboxColor(uint8_t col)
{
    textBoxColor = col;
}

void textEntrySetBG(wsg_t* BG)
{
    bgImage = BG;
}

// Drawing code

static void _drawStrPretty(int64_t eUs)
{
    // Draw the shadow box
    fillDisplayArea(MARGIN, STR_H_START - 8, TFT_WIDTH - MARGIN, STR_H_START + 22, textBoxColor);

    // Draw the typed text
    int16_t textLen = textWidth(activeFont, texString) + activeFont->chars[0].width;
    int16_t endPos  = drawText(activeFont, textColor, texString, (TFT_WIDTH - textLen) / 2, STR_H_START);

    _drawCursor(eUs, endPos);
}

static void _drawStrSimple(int64_t eUs)
{
    // Old, non-pretty keyboard routine
    int16_t textLen = textWidth(activeFont, texString) + activeFont->chars[0].width;
    int16_t endPos  = drawText(activeFont, textColor, texString, (TFT_WIDTH - textLen) / 2, STR_H_START);

    _drawCursor(eUs, endPos);
}

static void _drawCursor(int64_t eUs, int16_t end)
{
    cursorTimer += eUs;
    if (BLINK_RATE < cursorTimer)
    {
        cursorToggle = !cursorToggle;
        cursorTimer  = 0;
    }
    if (cursorToggle)
    {
        drawLineFast(end + 1, STR_H_START - 2, end + 1, STR_H_START + activeFont->height + 1, empahsisColor);
    }
}

static void _drawKeyboard(bool pretty)
{
    if (pretty)
    {
        // FIXME: Adjust width based on font size? Make a define?
        const uint8_t keyboardShadow = 136;
        fillDisplayArea(MARGIN, keyboardShadow, TFT_WIDTH - MARGIN, keyboardShadow + 80, textBoxColor);
    }
    int col = 0;
    int row = 0;
    char c;
    const char* s = (keyMod == NO_SHIFT) ? keyboard_lower : keyboard_upper;
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
            int posx  = col * 14 + 44 + row * 4;
            int posy  = row * 14 + 144;
            int width = 9;
            // Draw the character, may be a control char
            switch (c)
            {
                case KEY_CAPSLOCK:
                    if (keyMod == CAPS_LOCK)
                    {
                        _drawCaps(posx, posy, empahsisColor);
                    }
                    else
                    {
                        _drawCaps(posx, posy, textColor);
                    }
                    break;
                case KEY_SHIFT:
                    _drawShift(posx, posy, textColor);
                    break;
                case KEY_BACKSPACE:
                    _drawBackspace(posx, posy, textColor);
                    break;
                case KEY_SPACE:
                    _drawSpacebar(posx, posy, textColor);
                    width = 163;
                    break;
                case KEY_TAB:
                    _drawTab(posx, posy, textColor);
                    break;
                case KEY_ENTER:
                    width = _drawEnter(posx, posy, textColor);
                    break;
                default:
                    // Just draw the char
                    char sts[] = {c, 0};
                    drawText(activeFont, textColor, sts, posx, posy);
            }
            if (col == selx && row == sely)
            {
                // Draw Box around selected item.
                drawRect(posx - 2, posy - 2, posx + width, posy + 13, textColor);
                selChar = c;
            }
            col++;
        }
        s++;
    }
}

static void _drawCaps(int16_t x, int16_t y, uint8_t color)
{
    if (CAPS_NEW_STYLE)
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
        // Draw capslock extra arrow body thickness
        drawLineFast(x + 2, y + 4, x + 2, y + 9, color); // | (extra thickness left)
        drawLineFast(x + 4, y + 4, x + 4, y + 9, color); // | (extra thickness right)
        _drawShift(x, y, color);
    }
}

static void _drawShift(int16_t x, int16_t y, uint8_t color)
{
    // Draw shift/capslock
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

static void _drawSpacebar(int16_t x, int16_t y, uint8_t color)
{
    // Draw spacebar
    drawRect(x + 1, y + 1, x + 160, y + 3, color);
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
    if (!ENTER_STYLE)
    {
        drawText(activeFont, textColor, "OK", x, y);
        return textWidth(activeFont, "OK") + 2;
    }
    else
    {
        // Draw backspace
        drawLineFast(x + 0, y + 5, x + 12, y + 5, color);  // -
        drawLineFast(x + 1, y + 4, x + 3, y + 2, color);   // /
        drawLineFast(x + 2, y + 4, x + 4, y + 2, color);   // / (extra thickness)
        drawLineFast(x + 1, y + 6, x + 3, y + 8, color);   /* \ */
        drawLineFast(x + 2, y + 6, x + 4, y + 8, color);   // \ (extra thickness)
        drawLineFast(x + 12, y + 0, x + 12, y + 5, color); // |
        return 16;
    }
}

static void _drawTypeMode(uint8_t color, bool pretty)
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
        default:
            break;
    }
    int16_t width       = textWidth(activeFont, text);
    int16_t typingWidth = textWidth(activeFont, "Typing: ");
    if (pretty)
    {
        fillDisplayArea(TFT_WIDTH / 2 - 72, TFT_HEIGHT - 16, TFT_WIDTH / 2 + 72, TFT_HEIGHT, textBoxColor);
    }
    drawText(activeFont, color, text, (TFT_WIDTH - width) / 2, TFT_HEIGHT - activeFont->height - 4);
    if (useLine)
    {
        drawLineFast((TFT_WIDTH - width) / 2 + typingWidth, TFT_HEIGHT - 1, (TFT_WIDTH - width) / 2 + width,
                     TFT_HEIGHT - 1, color);
    }
}