#pragma once

#include "powerMeasure.h"

#ifdef PM_IMPLEMENTATION

void pmLedEnterMode(powerMeasure_t* pmp);
void pmLedMainLoop(int64_t elapsedUs, powerMeasure_t* pmp);
void pmLedBtnCb(buttonEvt_t* evt, powerMeasure_t* pmp);

pmMode_t pmLed_mode = {
    .fnEnterMode = pmLedEnterMode,
    .fnMainLoop  = pmLedMainLoop,
    .fnBtnCb     = pmLedBtnCb,
};

void pmLedEnterMode(powerMeasure_t* pm)
{
    turnPeripheralsOff();
}

void pmLedMainLoop(int64_t elapsedUs, powerMeasure_t* pm)
{
    const led_t colors[] = {
        {
            .r = 0xFF,
            .g = 0x00,
            .b = 0x00,
        },
        {
            .r = 0x00,
            .g = 0xFF,
            .b = 0x00,
        },
        {
            .r = 0x00,
            .g = 0x00,
            .b = 0xFF,
        },
        {
            .r = 0xFF,
            .g = 0xFF,
            .b = 0xFF,
        },
    };

    RUN_TIMER_EVERY(pm->stateTimer, 2000000, elapsedUs, {
        pm->state++;

        if (pm->state >= (8 * 4 * 2))
        {
            pm->state = 0;
        }

        led_t leds[CONFIG_NUM_LEDS] = {0};

        if (pm->state % 2 != 0)
        {
            int32_t idx        = pm->state / 2;
            leds[0]            = colors[idx % ARRAY_SIZE(colors)];
            int32_t brightness = 32 * (1 + idx / 4);

            for (int32_t lIdx = 0; lIdx < ARRAY_SIZE(leds); lIdx++)
            {
                leds[lIdx].r = (brightness * leds[0].r) / 256;
                leds[lIdx].g = (brightness * leds[0].g) / 256;
                leds[lIdx].b = (brightness * leds[0].b) / 256;
            }
        }

        setLeds(leds, CONFIG_NUM_LEDS);
    });
}

void pmLedBtnCb(buttonEvt_t* evt, powerMeasure_t* pm)
{
    if (evt->down)
    {
        turnPeripheralsOn();
        pm->cMode = NULL;
    }
}

#endif