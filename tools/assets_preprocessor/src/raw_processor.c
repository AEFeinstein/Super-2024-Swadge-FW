#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "fileUtils.h"
#include "heatshrink_util.h"

#include "raw_processor.h"

bool process_raw(const char* inFile, const char* outFilePath)
{
    // Read input file
    const char* errdesc = NULL;
    errno               = 0;
    FILE* fp            = fopen(inFile, "rb");
    if (!fp)
    {
        errdesc = strerror(errno);
        fprintf(stderr, "ERR: raw_processor.c: Failed to open file %s: %d - %s\n", inFile, errno, errdesc);
        return false;
    }

    fseek(fp, 0L, SEEK_END);
    long sz = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    uint8_t* byteString = malloc(sz + 1);
    if (!byteString)
    {
        fprintf(stderr, "ERR: raw_processor.c: Failed to allocate memory processing file %s\n", inFile);
        fclose(fp);
        return false;
    }

    errno       = 0;
    int readlen = fread(byteString, 1, sz, fp);
    if (readlen < sz)
    {
        errdesc = (errno == 0) ? "Read too small" : strerror(errno);
        fprintf(stderr, "ERR: raw_processor.c: Failed to read file %s: %d - %s\n", inFile, readlen,
                errdesc ? errdesc : "Unknown");

        free(byteString);
        fclose(fp);
        return false;
    }
    byteString[sz] = 0;
    fclose(fp);

    // Write the compressed bytes to a file
    if (!writeHeatshrinkFile(byteString, sz, outFilePath))
    {
        free(byteString);
        return false;
    }

    // Cleanup
    free(byteString);

    return true;
}