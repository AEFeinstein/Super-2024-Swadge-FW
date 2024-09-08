/**
 * @file cg_Types.h
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief Common types list for Chowa Garden
 * @version 0.1
 * @date 2024-09-08
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#pragma once

#include "swadge2024.h"
#include "geometry.h"

//==============================================================================
// Defines
//==============================================================================

// Asset COunts
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

// Chowa =======================================================================

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

/* typedef enum
{
    FACING_UP,
    FACING_LEFT,
    FACING_DOWN,
    FACING_RIGHT
} cgFacingEnum; */

//==============================================================================
// Structs
//==============================================================================

// cg_Field.h ==================================================================
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

// cg_GardenItems.h ===========================================================

typedef struct 
{
    char name[16];
    rectangle_t aabb;
    wsg_t spr;
    bool active;
} cgGardenItem_t;

// cg_Chowa.h ==================================================================
typedef struct
{
    vec_t headOffset;
    vec_t bodyOffset;
    vec_t limbRestPos[4];
    int8_t headSize;
    int8_t bodySize;
    int8_t limbSize[4];
} cgChowaBody_t;

typedef struct 
{
    paletteColor_t colors[CG_CHOWA_COLORS];
    rectangle_t aabb;
    cgChowaBody_t body;
    cgMoodEnum_t mood;
    bool active;

    // Garden
    bool holdingItem;
    cgGardenItem_t* heldItem;
    cgChowaGardenState_t gState;
    vec_t targetPos;
    int32_t waitTimer;
} cgChowa_t;

// cg_Garden.h ================================================================
typedef struct 
{
    // Objects
    cgField_t field;
    cgGardenItem_t items[CG_FIELD_ITEM_LIMIT];

    // Cursor
    rectangle_t cursorAABB;
    bool holdingItem;
    cgGardenItem_t* heldItem;
    bool holdingChowa;
    cgChowa_t* heldChowa;
} cgGarden_t;

// mode_cGrove.h ==============================================================
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

    // Structs
    // ========================================================================
    cgGarden_t garden;

    // Settings
    bool touch; // If using the touch pad for controls

    // Chowa
    cgChowa_t chowa[CG_MAX_CHOWA];
    // TODO: Guest Chowa
} cGrove_t;