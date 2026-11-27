#pragma once

#include <stdint.h>
#include "dance.h"

typedef struct
{
    uint32_t tAccumulated;
    uint8_t randLedMask;
    uint32_t randColor;
    uint8_t ledVal;
    bool ledRising;
    uint32_t randInterval;
} dancePureRandomState_t;

void dancePureRandom(uint32_t tElapsedUs, uint32_t arg, bool reset, bool useColor, void* state);

#ifdef DANCE_IMPLEMENTATION

/**
 * Turn a random LED on to a random color, one at a time
 *
 * @param tElapsedUs The time elapsed since last call, in microseconds
 * @param reset      true to reset this dance's variables
 */
void dancePureRandom(uint32_t tElapsedUs, uint32_t arg, bool reset, bool useColor, void* state)
{
    dancePureRandomState_t* danceState = state;

    if (reset)
    {
        danceState->randInterval = 5000;
        danceState->tAccumulated = danceState->randInterval;
        danceState->randLedMask  = 0;
        danceState->randColor    = 0;
        danceState->ledVal       = 0;
        danceState->ledRising    = true;
        return;
    }

    // Declare some LEDs, all off
    led_t leds[CONFIG_NUM_LEDS] = {{0}};
    bool ledsUpdated            = false;

    danceState->tAccumulated += tElapsedUs;
    while (danceState->tAccumulated >= danceState->randInterval)
    {
        danceState->tAccumulated -= danceState->randInterval;

        if (0 == danceState->ledVal)
        {
            danceState->randColor    = danceRand(256);
            danceState->randLedMask  = danceRand(1 << CONFIG_NUM_LEDS);
            danceState->randInterval = 500 + danceRand(4096);
            danceState->ledVal++;
        }
        else if (danceState->ledRising)
        {
            danceState->ledVal++;
            if (255 == danceState->ledVal)
            {
                danceState->ledRising = false;
            }
        }
        else
        {
            danceState->ledVal--;
            if (0 == danceState->ledVal)
            {
                danceState->ledRising = true;
            }
        }

        ledsUpdated    = true;
        uint32_t color = EHSVtoHEXhelper(danceState->randColor, 0xFF, danceState->ledVal, false);
        for (uint8_t i = 0; i < CONFIG_NUM_LEDS; i++)
        {
            if ((1 << i) & danceState->randLedMask)
            {
                if (useColor)
                {
                    leds[i].r = ARG_R(arg);
                    leds[i].g = ARG_G(arg);
                    leds[i].b = ARG_B(arg);
                }
                else
                {
                    leds[i].r = (color >> 0) & 0xFF;
                    leds[i].g = (color >> 8) & 0xFF;
                    leds[i].b = (color >> 16) & 0xFF;
                }
            }
        }
    }
    // Output the LED data, actually turning them on
    if (ledsUpdated)
    {
        setLeds(leds, CONFIG_NUM_LEDS);
    }
}

#endif
