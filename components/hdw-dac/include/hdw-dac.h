/*! \file hdw-dac.h
 *
 * \section dac_design Design Philosophy
 *
 * The Digital To Analog Converter (DAC) is used to drive a speaker or headphones. It uses the <a
 * href="https://docs.espressif.com/projects/esp-idf/en/v5.3.1/esp32s2/api-reference/peripherals/dac.html">DAC
 * peripheral</a>. The peripheral can be used a few different ways, but this project uses DMA to output a
 * continuous, arbitrary signal.
 *
 * This component is initialized by initDac() with a ::fnDacCallback_t callback which will request DAC samples from the
 * application when required. Sample requests come from the DAC peripheral in an interrupt and are queued to be serviced
 * out of the interrupt. This queue is checked in dacPoll(). Spending time to generate samples in an interrupt isn't a
 * good idea.
 *
 * \warning
 * Note that the DAC peripheral and the ADC peripheral (hdw-mic.h) use the same DMA controller, so they cannot both be
 * used at the same time. One must be deinitialize before initializing the other.
 *
 * \section dac_usage Usage
 *
 * You don't need to call initDac() or deinitDac(). The system does at the appropriate times.
 *
 * The system will also automatically call dacStart(), though the Swadge mode can later call dacStop() or dacStart()
 * when the DAC needs to be used. Stopping the DAC when not in use can save some processing cycles, but stopping it
 * abruptly may cause unwanted clicks or pops on the speaker.
 *
 * dacPoll() is called automatically by the system while the DAC is running. By default, samples are requested from
 * sngPlayerFillBuffer(). Swadge modes may override this by providing a non-NULL function pointer for
 * ::swadgeMode_t.fnDacCb.
 *
 * \section dac_example Example
 *
 * \code{.c}
 * void dacCallback(uint8_t* samples, int16_t len)
 * {
 *     // Generate something sawtooth-ish. It's just an example, gimme a break
 *     for(int16_t idx = 0; idx < len; idx++)
 *     {
 *         samples[idx] = idx;
 *     }
 * }
 *
 * int main()
 * {
 *     // Initialize and start the DAC
 *     initDac(dacCallback);
 *     dacStart();
 *
 *     // Loop forever and poll the DAC.
 *     bool running = true;
 *     while(running)
 *     {
 *         // This will end up calling dacCallback as appropriate
 *         dacPoll();
 *     }
 *
 *     // Cleanup
 *     dacStop();
 * }
 * \endcode
 */

#pragma once

//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>
#include <stdbool.h>
#include "driver/dac_continuous.h"
#include "driver/gpio.h"

//==============================================================================
// Defines
//==============================================================================

/** @brief The sample rate for the DAC */
#define DAC_SAMPLE_RATE_HZ 32768

/** The size of each buffer to fill with DAC samples */
#define DAC_BUF_SIZE 512

//==============================================================================
// Typedefs
//==============================================================================

/**
 * @brief A callback which requests DAC samples from the application
 *
 * @param samples A buffer to fill with 8 bit unsigned DAC samples
 * @param len The length of the buffer to fill
 */
typedef void (*fnDacCallback_t)(uint8_t* samples, int16_t len);

//==============================================================================
// Function Declarations
//==============================================================================

void initDac(dac_channel_mask_t channel, gpio_num_t shdn_gpio, fnDacCallback_t cb);
void deinitDac(void);
void dacPoll(void);
void dacStart(void);
void dacStop(void);
void setDacShutdown(bool shutdown);
