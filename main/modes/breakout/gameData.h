#ifndef _GAMEDATA_H_
#define _GAMEDATA_H_

//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>
#include <stdbool.h>
#include "hdw-led.h"
#include "common_typedef.h"
#include "palette.h"

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

    uint32_t score;
    uint8_t lives;

    int16_t countdown;
    uint16_t frameCount;

    uint16_t targetBlocksBroken;
    
    uint8_t level;

    int16_t combo;
    //int16_t comboTimer;
    uint32_t comboScore;

    entity_t* playerBombs[3];
    uint8_t playerBombsCount;
    uint8_t nextBombToDetonate;
    uint8_t nextBombSlot;
    uint8_t bombDetonateCooldown;

    int32_t touchPhi;
    int32_t touchRadius;
    int32_t touchIntensity;
    int32_t isTouched;
    int32_t touchX;
    int32_t touchY;

    led_t leds[8 /*CONFIG_NUM_LEDS*/];

    paletteColor_t bgColor;

    char initials[3];
    uint8_t rank;
    bool debugMode;

    uint8_t changeBgm;
    uint8_t currentBgm;

    bool continuesUsed;
    uint32_t inGameTimer;
} gameData_t;

//==============================================================================
// Functions
//==============================================================================
void initializeGameData(gameData_t * gameData);
void initializeGameDataFromTitleScreen(gameData_t * gameData);
void updateLedsHpMeter(entityManager_t *entityManager, gameData_t *gameData);
void scorePoints(gameData_t * gameData, uint16_t points, int16_t incCombo);
void resetGameDataLeds(gameData_t * gameData);
void updateLedsShowHighScores(gameData_t * gameData);
void updateLedsLevelClear(gameData_t * gameData);
void updateLedsGameClear(gameData_t * gameData);
void updateLedsGameOver(gameData_t * gameData);
void updateLedsInGame(gameData_t * gameData);
void updateTouchInput(gameData_t * gameData);

#endif