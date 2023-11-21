/*
 * mode_slide_whistle.c
 *
 *  Created on: 21 Nov 2019
 *      Author: adam and bbkiwi
 */

/*==============================================================================
 * Includes
 *============================================================================*/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>

#include "embeddedNf.h"
#include "embeddedOut.h"
#include "esp_timer.h"
#include "linked_list.h"
#include "mainMenu.h"
#include "settingsManager.h"
#include "hdw-imu.h"

#include "slideWhistle.h"

/*==============================================================================
 * Defines
 *============================================================================*/

#define CORNER_OFFSET     14
#define TOP_TEXT_X_MARGIN CORNER_OFFSET / 2
#define LINE_BREAK_Y      8
#define TICK_HEIGHT       4
#define CURSOR_HEIGHT     5
#define CURSOR_WIDTH      1
#define BAR_X_MARGIN      0
#define BAR_Y_MARGIN      (slideWhistle->radiostars.height + CURSOR_HEIGHT + 2 + CORNER_OFFSET * 2)
#define BAR_X_WIDTH       (TFT_WIDTH - (2 * BAR_X_MARGIN) - 1)
#define BAR_Y_SPACING     (2 * CURSOR_HEIGHT + 5)

#define REST_BIT 0x10000 // Largest note is 144, which is 0b10010000

#define DEFAULT_PAUSE 5

// 270<-180->90
#define EDGE_OFFSET_DEGREES    45
#define LEFT_BOUND_DEGREES     (270 - EDGE_OFFSET_DEGREES)
#define RIGHT_BOUND_DEGREES    (90 + EDGE_OFFSET_DEGREES)
#define STEERING_RANGE_DEGREES (LEFT_BOUND_DEGREES - RIGHT_BOUND_DEGREES)

#define lengthof(x) (sizeof(x) / sizeof(x[0]))

/*==============================================================================
 * Enums
 *============================================================================*/

typedef enum
{
    THIRTYSECOND_NOTE = 3,
    THIRTYSECOND_REST = 3 | REST_BIT,
    SIXTEENTH_NOTE    = 6,
    SIXTEENTH_REST    = 6 | REST_BIT,
    EIGHTH_NOTE       = 12,
    EIGHTH_REST       = 12 | REST_BIT,
    QUARTER_NOTE      = 24,
    QUARTER_REST      = 24 | REST_BIT,
    HALF_NOTE         = 48,
    HALF_REST         = 48 | REST_BIT,
    WHOLE_NOTE        = 96,
    WHOLE_REST        = 96 | REST_BIT,

    TRIPLET_SIXTYFOURTH_NOTE  = 1,
    TRIPLET_SIXTYFOURTH_REST  = 1 | REST_BIT,
    TRIPLET_THIRTYSECOND_NOTE = 2,
    TRIPLET_THIRTYSECOND_REST = 2 | REST_BIT,
    TRIPLET_SIXTEENTH_NOTE    = 4,
    TRIPLET_SIXTEENTH_REST    = 4 | REST_BIT,
    TRIPLET_EIGHTH_NOTE       = 8,
    TRIPLET_EIGHTH_REST       = 8 | REST_BIT,
    TRIPLET_QUARTER_NOTE      = 16,
    TRIPLET_QUARTER_REST      = 16 | REST_BIT,
    TRIPLET_HALF_NOTE         = 32,
    TRIPLET_HALF_REST         = 32 | REST_BIT,
    TRIPLET_WHOLE_NOTE        = 64,
    TRIPLET_WHOLE_REST        = 64 | REST_BIT,

    DOTTED_SIXTEENTH_NOTE = 9,
    DOTTED_SIXTEENTH_REST = 9 | REST_BIT,
    DOTTED_EIGHTH_NOTE    = 18,
    DOTTED_EIGHTH_REST    = 18 | REST_BIT,
    DOTTED_QUARTER_NOTE   = 36,
    DOTTED_QUARTER_REST   = 36 | REST_BIT,
    DOTTED_HALF_NOTE      = 72,
    DOTTED_HALF_REST      = 72 | REST_BIT,
    DOTTED_WHOLE_NOTE     = 144,
    DOTTED_WHOLE_REST     = 144 | REST_BIT,
} rhythmNote_t;

/*==============================================================================
 * Prototypes
 *============================================================================*/

void slideWhistleEnterMode(void);
void slideWhistleExitMode(void);
void slideWhistleButtonCallback(buttonEvt_t* evt);
void slideWhistleProcessTouch(void);
void slideWhistleProcessAccelerometer(void);
void slideWhistleMainLoop(int64_t elapsedUs);
void slideWhistleBeatTimerFunc(void* arg __attribute__((unused)));
noteFrequency_t arpModify(noteFrequency_t note, int8_t arpInterval);
int16_t getCurrentCursorX(void);
int16_t getCurrentCursorY(void);
uint8_t getCurrentOctaveIdx(void);
noteFrequency_t getCurrentNote(void);
char* noteToStr(noteFrequency_t note);
void plotBar(uint8_t yOffset);
void noteToColor(led_t* led, noteFrequency_t note, uint8_t brightness);
bool getSfxIsMuted(void);

/*==============================================================================
 * Structs
 *============================================================================*/

typedef struct
{
    uint8_t bpmMultiplier;
    uint8_t bpm;
} bpm_t;

typedef struct
{
    rhythmNote_t note;
    int8_t arp;
} rhythmArp_t;

typedef struct
{
    // The parameter's name
    char* name;
    // The rhythm
    const rhythmArp_t* rhythm;
    const uint16_t rhythmLen;
    // Inter-note pause
    const uint16_t interNotePauseMs;
    // Default BPM
    const uint8_t defaultBpm;
} rhythm_t;

typedef struct
{
    // The parameter's name
    char* name;
    // The notes
    const noteFrequency_t* notes;
    const uint16_t notesLen;
} scale_t;

/*==============================================================================
 * Variables
 *============================================================================*/

// The swadge mode
swadgeMode_t slideWhistleMode = {.modeName                 = "Techo SlideWhistle",
                                 .wifiMode                 = NO_WIFI,
                                 .overrideUsb              = false,
                                 .usesAccelerometer        = true,
                                 .usesThermometer          = false,
                                 .overrideSelectBtn        = false,
                                 .fnEnterMode              = slideWhistleEnterMode,
                                 .fnExitMode               = slideWhistleExitMode,
                                 .fnMainLoop               = slideWhistleMainLoop,
                                 .fnAudioCallback          = NULL,
                                 .fnBackgroundDrawCallback = NULL,
                                 .fnEspNowRecvCb           = NULL,
                                 .fnEspNowSendCb           = NULL,
                                 .fnAdvancedUSB            = NULL};

// The state data
typedef struct
{
    // display_t* disp;
    font_t ibm_vga8;
    font_t radiostars;
    font_t logbook;

    // Track motion
    int16_t roll;
    char accelStr[32];
    char accelStr2[32];

    // Track touch
    bool touchHeld;
    int32_t touchPosition;
    int32_t touchIntensity;

    // Track rhythm
    esp_timer_create_args_t beatTimerArgs;
    esp_timer_handle_t beatTimer;
    uint32_t timeUs;
    uint32_t lastCallTimeUs;
    int16_t rhythmNoteIdx;
    uint8_t bpmIdx;

    // Track the button
    bool aHeld;
    bool upHeld;
    bool downHeld;
    bool shouldPlay;
    noteFrequency_t heldNote;
    int16_t heldCursorX;
    int16_t heldCursorY;
    uint8_t rhythmIdx;
    uint8_t scaleIdx;
    led_t led;
    noteFrequency_t note;
} slideWhistle_t;

slideWhistle_t* slideWhistle;

/*==============================================================================
 * Const Variables
 *============================================================================*/

// Text
const char rhythmText[]  = "<: Rhythm";
const char scaleText[]   = "PAUSE: Scale";
const char bpmText[]     = ">: BPM";
const char steerMeText[] = "Turn me like a steering wheel";
const char mutedText[]   = "Swadge is muted!";
const char holdText[]    = ": Hold";
const char playText[]    = ": Play";

// All the scales
#define NUM_OCTAVES 3
#define LOWER_OCTAVE

