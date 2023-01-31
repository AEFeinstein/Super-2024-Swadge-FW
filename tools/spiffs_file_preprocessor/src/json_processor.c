#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "json_processor.h"
#include "cJSON.h"
#include "heatshrink_encoder.h"
#include "fileUtils.h"

void process_json(const char* infile, const char* outdir)
{
    /* Determine if the output file already exists */
    char outFilePath[128] = {0};
    strcat(outFilePath, outdir);
    strcat(outFilePath, "/");
    strcat(outFilePath, get_filename(infile));

#ifdef JSON_COMPRESSION
    /* Change the file extension */
    char* dotptr = strrchr(outFilePath, '.');
    dotptr[1]    = 'h';
    dotptr[2]    = 'o';
    dotptr[3]    = 'n';
    dotptr[4]    = 0;
#endif

    // if(doesFileExist(outFilePath))
    // {
    //     printf("Output for %s already exists\n", infile);
    //     return;
    // }

    /* Read input file */
    FILE* fp = fopen(infile, "rb");
    fseek(fp, 0L, SEEK_END);
    long sz = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    char jsonInStr[sz + 1];
    fread(jsonInStr, sz, 1, fp);
    jsonInStr[sz] = 0;
    fclose(fp);

#ifndef JSON_COMPRESSION
    /* Write input directly to output */
    FILE* outFile = fopen(outFilePath, "wb");
    fwrite(jsonInStr, sz, 1, outFile);
    fclose(outFile);
#else
    /* Minify input file */
    cJSON* jsonIn           = cJSON_Parse(jsonInStr);
    char* jsonInUnformatted = cJSON_PrintUnformatted(jsonIn);
    cJSON_free(jsonIn);

    /* Compress the JSON */
    uint8_t output[strlen(jsonInUnformatted) + 1];
    uint32_t outputIdx = 0;
    uint32_t inputIdx  = 0;
    size_t copied      = 0;

    /* Create the encoder */
    heatshrink_encoder* hse = heatshrink_encoder_alloc(8, 4);
    heatshrink_encoder_reset(hse);

    /* Stream the data in chunks */
    while (inputIdx < strlen(jsonInUnformatted) + 1)
    {
        /* Pass chars to the encoder for compression */
        copied = 0;
        heatshrink_encoder_sink(hse, (uint8_t*)(&jsonInUnformatted[inputIdx]), strlen(jsonInUnformatted) + 1 - inputIdx,
                                &copied);
        inputIdx += copied;

        /* Save compressed data */
        copied = 0;
        heatshrink_encoder_poll(hse, &output[outputIdx], sizeof(output) - outputIdx, &copied);
        outputIdx += copied;
    }

    /* Mark all input as processed */
    heatshrink_encoder_finish(hse);

    /* Flush the last bits of output */
    copied = 0;
    heatshrink_encoder_poll(hse, &output[outputIdx], sizeof(output) - outputIdx, &copied);
    outputIdx += copied;

    /* Free the input */
    cJSON_free(jsonInUnformatted);

    /* Free the encoder */
    heatshrink_encoder_free(hse);

    /* Write a Heatshrink Object Notation file */
    FILE* honFile = fopen(outFilePath, "wb");
    /* Write uncompressed size first */
    putc(HI_BYTE(inputIdx), honFile);
    putc(LO_BYTE(inputIdx), honFile);
    /* Write compressed bytes */
    fwrite(output, outputIdx, 1, honFile);
    fclose(honFile);

    /* Print results */
    printf("%s:\n  Source file size: %d\n  WSG   file size: %d\n", infile, inputIdx, outputIdx);
#endif
}