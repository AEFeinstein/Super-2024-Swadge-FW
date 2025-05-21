#pragma once

#include <stdint.h>
#include "dance.h"

void danceCondiment(uint32_t tElapsedUs, uint32_t arg, bool reset);

#ifdef DANCE_IMPLEMENTATION

/**
 * @brief Run the LED along the condiment, then pulse the bun
 *
 * @param tElapsedUs The time elapsed since last call, in microseconds
 * @param arg        The base color to use
 * @param reset      true to reset this dance's variables
 */
void danceCondiment(uint32_t tElapsedUs, uint32_t arg, bool reset)
{
    static const int8_t pulseLeds[] = {0, 1, 2, 3, 4};
    static const int8_t stripLeds[] = {5, 6, 7, 8};

    static bool isPulse                             = false;
    static int16_t pulseVal                         = 0;
    static bool pulseRising                         = true;
    static int16_t stripVals[ARRAY_SIZE(stripLeds)] = {0};
    static int32_t stripExciter                     = 0;

    if (reset)
    {
        isPulse     = false;
        pulseVal    = 0;
        pulseRising = true;
        memset(stripVals, 0, sizeof(stripVals));
        stripExciter = 0;
    }

    // Declare some LEDs, all off
    led_t leds[CONFIG_NUM_LEDS] = {{0}};
    bool ledsUpdated            = false;

    // Run this code every frame
    static uint32_t condimentTimer = 0;
    RUN_TIMER_EVERY(condimentTimer, DEFAULT_FRAME_RATE_US, tElapsedUs, {
        if (isPulse)
        {
            if (pulseRising)
            {
                pulseVal += 8;
                if (0xFF <= pulseVal)
                {
                    pulseRising = false;
                    pulseVal    = 0xFF;
                }
            }
            else
            {
                pulseVal -= 8;
                if (0 >= pulseVal)
                {
                    isPulse     = false;
                    pulseRising = true;
                    pulseVal    = 0;
                }
            }

            // Light the pulse LEDs
            for (int32_t lIdx = 0; lIdx < ARRAY_SIZE(pulseLeds); lIdx++)
            {
                leds[lIdx].r = (pulseVal * ARG_R(arg)) / 256;
                leds[lIdx].g = (pulseVal * ARG_G(arg)) / 256;
                leds[lIdx].b = (pulseVal * ARG_B(arg)) / 256;
            }
            ledsUpdated = true;
        }
        else
        {
            // Run an exciter to lead the strip
            if (stripExciter % 8 == 0 && (stripExciter / 8) < ARRAY_SIZE(stripLeds))
            {
                stripVals[stripExciter / 8] = 0xFF;
            }
            stripExciter++;

            // Decay the strip
            bool someLedOn = false;
            for (int32_t lIdx = 0; lIdx < ARRAY_SIZE(stripLeds); lIdx++)
            {
                stripVals[lIdx] -= 8;
                if (stripVals[lIdx] < 0)
                {
                    stripVals[lIdx] = 0;
                }
                else
                {
                    someLedOn = true;
                }
                leds[stripLeds[lIdx]].r = (stripVals[lIdx] * ARG_R(arg)) / 256;
                leds[stripLeds[lIdx]].g = (stripVals[lIdx] * ARG_G(arg)) / 256;
                leds[stripLeds[lIdx]].b = (stripVals[lIdx] * ARG_B(arg)) / 256;
            }
            ledsUpdated = true;

            // All off, switch back to pulse
            if (!someLedOn)
            {
                isPulse      = true;
                stripExciter = 0;
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