#ifdef LOWER_OCTAVE
const noteFrequency_t scl_M_Penta[] = {
    C_4, D_4, E_4, G_4, A_4, C_5, C_5, D_5, E_5, G_5, A_5, C_6, C_6, D_6, E_6, G_6, A_6, C_7,
};
const noteFrequency_t scl_m_Penta[] = {
    C_4, D_SHARP_4, F_4, G_4, A_SHARP_4, C_5, C_5, D_SHARP_5, F_5,
    G_5, A_SHARP_5, C_6, C_6, D_SHARP_6, F_6, G_6, A_SHARP_6, C_7,
};
const noteFrequency_t scl_Chromatic[] = {
    C_4, C_SHARP_4, D_4, D_SHARP_4, E_4, F_4, F_SHARP_4, G_4, G_SHARP_4, A_4, A_SHARP_4, B_4, C_5,
    C_5, C_SHARP_5, D_5, D_SHARP_5, E_5, F_5, F_SHARP_5, G_5, G_SHARP_5, A_5, A_SHARP_5, B_5, C_6,
    C_6, C_SHARP_6, D_6, D_SHARP_6, E_6, F_6, F_SHARP_6, G_6, G_SHARP_6, A_6, A_SHARP_6, B_6, C_7,
};
const noteFrequency_t scl_m_Blues[] = {
    C_4, D_SHARP_4, F_4, F_SHARP_4, G_4,       A_SHARP_4, C_5,       C_5, D_SHARP_5, F_5, F_SHARP_5,
    G_5, A_SHARP_5, C_6, C_6,       D_SHARP_6, F_6,       F_SHARP_6, G_6, A_SHARP_6, C_7,
};
const noteFrequency_t scl_M_Blues[] = {
    C_4, D_4, D_SHARP_4, E_4, G_4, A_4,       C_5, C_5, D_5, D_SHARP_5, E_5,
    G_5, A_5, C_6,       C_6, D_6, D_SHARP_6, E_6, G_6, A_6, C_7,
};
const noteFrequency_t scl_Major[] = {
    C_4, D_4, E_4, F_4, G_4, A_4, B_4, C_5, C_5, D_5, E_5, F_5,
    G_5, A_5, B_5, C_6, C_6, D_6, E_6, F_6, G_6, A_6, B_6, C_7,
};
const noteFrequency_t scl_Minor_Aeolian[] = {
    C_4, D_4,       D_SHARP_4, F_4, G_4, G_SHARP_4, A_SHARP_4, C_5, C_5, D_5,       D_SHARP_5, F_5,
    G_5, G_SHARP_5, A_SHARP_5, C_6, C_6, D_6,       D_SHARP_6, F_6, G_6, G_SHARP_6, A_SHARP_6, C_7,
};
const noteFrequency_t scl_Harm_Minor[] = {
    C_4, D_4,       D_SHARP_4, F_4, G_4, G_SHARP_4, B_4,       C_5, C_5, D_5,       D_SHARP_5, F_5,
    G_5, G_SHARP_5, B_5,       C_6, C_6, D_6,       D_SHARP_6, F_6, G_6, G_SHARP_6, B_6,       C_7,
};
const noteFrequency_t scl_Dorian[] = {
    C_4, D_4, D_SHARP_4, F_4, G_4, A_4, A_SHARP_4, C_5, C_5, D_5, D_SHARP_5, F_5,
    G_5, A_5, A_SHARP_5, C_6, C_6, D_6, D_SHARP_6, F_6, G_6, A_6, A_SHARP_6, C_7,
};
const noteFrequency_t scl_Phrygian[] = {
    C_4, C_SHARP_4, D_SHARP_4, F_4, G_4, G_SHARP_4, A_SHARP_4, C_5, C_5, C_SHARP_5, D_SHARP_5, F_5,
    G_5, G_SHARP_5, A_SHARP_5, C_6, C_6, C_SHARP_6, D_SHARP_6, F_6, G_6, G_SHARP_6, A_SHARP_6, C_7,
};
const noteFrequency_t scl_Lydian[] = {
    C_4, D_4, E_4, F_SHARP_4, G_4, A_4, B_4, C_5,       C_5, D_5, E_5, F_SHARP_5,
    G_5, A_5, B_5, C_6,       C_6, D_6, E_6, F_SHARP_6, G_6, A_6, B_6, C_7,
};
const noteFrequency_t scl_Mixolydian[] = {
    C_4, D_4, E_4,       F_4, G_4, A_4, A_SHARP_4, C_5, C_5, D_5, E_5,       F_5,
    G_5, A_5, A_SHARP_5, C_6, C_6, D_6, E_6,       F_6, G_6, A_6, A_SHARP_6, C_7,
};
const noteFrequency_t scl_Locrian[] = {
    C_4,       C_SHARP_4, D_SHARP_4, F_4, F_SHARP_4, G_SHARP_4, A_SHARP_4, C_5, C_5,       C_SHARP_5, D_SHARP_5, F_5,
    F_SHARP_5, G_SHARP_5, A_SHARP_5, C_6, C_6,       C_SHARP_6, D_SHARP_6, F_6, F_SHARP_6, G_SHARP_6, A_SHARP_6, C_7,
};
const noteFrequency_t scl_Dom_Bebop[] = {
    C_4, D_4,       E_4, F_4, G_4, A_4, A_SHARP_4, B_4, C_5, C_5, D_5,       E_5, F_5, G_5,
    A_5, A_SHARP_5, B_5, C_6, C_6, D_6, E_6,       F_6, G_6, A_6, A_SHARP_6, B_6, C_7,
};
const noteFrequency_t scl_M_Bebop[] = {
    C_4,       D_4,       E_4, F_4, G_4, G_SHARP_4, A_SHARP_4, B_4, C_5, C_5,       D_5,       E_5, F_5, G_5,
    G_SHARP_5, A_SHARP_5, B_5, C_6, C_6, D_6,       E_6,       F_6, G_6, G_SHARP_6, A_SHARP_6, B_6, C_7,
};
const noteFrequency_t scl_Whole_Tone[] = {
    C_4,       D_4,       E_4, F_SHARP_4, G_SHARP_4, A_SHARP_4, C_5,       C_5,       D_5,       E_5, F_SHARP_5,
    G_SHARP_5, A_SHARP_5, C_6, C_6,       D_6,       E_6,       F_SHARP_6, G_SHARP_6, A_SHARP_6, C_7,
};
const noteFrequency_t scl_Dacs[] = {
    C_4, D_SHARP_4, F_4, F_SHARP_4, G_4,       A_4, C_5,       C_5, D_SHARP_5, F_5, F_SHARP_5,
    G_5, A_5,       C_6, C_6,       D_SHARP_6, F_6, F_SHARP_6, G_6, A_6,       C_7,
};
#else
const noteFrequency_t scl_M_Penta[] = {
    C_5, D_5, E_5, G_5, A_5, C_6, C_6, D_6, E_6, G_6, A_6, C_7, C_7, D_7, E_7, G_7, A_7, C_8,
};
const noteFrequency_t scl_m_Penta[] = {
    C_5, D_SHARP_5, F_5, G_5, A_SHARP_5, C_6, C_6, D_SHARP_6, F_6,
    G_6, A_SHARP_6, C_7, C_7, D_SHARP_7, F_7, G_7, A_SHARP_7, C_8,
};
const noteFrequency_t scl_Chromatic[] = {
    C_5, C_SHARP_5, D_5, D_SHARP_5, E_5, F_5, F_SHARP_5, G_5, G_SHARP_5, A_5, A_SHARP_5, B_5, C_6,
    C_6, C_SHARP_6, D_6, D_SHARP_6, E_6, F_6, F_SHARP_6, G_6, G_SHARP_6, A_6, A_SHARP_6, B_6, C_7,
    C_7, C_SHARP_7, D_7, D_SHARP_7, E_7, F_7, F_SHARP_7, G_7, G_SHARP_7, A_7, A_SHARP_7, B_7, C_8,
};
const noteFrequency_t scl_m_Blues[] = {
    C_5, D_SHARP_5, F_5, F_SHARP_5, G_5,       A_SHARP_5, C_6,       C_6, D_SHARP_6, F_6, F_SHARP_6,
    G_6, A_SHARP_6, C_7, C_7,       D_SHARP_7, F_7,       F_SHARP_7, G_7, A_SHARP_7, C_8,
};
const noteFrequency_t scl_M_Blues[] = {
    C_5, D_5, D_SHARP_5, E_5, G_5, A_5,       C_6, C_6, D_6, D_SHARP_6, E_6,
    G_6, A_6, C_7,       C_7, D_7, D_SHARP_7, E_7, G_7, A_7, C_8,
};
const noteFrequency_t scl_Major[] = {
    C_5, D_5, E_5, F_5, G_5, A_5, B_5, C_6, C_6, D_6, E_6, F_6,
    G_6, A_6, B_6, C_7, C_7, D_7, E_7, F_7, G_7, A_7, B_7, C_8,
};
const noteFrequency_t scl_Minor_Aeolian[] = {
    C_5, D_5,       D_SHARP_5, F_5, G_5, G_SHARP_5, A_SHARP_5, C_6, C_6, D_6,       D_SHARP_6, F_6,
    G_6, G_SHARP_6, A_SHARP_6, C_7, C_7, D_7,       D_SHARP_7, F_7, G_7, G_SHARP_7, A_SHARP_7, C_8,
};
const noteFrequency_t scl_Harm_Minor[] = {
    C_5, D_5,       D_SHARP_5, F_5, G_5, G_SHARP_5, B_5,       C_6, C_6, D_6,       D_SHARP_6, F_6,
    G_6, G_SHARP_6, B_6,       C_7, C_7, D_7,       D_SHARP_7, F_7, G_7, G_SHARP_7, B_7,       C_8,
};
const noteFrequency_t scl_Dorian[] = {
    C_5, D_5, D_SHARP_5, F_5, G_5, A_5, A_SHARP_5, C_6, C_6, D_6, D_SHARP_6, F_6,
    G_6, A_6, A_SHARP_6, C_7, C_7, D_7, D_SHARP_7, F_7, G_7, A_7, A_SHARP_7, C_8,
};
const noteFrequency_t scl_Phrygian[] = {
    C_5, C_SHARP_5, D_SHARP_5, F_5, G_5, G_SHARP_5, A_SHARP_5, C_6, C_6, C_SHARP_6, D_SHARP_6, F_6,
    G_6, G_SHARP_6, A_SHARP_6, C_7, C_7, C_SHARP_7, D_SHARP_7, F_7, G_7, G_SHARP_7, A_SHARP_7, C_8,
};
const noteFrequency_t scl_Lydian[] = {
    C_5, D_5, E_5, F_SHARP_5, G_5, A_5, B_5, C_6,       C_6, D_6, E_6, F_SHARP_6,
    G_6, A_6, B_6, C_7,       C_7, D_7, E_7, F_SHARP_7, G_7, A_7, B_7, C_8,
};
const noteFrequency_t scl_Mixolydian[] = {
    C_5, D_5, E_5,       F_5, G_5, A_5, A_SHARP_5, C_6, C_6, D_6, E_6,       F_6,
    G_6, A_6, A_SHARP_6, C_7, C_7, D_7, E_7,       F_7, G_7, A_7, A_SHARP_7, C_8,
};
const noteFrequency_t scl_Locrian[] = {
    C_5,       C_SHARP_5, D_SHARP_5, F_5, F_SHARP_5, G_SHARP_5, A_SHARP_5, C_6, C_6,       C_SHARP_6, D_SHARP_6, F_6,
    F_SHARP_6, G_SHARP_6, A_SHARP_6, C_7, C_7,       C_SHARP_7, D_SHARP_7, F_7, F_SHARP_7, G_SHARP_7, A_SHARP_7, C_8,
};
const noteFrequency_t scl_Dom_Bebop[] = {
    C_5, D_5,       E_5, F_5, G_5, A_5, A_SHARP_5, B_5, C_6, C_6, D_6,       E_6, F_6, G_6,
    A_6, A_SHARP_6, B_6, C_7, C_7, D_7, E_7,       F_7, G_7, A_7, A_SHARP_7, B_7, C_8,
};
const noteFrequency_t scl_M_Bebop[] = {
    C_5,       D_5,       E_5, F_5, G_5, G_SHARP_5, A_SHARP_5, B_5, C_6, C_6,       D_6,       E_6, F_6, G_6,
    G_SHARP_6, A_SHARP_6, B_6, C_7, C_7, D_7,       E_7,       F_7, G_7, G_SHARP_7, A_SHARP_7, B_7, C_8,
};
const noteFrequency_t scl_Whole_Tone[] = {
    C_5,       D_5,       E_5, F_SHARP_5, G_SHARP_5, A_SHARP_5, C_6,       C_6,       D_6,       E_6, F_SHARP_6,
    G_SHARP_6, A_SHARP_6, C_7, C_7,       D_7,       E_7,       F_SHARP_7, G_SHARP_7, A_SHARP_7, C_8,
};
const noteFrequency_t scl_Dacs[] = {
    C_5, D_SHARP_5, F_5, F_SHARP_5, G_5,       A_5, C_6,       C_6, D_SHARP_6, F_6, F_SHARP_6,
    G_6, A_6,       C_7, C_7,       D_SHARP_7, F_7, F_SHARP_7, G_7, A_7,       C_8,
};
#endif

