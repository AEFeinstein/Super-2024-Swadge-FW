//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include "gameData_bigbug.h"
#include "entityManager_bigbug.h"
#include "esp_random.h"
#include "hdw-btn.h"
#include "touchUtils.h"
#include "soundFuncs.h"

//==============================================================================
// Functions
//==============================================================================
void bb_initializeGameData(bb_gameData_t* gameData, bb_soundManager_t* soundManager)
{
    gameData->gameState = 0;

    gameData->bgColor     = c335;
    gameData->debugMode   = false;
    gameData->inGameTimer = 0;

    gameData->soundManager = soundManager;

    gameData->neighbors[0][0] = -1; // left  neighbor x offset
    gameData->neighbors[0][1] = 0;  // left  neighbor y offset
    gameData->neighbors[1][0] = 0;  // up    neighbor x offset
    gameData->neighbors[1][1] = -1; // up    neighbor y offset
    gameData->neighbors[2][0] = 1;  // right neighbor x offset
    gameData->neighbors[2][1] = 0;  // right neighbor y offset
    gameData->neighbors[3][0] = 0;  // down  neighbor x offset
    gameData->neighbors[3][1] = 1;  // down  neighbor y offset

    // Load font
    loadFont("ibm_vga8.font", &gameData->font, false);

    memset(&gameData->pleaseCheck, 0, sizeof(list_t));
    memset(&gameData->unsupported, 0, sizeof(list_t));
}

void bb_initializeGameDataFromTitleScreen(bb_gameData_t* gameData)
{
    gameData->gameState = 0;

    gameData->bgColor     = c000;
    gameData->currentBgm  = 0;
    gameData->changeBgm   = 0;
    gameData->inGameTimer = 0;

    bb_resetGameDataLeds(gameData);
}

void bb_updateLeds(bb_entityManager_t* entityManager, bb_gameData_t* gameData)
{
    if (entityManager->playerEntity == NULL)
    {
        return;
    }

    for (int32_t i = 1; i < 7; i++)
    {
        gameData->leds[i].r = 0x80;
        gameData->leds[i].g = 0x00;
        gameData->leds[i].b = 0x00;
    }

    setLeds(gameData->leds, CONFIG_NUM_LEDS);
}

void bb_resetGameDataLeds(bb_gameData_t* gameData)
{
    for (uint8_t i = 0; i < CONFIG_NUM_LEDS; i++)
    {
        gameData->leds[i].r = 0;
        gameData->leds[i].g = 0;
        gameData->leds[i].b = 0;
    }

    setLeds(gameData->leds, CONFIG_NUM_LEDS);
}

void bb_updateTouchInput(bb_gameData_t* gameData)
{
    if (getTouchJoystick(&(gameData->touchPhi), &(gameData->touchRadius), &(gameData->touchIntensity)))
    {
        gameData->isTouched = true;
        getTouchCartesian(gameData->touchPhi, gameData->touchRadius, &(gameData->touchX), &(gameData->touchY));
    }
    else
    {
        gameData->isTouched = false;
    }
}
