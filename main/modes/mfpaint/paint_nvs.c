#include "paint_nvs.h"

#include "nvs_manager.h"

#include "paint_common.h"
#include "paint_draw.h"
#include "paint_ui.h"
#include "paint_util.h"

static const char KEY_PAINT_INDEX[] = "pnt_idx";
static const char KEY_PAINT_SLOT_PALETTE[] = "paint_%02d_pal";
static const char KEY_PAINT_SLOT_DIM[] = "paint_%02d_dim";
static const char KEY_PAINT_SLOT_CHUNK[] = "paint_%02dc%05u";

// void paintDebugIndex(int32_t index)
// {
//     char bintext[33];

//     for (uint8_t i = 0; i < 32; i++)
//     {
//         bintext[31 - i] = (index & (1 << i)) ? '1' : '0';
//     }

//     bintext[32] = '\0';

//     PAINT_LOGD("PAINT INDEX:");
//     PAINT_LOGD("  Raw: %s", bintext);
//     PAINT_LOGD("  Slots:");
//     PAINT_LOGD("  - [%c] Slot 1", paintGetSlotInUse(index, 0) ? 'X' : ' ');
//     PAINT_LOGD("  - [%c] Slot 2", paintGetSlotInUse(index, 1) ? 'X' : ' ');
//     PAINT_LOGD("  - [%c] Slot 3", paintGetSlotInUse(index, 2) ? 'X' : ' ');
//     PAINT_LOGD("  - [%c] Slot 4", paintGetSlotInUse(index, 3) ? 'X' : ' ');
//     if (paintGetRecentSlot(index) == PAINT_SAVE_SLOTS)
//     {
//         PAINT_LOGD("  Recent Slot: None");
//     }
//     else
//     {
//         PAINT_LOGD("  Recent Slot: %d", paintGetRecentSlot(index) + 1);
//     }
//     PAINT_LOGD("  LEDs: %s", (index & PAINT_ENABLE_LEDS) ? "Yes" : "No");
//     PAINT_LOGD("  Blink: %s", (index & PAINT_ENABLE_BLINK) ? "Yes" : "No");
//     PAINT_LOGD("===========");
// }

void paintLoadIndex(int32_t* index)
{
    //       SFX?
    //     BGM | Lights?
    //        \|/
    // |xxxxxvvv  |Recent?|  |Inuse? |
    // 0000 0000  0000 0000  0000 0000

    if (!readNvs32(KEY_PAINT_INDEX, index))
    {
        PAINT_LOGW("No metadata! Setting defaults");
        *index = PAINT_DEFAULTS;
        paintSaveIndex(*index);
    }
}

void paintSaveIndex(int32_t index)
{
    if (writeNvs32(KEY_PAINT_INDEX, index))
    {
        PAINT_LOGD("Saved index: %04x", index);
    }
    else
    {
        PAINT_LOGE("Failed to save index :(");
    }
}

// void paintResetStorage(int32_t* index)
// {
//     *index = PAINT_DEFAULTS;
//     paintSaveIndex(*index);
// }

bool paintGetSlotInUse(int32_t index, uint8_t slot)
{
    return (index & (1 << slot)) != 0;
}

void paintClearSlot(int32_t* index, uint8_t slot)
{
    *index &= ~(1 << slot);
    paintSaveIndex(*index);
}

void paintSetSlotInUse(int32_t* index, uint8_t slot)
{
    *index |= (1 << slot);
    paintSaveIndex(*index);
}

bool paintGetAnySlotInUse(int32_t index)
{
    return (index & ((1 << PAINT_SAVE_SLOTS) - 1)) != 0;
}

/**
 * Returns the most recent save slot used, or PAINT_SAVE_SLOTS if there is none set
*/
uint8_t paintGetRecentSlot(int32_t index)
{
    return (index >> PAINT_SAVE_SLOTS) & 0b111;
}

