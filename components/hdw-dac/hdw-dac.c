/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <inttypes.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/dac_channel.h"
#include "esp_check.h"
#include "dac_continuous_example.h"
#include "hdw-dac.h"

/**
 *  There are two ways to convert digital data to analog signal continuously:
 *  - Using a timer: setting DAC voltage periodically in the timer interrupt
 *                   in this way, DAC can achieve a relatively low conversion frequency
 *                   but it is not a efficient way comparing to using the DMA
 *  - Using DMA: transmitting the data buffer via DMA,
 *               the conversion frequency is controlled by how fast it is transmitted by DMA
 *               in this way, the conversion frequency can reach several MHz,
 *               but it can't achieve a very low conversion frequency because it is limited by the DMA clock source
 *  Generally, recommend to use DMA, if the DMA peripheral is occupied or the required conversion frequency is very low,
 *  then use timer instead
 */

/* ADC configuration */
#define EXAMPLE_DAC_CHAN0_IO                DAC_CHAN0_GPIO_NUM             // DAC channel 0 io number
#define EXAMPLE_DAC_CHAN1_IO                DAC_CHAN1_GPIO_NUM             // DAC channel 1 io number

_Static_assert(EXAMPLE_DAC_AMPLITUDE < 256, "The DAC accuracy is 8 bit-width, doesn't support the amplitude beyond 255");

static const char *TAG = "dac continuous";

uint8_t sin_wav[EXAMPLE_ARRAY_LEN];                      // Used to store sine wave values
uint8_t tri_wav[EXAMPLE_ARRAY_LEN];                      // Used to store triangle wave values
uint8_t saw_wav[EXAMPLE_ARRAY_LEN];                      // Used to store sawtooth wave values
uint8_t squ_wav[EXAMPLE_ARRAY_LEN];                      // Used to store square wave values

static void example_generate_wave(void)
{
    uint32_t pnt_num = EXAMPLE_ARRAY_LEN;

    for (int i = 0; i < pnt_num; i ++) {
        sin_wav[i] = (uint8_t)((sin( i * CONST_PERIOD_2_PI / pnt_num) + 1) * (double)(EXAMPLE_DAC_AMPLITUDE) / 2 + 0.5);
        tri_wav[i] = (i > (pnt_num / 2)) ? (2 * EXAMPLE_DAC_AMPLITUDE * (pnt_num - i) / pnt_num) : (2 * EXAMPLE_DAC_AMPLITUDE * i / pnt_num);
        saw_wav[i] = (i == pnt_num) ? 0 : (i * EXAMPLE_DAC_AMPLITUDE / pnt_num);
        squ_wav[i] = (i < (pnt_num / 2)) ? EXAMPLE_DAC_AMPLITUDE : 0;
    }
}

void example_log_info(uint32_t conv_freq, uint32_t wave_freq)
{
    ESP_LOGI(TAG, "--------------------------------------------------");
    ESP_LOGI(TAG, "DAC continuous output by DMA");
    ESP_LOGI(TAG, "DAC channel 0 io: GPIO_NUM_%d", EXAMPLE_DAC_CHAN0_IO);
    ESP_LOGI(TAG, "DAC channel 1 io: GPIO_NUM_%d", EXAMPLE_DAC_CHAN1_IO);
    ESP_LOGI(TAG, "Waveform: SINE -> TRIANGLE -> SAWTOOTH -> SQUARE");
    ESP_LOGI(TAG, "DAC conversion frequency (Hz): %"PRIu32, conv_freq);
    ESP_LOGI(TAG, "DAC wave frequency (Hz): %"PRIu32, wave_freq);
    ESP_LOGI(TAG, "--------------------------------------------------");
}

void dacInit(void)
{
    example_generate_wave();

    /* Output 2 kHz waves using DMA */
    example_dac_continuous_by_dma();
}
