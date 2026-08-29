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
    CI_NONE,
    CI_ALL,
} ciLocations_t;

typedef enum
{
    CI_SMALL,
    CI_MED,
    CI_LARGE
} ciItemSizes_t;

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
    uint8_t Rope             : 8;
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
} ciInvQtys_t;

extern const ciItem_t ciItemData[];

//==============================================================================
// Functions
//==============================================================================

int ciGetArrayLength(void);