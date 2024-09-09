#ifndef _SOUNDMANAGER_BIGBUG_H_
#define _SOUNDMANAGER_BIGBUG_H_

//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>
#include <stdbool.h>
#include <spiffs_song.h>

//==============================================================================
// Constants
//==============================================================================

static const char* const BB_LEVEL_BGMS[] = {"brkBgmSkill.sng",  "brkBgmPixel.sng",  "brkBgmCrazy.sng",
                                            "brkBgmFinale.sng", "brkHighScore.sng", "brkBgmTitle.sng"};

typedef enum
{
    BB_BGM_NULL,
    BB_BGM_SKILL,
    BB_BGM_PIXEL,
    BB_BGM_CRAZY,
    BB_BGM_FINALE,
    BB_BGM_NAME_ENTRY,
    BB_BGM_TITLE
} bb_BgmIndex_t;

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
} bb_soundManager_t;

//==============================================================================
// Functions
//==============================================================================
void bb_initializeSoundManager(bb_soundManager_t* self);
void bb_freeSoundManager(bb_soundManager_t* self);
void bb_setLevelBgm(bb_soundManager_t* self, uint16_t newBgmIndex);

#endif