const scale_t scales[] = {
    {.name = "Ma Pent", .notes = scl_M_Penta, .notesLen = lengthof(scl_M_Penta)},
    {.name = "mi Pent", .notes = scl_m_Penta, .notesLen = lengthof(scl_m_Penta)},
    {.name = "Chroma", .notes = scl_Chromatic, .notesLen = lengthof(scl_Chromatic)},
    {.name = "Ma Blu", .notes = scl_M_Blues, .notesLen = lengthof(scl_M_Blues)},
    {.name = "mi Blu", .notes = scl_m_Blues, .notesLen = lengthof(scl_m_Blues)},
    {.name = "Major", .notes = scl_Major, .notesLen = lengthof(scl_Major)},
    {.name = "Minor", .notes = scl_Minor_Aeolian, .notesLen = lengthof(scl_Minor_Aeolian)},
    {.name = "H Minor", .notes = scl_Harm_Minor, .notesLen = lengthof(scl_Harm_Minor)},
    {.name = "Dorian", .notes = scl_Dorian, .notesLen = lengthof(scl_Dorian)},
    {.name = "Phrygian", .notes = scl_Phrygian, .notesLen = lengthof(scl_Phrygian)},
    {.name = "Lydian", .notes = scl_Lydian, .notesLen = lengthof(scl_Lydian)},
    {.name = "Mixolyd", .notes = scl_Mixolydian, .notesLen = lengthof(scl_Mixolydian)},
    {.name = "Locrian", .notes = scl_Locrian, .notesLen = lengthof(scl_Locrian)},
    {.name = "D Bebop", .notes = scl_Dom_Bebop, .notesLen = lengthof(scl_Dom_Bebop)},
    {.name = "M Bebop", .notes = scl_M_Bebop, .notesLen = lengthof(scl_M_Bebop)},
    {.name = "Whole T", .notes = scl_Whole_Tone, .notesLen = lengthof(scl_Whole_Tone)},
    {.name = "DACs", .notes = scl_Dacs, .notesLen = lengthof(scl_Dacs)},
};

// All the rhythms
const bpm_t bpms[] = {
    {.bpmMultiplier = 10, .bpm = 250}, {.bpmMultiplier = 11, .bpm = 227}, {.bpmMultiplier = 13, .bpm = 192},
    {.bpmMultiplier = 15, .bpm = 167}, {.bpmMultiplier = 18, .bpm = 139}, {.bpmMultiplier = 22, .bpm = 114},
    {.bpmMultiplier = 29, .bpm = 86},  {.bpmMultiplier = 41, .bpm = 61},
};

const rhythmArp_t constant[] = {
    {.note = TRIPLET_SIXTYFOURTH_NOTE, .arp = 1},
};

const rhythmArp_t one_note[] = {
    {.note = EIGHTH_NOTE, .arp = 1},
};

const rhythmArp_t octaves[] = {
    {.note = SIXTEENTH_NOTE, .arp = 1},
    {.note = SIXTEENTH_NOTE, .arp = 13},
};

const rhythmArp_t fifth[] = {
    {.note = SIXTEENTH_NOTE, .arp = 1},
    {.note = SIXTEENTH_NOTE, .arp = 8},
};

const rhythmArp_t major_tri[] = {
    {.note = SIXTEENTH_NOTE, .arp = 1},
    {.note = SIXTEENTH_NOTE, .arp = 5},
    {.note = SIXTEENTH_NOTE, .arp = 8},
};

const rhythmArp_t minor_tri[] = {
    {.note = SIXTEENTH_NOTE, .arp = 1},
    {.note = SIXTEENTH_NOTE, .arp = 4},
    {.note = SIXTEENTH_NOTE, .arp = 8},
};

const rhythmArp_t major_7[] = {
    {.note = TRIPLET_EIGHTH_NOTE, .arp = 1}, {.note = TRIPLET_EIGHTH_NOTE, .arp = 5},
    {.note = TRIPLET_EIGHTH_NOTE, .arp = 8}, {.note = TRIPLET_EIGHTH_NOTE, .arp = 12},
    {.note = TRIPLET_EIGHTH_NOTE, .arp = 8}, {.note = TRIPLET_EIGHTH_NOTE, .arp = 5},
};

const rhythmArp_t minor_7[] = {
    {.note = TRIPLET_EIGHTH_NOTE, .arp = 1}, {.note = TRIPLET_EIGHTH_NOTE, .arp = 4},
    {.note = TRIPLET_EIGHTH_NOTE, .arp = 8}, {.note = TRIPLET_EIGHTH_NOTE, .arp = 11},
    {.note = TRIPLET_EIGHTH_NOTE, .arp = 8}, {.note = TRIPLET_EIGHTH_NOTE, .arp = 4},
};

const rhythmArp_t dom_7[] = {
    {.note = TRIPLET_EIGHTH_NOTE, .arp = 1}, {.note = TRIPLET_EIGHTH_NOTE, .arp = 5},
    {.note = TRIPLET_EIGHTH_NOTE, .arp = 8}, {.note = TRIPLET_EIGHTH_NOTE, .arp = 11},
    {.note = TRIPLET_EIGHTH_NOTE, .arp = 8}, {.note = TRIPLET_EIGHTH_NOTE, .arp = 5},
};

const rhythmArp_t swing[] = {
    {.note = TRIPLET_QUARTER_NOTE, .arp = 1},
    {.note = TRIPLET_EIGHTH_NOTE, .arp = 1},
};

const rhythmArp_t syncopa[] = {
    {.note = EIGHTH_NOTE, .arp = 1}, {.note = EIGHTH_REST, .arp = 1}, {.note = EIGHTH_NOTE, .arp = 1},
    {.note = EIGHTH_NOTE, .arp = 1}, {.note = EIGHTH_REST, .arp = 1}, {.note = EIGHTH_NOTE, .arp = 1},
    {.note = EIGHTH_NOTE, .arp = 1}, {.note = EIGHTH_REST, .arp = 1},
};

const rhythmArp_t dw_stabs[] = {
    {.note = SIXTEENTH_NOTE, .arp = 1}, {.note = EIGHTH_NOTE, .arp = 1},    {.note = EIGHTH_NOTE, .arp = 1},
    {.note = SIXTEENTH_NOTE, .arp = 1}, {.note = SIXTEENTH_NOTE, .arp = 1}, {.note = SIXTEENTH_REST, .arp = 1},
    {.note = EIGHTH_NOTE, .arp = 1},    {.note = SIXTEENTH_NOTE, .arp = 1}, {.note = EIGHTH_NOTE, .arp = 1},
    {.note = SIXTEENTH_NOTE, .arp = 1}, {.note = SIXTEENTH_NOTE, .arp = 1}, {.note = SIXTEENTH_REST, .arp = 1},
};

