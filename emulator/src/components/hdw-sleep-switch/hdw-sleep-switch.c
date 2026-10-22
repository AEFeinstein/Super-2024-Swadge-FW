#include "emu_main.h"
#include "hdw-sleep-switch.h"

/**
 * @brief Initialize a GPIO as an RTC GPIO which monitors the sleep switch
 *
 * DOES NOTHING IN THE EMULATOR
 *
 * @param gpio The GPIO connected to the sleep switch. Must ben an RTC GPIO (GPIO_NUM_0 through GPIO_NUM_21 on ESP32-S3)
 * @return esp_err_t ESP_OK on success, or an ESP_ERR_ value
 */
esp_err_t initSleepSwitch(gpio_num_t gpio)
{
    WARN_UNIMPLEMENTED();
    return ESP_OK;
}

/**
 * @brief Check if the sleep switch should put the Swadge to sleep.
 * This function does not call esp_deep_sleep_start(). That must be done by the
 * caller after de-initializing the Swadge mode, hardware peripherals, etc.
 *
 * DOES NOTHING IN THE EMULATOR
 *
 * @return true if the Swadge should enter deep sleep, false if it should stay awake
 */
bool sleepSwitchShouldSleep(void)
{
    WARN_UNIMPLEMENTED();
    return false;
}
