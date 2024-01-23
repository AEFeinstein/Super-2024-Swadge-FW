#include <stdio.h>
#include <stdlib.h>

#include "fileUtils.h"
#include "heatshrink_encoder.h"
#include "heatshrink_util.h"

/**
 * @brief Utility to compress the given bytes and write them to a file
 *
 * @param input The bytes to compress and write to a file
 * @param len The length of the bytes to compress and write
 * @param outFilePath The filename to write to
 */
void writeHeatshrinkFile(uint8_t* input, uint32_t len, const char* outFilePath)
{
    int32_t errLine = -1;
    /* Set up variables for compression */
    uint32_t outputSize = len;
    uint8_t* output     = calloc(1, outputSize);
    uint32_t outputIdx  = 0;
    uint32_t inputIdx   = 0;
    size_t copied       = 0;
    FILE* shrunkFile    = NULL;

    /* Creete the encoder */
    heatshrink_encoder* hse = heatshrink_encoder_alloc(8, 4);
    heatshrink_encoder_reset(hse);

    /* Stream the data in chunks */
    while (inputIdx < len)
    {
        /* Pass bytes to the encoder for compression */
        copied = 0;
        switch (heatshrink_encoder_sink(hse, &input[inputIdx], len - inputIdx, &copied))
        {
            case HSER_SINK_OK:
            {
                // Continue
                break;
            }
            case HSER_SINK_ERROR_NULL:
            case HSER_SINK_ERROR_MISUSE:
            {
                // Error handling
                errLine = __LINE__;
                goto heatshrink_error;
            }
        }
        inputIdx += copied;

        /* Save compressed data */
        copied             = 0;
        bool stillEncoding = true;
        while (stillEncoding)
        {
            switch (heatshrink_encoder_poll(hse, &output[outputIdx], outputSize - outputIdx, &copied))
            {
                case HSER_POLL_EMPTY:
                {
                    stillEncoding = false;
                    break;
                }
                case HSER_POLL_MORE:
                {
                    stillEncoding = true;
                    break;
                }
                case HSER_POLL_ERROR_NULL:
                case HSER_POLL_ERROR_MISUSE:
                {
                    // Error handling
                    errLine = __LINE__;
                    goto heatshrink_error;
                }
            }
            outputIdx += copied;
        }
    }

    /* Mark all input as processed */
    switch (heatshrink_encoder_finish(hse))
    {
        case HSER_FINISH_DONE:
        {
            // Continue
            break;
        }
        case HSER_FINISH_MORE:
        {
            /* Flush the last bits of output */
            copied             = 0;
            bool stillEncoding = true;
            while (stillEncoding)
            {
                switch (heatshrink_encoder_poll(hse, &output[outputIdx], outputSize - outputIdx, &copied))
                {
                    case HSER_POLL_EMPTY:
                    {
                        stillEncoding = false;
                        break;
                    }
                    case HSER_POLL_MORE:
                    {
                        stillEncoding = true;
                        break;
                    }
                    case HSER_POLL_ERROR_NULL:
                    case HSER_POLL_ERROR_MISUSE:
                    {
                        // Error handling
                        errLine = __LINE__;
                        goto heatshrink_error;
                    }
                }
                outputIdx += copied;
            }
            break;
        }
        case HSER_FINISH_ERROR_NULL:
        {
            // Error handling
            errLine = __LINE__;
            goto heatshrink_error;
        }
    }

    /* Write a compressed file */
    shrunkFile = fopen(outFilePath, "wb");
    if (shrunkFile == NULL) {
        perror("Error occurred while writing file.\n");
    }
    /* First four bytes are decompresed size */
    putc(HI_BYTE(HI_WORD(len)), shrunkFile);
    putc(LO_BYTE(HI_WORD(len)), shrunkFile);
    putc(HI_BYTE(LO_WORD(len)), shrunkFile);
    putc(LO_BYTE(LO_WORD(len)), shrunkFile);
    /* Then dump the compressed bytes */
    fwrite(output, outputIdx, 1, shrunkFile);
    /* Done writing to the file */
    fclose(shrunkFile);

    /* Print results */
    // printf("  Source length: %d\n  Shrunk length: %d\n", inputIdx, 4 + outputIdx);

    /* Error handling and cleanup */
heatshrink_error:
    if (NULL != output)
    {
        free(output);
    }
    if (NULL != hse)
    {
        heatshrink_encoder_free(hse);
    }
    if (-1 != errLine)
    {
        fprintf(stderr, "[%d]: Heatshrink error\n", errLine);
    }
}
