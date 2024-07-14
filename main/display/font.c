//==============================================================================
// Includes
//==============================================================================

#include <string.h>
#include <esp_heap_caps.h>

#include "macros.h"
#include "hdw-tft.h"
#include "font.h"

//==============================================================================
// Enums
//==============================================================================

/// @brief Flag for drawTextWordWrapFlags() to draw the text normally
#define TEXT_DRAW 0

/// @brief Flag for drawTexTWordWrapFlags() to measure the text without drawing
#define TEXT_MEASURE 1

//==============================================================================
// Static Function Declarations
//==============================================================================

static const char* drawTextWordWrapFlags(const font_t* font, paletteColor_t color, const char* text, int16_t* xOff,
                                         int16_t* yOff, int16_t xMax, int16_t yMax, uint16_t flags);

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
 * @param xMin  The left edge of the text bounds
 * @param xMax  The
 */
void drawCharBounds(paletteColor_t color, int h, const font_ch_t* ch, int16_t xOff, int16_t yOff, int16_t xMin,
                    int16_t yMin, int16_t xMax, int16_t yMax)
{
    // Do not draw transparent chars
    if (cTransparent == color)
    {
        return;
    }

    //  This function has been micro optimized by cnlohr on 2022-09-07, using gcc version 8.4.0 (crosstool-NG
    //  esp-2021r2-patch3)
    int bitIdx            = 0;
    const uint8_t* bitmap = ch->bitmap;
    int wch               = ch->width;

    // Get a pointer to the end of the bitmap
    const uint8_t* endOfBitmap = &bitmap[((wch * h) + 7) >> 3] - 1;

    // Don't draw off the bottom of the screen.
    if (yOff + h > yMax)
    {
        h = yMax - yOff;
    }

    // Check Y bounds
    if (yOff < yMin)
    {
        // This line not micro-optimized, but hopefully it's ok
        bitIdx += yMin * wch;
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
        if (xOff < xMin)
        {
            // Track how many groups of pixels we are skipping over
            // that weren't displayed on the left of the screen.
            startX = xMin;
            bitIdx += xMin;
            bitIdx += -xOff;
            bitmap += bitIdx >> 3;
            bitIdx &= 7;
        }
        int endX = xOff + wch;
        if (endX > xMax)
        {
            // Track how many groups of pixels we are skipping over,
            // if the letter falls off the end of the screen.
            truncate = endX - xMax;
            endX     = xMax;
        }

        if (bitmap > endOfBitmap)
        {
            return;
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
    drawCharBounds(color, h, ch, xOff, yOff, 0, 0, TFT_WIDTH, TFT_HEIGHT);
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
int16_t drawTextBounds(const font_t* font, paletteColor_t color, const char* text, int16_t xOff, int16_t yOff,
                       int16_t xMin, int16_t yMin, int16_t xMax, int16_t yMax)
{
    while (*text >= ' ')
    {
        // Only draw if the char is on the screen
        if ((xOff + font->chars[(*text) - ' '].width >= xMin) && (xOff < xMax))
        {
            // Draw char
            drawCharBounds(color, font->height, &font->chars[(*text) - ' '], xOff, yOff, xMin, yMin, xMax, yMax);
        }

        // Move to the next char
        xOff += (font->chars[(*text) - ' '].width + 1);
        text++;

        // If this char is offscreen, finish drawing
        if (xOff >= xMax)
        {
            return xOff;
        }
    }
    return xOff;
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
    return drawTextBounds(font, color, text, xOff, yOff, 0, 0, TFT_WIDTH, TFT_HEIGHT);
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
            width += (font->chars[(*text) - ' '].width + 1);
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

static const char* drawTextWordWrapFlags(const font_t* font, paletteColor_t color, const char* text, int16_t* xOff,
                                         int16_t* yOff, int16_t xMax, int16_t yMax, uint16_t flags)
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
    while (*textPtr && (textY + font->height <= yMax))
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
            textY += font->height + 1;
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
            textY += font->height + 1;
            textX = *xOff;
            continue;
        }

        // the line must have enough space for the rest of the buffer
        // print the line, and advance the text pointer and offset
        if (!(flags & TEXT_MEASURE) && textY + font->height >= 0 && textY <= TFT_HEIGHT)
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
    return drawTextWordWrapFlags(font, color, text, xOff, yOff, xMax, yMax, TEXT_DRAW);
}

/**
 * @brief Return the height of a block of word wrapped text
 *
 * @param font The font to use when drawing the text
 * @param text The text to be pointed, as a null-terminated string
 * @param width The maximum x-coordinate at which any text may be drawn
 * @param maxHeight The maximum y-coordinate at which text may be drawn
 * @return The height of the word wrapped text block
 */
uint16_t textWordWrapHeight(const font_t* font, const char* text, int16_t width, int16_t maxHeight)
{
    int16_t xEnd = 0;
    int16_t yEnd = 0;
    drawTextWordWrapFlags(font, cTransparent, text, &xEnd, &yEnd, width, maxHeight, TEXT_MEASURE);
    return yEnd + font->height + 1;
}

/**
 * @brief Get a single pixel from a font character
 *
 * @param ch The character to get a pixel from (includes width)
 * @param height The height of the character
 * @param x The X coordinate of the pixel
 * @param y The Y coordinate of the pixel
 * @return true if the pixel is set, false if it is not
 */
static bool getFontPx(font_ch_t* ch, int16_t height, int16_t x, int16_t y)
{
    // Bounds checks
    if (x < 0 || x >= ch->width || y < 0 || y >= height)
    {
        return false;
    }

    int16_t pxIdx   = (y * ch->width) + x;
    int16_t byteIdx = pxIdx / 8;
    int16_t bitIdx  = pxIdx % 8;
    return (ch->bitmap[byteIdx] & (1 << bitIdx)) ? true : false;
}

/**
 * @brief Set a single pixel from a font character
 *
 * @param ch The character to set a pixel in (includes width)
 * @param x The X coordinate of the pixel
 * @param y The Y coordinate of the pixel
 * @param isSet true to set the pixel, false to clear it
 */
static void setFontPx(font_ch_t* ch, int16_t x, int16_t y, bool isSet)
{
    int16_t pxIdx   = (y * ch->width) + x;
    int16_t byteIdx = pxIdx / 8;
    int16_t bitIdx  = pxIdx % 8;
    if (isSet)
    {
        ch->bitmap[byteIdx] |= (1 << bitIdx);
    }
    else
    {
        ch->bitmap[byteIdx] &= ~(1 << bitIdx);
    }
}

/**
 * @brief Create the outline of a font as a separate font
 *
 * @param srcFont The source font to make an outline of
 * @param dstFont The destination font that will be initialized as an outline of the source font
 * @param spiRam true to allocate memory in SPI RAM, false to allocate memory in normal RAM
 */
void makeOutlineFont(font_t* srcFont, font_t* dstFont, bool spiRam)
{
    // Set up calloc flags
    uint32_t callocFlags = MALLOC_CAP_DEFAULT;
    if (spiRam)
    {
        callocFlags = MALLOC_CAP_SPIRAM;
    }

    // Copy the height
    dstFont->height = srcFont->height;

    // For each character
    for (int16_t cIdx = 0; cIdx < ARRAY_SIZE(dstFont->chars); cIdx++)
    {
        // Make easy reference pointers
        font_ch_t* oCh = &dstFont->chars[cIdx];
        font_ch_t* sCh = &srcFont->chars[cIdx];

        // Copy the character width
        oCh->width = sCh->width;

        // Allocate space for the outline bitmap
        int pixels  = dstFont->height * oCh->width;
        int bytes   = (pixels / 8) + ((pixels % 8 == 0) ? 0 : 1);
        oCh->bitmap = heap_caps_calloc(bytes, sizeof(uint8_t), callocFlags);

        for (int16_t y = 0; y < dstFont->height; y++)
        {
            for (int16_t x = 0; x < oCh->width; x++)
            {
                bool onBoundary = false;
                // If the source pixel is set
                if (getFontPx(sCh, srcFont->height, x, y))
                {
                    // Check for boundaries
                    if (!getFontPx(sCh, srcFont->height, x - 1, y + 0) //
                        || !getFontPx(sCh, srcFont->height, x + 1, y + 0)
                        || !getFontPx(sCh, srcFont->height, x + 0, y - 1)
                        || !getFontPx(sCh, srcFont->height, x + 0, y + 1))
                    {
                        onBoundary = true;
                    }
                }

                // Set the outline pixel accordingly
                setFontPx(oCh, x, y, onBoundary);
            }
        }
    }
}

/**
 * @brief Draw text to the display with a marquee effect
 *
 * @param font  The font to use for the text
 * @param color The color of the character to draw
 * @param text  The text to draw to the display
 * @param xOff  The x offset to draw the text at
 * @param yOff  The y offset to draw the text at
 * @param xMax  The x offset of the right edge of the marquee text
 * @param[in,out] timer A pointer to a timer value used to time the marquee
 * @return The x offset at the end of the drawn string
 */
int16_t drawTextMarquee(const font_t* font, paletteColor_t color, const char* text, int16_t xOff, int16_t yOff,
                        int16_t xMax, int64_t* timer)
{
// Marquee speed in microseconds per pixel
#define MARQUEE_SPEED 40000
    int16_t gapW        = 4 * textWidth(font, " ");
    int64_t repeatDelay = gapW * MARQUEE_SPEED;
    uint16_t textW      = textWidth(font, text);
    int64_t loopPeriod  = (textW + 1 /*- (xMax - xOff)*/) * MARQUEE_SPEED + repeatDelay;

    if (*timer > loopPeriod)
    {
        *timer %= loopPeriod;
    }

    int16_t offset   = *timer / MARQUEE_SPEED;
    int16_t endX     = drawTextBounds(font, color, text, xOff - offset, yOff, xOff, 0, xMax, TFT_HEIGHT);
    int16_t endStart = endX + gapW;

    if (endStart < xMax)
    {
        return drawTextBounds(font, color, text, endStart, yOff, xOff, 0, xMax, TFT_HEIGHT);
    }

    return endX;
}

/**
 * @brief Draw text to the display that fits within a given length, replacing any overflowing text with "..."
 *
 * @param font  The font to use for the text
 * @param color The color of the character to draw
 * @param text  The text to draw to the display
 * @param xOff  The x offset to draw the text at
 * @param yOff  The y offset to draw the text at
 * @param maxW  The maximum width of text to draw
 * @return true
 * @return false
 */
bool drawTextEllipsize(const font_t* font, paletteColor_t color, const char* text, int16_t xOff, int16_t yOff,
                       int16_t maxW)
{
    uint16_t textW = textWidth(font, text);
    if (textW <= maxW)
    {
        drawText(font, color, text, xOff, yOff);
        return false;
    }
    else
    {
        uint16_t ellipsisW = textWidth(font, "...");
        uint16_t trimW     = textW;

        // Reduce the text width by one character-width until it all fits
        if (strlen(text))
        {
            const char* cur = text + strlen(text) - 1;
            while (cur >= text && trimW + ellipsisW > maxW)
            {
                trimW -= font->chars[*(cur--) - ' '].width + 1;
            }
        }

        drawTextBounds(font, color, text, xOff, yOff, 0, 0, xOff + trimW, TFT_HEIGHT);
        drawText(font, color, "...", xOff + trimW + 1, yOff);

        return true;
    }
}
