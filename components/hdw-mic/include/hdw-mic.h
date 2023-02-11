#ifndef _HDW_MIC_H_
#define _HDW_MIC_H_

//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>

#include <hal/gpio_types.h>
#include <esp_adc/adc_continuous.h>

//==============================================================================
// Defines
//==============================================================================

#define ADC_READ_LEN 512

//==============================================================================
// Function Prototypes
//==============================================================================

void initMic(gpio_num_t gpio);
void startMic(void);
uint32_t loopMic(uint16_t* outSamples, uint32_t outSamplesMax);
void stopMic(void);
void deinitMic(void);

#endif