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
    loadMidiFile("sndBlockCombo.mid", &self->sndBonus, true);
    loadMidiFile("sndDie.mid", &self->sndDie, true);
    loadMidiFile("Pango_Game Over.mid", &self->bgmGameOver, true);
    loadMidiFile("sndBlockStop.mid", &self->sndBlockStop, true);
    loadMidiFile("sndSquish.mid", &self->sndSquish, true);
    loadMidiFile("Pango_Level Clear.mid", &self->bgmLevelClear, true);
    loadMidiFile("sndMenuConfirm.mid", &self->sndMenuConfirm, true);
    loadMidiFile("sndMenuDeny.mid", &self->sndMenuDeny, true);
    loadMidiFile("sndMenuSelect.mid", &self->sndMenuSelect, true);
    loadMidiFile("sndPause.mid", &self->sndPause, true);
    loadMidiFile("sndSlide.mid", &self->sndSlide, true);
    loadMidiFile("sndTally.mid", &self->sndTally, true);
    loadMidiFile("sndSpawn.mid", &self->sndSpawn, true);
}

void pa_freeSoundManager(paSoundManager_t* self)
{
    unloadMidiFile(&self->snd1up);
    unloadMidiFile(&self->sndBonus);
    unloadMidiFile(&self->sndDie);
    unloadMidiFile(&self->bgmGameOver);
    unloadMidiFile(&self->sndBlockStop);
    unloadMidiFile(&self->sndSquish);
    unloadMidiFile(&self->sndMenuConfirm);
    unloadMidiFile(&self->sndMenuDeny);
    unloadMidiFile(&self->sndMenuSelect);
    unloadMidiFile(&self->sndPause);
    unloadMidiFile(&self->sndSquish);
    unloadMidiFile(&self->sndTally);
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
bool pa_setBgm(paSoundManager_t* self, uint16_t newBgmIndex)
{
    // All BGM's are intended to loop!
    midiPlayer_t* player = globalMidiPlayerGet(MIDI_BGM);
    player->loop         = true;
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
    }

    self->currentBgmIndex = newBgmIndex;
    return true;
}