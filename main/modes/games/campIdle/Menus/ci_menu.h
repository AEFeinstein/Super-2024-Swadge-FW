#pragma once

//==============================================================================
// Include
//==============================================================================

#include "ci_genericData.h"

//==============================================================================
// Consts
//==============================================================================

static const char* const menuText[] = {
    "Main Menu", "Play!", "Encyclopedia", "Tutorial", "Quit", "Cozy Camper", "Press 'A' to play!",
};

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    CI_MENU_TITLE,
    CI_MENU_PLAY,
    CI_MENU_ENCYCLOPEDIA,
    CI_MENU_TUTORIAL,
    CI_MENU_QUIT,
    CI_MENU_SPLASH,
    CI_MENU_PRESS_A,
} ci_menuText;

//==============================================================================
// Function Definitions
//==============================================================================

/**
 * @brief Initializes the Splash state
 *
 * @param ccd Game Data
 */
void ciInitSplash(ciCampData_t* ccd);

/**
 * @brief Initializes the Menu state
 *
 * @param ccd Game Data
 */
void ciInitMenu(ciCampData_t* ccd);

/**
 * @brief Initializes the Encyclopedia state
 *
 * @param ccd Game Data
 */
void ciInitEncyclopedia(ciCampData_t* ccd);

/**
 * @brief Runs the Splash state
 *
 * @param ccd Game Data
 * @param elapsedUs
 */
void ciRunSplash(ciCampData_t* ccd, int64_t elapsedUs);

/**
 * @brief Runs the Menu state
 *
 * @param ccd Game Data
 */
void ciRunMenu(ciCampData_t* ccd);

/**
 * @brief Runs the Encyclopedia state
 *
 * @param ccd Game Data
 */
void ciRunEncyclopedia(ciCampData_t* ccd);