void paintSetRecentSlot(int32_t* index, uint8_t slot)
{
    // TODO if we change the number of slots this will totally not work anymore
    // I mean, we could just do & 0xFF and waste 5 whole bits
    *index = (*index & PAINT_MASK_NOT_RECENT) | ((slot & 0b111) << PAINT_SAVE_SLOTS);
    paintSaveIndex(*index);
}

/**
 * Returns the number of bytes needed to store the image pixel data
 */
size_t paintGetStoredSize(const paintCanvas_t* canvas)
{
    return (canvas->w * canvas->h + 1) / 2;
}

bool paintDeserialize(paintCanvas_t* dest, const uint8_t* data, size_t offset, size_t count)
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

        setPxScaled(dest->disp, x0, y0, dest->palette[data[n] >> 4], dest->x, dest->y, dest->xScale, dest->yScale);
        setPxScaled(dest->disp, x1, y1, dest->palette[data[n] & 0xF], dest->x, dest->y, dest->xScale, dest->yScale);
    }

    return offset * 2 + (count * 2) < dest->w * dest->h;
}

size_t paintSerialize(uint8_t* dest, const paintCanvas_t* canvas, size_t offset, size_t count)
{
    uint8_t paletteIndex[cTransparent + 1];

    // Build the reverse-palette map
    for (uint16_t i = 0; i < PAINT_MAX_COLORS; i++)
    {
        paletteIndex[((uint8_t)canvas->palette[i])] = i;
    }

    uint16_t x0, y0, x1, y1;
    // build the chunk
    for (uint16_t n = 0; n < count; n++)
    {
        if (offset * 2 + (n * 2) >= canvas->w * canvas->h)
        {
            // If we've just stored the last pixel, return false to indicate that we're done
            return n;
        }

        // calculate the real coordinates given the pixel indices
        // (we store 2 pixels in each byte)
        // that's 100% more pixel, per pixel!
        x0 = canvas->x + (offset * 2 + (n * 2)) % canvas->w * canvas->xScale;
        y0 = canvas->y + (offset * 2 + (n * 2)) / canvas->w * canvas->yScale;
        x1 = canvas->x + (offset * 2 + (n * 2 + 1)) % canvas->w * canvas->xScale;
        y1 = canvas->y + (offset * 2 + (n * 2 + 1)) / canvas->w * canvas->yScale;

        // we only need to save the top-left pixel of each scaled pixel, since they're the same unless something is very broken
        dest[n] = paletteIndex[(uint8_t)canvas->disp->getPx(x0, y0)] << 4 | paletteIndex[(uint8_t)canvas->disp->getPx(x1, y1)];
    }

    return count;
}

