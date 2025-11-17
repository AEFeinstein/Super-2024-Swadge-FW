#ifndef _MG_GAMEDATA_H_
#define _MG_GAMEDATA_H_

//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>
#include "hdw-led.h"
#include "mega_pulse_ex_typedef.h"
// #include "swadge2024.h"
#include "palette.h"
#include "mgSoundManager.h"

//==============================================================================
// Constants
//==============================================================================
#define MG_DOUBLE_TAP_TIMER_FRAMES 20

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    int16_t btnState;
    int16_t prevBtnState;

    int16_t doubleTapBtnTimer;
    int16_t doubleTapBtnState;

    uint8_t gameState;
    uint8_t changeState;

    uint32_t score;
    uint8_t lives;
    uint8_t coins;
    int16_t countdown;
    uint16_t frameCount;

    uint8_t level;

    uint16_t combo;
    int16_t comboTimer;
    uint32_t comboScore;

    bool extraLifeCollected;
    uint8_t checkpoint;
    uint8_t levelDeaths;
    uint8_t initialHp;

    led_t leds[CONFIG_NUM_LEDS];

    const paletteColor_t* bgColors;

    char initials[3];
    uint8_t rank;
    bool debugMode;
    bool customLevel;

    int8_t changeBgm;
    uint8_t currentBgm;

    bool continuesUsed;
    uint32_t inGameTimer;

    mgSoundManager_t* soundManager;
} mgGameData_t;

//==============================================================================
// Functions
//==============================================================================
void mg_initializeGameData(mgGameData_t* gameData, mgSoundManager_t* soundManager);
void mg_initializeGameDataFromTitleScreen(mgGameData_t* gameData);
void mg_updateLedsHpMeter(mgEntityManager_t* entityManager, mgGameData_t* gameData);
void mg_scorePoints(mgGameData_t* gameData, uint16_t points);
void addCoins(mgGameData_t* gameData, uint8_t coins);
void updateComboTimer(mgGameData_t* gameData);
void mg_resetGameDataLeds(mgGameData_t* gameData);
void mg_updateLedsShowHighScores(mgGameData_t* gameData);
void mg_updateLedsLevelClear(mgGameData_t* gameData);
void mg_updateLedsGameClear(mgGameData_t* gameData);
void mg_updateLeds(mgEntityManager_t* entityManager);
void mg_updateLedsDead(mgGameData_t* gameData);
void mg_updateLedsGameOver(mgGameData_t* gameData);

#endif