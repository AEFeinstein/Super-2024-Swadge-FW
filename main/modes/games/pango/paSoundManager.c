//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include "paSoundManager.h"

//==============================================================================
// Functions
//==============================================================================
void pa_initializeSoundManager(paSoundManager_t* self)
{
    loadMidiFile("bgmCastle.mid", &self->bgmCastle, false);
    //self->bgmCastle.shouldLoop = true;

    loadMidiFile("bgmDeMAGio.mid", &self->bgmDemagio, false);
    //self->bgmDemagio.shouldLoop = true;

    loadMidiFile("bgmGameStart.mid", &self->bgmGameStart, false);
    loadMidiFile("bgmIntro.mid", &self->bgmIntro, false);
    loadMidiFile("bgmNameEntry.mid", &self->bgmNameEntry, false);
    //self->bgmNameEntry.shouldLoop = true;

    loadMidiFile("bgmSmooth.mid", &self->bgmSmooth, false);
    //self->bgmSmooth.shouldLoop = true;

    loadMidiFile("bgmUnderground.mid", &self->bgmUnderground, false);
    //self->bgmUnderground.shouldLoop = true;

    loadMidiFile("snd1up.mid", &self->snd1up, false);
    //loadMidiFile("sndBreak.mid", &self->sndBreak, false);
    loadMidiFile("sndCheckpoint.mid", &self->sndCheckpoint, false);
    loadMidiFile("sndCoin.mid", &self->sndCoin, false);
    loadMidiFile("sndDie.mid", &self->sndDie, false);
    loadMidiFile("bgmGameOver.mid", &self->bgmGameOver, false);
    loadMidiFile("sndBlockStop.mid", &self->sndHit, false);
    loadMidiFile("sndSquish.mid", &self->sndHurt, false);
    loadMidiFile("sndJump1.mid", &self->sndJump1, false);
    loadMidiFile("sndJump2.mid", &self->sndJump2, false);
    loadMidiFile("sndJump3.mid", &self->sndJump3, false);
    loadMidiFile("sndLevelClearA.mid", &self->sndLevelClearA, false);
    loadMidiFile("sndLevelClearB.mid", &self->sndLevelClearB, false);
    loadMidiFile("sndLevelClearC.mid", &self->sndLevelClearC, false);
    loadMidiFile("sndLevelClearD.mid", &self->sndLevelClearD, false);
    loadMidiFile("sndLevelClearS.mid", &self->sndLevelClearS, false);
    loadMidiFile("sndMenuConfirm.mid", &self->sndMenuConfirm, false);
    loadMidiFile("sndMenuDeny.mid", &self->sndMenuDeny, false);
    loadMidiFile("sndMenuSelect.mid", &self->sndMenuSelect, false);
    loadMidiFile("sndOutOfTime.mid", &self->sndOuttaTime, false);
    loadMidiFile("sndPause.mid", &self->sndPause, false);
    loadMidiFile("sndPowerUp.mid", &self->sndPowerUp, false);
    loadMidiFile("sndSlide.mid", &self->sndSquish, false);
    //loadMidiFile("sndTally.mid", &self->sndTally, false);
    loadMidiFile("sndWarp.mid", &self->sndWarp, false);
    loadMidiFile("sndWaveBall.mid", &self->sndWaveBall, false);

    loadMidiFile("sndSpawn.mid", &self->sndSpawn, false);
}

void pa_freeSoundManager(paSoundManager_t* self)
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

    unloadMidiFile(&self->sndSpawn);
}