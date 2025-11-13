#ifndef _PL_SOUNDMANAGER_H_
#define _PL_SOUNDMANAGER_H_

//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>
#include <stdbool.h>
#include <spiffs_song.h>

//==============================================================================
// Constants
//==============================================================================

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    song_t bgmDemagio;
    song_t bgmIntro;
    song_t bgmSmooth;
    song_t bgmUnderground;
    song_t bgmCastle;
    song_t bgmGameStart;
    song_t sndDie;
    song_t sndMenuSelect;
    song_t sndMenuConfirm;
    song_t sndMenuDeny;
    song_t sndPause;
    song_t sndHit;
    song_t sndSquish;
    song_t sndBreak;
    song_t sndCoin;
    song_t sndPowerUp;
    song_t sndJump1;
    song_t sndJump2;
    song_t sndJump3;
    song_t sndWarp;
    song_t sndHurt;
    song_t sndWaveBall;
    song_t snd1up;
    song_t sndCheckpoint;
    song_t sndLevelClearD;
    song_t sndLevelClearC;
    song_t sndLevelClearB;
    song_t sndLevelClearA;
    song_t sndLevelClearS;
    song_t sndTally;
    song_t bgmNameEntry;
    song_t bgmGameOver;
    song_t sndOuttaTime;
} plSoundManager_t;

//==============================================================================
// Functions
//==============================================================================
void pl_initializeSoundManager(plSoundManager_t* self);
void pl_freeSoundManager(plSoundManager_t* self);

#endif