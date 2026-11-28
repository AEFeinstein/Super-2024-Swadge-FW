#pragma once

#include <stdint.h>
#include "dance.h"

typedef struct
{
    int32_t ledCount;
    int32_t ledCount2;
    uint8_t color_hue_save[CONFIG_NUM_LEDS];
    uint8_t color_saturation_save[CONFIG_NUM_LEDS];
    uint8_t current_color_hue[CONFIG_NUM_LEDS];
    uint8_t current_color_saturation[CONFIG_NUM_LEDS];
    uint8_t target_value[CONFIG_NUM_LEDS];
    uint8_t current_value[CONFIG_NUM_LEDS];
    uint32_t tAccumulated;
    uint32_t tAccumulatedValue;
} danceChristmasState_t;

void danceChristmas(uint32_t tElapsedUs, uint32_t arg, bool reset, bool useColor __attribute__((unused)), void* state);

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
void danceChristmas(uint32_t tElapsedUs, uint32_t arg, bool reset, bool useColor __attribute__((unused)), void* state)
{
    danceChristmasState_t* danceState = state;

    if (reset)
    {
        danceState->ledCount  = 0;
        danceState->ledCount2 = 0;
        memset(danceState->color_saturation_save, 0xFF, sizeof(danceState->color_saturation_save));
        memset(danceState->current_color_saturation, 0xFF, sizeof(danceState->current_color_saturation));
        memset(danceState->target_value, 0xFF, sizeof(danceState->target_value));
        memset(danceState->current_value, 0x00, sizeof(danceState->current_value));
        if (arg)
        {
            memset(danceState->color_hue_save, 0, sizeof(danceState->color_hue_save));
            memset(danceState->current_color_hue, 0, sizeof(danceState->current_color_hue)); // All red
        }
        else
        {
            memset(danceState->color_hue_save, 171, sizeof(danceState->color_hue_save));
            memset(danceState->current_color_hue, 171, sizeof(danceState->current_color_hue)); // All blue
        }
        danceState->tAccumulated      = 0;
        danceState->tAccumulatedValue = 0;
        return;
    }

    // Declare some LEDs, all off
    led_t leds[CONFIG_NUM_LEDS] = {{0}};
    bool ledsUpdated            = false;

    // Run a faster loop for LED brightness updates, this gives a twinkling effect
    danceState->tAccumulatedValue += tElapsedUs;
    while (danceState->tAccumulatedValue > 3500)
    {
        danceState->tAccumulatedValue -= 3500;

        uint8_t i;
        for (i = 0; i < CONFIG_NUM_LEDS; i++)
        {
            if (danceState->current_value[i] == danceState->target_value[i])
            {
                if (0xFF == danceState->target_value[i])
                {
                    // Reached full bright, pick new target value
                    danceState->target_value[i] = danceRand(64) + 192;
                }
                else
                {
                    // Reached target value, reset target to full bright
                    danceState->target_value[i] = 0xFF;
                }
            }
            // Smoothly move to the target value
            else if (danceState->current_value[i] > danceState->target_value[i])
            {
                danceState->current_value[i] -= 1;
            }
            else // if (current_value[i] < target_value[i])
            {
                danceState->current_value[i] += 1;
            }
        }
    }

    // Run a slower loop for hue and saturation updates
    danceState->tAccumulated += tElapsedUs;
    while (danceState->tAccumulated > 7000)
    {
        danceState->tAccumulated -= 7000;

        danceState->ledCount += 1;
        if (danceState->ledCount > danceState->ledCount2)
        {
            danceState->ledCount  = 0;
            danceState->ledCount2 = danceRand(1000) + 50; // 350ms to 7350ms
            int color_picker      = danceRand(CONFIG_NUM_LEDS - 1);
            int node_select       = danceRand(CONFIG_NUM_LEDS);

            if (color_picker < 4)
            {
                // Flip some color targets
                if (arg)
                {
                    if (danceState->color_hue_save[node_select] == 0) // red
                    {
                        danceState->color_hue_save[node_select] = 86; // green
                    }
                    else
                    {
                        danceState->color_hue_save[node_select] = 0; // red
                    }
                }
                else
                {
                    if (danceState->color_hue_save[node_select] == 171) // blue
                    {
                        danceState->color_hue_save[node_select] = 43; // yellow
                    }
                    else
                    {
                        danceState->color_hue_save[node_select] = 171; // blue
                    }
                }
                // Pick a random saturation target
                danceState->color_saturation_save[node_select] = danceRand(15) + 240;
            }
            else
            {
                // White-ish target
                danceState->color_saturation_save[node_select] = danceRand(25);
            }
        }

        uint8_t i;
        for (i = 0; i < CONFIG_NUM_LEDS; i++)
        {
            // Smoothly move hue to the target
            if (danceState->current_color_hue[i] > danceState->color_hue_save[i])
            {
                danceState->current_color_hue[i] -= 1;
            }
            else if (danceState->current_color_hue[i] < danceState->color_hue_save[i])
            {
                danceState->current_color_hue[i] += 1;
            }

            // Smoothly move saturation to the target
            if (danceState->current_color_saturation[i] > danceState->color_saturation_save[i])
            {
                danceState->current_color_saturation[i] -= 1;
            }
            else if (danceState->current_color_saturation[i] < danceState->color_saturation_save[i])
            {
                danceState->current_color_saturation[i] += 1;
            }
        }

        // Calculate actual LED values
        for (i = 0; i < CONFIG_NUM_LEDS; i++)
        {
            leds[i].r = (EHSVtoHEXhelper(danceState->current_color_hue[i], danceState->current_color_saturation[i],
                                         danceState->current_value[i], false)
                         >> 0)
                        & 0xFF;
            leds[i].g = (EHSVtoHEXhelper(danceState->current_color_hue[i], danceState->current_color_saturation[i],
                                         danceState->current_value[i], false)
                         >> 8)
                        & 0xFF;
            leds[i].b = (EHSVtoHEXhelper(danceState->current_color_hue[i], danceState->current_color_saturation[i],
                                         danceState->current_value[i], false)
                         >> 16)
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
