#include <stddef.h>

#include <esp_log.h>
#include <esp_heap_caps.h>
#include <inttypes.h>
#include <esp_err.h>

#include "cnfs.h"
#include "hdw-nvs.h"
#include <nvs.h>

#include "heatshrink_helper.h"

/**
 * @brief Read a heatshrink compressed file from the filesystem into an output array.
 * Files that are in the assets_image folder before compilation and flashing
 * will automatically be included in the firmware.
 *
 * You must provide a decoder and decode space for this function
 *
 * @param fIdx    The CNFS index of the file to load
 * @param outsize A pointer to a size_t to return how much data was read
 * @param decompressedBuf Memory to store decoded data. This must be as large as the decoded data
 * @param hsd A heatshrink decoder
 * @return A pointer to the read data if successful, or NULL if there is a failure
 *         This data must be freed when done
 */
uint8_t* readHeatshrinkFileInplace(cnfsFileIdx_t fIdx, uint32_t* outsize, uint8_t* decompressedBuf,
                                   heatshrink_decoder* hsd)
{
    // Read WSG from file
    size_t sz;
    const uint8_t* buf = cnfsGetFile(fIdx, &sz);
    if (NULL == buf)
    {
        ESP_LOGE("WSG", "Failed to read %d", fIdx);
        (*outsize) = 0;
        return NULL;
    }

    // Pick out the decompressed size and create a space for it
    (*outsize) = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | (buf[3]);

    // Create the decoder
    size_t copied = 0;
    heatshrink_decoder_reset(hsd);

    // The decompressed filesize is four bytes, so start after that
    uint32_t inputIdx  = 4;
    uint32_t outputIdx = 0;
    // Decode the file in chunks
    while (inputIdx < sz)
    {
        // Decode some data
        copied = 0;
        heatshrink_decoder_sink(hsd, &buf[inputIdx], sz - inputIdx, &copied);
        inputIdx += copied;

        if (copied == 0)
        {
            ESP_LOGE("WSG", "Failed to read %d fault on decode", fIdx);
            heatshrink_decoder_finish(hsd);
            return 0;
        }

        // Save it to the output array
        copied = 0;
        heatshrink_decoder_poll(hsd, &decompressedBuf[outputIdx], (*outsize) - outputIdx, &copied);
        outputIdx += copied;
    }

    // Note that it's all done
    heatshrink_decoder_finish(hsd);

    // Flush any final output
    copied = 0;
    heatshrink_decoder_poll(hsd, &decompressedBuf[outputIdx], (*outsize) - outputIdx, &copied);
    outputIdx += copied;

    // All done decoding
    heatshrink_decoder_finish(hsd);

    // Return the decompressed bytes
    return decompressedBuf;
}

/**
 * @brief Read a heatshrink compressed file from the filesystem into an output array.
 * Files that are in the assets_image folder before compilation and flashing
 * will automatically be included in the firmware.
 *
 * @param fIdx    The CNFS index of the file to load
 * @param outsize A pointer to a size_t to return how much data was read
 * @param readToSpiRam true to use SPI RAM, false to use normal RAM
 * @return A pointer to the read data if successful, or NULL if there is a failure
 *         This data must be freed when done
 */
uint8_t* readHeatshrinkFile(cnfsFileIdx_t fIdx, uint32_t* outsize, bool readToSpiRam)
{
    // Read WSG from file
    size_t sz;
    const uint8_t* buf = cnfsGetFile(fIdx, &sz);
    if (NULL == buf)
    {
        ESP_LOGE("WSG", "Failed to read %d", fIdx);
        (*outsize) = 0;
        return NULL;
    }

    // Pick out the decompressed size and create a space for it
    int32_t decompressedSize = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | (buf[3]);
    uint8_t* decompressedBuf;
    if (readToSpiRam)
    {
        decompressedBuf = (uint8_t*)heap_caps_malloc(decompressedSize, MALLOC_CAP_SPIRAM);
    }
    else
    {
        decompressedBuf = (uint8_t*)heap_caps_malloc(decompressedSize, MALLOC_CAP_8BIT);
    }

    // Allocate the decoder
    heatshrink_decoder* hsd = heatshrink_decoder_alloc(256, 8, 4);

    // Decode the file
    uint8_t* data = readHeatshrinkFileInplace(fIdx, outsize, decompressedBuf, hsd);

    // Free the decoder
    heatshrink_decoder_free(hsd);

    // If there was an error, free decompressedBuf
    if (NULL == data)
    {
        heap_caps_free(decompressedBuf);
    }

    // Return the data
    return data;
}