const rhythmArp_t legendary[] = {
    {.note = QUARTER_NOTE, .arp = 1},        {.note = TRIPLET_EIGHTH_NOTE, .arp = 1},
    {.note = TRIPLET_EIGHTH_NOTE, .arp = 1}, {.note = TRIPLET_EIGHTH_NOTE, .arp = 1},
    {.note = QUARTER_NOTE, .arp = 1},        {.note = QUARTER_NOTE, .arp = 1},
};

const rhythmArp_t j_dawg[] = {
    {.note = EIGHTH_NOTE, .arp = 1}, {.note = EIGHTH_NOTE, .arp = 1}, {.note = EIGHTH_NOTE, .arp = 1},
    {.note = EIGHTH_REST, .arp = 1}, {.note = EIGHTH_REST, .arp = 1}, {.note = EIGHTH_NOTE, .arp = 1},
    {.note = EIGHTH_REST, .arp = 1}, {.note = EIGHTH_REST, .arp = 1}, {.note = EIGHTH_NOTE, .arp = 1},
    {.note = EIGHTH_REST, .arp = 1}, {.note = EIGHTH_NOTE, .arp = 1}, {.note = EIGHTH_NOTE, .arp = 1},
    {.note = EIGHTH_REST, .arp = 1}, {.note = EIGHTH_NOTE, .arp = 1},
};

const rhythmArp_t eightBMT[] = {
    {.note = EIGHTH_NOTE, .arp = 1},        {.note = EIGHTH_REST, .arp = 1},    {.note = SIXTEENTH_NOTE, .arp = 1},
    {.note = SIXTEENTH_NOTE, .arp = 1},     {.note = SIXTEENTH_REST, .arp = 1}, {.note = SIXTEENTH_NOTE, .arp = 1},
    {.note = EIGHTH_REST, .arp = 1},        {.note = EIGHTH_NOTE, .arp = 1},    {.note = SIXTEENTH_REST, .arp = 1},
    {.note = DOTTED_EIGHTH_NOTE, .arp = 1},
};

const rhythmArp_t the_goat[] = {
    {.note = EIGHTH_NOTE, .arp = 1},        {.note = EIGHTH_NOTE, .arp = 1},    {.note = EIGHTH_NOTE, .arp = 1},
    {.note = SIXTEENTH_NOTE, .arp = 1},     {.note = SIXTEENTH_NOTE, .arp = 1}, {.note = SIXTEENTH_REST, .arp = 1},
    {.note = DOTTED_EIGHTH_NOTE, .arp = 1}, {.note = EIGHTH_NOTE, .arp = 1},    {.note = EIGHTH_NOTE, .arp = 1},
};

const rhythmArp_t sgp[] = {
    {.note = DOTTED_EIGHTH_NOTE, .arp = 1},  {.note = SIXTEENTH_NOTE, .arp = 1},     {.note = EIGHTH_NOTE, .arp = 1},
    {.note = DOTTED_QUARTER_NOTE, .arp = 1}, {.note = DOTTED_EIGHTH_NOTE, .arp = 1}, {.note = SIXTEENTH_NOTE, .arp = 1},
    {.note = EIGHTH_NOTE, .arp = 1},         {.note = EIGHTH_NOTE, .arp = 1},        {.note = EIGHTH_NOTE, .arp = 1},
    {.note = EIGHTH_NOTE, .arp = 1},
};

const rhythmArp_t fourth_rock[] = {
    {.note = TRIPLET_EIGHTH_NOTE, .arp = 1}, {.note = TRIPLET_EIGHTH_NOTE, .arp = 1},
    {.note = TRIPLET_EIGHTH_NOTE, .arp = 1}, {.note = QUARTER_NOTE, .arp = 1},
    {.note = QUARTER_NOTE, .arp = 1},        {.note = EIGHTH_NOTE, .arp = 1},
    {.note = EIGHTH_NOTE, .arp = 1},         {.note = QUARTER_NOTE, .arp = 1},
};

const rhythmArp_t dub[] = {
    {.note = QUARTER_NOTE, .arp = 1},         {.note = QUARTER_NOTE, .arp = 1},
    {.note = TRIPLET_EIGHTH_NOTE, .arp = 13}, {.note = TRIPLET_EIGHTH_NOTE, .arp = 13},
    {.note = TRIPLET_EIGHTH_NOTE, .arp = 13}, {.note = TRIPLET_EIGHTH_NOTE, .arp = 13},
    {.note = TRIPLET_EIGHTH_NOTE, .arp = 13}, {.note = TRIPLET_EIGHTH_NOTE, .arp = 13},
};

const rhythmArp_t octavio[] = {
    {.note = EIGHTH_NOTE, .arp = 13}, {.note = EIGHTH_NOTE, .arp = 13}, {.note = EIGHTH_NOTE, .arp = 1},
    {.note = EIGHTH_NOTE, .arp = 13}, {.note = EIGHTH_NOTE, .arp = 1},  {.note = EIGHTH_NOTE, .arp = 1},
    {.note = EIGHTH_NOTE, .arp = 8},  {.note = EIGHTH_NOTE, .arp = 1},
};

const rhythmArp_t cha_cha[] = {
    {.note = QUARTER_NOTE, .arp = 8}, {.note = EIGHTH_NOTE, .arp = 4},  {.note = QUARTER_NOTE, .arp = 9},
    {.note = QUARTER_NOTE, .arp = 4}, {.note = QUARTER_NOTE, .arp = 8}, {.note = QUARTER_NOTE, .arp = 4},
    {.note = EIGHTH_NOTE, .arp = 9},  {.note = QUARTER_NOTE, .arp = 9}, {.note = QUARTER_NOTE, .arp = 4},
};

const rhythmArp_t its_a_me[] = {
    {.note = EIGHTH_NOTE, .arp = 5},  {.note = EIGHTH_NOTE, .arp = 5},   {.note = EIGHTH_REST, .arp = 1},
    {.note = EIGHTH_NOTE, .arp = 5},  {.note = EIGHTH_REST, .arp = 1},   {.note = EIGHTH_NOTE, .arp = 1},
    {.note = EIGHTH_NOTE, .arp = 5},  {.note = EIGHTH_REST, .arp = 1},   {.note = QUARTER_NOTE, .arp = 8},
    {.note = QUARTER_REST, .arp = 1}, {.note = QUARTER_NOTE, .arp = -6}, {.note = QUARTER_REST, .arp = 1},
};

const rhythmArp_t so_strange[] = {
    {.note = SIXTEENTH_NOTE, .arp = 1},  {.note = SIXTEENTH_NOTE, .arp = 5},  {.note = SIXTEENTH_NOTE, .arp = 8},
    {.note = SIXTEENTH_NOTE, .arp = 12}, {.note = SIXTEENTH_NOTE, .arp = 13}, {.note = SIXTEENTH_NOTE, .arp = 12},
    {.note = SIXTEENTH_NOTE, .arp = 8},  {.note = SIXTEENTH_NOTE, .arp = 5},
};

const rhythmArp_t sans[] = {
    {.note = SIXTEENTH_NOTE, .arp = 1}, {.note = SIXTEENTH_NOTE, .arp = 1}, {.note = SIXTEENTH_NOTE, .arp = 13},
    {.note = SIXTEENTH_REST, .arp = 1}, {.note = EIGHTH_NOTE, .arp = 8},    {.note = SIXTEENTH_REST, .arp = 1},
    {.note = EIGHTH_NOTE, .arp = 7},    {.note = EIGHTH_NOTE, .arp = 6},    {.note = EIGHTH_NOTE, .arp = 4},
    {.note = SIXTEENTH_NOTE, .arp = 1}, {.note = SIXTEENTH_NOTE, .arp = 4}, {.note = SIXTEENTH_NOTE, .arp = 6},
};

const rhythmArp_t ducktales_moon[] = {
    {.note = SIXTEENTH_NOTE, .arp = 1},  {.note = SIXTEENTH_NOTE, .arp = 8},  {.note = SIXTEENTH_NOTE, .arp = 13},
    {.note = SIXTEENTH_NOTE, .arp = 15}, {.note = SIXTEENTH_NOTE, .arp = 8},  {.note = SIXTEENTH_NOTE, .arp = 13},
    {.note = SIXTEENTH_NOTE, .arp = 15}, {.note = SIXTEENTH_NOTE, .arp = 18}, {.note = SIXTEENTH_NOTE, .arp = 8},
    {.note = SIXTEENTH_NOTE, .arp = 18}, {.note = SIXTEENTH_NOTE, .arp = 17}, {.note = SIXTEENTH_NOTE, .arp = 8},
    {.note = SIXTEENTH_NOTE, .arp = 17}, {.note = SIXTEENTH_NOTE, .arp = 15}, {.note = SIXTEENTH_NOTE, .arp = 13},
};

const rhythmArp_t stickerbush[] = {
    {.note = EIGHTH_NOTE, .arp = 12}, {.note = DOTTED_EIGHTH_NOTE, .arp = 13}, {.note = DOTTED_EIGHTH_NOTE, .arp = 12},
    {.note = EIGHTH_NOTE, .arp = 13}, {.note = DOTTED_EIGHTH_NOTE, .arp = 8},  {.note = SIXTEENTH_REST, .arp = 1},
    {.note = EIGHTH_REST, .arp = 1},
};

