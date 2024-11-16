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
    // loadMidiFile("bgmCastle.mid", &self->bgmCastle, true);
    //  self->bgmCastle.shouldLoop = true;

    loadMidiFile("Pango_Main.mid", &self->bgmMain, true);
    loadMidiFile("Pango_Faster.mid", &self->bgmFast, true);

    // loadMidiFile("bgmDeMAGio.mid", &self->bgmDemagio, true);
    //  self->bgmDemagio.shouldLoop = true;

    loadMidiFile("bgmGameStart.mid", &self->bgmGameStart, true);
    loadMidiFile("bgmIntro.mid", &self->bgmIntro, true);
    loadMidiFile("Pango_High Score.mid", &self->bgmNameEntry, true);
    // self->bgmNameEntry.shouldLoop = true;

    // loadMidiFile("bgmSmooth.mid", &self->bgmSmooth, true);
    //  self->bgmSmooth.shouldLoop = true;

    // loadMidiFile("bgmUnderground.mid", &self->bgmUnderground, true);
    //  self->bgmUnderground.shouldLoop = true;

    loadMidiFile("snd1up.mid", &self->snd1up, true);
    // loadMidiFile("sndBreak.mid", &self->sndBreak, true);
    loadMidiFile("sndCheckpoint.mid", &self->sndCheckpoint, true);
    loadMidiFile("sndBlockCombo.mid", &self->sndCoin, true);
    loadMidiFile("sndDie.mid", &self->sndDie, true);
    loadMidiFile("bgmGameOver.mid", &self->bgmGameOver, true);
    loadMidiFile("sndBlockStop.mid", &self->sndHit, true);
    loadMidiFile("sndSquish.mid", &self->sndHurt, true);
    loadMidiFile("sndJump1.mid", &self->sndJump1, true);
    loadMidiFile("sndJump2.mid", &self->sndJump2, true);
    loadMidiFile("sndJump3.mid", &self->sndJump3, true);
    loadMidiFile("sndLevelClearA.mid", &self->sndLevelClearA, true);
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
    unloadMidiFile(&self->bgmMain);
    unloadMidiFile(&self->bgmFast);
    unloadMidiFile(&self->bgmGameStart);
    unloadMidiFile(&self->bgmIntro);
    unloadMidiFile(&self->bgmNameEntry);
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
}