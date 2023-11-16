#include "paint_canvas.h"

#include "hdw-tft.h"
#include "paint_common.h"
#include "paint_util.h"

static size_t _paintSerialize(uint8_t* dest, const paintCanvas_t* canvas, const wsg_t* wsg, size_t offset, size_t count,
                              const paletteColor_t palette[16]);

/**
 * @brief Draw a buffered canvas to the screen, overwriting whatever was drawn previously
 *
 * Does nothing on a canvas with \c{buffered == false}
 *
 * @param canvas The buffered canvas to draw to the screen
 */
void paintBlitCanvas(const paintCanvas_t* canvas)
{
    if (!canvas->buffered || !canvas->buffer)
    {
        PAINT_LOGE("Attempting to blit from a non-buffered canvas! Doing nothing!");
        return;
    }

    // Super simple
    paintDeserialize(canvas, canvas->buffer, 0, paintGetStoredSize(canvas));
}

/**
 * @brief Copy a canvas from the screen into the buffer, overwriting whatever was stored previously
 *
 * Does nothing on a canvas with \c{buffered == false}
 *
 * @param canvas The buffered canvas to copy from the screen
 */
void paintSyncCanvas(paintCanvas_t* canvas)
{
    if (!canvas->buffered || !canvas->buffer)
    {
        PAINT_LOGE("Attempting to sync to a non-buffered canvas! Doing nothing!");
        return;
    }
    // Also super simple, come to think of it
    paintSerialize(canvas->buffer, canvas, 0, paintGetStoredSize(canvas));
}

/**
 * Returns the number of bytes needed to store the image pixel data
 */
size_t paintGetStoredSize(const paintCanvas_t* canvas)
{
    return (canvas->w * canvas->h + 1) / 2;
}

/**
 * @brief Returns the number of bytes needed to store an image of te given size
 *
 * @param w Width of the image
 * @param h Height of the image
 * @return size_t The size of the buffer needed to store the image
 */
size_t paintGetStoredSizeDim(uint16_t w, uint16_t h)
{
    return (w * h + 1) / 2;
}

/**
 * @brief Construct a palette from the given image data.
 *
 * If the image contains more than 16 colors, some color data will be lost
 *
 * @param[out] palette A pointer to the output palette
 * @param[in] img The image data to build the palette from
 * @param w The width of the image
 * @param h The height of the image
 */
void paintRebuildPalette(paletteColor_t palette[16], const paletteColor_t* img, uint16_t w, uint16_t h)
{
    uint8_t paletteIndex[cTransparent + 1] = {0};

    int max = w * h;
    for (int i = 0; i < max; i++)
    {
        paletteIndex[img[i]] = 1;
    }

    int pOut = 0;
    for (int i = 0; i < ARRAY_SIZE(paletteIndex); i++)
    {
        if (paletteIndex[i])
        {
            palette[pOut++] = i;

            if (pOut == PAINT_MAX_COLORS)
            {
                break;
            }
        }
    }

    // If, somehow, the image had no colors?
    if (pOut == 0)
    {
        palette[pOut++] = c000;
    }

    while (pOut < PAINT_MAX_COLORS)
    {
        // Fill in any unused colors if there were fewer than the max
        palette[pOut++] = palette[0];
    }
}

/**
 * @brief Draw a segment of a serialized paint image onto the screen
 *
 * @param dest The canvas defining the draw destination
 * @param data The buffer to be drawn
 * @param offset The offset of this data segment within the image
 * @param count The length of the segment
 * @return true If there is more image to be drawn
 * @return false If the image was completely drawn
 */
bool paintDeserialize(const paintCanvas_t* dest, const uint8_t* data, size_t offset, size_t count)
{
    uint16_t x0, y0, x1, y1;
    for (uint16_t n = 0; n < count; n++)
    {
        if (offset * 2 + (n * 2) >= dest->w * dest->h)
        {
            // If we've just read the last pixel, exit early
            return false;
        }

        // no need for logic to exit the final chunk early, since each chunk's size is given to us
        // calculate the canvas coordinates given the pixel indices
        x0 = (offset * 2 + (n * 2)) % dest->w;
        y0 = (offset * 2 + (n * 2)) / dest->w;
        x1 = (offset * 2 + (n * 2 + 1)) % dest->w;
        y1 = (offset * 2 + (n * 2 + 1)) / dest->w;

        setPxScaled(x0, y0, dest->palette[data[n] >> 4], dest->x, dest->y, dest->xScale, dest->yScale);
        setPxScaled(x1, y1, dest->palette[data[n] & 0xF], dest->x, dest->y, dest->xScale, dest->yScale);
    }

    return offset * 2 + (count * 2) < dest->w * dest->h;
}

size_t paintSerializeWsg(uint8_t* dest, const wsg_t* wsg)
{
    // WSG has no palette, build it
    paletteColor_t palette[16];
    paintRebuildPalette(palette, wsg->px, wsg->w, wsg->h);

    return _paintSerialize(dest, NULL, wsg, 0, (wsg->w * wsg->h + 1) / 2, palette);
}

size_t paintSerialize(uint8_t* dest, const paintCanvas_t* canvas, size_t offset, size_t count)
{
    return _paintSerialize(dest, canvas, NULL, offset, count, canvas->palette);
}

size_t paintSerializeWsgPalette(uint8_t* dest, const wsg_t* wsg, const paletteColor_t palette[16])
{
    return _paintSerialize(dest, NULL, wsg, 0, paintGetStoredSizeDim(wsg->w, wsg->h), palette);
}

static size_t _paintSerialize(uint8_t* dest, const paintCanvas_t* canvas, const wsg_t* wsg, size_t offset, size_t count,
                       const paletteColor_t palette[16])
{
    uint8_t paletteIndex[cTransparent + 1] = {};

    // Build the reverse-palette map
    for (uint16_t i = 0; i < PAINT_MAX_COLORS; i++)
    {
        paletteIndex[((uint8_t)palette[i])] = i;
    }

    uint16_t x0, y0, x1, y1;
    // build the chunk
    for (uint16_t n = 0; n < count; n++)
    {
        if (offset * 2 + (n * 2) >= (wsg ? (wsg->w * wsg->h) : (canvas->w * canvas->h)))
        {
            // If we've just stored the last pixel, return false to indicate that we're done
            return n;
        }

        if (canvas)
        {
            // calculate the real coordinates given the pixel indices
            // (we store 2 pixels in each byte)
            // that's 100% more pixel, per pixel!
            x0 = canvas->x + (offset * 2 + (n * 2)) % canvas->w * canvas->xScale;
            y0 = canvas->y + (offset * 2 + (n * 2)) / canvas->w * canvas->yScale;
            x1 = canvas->x + (offset * 2 + (n * 2 + 1)) % canvas->w * canvas->xScale;
            y1 = canvas->y + (offset * 2 + (n * 2 + 1)) / canvas->w * canvas->yScale;

            // we only need to save the top-left pixel of each scaled pixel, since they're the same unless something is
            // very broken
            dest[n] = paletteIndex[(uint8_t)getPxTft(x0, y0)] << 4 | paletteIndex[(uint8_t)getPxTft(x1, y1)];
        }

        if (wsg)
        {
            dest[n] = paletteIndex[(uint8_t)wsg->px[offset * 2 + n * 2]] << 4
                      | paletteIndex[(uint8_t)wsg->px[offset * 2 + n * 2 + 1]];
        }
    }

    return count;
}
