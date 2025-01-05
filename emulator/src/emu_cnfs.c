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
static int32_t cnfsNumFiles;

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
    cnfsData     = getCnfsImage();
    cnfsDataSz   = getCnfsSize();
    cnfsFiles    = getCnfsFiles();
    cnfsNumFiles = getCnfsNumFiles();

    /* Debug print */
    ESP_LOGI("CNFS", "Size: %" PRIu32 ", Files: %" PRIu32, cnfsDataSz, cnfsNumFiles);
    return (0 != cnfsDataSz) && (0 != cnfsNumFiles);
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

const uint8_t* cnfsGetFile(const char* fname, size_t* flen)
{
    if (cnfsInjectedFilename && !strcmp(cnfsInjectedFilename, fname))
    {
        *flen = cnfsInjectedFileSize;
        return (uint8_t*)cnfsInjectedFileData;
    }
    else
    {
        // Real implementation - copied from cnfs.c
        int low  = 0;
        int high = cnfsNumFiles - 1;
        int mid  = (low + high) / 2;

        // Binary search the file list, since it's sorted.
        while (low <= high)
        {
            const cnfsFileEntry* e = cnfsFiles + mid;
            int sc                 = strcmp(e->name, fname);
            if (sc < 0)
            {
                low = mid + 1;
            }
            else if (sc == 0)
            {
                *flen = e->len;
                return &cnfsData[e->offset];
            }
            else
            {
                high = mid - 1;
            }
            mid = (low + high) / 2;
        }
        ESP_LOGE("CNFS", "Failed to open %s", fname);
        return 0;
    }
}

// Hack needed because we can't actually wrap the call that cnfsReadFile() makes to cnfsGetFile() because of compiler
// shenanigans
uint8_t* cnfsReadFile(const char* fname, size_t* outsize, bool readToSpiRam)
{
    const uint8_t* fptr = cnfsGetFile(fname, outsize);

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
