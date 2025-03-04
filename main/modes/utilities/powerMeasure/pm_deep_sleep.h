#pragma once

#include "powerMeasure.h"

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
}

void pmDeepSleepMainLoop(int64_t elapsedUs, powerMeasure_t* pm)
{
    RUN_TIMER_EVERY(pm->stateTimer, 100000, elapsedUs, { enterSwadgepassSleep(); });
}

void pmDeepSleepBtnCb(buttonEvt_t* evt, powerMeasure_t* pm)
{
}

#endif