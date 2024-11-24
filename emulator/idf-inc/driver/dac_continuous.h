#pragma once

#define BIT(nr) (1UL << (nr))

/**
 * @brief DAC channel mask
 *
 */
typedef enum {
    DAC_CHANNEL_MASK_CH0 = BIT(0),          /*!< DAC channel 0 is GPIO25(ESP32) / GPIO17(ESP32S2) */
    DAC_CHANNEL_MASK_CH1 = BIT(1),          /*!< DAC channel 1 is GPIO26(ESP32) / GPIO18(ESP32S2) */
    DAC_CHANNEL_MASK_ALL = BIT(0) | BIT(1), /*!< Both DAC channel 0 and channel 1 */
} dac_channel_mask_t;
