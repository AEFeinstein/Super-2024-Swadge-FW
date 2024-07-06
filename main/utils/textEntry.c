#include "textEntry.h"

/*============================================================================
 * Variables
 *==========================================================================*/

static font_t* textEntryIBM;
static int texLen;
static char* texString;
static keyModifier_t keyMod;
static int8_t selx;
static int8_t sely;
static char selChar;
static uint8_t cursorTimer;


// Graphical
static bool pretty;
static uint8_t textColor;
static uint8_t textBoxColor;

static wsg_t bgImage;

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
    texLen       = max_len;
    texString    = buffer;
    selx         = 1;
    sely         = 1;
    keyMod       = NO_SHIFT;
    texString[0] = 0;
    cursorTimer  = 0;
    textEntryIBM = usefont;
    textColor    = WHITE;
    pretty       = false;
}

void textEntryStartPretty(font_t* usefont, int max_len, char* buffer, wsg_t BG, uint8_t tbColor, uint8_t txtColor)
{
    texLen       = max_len;
    texString    = buffer;
    selx         = 1;
    sely         = 1;
    keyMod       = NO_SHIFT;
    texString[0] = 0;
    cursorTimer  = 0;
    textEntryIBM = usefont;
    textColor    = txtColor;
    pretty       = true;
    bgImage      = BG;
    textBoxColor = tbColor;
}

bool textEntryDraw(int64_t elapsedUs)
{
    // If we're done, return false
    if (keyMod == SPECIAL_DONE)
    {
        return false;
    }

    const uint8_t text_h = 64;
    
    if (pretty)
    {
        _drawStrPretty(text_h, elapsedUs);
    }
    else
    {
        _drawStrSimple(text_h);
    }

    // Draw an indicator for the current key modifier
    switch (keyMod)
    {
        case SHIFT:
        {
            int16_t width       = textWidth(textEntryIBM, "Typing: Upper");
            int16_t typingWidth = textWidth(textEntryIBM, "Typing: ");
            drawText(textEntryIBM, textColor, "Typing: Upper", (TFT_WIDTH - width) / 2,
                     TFT_HEIGHT - textEntryIBM->height - 2);
            drawLineFast((TFT_WIDTH - width) / 2 + typingWidth, TFT_HEIGHT - 1, (TFT_WIDTH - width) / 2 + width,
                         TFT_HEIGHT - 1, textColor);
            break;
        }
        case NO_SHIFT:
        {
            int16_t width = textWidth(textEntryIBM, "Typing: Lower");
            drawText(textEntryIBM, textColor, "Typing: Lower", (TFT_WIDTH - width) / 2,
                     TFT_HEIGHT - textEntryIBM->height - 2);
            break;
        }
        case CAPS_LOCK:
        {
            int16_t width       = textWidth(textEntryIBM, "Typing: CAPS LOCK");
            int16_t typingWidth = textWidth(textEntryIBM, "Typing: ");
            drawText(textEntryIBM, textColor, "Typing: CAPS LOCK", (TFT_WIDTH - width) / 2,
                     TFT_HEIGHT - textEntryIBM->height - 2);
            drawLineFast((TFT_WIDTH - width) / 2 + typingWidth, TFT_HEIGHT - 1, (TFT_WIDTH - width) / 2 + width,
                         TFT_HEIGHT - 1, textColor);
            break;
        }
        default:
        case SPECIAL_DONE:
        {
            break;
        }
    }

    // Draw the keyboard
    _drawKeyboard();
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

// Drawing code

static void _drawStrPretty(int8_t text_h, int64_t elaspedUs)
{
    const uint8_t margin         = 32;
    const uint8_t keyboardShadow = 136;

    // Draw the BG Image
    drawWsg(&bgImage, 0, 0, false, false, 0);

    // Draw the shadow boxes
    fillDisplayArea(margin, text_h - 8, TFT_WIDTH - margin, text_h + 22, textBoxColor);
    fillDisplayArea(margin, keyboardShadow, TFT_WIDTH - margin, keyboardShadow + 80, textBoxColor);
    fillDisplayArea(TFT_WIDTH / 2 - 72, TFT_HEIGHT - 16, TFT_WIDTH / 2 + 72, TFT_HEIGHT, textBoxColor);

    /* // Draw the typed text
    int16_t textLen = textWidth(textEntryIBM, texString) + textEntryIBM->chars[0].width;
    int16_t endPos  = drawTextWordWrap(textEntryIBM, textColor, texString, (TFT_WIDTH - textLen) / 2, text_h);

    // If the blinky cursor should be shown, draw it
    if ((cursorTimer++) & 0x10)
    {
        drawLineFast(endPos + 1, text_h - 2, endPos + 1, text_h + textEntryIBM->height + 1, textColor);
    } */
}

static void _drawStrSimple(uint8_t text_h)
{
    // Old, non-pretty keyboard routine
    int16_t textLen = textWidth(textEntryIBM, texString) + textEntryIBM->chars[0].width;
    int16_t endPos  = drawText(textEntryIBM, textColor, texString, (TFT_WIDTH - textLen) / 2, text_h);

    // If the blinky cursor should be shown, draw it
    // FIXME: Not changed because compatibility, but *should* rely on elaspsedUs
    if ((cursorTimer++) & 0x10)
    {
        drawLineFast(endPos + 1, text_h - 2, endPos + 1, text_h + textEntryIBM->height + 1, textColor);
    }
}

static void _drawKeyboard()
{
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
                    _drawCaps(posx, posy, textColor);
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
                    _drawEnter(posx, posy, textColor);
                    width = textWidth(textEntryIBM, "OK") + 2;
                    break;
                default:
                    // Just draw the char
                    char sts[] = {c, 0};
                    drawText(textEntryIBM, textColor, sts, posx, posy);
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

static void _drawEnter(int16_t x, int16_t y, uint8_t color)
{
    drawText(textEntryIBM, textColor, "OK", x, y);
}
