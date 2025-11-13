#ifndef PICROSS_MENU_H_
#define PICROSS_MENU_H_

#include "mode_picross.h"
#include "picross_select.h"

typedef enum
{
    PO_SHOW_HINTS  = 0,
    PO_SHOW_GUIDES = 1,
    PO_ANIMATE_BG  = 2,
    PO_MARK_X      = 3,
} picrossOption_t;

extern swadgeMode_t modePicross;
extern const char picrossCurrentPuzzleIndexKey[];
extern const char picrossSavedOptionsKey[];
extern const char picrossCompletedLevelData[];
extern const char picrossProgressData[];
extern const char picrossMarksData[];

void returnToPicrossMenu(void);
void returnToPicrossMenuFromGame(void);
// void returnToLevelSelect(void);
void selectPicrossLevel(picrossLevelDef_t* selectedLevel);
void exitTutorial(void);
void picrossSetSaveFlag(picrossOption_t pos, bool on);
bool picrossGetSaveFlag(picrossOption_t pos);
bool picrossGetLoadedSaveFlag(picrossOption_t pos);
void continueGame(void);
#endif