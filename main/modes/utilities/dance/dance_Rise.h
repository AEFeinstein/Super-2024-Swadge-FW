#pragma once

#include <stdint.h>
#include "dance.h"

void danceRise(uint32_t tElapsedUs, uint32_t arg, bool reset);

#ifdef DANCE_IMPLEMENTATION

/**
 * Rotate a single white LED around the swadge
 *
 * @param tElapsedUs The time elapsed since last call, in microseconds
 * @param reset      true to reset this dance's variables
 */
void danceRise(uint32_t tElapsedUs, uint32_t arg, bool reset)
{
    #define RISE_LEVELS 3
    static const int8_t ledsPerLevel[RISE_LEVELS][2] = {
        {4, 1},
        {3, 2},
        {5, 0},
    };

    static int16_t levels[RISE_LEVELS] = {0, -256, -512};
    static bool rising[RISE_LEVELS]    = {true, true, true};
    static uint8_t angle               = 0;
    static uint32_t tAccumulated       = 0;

    if (reset)
    {
        for (uint8_t i = 0; i < RISE_LEVELS; i++)
        {
            levels[i] = i * -256;
            rising[i] = true;
        }
        angle        = 0;
        tAccumulated = 800;
        return;
    }

    bool ledsUpdated            = false;
    led_t leds[CONFIG_NUM_LEDS] = {{0}};

    tAccumulated += tElapsedUs;
    while (tAccumulated >= 800)
    {
        tAccumulated -= 800;

        if (true == rising[0] && 0 == levels[0])
        {
            angle = danceRand(256);
        }

        for (uint8_t i = 0; i < RISE_LEVELS; i++)
        {
            if (rising[i])
            {
                levels[i]++;
                if (levels[i] == 255)
                {
                    rising[i] = false;
                }
            }
            else
            {
                levels[i]--;
                if (levels[i] == -512)
                {
                    rising[i] = true;
                }
            }
        }

        int32_t color;
        if (0 == arg)
        {
            color = EHSVtoHEXhelper(angle, 0xFF, 0xFF, false);
        }
        else
        {
            color = arg;
        }

        for (uint8_t i = 0; i < RISE_LEVELS; i++)
        {
            for (int8_t lIdx = 0; lIdx < ARRAY_SIZE(ledsPerLevel[0]); lIdx++)
            {
                int8_t ledNum = ledsPerLevel[i][lIdx];
                if (-1 != ledNum && levels[i] > 0)
                {
                    leds[ledNum].r = (levels[i] * ((color >> 16) & 0xFF) >> 8);
                    leds[ledNum].g = (levels[i] * ((color >> 8) & 0xFF) >> 8);
                    leds[ledNum].b = (levels[i] * ((color >> 0) & 0xFF) >> 8);
                }
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