const rhythm_t rhythms[] = {{
                                .name             = "Slide",
                                .rhythm           = constant,
                                .rhythmLen        = lengthof(constant),
                                .interNotePauseMs = 0,
                                .defaultBpm       = 0 // 250
                            },
                            {
                                .name             = "Qrtr",
                                .rhythm           = one_note,
                                .rhythmLen        = lengthof(one_note),
                                .interNotePauseMs = DEFAULT_PAUSE,
                                .defaultBpm       = 5 // 114
                            },
                            {
                                .name             = "Octave",
                                .rhythm           = octaves,
                                .rhythmLen        = lengthof(octaves),
                                .interNotePauseMs = DEFAULT_PAUSE,
                                .defaultBpm       = 4 // 139
                            },
                            {
                                .name             = "Fifth",
                                .rhythm           = fifth,
                                .rhythmLen        = lengthof(fifth),
                                .interNotePauseMs = DEFAULT_PAUSE,
                                .defaultBpm       = 4 // 139
                            },
                            {
                                .name             = "Maj 3",
                                .rhythm           = major_tri,
                                .rhythmLen        = lengthof(major_tri),
                                .interNotePauseMs = DEFAULT_PAUSE,
                                .defaultBpm       = 3 // 167
                            },
                            {
                                .name             = "Min 3",
                                .rhythm           = minor_tri,
                                .rhythmLen        = lengthof(minor_tri),
                                .interNotePauseMs = DEFAULT_PAUSE,
                                .defaultBpm       = 3 // 167
                            },
                            {
                                .name             = "Maj 7",
                                .rhythm           = major_7,
                                .rhythmLen        = lengthof(major_7),
                                .interNotePauseMs = DEFAULT_PAUSE,
                                .defaultBpm       = 4 // 139
                            },
                            {
                                .name             = "Min 7",
                                .rhythm           = minor_7,
                                .rhythmLen        = lengthof(minor_7),
                                .interNotePauseMs = DEFAULT_PAUSE,
                                .defaultBpm       = 4 // 139
                            },
                            {
                                .name             = "Dom 7",
                                .rhythm           = dom_7,
                                .rhythmLen        = lengthof(dom_7),
                                .interNotePauseMs = DEFAULT_PAUSE,
                                .defaultBpm       = 4 // 139
                            },
                            {
                                .name             = "Swing",
                                .rhythm           = swing,
                                .rhythmLen        = lengthof(swing),
                                .interNotePauseMs = DEFAULT_PAUSE,
                                .defaultBpm       = 3 // 167
                            },
                            {
                                .name             = "Syncopa",
                                .rhythm           = syncopa,
                                .rhythmLen        = lengthof(syncopa),
                                .interNotePauseMs = DEFAULT_PAUSE,
                                .defaultBpm       = 2 // 192
                            },
                            {
                                .name             = "DW_stabs",
                                .rhythm           = dw_stabs,
                                .rhythmLen        = lengthof(dw_stabs),
                                .interNotePauseMs = DEFAULT_PAUSE,
                                .defaultBpm       = 6 // 86
                            },
                            {
                                .name             = "Lgnd",
                                .rhythm           = legendary,
                                .rhythmLen        = lengthof(legendary),
                                .interNotePauseMs = DEFAULT_PAUSE,
                                .defaultBpm       = 4 // 139
                            },
                            // {
                            //     .name = "J-Dawg",
                            //     .rhythm = j_dawg,
                            //     .rhythmLen = lengthof(j_dawg),
                            //     .interNotePauseMs = DEFAULT_PAUSE,
                            //     .defaultBpm = 1 // 227
                            // },
                            {
                                .name             = "8BMT",
                                .rhythm           = eightBMT,
                                .rhythmLen        = lengthof(eightBMT),
                                .interNotePauseMs = DEFAULT_PAUSE,
                                .defaultBpm       = 3 // 167
                            },
                            // {
                            //     .name = "Goat",
                            //     .rhythm = the_goat,
                            //     .rhythmLen = lengthof(the_goat),
                            //     .interNotePauseMs = DEFAULT_PAUSE,
                            //     .defaultBpm = 5 // 114
                            // },
                            // {
                            //     .name = "Sgp",
                            //     .rhythm = sgp,
                            //     .rhythmLen = lengthof(sgp),
                            //     .interNotePauseMs = DEFAULT_PAUSE,
                            //     .defaultBpm = 3 // 167
                            // },
                            {
                                .name             = "Mars",
                                .rhythm           = fourth_rock,
                                .rhythmLen        = lengthof(fourth_rock),
                                .interNotePauseMs = DEFAULT_PAUSE,
                                .defaultBpm       = 4 // 139
                            },
                            {
                                .name             = "Dub",
                                .rhythm           = dub,
                                .rhythmLen        = lengthof(dub),
                                .interNotePauseMs = DEFAULT_PAUSE,
                                .defaultBpm       = 4 // 139
                            },
                            {
                                .name             = "Octavio",
                                .rhythm           = octavio,
                                .rhythmLen        = lengthof(octavio),
                                .interNotePauseMs = DEFAULT_PAUSE,
                                .defaultBpm       = 0 // 250
                            },
                            {
                                .name             = "Cha-Cha",
                                .rhythm           = cha_cha,
                                .rhythmLen        = lengthof(cha_cha),
                                .interNotePauseMs = DEFAULT_PAUSE,
                                .defaultBpm       = 3 // 167
                            },
                            {
                                .name             = "It's-A-Me",
                                .rhythm           = its_a_me,
                                .rhythmLen        = lengthof(its_a_me),
                                .interNotePauseMs = DEFAULT_PAUSE,
                                .defaultBpm       = 3 // 167
                            },
                            {
                                .name             = "Strange",
                                .rhythm           = so_strange,
                                .rhythmLen        = lengthof(so_strange),
                                .interNotePauseMs = DEFAULT_PAUSE,
                                .defaultBpm       = 4 // 139
                            },
                            {
                                .name             = "Sans?",
                                .rhythm           = sans,
                                .rhythmLen        = lengthof(sans),
                                .interNotePauseMs = DEFAULT_PAUSE,
                                .defaultBpm       = 5 // 114
                            },
                            {
                                .name             = "Quack",
                                .rhythm           = ducktales_moon,
                                .rhythmLen        = lengthof(ducktales_moon),
                                .interNotePauseMs = DEFAULT_PAUSE,
                                .defaultBpm       = 6 // 86
                            },
                            {
                                .name             = "Stckrbsh",
                                .rhythm           = stickerbush,
                                .rhythmLen        = lengthof(stickerbush),
                                .interNotePauseMs = DEFAULT_PAUSE,
                                .defaultBpm       = 6 // 86
                            }};

const noteFrequency_t allNotes[] = {
    C_0,  C_SHARP_0,  D_0,  D_SHARP_0,  E_0,  F_0,  F_SHARP_0,  G_0,  G_SHARP_0,  A_0,  A_SHARP_0,  B_0,

    C_1,  C_SHARP_1,  D_1,  D_SHARP_1,  E_1,  F_1,  F_SHARP_1,  G_1,  G_SHARP_1,  A_1,  A_SHARP_1,  B_1,

    C_2,  C_SHARP_2,  D_2,  D_SHARP_2,  E_2,  F_2,  F_SHARP_2,  G_2,  G_SHARP_2,  A_2,  A_SHARP_2,  B_2,

    C_3,  C_SHARP_3,  D_3,  D_SHARP_3,  E_3,  F_3,  F_SHARP_3,  G_3,  G_SHARP_3,  A_3,  A_SHARP_3,  B_3,

    C_4,  C_SHARP_4,  D_4,  D_SHARP_4,  E_4,  F_4,  F_SHARP_4,  G_4,  G_SHARP_4,  A_4,  A_SHARP_4,  B_4,

    C_5,  C_SHARP_5,  D_5,  D_SHARP_5,  E_5,  F_5,  F_SHARP_5,  G_5,  G_SHARP_5,  A_5,  A_SHARP_5,  B_5,

    C_6,  C_SHARP_6,  D_6,  D_SHARP_6,  E_6,  F_6,  F_SHARP_6,  G_6,  G_SHARP_6,  A_6,  A_SHARP_6,  B_6,

    C_7,  C_SHARP_7,  D_7,  D_SHARP_7,  E_7,  F_7,  F_SHARP_7,  G_7,  G_SHARP_7,  A_7,  A_SHARP_7,  B_7,

    C_8,  C_SHARP_8,  D_8,  D_SHARP_8,  E_8,  F_8,  F_SHARP_8,  G_8,  G_SHARP_8,  A_8,  A_SHARP_8,  B_8,

    C_9,  C_SHARP_9,  D_9,  D_SHARP_9,  E_9,  F_9,  F_SHARP_9,  G_9,  G_SHARP_9,  A_9,  A_SHARP_9,  B_9,

    C_10, C_SHARP_10, D_10, D_SHARP_10, E_10, F_10, F_SHARP_10, G_10, G_SHARP_10, A_10, A_SHARP_10, B_10,
};

/*============================================================================
 * Functions
 *==========================================================================*/

/**
 * Initializer for slideWhistle
 */
