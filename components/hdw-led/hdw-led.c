//==============================================================================
// Includes
//==============================================================================

#include <driver/rmt_tx.h>
#include <esp_rom_gpio.h>
#include <soc/gpio_sig_map.h>
#include <string.h>

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
static uint8_t ledBrightness            = 0;
static led_t localLeds[CONFIG_NUM_LEDS] = {0};

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initialize the RGB LEDs
 *
 * @param gpio The GPIO the LEDs are attached to
 * @param gpioAlt A GPIO to mirror the LED output to
 * @param brightness The brightness to start the LEDs at
 * @return ESP_OK if the LEDs initialized, or a nonzero value if they did not
 */
esp_err_t initLeds(gpio_num_t gpio, gpio_num_t gpioAlt, uint8_t brightness)
{
    setLedBrightness(brightness);

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

    if (GPIO_NUM_NC != gpioAlt)
    {
        // Mirror the output to another GPIO
        // Warning, this is a hack!! led_chan is a (rmt_channel_handle_t), which is
        // really a (rmt_channel_t *), and that struct has a private definition in
        // rmt_private.h. "int channel_id" happens to be the first struct member, so
        // it is accessed using *((int*)led_chan). If the private struct ever
        // changes, this will break!!!
        esp_rom_gpio_connect_out_signal(gpioAlt, RMT_SIG_OUT0_IDX + *((int*)led_chan), false, false);
    }
    return ESP_OK;
}

/**
 * @brief Deinitialize LEDs
 *
 * @return ESP_OK
 */
esp_err_t deinitLeds(void)
{
    ESP_ERROR_CHECK(rmt_disable(led_chan));
    ESP_ERROR_CHECK(rmt_del_encoder(led_encoder));
    ESP_ERROR_CHECK(rmt_del_channel(led_chan));
    return ESP_OK;
}

/**
 * @brief Set the global LED brightness. setLedBrightnessSetting() should be called instead if the new volume should be
 * persistent through a reboot.
 *
 * @param brightness 0 (off) to MAX_LED_BRIGHTNESS (max bright)
 */
void setLedBrightness(uint8_t brightness)
{
    // LED channels are right-shifted by this value when being set
    ledBrightness = (MAX_LED_BRIGHTNESS - brightness);
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

    // Make sure to not overflow
    if (numLeds > CONFIG_NUM_LEDS)
    {
        numLeds = CONFIG_NUM_LEDS;
    }

    // Fill a local copy of LEDs with brightness applied
    for (uint8_t i = 0; i < numLeds; i++)
    {
        localLeds[i].r = (leds[i].r >> ledBrightness);
        localLeds[i].g = (leds[i].g >> ledBrightness);
        localLeds[i].b = (leds[i].b >> ledBrightness);
    }

    // Write RGB values to LEDs
    return rmt_transmit(led_chan, led_encoder, (uint8_t*)localLeds, numLeds * sizeof(led_t), &tx_config);
}

/**
 * @brief Write the current LED state into the given array
 *
 * @param[out] leds The LED array to write the state into
 * @param numLeds The maximum number of LEDs to write
 * @return uint8_t The number of LEDs actually written to the array
 */
uint8_t getLedState(led_t* leds, uint8_t numLeds)
{
    if (NULL != leds && numLeds > 0)
    {
        if (numLeds > CONFIG_NUM_LEDS)
        {
            numLeds = CONFIG_NUM_LEDS;
        }

        memcpy(leds, localLeds, sizeof(led_t) * numLeds);
        return numLeds;
    }

    return 0;
}

/**
 * @brief Wait until any pending LED transactions are finished, then return
 */
void flushLeds(void)
{
    rmt_tx_wait_all_done(led_chan, -1);
}
