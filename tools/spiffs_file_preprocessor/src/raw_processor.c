#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "fileUtils.h"
#include "heatshrink_util.h"

#include "raw_processor.h"

void process_raw(const char* inFile, const char* outDir, const char* outExt)
{
    // Determine if the output file already exists
    char outFilePath[128] = {0};
    strcat(outFilePath, outDir);
    strcat(outFilePath, "/");
    strcat(outFilePath, get_filename(inFile));

    // Change the file extension
    char* dotPtr = strrchr(outFilePath, '.');
    strncpy(&dotPtr[1], outExt, sizeof(outFilePath) - (dotPtr - outFilePath) - 1);

    if (doesFileExist(outFilePath))
    {
        // printf("Output for %s already exists\n", inFile);
        return;
    }

    // Read input file
    FILE* fp = fopen(inFile, "rb");
    fseek(fp, 0L, SEEK_END);
    long sz = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    uint8_t* byteString = malloc(sz + 1);
    if (!byteString)
    {
        fprintf(stderr, "ERR: raw_processor.c: Failed to allocate memory processing file %s\n", inFile);
        fclose(fp);
        return;
    }

    errno = 0;
    if (sz > fread(byteString, sz, 1, fp))
    {
        const char* errdesc = (errno == 0) ? "Read too small" : strerror(errno);
        fprintf(stderr, "ERR: raw_processor.c: Failed to read file %s: %s\n", inFile, errdesc ? errdesc : "Unknown");

        free(byteString);
        fclose(fp);
        return;
    }
    byteString[sz] = 0;
    fclose(fp);

    // Write the compressed bytes to a file
    writeHeatshrinkFile(byteString, sz, outFilePath);

    // Cleanup
    free(byteString);
}