//==============================================================================
// Includes
//==============================================================================

#include "hdw-mic.h"
#include "hdw-mic_emu.h"
#include "emu_main.h"

//==============================================================================
// Defines
//==============================================================================

#define SSBUF 8192

//==============================================================================
// Variables
//==============================================================================

// Input sample circular buffer
static uint16_t ssamples[SSBUF] = {0};
static int sshead               = 0;
static int sstail               = 0;
static bool adcSampling         = false;

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initialize the ADC which continuously samples the microphone
 *
 * This does not start sampling, so startMic() must be called afterwards.
 *
 * @param gpio The GPIO the microphone is attached to
 */
void initMic(gpio_num_t gpio)
{
    // Nothing to do here, emulator sound is initialized in initBuzzer()
}

/**
 * @brief Start sampling the microphone's ADC
 */
void startMic(void)
{
    adcSampling = true;
}

/**
 * @brief Attempt to read a block of 12-bit samples from the ADC in continuous mode.
 * This may return fewer than expected samples (or zero samples) if the task rate is faster than the sampling rate.
 *
 * @param[out] outSamples A pointer to write 12-bit samples from the ADC
 * @param[in] outSamplesMax The maximum number of samples that can be written to outSamples
 * @return The number of samples which were actually written to outSamples
 */
uint32_t loopMic(uint16_t* outSamples, uint32_t outSamplesMax)
{
    uint32_t samplesRead = 0;
    while (adcSampling && (sshead != sstail) && samplesRead < outSamplesMax)
    {
        *(outSamples++) = ssamples[sstail];
        sstail          = (sstail + 1) % SSBUF;
        samplesRead++;
    }
    return samplesRead;
}

/**
 * @brief Stop sampling the microphone's ADC
 */
void stopMic(void)
{
    adcSampling = false;
}

/**
 * @brief Deinitialize the ADC which continuously samples the microphone
 */
void deinitMic(void)
{
    // Nothing to do here, emulator sound is deinitialized in deinitBuzzer()
}

/**
 * @brief Callback for sound events, both input and output
 * Only handle input here
 *
 * @param in A pointer to read samples from. May be NULL
 * @param framesr The number of samples to read
 * @param numChannels The number of channels to read
 */
void micHandleSoundInput(short* in, int framesr, short numChannels)
{
    // If there are samples to read
    if (adcSampling && framesr)
    {
        // For each sample
        for (int i = 0; i < framesr; i++)
        {
            // Read the sample into the circular ssamples[] buffer
            if (sstail != ((sshead + 1) % SSBUF))
            {
#ifndef ANDROID
                // 12 bit sound, unsigned
                uint16_t v = ((in[i] + INT16_MAX) >> 4);
#else
                // Android does something different
                uint16_t v = in[i] * 5;
                if (v > 32767)
                {
                    v = 32767;
                }
                else if (v < -32768)
                {
                    v = -32768;
                }
#endif

                // Find and print max and min samples for tuning
                // static int32_t vMin = INT32_MAX;
                // static int32_t vMax = INT32_MIN;
                // if(v > vMax)
                // {
                // 	vMax = v;
                // 	printf("Audio %d -> %d\n", vMin, vMax);
                // }
                // if(v < vMin)
                // {
                // 	vMin = v;
                // 	printf("Audio %d -> %d\n", vMin, vMax);
                // }

                ssamples[sshead] = v;
                sshead           = (sshead + 1) % SSBUF;
            }
        }
    }
}
