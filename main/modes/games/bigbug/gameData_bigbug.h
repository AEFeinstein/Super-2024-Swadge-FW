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
#include "linked_list.h"
#include "soundManager_bigbug.h"

//==============================================================================
// Constants
//==============================================================================

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    uint16_t btnState;

    int32_t touchPhi;
    int32_t touchRadius;
    int32_t touchIntensity;
    int32_t isTouched;
    int32_t touchX;
    int32_t touchY;

    uint8_t gameState;
    uint8_t changeState;

    uint8_t harpoons;



    led_t leds[CONFIG_NUM_LEDS];

    paletteColor_t bgColor;

    bool debugMode;

    uint8_t changeBgm;
    uint8_t currentBgm;

    uint32_t inGameTimer;

    bb_soundManager_t* soundManager;

    int8_t neighbors[4][2];//a handy table of left, up, right, and down offsets

    list_t* check;//a list of tiles to check if they are supported.
    list_t* unsupported;//a list of tiles that dynamically crumble.
    
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