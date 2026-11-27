#pragma once

#include <stdint.h>
#include "dance.h"

void hardwareIndices(uint32_t tElapsedUs, uint32_t arg, bool reset, bool useColor);

#ifdef DANCE_IMPLEMENTATION

/**
 * A snake travels through the hardware indices (at JFrye's request).
 *
 * @param tElapsedUs The time elapsed since last call, in microseconds
 * @param reset      true to reset this dance's variables
 */
void hardwareIndices(uint32_t tElapsedUs, uint32_t arg, bool reset, bool useColor)
{
    static int32_t ledCount      = 0;
    static uint32_t tAccumulated = 0;
    static int32_t head          = 0; // increments while less than CONFIG_NUM_LEDS
    static int32_t tail          = 0; // increments while head >= CONFIG_NUM_LEDS

    if (reset)
    {
        ledCount     = 0;
        tAccumulated = 300000;
        return;
    }

    // Declare some LEDs, all off
    led_t leds[CONFIG_NUM_LEDS] = {{0}};
    bool ledsUpdated            = false;

    tAccumulated += tElapsedUs;
    while (tAccumulated >= 300000)
    {
        tAccumulated -= 300000;
        ledsUpdated = true;

        if (head < CONFIG_NUM_LEDS)
        {
            head++;
        }
        else
        {
            if (tail < CONFIG_NUM_LEDS - 1)
            {
                tail++;
            }
            else
            {
                head = 0;
                tail = 0;
            }
        }

        ledCount = ledCount + 1;
        if (ledCount > CONFIG_NUM_LEDS - 1)
        {
            ledCount = 0;
        }

        uint8_t i;
        for (i = 0; i < CONFIG_NUM_LEDS; i++)
        {
            int16_t angle  = (((i * 256) / CONFIG_NUM_LEDS)) % 256;
            uint32_t color = EHSVtoHEXhelper(angle, 0xFF, 0xFF, false);

            uint8_t thisLed = (i + ledCount) % CONFIG_NUM_LEDS;
            if (thisLed >= tail && thisLed <= head)
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
