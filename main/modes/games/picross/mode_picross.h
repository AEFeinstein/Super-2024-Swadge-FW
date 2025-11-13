#ifndef _MODE_PICROSS_H_
#define _MODE_PICROSS_H_

#include "swadge2024.h"
#include "picross_select.h"

typedef struct
{
    int32_t x0;
    int32_t y0;
    int32_t x1;
    int32_t y1;
} box_t;

typedef enum
{
    SPACE_EMPTY     = 0, // 00
    SPACE_FILLED    = 1, // 01
    SPACE_MARKEMPTY = 2, // 10
    OUTOFBOUNDS     = 3  // 11
} picrossSpaceType_t;

typedef enum
{
    PICROSS_SOLVING   = 0,
    PICROSS_YOUAREWIN = 1,
} picrossGamePhase_t;

typedef enum
{
    PICROSSDIR_IDLE  = 0,
    PICROSSDIR_LEFT  = 1,
    PICROSSDIR_RIGHT = 2,
    PICROSSDIR_DOWN  = 3,
    PICROSSDIR_UP    = 4,
} picrossDir_t; // this could be made generic and used for counter or

typedef struct
{
    bool victories[PICROSS_LEVEL_COUNT];
} picrossVictoryData_t;

typedef struct
{
    // input x position
    // input y position
    //  uint8_t currentIndex;
    picrossSpaceType_t level[PICROSS_MAX_LEVELSIZE][PICROSS_MAX_LEVELSIZE];
} picrossProgressData_t;

typedef struct
{
    picrossSpaceType_t startHeldType;
    bool startTentativeMarkType;
    uint8_t x;
    uint8_t y;
    uint8_t hoverBlockSizeX;
    uint8_t hoverBlockSizeY;
    uint16_t prevBtnState;
    uint16_t btnState;
    bool prevTouchState;
    bool touchState;
    bool movedThisFrame;
    bool changedLevelThisFrame;
    int64_t timeHeldDirection;
    picrossDir_t holdingDir;
    int64_t DASTime;
    int64_t firstDASTime;
    paletteColor_t inputBoxColor;
    paletteColor_t inputBoxDefaultColor;
    paletteColor_t inputBoxErrorColor;
    paletteColor_t markXColor;
    // blinking
    bool blinkError;
    uint64_t blinkAnimTimer;
    uint64_t blinkTime; // half a blink cycle (on)(off) or full (on/off)(on/off)?
    uint8_t blinkCount;
    bool showHints;
    bool showGuides;
    bool DASActive; // true after the first DAS input has happened.
} picrossInput_t;

typedef struct
{
    bool filledIn;
    bool correct;
    bool complete;
    bool isRow;
    uint8_t index;
    uint8_t hints[PICROSS_MAX_HINTCOUNT]; // have to deal with 'flexible array member'
} picrossHint_t;

typedef struct
{
    uint8_t width;
    uint8_t height;
    picrossHint_t rowHints[PICROSS_MAX_LEVELSIZE];
    picrossHint_t colHints[PICROSS_MAX_LEVELSIZE];
    picrossSpaceType_t completeLevel[PICROSS_MAX_LEVELSIZE][PICROSS_MAX_LEVELSIZE];
    picrossSpaceType_t level[PICROSS_MAX_LEVELSIZE][PICROSS_MAX_LEVELSIZE];
} picrossPuzzle_t;

typedef struct
{
    picrossGamePhase_t previousPhase;
    picrossGamePhase_t currentPhase;
    font_t hintFont;
    font_t UIFont;
    uint16_t vFontPad;
    picrossPuzzle_t* puzzle;
    bool controlsEnabled;
    picrossInput_t* input;
    uint16_t drawScale;
    uint16_t leftPad;
    uint16_t topPad;
    uint8_t maxHintsX;
    uint8_t maxHintsY;
    uint8_t clueGap;
    uint64_t bgScrollTimer;
    uint64_t bgScrollSpeed;
    uint8_t bgScrollXFrame;
    uint8_t bgScrollYFrame;
    bool animateBG;
    bool fadeHints;
    bool markX; // as x or as solid
    picrossLevelDef_t selectedLevel;
    bool exitThisFrame;
    int8_t count;
    picrossDir_t countState;
    led_t errorALEDBlinkLEDS[CONFIG_NUM_LEDS];
    led_t errorBLEDBlinkLEDS[CONFIG_NUM_LEDS];
    led_t offLEDS[CONFIG_NUM_LEDS];
    uint8_t ledAnimCount;      // victory dance
    uint32_t animtAccumulated; // victory dance
    bool tentativeMarks[PICROSS_MAX_LEVELSIZE][PICROSS_MAX_LEVELSIZE];
    int64_t elapsedUs;      // for sliding to center after winning.
    int8_t loopingTimer;    // for flashing arrows after winning.
    int8_t offsetX;         // for sliding to center after winning.
    int8_t offsetY;         // for sliding to center after winning.
    uint16_t lerpAmount;    // for sliding to center after winning.
    int32_t marqueeScrollX; // for the marquee text
} picrossGame_t;

void picrossStartGame(font_t* mmFont, picrossLevelDef_t* selectedLevel, bool cont);
void picrossGameLoop(int64_t elapsedUs);
void picrossGameButtonCb(buttonEvt_t* evt);
void picrossGameTouchCb(bool touched);
void picrossExitGame(void);
void loadPicrossProgress(void);
void savePicrossProgress(void);
void drawBox(box_t box, paletteColor_t color, bool isFilled, int32_t scalingFactor);

#endif