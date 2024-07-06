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

//==============================================================================
// Defines
//==============================================================================

#define MAX_TEXT_LEN 255

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    wsg_t bg;
    font_t fnt1;
    font_t fnt2;
    bool displayText;
    char typedText[MAX_TEXT_LEN];
} keebTest_t;

extern swadgeMode_t keebTestMode;