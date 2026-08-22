#pragma once
#include "swadge.h"
#include "gs_typedef.h"
#include "gs_entityManager.h"

//==============================================================================
// Enums
//==============================================================================

typedef enum __attribute__((packed))
{
    UI_MENU,
    UI_GAME,
} gs_Ui_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct gs_gameData_t
{
    const trophyData_t (*trophyData)[1];
    // Current UI being shown
    gs_Ui_t ui;
    // Main Menu
    menu_t* menu;
    menuMegaRenderer_t* menuRenderer;
    font_t font_gossip; // IBM VGA 8 font
    // All buttons states
    uint16_t btnState;
    // Momentary downpresses on each button
    uint16_t btnDownState;
    rectangle_t camera;
    int32_t elapsedUs; // Time elapsed since the last frame in microseconds
    gs_entityManager_t entityManager;
    gs_asset_t assets[NUM_ASSETS];

    cnfsFileIdx_t songs[1];
    int8_t currentSongIdx;
    midiFile_t songMidi;
    int32_t headroom; // volume fade from 0x4000 to 0x0
    bool songFading;

    // NVS related
    // Usage: The second bit of the first number is 1 if the player has seen gossip idx 1. All 1's if they've seen 0
    // thru 31. The first bit of the second number is 1 if the player has seen gossip idx 32. The size of the array
    // times 32 is how many messages can be saved.
    uint32_t gossipProgress[2];
} gs_gameData_t;

//==============================================================================
// Functions
//==============================================================================

void gs_setAssetMetaData(void);

//==============================================================================
// Externs
//==============================================================================

extern swadgeMode_t gossipStoneMode;
extern heatshrink_decoder* gs_hsd;
extern uint8_t* gs_decodeSpace;