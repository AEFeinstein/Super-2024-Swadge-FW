#pragma once
#include "swadge.h"
#include "gs_typedef.h"
#include "gs_entityManager.h"

//==============================================================================
// Enums
//==============================================================================

typedef enum __attribute__((packed))
{
    GS_MENU_SUBMODE,
    GS_GOSSIP_SUBMODE,
    GS_AMA_SUBMODE,
    GS_PROPHECY_SUBMODE,
    GS_MOON_SUBMODE,
} gs_submode_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct gs_gameData_t
{
    const trophyData_t (*trophyData)[1];
    // Current stuff being shown
    gs_submode_t submode;
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
    // Usage: Each bit is a gossip seen or unseen.
    int32_t* gossipProgress;
} gs_gameData_t;

//==============================================================================
// Functions
//==============================================================================

void gs_populateMenu(void);
void gs_setAssetMetaData(void);

//==============================================================================
// Externs
//==============================================================================

extern swadgeMode_t gossipStoneMode;
extern heatshrink_decoder* gs_hsd;
extern uint8_t* gs_decodeSpace;