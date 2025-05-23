//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include <string.h>

#include <esp_log.h>
#include <inttypes.h>
#include <esp_heap_caps.h>

#include "cnfs.h"
#include "hdw-nvs.h"
#include "fs_wsg.h"
#include "macros.h"

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Load a WSG from ROM to RAM. WSGs placed in the assets_image folder
 * before compilation will be automatically flashed to ROM
 *
 * @param fIdx The cnfsFileIdx_t the WSG to load
 * @param wsg  A handle to load the WSG to
 * @param spiRam true to load to SPI RAM, false to load to normal RAM. SPI RAM is more plentiful but slower to access
 * than normal RAM
 * @return true if the WSG was loaded successfully,
 *         false if the WSG load failed and should not be used
 */
bool loadWsg(cnfsFileIdx_t fIdx, wsg_t* wsg, bool spiRam)
{
    // Read and decompress file
    uint32_t decompressedSize = 0;
    uint8_t* decompressedBuf  = readHeatshrinkFile(fIdx, &decompressedSize, spiRam);

    if (NULL == decompressedBuf)
    {
        return false;
    }

    // Save the decompressed info to the wsg. The first four bytes are dimension
    wsg->w = (decompressedBuf[0] << 8) | decompressedBuf[1];
    wsg->h = (decompressedBuf[2] << 8) | decompressedBuf[3];
    // The rest of the bytes are pixels
    wsg->px = (paletteColor_t*)heap_caps_malloc_tag(sizeof(paletteColor_t) * wsg->w * wsg->h,
                                                    spiRam ? MALLOC_CAP_SPIRAM : MALLOC_CAP_8BIT, "wsg");

    if (NULL != wsg->px)
    {
        memcpy(wsg->px, &decompressedBuf[4], decompressedSize - 4);
        heap_caps_free(decompressedBuf);
        return true;
    }

    // all done
    heap_caps_free(decompressedBuf);
    return false;
}

/**
 * @brief Load a WSG from ROM to RAM. WSGs placed in the assets_image folder
 * before compilation will be automatically flashed to ROM.
 * You must provide a decoder and decode space to this function. It's useful
 * when creating one decoder & space to decode many consecutive WSGs
 *
 * @param fIdx The cnfsFileIdx_t the WSG to load
 * @param wsg  A handle to load the WSG to
 * @param spiRam true to load to SPI RAM, false to load to normal RAM. SPI RAM is more plentiful but slower to access
 * than normal RAM
 * @param decompressedBuf Memory to store decoded data. This must be as large as the decoded data
 * @param hsd A heatshrink decoder
 * @return true if the WSG was loaded successfully,
 *         false if the WSG load failed and should not be used
 */
bool loadWsgInplace(cnfsFileIdx_t fIdx, wsg_t* wsg, bool spiRam, uint8_t* decompressedBuf, heatshrink_decoder* hsd)
{
    // Read and decompress file
    uint32_t decompressedSize = 0;
    decompressedBuf           = readHeatshrinkFileInplace(fIdx, &decompressedSize, decompressedBuf, hsd);

    if (NULL == decompressedBuf)
    {
        return false;
    }

    // Save the decompressed info to the wsg. The first four bytes are dimension
    wsg->w = (decompressedBuf[0] << 8) | decompressedBuf[1];
    wsg->h = (decompressedBuf[2] << 8) | decompressedBuf[3];
    // The rest of the bytes are pixels
    wsg->px = (paletteColor_t*)heap_caps_malloc_tag(sizeof(paletteColor_t) * wsg->w * wsg->h,
                                                    spiRam ? MALLOC_CAP_SPIRAM : MALLOC_CAP_8BIT, "wsg_inplace");

    if (NULL != wsg->px)
    {
        memcpy(wsg->px, &decompressedBuf[4], decompressedSize - 4);
        return true;
    }

    // all done
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

    ESP_LOGD("WSG", "decompressedBuf size is %" PRIu32, decompressedSize);

    // Save the decompressed info to the wsg. The first four bytes are dimension
    wsg->w = (decompressedBuf[0] << 8) | decompressedBuf[1];
    wsg->h = (decompressedBuf[2] << 8) | decompressedBuf[3];

    ESP_LOGD("WSG", "full WSG is %" PRIu16 " x %" PRIu16 ", or %d pixels", wsg->w, wsg->h, wsg->w * wsg->h);

    // The rest of the bytes are pixels
    wsg->px = (paletteColor_t*)heap_caps_malloc_tag(sizeof(paletteColor_t) * wsg->w * wsg->h,
                                                    spiRam ? MALLOC_CAP_SPIRAM : MALLOC_CAP_8BIT, key);

    if (NULL != wsg->px)
    {
        ESP_LOGD("WSG", "Copying %" PRIu32 " pixels into WSG now", decompressedSize - 4);
        memcpy(wsg->px, &decompressedBuf[4], decompressedSize - 4);
        heap_caps_free(decompressedBuf);
        return true;
    }

    ESP_LOGE("WSG", "Allocating pixels failed");

    // all done
    heap_caps_free(decompressedBuf);
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
    uint8_t* output = heap_caps_malloc_tag(imageSize, MALLOC_CAP_SPIRAM, key);

    if (!output)
    {
        ESP_LOGE("WSG", "Failed to allocate output buffer for image");
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
        ESP_LOGD("WSG", "Compressed image from %" PRIu32 " bytes to %" PRIu32, imageSize, outputSize);
        result = writeNamespaceNvsBlob(namespace, key, output, outputSize);
    }

    heap_caps_free(output);
    return result;
}

/**
 * @brief Free the memory for a loaded WSG
 *
 * @param wsg The WSG handle to free memory from
 */
void freeWsg(wsg_t* wsg)
{
    if (wsg->w && wsg->h)
    {
        heap_caps_free(wsg->px);
        wsg->h = 0;
        wsg->w = 0;
    }
}
