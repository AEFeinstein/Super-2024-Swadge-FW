//==============================================================================
// Includes
//==============================================================================

#include <stddef.h>
#include <string.h>
#include "hdw-dac.h"
#include "hdw-dac_emu.h"
#include "circular_buffer.h"

//==============================================================================
// Defines
//==============================================================================

/// The number of DAC-buffer-fuls (DAC_BUF_SIZE) to keep in the buffer
#define CIRCULAR_BUF_SIZE_MULT 4

// The threshold of DAC-buffer-fuls at which the circular buffer refills all the way
#define CIRCULAR_BUF_MIN_AVAIL 3

//==============================================================================
// Variables
//==============================================================================

fnDacCallback_t dacCb = NULL;
static bool shdn      = false;

static int playChannels = 0;
static int recChannels  = 0;

// Circular buffer for audio data
circularBuffer_t circBuf = {0};

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initialize the DAC
 *
 * @param channel
 * @param shdn_gpio
 * @param cb
 */
void initDac(dac_channel_mask_t channel, gpio_num_t shdn_gpio, fnDacCallback_t cb)
{
    // TODO
    dacCb = cb;
}

/**
 * @brief TODO
 *
 */
void deinitDac(void)
{
}

/**
 * @brief TODO
 *
 */
void dacStart(void)
{
    // TODO
}

/**
 * @brief TODO
 *
 */
void dacStop(void)
{
    // TODO
}

/**
 * @brief Poll the queue to see if it needs to be filled with audio samples
 */
void dacPoll(void)
{
    // In actual firmware, this function will fill sample buffers received in interrupts.
    // In the emulator, that's handled in dacHandleSoundOutput() instead

    // Check whether we have any space in the buffer
    // If we do, call the dacCb to fill it
    if (NULL != dacCb && playChannels > 0 && !shdn)
    {
        size_t avail = circularBufferAvailable(&circBuf);
        // Go one dac buffer size at a time
        uint8_t tempSamps[DAC_BUF_SIZE];

        if (avail < (CIRCULAR_BUF_MIN_AVAIL * DAC_BUF_SIZE))
        {
            // Circular buffer is less than 50% full, call the Dac until it's full
            while (circularBufferCapacity(&circBuf) > DAC_BUF_SIZE)
            {
                dacCb(tempSamps, DAC_BUF_SIZE);
                circularBufferWrite(&circBuf, tempSamps, DAC_BUF_SIZE);
            }
        }
    }
}

/**
 * @brief Initialize the emulator DAC interface
 *
 * @param numChannelsRec The number of channels for recording
 * @param numChannelsPlay The number of channels for playback
 */
void dacInitSoundOutput(int numChannelsRec, int numChannelsPlay)
{
    recChannels  = numChannelsRec;
    playChannels = numChannelsPlay;

    // Store several DAC-buffer-sizes worth of data
    circularBufferInit(&circBuf, 1, DAC_BUF_SIZE * CIRCULAR_BUF_SIZE_MULT);
}

/**
 * @brief Fill a buffer with sample output
 *
 * @param out     A pointer to fill with samples
 * @param framesp The number of samples to fill, per-channel
 * @param numChannels The number of channels to write to. Channels are interleaved in \c out
 */
void dacHandleSoundOutput(short* out, int framesp, short numChannels)
{
    // Make sure there is a buffer to fill
    if (NULL != out && circBuf.buffer)
    {
        uint8_t tempSamps[framesp];
        size_t read = circularBufferRead(&circBuf, tempSamps, framesp);

        // Write the samples to the emulator output, in signed short format
        uint32_t i;
        for (i = 0; i < read; i++)
        {
            short samp = (tempSamps[i] - 127) * 256;
            // Copy the same sample to each channel
            for (int j = 0; j < numChannels; j++)
            {
                if (shdn)
                {
                    out[i * numChannels + j] = 0;
                }
                else
                {
                    out[i * numChannels + j] = samp;
                }
            }
        }

        if (i < framesp)
        {
            memset(out + read * numChannels, 0, (framesp - read) * sizeof(short) * numChannels);
        }
    }
}

/**
 * @brief Set the shutdown state of the DAC
 *
 * @param shutdown true to shut down the DAC, false to enable it
 */
void setDacShutdown(bool shutdown)
{
    shdn = shutdown;
}
