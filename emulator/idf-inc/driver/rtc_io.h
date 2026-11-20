#pragma once

#include "esp_err.h"
#include "gpio_num.h"

#define SOC_RTCIO_PIN_COUNT 22

esp_err_t rtc_gpio_isolate(gpio_num_t gpio_num);
esp_err_t rtc_gpio_hold_dis(gpio_num_t gpio_num);