bool paintSave(int32_t* index, const paintCanvas_t* canvas, uint8_t slot)
{
    // NVS blob key name
    char key[16];

    // pointer to converted image segment
    uint8_t* imgChunk = NULL;

    if ((imgChunk = malloc(sizeof(uint8_t) * PAINT_SAVE_CHUNK_SIZE)) != NULL)
    {
        PAINT_LOGD("Allocated %d bytes for image chunk", PAINT_SAVE_CHUNK_SIZE);
    }
    else
    {
        PAINT_LOGE("malloc failed for %d bytes", PAINT_SAVE_CHUNK_SIZE);
        return false;
    }

    // Save the palette map, this lets us compact the image by 50%
    snprintf(key, 16, KEY_PAINT_SLOT_PALETTE, slot);
    PAINT_LOGD("paletteColor_t size: %zu, max colors: %d", sizeof(paletteColor_t), PAINT_MAX_COLORS);
    PAINT_LOGD("Palette will take up %zu bytes", sizeof(canvas->palette));
    if (writeNvsBlob(key, canvas->palette, sizeof(canvas->palette)))
    {
        PAINT_LOGD("Saved palette to slot %s", key);
    }
    else
    {
        PAINT_LOGE("Could't save palette to slot %s", key);
        free(imgChunk);
        return false;
    }

    // Save the canvas dimensions
    uint32_t packedSize = canvas->w << 16 | canvas->h;
    snprintf(key, 16, KEY_PAINT_SLOT_DIM, slot);
    if (writeNvs32(key, packedSize))
    {
        PAINT_LOGD("Saved dimensions to slot %s", key);
    }
    else
    {
        PAINT_LOGE("Couldn't save dimensions to slot %s",key);
        free(imgChunk);
        return false;
    }

    size_t offset = 0;
    size_t written = 0;
    uint32_t chunkNumber = 0;

    // Write until we're done
    while (0 != (written = paintSerialize(imgChunk, canvas, offset, PAINT_SAVE_CHUNK_SIZE)))
    {
        // save the chunk
        snprintf(key, 16, KEY_PAINT_SLOT_CHUNK, slot, chunkNumber);
        if (writeNvsBlob(key, imgChunk, written))
        {
            PAINT_LOGD("Saved blob %u with %zu bytes", chunkNumber, written);
        }
        else
        {
            PAINT_LOGE("Unable to save blob %u", chunkNumber);
            free(imgChunk);
            return false;
        }

        offset += written;
        chunkNumber++;
    }

    paintSetSlotInUse(index, slot);
    paintSetRecentSlot(index, slot);
    paintSaveIndex(*index);

    free(imgChunk);
    imgChunk = NULL;

    return true;
}

bool paintLoad(int32_t* index, paintCanvas_t* canvas, uint8_t slot)
{
    // NVS blob key name
    char key[16];

    // pointer to converted image segment
    uint8_t* imgChunk = NULL;

    // Allocate space for the chunk
    if ((imgChunk = malloc(PAINT_SAVE_CHUNK_SIZE)) != NULL)
    {
        PAINT_LOGD("Allocated %d bytes for image chunk", PAINT_SAVE_CHUNK_SIZE);
    }
    else
    {
        PAINT_LOGE("malloc failed for %d bytes", PAINT_SAVE_CHUNK_SIZE);
        return false;
    }

    // read the palette and load it into the recentColors
    // read the dimensions and do the math
    // read the pixels

    size_t paletteSize;

    if (!paintGetSlotInUse(*index, slot))
    {
        PAINT_LOGW("Attempted to load from uninitialized slot %d", slot);
        free(imgChunk);
        return false;
    }

    // Load the palette map
    snprintf(key, 16, KEY_PAINT_SLOT_PALETTE, slot);

    if (!readNvsBlob(key, NULL, &paletteSize))
    {
        PAINT_LOGE("Couldn't read size of palette in slot %s", key);
        free(imgChunk);
        return false;
    }

    // TODO Move this outside of this function
    if (readNvsBlob(key, canvas->palette, &paletteSize))
    {
        PAINT_LOGD("Read %zu bytes of palette from slot %s", paletteSize, key);
    }
    else
    {
        PAINT_LOGE("Could't read palette from slot %s", key);
        free(imgChunk);
        return false;
    }

    if (!paintLoadDimensions(canvas, slot))
    {
        PAINT_LOGE("Slot %d has 0 dimension! Stopping load and clearing slot", slot);
        paintClearSlot(index, slot);
        free(imgChunk);
        return false;
    }

    // Read all the chunks
    size_t lastChunkSize = 0;
    uint32_t chunkNumber = 0;
    size_t offset = 0;

    do
    {
        offset += lastChunkSize;
        snprintf(key, 16, KEY_PAINT_SLOT_CHUNK, slot, chunkNumber);
        // panic
        if (!readNvsBlob(key, NULL, &lastChunkSize) || lastChunkSize > PAINT_SAVE_CHUNK_SIZE)
        {
            PAINT_LOGE("Unable to read size of blob %u in slot %s", chunkNumber, key);
            free(imgChunk);
            return false;
        }

        // read the chunk
        if (readNvsBlob(key, imgChunk, &lastChunkSize))
        {
            PAINT_LOGD("Read blob %d (%zu bytes)", chunkNumber, lastChunkSize);
        }
        else
        {
            PAINT_LOGE("Unable to read blob %d", chunkNumber);
            // do panic if we miss one chunk, it's probably not ok...
            free(imgChunk);
            return false;
        }

        chunkNumber++;
        // paintDeserialize() will return true if there's more to be read
    } while (paintDeserialize(canvas, imgChunk, offset, lastChunkSize));

    free(imgChunk);
    imgChunk = NULL;

    return true;
}

