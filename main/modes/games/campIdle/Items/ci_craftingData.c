//==============================================================================
// Include
//==============================================================================

#include "ci_craftingData.h"
#include "macros.h"

//==============================================================================
// Const
//==============================================================================

const ciRecipeProto_t recipeList[] = {
    {
        .craftingStation = CI_CRAFT_WEAVER,
        .items[0].item   = CI_COTTON,
        .items[0].qty    = 10,
        .items[1].item   = CI_NO_ITEM,
        .items[1].qty    = 0,
        .result          = CI_CLOTH,
        .time            = 2,
    },
    {
        .craftingStation = CI_CRAFT_STONE_CUTTER,
        .items[0].item   = CI_ROCKS,
        .items[0].qty    = 4,
        .items[1].item   = CI_NO_ITEM,
        .items[1].qty    = 0,
        .result          = CI_CUT_ROCK,
        .time            = 8,
    },
    {
        .craftingStation = CI_CRAFT_SMASHER,
        .items[0].item   = CI_ROCKS,
        .items[0].qty    = 10,
        .items[1].item   = CI_NO_ITEM,
        .items[1].qty    = 0,
        .result          = CI_DIAMOND,
        .time            = 5,
    },
    {
        .craftingStation = CI_CRAFT_SMASHER,
        .items[0].item   = CI_DIAMOND,
        .items[0].qty    = 1,
        .items[1].item   = CI_NO_ITEM,
        .items[1].qty    = 0,
        .result          = CI_DIAMOND_POWDER,
        .time            = 10,
    },
    {
        .craftingStation = CI_CRAFT_WORKBENCH,
        .items[0].item   = CI_STICK,
        .items[0].qty    = 8,
        .items[1].item   = CI_RESIN,
        .items[1].qty    = 5,
        .result          = CI_GEAR,
        .time            = 4,
    },
    {
        .craftingStation = CI_CRAFT_SMELTER,
        .items[0].item   = CI_IRON_ORE,
        .items[0].qty    = 1,
        .items[1].item   = CI_COAL,
        .items[1].qty    = 1,
        .result          = CI_IRON,
        .time            = 40,
    },
    {
        .craftingStation = CI_CRAFT_TANNING_RACK,
        .items[0].item   = CI_UNCURED_HIDE,
        .items[0].qty    = 1,
        .items[1].item   = CI_NO_ITEM,
        .items[1].qty    = 0,
        .result          = CI_HIDE,
        .time            = 100,
    },
    {
        .craftingStation = CI_CRAFT_CRYSTAL_POLISHER,
        .items[0].item   = CI_CUT_ROCK,
        .items[0].qty    = 1,
        .items[1].item   = CI_DIAMOND_POWDER,
        .items[1].qty    = 1,
        .result          = CI_POLISHED_ROCK,
        .time            = 60,
    },
    {
        .craftingStation = CI_CRAFT_CRYSTAL_POLISHER,
        .items[0].item   = CI_CRYSTAL,
        .items[0].qty    = 1,
        .items[1].item   = CI_DIAMOND_POWDER,
        .items[1].qty    = 1,
        .result          = CI_POLISHED_CRYSTAL,
        .time            = 80,
    },
    {
        .craftingStation = CI_CRAFT_WORKBENCH,
        .items[0].item   = CI_VINE,
        .items[0].qty    = 2,
        .items[1].item   = CI_NO_ITEM,
        .items[1].qty    = 0,
        .result          = CI_ROPE,
        .time            = 3,
    },
    {
        .craftingStation = CI_CRAFT_SMASHER,
        .items[0].item   = CI_ROCK_SALT,
        .items[0].qty    = 1,
        .items[1].item   = CI_NO_ITEM,
        .items[1].qty    = 0,
        .result          = CI_SALT,
        .time            = 1,
    },
};

int ciGetRecipeCount(void)
{
    return ARRAY_SIZE(recipeList);
}