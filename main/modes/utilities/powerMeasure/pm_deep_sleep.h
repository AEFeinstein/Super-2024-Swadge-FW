#pragma once

#include "powerMeasure.h"
#include "esp_sleep.h"
#include "driver/rtc_io.h"

#ifdef PM_IMPLEMENTATION

void pmDeepSleepEnterMode(powerMeasure_t* pmp);
void pmDeepSleepMainLoop(int64_t elapsedUs, powerMeasure_t* pmp);
void pmDeepSleepBtnCb(buttonEvt_t* evt, powerMeasure_t* pmp);
static void wakeFromDeepSleep(void);

pmMode_t pmDeepSleep_mode = {
    .fnEnterMode = pmDeepSleepEnterMode,
    .fnMainLoop  = pmDeepSleepMainLoop,
    .fnBtnCb     = pmDeepSleepBtnCb,
};

void pmDeepSleepEnterMode(powerMeasure_t* pm)
{
    turnPeripheralsOff();
}

void pmDeepSleepMainLoop(int64_t elapsedUs, powerMeasure_t* pm)
{
    RUN_TIMER_EVERY(pm->stateTimer, 100000, elapsedUs, {
        // Wake when a button is pressed
        uint64_t ioMask = 0;
        for (int32_t pbIdx = 0; pbIdx < ARRAY_SIZE(pushButtons); pbIdx++)
        {
            ioMask |= (1 << pushButtons[pbIdx]);
            rtc_gpio_pullup_en(pushButtons[pbIdx]);
            rtc_gpio_pulldown_dis(pushButtons[pbIdx]);
        }
        esp_sleep_enable_ext1_wakeup_io(ioMask, ESP_EXT1_WAKEUP_ANY_LOW);

        // TODO worth isolating any GPIO?
        // rtc_gpio_isolate(gpio);

        // Wake in five seconds
        esp_sleep_enable_timer_wakeup(1000 * 1000 * 5);

        // Disable some logging
        esp_deep_sleep_disable_rom_logging();

        // Set function to fix GPIO after waking
        esp_set_deep_sleep_wake_stub(wakeFromDeepSleep);

        // Sleep
        esp_deep_sleep_start();
    });
}

static void wakeFromDeepSleep(void)
{
    uint64_t ioMask = 0;
    for (int32_t pbIdx = 0; pbIdx < ARRAY_SIZE(pushButtons) / 2; pbIdx++)
    {
        rtc_gpio_deinit(pushButtons[pbIdx]);
    }
}

void pmDeepSleepBtnCb(buttonEvt_t* evt, powerMeasure_t* pm)
{
}

#endif