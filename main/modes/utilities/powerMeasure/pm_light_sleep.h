#pragma once

#include "powerMeasure.h"

#ifdef PM_IMPLEMENTATION

void pmLightSleepEnterMode(powerMeasure_t* pmp);
void pmLightSleepMainLoop(int64_t elapsedUs, powerMeasure_t* pmp);
void pmLightSleepBtnCb(buttonEvt_t* evt, powerMeasure_t* pmp);

pmMode_t pmLightSleep_mode = {
    .fnEnterMode = pmLightSleepEnterMode,
    .fnMainLoop  = pmLightSleepMainLoop,
    .fnBtnCb     = pmLightSleepBtnCb,
};

void pmLightSleepEnterMode(powerMeasure_t* pm)
{
    turnPeripheralsOff();
}

void pmLightSleepMainLoop(int64_t elapsedUs, powerMeasure_t* pm)
{
    RUN_TIMER_EVERY(pm->stateTimer, 100000, elapsedUs, {
        enterSwadgepassSleep();
    });
}

void pmLightSleepBtnCb(buttonEvt_t* evt, powerMeasure_t* pm)
{
}

#endif