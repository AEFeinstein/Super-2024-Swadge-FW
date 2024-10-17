#ifndef _GAMEDATA_BIGBUG_H_
#define _GAMEDATA_BIGBUG_H_

//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>
#include <stdbool.h>
#include <esp_heap_caps.h>
#include "hdw-led.h"
#include "typedef_bigbug.h"
#include "entityManager_bigbug.h"
#include "tilemap_bigbug.h"
#include "palette.h"
#include "linked_list.h"
#include "soundManager_bigbug.h"

//==============================================================================
// Constants
//==============================================================================

//==============================================================================
// Structs
//==============================================================================

struct bb_gameData_t
{
    int32_t elapsedUs;

    uint16_t btnState;

    int32_t touchPhi;
    int32_t touchRadius;
    int32_t touchIntensity;
    int32_t isTouched;
    int32_t touchX;
    int32_t touchY;

    rectangle_t camera;

    uint8_t gameState;
    uint8_t changeState;

    bb_entityManager_t entityManager;

    led_t leds[CONFIG_NUM_LEDS];

    paletteColor_t bgColor;

    bool debugMode;

    uint8_t changeBgm;
    uint8_t currentBgm;

    uint32_t inGameTimer;

    bb_soundManager_t* soundManager;

    bb_tilemap_t tilemap;

    int8_t neighbors[4][2]; // a handy table of left, up, right, and down offsets

    list_t pleaseCheck; // a list of tiles to check if they are supported.
    list_t unsupported; // a list of tiles that flood-fill crumble.

    font_t font;
};

//==============================================================================
// Functions
//==============================================================================
void bb_initializeGameData(bb_gameData_t* gameData, bb_soundManager_t* soundManager);
void bb_initializeGameDataFromTitleScreen(bb_gameData_t* gameData);
void bb_updateLeds(bb_entityManager_t* entityManager, bb_gameData_t* gameData);
void bb_resetGameDataLeds(bb_gameData_t* gameData);
void bb_updateTouchInput(bb_gameData_t* gameData);

#endif