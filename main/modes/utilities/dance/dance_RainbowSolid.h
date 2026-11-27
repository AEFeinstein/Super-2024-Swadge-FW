#pragma once

#include <stdint.h>
#include "dance.h"

typedef struct
{
    int32_t ledCount;
    int32_t color_save;
    uint32_t tAccumulated;
} danceRainbowSolidState_t;

void danceRainbowSolid(uint32_t tElapsedUs, uint32_t arg __attribute__((unused)), bool reset,
                       bool useColor __attribute__((unused)), void* state);

#ifdef DANCE_IMPLEMENTATION

/**
 * Turn on all LEDs and smooth iterate their singular color around the color wheel
 *
 * @param tElapsedUs The time elapsed since last call, in microseconds
 * @param reset      true to reset this dance's variables
 */
void danceRainbowSolid(uint32_t tElapsedUs, uint32_t arg __attribute__((unused)), bool reset,
                       bool useColor __attribute__((unused)), void* state)
{
    danceRainbowSolidState_t* danceState = state;

    if (reset)
    {
        danceState->ledCount     = 0;
        danceState->color_save   = 0;
        danceState->tAccumulated = 70000;
        return;
    }

    // Declare some LEDs, all off
    led_t leds[CONFIG_NUM_LEDS] = {{0}};
    bool ledsUpdated            = false;

    danceState->tAccumulated += tElapsedUs;
    while (danceState->tAccumulated >= 70000)
    {
        danceState->tAccumulated -= 70000;
        ledsUpdated = true;

        danceState->ledCount = danceState->ledCount + 1;
        if (danceState->ledCount > 255)
        {
            danceState->ledCount = 0;
        }
        int16_t angle = danceState->ledCount % 256;
        danceState->color_save = EHSVtoHEXhelper(angle, 0xFF, 0xFF, false);

        uint8_t i;
        for (i = 0; i < CONFIG_NUM_LEDS; i++)
        {
            leds[i].r = (danceState->color_save >> 0) & 0xFF;
            leds[i].g = (danceState->color_save >> 8) & 0xFF;
            leds[i].b = (danceState->color_save >> 16) & 0xFF;
        }
    }
    // Output the LED data, actually turning them on
    if (ledsUpdated)
    {
        setLeds(leds, CONFIG_NUM_LEDS);
    }
}

#endif
