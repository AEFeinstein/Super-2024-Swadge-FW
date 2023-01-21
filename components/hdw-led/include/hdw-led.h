#ifndef _HDW_LED_H_
#define _HDW_LED_H_

#include <stdint.h>
#include "hal/gpio_types.h"

typedef struct __attribute__((packed))
{
    uint8_t g; //!< The green component, 0-255
    uint8_t r; //!< The red component, 0-255
    uint8_t b; //!< The blue component, 0-255
} led_t;

esp_err_t initLeds(gpio_num_t gpio);
esp_err_t setLeds(led_t* leds, uint8_t numLeds);

#endif