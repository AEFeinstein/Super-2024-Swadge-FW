//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include <esp_log.h>

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
#ifndef JSON_COMPRESSION
    // Read JSON from file
    size_t sz;
    uint8_t* buf = spiffsReadFile(name, &sz, true);
    if (NULL == buf)
    {
        ESP_LOGE("JSON", "Failed to read %s", name);
        return NULL;
    }
    return (char*)buf;
#else
    uint32_t decompressedSize = 0;
    return readHeatshrinkFile(name, &decompressedSize);
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