uint8_t* readHeatshrinkNvs(const char* namespace, const char* key, uint32_t* outsize, bool spiRam)
{
    // Read WSG from NVS
    size_t sz;

    // Get full size
    readNamespaceNvsBlob(namespace, key, NULL, &sz);

    ESP_LOGD("Heatshrink", "Compressed size is %" PRIu64, (uint64_t)sz);

    uint8_t* buf = (uint8_t*)heap_caps_malloc(sz, spiRam ? MALLOC_CAP_SPIRAM : MALLOC_CAP_8BIT);
    if (!buf)
    {
        return NULL;
    }

    if (!readNamespaceNvsBlob(namespace, key, buf, &sz))
    {
        heap_caps_free(buf);
        return NULL;
    }

    // Pick out the decompresed size and create a space for it
    (*outsize) = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | (buf[3]);

    uint8_t* decompressedBuf = (uint8_t*)heap_caps_malloc((*outsize), spiRam ? MALLOC_CAP_SPIRAM : MALLOC_CAP_8BIT);

    // Create the decoder
    size_t copied           = 0;
    heatshrink_decoder* hsd = heatshrink_decoder_alloc(256, 8, 4);
    heatshrink_decoder_reset(hsd);

    // The decompressed filesize is four bytes, so start after that
    uint32_t inputIdx  = 4;
    uint32_t outputIdx = 0;
    // Decode the file in chunks
    while (inputIdx < sz)
    {
        // Decode some data
        copied = 0;
        heatshrink_decoder_sink(hsd, &buf[inputIdx], sz - inputIdx, &copied);
        inputIdx += copied;

        // Save it to the output array
        copied = 0;
        heatshrink_decoder_poll(hsd, &decompressedBuf[outputIdx], (*outsize) - outputIdx, &copied);
        outputIdx += copied;
    }

    // Note that it's all done
    heatshrink_decoder_finish(hsd);

    // Flush any final output
    copied = 0;
    heatshrink_decoder_poll(hsd, &decompressedBuf[outputIdx], (*outsize) - outputIdx, &copied);
    outputIdx += copied;

    // All done decoding
    heatshrink_decoder_finish(hsd);
    heatshrink_decoder_free(hsd);
    // Free the bytes read from the file
    heap_caps_free(buf);

    // Return the decompressed bytes
    return decompressedBuf;
}

uint32_t heatshrinkCompress(uint8_t* dest, const uint8_t* src, uint32_t size)
{
    heatshrink_encoder* hse = heatshrink_encoder_alloc(8, 4);
    heatshrink_encoder_reset(hse);

    size_t inputIdx = 0;
    // Start writing past the header first
    size_t outputIdx = 4;
    size_t copied;

    while (inputIdx < size)
    {
        copied = 0;
        HSE_sink_res result;

        result = heatshrink_encoder_sink(hse, &src[inputIdx], size - inputIdx, &copied);
        inputIdx += copied;

        switch (result)
        {
            case HSER_SINK_OK:
            {
                // Continue
                break;
            }

            case HSER_SINK_ERROR_NULL:
            case HSER_SINK_ERROR_MISUSE:
            {
                goto heatshrink_error;
            }
        }

        bool stillEncoding = true;
        while (stillEncoding)
        {
            copied = 0;
            switch (heatshrink_encoder_poll(hse, &dest[outputIdx], size - outputIdx, &copied))
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
                    goto heatshrink_error;
                }
            }
            if (outputIdx > inputIdx)
            {
                ESP_LOGE("WSG", "Failed to compress buffer with heatshrink, output takes up more space than input");
                goto heatshrink_error;
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
                switch (heatshrink_encoder_poll(hse, &dest[outputIdx], size - outputIdx, &copied))
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
            goto heatshrink_error;
        }
    }

    ESP_LOGD("WSG", "Compressed size is %" PRIu32, (uint32_t)outputIdx);

    heatshrink_encoder_free(hse);
    dest[0] = (size >> 24) & 0xFF;
    dest[1] = (size >> 16) & 0xFF;
    dest[2] = (size >> 8) & 0xFF;
    dest[3] = (size >> 0) & 0xFF;
    return (uint32_t)outputIdx;

heatshrink_error:
    if (NULL != hse)
    {
        heatshrink_encoder_free(hse);
    }

    return 0;
}

/**
 * @brief Get the size of and decompress heatshrink data
 *
 * @param dest
 * @param destSize
 * @param source
 * @param sourceSize
 * @return true
 * @return false
 */
bool heatshrinkDecompress(uint8_t* dest, uint32_t* destSize, const uint8_t* source, uint32_t sourceSize)
{
    // This is a multi-step process...
    // 1. Call with NULL dest and non-null destSize -- this will set *destSize to the actual size
    // 2. Call again with non-null dest of at least (*destSize) bytes, and also the same destSize
    // -- It should be possible to pass both a non-null destSize and non-null dest, as long as it's big enough
    bool sizeRead = false;

    // Can't decompress if the heatshrnk header doesn't even fit
    if (sourceSize < 4)
    {
        return false;
    }

    // Write the destSize
    if (destSize)
    {
        (*destSize) = (source[0] << 24) | (source[1] << 16) | (source[2] << 8) | (source[3]);
        sizeRead    = true;
    }

    // Write the actual data
    if (dest)
    {
        // Create the decoder
        size_t copied           = 0;
        heatshrink_decoder* hsd = heatshrink_decoder_alloc(256, 8, 4);
        heatshrink_decoder_reset(hsd);

        // The decompressed filesize is four bytes, so start after that
        uint32_t inputIdx  = 4;
        uint32_t outputIdx = 0;
        // Decode the file in chunks
        while (inputIdx < sourceSize)
        {
            // Decode some data
            copied = 0;
            heatshrink_decoder_sink(hsd, &source[inputIdx], sourceSize - inputIdx, &copied);
            inputIdx += copied;

            if (copied == 0)
            {
                ESP_LOGE("WSG", "Failed to decompress heatshrink buffer -- fault on decode");
                heatshrink_decoder_finish(hsd);
                heatshrink_decoder_free(hsd);
                return false;
            }

            // Save it to the output array
            copied = 0;
            heatshrink_decoder_poll(hsd, &dest[outputIdx], (*destSize) - outputIdx, &copied);
            outputIdx += copied;
        }

        // Note that it's all done
        heatshrink_decoder_finish(hsd);

        // Flush any final output
        copied = 0;
        heatshrink_decoder_poll(hsd, &dest[outputIdx], (*destSize) - outputIdx, &copied);
        outputIdx += copied;

        // All done decoding
        heatshrink_decoder_finish(hsd);
        heatshrink_decoder_free(hsd);

        return true;
    }

    return sizeRead;
}
