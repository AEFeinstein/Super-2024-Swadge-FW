//==============================================================================
// Includes
//==============================================================================

#include <stddef.h>
#include <string.h>
#include <esp_log.h>
#include "hdw-dac.h"
#include "hdw-dac_emu.h"
#include "emu_main.h"

//==============================================================================
// Defines
//==============================================================================

//==============================================================================
// Variables
//==============================================================================

static const char DAC_TAG[] = "DAC";

static fnDacCallback_t dacCb = NULL;
static bool shutdownState    = false;
static bool dacWriting       = false;

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
    ESP_LOGI(DAC_TAG, " ");
    dacCb         = cb;
    shutdownState = false;
    dacWriting    = true;
}

/**
 * @brief TODO
 *
 */
void deinitDac(void)
{
    ESP_LOGI(DAC_TAG, " ");
    shutdownState = true;
    dacWriting    = false;
}

/**
 * @brief
 */
void powerDownDac(void)
{
    ESP_LOGI(DAC_TAG, " ");
    setDacShutdown(true);
}

/**
 * @brief
 */
void powerUpDac(void)
{
    ESP_LOGI(DAC_TAG, " ");
    setDacShutdown(false);
}

/**
 * @brief TODO
 *
 */
void dacStart(void)
{
    ESP_LOGI(DAC_TAG, " ");
    dacWriting = true;
}

/**
 * @brief TODO
 *
 */
void dacStop(void)
{
    ESP_LOGI(DAC_TAG, " ");
    dacWriting = false;
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
        if (NULL != dacCb && dacWriting)
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
                    if (shutdownState)
                    {
                        out[i * numChannels + j] = 0;
                    }
                    else
                    {
                        out[i * numChannels + j] = samp;
                    }
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

/**
 * @brief Set the shutdown state of the DAC
 *
 * @param shutdown true to shut down the DAC, false to enable it
 */
void setDacShutdown(bool shutdown)
{
    ESP_LOGI(DAC_TAG, " ");
    shutdownState = shutdown;
    if (shutdown)
    {
        dacStop();
    }
    else
    {
        dacStart();
    }
}
