#ifndef _MG_SOUNDMANAGER_H_
#define _MG_SOUNDMANAGER_H_

//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>
#include <stdbool.h>
#include <midiPlayer.h>

//==============================================================================
// Constants
//==============================================================================

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    midiFile_t currentBgm;
    uint16_t currentBgmIndex;

    midiFile_t bgmIntro;
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
} mgSoundManager_t;

//==============================================================================
// Functions
//==============================================================================
void mg_initializeSoundManager(mgSoundManager_t* self);
void mg_freeSoundManager(mgSoundManager_t* self);
bool mg_setBgm(mgSoundManager_t* self, uint16_t newBgmIndex);

#endif