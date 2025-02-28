#pragma once

#include "powerMeasure.h"
#include "esp_sleep.h"
#include "driver/rtc_io.h"

#ifdef PM_IMPLEMENTATION

void pmDeepSleepEnterMode(powerMeasure_t* pmp);
void pmDeepSleepMainLoop(int64_t elapsedUs, powerMeasure_t* pmp);
void pmDeepSleepBtnCb(buttonEvt_t* evt, powerMeasure_t* pmp);

pmMode_t pmDeepSleep_mode = {
    .fnEnterMode = pmDeepSleepEnterMode,
    .fnMainLoop  = pmDeepSleepMainLoop,
    .fnBtnCb     = pmDeepSleepBtnCb,
};

void pmDeepSleepEnterMode(powerMeasure_t* pm)
{
    turnPeripheralsOff();

    // Necessary?
    // esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);

    // Either set GPIO as a wakeup source or isolate it
    uint64_t ioMask = 0;
    for(gpio_num_t gpio = GPIO_NUM_0; gpio < GPIO_NUM_MAX; gpio++)
    {
        bool isPb = false;
        for(int32_t pbIdx = 0; pbIdx < ARRAY_SIZE(pushButtons); pbIdx++)
        {
            if (gpio == pushButtons[pbIdx])
            {
                isPb = true;
                break;
            }
        }    

        if(isPb)
        {
            // Can only use GPIO 0-21
            ioMask |= (1 << gpio);

            rtc_gpio_pullup_en(gpio);
            rtc_gpio_pulldown_dis(gpio);

            // EXT0
            // rtc_gpio_pullup_en(pushButtons[gIdx]);
            // esp_sleep_enable_ext0_wakeup(pushButtons[gIdx], 0);
        }
        else
        {
            // TODO this causes major screen glitches
            // rtc_gpio_isolate(gpio);
        }
    }

    // TODO this is waking up the ESP immediately?
    // esp_sleep_enable_ext1_wakeup(ioMask, ESP_EXT1_WAKEUP_ANY_LOW);
    esp_sleep_enable_timer_wakeup(1000 * 1000 * 5); // microseconds
    esp_deep_sleep_disable_rom_logging();
    esp_deep_sleep_start();

    // TODO call rtc_gpio_deinit() after wake
    // TODO call esp_set_deep_sleep_wake_stub()
}

void pmDeepSleepMainLoop(int64_t elapsedUs, powerMeasure_t* pm)
{
}

void pmDeepSleepBtnCb(buttonEvt_t* evt, powerMeasure_t* pm)
{
}

#endif