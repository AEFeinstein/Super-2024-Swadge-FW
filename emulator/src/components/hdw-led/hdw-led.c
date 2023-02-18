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

static led_t rdLeds[CONFIG_NUM_LEDS];

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
    memset(rdLeds, 0, sizeof(rdLeds));
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
    memcpy(rdLeds, leds, sizeof(led_t) * numLeds);
    return ESP_OK;
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
