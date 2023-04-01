//==============================================================================
// Includes
//==============================================================================

#include <driver/rmt_tx.h>

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

    // Set to max brightness by default
    ledBrightness = 0;

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
 * @brief Set the global LED brightness
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

    // Make a local copy of LEDs with brightness applied
    led_t localLeds[numLeds];
    for (uint8_t i = 0; i < numLeds; i++)
    {
        localLeds[i].r = (leds[i].r >> ledBrightness);
        localLeds[i].g = (leds[i].g >> ledBrightness);
        localLeds[i].b = (leds[i].b >> ledBrightness);
    }

    // Write RGB values to LEDs
    if (ESP_OK == rmt_transmit(led_chan, led_encoder, (uint8_t*)localLeds, numLeds * sizeof(led_t), &tx_config))
    {
        // Wait until transmission is done
        // TODO I don't want to do this, but if data is transmitted too quickly, it junks up RMT and the LEDs
        // Maybe fix it with rmt_tx_register_event_callbacks()? That didn't seem to work....
        // Maybe use accumulation timer to only write LEDs every 5ms max?
        return rmt_tx_wait_all_done(led_chan, -1);
    }
    return ESP_ERR_TIMEOUT;
}
