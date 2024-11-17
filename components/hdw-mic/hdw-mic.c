//==============================================================================
// Includes
//==============================================================================

#include <esp_adc/adc_continuous.h>
#include "hdw-mic.h"

//==============================================================================
// Variables
//==============================================================================

static adc_continuous_handle_t adc_handle = NULL;

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
    // If the mic isn't initalized
    if (!adc_handle)
    {
        adc_unit_t unit;
        adc_channel_t channel;
        if (ESP_OK == adc_continuous_io_to_channel(gpio, &unit, &channel))
        {
            // Configure the continuous ADC read sizes
            adc_continuous_handle_cfg_t adc_config = {
                .max_store_buf_size = 1024,
                .conv_frame_size    = ADC_READ_LEN,
            };
            ESP_ERROR_CHECK(adc_continuous_new_handle(&adc_config, &adc_handle));

            // Pick the conversion mode based on the given channels.
            // This works because, ADC_CONV_BOTH_UNIT == (ADC_CONV_SINGLE_UNIT_1 | ADC_CONV_SINGLE_UNIT_2)
            adc_digi_convert_mode_t conv = 0;
            switch (unit)
            {
                case ADC_UNIT_1:
                {
                    conv = ADC_CONV_SINGLE_UNIT_1;
                    break;
                }
                case ADC_UNIT_2:
                {
                    conv = ADC_CONV_SINGLE_UNIT_2;
                    break;
                }
            }

            // Configure the polling frequency, conversion mode, and data format
            adc_continuous_config_t dig_cfg = {
                .sample_freq_hz = ADC_SAMPLE_RATE_HZ,
                .conv_mode      = conv,
                .format         = ADC_DIGI_OUTPUT_FORMAT_TYPE1,
            };

            adc_digi_pattern_config_t adc_pattern[SOC_ADC_PATT_LEN_MAX] = {0};

            // Configure the ADC pattern. We only configure a single channel
            adc_pattern[0].atten     = ADC_ATTEN_DB_12;
            adc_pattern[0].channel   = channel;
            adc_pattern[0].unit      = unit;
            adc_pattern[0].bit_width = SOC_ADC_DIGI_MAX_BITWIDTH;

            // Set the patterns
            dig_cfg.pattern_num = 1;
            dig_cfg.adc_pattern = adc_pattern;
            ESP_ERROR_CHECK(adc_continuous_config(adc_handle, &dig_cfg));
        }
    }
}

/**
 * @brief Start sampling the microphone's ADC
 */
void startMic(void)
{
    if (adc_handle)
    {
        ESP_ERROR_CHECK(adc_continuous_start(adc_handle));
    }
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
    if (adc_handle)
    {
        // Read the continuous ADC to this memory
        uint8_t result[ADC_READ_LEN];
        uint32_t ret_num = 0;

        // Condition the read samples for return
        uint32_t samplesRead = 0;
        if (adc_continuous_read(adc_handle, result, ADC_READ_LEN, &ret_num, 0) == ESP_OK)
        {
            // Loop, but don't go over the number of read samples or number of samples to return
            for (int i = 0; (i < ret_num) && (samplesRead < outSamplesMax); i += SOC_ADC_DIGI_RESULT_BYTES)
            {
                // ADC_DIGI_OUTPUT_FORMAT_TYPE1 is specified in continuous_adc_init()
                *(outSamples++) = ((adc_digi_output_data_t*)(&result[i]))->type1.data;
                samplesRead++;
            }
        }

        // Return the number of samples read
        return samplesRead;
    }
    return 0;
}

/**
 * @brief Stop sampling the microphone's ADC
 */
void stopMic(void)
{
    if (adc_handle)
    {
        ESP_ERROR_CHECK(adc_continuous_stop(adc_handle));
    }
}

/**
 * @brief Deinitialize the ADC which continuously samples the microphone
 */
void deinitMic(void)
{
    if (adc_handle)
    {
        stopMic();
        ESP_ERROR_CHECK(adc_continuous_deinit(adc_handle));
        adc_handle = NULL;
    }
}
