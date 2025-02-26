#pragma once

#include "swadge2024.h"

struct _powerMeasure_t;

typedef struct
{
    void (*fnEnterMode)(struct _powerMeasure_t* pmp);
    void (*fnMainLoop)(int64_t elapsedUs, struct _powerMeasure_t* pmp);
    void (*fnBtnCb)(buttonEvt_t* evt, struct _powerMeasure_t* pmp);
} pmMode_t;

typedef struct _powerMeasure_t
{
    menu_t* menu;
    menuManiaRenderer_t* renderer;
    font_t ibm;
    pmMode_t* cMode;
    int32_t stateTimer;
    int32_t state;
} powerMeasure_t;

extern swadgeMode_t powerMeasureMode;

void turnPeripheralsOff(void);
void turnPeripheralsOn(void);