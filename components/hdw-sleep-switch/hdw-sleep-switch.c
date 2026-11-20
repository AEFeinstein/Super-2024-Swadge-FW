#include <stdio.h>
#include "esp_sleep.h"
#include "hdw-sleep-switch.h"
#include "driver/rtc_io.h"

extern void rtc_sleep_enable_ultra_low(bool);

static gpio_num_t ssGpio = GPIO_NUM_NC;

/**
 * @brief Initialize a GPIO as an RTC GPIO which monitors the sleep switch
 *
 * @param gpio The GPIO connected to the sleep switch. Must ben an RTC GPIO (GPIO_NUM_0 through GPIO_NUM_21 on ESP32-S3)
 * @return esp_err_t ESP_OK on success, or an ESP_ERR_ value
 */
esp_err_t initSleepSwitch(gpio_num_t gpio)
{
    if (GPIO_NUM_NC == gpio)
    {
        return ESP_ERR_INVALID_ARG;
    }

    // Save the GPIO
    ssGpio = gpio;

    // Initialize the RTC GPIO
    ESP_ERROR_CHECK(rtc_gpio_init(gpio));
    ESP_ERROR_CHECK(rtc_gpio_set_direction(gpio, RTC_GPIO_MODE_INPUT_ONLY));
    ESP_ERROR_CHECK(rtc_gpio_set_direction_in_sleep(gpio, RTC_GPIO_MODE_INPUT_ONLY));
    ESP_ERROR_CHECK(rtc_gpio_pulldown_dis(gpio));
    ESP_ERROR_CHECK(rtc_gpio_pullup_dis(gpio));
    ESP_ERROR_CHECK(rtc_gpio_wakeup_enable(gpio, GPIO_INTR_HIGH_LEVEL));

    // Set the RTC GPIO as a wakeup source
    ESP_ERROR_CHECK(esp_sleep_enable_ext0_wakeup(ssGpio, 1));

    // Enable a sleepier sleep
    rtc_sleep_enable_ultra_low(true);

    return ESP_OK;
}

/**
 * @brief Check if the sleep switch should put the Swadge to sleep.
 * This function does not call esp_deep_sleep_start(). That must be done by the
 * caller after de-initializing the Swadge mode, hardware peripherals, etc.
 *
 * @return true if the Swadge should enter deep sleep, false if it should stay awake
 */
bool sleepSwitchShouldSleep(void)
{
    if (GPIO_NUM_NC != ssGpio)
    {
        return (0 == rtc_gpio_get_level(ssGpio));
    }
    return false;
}