void slideWhistleEnterMode(void)
{
    // Allocate zero'd memory for the mode
    slideWhistle = calloc(1, sizeof(slideWhistle_t));

    loadFont("ibm_vga8.font", &slideWhistle->ibm_vga8, false);
    loadFont("radiostars.font", &slideWhistle->radiostars, false);
    loadFont("logbook.font", &slideWhistle->logbook, false);

    bzrStopNote(BZR_STEREO);
    slideWhistle->note = SILENCE;

    // Set default BPM to default
    slideWhistle->bpmIdx = rhythms[slideWhistle->rhythmIdx].defaultBpm;

    // Set a timer to tick every 1ms, forever
    slideWhistle->beatTimerArgs.arg                   = NULL;
    slideWhistle->beatTimerArgs.callback              = slideWhistleBeatTimerFunc;
    slideWhistle->beatTimerArgs.dispatch_method       = ESP_TIMER_TASK;
    slideWhistle->beatTimerArgs.name                  = "slideWhistleBeatTimer";
    slideWhistle->beatTimerArgs.skip_unhandled_events = true;
    esp_timer_create(&slideWhistle->beatTimerArgs, &slideWhistle->beatTimer);
    if (!getSfxIsMuted())
    {
        esp_timer_start_periodic(slideWhistle->beatTimer, 1000);
    }

    // We shold go as fast as we can
    // setFrameRateUs(0);

    // Set up the IMU
    accelSetRegistersAndReset();
}

/**
 * Called when slideWhistle is exited
 */
void slideWhistleExitMode(void)
{
    bzrStopNote(BZR_STEREO);
    slideWhistle->note = SILENCE;

    freeFont(&slideWhistle->ibm_vga8);
    freeFont(&slideWhistle->radiostars);
    freeFont(&slideWhistle->logbook);

    esp_timer_stop(slideWhistle->beatTimer);

    esp_timer_delete(slideWhistle->beatTimer);

    free(slideWhistle);
}

/**
 * This function is called when a button press is pressed. Buttons are
 * handled by interrupts and queued up for this callback, so there are no
 * strict timing restrictions for this function.
 *
 */
void slideWhistleProcessTouch(void)
{
    int32_t phi, r, intensity;
    if (getTouchJoystick(&phi, &r, &intensity))
    {
        slideWhistle->rhythmNoteIdx  = 0;
        slideWhistle->lastCallTimeUs = 0;
        slideWhistle->touchHeld      = true;
        slideWhistle->shouldPlay     = true;
        slideWhistle->touchPosition  = phi;
        slideWhistle->touchIntensity = intensity;
    }
    else
    {
        slideWhistle->touchHeld  = false;
        slideWhistle->shouldPlay = slideWhistle->aHeld;
    }

    // snprintf(slideWhistle->accelStr, sizeof(slideWhistle->accelStr), "phi %n\n",
    //        slideWhistle->touchPosition);

    // Clamp touch value and map to the proper scale
    slideWhistle->touchPosition = roundf((slideWhistle->touchPosition / 360.0f) * (TFT_WIDTH - 1));
}

/**
 * @brief Button callback function, plays notes and switches parameters
 *
 * @param evt The button event that occurred
 */
void slideWhistleButtonCallback(buttonEvt_t* evt)
{
    switch (evt->button)
    {
        case PB_START:
        {
            if (evt->down)
            {
                // Cycle the scale
                slideWhistle->scaleIdx = (slideWhistle->scaleIdx + 1) % lengthof(scales);
            }
            break;
        }
        case PB_LEFT:
        {
            if (evt->down)
            {
                // Cycle the rhythm
                slideWhistle->rhythmIdx      = (slideWhistle->rhythmIdx + 1) % lengthof(rhythms);
                slideWhistle->timeUs         = 0;
                slideWhistle->rhythmNoteIdx  = 0;
                slideWhistle->lastCallTimeUs = 0;
                slideWhistle->bpmIdx         = rhythms[slideWhistle->rhythmIdx].defaultBpm;
            }

            break;
        }
        case PB_A:
        {
            slideWhistle->rhythmNoteIdx  = 0;
            slideWhistle->lastCallTimeUs = 0;
            slideWhistle->aHeld          = evt->down;
            slideWhistle->shouldPlay     = evt->down || slideWhistle->touchHeld;
            break;
        }
        case PB_B:
        {
            if (evt->down)
            {
                slideWhistle->heldNote    = getCurrentNote();
                slideWhistle->heldCursorX = getCurrentCursorX();
                slideWhistle->heldCursorY = getCurrentCursorY();
            }
            else
            {
                slideWhistle->heldNote    = SILENCE;
                slideWhistle->heldCursorX = 0;
                slideWhistle->heldCursorY = 0;
            }
            break;
        }
        case PB_UP:
        {
            slideWhistle->upHeld = evt->down;
            break;
        }
        case PB_DOWN:
        {
            slideWhistle->downHeld = evt->down;
            break;
        }
        case PB_RIGHT:
        {
            if (evt->down)
            {
                if (slideWhistle->bpmIdx == 0)
                {
                    slideWhistle->bpmIdx = lengthof(bpms);
                }
                slideWhistle->bpmIdx = slideWhistle->bpmIdx - 1;
            }
            break;
        }
        default:
        {
            break;
        }
    }
}

/**
 * @brief Callback function for accelerometer values
 * Use the current vector to find pitch and roll, then update the display
 *
 */
void slideWhistleProcessAccelerometer(void)
{
    // Get steering angle: 0..360 with 180 being up.
    int16_t xcomp, ycomp;
    int16_t steeringAngleDegrees = 180;
    if (ESP_OK == accelIntegrate() && ESP_OK == accelGetSteeringAngleDegrees(&xcomp, &ycomp))
    {
        steeringAngleDegrees = getAtan2(xcomp, ycomp);
        // snprintf(slideWhistle->accelStr, sizeof(slideWhistle->accelStr), "%d %d %d\n",
        //     steeringAngleDegrees, xcomp, ycomp);
    }

    // Clamp to the range of steering angles we care about, then normalize from 0 to 1
    float clampedSteering = CLAMP(steeringAngleDegrees, RIGHT_BOUND_DEGREES, LEFT_BOUND_DEGREES) - RIGHT_BOUND_DEGREES;
    clampedSteering /= STEERING_RANGE_DEGREES;

    // Conver to a number between 0 and TFT_WIDTH - 1
    slideWhistle->roll = roundf(clampedSteering * (TFT_WIDTH - 1));
}

/**
 * Update the display by drawing the current state of affairs
 */
void slideWhistleMainLoop(int64_t elapsedUs)
{
    slideWhistleProcessAccelerometer();

    slideWhistleProcessTouch();

    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        slideWhistleButtonCallback(&evt);
    }

    clearPxTft();
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c100);

    // Plot the bars
    for (int i = 0; i < NUM_OCTAVES; i++)
    {
        plotBar(TFT_HEIGHT - BAR_Y_MARGIN - i * BAR_Y_SPACING);
    }

    if (slideWhistle->heldNote != SILENCE)
    {
        // Plot the cursor for the held note
        fillDisplayArea(slideWhistle->heldCursorX - CURSOR_WIDTH, slideWhistle->heldCursorY - CURSOR_HEIGHT,
                        slideWhistle->heldCursorX + CURSOR_WIDTH, slideWhistle->heldCursorY + CURSOR_HEIGHT, c511);
    }

    // Plot the cursor
    fillDisplayArea(getCurrentCursorX() - CURSOR_WIDTH, getCurrentCursorY() - CURSOR_HEIGHT,
                    getCurrentCursorX() + CURSOR_WIDTH, getCurrentCursorY() + CURSOR_HEIGHT, c551);

    // Plot instructions
    drawText(&slideWhistle->ibm_vga8, c235, steerMeText,
             (TFT_WIDTH - textWidth(&slideWhistle->ibm_vga8, steerMeText)) / 2, TOP_TEXT_X_MARGIN);

    int16_t secondLineStartY = slideWhistle->ibm_vga8.height + LINE_BREAK_Y + TOP_TEXT_X_MARGIN;

    // Plot the rhythm
    drawText(&slideWhistle->radiostars, c444, rhythmText, CORNER_OFFSET, secondLineStartY);
    drawText(&slideWhistle->radiostars, c555, rhythms[slideWhistle->rhythmIdx].name,
             TFT_WIDTH - textWidth(&slideWhistle->radiostars, rhythms[slideWhistle->rhythmIdx].name) - CORNER_OFFSET,
             secondLineStartY);

    // Plot the scale
    drawText(&slideWhistle->radiostars, c444, scaleText, CORNER_OFFSET,
             secondLineStartY + slideWhistle->radiostars.height + LINE_BREAK_Y);
    drawText(&slideWhistle->radiostars, c555, scales[slideWhistle->scaleIdx].name,
             TFT_WIDTH - textWidth(&slideWhistle->radiostars, scales[slideWhistle->scaleIdx].name) - CORNER_OFFSET,
             secondLineStartY + slideWhistle->radiostars.height + LINE_BREAK_Y);

    // Plot the BPM
    drawText(&slideWhistle->radiostars, c444, bpmText, CORNER_OFFSET,
             secondLineStartY + (slideWhistle->radiostars.height + LINE_BREAK_Y) * 2);

    char bpmStr[16] = {0};
    snprintf(bpmStr, sizeof(bpmStr), "%d", bpms[slideWhistle->bpmIdx].bpm);
    drawText(&slideWhistle->radiostars, c555, bpmStr,
             TFT_WIDTH - textWidth(&slideWhistle->radiostars, bpmStr) - CORNER_OFFSET,
             secondLineStartY + (slideWhistle->radiostars.height + LINE_BREAK_Y) * 2);

    // Debug print
    drawText(&slideWhistle->ibm_vga8, c444, slideWhistle->accelStr, 0, TFT_HEIGHT / 4 + CORNER_OFFSET + 10);
    drawText(&slideWhistle->ibm_vga8, c444, slideWhistle->accelStr2, 0,
             TFT_HEIGHT / 4 + slideWhistle->ibm_vga8.height + 11 + CORNER_OFFSET);

    // Warn the user that the swadge is muted, if that's the case
    if (getSfxIsMuted())
    {
        drawText(&slideWhistle->radiostars, c551, mutedText,
                 (TFT_WIDTH - textWidth(&slideWhistle->radiostars, mutedText)) / 2, TFT_HEIGHT / 2);
    }
    else
    {
        // Plot the note
        drawText(&slideWhistle->logbook, c555, noteToStr(getCurrentNote()),
                 (TFT_WIDTH - textWidth(&slideWhistle->logbook, noteToStr(getCurrentNote()))) / 2, TFT_HEIGHT / 2);
    }

    // Plot the button funcs
    int16_t afterText = drawText(&slideWhistle->radiostars, c511, "B", CORNER_OFFSET,
                                 TFT_HEIGHT - slideWhistle->radiostars.height - CORNER_OFFSET);
    drawText(&slideWhistle->radiostars, c444, holdText, afterText,
             TFT_HEIGHT - slideWhistle->radiostars.height - CORNER_OFFSET);

    afterText = drawText(&slideWhistle->radiostars, c444, "PAD/",
                         TFT_WIDTH - textWidth(&slideWhistle->radiostars, playText)
                             - textWidth(&slideWhistle->radiostars, "PAD/A") - CORNER_OFFSET,
                         TFT_HEIGHT - slideWhistle->radiostars.height - CORNER_OFFSET);
    afterText = drawText(&slideWhistle->radiostars, c151, "A", afterText,
                         TFT_HEIGHT - slideWhistle->radiostars.height - CORNER_OFFSET);
    drawText(&slideWhistle->radiostars, c444, playText, afterText,
             TFT_HEIGHT - slideWhistle->radiostars.height - CORNER_OFFSET);

    // Copy LED color from the first LED to all of them
    led_t leds[CONFIG_NUM_LEDS] = {{0}};
    for (uint8_t ledIdx = 1; ledIdx < CONFIG_NUM_LEDS; ledIdx++)
    {
        leds[ledIdx].r = slideWhistle->led.r;
        leds[ledIdx].g = slideWhistle->led.g;
        leds[ledIdx].b = slideWhistle->led.b;
    }
    setLeds(leds, CONFIG_NUM_LEDS);
}

