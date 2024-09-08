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

// Grove
#define CG_OBJ_COUNT 64
#define CG_FIELD_BOUNDARY 32
#define CG_FIELD_ITEM_LIMIT 10

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

// cg_Garden.h ================================================================
typedef struct 
{
    cgField_t field;
    cgGardenItem_t items[CG_FIELD_ITEM_LIMIT];
    rectangle_t cursorAABB;
} cgGarden_t;

// mode_cGrove.h ==============================================================
typedef struct
{
    // Assets
    // ========================================================================
    // Fonts
    font_t menuFont;
    // Sprites
    wsg_t gardenSpr[1];
    wsg_t cursors[1];
    wsg_t items[1];

    // Structs
    // ========================================================================
    cgGarden_t garden;

    // Settings
    bool touch; // If using the touch pad for controls

    // Garden vars
    bool holding;
    cgGardenObject_t* heldItem;
    // TODO: Chowa
} cGrove_t;