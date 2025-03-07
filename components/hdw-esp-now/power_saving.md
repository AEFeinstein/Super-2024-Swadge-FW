# Wireless Power Saving Notes

## `menuconfig` Options

| Option | Automatically Called Function | Notes |
|----------------------------------------|----------------------------------------------------------------------------------|---------------------------------------------------------|
| `CONFIG_PM_ENABLE` | | If enabled, application is compiled with support for power management. This option has run-time overhead (increased interrupt latency, longer time to enter idle state), and it also reduces accuracy of RTOS ticks and timers used for timekeeping. Enable this option if application uses power management APIs. |
| `CONFIG_FREERTOS_USE_TICKLESS_IDLE` | `esp_sleep_enable_wifi_wakeup()` `esp_sleep_disable_wifi_wakeup()` | If power management support is enabled, FreeRTOS will be able to put the system into light sleep mode when no tasks need to run for a number of ticks. This number can be set using `FREERTOS_IDLE_TIME_BEFORE_SLEEP` option. This feature is also known as "automatic light sleep". Note that timers created using esp_timer APIs may prevent the system from entering sleep mode, even when no tasks need to run. To skip unnecessary wake-up initialize a timer with the "skip_unhandled_events" option as true. If disabled, automatic light sleep support will be disabled. |
| `CONFIG_ESP_WIFI_ENHANCED_LIGHT_SLEEP` | `esp_sleep_enable_wifi_beacon_wakeup()` `esp_sleep_disable_wifi_beacon_wakeup()` | The wifi modem automatically receives the beacon frame during light sleep. `CONFIG_FREERTOS_USE_TICKLESS_IDLE` must be set as well |
| `CONFIG_ESP_WIFI_SLP_IRAM_OPT` | | Select this option to place called Wi-Fi library TBTT process and receive beacon functions in IRAM. Some functions can be put in IRAM either by `ESP_WIFI_IRAM_OPT` and `ESP_WIFI_RX_IRAM_OPT`, or this one. If already enabled `ESP_WIFI_IRAM_OPT`, the other 7.3KB IRAM memory would be taken by this option. If already enabled `ESP_WIFI_RX_IRAM_OPT`, the other 1.3KB IRAM memory would be taken by this option. If neither of them are enabled, the other 7.4KB IRAM memory would be taken by this option. Wi-Fi power-save mode average current would be reduced if this option is enabled. |

## ESP-NOW Window & Interval

[`esp_err_t esp_now_set_wake_window(uint16_t window)`](https://docs.espressif.com/projects/esp-idf/en/v5.2.5/esp32s2/api-reference/network/esp_now.html#_CPPv423esp_now_set_wake_window8uint16_t)
> Set wake window for esp_now to wake up in interval unit.

[`esp_err_t esp_wifi_connectionless_module_set_wake_interval(uint16_t wake_interval)`](
https://docs.espressif.com/projects/esp-idf/en/v5.2.5/esp32s2/api-reference/network/esp_wifi.html#_CPPv448esp_wifi_connectionless_module_set_wake_interval8uint16_t)
> Set wake interval for connectionless modules to wake up periodically.

### Example Configuration

```c
esp_now_set_wake_window(50);
esp_wifi_connectionless_module_set_wake_interval(100);
``` 

## Espressif Documentation

* [Sleep Modes, Wi-Fi and Sleep Modes](https://docs.espressif.com/projects/esp-idf/en/v5.2.5/esp32s2/api-reference/system/sleep_modes.html#wi-fi-and-sleep-modes)
	> In Deep-sleep and Light-sleep modes, the wireless peripherals are powered down. Before entering Deep-sleep or Light-sleep modes, applications must disable Wi-Fi using the appropriate calls (`esp_wifi_stop()`). Wi-Fi connections are not maintained in Deep-sleep or Light-sleep mode, even if these functions are not called.
* [Wi-Fi Driver, Connectionless Modules Power-saving](https://docs.espressif.com/projects/esp-idf/en/v5.2.5/esp32s2/api-guides/wifi.html#connectionless-module-power-save)
    > For each connectionless module, its supported to TX at any sleeping time without any extra configuration.
    >
    > For each connectionless module, two parameters shall be configured to RX at sleep, which are Window and Interval. At the start of Interval time, RF, PHY, BB would be turned on and kept for Window time. Connectionless Module could RX in the duration.
* [ESP-NOW, Config ESP-NOW Power-saving Parameter](https://docs.espressif.com/projects/esp-idf/en/v5.2.5/esp32s2/api-reference/network/esp_now.html#config-esp-now-power-saving-parameter)
    > Call `esp_now_set_wake_window()` to configure Window for ESP-NOW RX at sleep. The default value is the maximum, which allowing RX all the time.
    > 
    > If Power-saving is needed for ESP-NOW, call `esp_wifi_connectionless_module_set_wake_interval()` to configure Interval as well.

#### Wi-Fi Specific, not relevant to ESP-NOW?
* [Wi-Fi Driver, ESP32-S2 Wi-Fi Power-saving Mode](https://docs.espressif.com/projects/esp-idf/en/v5.2.5/esp32s2/api-guides/wifi.html#esp32-s2-wi-fi-power-saving-mode)
* [Low Power Mode Usage Guide, Auto Light-sleep + Wi-Fi scenario](https://docs.espressif.com/projects/esp-idf/zh_CN/v5.2.5/esp32s2/api-guides/low-power-mode.html#auto-light-sleep-wi-fi)
