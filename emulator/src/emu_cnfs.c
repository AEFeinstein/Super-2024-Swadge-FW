#include "emu_cnfs.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "cnfs.h"
#include "cnfs_image.h"

//==============================================================================
// Variables
//==============================================================================

// Original CNFS Variables
static const uint8_t* cnfsData;
static int32_t cnfsDataSz;

static const cnfsFileEntry* cnfsFiles;

// Extended CNFS Variables

static char* cnfsInjectedFilename   = NULL;
static int32_t cnfsInjectedFileSize = 0;
static void* cnfsInjectedFileData   = NULL;

//==============================================================================
// Functions
//==============================================================================

bool initCnfs(void)
{
    /* Get local references from cnfs_data.c */
    cnfsData   = getCnfsImage();
    cnfsDataSz = getCnfsSize();
    cnfsFiles  = getCnfsFiles();

    /* Debug print */
    ESP_LOGI("CNFS", "Size: %" PRIu32 ", Files: %" PRIu32, cnfsDataSz, CNFS_NUM_FILES);
    return (0 != cnfsDataSz) && (0 != CNFS_NUM_FILES);
}

bool emuCnfsInjectFile(const char* name, const char* filePath)
{
    FILE* dataFile = fopen(filePath, "rb");
    if (dataFile != NULL)
    {
        fseek(dataFile, 0L, SEEK_END);
        size_t fileSize = ftell(dataFile);
        fseek(dataFile, 0L, SEEK_SET);

        if (fileSize > 0)
        {
            void* fileData = heap_caps_malloc(fileSize, MALLOC_CAP_8BIT);
            if (fileData != NULL)
            {
                if (fileSize == fread(fileData, 1, fileSize, dataFile))
                {
                    fclose(dataFile);
                    emuCnfsInjectFileData(name, fileSize, fileData);
                    return true;
                }
            }
        }

        fclose(dataFile);
        return false;
    }
    else
    {
        printf("ERR: Could not open %s\n", filePath);
        return false;
    }
}

void emuCnfsInjectFileData(const char* name, size_t length, void* data)
{
    cnfsInjectedFilename = strdup(name);
    cnfsInjectedFileSize = length;
    cnfsInjectedFileData = data;
}

bool deinitCnfs(void)
{
    free(cnfsInjectedFilename);
    free(cnfsInjectedFileData);

    cnfsInjectedFileSize = 0;
    cnfsInjectedFilename = NULL;
    cnfsInjectedFileData = NULL;

    return true;
}

const uint8_t* cnfsGetFile(cnfsFileIdx_t fIdx, size_t* flen)
{
    if (cnfsInjectedFilename && fIdx >= CNFS_NUM_FILES)
    {
        *flen = cnfsInjectedFileSize;
        return (uint8_t*)cnfsInjectedFileData;
    }
    else
    {
        // Real implementation - copied from cnfs.c
        if (0 <= fIdx && fIdx < CNFS_NUM_FILES)
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
}

/**
 * @brief Return the type of a file
 *
 * @param fIdx the cnfsFileIdx_t of the file to get the type of
 * @return The cnfsFileType_t for that file extension, or CNFS_NUM_TYPES if the file is not found
 */
cnfsFileType_t cnfsGetFileType(cnfsFileIdx_t fIdx)
{
    if (0 <= fIdx && fIdx < CNFS_NUM_FILES)
    {
        return cnfsFiles[fIdx].type;
    }
    else
    {
        return CNFS_NUM_TYPES;
    }
}

/**
 * @brief Return the index of the next file matching the type which is greater than the given one
 *
 * @param after The index before the first one to search, or CNFS_NUM_FILES to search the beginning
 * @param type The type of file to search for
 * @return cnfsFileIdx_t The index of the next file of that type, or CNFS_NUM_FILES if no more were found
 */
cnfsFileIdx_t cnfsFindNextFileOfType(cnfsFileIdx_t after, cnfsFileType_t type)
{
    if (type >= CNFS_NUM_TYPES)
    {
        // Short-circuit on a nonexistent type
        return CNFS_NUM_FILES;
    }

    int cur = after;
    if (cur >= CNFS_NUM_FILES)
    {
        // This is the first call, so search from the beginning
        cur = 0;
    }
    else
    {
        // Move to the next one before searching!
        cur++;
    }

    while (cur < CNFS_NUM_FILES)
    {
        if (cnfsFiles[cur].type == type)
        {
            return (cnfsFileIdx_t)cur;
        }

        cur++;
    }

    // We must not have found it
    return CNFS_NUM_FILES;
}

// Hack needed because we can't actually wrap the call that cnfsReadFile() makes to cnfsGetFile() because of compiler
// shenanigans
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
