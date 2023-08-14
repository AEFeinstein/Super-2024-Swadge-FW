//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include "soundManager.h"

//==============================================================================
// Functions
//==============================================================================
void initializeSoundManager(soundManager_t *self){
    loadSong("block1.sng", &self->hit1, false);
    loadSong("block2.sng", &self->hit2, false);
}

void freeSoundManager(soundManager_t *self){
    freeSong(&self->hit1);
    freeSong(&self->hit2);
}