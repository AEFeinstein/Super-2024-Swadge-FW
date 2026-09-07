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
        .craftingStation = CI_CRAFT_WORKBENCH,
        .items[0].item   = CI_IRON,
        .items[0].qty    = 1,
        .items[1].item   = CI_NO_ITEM,
        .items[1].qty    = 0,
        .result          = CI_IRON_NAIL,
        .time            = 4,
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

const ciWorkbench_t workbenchList[] = {
    {
        .title         = "Polisher",
        .desc          = "Allows you to polish rocks and crystals",
        .items[0].item = CI_CUT_ROCK,
        .items[0].qty  = 20,
        .items[1].item = CI_DIAMOND_POWDER,
        .items[1].qty  = 10,
        .items[2].item = CI_NO_ITEM,
        .items[2].qty  = 0,
    },
    {
        .title         = "Heartmaker",
        .desc          = "Pulls magic out of the air to heal you",
        .items[0].item = CI_POLISHED_CRYSTAL,
        .items[0].qty  = 10,
        .items[1].item = CI_CLOTH,
        .items[1].qty  = 10,
        .items[2].item = CI_ROPE,
        .items[2].qty  = 3,
    },
    {
        .title         = "Magic Workbench",
        .desc          = "Allows you to craft magical items",
        .items[0].item = CI_POLISHED_CRYSTAL,
        .items[0].qty  = 4,
        .items[1].item = CI_LOG,
        .items[1].qty  = 4,
        .items[2].item = CI_NO_ITEM,
        .items[2].qty  = 0,
    },
    {
        .title         = "Smasher",
        .desc          = "Smashes things to bits",
        .items[0].item = CI_ROCKS,
        .items[0].qty  = 10,
        .items[1].item = CI_STICK,
        .items[1].qty  = 10,
        .items[2].item = CI_GEAR,
        .items[2].qty  = 5,
    },
    {
        .title         = "Smelter",
        .desc          = "Turn your ore into Iron",
        .items[0].item = CI_ROCKS,
        .items[0].qty  = 20,
        .items[1].item = CI_COAL,
        .items[1].qty  = 10,
        .items[2].item = CI_NO_ITEM,
        .items[2].qty  = 0,
    },
    {
        .title         = "Stone Cutter",
        .desc          = "Allows you to cut stones and make them square",
        .items[0].item = CI_LOG,
        .items[0].qty  = 4,
        .items[1].item = CI_ROCKS,
        .items[1].qty  = 10,
        .items[2].item = CI_NO_ITEM,
        .items[2].qty  = 0,
    },
    {
        .title         = "Tanning rack",
        .desc          = "The hides may be pleather, but you still need to tan them",
        .items[0].item = CI_STICK,
        .items[0].qty  = 10,
        .items[1].item = CI_ROPE,
        .items[1].qty  = 4,
        .items[2].item = CI_NO_ITEM,
        .items[2].qty  = 0,
    },
    {
        .title         = "Weaver",
        .desc          = "Allows for the creation of cloth",
        .items[0].item = CI_STICK,
        .items[0].qty  = 20,
        .items[1].item = CI_GEAR,
        .items[1].qty  = 4,
        .items[2].item = CI_IRON_NAIL,
        .items[2].qty  = 10,
    },
    {
        .title         = "Workbench",
        .desc          = "A simple table for building more complex things",
        .items[0].item = CI_LOG,
        .items[0].qty  = 2,
        .items[1].item = CI_STICK,
        .items[1].qty  = 10,
        .items[2].item = CI_RESIN,
        .items[2].qty  = 2,
    },
};

int ciGetRecipeCount(void)
{
    return ARRAY_SIZE(recipeList);
}

int ciGetWorkbenchCount(void)
{
    return ARRAY_SIZE(workbenchList);
}