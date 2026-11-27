#pragma once

#include <stdint.h>
#include "dance.h"

enum
{
    RISE_LEVELS = 7,
};

typedef struct
{
    int16_t levels[RISE_LEVELS];
    bool rising[RISE_LEVELS];
    uint8_t angle;
    uint32_t tAccumulated;
} danceRiseState_t;

void danceRise(uint32_t tElapsedUs, uint32_t arg, bool reset, bool useColor, void* state);

#ifdef DANCE_IMPLEMENTATION

/**
 * Rotate a single white LED around the swadge
 *
 * @param tElapsedUs The time elapsed since last call, in microseconds
 * @param reset      true to reset this dance's variables
 */
void danceRise(uint32_t tElapsedUs, uint32_t arg, bool reset, bool useColor, void* state)
{
    danceRiseState_t* danceState = state;
    static const int8_t ledsPerLevel[RISE_LEVELS][2] = {
        {6, 7}, {13, 8}, {0, 1}, {5, 2}, {12, 9}, {4, 3}, {11, 10},
    };

    if (reset)
    {
        for (uint8_t i = 0; i < RISE_LEVELS; i++)
        {
            danceState->levels[i] = i * (-512 / RISE_LEVELS);
            danceState->rising[i] = true;
        }
        danceState->angle        = 0;
        danceState->tAccumulated = 800;
        return;
    }

    bool ledsUpdated            = false;
    led_t leds[CONFIG_NUM_LEDS] = {{0}};

    RUN_TIMER_EVERY(danceState->tAccumulated, 800, tElapsedUs, {
        // Pick a random color when the first level starts rising
        if (true == danceState->rising[0] && 0 == danceState->levels[0])
        {
            danceState->angle = danceRand(256);
        }

        // Select the color
        int32_t color;
        if (useColor)
        {
            color = arg;
        }
        else
        {
            color = EHSVtoHEXhelper(danceState->angle, 0xFF, 0xFF, false);
        }

        // For each level
        for (uint8_t i = 0; i < RISE_LEVELS; i++)
        {
            // Increment if that level is rising
            if (danceState->rising[i])
            {
                danceState->levels[i]++;
                if (danceState->levels[i] == 255)
                {
                    danceState->rising[i] = false;
                }
            }
            // Or decrement if it is falling
            else
            {
                danceState->levels[i]--;
                if (danceState->levels[i] == -512)
                {
                    danceState->rising[i] = true;
                }
            }
        }

        // Set the LEDs according to values in levels[]
        for (uint8_t i = 0; i < RISE_LEVELS; i++)
        {
            // Only set the value when positive
            if (danceState->levels[i] > 0)
            {
                // For each LED in this row
                for (int8_t lIdx = 0; lIdx < ARRAY_SIZE(ledsPerLevel[0]); lIdx++)
                {
                    // Only set valid LEDs
                    int8_t ledNum = ledsPerLevel[i][lIdx];
                    if (-1 != ledNum)
                    {
                        // Set the LEDs
                        leds[ledNum].r = (danceState->levels[i] * ((color >> 16) & 0xFF) >> 8);
                        leds[ledNum].g = (danceState->levels[i] * ((color >> 8) & 0xFF) >> 8);
                        leds[ledNum].b = (danceState->levels[i] * ((color >> 0) & 0xFF) >> 8);
                    }
                }
            }
        }
        ledsUpdated = true;
    });

    if (ledsUpdated)
    {
        setLeds(leds, CONFIG_NUM_LEDS);
    }
}

#endif
