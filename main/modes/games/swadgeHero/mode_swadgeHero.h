#pragma once

//==============================================================================
// Includes
//==============================================================================

#include <esp_heap_caps.h>
#include <swadge2024.h>

//==============================================================================
// Defines
//==============================================================================

#define NUM_NOTE_TIMINGS 6

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    SH_NONE,
    SH_MENU,
    SH_GAME,
    SH_GAME_END,
} shScreen_t;

typedef enum
{
    SH_EASY,
    SH_MEDIUM,
    SH_HARD,
} shDifficulty_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    char* name;
    char* midi;
    char* easy;
    char* med;
    char* hard;
} shSong_t;

typedef struct
{
    int32_t tick;
    int32_t note;
    int32_t hold;
} shChartNote_t;

typedef struct
{
    int32_t note;
    int32_t headTimeUs;
    int32_t tailTimeUs;
    int32_t headPosY;
    int32_t tailPosY;
    bool held;
} shGameNote_t;

typedef struct
{
    int32_t headTimeUs;
    int32_t headPosY;
} shFretLine_t;

typedef struct
{
    // Font and menu
    font_t ibm;
    font_t righteous;
    font_t rodin;
    menu_t* menu;
    menuManiaRenderer_t* renderer;
    const char* submenu;
    const shSong_t* menuSong;
    shDifficulty_t difficulty;

    // Currently shown screen
    shScreen_t screen;

    // Song being played
    midiFile_t midiSong;
    int32_t leadInUs;
    char hsKey[16];
    const char* songName;

    // Chart data
    int32_t numChartNotes;
    shChartNote_t* chartNotes;
    int32_t currentChartNote;
    paletteColor_t const* colors;
    buttonBit_t const* noteToBtn;
    int32_t const* btnToNote;
    int32_t const* noteToIcon;
    int32_t tempo;

    // Fail meter tracking
    int32_t failMeter;
    int32_t failSampleInterval;
    list_t failSamples;

    // Setting data
    bool failOn;
    int32_t scrollTime;

    // Score data
    int32_t score;
    int32_t combo;
    int32_t maxCombo;
    bool gameEnd;
    int32_t totalNotes;
    int32_t notesHit;
    const char* grade;
    int32_t noteHistogram[NUM_NOTE_TIMINGS];

    // Fret line data
    list_t fretLines;
    int32_t lastFretLineUs;

    // Drawing data
    list_t gameNotes;
    buttonBit_t btnState;
    int32_t numFrets;
    const char* hitText;
    const char* timingText;
    int32_t textTimerUs;
    wsg_t icons[6];

    // High score display
    list_t hsStrs;
} shVars_t;

//==============================================================================
// Extern variables
//==============================================================================

extern swadgeMode_t swadgeHeroMode;

//==============================================================================
// Function Declarations
//==============================================================================

shVars_t* getShVars(void);
void shChangeScreen(shVars_t* sh, shScreen_t newScreen);
void shGetNvsKey(const char* songName, shDifficulty_t difficulty, char* key);
