/**
 * @file ext_touch_linear.h
 * @author gelakinetic@gmail.com
 * @brief
 * @date 2026-06-08
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>
#include <stdbool.h>
#include "emu_ext.h"

//==============================================================================
// Defines
//==============================================================================

// The minimum width & height of the pane
#define TOUCH_PANE_MIN_SIZE 128

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    NOT_CLICKED,
    LEFT_CLICKED,
    LEFT_RELEASED,
    RIGHT_CLICKED,
    RIGHT_RELEASED,
} mouseClickState_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    bool isHorz;

    mouseClickState_t clickState;
    int32_t mouseX;
    int32_t mouseY;
    int32_t intensity;
    int32_t keyState;

    uint32_t paneX;
    uint32_t paneY;
    uint32_t paneW;
    uint32_t paneH;

    const char* keys;
    uint8_t numKeys;
} emuTouch_t;

//==============================================================================
// Function Declarations
//==============================================================================

bool touchLinearInit(emuTouch_t* et, emuExtension_t* ext, const emuArgs_t* emuArgs, bool isHorz, const char* keys,
                     uint8_t numKeys);
int32_t touchLinearKey(emuTouch_t* et, uint32_t key, bool down, modKey_t modifiers);
bool touchLinearMouseMove(emuTouch_t* et, int32_t x, int32_t y, mouseBit_t buttonMask);
bool touchLinearMouseButton(emuTouch_t* et, int32_t x, int32_t y, mouseButton_t button, bool down);
void touchLinearRender(emuTouch_t* et, uint32_t winW, uint32_t winH, const emuPane_t* pane, uint8_t numPanes);