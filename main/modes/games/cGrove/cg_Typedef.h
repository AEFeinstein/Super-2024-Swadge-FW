/**
 * @file cg_Typedef.h
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief Types for Chowa Grove
 * @version 0.1
 * @date 2024-09-19
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#pragma once

//==============================================================================
// Defines
//==============================================================================

#include "swadge2024.h"

//==============================================================================
// Defines
//==============================================================================

// Asset Counts
#define CG_CHOWA_EXPRESSION_COUNT 3
#define CG_GARDEN_ITEMS_COUNT 1
#define CG_GARDEN_STATIC_OBJECTS 1
#define CG_GARDEN_CURSORS 1

// Grove
#define CG_OBJ_COUNT 64
#define CG_FIELD_BOUNDARY 32
#define CG_FIELD_ITEM_LIMIT 10

// Field
#define CG_FIELD_HEIGHT 750
#define CG_FIELD_WIDTH 750

// Chowa
#define CG_CHOWA_COLORS 6
#define CG_MAX_CHOWA 10

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    CG_NEUTRAL,
    CG_HAPPY,
    CG_WORRIED,
    CG_SAD,
    CG_ANGRY,
    CG_CONFUSED,
    CG_SURPRISED,
    CG_SICK,
} cgMoodEnum_t;

typedef enum
{
    CHOWA_STATIC,
    CHOWA_WANDER,
    CHOWA_CHASE,
    CHOWA_HELD,
} cgChowaGardenState_t;

//==============================================================================
// Chowa
//==============================================================================

typedef struct 
{
    char name[16];
    rectangle_t aabb;
    wsg_t spr;
    bool active;
} cgItem_t;

typedef struct 
{
    rectangle_t aabb;
    cgMoodEnum_t mood;
    bool active;

    // Garden
    bool holdingItem;
    cgItem_t* heldItem;
    cgChowaGardenState_t gState;
    vec_t targetPos;
    int32_t waitTimer;
} cgChowa_t;

//==============================================================================
// Garden
//==============================================================================

typedef struct
{
    rectangle_t aabb;
} cgGardenCamera_t;

typedef struct 
{
    wsg_t spr;
    rectangle_t aabb;
} cgGardenObject_t;

typedef struct 
{
    cgGardenObject_t staticObjects[CG_OBJ_COUNT];
    cgGardenCamera_t cam;
} cgField_t;

typedef struct 
{
    // Objects
    cgField_t field;
    cgItem_t items[CG_FIELD_ITEM_LIMIT];

    // Cursor
    rectangle_t cursorAABB;
    bool holdingItem;
    cgItem_t* heldItem;
    bool holdingChowa;
    cgChowa_t* heldChowa;
} cgGarden_t;

//==============================================================================
// Sparring
//==============================================================================

typedef struct 
{
    uint8_t stamina[2];
    cgChowa_t* participants;
} cgMatch_t;

typedef struct 
{
    // Assets
    // Audio
    // - BGM, menus
    // - BGM, match
    // - Combat sounds
    //   - Pain sounds
    //   - Impact sounds
    //   - Gong crash
    //   - Countdown noises
    //   - Cheer noises
    // Sprites
    // Chowa
    // - Attack animations
    // - Exhaustion animation
    // - Win (cheer)
    // - Lose (cry)
    // Dojo
    // - Room
    // - Japanese themed dojo items
    // UI
    // - Punch icon
    // - Kick icon

    // Menu
    // Menu object
    // TODO: New renderer

    // Spar

    // Match

    // Input

    // LEDs

    
} cgSpar_t;

//==============================================================================
// Mode Data
//==============================================================================

typedef struct
{
    // Assets
    // ========================================================================
    // Fonts
    font_t menuFont;
    // Sprites
    wsg_t gardenSpr[CG_GARDEN_STATIC_OBJECTS];
    wsg_t cursors[CG_GARDEN_CURSORS];
    wsg_t items[CG_GARDEN_ITEMS_COUNT];
    wsg_t chowaExpressions[CG_CHOWA_EXPRESSION_COUNT];

    // Modes
    cgGarden_t garden;
    cgSpar_t spar;

    // Settings
    bool touch; // If using the touch pad for controls

    // Chowa
    cgChowa_t chowa[CG_MAX_CHOWA];
} cGrove_t;