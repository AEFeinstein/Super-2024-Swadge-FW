#pragma once

//==============================================================================
// Includes
//==============================================================================

// Core
#include "cnfs.h"

#define MAX_ITEM_BITS 16

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
    CI_NO_ITEM = -1,
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
    CI_COTTON,
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
    const char* abbr;
} ciItem_t;

/// @brief Used to pack the inventory as tightly as possible
typedef struct __attribute__((packed))
{
    // Bad Food
    int16_t floorPizza : MAX_ITEM_BITS;
    int16_t furryMilk : MAX_ITEM_BITS;
    int16_t notMargarine : MAX_ITEM_BITS;
    int16_t malort : MAX_ITEM_BITS;
    int16_t pilk : MAX_ITEM_BITS;
    int16_t raverSweat : MAX_ITEM_BITS;
    int16_t squirrelNuts : MAX_ITEM_BITS;
    int16_t ypla : MAX_ITEM_BITS;
    // Crafted
    int16_t cloth : MAX_ITEM_BITS;
    int16_t cutBlocks : MAX_ITEM_BITS;
    int16_t diamond : MAX_ITEM_BITS;
    int16_t diamondPowder : MAX_ITEM_BITS;
    int16_t gears : MAX_ITEM_BITS;
    int16_t iron : MAX_ITEM_BITS;
    int16_t pelts : MAX_ITEM_BITS;
    int16_t polishedBlocks : MAX_ITEM_BITS;
    int16_t polishedCrystals : MAX_ITEM_BITS;
    int16_t rope : MAX_ITEM_BITS;
    int16_t salt : MAX_ITEM_BITS;
    int16_t string : MAX_ITEM_BITS;
    // Food
    int16_t apple : MAX_ITEM_BITS;
    int16_t beans : MAX_ITEM_BITS;
    int16_t berries : MAX_ITEM_BITS;
    int16_t donut : MAX_ITEM_BITS;
    int16_t energyDrink : MAX_ITEM_BITS;
    int16_t honey : MAX_ITEM_BITS;
    int16_t MRE : MAX_ITEM_BITS;
    int16_t mushrooms : MAX_ITEM_BITS;
    int16_t mysteryMeat : MAX_ITEM_BITS;
    int16_t panPizza : MAX_ITEM_BITS;
    int16_t pickles : MAX_ITEM_BITS;
    int16_t proteinPowder : MAX_ITEM_BITS;
    int16_t pudding : MAX_ITEM_BITS;
    int16_t roastTurkey : MAX_ITEM_BITS;
    int16_t squeezyPB : MAX_ITEM_BITS;
    int16_t stringCheese : MAX_ITEM_BITS;
    int16_t noodz : MAX_ITEM_BITS;
    // Foraged
    int16_t bamboo : MAX_ITEM_BITS;
    int16_t honeyComb : MAX_ITEM_BITS;
    int16_t birchBark : MAX_ITEM_BITS;
    int16_t coal : MAX_ITEM_BITS;
    int16_t cotton : MAX_ITEM_BITS;
    int16_t crystal : MAX_ITEM_BITS;
    int16_t driedGrass : MAX_ITEM_BITS;
    int16_t ironOre : MAX_ITEM_BITS;
    int16_t largeLeaf : MAX_ITEM_BITS;
    int16_t latex : MAX_ITEM_BITS;
    int16_t log : MAX_ITEM_BITS;
    int16_t resin : MAX_ITEM_BITS;
    int16_t rocks : MAX_ITEM_BITS;
    int16_t rockSalt : MAX_ITEM_BITS;
    int16_t spiderWeb : MAX_ITEM_BITS;
    int16_t stick : MAX_ITEM_BITS;
    int16_t uncuredHide : MAX_ITEM_BITS;
    int16_t vine : MAX_ITEM_BITS;
    int16_t tar : MAX_ITEM_BITS;
    // Health
    int16_t healingPowder : MAX_ITEM_BITS;
    int16_t bandages : MAX_ITEM_BITS;
    int16_t poultice : MAX_ITEM_BITS;
    int16_t healingPotion : MAX_ITEM_BITS;
    int16_t heart : MAX_ITEM_BITS;
} ciInvQtysPacked_t;

typedef struct
{
    ciItemIdx_t item;
    int qty;
} ciContainerItem_t;

typedef struct
{
    int sizeLim;
    int weightLim;
    int slotsLim;
    ciContainerItem_t* items;
} ciContainer_t;

//==============================================================================
// Consts
//==============================================================================

extern const ciItem_t ciItemData[];

//==============================================================================
// Functions
//==============================================================================

int ciGetItemCount(void);