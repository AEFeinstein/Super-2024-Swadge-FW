//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include <string.h>

#include <esp_log.h>
#include <esp_heap_caps.h>

#include "heatshrink_decoder.h"
#include "hdw-spiffs.h"
#include "spiffs_wsg.h"

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Load a WSG from ROM to RAM. WSGs placed in the spiffs_image folder
 * before compilation will be automatically flashed to ROM
 *
 * @param name The filename of the WSG to load. The ::wsg_t is not allocated by this function.
 * @param wsg  A handle to load the WSG to
 * @return true if the WSG was loaded successfully,
 *         false if the WSG load failed and should not be used
 */
bool loadWsg(char* name, wsg_t* wsg)
{
    return loadWsgSpiRam(name, wsg, false);
}

/**
 * @brief Load a WSG from ROM to RAM. WSGs placed in the spiffs_image folder
 * before compilation will be automatically flashed to ROM
 *
 * @param name The filename of the WSG to load
 * @param wsg  A handle to load the WSG to
 * @param spiRam true to load to SPI RAM, false to load to normal RAM
 * @return true if the WSG was loaded successfully,
 *         false if the WSG load failed and should not be used
 */
bool loadWsgSpiRam(char* name, wsg_t* wsg, bool spiRam)
{
    // Read WSG from file
    uint8_t* buf = NULL;
    size_t sz;
    if (!spiffsReadFile(name, &buf, &sz, true))
    {
        ESP_LOGE("WSG", "Failed to read %s", name);
        return false;
    }

    // Pick out the decompresed size and create a space for it
    uint32_t decompressedSize = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | (buf[3]);
    uint8_t* decompressedBuf  = (uint8_t*)heap_caps_malloc(decompressedSize, MALLOC_CAP_SPIRAM);

    // Create the decoder
    size_t copied           = 0;
    heatshrink_decoder* hsd = heatshrink_decoder_alloc(256, 8, 4);
    heatshrink_decoder_reset(hsd);

    // Decode the file in chunks
    uint32_t inputIdx  = 0;
    uint32_t outputIdx = 0;
    while (inputIdx < (sz - 4))
    {
        // Decode some data
        copied = 0;
        heatshrink_decoder_sink(hsd, &buf[4 + inputIdx], sz - 4 - inputIdx, &copied);
        inputIdx += copied;

        // Save it to the output array
        copied = 0;
        heatshrink_decoder_poll(hsd, &decompressedBuf[outputIdx], decompressedSize - outputIdx, &copied);
        outputIdx += copied;
    }

    // Note that it's all done
    heatshrink_decoder_finish(hsd);

    // Flush any final output
    copied = 0;
    heatshrink_decoder_poll(hsd, &decompressedBuf[outputIdx], decompressedSize - outputIdx, &copied);
    outputIdx += copied;

    // All done decoding
    heatshrink_decoder_finish(hsd);
    heatshrink_decoder_free(hsd);
    // Free the bytes read from the file
    free(buf);

    // Save the decompressed info to the wsg. The first four bytes are dimension
    wsg->w = (decompressedBuf[0] << 8) | decompressedBuf[1];
    wsg->h = (decompressedBuf[2] << 8) | decompressedBuf[3];
    // The rest of the bytes are pixels
    if (spiRam)
    {
        wsg->px = (paletteColor_t*)heap_caps_malloc(sizeof(paletteColor_t) * wsg->w * wsg->h, MALLOC_CAP_SPIRAM);
    }
    else
    {
        wsg->px = (paletteColor_t*)malloc(sizeof(paletteColor_t) * wsg->w * wsg->h);
    }

    if (NULL != wsg->px)
    {
        memcpy(wsg->px, &decompressedBuf[4], outputIdx - 4);
        free(decompressedBuf);
        return true;
    }

    // all done
    free(decompressedBuf);
    return false;
}

/**
 * @brief Free the memory for a loaded WSG
 *
 * @param wsg The WSG handle to free memory from
 */
void freeWsg(wsg_t* wsg)
{
    free(wsg->px);
}
