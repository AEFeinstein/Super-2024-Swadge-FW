#pragma once

//==============================================================================
// Include
//==============================================================================

#include "ci_itemData.h"

//==============================================================================
// Defines
//==============================================================================

// Sizes
#define MAX_CRAFT_ITEM_COUNT 2
#define MAX_WORKBENCH_ITEMS  3

// Time
#define SECOND   1000000
#define UNIT_LEN 15
#define UNIT     (UNIT_LEN * SECOND)

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    CI_RECIPE_CLOTH,
    CI_RECIPE_CUT_BLOCK,
    CI_RECIPE_DIAMOND,
    CI_RECIPE_DIAMOND_POWDER,
    CI_RECIPE_GEARS,
    CI_RECIPE_IRON,
    CI_RECIPE_PELTS,
    CI_RECIPE_POLISHED_BLOCK,
    CI_RECIPE_POLISHED_CRYSTAL,
    CI_RECIPE_ROPE,
    CI_RECIPE_SALT,
    CI_RECIPE_STRING,
} ciRecipesEnum_t;

typedef enum
{
    CI_CRAFT_CRYSTAL_POLISHER,
    CI_CRAFT_HEARTMAKER,
    CI_CRAFT_MAGIC_WORKBENCH,
    CI_CRAFT_SMASHER,
    CI_CRAFT_SMELTER,
    CI_CRAFT_STONE_CUTTER,
    CI_CRAFT_TANNING_RACK,
    CI_CRAFT_WEAVER,
    CI_CRAFT_WORKBENCH,
} ciCraftingStation_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    ciCraftingStation_t craftingStation;
    ciContainerItem_t items[MAX_CRAFT_ITEM_COUNT];
    ciItemTypes_t result;
    int16_t time; ///< time that it takes to craft in quarter-minutes
} ciRecipeProto_t;

// TODO: Add Workbench data
// - Associated images
// - etc
// Also need functions

typedef struct
{
    const char* title;
    const char* desc;
    ciContainerItem_t items[MAX_WORKBENCH_ITEMS];
} ciWorkbench_t;

//==============================================================================
// Const
//==============================================================================

extern const ciRecipeProto_t recipeList[];

extern const ciWorkbench_t workbenchList[];

//==============================================================================
// Function Definitions
//==============================================================================

int ciGetRecipeCount(void);

int ciGetWorkbenchCount(void);