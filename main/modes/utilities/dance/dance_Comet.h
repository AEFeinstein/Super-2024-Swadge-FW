#pragma once

#include <stdint.h>
#include "dance.h"

typedef struct
{
    int32_t ledCount;
    uint8_t rainbow;
    int32_t msCount;
    uint32_t tAccumulated;
    led_t leds[CONFIG_NUM_LEDS];
} danceCometState_t;

void danceComet(uint32_t tElapsedUs, uint32_t arg, bool reset, bool useColor, void* state);

#ifdef DANCE_IMPLEMENTATION

/**
 * Rotate a single white LED around the swadge
 *
 * @param tElapsedUs The time elapsed since last call, in microseconds
 * @param reset      true to reset this dance's variables
 */
void danceComet(uint32_t tElapsedUs, uint32_t arg, bool reset, bool useColor, void* state)
{
    danceCometState_t* danceState = state;
    // Map comet index to LED index
    const int8_t ledMap[CONFIG_NUM_LEDS] = {// Down the right wing
                                            10, 9, 8, 7,
                                            // Up the left wing
                                            6, 13, 12, 11,
                                            // Around the screen
                                            3, 2, 1, 0, 5, 4};

    if (reset)
    {
        danceState->ledCount     = 0;
        danceState->rainbow      = 0;
        danceState->msCount      = 80;
        danceState->tAccumulated = 2000;
        memset(danceState->leds, 0, sizeof(danceState->leds));
        return;
    }

    bool ledsUpdated = false;

    danceState->tAccumulated += tElapsedUs;
    while (danceState->tAccumulated >= 2000)
    {
        danceState->tAccumulated -= 2000;
        for (uint8_t i = 0; i < CONFIG_NUM_LEDS; i++)
        {
            led_t* led = &danceState->leds[ledMap[i]];
            if (led->r > 0)
            {
                led->r--;
            }
            if (led->g > 0)
            {
                led->g--;
            }
            if (led->b > 0)
            {
                led->b--;
            }
        }
        danceState->msCount++;

        if (danceState->msCount % 10 == 0)
        {
            danceState->rainbow++;
        }

        if (danceState->msCount >= 80)
        {
            led_t* led = &danceState->leds[ledMap[danceState->ledCount]];
            if (useColor)
            {
                led->r = ARG_R(arg);
                led->g = ARG_G(arg);
                led->b = ARG_B(arg);
            }
            else
            {
                int32_t color = EHSVtoHEXhelper(danceState->rainbow, 0xFF, 0xFF, false);
                led->r        = ARG_R(color);
                led->g        = ARG_G(color);
                led->b        = ARG_B(color);
            }
            danceState->ledCount = (danceState->ledCount + 1) % CONFIG_NUM_LEDS;
            danceState->msCount  = 0;
        }
        ledsUpdated = true;
    }

    if (ledsUpdated)
    {
        setLeds(danceState->leds, CONFIG_NUM_LEDS);
    }
}

#endif
