#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fileUtils.h"

#include "bin_processor.h"

void process_bin(const char* infile, const char* outdir)
{
    /* Determine if the output file already exists */
    char outFilePath[128] = {0};
    strcat(outFilePath, outdir);
    strcat(outFilePath, "/");
    strcat(outFilePath, get_filename(infile));

    if (!isSourceFileNewer(infile, outFilePath))
    {
        return;
    }
    else if (doesFileExist(outFilePath))
    {
        printf("[assets-preprocessor] %s modified! Regenerating %s\n", infile, get_filename(outFilePath));
    }

    /* Read input file */
    FILE* fp = fopen(infile, "rb");
    fseek(fp, 0L, SEEK_END);
    long sz = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    char byteString[sz + 1];
    fread(byteString, sz, 1, fp);
    byteString[sz] = 0;
    fclose(fp);

    /* Write input directly to output */
    FILE* outFile = fopen(outFilePath, "wb");
    fwrite(byteString, sz, 1, outFile);
    fclose(outFile);
}