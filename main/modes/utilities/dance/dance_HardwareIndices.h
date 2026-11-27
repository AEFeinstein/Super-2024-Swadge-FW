#pragma once

#include <stdint.h>
#include "dance.h"

typedef struct
{
    int32_t ledCount;
    uint32_t tAccumulated;
    int32_t head;
    int32_t tail;
} hardwareIndicesState_t;

void hardwareIndices(uint32_t tElapsedUs, uint32_t arg, bool reset, bool useColor, void* state);

#ifdef DANCE_IMPLEMENTATION

/**
 * A snake travels through the hardware indices (at JFrye's request).
 *
 * @param tElapsedUs The time elapsed since last call, in microseconds
 * @param reset      true to reset this dance's variables
 */
void hardwareIndices(uint32_t tElapsedUs, uint32_t arg, bool reset, bool useColor, void* state)
{
    hardwareIndicesState_t* danceState = state;

    if (reset)
    {
        danceState->ledCount     = 0;
        danceState->tAccumulated = 300000;
        return;
    }

    // Declare some LEDs, all off
    led_t leds[CONFIG_NUM_LEDS] = {{0}};
    bool ledsUpdated            = false;

    danceState->tAccumulated += tElapsedUs;
    while (danceState->tAccumulated >= 300000)
    {
        danceState->tAccumulated -= 300000;
        ledsUpdated = true;

        if (danceState->head < CONFIG_NUM_LEDS)
        {
            danceState->head++;
        }
        else
        {
            if (danceState->tail < CONFIG_NUM_LEDS - 1)
            {
                danceState->tail++;
            }
            else
            {
                danceState->head = 0;
                danceState->tail = 0;
            }
        }

        danceState->ledCount = danceState->ledCount + 1;
        if (danceState->ledCount > CONFIG_NUM_LEDS - 1)
        {
            danceState->ledCount = 0;
        }

        uint8_t i;
        for (i = 0; i < CONFIG_NUM_LEDS; i++)
        {
            int16_t angle  = (((i * 256) / CONFIG_NUM_LEDS)) % 256;
            uint32_t color = EHSVtoHEXhelper(angle, 0xFF, 0xFF, false);

            uint8_t thisLed = (i + danceState->ledCount) % CONFIG_NUM_LEDS;
            if (thisLed >= danceState->tail && thisLed <= danceState->head)
            {
                if (useColor)
                {
                    leds[thisLed].r = ARG_R(arg);
                    leds[thisLed].g = ARG_G(arg);
                    leds[thisLed].b = ARG_B(arg);
                }
                else
                {
                    leds[thisLed].r = (color >> 0) & 0xFF;
                    leds[thisLed].g = (color >> 8) & 0xFF;
                    leds[thisLed].b = (color >> 16) & 0xFF;
                }
            }
            else
            {
                leds[thisLed].r = 0x0;
                leds[thisLed].g = 0x0;
                leds[thisLed].b = 0x0;
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
