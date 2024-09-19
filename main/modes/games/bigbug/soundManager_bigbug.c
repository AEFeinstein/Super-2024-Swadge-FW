//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include "soundManager_bigbug.h"
#include "soundFuncs.h"

//==============================================================================
// Functions
//==============================================================================
void bb_initializeSoundManager(bb_soundManager_t* self)
{
    loadMidiFile("sndBreak2.sng", &self->hit1, false);
    loadMidiFile("sndBreak3.sng", &self->hit2, false);
    loadMidiFile("sndBounce.sng", &self->hit3, false);
    loadMidiFile("sndWaveBall.sng", &self->launch, false);
    loadMidiFile("sndBrkDie.sng", &self->die, false);
    loadMidiFile("sndTally.sng", &self->tally, false);
    loadMidiFile("sndDropBomb.sng", &self->dropBomb, false);
    loadMidiFile("sndDetonate.sng", &self->detonate, false);
    loadMidiFile("sndBrk1up.sng", &self->snd1up, false);
    loadMidiFile("brkGetReady.sng", &self->getReady, false);
    loadMidiFile("brkLvlClear.sng", &self->levelClear, false);
    loadMidiFile("brkGameOver.sng", &self->gameOver, false);
    self->currentBgmIndex = BB_BGM_NULL;
}

void bb_freeSoundManager(bb_soundManager_t* self)
{
    unloadMidiFile(&self->hit1);
    unloadMidiFile(&self->hit2);
    unloadMidiFile(&self->hit3);
    unloadMidiFile(&self->launch);
    unloadMidiFile(&self->die);
    unloadMidiFile(&self->tally);
    unloadMidiFile(&self->dropBomb);
    unloadMidiFile(&self->detonate);
    unloadMidiFile(&self->snd1up);
    unloadMidiFile(&self->levelClear);
    unloadMidiFile(&self->gameOver);
    unloadMidiFile(&self->getReady);

    if (self->currentBgmIndex != BB_BGM_NULL)
    {
        unloadMidiFile(&self->levelBgm);
    }
}

void bb_setLevelBgm(bb_soundManager_t* self, uint16_t newBgmIndex)
{
    if (self->currentBgmIndex == newBgmIndex)
    {
        return;
    }

    if (self->currentBgmIndex != BB_BGM_NULL)
    {
        soundStop(true);
        unloadMidiFile(&self->levelBgm);
    }

    if (newBgmIndex != BB_BGM_NULL)
    {
        loadMidiFile(BB_LEVEL_BGMS[newBgmIndex - 1], &self->levelBgm, false);
        // TODO make sure BGM loops
        // self->levelBgm.shouldLoop = true;
    }

    self->currentBgmIndex = newBgmIndex;
}