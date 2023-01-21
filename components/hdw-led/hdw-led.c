/*! \file hdw-led.c
 *
 * \section led_design Design Philosophy
 *
 * LED code is based on <a
 * href="https://github.com/espressif/esp-idf/tree/master/examples/peripherals/rmt/led_strip">Espressif's RMT Transmit
 * Example - LED Strip</a>.
 *
 * Each LED has a red, green, and blue component. Each component ranges from 0 to 255.
 *
 * LED brightness is not linear, so differences in brightness are more noticeable between small values (like 1 and 10)
 * than between large values (like 240 and 250).
 *
 * The number of LEDs are configurable by "idf.py menuconfig" and is accessible in code with the CONFIG_NUM_LEDS macro.
 *
 * \section led_usage Usage
 *
 * You don't need to call initLeds(). The system does so at the appropriate time.
 *
 * You should call setLeds() any time you want to set the LEDs. There is no buffer, so the LEDs are immediately set to
 * the values given. setLeds() takes a pointer to an array of ::led_t as an argument. These structs each have a red,
 * green, and blue field.
 *
 * \section led_example Example
 *
 * Set the LEDs to a rough rainbow:
 * \code{.c}
 * #include "hdw-led.c"
 *
 * led_t leds[CONFIG_NUM_LEDS] = {0};
 * for (uint8_t i = 0; i < CONFIG_NUM_LEDS; i++)
 * {
 *     leds[i].r = (255 * ((i + 0) % CONFIG_NUM_LEDS)) / (CONFIG_NUM_LEDS - 1);
 *     leds[i].g = (255 * ((i + 3) % CONFIG_NUM_LEDS)) / (CONFIG_NUM_LEDS - 1);
 *     leds[i].b = (255 * ((i + 6) % CONFIG_NUM_LEDS)) / (CONFIG_NUM_LEDS - 1);
 * }
 * setLeds(leds, CONFIG_NUM_LEDS);
 * \endcode
 */

//==============================================================================
// Includes
//==============================================================================

#include "driver/rmt_tx.h"
#include "led_strip_encoder.h"
#include "hdw-led.h"

//==============================================================================
// Defines
//==============================================================================

/// 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)
#define RMT_LED_STRIP_RESOLUTION_HZ 10000000

//==============================================================================
// Variables
//==============================================================================

static rmt_channel_handle_t led_chan    = NULL;
static rmt_encoder_handle_t led_encoder = NULL;

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initialize the RGB LEDs
 *
 * @param gpio The GPIO the LEDs are attached to
 * @return ESP_OK if the LEDs initialized, or a nonzero value if they did not
 */
esp_err_t initLeds(gpio_num_t gpio)
{
    rmt_tx_channel_config_t tx_chan_config = {
        .clk_src           = RMT_CLK_SRC_DEFAULT, // select source clock
        .gpio_num          = gpio,
        .mem_block_symbols = 64, // increase the block size can make the LED less flickering
        .resolution_hz     = RMT_LED_STRIP_RESOLUTION_HZ,
        .trans_queue_depth = 4, // set the number of transactions that can be pending in the background
    };
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &led_chan));

    led_strip_encoder_config_t encoder_config = {
        .resolution = RMT_LED_STRIP_RESOLUTION_HZ,
    };
    ESP_ERROR_CHECK(rmt_new_led_strip_encoder(&encoder_config, &led_encoder));

    ESP_ERROR_CHECK(rmt_enable(led_chan));

    return ESP_OK;
}

/**
 * @brief Set the RGB LEDs to the given values
 *
 * @param leds A pointer to an array of ::led_t structs to set the LEDs to. The array must have at least numLeds
 * elements
 * @param numLeds The number of LEDs to set, probably CONFIG_NUM_LEDS
 * @return ESP_OK if the LEDs were set, or a nonzero value if they did were not
 */
esp_err_t setLeds(led_t* leds, uint8_t numLeds)
{
    rmt_transmit_config_t tx_config = {
        .loop_count = 0, // no transfer loop
    };
    // Flush RGB values to LEDs
    return rmt_transmit(led_chan, led_encoder, (uint8_t*)leds, numLeds * sizeof(led_t), &tx_config);
}
