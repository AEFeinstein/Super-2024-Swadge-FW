//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include "plSoundManager.h"
#include "soundFuncs.h"

//==============================================================================
// Functions
//==============================================================================
void pl_initializeSoundManager(plSoundManager_t* self)
{
    loadMidiFile("bgmCastle.sng", &self->bgmCastle, false);

    loadMidiFile("bgmDeMAGio.sng", &self->bgmDemagio, false);

    loadMidiFile("bgmGameStart.sng", &self->bgmGameStart, false);
    loadMidiFile("bgmIntro.sng", &self->bgmIntro, false);
    loadMidiFile("bgmNameEntry.sng", &self->bgmNameEntry, false);

    loadMidiFile("bgmSmooth.sng", &self->bgmSmooth, false);

    loadMidiFile("bgmUnderground.sng", &self->bgmUnderground, false);

    loadMidiFile("snd1up.sng", &self->snd1up, false);
    loadMidiFile("sndBreak.sng", &self->sndBreak, false);
    loadMidiFile("sndCheckpoint.sng", &self->sndCheckpoint, false);
    loadMidiFile("sndCoin.sng", &self->sndCoin, false);
    loadMidiFile("sndDie.sng", &self->sndDie, false);
    loadMidiFile("bgmGameOver.sng", &self->bgmGameOver, false);
    loadMidiFile("sndHit.sng", &self->sndHit, false);
    loadMidiFile("sndHurt.sng", &self->sndHurt, false);
    loadMidiFile("sndJump1.sng", &self->sndJump1, false);
    loadMidiFile("sndJump2.sng", &self->sndJump2, false);
    loadMidiFile("sndJump3.sng", &self->sndJump3, false);
    loadMidiFile("sndLevelClearA.sng", &self->sndLevelClearA, false);
    loadMidiFile("sndLevelClearB.sng", &self->sndLevelClearB, false);
    loadMidiFile("sndLevelClearC.sng", &self->sndLevelClearC, false);
    loadMidiFile("sndLevelClearD.sng", &self->sndLevelClearD, false);
    loadMidiFile("sndLevelClearS.sng", &self->sndLevelClearS, false);
    loadMidiFile("sndMenuConfirm.sng", &self->sndMenuConfirm, false);
    loadMidiFile("sndMenuDeny.sng", &self->sndMenuDeny, false);
    loadMidiFile("sndMenuSelect.sng", &self->sndMenuSelect, false);
    loadMidiFile("sndOutOfTime.sng", &self->sndOuttaTime, false);
    loadMidiFile("sndPause.sng", &self->sndPause, false);
    loadMidiFile("sndPowerUp.sng", &self->sndPowerUp, false);
    loadMidiFile("sndSquish.sng", &self->sndSquish, false);
    loadMidiFile("sndTally.sng", &self->sndTally, false);
    loadMidiFile("sndWarp.sng", &self->sndWarp, false);
    loadMidiFile("sndWaveBall.sng", &self->sndWaveBall, false);
}

void pl_freeSoundManager(plSoundManager_t* self)
{
    unloadMidiFile(&self->bgmCastle);
    unloadMidiFile(&self->bgmDemagio);
    unloadMidiFile(&self->bgmGameStart);
    unloadMidiFile(&self->bgmIntro);
    unloadMidiFile(&self->bgmNameEntry);
    unloadMidiFile(&self->bgmSmooth);
    unloadMidiFile(&self->bgmUnderground);
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