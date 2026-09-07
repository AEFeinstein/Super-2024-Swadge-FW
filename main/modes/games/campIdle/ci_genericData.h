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

/// @brief NOTE: Match to NVS Key string array above
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
    wsg_t* itemImages; ///< Item Icons
    wsg_t* uiImages;   ///< All UI images

    // Fonts
    font_t smallFont; ///< Smaller font used for dense text
    font_t largeText; ///< Larger font used for titles

    // Current state
    ciState_t state;    ///< Current game state
    intptr_t selection; ///< Current selection
    int64_t timer;      ///< Generic timer

    // Items
    int16_t* qtys; ///< Array of ints used as the inventory

    // Craft
    list_t craftQueue;  ///< Queue of items to be autocrafted
    int64_t timerUnits; ///< Units of time to partition out. Shared betweeen craft and forage
    int64_t timerUs;    ///< Microseconds for crafting queue

    // Foraging
    bool foraging; ///< If player is foraging
} ciCampData_t;