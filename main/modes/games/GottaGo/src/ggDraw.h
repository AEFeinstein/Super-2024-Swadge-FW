#pragma once

//==============================================================================
// Includes
//==============================================================================

#include "ggCommonTypes.h"

//==============================================================================
// Defines
//==============================================================================

#define LONG_WAIT (GG_SECOND * 15)

//==============================================================================
// Function definitions
//==============================================================================

// init
/**
 * @brief Initializes the warning data
 *
 * @param ggd Game data
 */
void ggInitWarning(ggData_t* ggd);

/**
 * @brief Initializes the splash screen
 *
 * @param ggd Game data
 */
void ggInitSplash(ggData_t* ggd);

// Single screens
/**
 * @brief Draws the initial warning screen
 *
 * @param ggd Game data
 * @param elapsedUs Time since last frame
 */
void ggDrawWarning(ggData_t* ggd, int64_t elapsedUs);

/**
 * @brief Draws the splash screen
 *
 * @param ggd Game data
 * @param elapsedUs Time since last frame
 */
void ggDrawSplash(ggData_t* ggd, int64_t elapsedUs);

/**
 * @brief Draws the ready screen
 *
 * @param ggd Game data
 * @param elapsedUs Time since last frame
 */
void ggDrawReady(ggData_t* ggd, int64_t elapsedUs);

/**
 * @brief Draws the pause screen
 *
 * @param ggd Game data
 */
void ggDrawPause(ggData_t* ggd);

/**
 * @brief Draws the game's end result
 *
 * @param ggd Game data
 */
void ggDrawResult(ggData_t* ggd);

// Levels

/**
 * @brief Draws a level
 *
 * @param ggd Game data
 * @param solution If the solution should be drawn
 */
void ggDrawLevel(ggData_t* ggd, bool solution);

// Menus

/**
 * @brief Draws the menu
 *
 * @param ggd Game data
 */
void ggDrawMenu(ggData_t* ggd);

/**
 * @brief Draws the rules tabs
 *
 * @param ggd Game data
 */
void ggDrawRules(ggData_t* ggd);

/**
 * @brief Draws the high score tables
 *
 * @param ggd Game data
 */
void ggDrawHighScore(ggData_t* ggd);

/**
 * @brief Draws the options menu
 * 
 * @param ggd Game data
 */
void ggDrawOptions(ggData_t* ggd);