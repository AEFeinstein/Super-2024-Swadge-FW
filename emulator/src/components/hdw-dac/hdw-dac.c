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
    if (dacCb)
    {
        // TODO
        uint8_t samps[2048];
        dacCb(samps, 2048);
    }
}

/**
 * @brief TODO
 *
 * @param out
 * @param framesp
 */
void dacHandleSoundOutput(short* out, int framesp)
{
    if (NULL != out)
    {
        if (NULL != dacCb)
        {
            // Get samples from the Swadge mode
            uint8_t tempSamps[framesp];
            dacCb(tempSamps, framesp);

            // Write the samples to the emulator output, in signed short format
            for (int i = 0; i < framesp; i++)
            {
                short samp     = (tempSamps[i] - 127) * 256;
                out[i * 2]     = samp;
                out[i * 2 + 1] = samp;
            }
        }
        else
        {
            // Write zeros
            memset(out, 0, sizeof(short) * 2 * framesp);
        }
    }
}
