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
    uint32_t imageSize = pixelSize + wsgHeaderSize;

    uint8_t wsgHeader[4];
    wsgHeader[0] = (wsg->w >> 8) & 0xFF;
    wsgHeader[1] = (wsg->w) & 0xFF;
    wsgHeader[2] = (wsg->h >> 8) & 0xFF;
    wsgHeader[3] = (wsg->h) & 0xFF;

    // The image size, plus the heatshrink header for total size
    uint32_t outputSize = 4 + imageSize;

    ESP_LOGI("WSG", "Uncompressed space is %" PRIu32, outputSize);

    // TODO this may be too big for stack?
    uint8_t output[outputSize];

    // Write the total
    output[0] = (imageSize >> 24) & 0xFF;
    output[1] = (imageSize >> 16) & 0xFF;
    output[2] = (imageSize >> 8) & 0xFF;
    output[3] = (imageSize) & 0xFF;

    // move output buffer to account for header
    uint32_t outputIdx  = 4;
    uint32_t inputIdx   = 0;
    size_t copied       = 0;

    heatshrink_encoder* hse = heatshrink_encoder_alloc(8, 4);
    heatshrink_encoder_reset(hse);

    bool headerDone = false;

    while (inputIdx < pixelSize)
    {
        copied = 0;
        HSE_sink_res result;

        if (!headerDone)
        {
            // So just send 4 bytes of the WSG header first to avoid making another huge buffer
            result = heatshrink_encoder_sink(hse, wsgHeader, 4, &copied);
            if (copied == 4)
            {
                headerDone = true;
            }
        }
        else
        {
            // Then send the rest of the data
            result = heatshrink_encoder_sink(hse, &wsg->px[inputIdx], pixelSize - inputIdx, &copied);
            inputIdx += copied;
        }

        switch (result)
        {
            case HSER_SINK_OK:
            {
                // Continue
                break;
            }

            case HSER_SINK_ERROR_NULL:
            case HSER_SINK_ERROR_MISUSE:
            {
                goto heatshrink_error;
            }
        }

        bool stillEncoding = true;
        while (stillEncoding)
        {
            copied = 0;
            switch (heatshrink_encoder_poll(hse, &output[outputIdx], outputSize - outputIdx, &copied))
            {
                case HSER_POLL_EMPTY:
                {
                    stillEncoding = false;
                    break;
                }
                case HSER_POLL_MORE:
                {
                    stillEncoding = true;
                    break;
                }
                case HSER_POLL_ERROR_NULL:
                case HSER_POLL_ERROR_MISUSE:
                {
                    // Error handling
                    goto heatshrink_error;
                }
            }
            outputIdx += copied;
        }
    }

    /* Mark all input as processed */
    switch (heatshrink_encoder_finish(hse))
    {
        case HSER_FINISH_DONE:
        {
            // Continue
            break;
        }
        case HSER_FINISH_MORE:
        {
            /* Flush the last bits of output */
            copied             = 0;
            bool stillEncoding = true;
            while (stillEncoding)
            {
                switch (heatshrink_encoder_poll(hse, &output[outputIdx], outputSize - outputIdx, &copied))
                {
                    case HSER_POLL_EMPTY:
                    {
                        stillEncoding = false;
                        break;
                    }
                    case HSER_POLL_MORE:
                    {
                        stillEncoding = true;
                        break;
                    }
                    case HSER_POLL_ERROR_NULL:
                    case HSER_POLL_ERROR_MISUSE:
                    {
                        // Error handling
                        goto heatshrink_error;
                    }
                }
                outputIdx += copied;
            }
            break;
        }
        case HSER_FINISH_ERROR_NULL:
        {
            // Error handling
            goto heatshrink_error;
        }
    }

    ESP_LOGI("WSG", "Compressed size is %" PRIu32, outputIdx);

    writeNvsBlob(key, output, outputIdx);

    heatshrink_encoder_free(hse);
    return true;

heatshrink_error:
    if (NULL != hse)
    {
        heatshrink_encoder_free(hse);
    }

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
