#pragma once

//==============================================================================
// Include
//==============================================================================

#include "swadge.h"

#include "ci_itemData.h"
#include "ci_craftingData.h"

//==============================================================================
// Consts
//==============================================================================

static const char* const ciNVSKeys[] = {
    "cozyCamping",
    "Inventory",
};

//==============================================================================
// Enum
//==============================================================================

typedef enum
{
    CI_NVS_NAMESPACE,
    CI_NVS_INVENTORY,
} ciNVSKeyEnum_t;

typedef enum
{
    CI_SPLASH,
    CI_MENU,
    CI_ENCYC,
    CI_ENCYC_DESC,
    CI_CRAFTING,
    CI_CRAFTING_PREP,
    CI_DAY,
    CI_NIGHT,
} ciState_t;

typedef enum
{
    CI_UI_ARROW,
} ciUIImgs_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    // WSGs
    wsg_t* itemImages;
    wsg_t* uiImages;

    // Fonts
    font_t smallFont;
    font_t largeText;

    // Current state
    ciState_t state;
    intptr_t selection;
    int64_t timer;

    // Items
    int16_t* qtys;

    // Craft
    list_t craftQueue;
    int64_t timerUnits; // Shared betweeen craft and forage
    int64_t timerUs;

    // Foraging
    bool foraging;
} ciCampData_t;