/**
 * @brief Plot a horizontal bar indicating where the note boundaries are
 *
 * @param yOffset The Y Offset of the middle of the bar, not the ticks
 */
void plotBar(uint8_t yOffset)
{
    // Plot the main bar
    drawLine(BAR_X_MARGIN, yOffset, TFT_WIDTH - BAR_X_MARGIN, yOffset, c555, 0);

    // Plot tick marks at each of the note boundaries
    for (uint8_t tick = 0; tick < (scales[slideWhistle->scaleIdx].notesLen / NUM_OCTAVES) + 1; tick++)
    {
        int16_t x = BAR_X_MARGIN
                    + ((tick * ((TFT_WIDTH - 1) - (BAR_X_MARGIN * 2)))
                       / (scales[slideWhistle->scaleIdx].notesLen / NUM_OCTAVES));
        drawLine(x, yOffset - TICK_HEIGHT, x, yOffset + TICK_HEIGHT, c555, 0);
    }
}

/**
 * @brief Called every 1ms to handle the rhythm, arpeggios, and setting the buzzer
 *
 * @param arg unused
 */
void slideWhistleBeatTimerFunc(void* arg __attribute__((unused)))
{
    // Keep track of time with microsecond precision
    uint32_t currentCallTimeUs = esp_timer_get_time();

    // These bools control what we should do
    bool shouldPlayNote   = false;
    bool isFirstNote      = false;
    bool isInterNotePause = false;

    // Figure out what to do based on the time
    if (0 == slideWhistle->lastCallTimeUs)
    {
        // Being called for the fist time, start playing the first note
        slideWhistle->timeUs         = 0;
        slideWhistle->lastCallTimeUs = currentCallTimeUs;
        shouldPlayNote               = true;
        isFirstNote                  = true;
    }
    else
    {
        // Figure out the delta between calls in microseconds, then increment time
        slideWhistle->timeUs += (currentCallTimeUs - slideWhistle->lastCallTimeUs);
        slideWhistle->lastCallTimeUs = currentCallTimeUs;

        // Find the time where we should change notes
        uint32_t rhythmIntervalUs
            = (1000 * bpms[slideWhistle->bpmIdx].bpmMultiplier
               * ((~REST_BIT) & rhythms[slideWhistle->rhythmIdx].rhythm[slideWhistle->rhythmNoteIdx].note));
        // If time crossed a rhythm boundary do something different
        if (slideWhistle->timeUs >= rhythmIntervalUs)
        {
            slideWhistle->timeUs -= rhythmIntervalUs;
            shouldPlayNote = true;
        }
        // Or if it crossed the boundary into an inter-note pause
        else if (slideWhistle->timeUs >= rhythmIntervalUs - (1000 * rhythms[slideWhistle->rhythmIdx].interNotePauseMs))
        {
            shouldPlayNote   = true;
            isInterNotePause = true;
        }
        else
        {
            // Nothing to do here
            return;
        }
    }

    // If this is an internote pause
    if (isInterNotePause)
    {
        // Turn off the buzzer and set the LEDs to dim
        bzrStopNote(BZR_STEREO);
        slideWhistle->note = SILENCE;
        noteToColor(&slideWhistle->led, getCurrentNote(), 0x20);
    }
    else if (true == shouldPlayNote)
    {
        // If this isn't the first note (we need to start the first note sometime)
        if (false == isFirstNote)
        {
            // Move to play the next note
            slideWhistle->rhythmNoteIdx
                = (slideWhistle->rhythmNoteIdx + 1) % rhythms[slideWhistle->rhythmIdx].rhythmLen;
        }

        // If the button isn't held, or the BPM is being modified, or this is a rest
        if (!slideWhistle->shouldPlay
            || (rhythms[slideWhistle->rhythmIdx].rhythm[slideWhistle->rhythmNoteIdx].note & REST_BIT))
        {
            // Turn off the buzzer and set the LEDs to dim
            bzrStopNote(BZR_STEREO);
            slideWhistle->note = SILENCE;
            noteToColor(&slideWhistle->led, getCurrentNote(), 0x20);
        }
        else
        {
            // Arpeggiate as necessary
            noteFrequency_t nextNote
                = arpModify(getCurrentNote(), rhythms[slideWhistle->rhythmIdx].rhythm[slideWhistle->rhythmNoteIdx].arp);
            if (nextNote != slideWhistle->note)
            {
                bzrPlayNote(nextNote, BZR_STEREO, volLevelFromSetting(getSfxVolumeSetting()));
                slideWhistle->note = nextNote;
            }
            // Set LEDs to bright
            noteToColor(&slideWhistle->led, getCurrentNote(), 0x40);
        }
    }
    else
    {
        return;
    }
}

/**
 * @return the current X coordinate of the cursor
 *
 */
int16_t getCurrentCursorX(void)
{
    if (slideWhistle->touchHeld)
    {
        return slideWhistle->touchPosition;
    }
    else
    {
        return slideWhistle->roll;
    }
}

/**
 * @return the current Y coordinate of the cursor
 *
 */
int16_t getCurrentCursorY(void)
{
    return TFT_HEIGHT - BAR_Y_MARGIN - getCurrentOctaveIdx() * BAR_Y_SPACING;
}

/**
 * @return the current octave index selected
 *
 */
