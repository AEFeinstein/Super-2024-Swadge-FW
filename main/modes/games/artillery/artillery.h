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
    // The physics simulation
    physSim_t* phys;

    // The players, pointers to objects in the simulation
    physCirc_t* players[NUM_PLAYERS];
    int32_t plIdx;

    // The mode state (i.e. main menu, connecting, game)
    artilleryModeState_t mState;

    // The game state (i.e. moving, adjusting shot, etc.)
    artilleryGameState_t gState;

    // In-game menu and renderer
    menu_t* gameMenu;
    menuSimpleRenderer_t* smRenderer;

} artilleryData_t;

extern const char load_ammo[];
extern swadgeMode_t artilleryMode;