#pragma once

#include <stdint.h>
#include "dance.h"

typedef struct
{
    uint8_t ledVal;
    uint8_t randColor;
    bool goingUp;
    uint32_t tAccumulated;
} dancePulseState_t;

void dancePulse(uint32_t tElapsedUs, uint32_t arg, bool reset, bool useColor, void* state);

#ifdef DANCE_IMPLEMENTATION

/**
 * Blink all LEDs red for on for 500ms, then off for 500ms
 *
 * @param tElapsedUs The time elapsed since last call, in microseconds
 * @param reset      true to reset this dance's variables
 */
void dancePulse(uint32_t tElapsedUs, uint32_t arg, bool reset, bool useColor, void* state)
{
    dancePulseState_t* danceState = state;

    if (reset)
    {
        danceState->ledVal       = 0;
        danceState->randColor    = 0;
        danceState->goingUp      = true;
        danceState->tAccumulated = 5000;
        return;
    }

    // Declare some LEDs, all off
    led_t leds[CONFIG_NUM_LEDS] = {{0}};
    bool ledsUpdated            = false;

    danceState->tAccumulated += tElapsedUs;
    while (danceState->tAccumulated >= 5000)
    {
        danceState->tAccumulated -= 5000;

        if (danceState->goingUp)
        {
            danceState->ledVal++;
            if (255 == danceState->ledVal)
            {
                danceState->goingUp = false;
            }
        }
        else
        {
            danceState->ledVal--;
            if (0 == danceState->ledVal)
            {
                danceState->goingUp   = true;
                danceState->randColor = danceRand(256);
            }
        }

        for (int i = 0; i < CONFIG_NUM_LEDS; i++)
        {
            if (useColor)
            {
                leds[i].r = (danceState->ledVal * ARG_R(arg)) >> 8;
                leds[i].g = (danceState->ledVal * ARG_G(arg)) >> 8;
                leds[i].b = (danceState->ledVal * ARG_B(arg)) >> 8;
            }
            else
            {
                int32_t color = EHSVtoHEXhelper(danceState->randColor, 0xFF, 0xFF, false);
                leds[i].r     = (danceState->ledVal * ARG_R(color)) >> 8;
                leds[i].g     = (danceState->ledVal * ARG_G(color)) >> 8;
                leds[i].b     = (danceState->ledVal * ARG_B(color)) >> 8;
            }
        }
        ledsUpdated = true;
    }

    if (ledsUpdated)
    {
        setLeds(leds, CONFIG_NUM_LEDS);
    }
}

#endif
