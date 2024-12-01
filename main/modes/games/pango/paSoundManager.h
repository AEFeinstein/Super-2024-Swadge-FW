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
    midiFile_t sndHit;
    midiFile_t sndSquish;
    midiFile_t sndBreak;
    midiFile_t sndCoin;
    midiFile_t sndPowerUp;
    midiFile_t sndJump1;
    midiFile_t sndJump2;
    midiFile_t sndJump3;
    midiFile_t sndWarp;
    midiFile_t sndHurt;
    midiFile_t sndWaveBall;
    midiFile_t snd1up;
    midiFile_t sndCheckpoint;
    midiFile_t sndLevelClearD;
    midiFile_t sndLevelClearC;
    midiFile_t sndLevelClearB;
    midiFile_t sndLevelClearA;
    midiFile_t sndLevelClearS;
    midiFile_t sndTally;
    midiFile_t bgmGameOver;
    midiFile_t sndOuttaTime;

    midiFile_t sndSpawn;

    midiFile_t currentBgm;
    uint16_t currentBgmIndex;
} paSoundManager_t;

//==============================================================================
// Functions
//==============================================================================
void pa_initializeSoundManager(paSoundManager_t* self);
void pa_freeSoundManager(paSoundManager_t* self);
void pa_setBgm(paSoundManager_t* self, uint16_t newBgmIndex);

#endif