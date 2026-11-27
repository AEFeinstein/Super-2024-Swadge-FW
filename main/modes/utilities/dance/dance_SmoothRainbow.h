#pragma once

#include <stdint.h>
#include "dance.h"

typedef struct
{
    uint32_t tAccumulated;
    uint8_t ledCount;
} danceSmoothRainbowState_t;

void danceSmoothRainbow(uint32_t tElapsedUs, uint32_t arg, bool reset, bool useColor __attribute__((unused)), void* state);

#ifdef DANCE_IMPLEMENTATION

/**
 * Smoothly rotate a color wheel around the swadge
 *
 * @param tElapsedUs The time elapsed since last call, in microseconds
 * @param reset      true to reset this dance's variables
 */
void danceSmoothRainbow(uint32_t tElapsedUs, uint32_t arg, bool reset, bool useColor __attribute__((unused)), void* state)
{
    danceSmoothRainbowState_t* danceState = state;
    // Map comet index to LED index
    const int8_t ledMap[CONFIG_NUM_LEDS] = {// Down the right side
                                            10, 3, 9, 2, 1, 8, 7,
                                            // Up the left side
                                            6, 13, 0, 5, 12, 4, 11};

    if (reset)
    {
        danceState->ledCount     = 0;
        danceState->tAccumulated = arg;
        return;
    }

    // Declare some LEDs, all off
    led_t leds[CONFIG_NUM_LEDS] = {{0}};
    bool ledsUpdated            = false;

    danceState->tAccumulated += tElapsedUs;
    while (danceState->tAccumulated >= arg)
    {
        danceState->tAccumulated -= arg;
        ledsUpdated = true;

        danceState->ledCount--;

        uint8_t i;
        for (i = 0; i < CONFIG_NUM_LEDS; i++)
        {
            int16_t angle  = ((((i * 256) / CONFIG_NUM_LEDS)) + danceState->ledCount) % 256;
            uint32_t color = EHSVtoHEXhelper(angle, 0xFF, 0xFF, false);

            leds[ledMap[i]].r = (color >> 0) & 0xFF;
            leds[ledMap[i]].g = (color >> 8) & 0xFF;
            leds[ledMap[i]].b = (color >> 16) & 0xFF;
        }
    }
    // Output the LED data, actually turning them on
    if (ledsUpdated)
    {
        setLeds(leds, CONFIG_NUM_LEDS);
    }
}

#endif
