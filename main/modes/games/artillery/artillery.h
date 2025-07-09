#pragma once

#include "swadge2024.h"
#include "artillery_phys.h"
#include "menuSimpleRenderer.h"

//==============================================================================
// Defines
//==============================================================================

#define NUM_PLAYERS 2

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    AMS_MENU,
    AMS_GAME,
} artilleryModeState_t;

typedef enum
{
    AGS_WAIT,
    AGS_MENU,
    AGS_MOVE,
    AGS_ADJUST,
    AGS_FIRE,
    AGS_LOOK,
} artilleryGameState_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    physSim_t* phys;
    physCirc_t* players[NUM_PLAYERS];
    int32_t autofire;
    artilleryModeState_t mState;
    artilleryGameState_t gState;
    uint8_t gMenuSel;
    menu_t* gameMenu;
    menuSimpleRenderer_t* smRenderer;
} artilleryData_t;

extern const char load_ammo[];
extern swadgeMode_t artilleryMode;