#pragma once

//==============================================================================
// Include
//==============================================================================

// Swadge
#include "font.h"
#include "linked_list.h"

// Camp
#include "ci_items.h"
#include "ci_workbench.h"

//==============================================================================
// Defines
//==============================================================================

// Graphics
#define MAX_COLS 4

// Time
#define SECOND   1000000
#define UNIT_LEN 15
#define UNIT     (UNIT_LEN * SECOND)

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    list_t craftQueue; ///< Queue of items to be autocrafted
} ciCrafting_t;

//==============================================================================
// Function Definitions
//==============================================================================

/**
 * @brief Loads current crafting queue fro NVS
 *
 * @param ccd Game Data
 */
void ciLoadCraftFromNVS(ciCrafting_t* cft);

/**
 * @brief Saves current crafting queue to NVS
 *
 * @param ccd Game Data
 */
void ciSaveCraftToNVS(ciCrafting_t* cft);

/**
 * @brief Attempts to craft items
 *
 * @param ccd Game Data
 */
void ciCraft(ciCrafting_t* cft, ciInventory_t* inv, int64_t* timeUnits);

/**
 * @brief Loads the craft units and compares with the new time to craft items/Forage
 *
 * @param ccd Game Data
 */
// void ciInitCraftTimer(ciCampData_t* ccd);

/**
 * @brief Draws the crafting screen
 *
 * @param ccd Game Data
 */
void drawCraft(ciCrafting_t* cft, ciInventory_t* inv, font_t* lFont, font_t* sFont, int timeUnits, int64_t timerUs);

/**
 * @brief Draws the crafting selection screen
 *
 * @param ccd Game data
 */
void drawCraftSelection(ciCrafting_t* cft, ciInventory_t* inv, ciWorkbenchData_t* wbd, font_t* lFont, font_t* sFont,
                        int selection);
