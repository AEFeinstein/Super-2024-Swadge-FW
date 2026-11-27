#pragma once

#include <stdint.h>
#include "dance.h"

typedef struct
{
    int32_t exciterTimer;
    int32_t exciterIdx;
    int16_t stripVals[4];
    bool exciterIncrementing;
    int32_t decayTimer;
    int32_t rgbAngle;
    uint32_t rainbowColor;
} danceSweepState_t;

void danceSweep(uint32_t tElapsedUs, uint32_t arg, bool reset, bool useColor, void* state);

#ifdef DANCE_IMPLEMENTATION

/**
 * @brief Sweep the LEDs left and right, like a Cylon
 *
 * @param tElapsedUs The time elapsed since last call, in microseconds
 * @param arg        The color for this sweep
 * @param reset      true to reset this dance's variables
 */
void danceSweep(uint32_t tElapsedUs, uint32_t arg, bool reset, bool useColor, void* state)
{
    danceSweepState_t* danceState = state;
    static const int8_t ledOrder[][4] = {
        {6, 11, 12, 13},
        {0, 4, 5, -1},
        {1, 2, 3, -1},
        {7, 8, 9, 10},
    };

    if (reset)
    {
        danceState->exciterTimer = 0;
        danceState->exciterIdx   = 0;
        memset(danceState->stripVals, 0, sizeof(danceState->stripVals));
        danceState->exciterIncrementing = true;
        danceState->decayTimer          = 0;
        danceState->rgbAngle            = 0;
        danceState->rainbowColor        = 0;
    }

    // Declare some LEDs, all off
    led_t leds[CONFIG_NUM_LEDS] = {{0}};
    bool ledsUpdated            = false;

    RUN_TIMER_EVERY(danceState->exciterTimer, 250000, tElapsedUs, {
        // Excite a set of LEDs
        danceState->stripVals[danceState->exciterIdx] = 0xFF;

        // Move the exciter
        if (danceState->exciterIncrementing)
        {
            danceState->exciterIdx++;
        }
        else
        {
            danceState->exciterIdx--;
        }

        // Flip directions at the end
        if (danceState->exciterIdx < 0)
        {
            danceState->exciterIncrementing = true;
            danceState->exciterIdx          = 1;
        }
        else if (danceState->exciterIdx >= ARRAY_SIZE(ledOrder))
        {
            danceState->exciterIncrementing = false;
            danceState->exciterIdx          = ARRAY_SIZE(ledOrder) - 2;
        }

        // Apply rainbow if there's no color
        if (!useColor)
        {
            danceState->rainbowColor = EHSVtoHEXhelper(danceState->rgbAngle, 0xFF, 0xFF, false);
            danceState->rgbAngle++;
            if (256 == danceState->rgbAngle)
            {
                danceState->rgbAngle = 0;
            }
        }
    });

    uint32_t color = useColor ? arg : danceState->rainbowColor;

    // Run a timer to decay the LEDs
    RUN_TIMER_EVERY(danceState->decayTimer, 2000, tElapsedUs, {
        // Decay the strip
        for (int32_t sIdx = 0; sIdx < ARRAY_SIZE(ledOrder); sIdx++)
        {
            danceState->stripVals[sIdx]--;
            if (danceState->stripVals[sIdx] < 0)
            {
                danceState->stripVals[sIdx] = 0;
            }

            for (int32_t lIdx = 0; lIdx < ARRAY_SIZE(ledOrder[0]); lIdx++)
            {
                int8_t numLed = ledOrder[sIdx][lIdx];
                if (0 <= numLed)
                {
                    leds[ledOrder[sIdx][lIdx]].r = (danceState->stripVals[sIdx] * ARG_R(color)) / 256;
                    leds[ledOrder[sIdx][lIdx]].g = (danceState->stripVals[sIdx] * ARG_G(color)) / 256;
                    leds[ledOrder[sIdx][lIdx]].b = (danceState->stripVals[sIdx] * ARG_B(color)) / 256;
                }
            }
            ledsUpdated = true;
        }
    });

    // Light the LEDs
    if (ledsUpdated)
    {
        setLeds(leds, CONFIG_NUM_LEDS);
    }
}

#endif
