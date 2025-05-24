#pragma once

#include <stdint.h>
#include "dance.h"

void danceChristmas(uint32_t tElapsedUs, uint32_t arg, bool reset);

#ifdef DANCE_IMPLEMENTATION

/**
 * Holiday lights. Picks random target hues (red or green) or (blue or yellow) and saturations for
 * random LEDs at random intervals, then smoothly iterates towards those targets.
 * All LEDs are shown with a randomness added to their brightness for a little
 * sparkle
 *
 * @param tElapsedUs The time elapsed since last call, in microseconds
 * @param arg        unused
 * @param reset      true to reset this dance's variables
 */
void danceChristmas(uint32_t tElapsedUs, uint32_t arg, bool reset)
{
    static int32_t ledCount                                  = 0;
    static int32_t ledCount2                                 = 0;
    static uint8_t color_hue_save[CONFIG_NUM_LEDS]           = {0};
    static uint8_t color_saturation_save[CONFIG_NUM_LEDS]    = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    static uint8_t current_color_hue[CONFIG_NUM_LEDS]        = {0};
    static uint8_t current_color_saturation[CONFIG_NUM_LEDS] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    static uint8_t target_value[CONFIG_NUM_LEDS]             = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    static uint8_t current_value[CONFIG_NUM_LEDS]            = {0};

    static uint32_t tAccumulated      = 0;
    static uint32_t tAccumulatedValue = 0;

    if (reset)
    {
        ledCount  = 0;
        ledCount2 = 0;
        memset(color_saturation_save, 0xFF, sizeof(color_saturation_save));
        memset(current_color_saturation, 0xFF, sizeof(current_color_saturation));
        memset(target_value, 0xFF, sizeof(target_value));
        memset(current_value, 0x00, sizeof(current_value));
        if (arg)
        {
            memset(color_hue_save, 0, sizeof(color_hue_save));
            memset(current_color_hue, 0, sizeof(current_color_hue)); // All red
        }
        else
        {
            memset(color_hue_save, 171, sizeof(color_hue_save));
            memset(current_color_hue, 171, sizeof(current_color_hue)); // All blue
        }
        tAccumulated      = 0;
        tAccumulatedValue = 0;
        return;
    }

    // Declare some LEDs, all off
    led_t leds[CONFIG_NUM_LEDS] = {{0}};
    bool ledsUpdated            = false;

    // Run a faster loop for LED brightness updates, this gives a twinkling effect
    tAccumulatedValue += tElapsedUs;
    while (tAccumulatedValue > 3500)
    {
        tAccumulatedValue -= 3500;

        uint8_t i;
        for (i = 0; i < CONFIG_NUM_LEDS; i++)
        {
            if (current_value[i] == target_value[i])
            {
                if (0xFF == target_value[i])
                {
                    // Reached full bright, pick new target value
                    target_value[i] = danceRand(64) + 192;
                }
                else
                {
                    // Reached target value, reset target to full bright
                    target_value[i] = 0xFF;
                }
            }
            // Smoothly move to the target value
            else if (current_value[i] > target_value[i])
            {
                current_value[i] -= 1;
            }
            else // if (current_value[i] < target_value[i])
            {
                current_value[i] += 1;
            }
        }
    }

    // Run a slower loop for hue and saturation updates
    tAccumulated += tElapsedUs;
    while (tAccumulated > 7000)
    {
        tAccumulated -= 7000;

        ledCount += 1;
        if (ledCount > ledCount2)
        {
            ledCount         = 0;
            ledCount2        = danceRand(1000) + 50; // 350ms to 7350ms
            int color_picker = danceRand(CONFIG_NUM_LEDS - 1);
            int node_select  = danceRand(CONFIG_NUM_LEDS);

            if (color_picker < 4)
            {
                // Flip some color targets
                if (arg)
                {
                    if (color_hue_save[node_select] == 0) // red
                    {
                        color_hue_save[node_select] = 86; // green
                    }
                    else
                    {
                        color_hue_save[node_select] = 0; // red
                    }
                }
                else
                {
                    if (color_hue_save[node_select] == 171) // blue
                    {
                        color_hue_save[node_select] = 43; // yellow
                    }
                    else
                    {
                        color_hue_save[node_select] = 171; // blue
                    }
                }
                // Pick a random saturation target
                color_saturation_save[node_select] = danceRand(15) + 240;
            }
            else
            {
                // White-ish target
                color_saturation_save[node_select] = danceRand(25);
            }
        }

        uint8_t i;
        for (i = 0; i < CONFIG_NUM_LEDS; i++)
        {
            // Smoothly move hue to the target
            if (current_color_hue[i] > color_hue_save[i])
            {
                current_color_hue[i] -= 1;
            }
            else if (current_color_hue[i] < color_hue_save[i])
            {
                current_color_hue[i] += 1;
            }

            // Smoothly move saturation to the target
            if (current_color_saturation[i] > color_saturation_save[i])
            {
                current_color_saturation[i] -= 1;
            }
            else if (current_color_saturation[i] < color_saturation_save[i])
            {
                current_color_saturation[i] += 1;
            }
        }

        // Calculate actual LED values
        for (i = 0; i < CONFIG_NUM_LEDS; i++)
        {
            leds[i].r
                = (EHSVtoHEXhelper(current_color_hue[i], current_color_saturation[i], current_value[i], false) >> 0)
                  & 0xFF;
            leds[i].g
                = (EHSVtoHEXhelper(current_color_hue[i], current_color_saturation[i], current_value[i], false) >> 8)
                  & 0xFF;
            leds[i].b
                = (EHSVtoHEXhelper(current_color_hue[i], current_color_saturation[i], current_value[i], false) >> 16)
                  & 0xFF;
        }
        ledsUpdated = true;
    }
    // Output the LED data, actually turning them on
    if (ledsUpdated)
    {
        setLeds(leds, CONFIG_NUM_LEDS);
    }
}

#endif
