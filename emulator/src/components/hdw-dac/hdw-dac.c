//==============================================================================
// Includes
//==============================================================================

#include <stddef.h>
#include <string.h>
#include "hdw-dac.h"
#include "hdw-dac_emu.h"

//==============================================================================
// Defines
//==============================================================================

//==============================================================================
// Variables
//==============================================================================

fnDacCallback_t dacCb = NULL;

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initialize the DAC
 *
 * @param cb
 */
void initDac(fnDacCallback_t cb)
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
    if (NULL != out)
    {
        // Make sure there is a callback function to call
        if (NULL != dacCb)
        {
            // Get samples from the Swadge mode
            uint8_t tempSamps[framesp];
            dacCb(tempSamps, framesp);

            // Write the samples to the emulator output, in signed short format
            for (int i = 0; i < framesp; i++)
            {
                short samp = (tempSamps[i] - 127) * 256;
                // Copy the same sample to each channel
                for (int j = 0; j < numChannels; j++)
                {
                    out[i * numChannels + j] = samp;                    
                }
            }
        }
        else
        {
            // No callback function, write zeros
            memset(out, 0, sizeof(short) * 2 * framesp);
        }
    }
}
