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

} soundManager_t;

//==============================================================================
// Functions
//==============================================================================
void initializeSoundManager(soundManager_t *self);
void freeSoundManager(soundManager_t *self);

#endif