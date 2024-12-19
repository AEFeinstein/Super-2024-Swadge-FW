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
#define NUM_NOTES        6
#define NUM_NOTE_FRAMES  2

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
    const char* name;
    const char* artist;
    const char* fName;
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
#ifdef SH_NOTE_DBG
    int32_t headTick;
    int32_t tailTick;
#endif
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
    int32_t leadOutUs;
    char hsKey[16];
    const char* songName;
    uint32_t songStartUs;
    bool paused;

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
    int32_t totalHitNotes; ///< Similar to numChartNotes, but counts holds as two notes (on and off)
    int32_t notesHit;
    const char* grade;
    int32_t noteHistogram[NUM_NOTE_TIMINGS];

    // Fret line data
    list_t fretLines;
    int32_t lastFretLineUs;

    // LED data
    int32_t nextBlinkUs;
    int8_t ledBaseVal;
    int32_t ledDecayTimer;
    int32_t usPerLedDecay;
    led_t ledHitVal;

    // Drawing data
    list_t gameNotes;
    buttonBit_t btnState;
    int32_t numFrets;
    const char* hitText;
    const char* timingText;
    int32_t textTimerUs;
    wsg_t icons[NUM_NOTES][NUM_NOTE_FRAMES];
    wsg_t outlines[NUM_NOTES];
    wsg_t pressed[NUM_NOTES];
    int32_t iconIdx;
    int32_t iconTimerUs;
    wsg_t star;
    list_t starList;

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
