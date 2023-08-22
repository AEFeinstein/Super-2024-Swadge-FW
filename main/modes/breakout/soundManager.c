//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include "soundManager.h"

//==============================================================================
// Functions
//==============================================================================
void initializeSoundManager(soundManager_t *self){
    loadSong("sndBreak2.sng", &self->hit1, false);
    loadSong("sndBreak3.sng", &self->hit2, false);
    loadSong("sndBounce.sng", &self->hit3, false);
    loadSong("sndWaveBall.sng", &self->launch, false);
    loadSong("sndBrkDie.sng", &self->die, false);
    loadSong("sndTally.sng", &self->tally, false);
}

void freeSoundManager(soundManager_t *self){
    freeSong(&self->hit1);
    freeSong(&self->hit2);
    freeSong(&self->hit3);
    freeSong(&self->launch);
    freeSong(&self->die);
    freeSong(&self->tally);
}