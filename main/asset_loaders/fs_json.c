//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include <esp_log.h>
#include <esp_heap_caps.h>

#include "cnfs.h"
#include "heatshrink_helper.h"
#include "fs_json.h"

//==============================================================================
// Defines
//==============================================================================

#define JSON_COMPRESSION

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Load a JSON from ROM to RAM. JSONs placed in the assets_image folder
 * before compilation will be automatically flashed to ROM
 *
 * @param name The filename of the JSON to load
 * @param spiRam true to load to SPI RAM, false to load to normal RAM. SPI RAM is more plentiful but slower to access
 * than normal RAM
 * @return A pointer to a null terminated JSON string. May be NULL if the load
 *         fails. Must be freed after use
 */
char* loadJson(const char* name, bool spiRam)
{
#ifndef JSON_COMPRESSION
    // Read JSON from file
    size_t sz;
    uint8_t* buf = cnfsReadFile(name, &sz, spiRam);
    if (NULL == buf)
    {
        ESP_LOGE("JSON", "Failed to read %s", name);
        return NULL;
    }
    return (char*)buf;
#else
    uint32_t decompressedSize = 0;
    return (char*)readHeatshrinkFile(name, &decompressedSize, spiRam);
#endif
}

/**
 * @brief Free an allocated JSON string
 *
 * @param jsonStr the JSON string to free
 */
void freeJson(char* jsonStr)
{
    heap_caps_free(jsonStr);
}
