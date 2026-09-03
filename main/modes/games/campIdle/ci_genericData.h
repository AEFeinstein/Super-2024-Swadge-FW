#pragma once

//==============================================================================
// Include
//==============================================================================

#include "swadge.h"

#include "ci_itemData.h"

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
    int selection;
    int64_t timer;

    // Items
    // ciInvQtys_t qtys;
    uint8_t* qtys;
} ciCampData_t;