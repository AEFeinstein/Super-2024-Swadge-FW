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
