//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include <string.h>

#include <esp_log.h>
#include <inttypes.h>
#include <esp_heap_caps.h>

#include "heatshrink_helper.h"
#include "heatshrink_encoder.h"
#include "hdw-spiffs.h"
#include "hdw-nvs.h"
#include "spiffs_wsg.h"
#include "macros.h"

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Load a WSG from ROM to RAM. WSGs placed in the spiffs_image folder
 * before compilation will be automatically flashed to ROM
 *
 * @param name The filename of the WSG to load
 * @param wsg  A handle to load the WSG to
 * @param spiRam true to load to SPI RAM, false to load to normal RAM. SPI RAM is more plentiful but slower to access
 * than normal RAM
 * @return true if the WSG was loaded successfully,
 *         false if the WSG load failed and should not be used
 */
bool loadWsg(const char* name, wsg_t* wsg, bool spiRam)
{
    // Read and decompress file
    uint32_t decompressedSize = 0;
    uint8_t* decompressedBuf  = readHeatshrinkFile(name, &decompressedSize, spiRam);

    if (NULL == decompressedBuf)
    {
        return false;
    }

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
        memcpy(wsg->px, &decompressedBuf[4], decompressedSize - 4);
        free(decompressedBuf);
        return true;
    }

    // all done
    free(decompressedBuf);
    return false;
}

bool loadWsgNvs(const char* namespace, const char* key, wsg_t* wsg, bool spiRam)
{
    // Read and decompress file
    uint32_t decompressedSize = 0;
    uint8_t* decompressedBuf  = readHeatshrinkNvs(namespace, key, &decompressedSize, spiRam);

    if (NULL == decompressedBuf)
    {
        return false;
    }

    ESP_LOGI("WSG", "decompressedBuf size is %" PRIu32, decompressedSize);

    // Save the decompressed info to the wsg. The first four bytes are dimension
    wsg->w = (decompressedBuf[0] << 8) | decompressedBuf[1];
    wsg->h = (decompressedBuf[2] << 8) | decompressedBuf[3];

    ESP_LOGI("WSG", "full WSG is %" PRIu16 " x %" PRIu16 ", or %d pixels", wsg->w, wsg->h, wsg->w * wsg->h);

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
        ESP_LOGI("WSG", "Copying %" PRIu32 " pixels into WSG now", decompressedSize - 4);
        memcpy(wsg->px, &decompressedBuf[4], decompressedSize - 4);
        free(decompressedBuf);
        return true;
    }

    ESP_LOGI("WSG", "Allocating pixels failed");

    // all done
    free(decompressedBuf);
    return false;
}

bool saveWsgNvs(const char* namespace, const char* key, const wsg_t* wsg)
{
    // pixelSize is just the WSG pixels, no dimensions
    uint32_t pixelSize = sizeof(uint8_t) * wsg->w * wsg->h;

    // wsgHeaderSize is the 4 bytes of WSG header, 2 for W and 2 for H
    uint32_t wsgHeaderSize = 4;

    // imageSize is the total image data size; header + pixels
    uint32_t imageSize = wsgHeaderSize + pixelSize;

    // Create an output buffer big enough to hold the uncompressed image
    uint8_t* output = heap_caps_malloc(imageSize, MALLOC_CAP_SPIRAM);

    if (!output)
    {
        ESP_LOGI("WSG", "Failed to allocate output buffer for image");
        return false;
    }

    // Write the WSG header to the buffer
    output[0] = (wsg->w >> 8) & 0xFF;
    output[1] = (wsg->w) & 0xFF;
    output[2] = (wsg->h >> 8) & 0xFF;
    output[3] = (wsg->h) & 0xFF;

    // Copy the pixels into the input buffer after the
    memcpy(output + 4, wsg->px, pixelSize);

    // Compress the buffer in-place
    uint32_t outputSize = heatshrinkCompress(output, output, imageSize);

    bool result;
    if (outputSize == 0)
    {
        ESP_LOGE("WSG", "Failed to save WSG to NVS");
        result = false;
    }
    else
    {
        ESP_LOGI("WSG", "Compressed image from %" PRIu32 " bytes to %" PRIu32, imageSize, outputSize);
        result = writeNamespaceNvsBlob(namespace, key, output, outputSize);
    }

    free(output);
    return result;
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
