//==============================================================================
// Includes
//==============================================================================

#include "hdw-mic.h"
#include "emu_main.h"

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
    WARN_UNIMPLEMENTED();
}

/**
 * @brief Start sampling the microphone's ADC
 */
void startMic(void)
{
    WARN_UNIMPLEMENTED();
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
    WARN_UNIMPLEMENTED();
    return 0;
}

/**
 * @brief Stop sampling the microphone's ADC
 */
void stopMic(void)
{
    WARN_UNIMPLEMENTED();
}

/**
 * @brief Deinitialize the ADC which continuously samples the microphone
 */
void deinitMic(void)
{
    WARN_UNIMPLEMENTED();
}
