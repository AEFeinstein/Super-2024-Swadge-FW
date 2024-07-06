/**
 * @file keebTest.h
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief A mode designed to test keyboard variations in rapid succession 
 * @version 1.0
 * @date 2024-07-06
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#pragma once

//==============================================================================
// Includes
//==============================================================================

#include "swadge2024.h"
#include "menu.h"

//==============================================================================
// Defines
//==============================================================================

#define MAX_TEXT_LEN 254

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    MENU,
    TYPING,
    DISPLAYING,
} State_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    wsg_t bg;
    font_t fnt1;
    font_t fnt2;
    font_t fnt3;
    font_t fnt4;
    State_t currState;
    menu_t *menu;
    char typedText[MAX_TEXT_LEN];
} keebTest_t;

extern swadgeMode_t keebTestMode;