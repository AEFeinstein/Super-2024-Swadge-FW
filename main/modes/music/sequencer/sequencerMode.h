#ifndef _SEQUENCER_MODE_H_
#define _SEQUENCER_MODE_H_

#include "swadge2024.h"
#include "wheel_menu.h"

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
    int32_t midiNum;
    int32_t sixteenthOn;
    int32_t sixteenthOff;
    bool isOn;
} sequencerNote_t;

typedef struct
{
    // Menu
    menu_t* songMenu;
    menuManiaRenderer_t* menuRenderer;
    menu_t* noteMenu;
    wheelMenuRenderer_t* wheelRenderer;

    // Mode state
    sequencerScreen_t screen;

    // Drawing
    font_t ibm;
    vec_t cursorPos;
    vec_t gridOffset;
    int32_t labelWidth;
    int32_t cellWidth;
    int32_t rowHeight;

    // UI Images
    wsg_t noteWsgs[5];
    wsg_t instrumentWsgs[3];

    // Scrolling
    bool isPlaying;
    int32_t usPerPx;
    int32_t scrollTimer;

    // Song parameters
    int32_t usPerBeat;
    int32_t numBars;
    int32_t timeSig;
    int32_t gridSize;

    // Playing
    int32_t songTimer;
    list_t notes;
    list_t midiQueue;
} sequencerVars_t;

//==============================================================================
// Extern Variables
//==============================================================================

extern swadgeMode_t sequencerMode;

#endif