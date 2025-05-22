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
#include "fs_txt.h"

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Load a TXT from ROM to RAM. TXTs placed in the assets_image folder
 * before compilation will be automatically flashed to ROM
 *
 * @param fIdx The cnfsFileIdx_t the TXT to load
 * @param spiRam true to load to SPI RAM, false to load to normal RAM. SPI RAM is more plentiful but slower to access
 * than normal RAM
 * @return A pointer to a null terminated TXT string. May be NULL if the load
 *         fails. Must be freed after use
 */
char* loadTxt(cnfsFileIdx_t fIdx, bool spiRam)
{
    // Read TXT from file
    size_t sz;
    uint8_t* buf = cnfsReadFile(fIdx, &sz, spiRam);
    if (NULL == buf)
    {
        ESP_LOGE("TXT", "Failed to read %d", fIdx);
        return NULL;
    }

    return (char*)buf;
}

/**
 * @brief Free an allocated TXT string
 *
 * @param txtStr the TXT string to free
 */
void freeTxt(char* txtStr)
{
    heap_caps_free(txtStr);
}
