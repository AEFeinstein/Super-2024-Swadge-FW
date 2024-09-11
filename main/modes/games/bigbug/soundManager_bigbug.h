#ifndef _SOUNDMANAGER_BIGBUG_H_
#define _SOUNDMANAGER_BIGBUG_H_

//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>
#include <stdbool.h>

#include "midiFileParser.h"

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
    midiFile_t hit1;
    midiFile_t hit2;
    midiFile_t hit3;
    midiFile_t launch;
    midiFile_t die;
    midiFile_t tally;
    midiFile_t dropBomb;
    midiFile_t detonate;
    midiFile_t snd1up;
    midiFile_t getReady;
    midiFile_t levelClear;
    midiFile_t gameOver;
    midiFile_t levelBgm;
    uint16_t currentBgmIndex;
} bb_soundManager_t;

//==============================================================================
// Functions
//==============================================================================
void bb_initializeSoundManager(bb_soundManager_t* self);
void bb_freeSoundManager(bb_soundManager_t* self);
void bb_setLevelBgm(bb_soundManager_t* self, uint16_t newBgmIndex);

#endif