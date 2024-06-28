/*============================================================================
 * Includes
 *==========================================================================*/

#include "textEntry.h"
#include "hdw-btn.h"
#include "shapes.h"
#include "hdw-tft.h"
#include <string.h>

/*============================================================================
 * Enums
 *==========================================================================*/

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
static wsg_t bgImage;
static uint8_t textBoxColor;
static bool pretty;
static uint8_t textColor;

#define WHITE 215

#define KB_LINES 5
#define ENTER_X  12
#define ENTER_Y  2

// 0 = Thicker Shift, 1 = Thick arrow with box below
#define CAPS_NEW_STYLE 1

// See controlChar_t
static const char keyboard_upper[] = "\
~!@#$%^&*()_+\x03\x05\
\x09QWERTYUIOP{}|\x05\
\002ASDFGHJKL:\"\x0a\x05\
\x01ZXCVBNM<>?\x01\x05\
\x20";

// See controlChar_t
static const char keyboard_lower[] = "\
`1234567890-=\x03\x05\
\x09qwertyuiop[]\\\x05\
\002asdfghjkl;\'\x0a\x05\
\x01zxcvbnm,./\x01\x05\
\x20";

static const uint8_t lengthperline[] = {14, 14, 13, 12, 1};

/*============================================================================
 * Functions
 *==========================================================================*/

/**
 * Initialize the text entry
 *
 * @param usefont The font to use, should be ibm_vga8
 * @param max_len The length of buffer
 * @param buffer  A char* to store the entered text in
 */
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
    textColor = WHITE;
    pretty = false;
}

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

/**
 * Finish the text entry by disarming the cursor blink timer
 */
void textEntryEnd(void)
{
}

/**
 * Draw the text entry UI
 *
 * @return true if text entry is still being used
 *         false if text entry is finished
 */
