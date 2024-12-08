//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include "paSoundManager.h"
#include "soundFuncs.h"

//==============================================================================
// Functions
//==============================================================================
void pa_initializeSoundManager(paSoundManager_t* self)
{
   self->currentBgmIndex = PA_BGM_NULL;

    loadMidiFile("snd1up.mid", &self->snd1up, true);
    // loadMidiFile("sndBreak.mid", &self->sndBreak, true);
    loadMidiFile("sndCheckpoint.mid", &self->sndCheckpoint, true);
    loadMidiFile("sndBlockCombo.mid", &self->sndCoin, true);
    loadMidiFile("sndDie.mid", &self->sndDie, true);
    loadMidiFile("Pango_Game Over.mid", &self->bgmGameOver, true);
    loadMidiFile("sndBlockStop.mid", &self->sndHit, true);
    loadMidiFile("sndSquish.mid", &self->sndHurt, true);
    loadMidiFile("sndJump1.mid", &self->sndJump1, true);
    loadMidiFile("sndJump2.mid", &self->sndJump2, true);
    loadMidiFile("sndJump3.mid", &self->sndJump3, true);
    loadMidiFile("Pango_Level Clear.mid", &self->sndLevelClearA, true);
    loadMidiFile("sndLevelClearB.mid", &self->sndLevelClearB, true);
    loadMidiFile("sndLevelClearC.mid", &self->sndLevelClearC, true);
    loadMidiFile("sndLevelClearD.mid", &self->sndLevelClearD, true);
    loadMidiFile("sndLevelClearS.mid", &self->sndLevelClearS, true);
    loadMidiFile("sndMenuConfirm.mid", &self->sndMenuConfirm, true);
    loadMidiFile("sndMenuDeny.mid", &self->sndMenuDeny, true);
    loadMidiFile("sndMenuSelect.mid", &self->sndMenuSelect, true);
    loadMidiFile("sndOutOfTime.mid", &self->sndOuttaTime, true);
    loadMidiFile("sndPause.mid", &self->sndPause, true);
    loadMidiFile("sndPowerUp.mid", &self->sndPowerUp, true);
    loadMidiFile("sndSlide.mid", &self->sndSquish, true);
    loadMidiFile("sndTally.mid", &self->sndTally, true);
    loadMidiFile("sndWarp.mid", &self->sndWarp, true);
    loadMidiFile("sndWaveBall.mid", &self->sndWaveBall, true);

    loadMidiFile("sndSpawn.mid", &self->sndSpawn, true);
}

void pa_freeSoundManager(paSoundManager_t* self)
{

    // unloadMidiFile(&self->bgmSmooth);
    // unloadMidiFile(&self->bgmUnderground);
    unloadMidiFile(&self->snd1up);
    unloadMidiFile(&self->sndBreak);
    unloadMidiFile(&self->sndCheckpoint);
    unloadMidiFile(&self->sndCoin);
    unloadMidiFile(&self->sndDie);
    unloadMidiFile(&self->bgmGameOver);
    unloadMidiFile(&self->sndHit);
    unloadMidiFile(&self->sndHurt);
    unloadMidiFile(&self->sndJump1);
    unloadMidiFile(&self->sndJump2);
    unloadMidiFile(&self->sndJump3);
    unloadMidiFile(&self->sndLevelClearA);
    unloadMidiFile(&self->sndLevelClearB);
    unloadMidiFile(&self->sndLevelClearC);
    unloadMidiFile(&self->sndLevelClearD);
    unloadMidiFile(&self->sndLevelClearS);
    unloadMidiFile(&self->sndMenuConfirm);
    unloadMidiFile(&self->sndMenuDeny);
    unloadMidiFile(&self->sndMenuSelect);
    unloadMidiFile(&self->sndOuttaTime);
    unloadMidiFile(&self->sndPause);
    unloadMidiFile(&self->sndPowerUp);
    unloadMidiFile(&self->sndSquish);
    unloadMidiFile(&self->sndTally);
    unloadMidiFile(&self->sndWarp);
    unloadMidiFile(&self->sndWaveBall);

    unloadMidiFile(&self->sndSpawn);

    if (self->currentBgmIndex != PA_BGM_NULL)
    {
        unloadMidiFile(&self->currentBgm);
    }
}

/*
    Loads the indexed BGM into memory.
    Returns true if the BGM was actually changed, otherwise false
*/
bool pa_setBgm(paSoundManager_t* self, uint16_t newBgmIndex){
    if (self->currentBgmIndex == newBgmIndex)
    {
        return false;
    }

    if (self->currentBgmIndex != PA_BGM_NULL)
    {
        soundStop(true);
        unloadMidiFile(&self->currentBgm);
    }

    if (newBgmIndex != PA_BGM_NULL)
    {
        loadMidiFile(PANGO_BGMS[newBgmIndex - 1], &self->currentBgm, true);

        midiPlayer_t* player = globalMidiPlayerGet(MIDI_BGM);
        player->loop         = true;
    }

    self->currentBgmIndex = newBgmIndex;
    return true;
}