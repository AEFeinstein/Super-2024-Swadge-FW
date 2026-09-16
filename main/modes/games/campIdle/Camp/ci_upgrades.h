#pragma once

//==============================================================================
// Include
//==============================================================================

// C
#include <inttypes.h>

// Camp
#include "ci_items.h"

//==============================================================================
// Defines
//==============================================================================

#define MAX_UPGRADE_ITEMS  3
#define REQUIRED_ITEMS_NUM 2

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    CI_AIR_DEFENSE,
    CI_BED,
    CI_FIRE,
    CI_FOOD_STORES,
    CI_GROUND_DEFENSE,
    CI_HAULING,
    CI_TENT,
    CI_WATER_STORES,
} ciUpgradeTypes_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    char* title;
    char* desc;
    ciUpgradeTypes_t type;
    int8_t rank;
    ciContainerItem_t items[MAX_UPGRADE_ITEMS];
    ciContainerItem_t requiredItem;
    int foodReq;
    int effect; // Bed - Stamina mult factor
                // Hauling - Carry weight
} ciUpgrade_t;

//==============================================================================
// Consts
//==============================================================================

extern const ciUpgrade_t upgradeList[];