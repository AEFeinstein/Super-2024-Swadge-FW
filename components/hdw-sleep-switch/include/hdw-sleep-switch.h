/*! \file hdw-sleep-switch.h
 *
 * \section slp_design Design Philosophy
 *
 * Prior to the Fairy Swadge, the power switch would cut power between the battery and the ESP.
 *
 * As of the Fairy Swadge, the power switch is connected to a GPIO on one end and either a 10K pullup to VCC or GND on
 * the other end.
 *
 * The Swadge <a
 * href="https://docs.espressif.com/projects/esp-idf/en/v5.2.8/esp32s3/api-reference/system/sleep_modes.html">enters or
 * exits deep sleep</a> depending on the GPIO state.
 *
 * The GPIO is configured as an <a
 * href="https://docs.espressif.com/projects/esp-idf/en/v5.2.8/esp32s3/api-reference/peripherals/gpio.html">RTC GPIO</a>
 * in order to wake the ESP from deep sleep.
 *
 * \section slp_usage Usage
 *
 * This component is not intended to be used by developers. It is set up and used in `app_main()` and should only be
 * used there.
 *
 * \section slp_example Example
 *
 * \code{.c}
 * // Initialize sleep switch
 * initSleepSwitch(GPIO_SLEEP);
 *
 * // If the switch says it's time to sleep
 * if (sleepSwitchShouldSleep())
 * {
 *     // Don't start a specific mode when awoken
 *     pendingSwadgeMode = NULL;
 *     // Deinitialize everything (including the swadge mode)
 *     deinitSystem();
 *     // Sleep until the switch wakes the swadge
 *     esp_deep_sleep_start();
 * }
 * \endcode
 */

#pragma once

#include <driver/gpio.h>
#include <esp_err.h>

esp_err_t initSleepSwitch(gpio_num_t gpio);
bool sleepSwitchShouldSleep(void);