bool textEntryDraw(void)
{
    // If we're done, return false
    if (keyMod == SPECIAL_DONE)
    {
        return false;
    }

    const uint8_t text_h = 64;
    const uint8_t margin = 32;
    const uint8_t keyboardShadow = 136;
    
    if (pretty){
        drawWsg(&bgImage, 0, 0, false, false, 0);
        fillDisplayArea(margin, text_h - 8, TFT_WIDTH - margin, text_h + 22, textBoxColor);
        fillDisplayArea(margin, keyboardShadow, TFT_WIDTH - margin, keyboardShadow + 80, textBoxColor);
        fillDisplayArea(TFT_WIDTH/2 - 72, TFT_HEIGHT - 16, TFT_WIDTH/2 + 72, TFT_HEIGHT, textBoxColor);
    }

    // Draw the text entered so far
    {
        int16_t textLen      = textWidth(textEntryIBM, texString) + textEntryIBM->chars[0].width;
        int16_t endPos       = drawText(textEntryIBM, textColor, texString, (TFT_WIDTH - textLen) / 2, text_h);

        // If the blinky cursor should be shown, draw it
        if ((cursorTimer++) & 0x10)
        {
            drawLineFast(endPos + 1, text_h - 2, endPos + 1, text_h + textEntryIBM->height + 1, textColor);
        }
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
    int x = 0;
    int y = 0;
    char c;
    const char* s = (keyMod == NO_SHIFT) ? keyboard_lower : keyboard_upper;
    while ((c = *s))
    {
        // EOL character hit, move to the next row
        if (c == KEY_EOL)
        {
            x = 0;
            y++;
        }
        else
        {
            int posx  = x * 14 + 44 + y * 4;
            int posy  = y * 14 + 144;
            int width = 9;
            // Draw the character, may be a control char
            switch (c)
            {
                case KEY_CAPSLOCK:
                {
#if CAPS_NEW_STYLE
                    drawRect(posx + 2, posy + 8, posx + 5, posy + 10, textColor);    // box
                    drawLineFast(posx + 3, posy + 0, posx + 3, posy + 6, textColor); // |
                    drawLineFast(posx + 2, posy + 2, posx + 2, posy + 6, textColor); // | (extra thickness left)
                    drawLineFast(posx + 4, posy + 2, posx + 4, posy + 6, textColor); // | (extra thickness right)
                    drawLineFast(posx + 0, posy + 3, posx + 2, posy + 1, textColor); // /
                    drawLineFast(posx + 0, posy + 4, posx + 2, posy + 2, textColor); // / (extra thickness)
                    drawLineFast(posx + 4, posy + 1, posx + 6, posy + 3, textColor); /* \ */
                    drawLineFast(posx + 4, posy + 2, posx + 6, posy + 4, textColor); // \ (extra thickness)
                    break;
#else
                    // Draw capslock extra arrow body thickness
                    drawLineFast(posx + 2, posy + 4, posx + 2, posy + 9, textColor); // | (extra thickness left)
                    drawLineFast(posx + 4, posy + 4, posx + 4, posy + 9, textColor); // | (extra thickness right)
                                                                                 // Intentional fallthrough
#endif
                }
                case KEY_SHIFT:
                {
                    // Draw shift/capslock
                    drawLineFast(posx + 1, posy + 9, posx + 5, posy + 9, textColor); // -
                    drawLineFast(posx + 3, posy + 2, posx + 3, posy + 9, textColor); // |
                    drawLineFast(posx + 0, posy + 5, posx + 2, posy + 3, textColor); // /
                    drawLineFast(posx + 0, posy + 6, posx + 2, posy + 4, textColor); // / (extra thickness)
                    drawLineFast(posx + 4, posy + 3, posx + 6, posy + 5, textColor); /* \ */
                    drawLineFast(posx + 4, posy + 4, posx + 6, posy + 6, textColor); // \ (extra thickness)
                    break;
                }
                case KEY_BACKSPACE:
                {
                    // Draw backspace
                    drawLineFast(posx + 0, posy + 5, posx + 6, posy + 5, textColor); // -
                    drawLineFast(posx + 1, posy + 4, posx + 3, posy + 2, textColor); // /
                    drawLineFast(posx + 2, posy + 4, posx + 4, posy + 2, textColor); // / (extra thickness)
                    drawLineFast(posx + 1, posy + 6, posx + 3, posy + 8, textColor); /* \ */
                    drawLineFast(posx + 2, posy + 6, posx + 4, posy + 8, textColor); // \ (extra thickness)
                    break;
                }
                case KEY_SPACE:
                {
                    // Draw spacebar
                    drawRect(posx + 1, posy + 1, posx + 160, posy + 3, textColor);
                    width = 163;
                    break;
                }
                case KEY_TAB:
                {
                    // Draw tab
                    drawLineFast(posx + 0, posy + 2, posx + 0, posy + 8, textColor); // |
                    drawLineFast(posx + 0, posy + 5, posx + 6, posy + 5, textColor); // -
                    drawLineFast(posx + 3, posy + 2, posx + 5, posy + 4, textColor); // \ (not a multiline comment)
                    drawLineFast(posx + 2, posy + 2, posx + 4, posy + 4, textColor); // \ (extra thickness)
                    drawLineFast(posx + 3, posy + 8, posx + 5, posy + 6, textColor); // /
                    drawLineFast(posx + 2, posy + 8, posx + 4, posy + 6, textColor); // / (extra thickness)
                    break;
                }
                case KEY_ENTER:
                {
                    // Draw an OK for enter

                    drawText(textEntryIBM, textColor, "OK", posx, posy);
                    width = textWidth(textEntryIBM, "OK") + 2;
                    break;
                }
                default:
                {
                    // Just draw the char
                    char sts[] = {c, 0};
                    drawText(textEntryIBM, textColor, sts, posx, posy);
                }
            }
            if (x == selx && y == sely)
            {
                // Draw Box around selected item.
                drawRect(posx - 2, posy - 2, posx + width, posy + 13, textColor);
                selChar = c;
            }
            x++;
        }
        s++;
    }
    return true;
}

/**
 * handle button input for text entry
 *
 * @param down   true if the button was pressed, false if it was released
 * @param button The button that was pressed
 * @return true if text entry is still ongoing
 *         false if the enter key was pressed and text entry is done
 */
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
