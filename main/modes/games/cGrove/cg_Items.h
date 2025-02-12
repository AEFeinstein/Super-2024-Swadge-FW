/**
 * @file cg_GardenItems.h
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief Interactive items inside of the main garden
 * @version 1.0
 * @date 2024-09-07
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once

//==============================================================================
// Includes
//==============================================================================

#include "cg_Typedef.h"

//==============================================================================
// Function declarations
//==============================================================================

/**
 * @brief Initializes an item slot
 *
 * @param cg Game Object
 * @param idx Index of item
 * @param name Text string of item
 * @param spr Sprite of item
 * @param pos Position of item
 */
void cgInitItem(cGrove_t* cg, int8_t idx, char* name, wsg_t spr, vec_t pos);

/**
 * @brief Removes an item form the game field
 *
 * @param cg Game Object
 * @param idx Index of item
 */
void cgDeactivateItem(cGrove_t* cg, int8_t idx);
