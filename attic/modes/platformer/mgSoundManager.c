//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include "mgSoundManager.h"
#include "soundFuncs.h"
#include "mega_pulse_ex_typedef.h"

//==============================================================================
// Functions
//==============================================================================
void mg_initializeSoundManager(mgSoundManager_t* self)
{
    self->currentBgmIndex = MG_BGM_NULL;
    loadMidiFile(BGM_INTRO_MID, &self->bgmIntro, true);
    loadMidiFile(SND_1UP_MID, &self->snd1up, true);
    loadMidiFile(SND_BREAK_MID, &self->sndBreak, true);
    loadMidiFile(SND_CHECKPOINT_MID, &self->sndCheckpoint, true);
    loadMidiFile(SND_COIN_MID, &self->sndCoin, true);
    loadMidiFile(SND_DIE_MID, &self->sndDie, true);
    loadMidiFile(BGM_GAME_OVER_MID, &self->bgmGameOver, true);
    loadMidiFile(SND_HIT_MID, &self->sndHit, true);
    loadMidiFile(SND_HURT_MID, &self->sndHurt, true);
    loadMidiFile(SND_JUMP_1_MID, &self->sndJump1, true);
    loadMidiFile(SND_JUMP_2_MID, &self->sndJump2, true);
    loadMidiFile(SND_JUMP_3_MID, &self->sndJump3, true);
    loadMidiFile(SND_LEVEL_CLEAR_A_MID, &self->sndLevelClearA, true);
    loadMidiFile(SND_LEVEL_CLEAR_B_MID, &self->sndLevelClearB, true);
    loadMidiFile(SND_LEVEL_CLEAR_C_MID, &self->sndLevelClearC, true);
    loadMidiFile(SND_LEVEL_CLEAR_D_MID, &self->sndLevelClearD, true);
    loadMidiFile(SND_LEVEL_CLEAR_MID, &self->sndLevelClearS, true);
    loadMidiFile(SWSN_CHOOSE_SFX_MID, &self->sndMenuConfirm, true);
    loadMidiFile(SND_MENU_DENY_MID, &self->sndMenuDeny, true);
    loadMidiFile(SWSN_MOVE_SFX_MID, &self->sndMenuSelect, true);
    loadMidiFile(SND_OUT_OF_TIME_MID, &self->sndOuttaTime, true);
    loadMidiFile(SND_PAUSE_MID, &self->sndPause, true);
    loadMidiFile(SND_POWER_UP_MID, &self->sndPowerUp, true);
    loadMidiFile(SND_SQUISH_MID, &self->sndSquish, true);
    loadMidiFile(SND_TALLY_MID, &self->sndTally, true);
    loadMidiFile(SND_WARP_MID, &self->sndWarp, true);
    loadMidiFile(SND_WAVE_BALL_MID, &self->sndWaveBall, true);
}

void mg_freeSoundManager(mgSoundManager_t* self)
{
    if (self->currentBgmIndex != MG_BGM_NULL)
    {
        unloadMidiFile(&self->currentBgm);
    }

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
}

/*
    Loads the indexed BGM_ into memory.
    Returns true if the BGM_ was actually changed, otherwise false
*/
bool mg_setBgm(mgSoundManager_t* self, uint16_t newBgmIndex)
{
    // All BGM_'s are intended to loop!
    midiPlayer_t* player = globalMidiPlayerGet(MIDI_BGM);
    player->loop         = true;
    if (self->currentBgmIndex == newBgmIndex)
    {
        return false;
    }

    if (self->currentBgmIndex != MG_BGM_NULL)
    {
        player->paused = true;
        unloadMidiFile(&self->currentBgm);
    }

    if (newBgmIndex != MG_BGM_NULL)
    {
        loadMidiFile(MG_BGMS[newBgmIndex - 1], &self->currentBgm, true);
    }
    self->currentBgmIndex = newBgmIndex;
    midiPlayerResetNewSong(globalMidiPlayerGet(MIDI_BGM));
    return true;
}