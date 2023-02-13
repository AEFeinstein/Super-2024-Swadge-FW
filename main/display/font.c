//==============================================================================
// Includes
//==============================================================================

#include <string.h>

#include "hdw-tft.h"
#include "font.h"

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Draw a single character from a font to a display
 *
 * @param color The color of the character to draw
 * @param h     The height of the character to draw
 * @param ch    The character bitmap to draw (includes the width of the char)
 * @param xOff  The x offset to draw the char at
 * @param yOff  The y offset to draw the char at
 */
void drawChar(paletteColor_t color, int h, const font_ch_t* ch, int16_t xOff, int16_t yOff)
{
    //  This function has been micro optimized by cnlohr on 2022-09-07, using gcc version 8.4.0 (crosstool-NG
    //  esp-2021r2-patch3)
    int bitIdx            = 0;
    const uint8_t* bitmap = ch->bitmap;
    int wch               = ch->w;

    // Get a pointer to the end of the bitmap
    const uint8_t* endOfBitmap = &bitmap[((wch * h) + 7) >> 3] - 1;

    // Don't draw off the bottom of the screen.
    if (yOff + h > TFT_HEIGHT)
    {
        h = TFT_HEIGHT - yOff;
    }

    // Check Y bounds
    if (yOff < 0)
    {
        // Above the display, do wacky math with -yOff
        bitIdx -= yOff * wch;
        bitmap += bitIdx >> 3;
        bitIdx &= 7;
        h += yOff;
        yOff = 0;
    }

    paletteColor_t* pxOutput = getPxTftFramebuffer() + (yOff * TFT_WIDTH);

    for (int y = 0; y < h; y++)
    {
        // Figure out where to draw
        int truncate = 0;

        int startX = xOff;
        if (xOff < 0)
        {
            // Track how many groups of pixels we are skipping over
            // that weren't displayed on the left of the screen.
            startX = 0;
            bitIdx += -xOff;
            bitmap += bitIdx >> 3;
            bitIdx &= 7;
        }
        int endX = xOff + wch;
        if (endX > TFT_WIDTH)
        {
            // Track how many groups of pixels we are skipping over,
            // if the letter falls off the end of the screen.
            truncate = endX - TFT_WIDTH;
            endX     = TFT_WIDTH;
        }

        uint8_t thisByte = *bitmap;
        for (int drawX = startX; drawX < endX; drawX++)
        {
            // Figure out where to draw
            // Check X bounds
            if (thisByte & (1 << bitIdx))
            {
                // Draw the pixel
                pxOutput[drawX] = color;
            }

            // Iterate over the bit data
            if (8 == ++bitIdx)
            {
                bitIdx = 0;
                // Make sure not to read past the bitmap
                if (bitmap < endOfBitmap)
                {
                    thisByte = *(++bitmap);
                }
                else
                {
                    // No more bitmap, so return
                    return;
                }
            }
        }

        // Handle any remaining bits if we have ended off the end of the display.
        bitIdx += truncate;
        bitmap += bitIdx >> 3;
        bitIdx &= 7;
        pxOutput += TFT_WIDTH;
    }
}

/**
 * @brief Draw text to a display with the given color and font
 *
 * @param font  The font to use for the text
 * @param color The color of the character to draw
 * @param text  The text to draw to the display
 * @param xOff  The x offset to draw the text at
 * @param yOff  The y offset to draw the text at
 * @return The x offset at the end of the drawn string
 */
int16_t drawText(const font_t* font, paletteColor_t color, const char* text, int16_t xOff, int16_t yOff)
{
    while (*text >= ' ')
    {
        // Only draw if the char is on the screen
        if (xOff + font->chars[(*text) - ' '].w >= 0)
        {
            // Draw char
            drawChar(color, font->h, &font->chars[(*text) - ' '], xOff, yOff);
        }

        // Move to the next char
        xOff += (font->chars[(*text) - ' '].w + 1);
        text++;

        // If this char is offscreen, finish drawing
        if (xOff >= TFT_WIDTH)
        {
            return xOff;
        }
    }
    return xOff;
}

/**
 * @brief Return the pixel width of some text in a given font
 *
 * @param font The font to use
 * @param text The text to measure
 * @return The width of the text rendered in the font
 */
uint16_t textWidth(const font_t* font, const char* text)
{
    uint16_t width = 0;
    while (*text != 0)
    {
        if ((*text) >= ' ')
        {
            width += (font->chars[(*text) - ' '].w + 1);
        }
        text++;
    }
    // Delete trailing space
    if (0 < width)
    {
        width--;
    }
    return width;
}

/**
 * @brief TODO
 *
 * @param font
 * @param color
 * @param text
 * @param xOff
 * @param yOff
 * @param xMax
 * @param yMax
 * @return const char*
 */
