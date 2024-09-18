#include "emu_cnfs.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "esp_heap_caps.h"

static char* cnfsInjectedFilename   = NULL;
static int32_t cnfsInjectedFileSize = 0;
static void* cnfsInjectedFileData   = NULL;

const uint8_t* __real_cnfsGetFile(const char* fname, size_t* flen);
bool __real_deinitCnfs(void);

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
            void* fileData = malloc(fileSize);
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

bool __wrap_deinitCnfs(void)
{
    free(cnfsInjectedFilename);
    free(cnfsInjectedFileData);

    cnfsInjectedFileSize = 0;
    cnfsInjectedFilename = NULL;
    cnfsInjectedFileData = NULL;

    return __real_deinitCnfs();
}

const uint8_t* __wrap_cnfsGetFile(const char* fname, size_t* flen)
{
    if (cnfsInjectedFilename && !strcmp(cnfsInjectedFilename, fname))
    {
        *flen = cnfsInjectedFileSize;
        return (uint8_t*)cnfsInjectedFileData;
    }
    else
    {
        return __real_cnfsGetFile(fname, flen);
    }
}

// Hack needed because we can't actually wrap the call that cnfsReadFile() makes to cnfsGetFile() because of compiler
// shenanigans
uint8_t* __wrap_cnfsReadFile(const char* fname, size_t* outsize, bool readToSpiRam)
{
    const uint8_t* fptr = __wrap_cnfsGetFile(fname, outsize);

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
        output = (uint8_t*)calloc((*outsize + 1), sizeof(uint8_t));
    }
    memcpy(output, fptr, *outsize);
    return output;
}
