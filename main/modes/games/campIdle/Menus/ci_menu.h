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

void ciInitSplash(ciCampData_t* ccd);

void ciInitMenu(ciCampData_t* ccd);

void ciInitEncyclopedia(ciCampData_t* ccd);

void ciRunSplash(ciCampData_t* ccd, int64_t elapsedUs);

void ciRunMenu(ciCampData_t* ccd);

void ciRunEncyclopedia(ciCampData_t* ccd);