bool paintLoadDimensions(paintCanvas_t* canvas, uint8_t slot)
{
    char key[16];
    // Read the canvas dimensions
    PAINT_LOGD("Reading dimensions");
    int32_t packedSize;
    snprintf(key, 16, KEY_PAINT_SLOT_DIM, slot);
    if (readNvs32(key, &packedSize))
    {
        canvas->h = (uint32_t)packedSize & 0xFFFF;
        canvas->w = (((uint32_t)packedSize) >> 16) & 0xFFFF;
        PAINT_LOGD("Read dimensions from slot %s: %d x %d", key, canvas->w, canvas->h);

        if (canvas->h == 0 || canvas->w == 0)
        {
            return false;
        }
    }
    else
    {
        PAINT_LOGE("Couldn't read dimensions from slot %s",key);
        return false;
    }

    return true;
}

uint8_t paintGetPrevSlotInUse(int32_t index, uint8_t slot)
{
    do
    {
        // Switch to the previous slot, wrapping back to the end
        slot = PREV_WRAP(slot, PAINT_SAVE_SLOTS);
    }
    // If we're loading, and there's actually a slot we can load from, skip empty slots until we find one that is in use
    while (paintGetAnySlotInUse(index) && !paintGetSlotInUse(index, slot));

    return slot;
}

uint8_t paintGetNextSlotInUse(int32_t index, uint8_t slot)
{
    do
    {
        slot = NEXT_WRAP(slot, PAINT_SAVE_SLOTS);
    } while (paintGetAnySlotInUse(index) && !paintGetSlotInUse(index, slot));

    return slot;
}

void paintDeleteSlot(int32_t* index, uint8_t slot)
{
    // NVS blob key name
    char key[16];

    if (slot >= PAINT_SAVE_SLOTS)
    {
        PAINT_LOGE("Attempt to delete invalid slto %d", slot);
        return;
    }

    if (!paintGetSlotInUse(*index, slot))
    {
        PAINT_LOGW("Attempting to delete allegedly unused slot %d", slot);
    }

    // Delete the palette
    snprintf(key, 16, KEY_PAINT_SLOT_PALETTE, slot);
    if (!eraseNvsKey(key))
    {
        PAINT_LOGE("Couldn't delete palette of slot %d", slot);
    }

    snprintf(key, 16, KEY_PAINT_SLOT_DIM, slot);
    if (!eraseNvsKey(key))
    {
        PAINT_LOGE("Couldn't delete dimensions of slot %d", slot);
    }

    // Erase chunks until we fail to find one
    uint8_t i = 0;
    do
    {
        snprintf(key, 16, KEY_PAINT_SLOT_CHUNK, slot, i++);
    } while (eraseNvsKey(key));

    PAINT_LOGI("Erased %d chunks of slot %d", i - 1, slot);
    paintClearSlot(index, slot);

    if (paintGetRecentSlot(*index) == slot)
    {
        // Unset the most recent slot if we're deleting it
        paintSetRecentSlot(index, PAINT_SAVE_SLOTS);
    }
}

bool paintDeleteIndex(void)
{
    if (eraseNvsKey(KEY_PAINT_INDEX))
    {
        PAINT_LOGI("Erased index!");
        return true;
    }
    else
    {
        PAINT_LOGE("Failed to erase index!");
        return false;
    }
}