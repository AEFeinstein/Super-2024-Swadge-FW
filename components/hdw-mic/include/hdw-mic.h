/*! \file hdw-mic.h
 *
 * \section mic_design Design Philosophy
 *
 * The microphone uses the <a
 * href="https://docs.espressif.com/projects/esp-idf/en/v5.2.1/esp32s2/api-reference/peripherals/adc_continuous.html">Analog
 * to Digital Converter (ADC) Continuous Mode Driver</a>.
 *
 * The microphone code is based on the <a
 * href="https://github.com/espressif/esp-idf/tree/v5.2.1/examples/peripherals/adc/continuous_read">ADC DMA
 * Example</a>.
 *
 * The microphone is continuously sampled at 8KHz.
 *
 * \warning The battery monitor (hdw-battmon.h) and microphone cannot be used at the same time! Each mode can either
 * continuously sample the microphone or measure the battery voltage, not both.
 *
 * \if Cond1
 * \endif
 *
 * \warning
 * Note that the DAC peripheral (hdw-dac.h) and the ADC peripheral use the same DMA controller, so they cannot both be
 * used at the same time. One must be deinitialize before initializing the other.
 *
 * \section mic_usage Usage
 *
 * You don't need to call initMic() or deinitMic(). The system does at the appropriate times.
 *
 * The system will also automatically call startMic(), though the Swadge mode can later call stopMic() or startMic()
 * when the microphone needs to be used. Stopping the microphone when not in use can save some processing cycles.
 *
 * loopMic() is called automatically by the system while the microphone is started and samples are delivered to the
 * Swadge mode through a callback, ::swadgeMode_t.fnAudioCallback. The Swadge mode can do what it wants with the samples
 * from there.
 *
 * If ::swadgeMode_t.fnAudioCallback is left NULL, then the microphone will not be initialized or sampled.
 *
 * \section mic_example Example
 *
 * \code{.c}
 * static void demoAudioCallback(uint16_t* samples, uint32_t sampleCnt);
 *
 * swadgeMode_t demoMode = {
 *     ...
 *     .fnAudioCallback = demoAudioCallback,
 *     ...
 * };
 *
 * ...
 *
 * // Start the mic
 * startMic();
 *
 * // Stop the mic
 * stopMic();
 *
 * ...
 *
 * static void demoAudioCallback(uint16_t* samples, uint32_t sampleCnt)
 * {
 *     ; // Do something with the audio samples, when the mic is started
 * }
 * \endcode
 */

#ifndef _HDW_MIC_H_
#define _HDW_MIC_H_

//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>

#include <hal/gpio_types.h>

//==============================================================================
// Defines
//==============================================================================

/// The maximum number of bytes read by the ADC in one go
#define ADC_READ_LEN 512

#define MAX_MIC_GAIN 7

//==============================================================================
// Function Prototypes
//==============================================================================

void initMic(gpio_num_t gpio);
void startMic(void);
uint32_t loopMic(uint16_t* outSamples, uint32_t outSamplesMax);
void stopMic(void);
void deinitMic(void);

#endif