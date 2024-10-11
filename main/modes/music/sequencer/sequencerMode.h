#ifndef _SEQUENCER_MODE_H_
#define _SEQUENCER_MODE_H_

#include "swadge2024.h"

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    SEQUENCER_MENU,
    SEQUENCER_SEQ,
} sequencerScreen_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    // Menu
    menu_t* menu;
    menuManiaRenderer_t* renderer;

    // Mode state
    sequencerScreen_t screen;

    // Drawing
    font_t ibm;
    vec_t cursorPos;
    vec_t gridOffset;
    int32_t labelWidth;
    int32_t cellWidth;
    int32_t rowHeight;

    // Song parameters
    int32_t usPerBeat;
    int32_t numBars;
    int32_t timeSig;
    int32_t gridSize;
} sequencerVars_t;

//==============================================================================
// Extern Variables
//==============================================================================

extern swadgeMode_t sequencerMode;

#endif