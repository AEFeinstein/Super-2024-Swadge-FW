#ifndef _PA_SOUNDMANAGER_H_
#define _PA_SOUNDMANAGER_H_

//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>
#include <stdbool.h>
#include <midiPlayer.h>

//==============================================================================
// Constants
//==============================================================================

static const char* const PANGO_BGMS[] = {"Pango_Main.mid",  "Pango_Faster.mid",  "Pango_Speed.mid",
                                         "Pango_Jump Start.mid", "Pango_High Score.mid"};

typedef enum
{
    PA_BGM_NULL,
    PA_BGM_MAIN,
    PA_BGM_FASTER,
    PA_BGM_SPEED,
    PA_BGM_JUMP_START,
    PA_BGM_HIGH_SCORE
} pa_bgmIndex_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
   
    midiFile_t sndDie;
    midiFile_t sndMenuSelect;
    midiFile_t sndMenuConfirm;
    midiFile_t sndMenuDeny;
    midiFile_t sndPause;
    midiFile_t sndBlockStop;
    midiFile_t sndSlide;
    midiFile_t sndBonus;
    midiFile_t sndSquish;
    midiFile_t snd1up;
    midiFile_t bgmLevelClear;
    midiFile_t sndTally;
    midiFile_t bgmGameOver;
    midiFile_t sndSpawn;

    midiFile_t currentBgm;
    uint16_t currentBgmIndex;
} paSoundManager_t;

//==============================================================================
// Functions
//==============================================================================
void pa_initializeSoundManager(paSoundManager_t* self);
void pa_freeSoundManager(paSoundManager_t* self);
bool pa_setBgm(paSoundManager_t* self, uint16_t newBgmIndex);

#endif