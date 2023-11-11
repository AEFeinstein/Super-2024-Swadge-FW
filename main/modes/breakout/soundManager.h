#ifndef _SOUNDMANAGER_H_
#define _SOUNDMANAGER_H_

//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>
#include <stdbool.h>
#include <spiffs_song.h>

//==============================================================================
// Constants
//==============================================================================

static const char* const LEVEL_BGMS[] = {
    "brkBgmSkill.sng", "brkBgmPixel.sng", "brkBgmCrazy.sng", "brkBgmCrazy.sng", "brkHighScore.sng","brkBgmTitle.sng"
};

typedef enum {
    BRK_BGM_NULL,
    BRK_BGM_SKILL,
    BRK_BGM_PIXEL,
    BRK_BGM_CRAZY,
    BRK_BGM_4,
    BRK_BGM_NAME_ENTRY,
    BRK_BGM_TITLE
} breakoutBgmIndex_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    song_t hit1;
    song_t hit2;
    song_t hit3;
    song_t launch;
    song_t die;
    song_t tally;
    song_t dropBomb;
    song_t detonate;
    song_t snd1up;
    song_t getReady;
    song_t levelClear;
    song_t gameOver;
    song_t levelBgm;
    uint16_t currentBgmIndex;
} soundManager_t;

//==============================================================================
// Functions
//==============================================================================
void initializeSoundManager(soundManager_t* self);
void freeSoundManager(soundManager_t* self);
void setLevelBgm(soundManager_t* self, uint16_t newBgmIndex);

#endif