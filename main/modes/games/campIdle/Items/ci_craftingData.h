#pragma once

//==============================================================================
// Include
//==============================================================================

#include "ci_itemData.h"

#define MAX_CRAFT_ITEM_COUNT 2

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
    const char* name;
    ciCraftingStation_t cs;
    ciContainerItem_t items[MAX_CRAFT_ITEM_COUNT];
    ciItemTypes_t result;
} ciRecipeProto_t;
