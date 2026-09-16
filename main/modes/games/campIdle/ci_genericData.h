#pragma once

//==============================================================================
// Include
//==============================================================================

// Swadge
#include "swadge.h"

// Camp
#include "ci_crafting.h"
#include "ci_items.h"
#include "ci_workbench.h"

//==============================================================================
// Enum
//==============================================================================

typedef enum
{
    CI_UI_ARROW,
} ciUIImgs_t;

typedef enum
{
    CI_SPLASH,
    CI_MENU,
    CI_ENCYC,
    CI_ENCYC_DESC,
    CI_CRAFTING,
    CI_CRAFTING_PREP,
} ciState_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    // WSGs
    
    wsg_t* uiImages;        ///< All UI images

    // Fonts
    font_t smallFont; ///< Smaller font used for dense text
    font_t largeText; ///< Larger font used for titles

    // Current state
    ciState_t state;    ///< Current game state
    intptr_t selection; ///< Current selection
    int64_t timer;      ///< Generic timer

    // Items
    ciInventory_t inv;

    // Craft
    ciCrafting_t cft;
    int64_t timerUnits; ///< Units of time to partition out. Shared between craft and forage
    int64_t timerUs;    ///< Microseconds for crafting queue
    int stage;

    // Camp
    ciWorkbenchData_t wbd; ///< Workbench data

    // Foraging
    bool foraging; ///< If player is foraging
} ciCampData_t;