uint8_t getCurrentOctaveIdx(void)
{
    if (slideWhistle->upHeld && !slideWhistle->downHeld)
    {
        return 2;
    }
    else if (slideWhistle->downHeld && !slideWhistle->upHeld)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

/**
 * @return the current note the angle or touch position coresponds to.
 * doesn't matter if it should be played right now or not
 *
 */
noteFrequency_t getCurrentNote(void)
{
    if (slideWhistle->heldNote != SILENCE)
    {
        return slideWhistle->heldNote;
    }

    // Get the index of the note to play
    uint8_t noteIdx = ((getCurrentCursorX() - BAR_X_MARGIN) * (scales[slideWhistle->scaleIdx].notesLen / NUM_OCTAVES))
                      / (BAR_X_WIDTH + 1);

    // See which octave we should play
    if (slideWhistle->upHeld && !slideWhistle->downHeld)
    {
        uint8_t offset = scales[slideWhistle->scaleIdx].notesLen / NUM_OCTAVES;
        return scales[slideWhistle->scaleIdx].notes[noteIdx + (offset * 2)];
    }
    else if (slideWhistle->downHeld && !slideWhistle->upHeld)
    {
        return scales[slideWhistle->scaleIdx].notes[noteIdx];
    }
    else
    {
        uint8_t offset = scales[slideWhistle->scaleIdx].notesLen / NUM_OCTAVES;
        return scales[slideWhistle->scaleIdx].notes[noteIdx + offset];
    }
}

/**
 * @brief Arpeggiate a note
 *
 * @param note The root note to arpeggiate
 * @param arpInterval The interval to arpeggiate it by
 * @return noteFrequency_t The arpeggiated note
 */
noteFrequency_t arpModify(noteFrequency_t note, int8_t arpInterval)
{
    // Don't need to do anything for these
    if (1 == arpInterval || 0 == arpInterval)
    {
        return note;
    }

    // First find the note in the list of all notes
    for (uint16_t i = 0; i < lengthof(allNotes); i++)
    {
        if (note == allNotes[i])
        {
            if (arpInterval < 0)
            {
                // Then shift down by arpInterval
                while (++arpInterval)
                {
                    i--;
                }
            }
            else if (arpInterval > 0)
            {
                // Then shift up by arpInterval
                while (--arpInterval)
                {
                    i++;
                }
            }

            // Then return the arpeggiated note
            return allNotes[i];
        }
    }
    return note;
}

/**
 * @brief Translate a musical note to a color
 *
 * @param led The led_t to write the color data to
 * @param note The note to translate to color
 * @param brightness The brightness of the LED
 */
void noteToColor(led_t* led, noteFrequency_t note, uint8_t brightness)
{
    // First find the note in the list of all notes
    for (uint16_t idx = 0; idx < lengthof(allNotes); idx++)
    {
        if (note == allNotes[idx])
        {
            idx = idx % 12;
            idx = (idx * 255) / 12;

            uint32_t col = ECCtoHEX(idx, 0xFF, brightness);
            led->r       = (col >> 16) & 0xFF;
            led->g       = (col >> 8) & 0xFF;
            led->b       = (col >> 0) & 0xFF;
            return;
        }
    }
}

bool getSfxIsMuted(void)
{
    return getSfxVolumeSetting() == 0;
}

/**
 * @brief Translate a musical note to a string. Only covers the notes we play
 *
 * @param note The note to translate to color
 * @return A null terminated string for this note
 */
char* noteToStr(noteFrequency_t note)
{
    switch (note)
    {
        case SILENCE:
        {
            return "SILENCE";
        }
        case C_0:
        {
            return "C0";
        }
        case C_SHARP_0:
        {
            return "C#0";
        }
        case D_0:
        {
            return "D0";
        }
        case D_SHARP_0:
        {
            return "D#0";
        }
        case E_0:
        {
            return "E0";
        }
        case F_0:
        {
            return "F0";
        }
        case F_SHARP_0:
        {
            return "F#0";
        }
        case G_0:
        {
            return "G0";
        }
        case G_SHARP_0:
        {
            return "G#0";
        }
        case A_0:
        {
            return "A0";
        }
        case A_SHARP_0:
        {
            return "A#0";
        }
        case B_0:
        {
            return "B0";
        }
        case C_1:
        {
            return "C1";
        }
        case C_SHARP_1:
        {
            return "C#1";
        }
        case D_1:
        {
            return "D1";
        }
        case D_SHARP_1:
        {
            return "D#1";
        }
        case E_1:
        {
            return "E1";
        }
        case F_1:
        {
            return "F1";
        }
        case F_SHARP_1:
        {
            return "F#1";
        }
        case G_1:
        {
            return "G1";
        }
        case G_SHARP_1:
        {
            return "G#1";
        }
        case A_1:
        {
            return "A1";
        }
        case A_SHARP_1:
        {
            return "A#1";
        }
        case B_1:
        {
            return "B1";
        }
        case C_2:
        {
            return "C2";
        }
        case C_SHARP_2:
        {
            return "C#2";
        }
        case D_2:
        {
            return "D2";
        }
        case D_SHARP_2:
        {
            return "D#2";
        }
        case E_2:
        {
            return "E2";
        }
        case F_2:
        {
            return "F2";
        }
        case F_SHARP_2:
        {
            return "F#2";
        }
        case G_2:
        {
            return "G2";
        }
        case G_SHARP_2:
        {
            return "G#2";
        }
        case A_2:
        {
            return "A2";
        }
        case A_SHARP_2:
        {
            return "A#2";
        }
        case B_2:
        {
            return "B2";
        }
        case C_3:
        {
            return "C3";
        }
        case C_SHARP_3:
        {
            return "C#3";
        }
        case D_3:
        {
            return "D3";
        }
        case D_SHARP_3:
        {
            return "D#3";
        }
        case E_3:
        {
            return "E3";
        }
        case F_3:
        {
            return "F3";
        }
        case F_SHARP_3:
        {
            return "F#3";
        }
        case G_3:
        {
            return "G3";
        }
        case G_SHARP_3:
        {
            return "G#3";
        }
        case A_3:
        {
            return "A3";
        }
        case A_SHARP_3:
        {
            return "A#3";
        }
        case B_3:
        {
            return "B3";
        }
        case C_4:
        {
            return "C4";
        }
        case C_SHARP_4:
        {
            return "C#4";
        }
        case D_4:
        {
            return "D4";
        }
        case D_SHARP_4:
        {
            return "D#4";
        }
        case E_4:
        {
            return "E4";
        }
        case F_4:
        {
            return "F4";
        }
        case F_SHARP_4:
        {
            return "F#4";
        }
        case G_4:
        {
            return "G4";
        }
        case G_SHARP_4:
        {
            return "G#4";
        }
        case A_4:
        {
            return "A4";
        }
        case A_SHARP_4:
        {
            return "A#4";
        }
        case B_4:
        {
            return "B4";
        }
        case C_5:
        {
            return "C5";
        }
        case C_SHARP_5:
        {
            return "C#5";
        }
        case D_5:
        {
            return "D5";
        }
        case D_SHARP_5:
        {
            return "D#5";
        }
        case E_5:
        {
            return "E5";
        }
        case F_5:
        {
            return "F5";
        }
        case F_SHARP_5:
        {
            return "F#5";
        }
        case G_5:
        {
            return "G5";
        }
        case G_SHARP_5:
        {
            return "G#5";
        }
        case A_5:
        {
            return "A5";
        }
        case A_SHARP_5:
        {
            return "A#5";
        }
        case B_5:
        {
            return "B5";
        }
        case C_6:
        {
            return "C6";
        }
        case C_SHARP_6:
        {
            return "C#6";
        }
        case D_6:
        {
            return "D6";
        }
        case D_SHARP_6:
        {
            return "D#6";
        }
        case E_6:
        {
            return "E6";
        }
        case F_6:
        {
            return "F6";
        }
        case F_SHARP_6:
        {
            return "F#6";
        }
        case G_6:
        {
            return "G6";
        }
        case G_SHARP_6:
        {
            return "G#6";
        }
        case A_6:
        {
            return "A6";
        }
        case A_SHARP_6:
        {
            return "A#6";
        }
        case B_6:
        {
            return "B6";
        }
        case C_7:
        {
            return "C7";
        }
        case C_SHARP_7:
        {
            return "C#7";
        }
        case D_7:
        {
            return "D7";
        }
        case D_SHARP_7:
        {
            return "D#7";
        }
        case E_7:
        {
            return "E7";
        }
        case F_7:
        {
            return "F7";
        }
        case F_SHARP_7:
        {
            return "F#7";
        }
        case G_7:
        {
            return "G7";
        }
        case G_SHARP_7:
        {
            return "G#7";
        }
        case A_7:
        {
            return "A7";
        }
        case A_SHARP_7:
        {
            return "A#7";
        }
        case B_7:
        {
            return "B7";
        }
        case C_8:
        {
            return "C8";
        }
        case C_SHARP_8:
        {
            return "C#8";
        }
        case D_8:
        {
            return "D8";
        }
        case D_SHARP_8:
        {
            return "D#8";
        }
        case E_8:
        {
            return "E8";
        }
        case F_8:
        {
            return "F8";
        }
        case F_SHARP_8:
        {
            return "F#8";
        }
        case G_8:
        {
            return "G8";
        }
        case G_SHARP_8:
        {
            return "G#8";
        }
        case A_8:
        {
            return "A8";
        }
        case A_SHARP_8:
        {
            return "A#8";
        }
        case B_8:
        {
            return "B8";
        }
        case C_9:
        {
            return "C9";
        }
        case C_SHARP_9:
        {
            return "C#9";
        }
        case D_9:
        {
            return "D9";
        }
        case D_SHARP_9:
        {
            return "D#9";
        }
        case E_9:
        {
            return "E9";
        }
        case F_9:
        {
            return "F9";
        }
        case F_SHARP_9:
        {
            return "F#9";
        }
        case G_9:
        {
            return "G9";
        }
        case G_SHARP_9:
        {
            return "G#9";
        }
        case A_9:
        {
            return "A9";
        }
        case A_SHARP_9:
        {
            return "A#9";
        }
        case B_9:
        {
            return "B9";
        }
        case C_10:
        {
            return "C10";
        }
        case C_SHARP_10:
        {
            return "C#10";
        }
        case D_10:
        {
            return "D10";
        }
        case D_SHARP_10:
        {
            return "D#10";
        }
        case E_10:
        {
            return "E10";
        }
        case F_10:
        {
            return "F10";
        }
        case F_SHARP_10:
        {
            return "F#10";
        }
        case G_10:
        {
            return "G10";
        }
        case G_SHARP_10:
        {
            return "G#10";
        }
        case A_10:
        {
            return "A10";
        }
        case A_SHARP_10:
        {
            return "A#10";
        }
        case B_10:
        {
            return "B10";
        }
        default:
        {
            return "";
        }
    }
}