static const char* drawTextWordWrapInner(const font_t* font, paletteColor_t color, const char* text, int16_t* xOff,
                                         int16_t* yOff, int16_t xMax, int16_t yMax)
{
    const char* textPtr = text;
    int16_t textX = *xOff, textY = *yOff;
    int nextSpace, nextDash, nextNl;
    int nextBreak;
    char buf[64];

    // don't dereference that null pointer
    if (text == NULL)
    {
        return NULL;
    }

    // while there is text left to print, and the text would not exceed the Y-bounds...
    while (*textPtr && (textY + font->h <= yMax))
    {
        *yOff = textY;

        // skip leading spaces if we're at the start of the line
        for (; textX == *xOff && *textPtr == ' '; textPtr++)
        {
            ;
        }

        // handle newlines
        if (*textPtr == '\n')
        {
            textX = *xOff;
            textY += font->h + 1;
            textPtr++;
            continue;
        }

        // if strchr() returns NULL, this will be negative...
        // otherwise, nextSpace will be the index of the next space of textPtr
        nextSpace = strchr(textPtr, ' ') - textPtr;
        nextDash  = strchr(textPtr, '-') - textPtr;
        nextNl    = strchr(textPtr, '\n') - textPtr;

        // copy as much text as will fit into the buffer
        // leaving room for a null-terminator in case the string is longer
        strncpy(buf, textPtr, sizeof(buf) - 1);

        // ensure there is always a null-terminator even if
        buf[sizeof(buf) - 1] = '\0';

        // worst case, there are no breaks remaining
        // I think this strlen call is necessary?
        nextBreak = strlen(buf);

        if (nextSpace >= 0 && nextSpace < nextBreak)
        {
            nextBreak = nextSpace + 1;
        }

        if (nextDash >= 0 && nextDash < nextBreak)
        {
            nextBreak = nextDash + 1;
        }

        if (nextNl >= 0 && nextNl < nextBreak)
        {
            nextBreak = nextNl;
        }

        // end the string at the break
        buf[nextBreak] = '\0';

        // The text is longer than an entire line, so we must shorten it
        if (*xOff + textWidth(font, buf) > xMax)
        {
            // shorten the text until it fits
            while (textX + textWidth(font, buf) > xMax && nextBreak > 0)
            {
                buf[--nextBreak] = '\0';
            }
        }

        // The text is longer than will fit on the rest of the current line
        // Or we shortened it down to nothing. Either way, move to next line.
        // Also, go back to the start of the loop so we don't
        // accidentally overrun the yMax
        if (textX + textWidth(font, buf) > xMax || nextBreak == 0)
        {
            // The line won't fit
            textY += font->h + 1;
            textX = *xOff;
            continue;
        }

        // the line must have enough space for the rest of the buffer
        // print the line, and advance the text pointer and offset
        if (textY + font->h >= 0 && textY <= TFT_HEIGHT)
        {
            textX = drawText(font, color, buf, textX, textY);
        }
        else
        {
            // drawText returns the next text position, which is 1px past the last char
            // textWidth returns, well, the text width, so add 1 to account for the last pixel
            textX += textWidth(font, buf) + 1;
        }
        textPtr += nextBreak;
    }

    // Return NULL if we've printed everything
    // Otherwise, return the remaining text
    *xOff = textX;
    return *textPtr ? textPtr : NULL;
}

/**
 * @brief Draws text, breaking on word boundaries, until the given bounds are filled or all text is drawn.
 *
 * Text will be drawn, starting at `(xOff, yOff)`, wrapping to the next line at ' ' or '-' when the next
 * word would exceed `xMax`, or immediately when a newline ('\\n') is encountered. Carriage returns and
 * tabs ('\\r', '\\t') are not supported. When the bottom of the next character would exceed `yMax`, no more
 * text is drawn and a pointer to the next undrawn character within `text` is returned. If all text has
 * been written, NULL is returned.
 *
 * @param font The font to use when drawing the text
 * @param color The color of the text to be drawn
 * @param text The text to be pointed, as a null-terminated string
 * @param xOff The X-coordinate to begin drawing the text at
 * @param yOff The Y-coordinate to begin drawing the text at
 * @param xMax The maximum x-coordinate at which any text may be drawn
 * @param yMax The maximum y-coordinate at which text may be drawn
 * @return A pointer to the first unprinted character within `text`, or NULL if all text has been written
 */
const char* drawTextWordWrap(const font_t* font, paletteColor_t color, const char* text, int16_t* xOff, int16_t* yOff,
                             int16_t xMax, int16_t yMax)
{
    return drawTextWordWrapInner(font, color, text, xOff, yOff, xMax, yMax);
}

/**
 * @brief TODO document
 *
 * @param font
 * @param text
 * @param width
 * @param maxHeight
 * @return uint16_t
 */
uint16_t textHeight(const font_t* font, const char* text, int16_t width, int16_t maxHeight)
{
    int16_t xEnd = 0;
    int16_t yEnd = 0;
    drawTextWordWrapInner(font, cTransparent, text, &xEnd, &yEnd, width, maxHeight);
    return yEnd + font->h + 1;
}
