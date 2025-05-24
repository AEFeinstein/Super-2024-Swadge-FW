#pragma once

#include <stdint.h>
#include "dance.h"

void danceComet(uint32_t tElapsedUs, uint32_t arg, bool reset);

#ifdef DANCE_IMPLEMENTATION

/**
 * Rotate a single white LED around the swadge
 *
 * @param tElapsedUs The time elapsed since last call, in microseconds
 * @param reset      true to reset this dance's variables
 */
void danceComet(uint32_t tElapsedUs, uint32_t arg, bool reset)
{
    static int32_t ledCount            = 0;
    static uint8_t rainbow             = 0;
    static int32_t msCount             = 0;
    static uint32_t tAccumulated       = 0;
    static led_t leds[CONFIG_NUM_LEDS] = {{0}};

    if (reset)
    {
        ledCount     = 0;
        rainbow      = 0;
        msCount      = 80;
        tAccumulated = 2000;
        memset(leds, 0, sizeof(leds));
        return;
    }

    bool ledsUpdated = false;

    tAccumulated += tElapsedUs;
    while (tAccumulated >= 2000)
    {
        tAccumulated -= 2000;
        for (uint8_t i = 0; i < CONFIG_NUM_LEDS; i++)
        {
            if (leds[i].r > 0)
            {
                leds[i].r--;
            }
            if (leds[i].g > 0)
            {
                leds[i].g--;
            }
            if (leds[i].b > 0)
            {
                leds[i].b--;
            }
        }
        msCount++;

        if (msCount % 10 == 0)
        {
            rainbow++;
        }

        if (msCount >= 80)
        {
            if (0 == arg)
            {
                int32_t color    = EHSVtoHEXhelper(rainbow, 0xFF, 0xFF, false);
                leds[ledCount].r = (color >> 0) & 0xFF;
                leds[ledCount].g = (color >> 8) & 0xFF;
                leds[ledCount].b = (color >> 16) & 0xFF;
            }
            else
            {
                leds[ledCount].r = ARG_R(arg);
                leds[ledCount].g = ARG_G(arg);
                leds[ledCount].b = ARG_B(arg);
            }
            ledCount = (ledCount + 1) % CONFIG_NUM_LEDS;
            msCount  = 0;
        }
        ledsUpdated = true;
    }

    if (ledsUpdated)
    {
        setLeds(leds, CONFIG_NUM_LEDS);
    }
}

#endif
