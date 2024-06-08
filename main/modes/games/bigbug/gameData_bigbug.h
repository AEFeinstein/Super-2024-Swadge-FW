#ifndef _GAMEDATA_BIGBUG_H_
#define _GAMEDATA_BIGBUG_H_

//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>
#include <stdbool.h>
#include "hdw-led.h"
#include "typedef_bigbug.h"
#include "palette.h"
#include "soundManager_bigbug.h"

//==============================================================================
// Constants
//==============================================================================

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    int16_t btnState;
    int16_t prevBtnState;
    uint8_t gameState;
    uint8_t changeState;

    uint8_t harpoons;

    int32_t touchPhi;
    int32_t touchRadius;
    int32_t touchIntensity;
    int32_t isTouched;
    int32_t touchX;
    int32_t touchY;

    led_t leds[CONFIG_NUM_LEDS];

    paletteColor_t bgColor;

    bool debugMode;

    uint8_t changeBgm;
    uint8_t currentBgm;

    uint32_t inGameTimer;

    bb_soundManager_t* soundManager;
} bb_gameData_t;

//==============================================================================
// Functions
//==============================================================================
void bb_initializeGameData(bb_gameData_t* gameData, bb_soundManager_t* soundManager);
void bb_initializeGameDataFromTitleScreen(bb_gameData_t* gameData);
void bb_updateLedsHpMeter(bb_entityManager_t* entityManager, bb_gameData_t* gameData);
void bb_resetGameDataLeds(bb_gameData_t* gameData);
void bb_updateTouchInput(bb_gameData_t* gameData);

#endif