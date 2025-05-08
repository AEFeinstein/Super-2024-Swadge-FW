#pragma once

#include <stdint.h>
#include "dance.h"

void dancePulse(uint32_t tElapsedUs, uint32_t arg, bool reset);

#ifdef DANCE_IMPLEMENTATION

/**
 * Blink all LEDs red for on for 500ms, then off for 500ms
 *
 * @param tElapsedUs The time elapsed since last call, in microseconds
 * @param reset      true to reset this dance's variables
 */
void dancePulse(uint32_t tElapsedUs, uint32_t arg, bool reset)
{
    static uint8_t ledVal        = 0;
    static uint8_t randColor     = 0;
    static bool goingUp          = true;
    static uint32_t tAccumulated = 0;

    if (reset)
    {
        ledVal       = 0;
        randColor    = 0;
        goingUp      = true;
        tAccumulated = 5000;
        return;
    }

    // Declare some LEDs, all off
    led_t leds[CONFIG_NUM_LEDS] = {{0}};
    bool ledsUpdated            = false;

    tAccumulated += tElapsedUs;
    while (tAccumulated >= 5000)
    {
        tAccumulated -= 5000;

        if (goingUp)
        {
            ledVal++;
            if (255 == ledVal)
            {
                goingUp = false;
            }
        }
        else
        {
            ledVal--;
            if (0 == ledVal)
            {
                goingUp   = true;
                randColor = danceRand(256);
            }
        }

        for (int i = 0; i < CONFIG_NUM_LEDS; i++)
        {
            if (0 == arg)
            {
                int32_t color = EHSVtoHEXhelper(randColor, 0xFF, 0xFF, false);
                leds[i].r     = (ledVal * ((color >> 0) & 0xFF) >> 8);
                leds[i].g     = (ledVal * ((color >> 8) & 0xFF) >> 8);
                leds[i].b     = (ledVal * ((color >> 16) & 0xFF) >> 8);
            }
            else
            {
                leds[i].r = (ledVal * ARG_R(arg)) >> 8;
                leds[i].g = (ledVal * ARG_G(arg)) >> 8;
                leds[i].b = (ledVal * ARG_B(arg)) >> 8;
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
