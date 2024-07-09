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
#include "textEntry.h"
#include "esp_random.h"

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
    WARNING,
} State_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    // Assets
    wsg_t bg;
    font_t fnt[4];

    // Vars
    State_t currState;
    char typedText[MAX_TEXT_LEN];

    // Menu
    menu_t* menu;
    menuManiaRenderer_t* renderer;

    // Menu vars
    int8_t fontSel;
    bgMode_t bckgrnd;
    uint8_t bgColor;
    uint8_t textColor;
    uint8_t empColor;
    uint8_t shadowColor;
    bool shadow;
    bool enter;
    bool caps;
    bool multi;
    bool count;
    bool reset;
    char prompt[16];
    uint8_t typingMode;

    // Warnings
    char warningText[32];
    int64_t warningTimer;
    bool updateString;
} keebTest_t;

extern swadgeMode_t keebTestMode;