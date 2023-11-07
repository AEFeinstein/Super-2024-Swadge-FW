//==============================================================================
// Includes
//==============================================================================

#include <string.h>
#include "hdw-led.h"
#include "hdw-led_emu.h"
#include "emu_main.h"

//==============================================================================
// Variables
//==============================================================================

static led_t rdLeds[CONFIG_NUM_LEDS + 1] = {0};
static uint8_t ledBrightness         = 0;

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initialize the RGB LEDs
 *
 * @param gpio The GPIO the LEDs are attached to
 * @param gpioAlt The GPIO to mirror the LEDs to
 * @return ESP_OK if the LEDs initialized, or a nonzero value if they did not
 */
esp_err_t initLeds(gpio_num_t gpio, gpio_num_t gpioAlt, uint8_t brightness)
{
    memset(rdLeds, 0, sizeof(rdLeds));
    setLedBrightness(brightness);
    return ESP_OK;
}

/**
 * @brief Deinit LEDs
 *
 * @return ESP_OK
 */
esp_err_t deinitLeds(void)
{
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
    if (numLeds > CONFIG_NUM_LEDS)
    {
        numLeds = CONFIG_NUM_LEDS;
    }

    for (uint8_t i = 0; i < numLeds; i++)
    {
        rdLeds[i].r = (leds[i].r >> ledBrightness);
        rdLeds[i].g = (leds[i].g >> ledBrightness);
        rdLeds[i].b = (leds[i].b >> ledBrightness);
    }

    return ESP_OK;
}

uint8_t getLedState(led_t* leds, uint8_t numLeds)
{
    if (NULL != leds && numLeds > 0)
    {
        if (numLeds > CONFIG_NUM_LEDS + 1)
        {
            numLeds = CONFIG_NUM_LEDS + 1;
        }

        memcpy(leds, rdLeds, sizeof(led_t) * numLeds);
        return numLeds;
    }

    return 0;
}

/**
 * @brief Get a pointer to the LED memory.
 *
 * @param numLeds A pointer to return the number of led_t through
 * @return A pointer to the current LED state
 */
led_t* getLedMemory(uint8_t* numLeds)
{
    *numLeds = CONFIG_NUM_LEDS;
    return rdLeds;
}
