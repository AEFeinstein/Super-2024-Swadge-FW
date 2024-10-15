#ifndef _SEQUENCER_MODE_H_
#define _SEQUENCER_MODE_H_

#include "swadge2024.h"
#include "wheel_menu.h"

#if !defined(__XTENSA__)
    #include "os_generic.h"
#endif

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
    int32_t tempo;   ///< The song tempo, beats per minute
    int32_t grid;    ///< Grid lines, 1, 2, 4, 8, or 16
    int32_t timeSig; ///< Time signature, 2, 3, 4, or 5
    int32_t songEnd; ///< Where the song ends, in 1/16 increments
    bool loop;       ///< True to loop when finished, false to stop
} seqSongParams_t;

typedef struct
{
    uint32_t type;
    uint32_t instrument;
} seqNoteParams_t;

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
    vec_t gridOffsetTarget;
    int32_t labelWidth;
    int32_t cellWidth;
    int32_t rowHeight;
    int32_t numRows;

    // UI Images
    wsg_t noteWsgs[5];
    wsg_t instrumentWsgs[3];

    // Scrolling
    bool isPlaying;
    int32_t usPerPx;
    int32_t scrollTimer;

    // Song parameters
    seqSongParams_t songParams;
    int32_t usPerBeat;
    seqNoteParams_t noteParams;

    // Playing
    int32_t songTimer;
    list_t notes;
    list_t midiQueue;

#if !defined(__XTENSA__)
    og_mutex_t midiQueueMutex;
#endif
} sequencerVars_t;

//==============================================================================
// Extern Variables
//==============================================================================

extern swadgeMode_t sequencerMode;

#endif