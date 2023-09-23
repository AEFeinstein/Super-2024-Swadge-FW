#ifndef _PL_GAMEDATA_H_
#define _PL_GAMEDATA_H_

//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>
#include "hdw-led.h"
#include "platformer_typedef.h"
//#include "swadgeMode.h"
#include "palette.h"
#include "plSoundManager.h"

//==============================================================================
// Constants
//==============================================================================
/*static const song_t snd1up =
{
    .notes = 
    {
        {G_7, 40},{D_6, 40},{B_5, 80}
    },
    .numNotes = 3,
    .shouldLoop = false
};*/

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
    uint8_t coins;
    int16_t countdown;
    uint16_t frameCount;

    uint8_t world;
    uint8_t level;

    uint16_t combo;
    int16_t comboTimer;
    uint32_t comboScore;

    bool extraLifeCollected;
    uint8_t checkpoint;
    uint8_t levelDeaths;
    uint8_t initialHp;
    
    led_t leds[CONFIG_NUM_LEDS];

    paletteColor_t bgColor;

    char initials[3];
    uint8_t rank;
    bool debugMode;

    uint8_t changeBgm;
    uint8_t currentBgm;

    bool continuesUsed;
    uint32_t inGameTimer;

    plSoundManager_t* soundManager;
} plGameData_t;

//==============================================================================
// Functions
//==============================================================================
void pl_initializeGameData(plGameData_t * gameData, plSoundManager_t * soundManager);
void pl_initializeGameDataFromTitleScreen(plGameData_t * gameData);
void pl_updateLedsHpMeter(plEntityManager_t *entityManager, plGameData_t *gameData);
void pl_scorePoints(plGameData_t * gameData, uint16_t points);
void addCoins(plGameData_t * gameData, uint8_t coins);
void updateComboTimer(plGameData_t * gameData);
void pl_resetGameDataLeds(plGameData_t * gameData);
void pl_updateLedsShowHighScores(plGameData_t * gameData);
void pl_updateLedsLevelClear(plGameData_t * gameData);
void pl_updateLedsGameClear(plGameData_t * gameData);
void pl_updateLedsGameOver(plGameData_t * gameData);

#endif