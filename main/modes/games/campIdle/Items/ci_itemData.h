#pragma once

//==============================================================================
// Includes
//==============================================================================

// Core
#include "cnfs.h"

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    CI_BAD_FOOD,
    CI_CRAFTED,
    CI_FOOD,
    CI_FORAGED,
    CI_HEALING,
    CI_TYPE_ALL
} ciItemTypes_t;

typedef enum
{
    CI_FOREST,
    CI_SWAMP,
    CI_MOUNTAIN,
    CI_JUNGLE,
    CI_M_FOREST,
    CI_LOCATION_COUNT,
    CI_NONE, // If crafted
    CI_ALL,
} ciLocations_t;

typedef enum
{
    CI_SMALL,
    CI_MED,
    CI_LARGE
} ciItemSizes_t;

/// @brief Must stay in the same order as ciItemData's items
typedef enum
{
    CI_FLOOR_PIZZA,
    CI_FURRY_MILK,
    CI_ICBINB,
    CI_MALORT,
    CI_PILK,
    CI_RAVER_SWEAT,
    CI_SQUIRREL_NUTS,
    CI_YPLA,
    CI_CLOTH,
    CI_CUT_ROCK,
    CI_DIAMOND,
    CI_DIAMOND_POWDER,
    CI_GEAR,
    CI_IRON,
    CI_HIDE,
    CI_POLISHED_ROCK,
    CI_POLISHED_CRYSTAL,
    CI_ROPE,
    CI_SALT,
    CI_STRING,
    CI_APPLE,
    CI_BEANS,
    CI_BERRIES,
    CI_DONUT,
    CI_ENERGY_DRINK,
    CI_HONEY,
    CI_MRE,
    CI_MUSHROOMS,
    CI_MYSTERY_MEAT,
    CI_PAN_PIZZA,
    CI_PICKLES,
    CI_PROTEIN_POWDER,
    CI_PUDDING,
    CI_ROAST_TURKEY,
    CI_SQUEEZY_PEANUT_BUTTER,
    CI_STRING_CHEESE,
    CI_TASTEFUL_NOODZ,
    CI_BAMBOO,
    CI_HONEY_COMB,
    CI_BIRCH_BARK,
    CI_COAL,
    CI_CRYSTAL,
    CI_DRIED_GRASS,
    CI_IRON_ORE,
    CI_LARGE_LEAF,
    CI_LATEX,
    CI_LOG,
    CI_RESIN,
    CI_ROCKS,
    CI_ROCK_SALT,
    CI_SPIDER_WEB,
    CI_STICK,
    CI_UNCURED_HIDE,
    CI_VINE,
    CI_TAR,
    CI_HEALING_POWDER,
    CI_BANDAGES,
    CI_POULTICE,
    CI_HEALING_POTION,
    CI_HEART,
} ciItemIdx_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    cnfsFileIdx_t image;
    ciItemTypes_t type;
    ciLocations_t loc;
    ciItemSizes_t size;
    int weight;
    int value;
    const char* title;
    const char* desc;
} ciItem_t;

/// @brief Used to pack the inventory as tightly as possible
typedef struct __attribute__((packed))
{
    // Bad Food
    uint8_t floorPizza   : 8;
    uint8_t furryMilk    : 8;
    uint8_t notMargarine : 8;
    uint8_t malort       : 8;
    uint8_t pilk         : 8;
    uint8_t raverSweat   : 8;
    uint8_t squirrelNuts : 8;
    uint8_t ypla         : 8;
    // Crafted
    uint8_t cloth            : 8;
    uint8_t cutBlocks        : 8;
    uint8_t diamond          : 8;
    uint8_t diamondPowder    : 8;
    uint8_t gears            : 8;
    uint8_t iron             : 8;
    uint8_t pelts            : 8;
    uint8_t polishedBlocks   : 8;
    uint8_t polishedCrystals : 8;
    uint8_t rope             : 8;
    uint8_t salt             : 8;
    uint8_t string           : 8;
    // Food
    uint8_t apple         : 8;
    uint8_t beans         : 8;
    uint8_t berries       : 8;
    uint8_t donut         : 8;
    uint8_t energyDrink   : 8;
    uint8_t honey         : 8;
    uint8_t MRE           : 8;
    uint8_t mushrooms     : 8;
    uint8_t mysteryMeat   : 8;
    uint8_t panPizza      : 8;
    uint8_t pickles       : 8;
    uint8_t proteinPowder : 8;
    uint8_t pudding       : 8;
    uint8_t roastTurkey   : 8;
    uint8_t squeezyPB     : 8;
    uint8_t stringCheese  : 8;
    uint8_t noodz         : 8;
    // Foraged
    uint8_t bamboo      : 8;
    uint8_t honeyComb   : 8;
    uint8_t birchBark   : 8;
    uint8_t coal        : 8;
    uint8_t crystal     : 8;
    uint8_t driedGrass  : 8;
    uint8_t ironOre     : 8;
    uint8_t largeLeaf   : 8;
    uint8_t latex       : 8;
    uint8_t log         : 8;
    uint8_t resin       : 8;
    uint8_t rocks       : 8;
    uint8_t rockSalt    : 8;
    uint8_t spiderWeb   : 8;
    uint8_t stick       : 8;
    uint8_t uncuredHide : 8;
    uint8_t vine        : 8;
    uint8_t tar         : 8;
    // Health
    uint8_t healingPowder : 8;
    uint8_t bandages      : 8;
    uint8_t poultice      : 8;
    uint8_t healingPotion : 8;
    uint8_t heart         : 8;
} ciInvQtysPacked_t;

extern const ciItem_t ciItemData[];

//==============================================================================
// Functions
//==============================================================================

int ciGetArrayLength(void);