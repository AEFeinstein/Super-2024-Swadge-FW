/**
 * @file cg_GardenItems.h
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief Interactive items inside of the main garden
 * @version 0.1
 * @date 2024-09-07
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#pragma once

// Include
#include "mode_cGrove.h"
#include "cg_Types.h"

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

/**
 * @brief Draws item at specified index
 * 
 * @param cg Game Object
 * @param idx index of item to draw
 */
void cgDrawItem(cGrove_t* cg, int8_t idx);