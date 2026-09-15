#pragma once

//==============================================================================
// Include
//==============================================================================

#include "ci_itemData.h"
#include "ci_workbench.h"

//==============================================================================
// Defines
//==============================================================================

// Sizes
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

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    ciWorkbenchCraft_t craftingStation;
    ciContainerItem_t items[MAX_CRAFT_ITEM_COUNT];
    ciItemTypes_t result;
    int16_t time; ///< time that it takes to craft in quarter-minutes
} ciRecipeProto_t;

//==============================================================================
// Const
//==============================================================================

extern const ciRecipeProto_t recipeList[];

//==============================================================================
// Function Definitions
//==============================================================================

int ciGetRecipeCount(void);
