#pragma once

//==============================================================================
// Include
//==============================================================================

#include "ci_genericData.h"

//==============================================================================
// Consts
//==============================================================================

static const char* const menuText[] = {
    "Cozy Camper", "Main Menu", "Play!", "Encyclopedia", "Tutorial", "Quit",
};

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    CI_MENU_SPLASH,
    CI_MENU_TITLE,
    CI_MENU_PLAY,
    CI_MENU_ENCYCLOPEDIA,
    CI_MENU_TUTORIAL,
    CI_MENU_QUIT,
} ci_menuText;

//==============================================================================
// Function Definitions
//==============================================================================

void ciInitSplash(ciCampData_t* ccd);

void ciInitMenu(ciCampData_t* ccd);

void ciRunSplash(ciCampData_t* ccd);

void ciRunMenu(ciCampData_t* ccd);