#ifndef _PA_GAMEDATA_H_
#define _PA_GAMEDATA_H_

//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>
#include "hdw-led.h"
#include "pango_typedef.h"
#include "palette.h"
#include "paSoundManager.h"
#include "font.h"

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
    uint32_t bonusScore;
    uint8_t lives;
    int16_t levelTime;
    uint16_t frameCount;

    uint8_t level;

    led_t leds[CONFIG_NUM_LEDS];

    paletteColor_t bgColor;

    char initials[3];
    uint8_t rank;
    bool debugMode;

    uint8_t changeBgm;
    uint8_t currentBgm;

    bool continuesUsed;
    uint32_t inGameTimer;

    int16_t maxActiveEnemies;
    int16_t remainingEnemies;
    int16_t enemyInitialSpeed;
    int16_t minAggroEnemies;
    int16_t maxAggroEnemies;

    paSoundManager_t* soundManager;

    font_t scoreFont;
} paGameData_t;

//==============================================================================
// Functions
//==============================================================================
void pa_initializeGameData(paGameData_t* gameData, paSoundManager_t* soundManager);
void pa_initializeGameDataFromTitleScreen(paGameData_t* gameData, uint16_t levelIndex);
void pa_updateLedsHpMeter(paEntityManager_t* entityManager, paGameData_t* gameData);
void pa_scorePoints(paGameData_t* gameData, uint16_t points);
void addCoins(paGameData_t* gameData, uint8_t coins);
void updateComboTimer(paGameData_t* gameData);
void pa_resetGameDataLeds(paGameData_t* gameData);
void pa_updateLedsShowHighScores(paGameData_t* gameData);
void pa_updateLedsLevelClear(paGameData_t* gameData);
void pa_updateLedsGameClear(paGameData_t* gameData);
void pa_updateLedsGameOver(paGameData_t* gameData);
void pa_setDifficultyLevel(paGameData_t* gameData, uint16_t levelIndex);

#endif