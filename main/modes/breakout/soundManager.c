//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include "soundManager.h"

//==============================================================================
// Functions
//==============================================================================
void initializeSoundManager(soundManager_t* self)
{
    loadSong("sndBreak2.sng", &self->hit1, false);
    loadSong("sndBreak3.sng", &self->hit2, false);
    loadSong("sndBounce.sng", &self->hit3, false);
    loadSong("sndWaveBall.sng", &self->launch, false);
    loadSong("sndBrkDie.sng", &self->die, false);
    loadSong("sndTally.sng", &self->tally, false);
    loadSong("sndDropBomb.sng", &self->dropBomb, false);
    loadSong("sndDetonate.sng", &self->detonate, false);
    loadSong("sndBrk1up.sng", &self->snd1up, false);
    loadSong("brkGetReady.sng", &self->getReady, false);
    loadSong("brkLvlClear.sng", &self->levelClear, false);
    loadSong("brkGameOver.sng", &self->gameOver, false);
    self->currentBgmIndex = BRK_BGM_NULL;
}

void freeSoundManager(soundManager_t* self)
{
    freeSong(&self->hit1);
    freeSong(&self->hit2);
    freeSong(&self->hit3);
    freeSong(&self->launch);
    freeSong(&self->die);
    freeSong(&self->tally);
    freeSong(&self->dropBomb);
    freeSong(&self->detonate);
    freeSong(&self->snd1up);
    freeSong(&self->levelClear);
    freeSong(&self->gameOver);
    freeSong(&self->getReady);

    if (self->currentBgmIndex != BRK_BGM_NULL)
    {
        freeSong(&self->levelBgm);
    }
}

void setLevelBgm(soundManager_t* self, uint16_t newBgmIndex)
{
    if (self->currentBgmIndex == newBgmIndex)
    {
        return;
    }

    if (self->currentBgmIndex != BRK_BGM_NULL)
    {
        bzrStop(true);
        freeSong(&self->levelBgm);
    }

    if (newBgmIndex != BRK_BGM_NULL)
    {
        loadSong(LEVEL_BGMS[newBgmIndex - 1], &self->levelBgm, false);
        self->levelBgm.shouldLoop = true;
    }

    self->currentBgmIndex = newBgmIndex;
}