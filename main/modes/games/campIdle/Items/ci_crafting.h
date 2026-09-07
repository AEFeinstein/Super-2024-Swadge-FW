#pragma once

//==============================================================================
// Include
//==============================================================================

#include "ci_genericData.h"

//==============================================================================
// Function Definitions
//==============================================================================

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
 * @brief Initializes the Craftign selections screen
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


void ciLoadWorkbenches(ciCampData_t* ccd);


void ciAddWorkbench(ciCampData_t* ccd, ciCraftingStation_t wb);
