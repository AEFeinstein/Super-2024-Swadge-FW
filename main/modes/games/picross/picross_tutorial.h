#ifndef _MODE_PICROSSTUTORIAL_H_
#define _MODE_PICROSSTUTORIAL_H_

#include "swadgeMode.h"
#include "aabb_utils.h"

typedef struct
{
    display_t* d;
    font_t titleFont;
    font_t smallFont;
    wsg_t qrlink;
    uint8_t totalPages;
    uint8_t pageIndex;
    uint16_t btn;
    uint16_t prevBtn;
} picrossTutorial_t;



void picrossStartTutorial(display_t* disp, font_t* font);
void picrossTutorialLoop(int64_t elapsedUs);
void picrossTutorialButtonCb(buttonEvt_t* evt);
void picrossExitTutorial(void);

#endif