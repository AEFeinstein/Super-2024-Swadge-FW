#include <stddef.h>

#include <esp_log.h>
#include <inttypes.h>
#include <esp_err.h>

#include "hdw-spiffs.h"
#include "hdw-nvs.h"
#include <nvs.h>

#include "heatshrink_decoder.h"
#include "heatshrink_helper.h"

/**
 * @brief Read a heatshrink compressed file from SPIFFS into an output array.
 * Files that are in the spiffs_image folder before compilation and flashing
 * will automatically be included in the firmware.
 *
 * @param fname   The name of the file to load
 * @param outsize A pointer to a size_t to return how much data was read
 * @param readToSpiRam true to use SPI RAM, false to use normal RAM
 * @return A pointer to the read data if successful, or NULL if there is a failure
 *         This data must be freed when done
 */
uint8_t* readHeatshrinkFile(const char* fname, uint32_t* outsize, bool readToSpiRam)
{
    // Read WSG from file
    size_t sz;
    uint8_t* buf = spiffsReadFile(fname, &sz, readToSpiRam);
    if (NULL == buf)
    {
        ESP_LOGE("WSG", "Failed to read %s", fname);
        (*outsize) = 0;
        return NULL;
    }

    // Pick out the decompresed size and create a space for it
    (*outsize) = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | (buf[3]);
    uint8_t* decompressedBuf;
    if (readToSpiRam)
    {
        decompressedBuf = (uint8_t*)heap_caps_malloc((*outsize), MALLOC_CAP_SPIRAM);
    }
    else
    {
        decompressedBuf = (uint8_t*)malloc((*outsize));
    }

    // Create the decoder
    size_t copied           = 0;
    heatshrink_decoder* hsd = heatshrink_decoder_alloc(256, 8, 4);
    heatshrink_decoder_reset(hsd);

    // The decompressed filesize is four bytes, so start after that
    uint32_t inputIdx  = 4;
    uint32_t outputIdx = 0;
    // Decode the file in chunks
    while (inputIdx < sz)
    {
        // Decode some data
        copied = 0;
        heatshrink_decoder_sink(hsd, &buf[inputIdx], sz - inputIdx, &copied);
        inputIdx += copied;

        // Save it to the output array
        copied = 0;
        heatshrink_decoder_poll(hsd, &decompressedBuf[outputIdx], (*outsize) - outputIdx, &copied);
        outputIdx += copied;
    }

    // Note that it's all done
    heatshrink_decoder_finish(hsd);

    // Flush any final output
    copied = 0;
    heatshrink_decoder_poll(hsd, &decompressedBuf[outputIdx], (*outsize) - outputIdx, &copied);
    outputIdx += copied;

    // All done decoding
    heatshrink_decoder_finish(hsd);
    heatshrink_decoder_free(hsd);
    // Free the bytes read from the file
    free(buf);

    // Return the decompressed bytes
    return decompressedBuf;
}

uint8_t* readHeatshrinkNvs(const char* namespace, const char* key, uint32_t* outsize, bool spiRam)
{
    // Read WSG from NVS
    size_t sz;

    // Get full size
    readNamespaceNvsBlob(namespace, key, NULL, &sz);

    ESP_LOGI("Heatshrink", "Compressed size is %" PRIu64, (uint64_t)sz);

    uint8_t* buf = (uint8_t*)heap_caps_malloc(sz, spiRam ? MALLOC_CAP_SPIRAM : 0);
    if (!buf)
    {
        return NULL;
    }

    if (!readNamespaceNvsBlob(namespace, key, buf, &sz))
    {
        free(buf);
        return NULL;
    }

    // Pick out the decompresed size and create a space for it
    (*outsize) = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | (buf[3]);
    ESP_LOGI("Heatshrink", "Outsize is %" PRIu64, (uint64_t)*outsize);
    uint8_t* decompressedBuf = (uint8_t*)heap_caps_malloc((*outsize), spiRam ? MALLOC_CAP_SPIRAM : 0);

    // Create the decoder
    size_t copied           = 0;
    heatshrink_decoder* hsd = heatshrink_decoder_alloc(256, 8, 4);
    heatshrink_decoder_reset(hsd);

    // The decompressed filesize is four bytes, so start after that
    uint32_t inputIdx  = 4;
    uint32_t outputIdx = 0;
    // Decode the file in chunks
    while (inputIdx < sz)
    {
        // Decode some data
        copied = 0;
        heatshrink_decoder_sink(hsd, &buf[inputIdx], sz - inputIdx, &copied);
        inputIdx += copied;

        // Save it to the output array
        copied = 0;
        heatshrink_decoder_poll(hsd, &decompressedBuf[outputIdx], (*outsize) - outputIdx, &copied);
        outputIdx += copied;
    }

    // Note that it's all done
    heatshrink_decoder_finish(hsd);

    // Flush any final output
    copied = 0;
    heatshrink_decoder_poll(hsd, &decompressedBuf[outputIdx], (*outsize) - outputIdx, &copied);
    outputIdx += copied;

    // All done decoding
    heatshrink_decoder_finish(hsd);
    heatshrink_decoder_free(hsd);
    // Free the bytes read from the file
    free(buf);

    // Return the decompressed bytes
    return decompressedBuf;
}

bool writeHeatshrinkNvs(const char* namespace, const char* key, const uint8_t* data, uint32_t size)
{
    return false;
}