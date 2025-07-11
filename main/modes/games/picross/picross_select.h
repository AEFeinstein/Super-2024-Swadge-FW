#ifndef _MODE_PICROSSLEVELSELECT_H_
#define _MODE_PICROSSLEVELSELECT_H_

#include "swadge2024.h"
#include "picross_consts.h"

typedef struct
{
    int8_t index;
    wsg_t levelWSG;
    wsg_t completedWSG;
    bool completed;
    const char* title;
} picrossLevelDef_t;

typedef struct
{
    font_t* game_font;
    font_t smallFont;
    picrossLevelDef_t* chosenLevel;
    uint8_t gridScale;
    int8_t hoverLevelIndex;
    int8_t hoverX;
    int8_t hoverY;
    uint8_t rows;
    uint8_t cols;
    uint16_t prevBtnState;
    uint16_t btnState;
    uint8_t paddingTop;
    uint8_t paddingLeft;
    uint8_t gap;
    int32_t currentIndex; // s32bit because its stored i an nvs
    wsg_t unknownPuzzle;
    bool allLevelsComplete;
    picrossLevelDef_t levels[PICROSS_LEVEL_COUNT];
} picrossLevelSelect_t;

void picrossStartLevelSelect(font_t* mmFont, picrossLevelDef_t levels[]);
void picrossLevelSelectLoop(int64_t elapsedUs);
void picrossLevelSelectButtonCb(buttonEvt_t* evt);
void picrossExitLevelSelect(void);

#endif