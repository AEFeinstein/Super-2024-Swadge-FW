//==============================================================================
// Includes
//==============================================================================

#include <stdio.h>
#include <string.h>

#include <esp_err.h>
#include <esp_log.h>
#include <esp_heap_caps.h>
#include "cnfs.h"

#include "cnfs_image.h"

//==============================================================================
// Variables
//==============================================================================

static const uint8_t* cnfsData;
static int32_t cnfsDataSz;

static const cnfsFileEntry* cnfsFiles;
static int32_t cnfsNumFiles;

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initialize the CNFS file system. This is used to store assets like
 * WSGs and fonts
 *
 * @return true if CNFS was initialized and can be used, false if it failed
 */
bool initCnfs(void)
{
    /* Get local references from cnfs_data.c */
    cnfsData     = getCnfsImage();
    cnfsDataSz   = getCnfsSize();
    cnfsFiles    = getCnfsFiles();
    cnfsNumFiles = getCnfsNumFiles();

    /* Debug print */
    ESP_LOGI("CNFS", "Size: %" PRIu32 ", Files: %" PRIu32, cnfsDataSz, cnfsNumFiles);
    return (0 != cnfsDataSz) && (0 != cnfsNumFiles);
}

/**
 * @brief De-initialize the CNFS file system (right now a noop)
 *
 * @return true if de-initialize ok, false if it was not
 */
bool deinitCnfs(void)
{
    return true;
}

/**
 * @brief Get a pointer to a file, without needing to read it. Same rules that
 * apply to cnfsGetFile, and under the hood, cnfsGetFile uses this function.
 *
 * @param fIdx the cnfsFileIdx_t of the file to get
 * @param flen    A pointer to a size_t to return the size of the file.
 * @return A pointer to the read data if successful, or NULL if there is a failure
 *         Do not free this pointer. It is pointing to flash.
 */
const uint8_t* cnfsGetFile(cnfsFileIdx_t fIdx, size_t* flen)
{
    if (fIdx < CNFS_NUM_FILES)
    {
        *flen = cnfsFiles[fIdx].len;
        return &cnfsData[cnfsFiles[fIdx].offset];
    }
    else
    {
        *flen = 0;
        return NULL;
    }
}

/**
 * @brief Read a file from CNFS into an output array. Files that are in the
 * assets_image folder before compilation and flashing will automatically
 * be included in the firmware.
 *
 * @param fIdx The cnfsFileIdx_t of the file to load
 * @param outsize A pointer to a size_t to return how much data was read
 * @param readToSpiRam true to use SPI RAM, false to use normal RAM
 * @return A pointer to the read data if successful, or NULL if there is a failure
 *         This data must be freed when done
 */
uint8_t* cnfsReadFile(cnfsFileIdx_t fIdx, size_t* outsize, bool readToSpiRam)
{
    const uint8_t* fptr = cnfsGetFile(fIdx, outsize);

    if (!fptr)
    {
        return 0;
    }

    uint8_t* output;

    if (readToSpiRam)
    {
        output = (uint8_t*)heap_caps_calloc((*outsize + 1), sizeof(uint8_t), MALLOC_CAP_SPIRAM);
    }
    else
    {
        output = (uint8_t*)heap_caps_calloc((*outsize + 1), sizeof(uint8_t), MALLOC_CAP_8BIT);
    }
    memcpy(output, fptr, *outsize);
    return output;
}
