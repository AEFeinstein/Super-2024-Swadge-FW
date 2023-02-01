//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "esp_log.h"
#include "hdw-spiffs.h"
#include "heatshrink_decoder.h"
#include "spiffs_json.h"

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Load a JSON from ROM to RAM. JSONs placed in the spiffs_image folder
 * before compilation will be automatically flashed to ROM
 *
 * @param name The filename of the JSON to load
 * @return A pointer to a null terminated JSON string. May be NULL if the load
 *         fails. Must be freed after use
 */
char* loadJson(const char* name)
{
    // Read JSON from file
    uint8_t* buf = NULL;
    size_t sz;
    if (!spiffsReadFile(name, &buf, &sz, true))
    {
        ESP_LOGE("JSON", "Failed to read %s", name);
        return NULL;
    }

#ifndef JSON_COMPRESSION
    return (char*)buf;
#else
    // Pick out the decompresed size and create a space for it
    uint16_t decompressedSize = (buf[0] << 8) | buf[1];
    uint8_t* decompressedBuf  = calloc(sizeof(char) * (decompressedSize + 1));

    // Create the decoder
    size_t copied           = 0;
    heatshrink_decoder* hsd = heatshrink_decoder_alloc(256, 8, 4);
    heatshrink_decoder_reset(hsd);

    // Decode the file in chunks
    uint32_t inputIdx  = 0;
    uint32_t outputIdx = 0;
    while (inputIdx < (sz - 2))
    {
        // Decode some data
        copied = 0;
        heatshrink_decoder_sink(hsd, &buf[2 + inputIdx], sz - 2 - inputIdx, &copied);
        inputIdx += copied;

        // Save it to the output array
        copied = 0;
        heatshrink_decoder_poll(hsd, &decompressedBuf[outputIdx], sizeof(decompressedBuf) - outputIdx, &copied);
        outputIdx += copied;
    }

    // Note that it's all done
    heatshrink_decoder_finish(hsd);

    // Flush any final output
    copied = 0;
    heatshrink_decoder_poll(hsd, &decompressedBuf[outputIdx], sizeof(decompressedBuf) - outputIdx, &copied);
    outputIdx += copied;

    // All done decoding
    heatshrink_decoder_finish(hsd);
    heatshrink_decoder_free(hsd);
    free(buf);

    // Add null terminator
    decompressedBuf[decompressedSize] = 0;

    return decompressedBuf;
#endif
}

/**
 * @brief Free an allocated JSON string
 *
 * @param jsonStr the JSON string to free
 */
void freeJson(char* jsonStr)
{
    free(jsonStr);
}
