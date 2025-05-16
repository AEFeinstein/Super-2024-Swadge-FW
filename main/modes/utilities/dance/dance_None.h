#pragma once

#include <stdint.h>
#include "dance.h"

void danceNone(uint32_t tElapsedUs __attribute__((unused)), uint32_t arg __attribute__((unused)), bool reset);

#ifdef DANCE_IMPLEMENTATION

/**
 * @brief Blank the LEDs
 *
 * @param tElapsedUs
 * @param arg
 * @param reset
 */
void danceNone(uint32_t tElapsedUs __attribute__((unused)), uint32_t arg __attribute__((unused)), bool reset)
{
    if (reset)
    {
        led_t leds[CONFIG_NUM_LEDS] = {{0}};
        setLeds(leds, CONFIG_NUM_LEDS);
    }
}

#endif
