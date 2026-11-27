#pragma once

#include <stdint.h>
#include "dance.h"

typedef struct
{
    bool isPulse;
    int16_t pulseVal;
    bool pulseRising;
    int16_t stripVals[4];
    int32_t stripExciter;
    uint32_t condimentTimer;
} danceCondimentState_t;

void danceCondiment(uint32_t tElapsedUs, uint32_t arg, bool reset, bool useColor __attribute__((unused)), void* state);

#ifdef DANCE_IMPLEMENTATION

/**
 * @brief Run the LED along the condiment, then pulse the bun
 *
 * @param tElapsedUs The time elapsed since last call, in microseconds
 * @param arg        The base color to use
 * @param reset      true to reset this dance's variables
 */
void danceCondiment(uint32_t tElapsedUs, uint32_t arg, bool reset, bool useColor __attribute__((unused)), void* state)
{
    danceCondimentState_t* danceState = state;
    static const int8_t pulseLeds[] = {0, 1, 2, 3, 4};
    static const int8_t stripLeds[] = {5, 6, 7, 8};

    if (reset)
    {
        danceState->isPulse      = false;
        danceState->pulseVal     = 0;
        danceState->pulseRising  = true;
        memset(danceState->stripVals, 0, sizeof(danceState->stripVals));
        danceState->stripExciter = 0;
    }

    // Declare some LEDs, all off
    led_t leds[CONFIG_NUM_LEDS] = {{0}};
    bool ledsUpdated            = false;

    // Run this code every frame
    RUN_TIMER_EVERY(danceState->condimentTimer, DEFAULT_FRAME_RATE_US, tElapsedUs, {
        if (danceState->isPulse)
        {
            if (danceState->pulseRising)
            {
                danceState->pulseVal += 8;
                if (0xFF <= danceState->pulseVal)
                {
                    danceState->pulseRising = false;
                    danceState->pulseVal    = 0xFF;
                }
            }
            else
            {
                danceState->pulseVal -= 8;
                if (0 >= danceState->pulseVal)
                {
                    danceState->isPulse     = false;
                    danceState->pulseRising = true;
                    danceState->pulseVal    = 0;
                }
            }

            // Light the pulse LEDs
            for (int32_t lIdx = 0; lIdx < ARRAY_SIZE(pulseLeds); lIdx++)
            {
                leds[lIdx].r = (danceState->pulseVal * ARG_R(arg)) / 256;
                leds[lIdx].g = (danceState->pulseVal * ARG_G(arg)) / 256;
                leds[lIdx].b = (danceState->pulseVal * ARG_B(arg)) / 256;
            }
            ledsUpdated = true;
        }
        else
        {
            // Run an exciter to lead the strip
            if (danceState->stripExciter % 8 == 0 && (danceState->stripExciter / 8) < ARRAY_SIZE(stripLeds))
            {
                danceState->stripVals[danceState->stripExciter / 8] = 0xFF;
            }
            danceState->stripExciter++;

            // Decay the strip
            bool someLedOn = false;
            for (int32_t lIdx = 0; lIdx < ARRAY_SIZE(stripLeds); lIdx++)
            {
                danceState->stripVals[lIdx] -= 8;
                if (danceState->stripVals[lIdx] < 0)
                {
                    danceState->stripVals[lIdx] = 0;
                }
                else
                {
                    someLedOn = true;
                }
                leds[stripLeds[lIdx]].r = (danceState->stripVals[lIdx] * ARG_R(arg)) / 256;
                leds[stripLeds[lIdx]].g = (danceState->stripVals[lIdx] * ARG_G(arg)) / 256;
                leds[stripLeds[lIdx]].b = (danceState->stripVals[lIdx] * ARG_B(arg)) / 256;
            }
            ledsUpdated = true;

            // All off, switch back to pulse
            if (!someLedOn)
            {
                danceState->isPulse      = true;
                danceState->stripExciter = 0;
            }
        }
    });

    // Light the LEDs
    if (ledsUpdated)
    {
        setLeds(leds, CONFIG_NUM_LEDS);
    }
}

#endif
