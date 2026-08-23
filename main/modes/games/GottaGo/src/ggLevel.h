#pragma once

//==============================================================================
// Includes
//==============================================================================

#include "ggCommonTypes.h"

//==============================================================================
// Function definitions
//==============================================================================

/**
 * @brief Resets the level to defaults
 * 
 * @param ggd game data
 */
void ggLevelReset(ggData_t* ggd);

/**
 * @brief Resets the urinals to default
 * 
 * @param ggd Game data
 */
void ggClearUrinals(ggData_t* ggd);

/**
 * @brief Handles ending the level
 * 
 * @param ggd game data
 * @return true game won, false game lost
 */
bool ggEndLevel(ggData_t* ggd);

