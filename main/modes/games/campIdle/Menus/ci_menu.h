#pragma once

//==============================================================================
// Include
//==============================================================================

#include "ci_genericData.h"

//==============================================================================
// Consts
//==============================================================================

extern const char* const menuText[];

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
} ciMenuTextEnum_t;

//==============================================================================
// Function Definitions
//==============================================================================

/**
 * @brief 
 * 
 * @param ccd 
 * @param state 
 */
void ciInitState(ciCampData_t* ccd, ciState_t state);

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

/**
 * @brief Runs the crafting mode
 *
 * @param ccd Game Data
 * @return true If mode is exiting
 * @return false If mode is not ready to exit
 */
bool ciRunCraft(ciCampData_t* ccd);

/**
 * @brief Runs the crafting selection mode
 *
 * @param ccd Game Data
 */
void ciRunCraftSelection(ciCampData_t* ccd);
