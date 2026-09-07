#pragma once

//==============================================================================
// Include
//==============================================================================

#include "ci_genericData.h"

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    CI_HEARTMAKER_1,
    CI_HEARTMAKER_2,
    CI_HEARTMAKER_3,
    CI_HEARTMAKER_4,
    CI_M_SPHERE_1,
    CI_M_SPHERE_2,
    CI_M_SPHERE_3,
    CI_M_WORKBENCH,
    CI_POLISHER,
    CI_POLISHER_BOX,
    CI_SMASHER,
    CI_SMASHER_HAMMER,
    CI_SMELTER,
    CI_SMELTER_FIRE_1,
    CI_SMELTER_FIRE_2,
    CI_SMELTER_FIRE_3,
    C_SMELTER_WOOD,
    CI_TANNING_RACK,
    CI_TANNING_RACK_PELT,
    CI_WEAVER,
    CI_WORKBENCH,
} ciWorkbenchEnum_t;

//==============================================================================
// Function Definitions
//==============================================================================

void ciInitWorkbenches(ciCampData_t* ccd);

void ciFreeWorkbenches(ciCampData_t* ccd);

/**
 * @brief Loads current crafting queue fro NVS
 *
 * @param ccd Game Data
 */
void ciLoadCraftFromNVS(ciCampData_t* ccd);

/**
 * @brief Saves current crafting queue to NVS
 *
 * @param ccd Game Data
 */
void ciSaveCraftFromNVS(ciCampData_t* ccd);

/**
 * @brief Initializes the Crafting selections screen
 *
 * @param ccd Game Data
 */
void ciInitCraftSelection(ciCampData_t* ccd);

/**
 * @brief Initializes the crafting status screen
 *
 * @param ccd Game Data
 */
void ciInitCraft(ciCampData_t* ccd);

/**
 * @brief Runs the crafting selection mode
 *
 * @param ccd Game Data
 */
void ciRunCraftSelection(ciCampData_t* ccd);

/**
 * @brief Runs the crafting mode
 *
 * @param ccd Game Data
 * @return true If mode is exiting
 * @return false If mode is not ready to exit
 */
bool ciRunCraft(ciCampData_t* ccd);

/**
 * @brief Loads the craft units and compares with the new time to craft items/Forage
 *
 * @param ccd Game Data
 */
void ciInitCraftTimer(ciCampData_t* ccd);

/**
 * @brief Attempts to craft items
 *
 * @param ccd Game Data
 */
void ciCraft(ciCampData_t* ccd);

/**
 * @brief Loads the workbenches from NVS
 *
 * @param ccd Game Data
 */
void ciLoadWorkbenches(ciCampData_t* ccd);

/**
 * @brief Attempts to add a new workbench
 *
 * @param ccd Game Data
 * @param wb Workbench idx
 */
void ciAddWorkbench(ciCampData_t* ccd, ciCraftingStation_t wb);

