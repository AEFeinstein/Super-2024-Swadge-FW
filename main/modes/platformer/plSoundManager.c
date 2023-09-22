//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include "plSoundManager.h"

//==============================================================================
// Functions
//==============================================================================
void initializeSoundManager(soundManager_t *self){
    loadSong("bgmCastle.sng", &self->bgmCastle, false);
    loadSong("bgmDeMAGio.sng", &self->bgmDemagio, false);
    loadSong("bgmGameStart.sng", &self->bgmGameStart, false);
    loadSong("bgmIntro.sng", &self->bgmIntro, false); 
    loadSong("bgmNameEntry.sng", &self->bgmNameEntry, false); 
    loadSong("bgmSmooth.sng", &self->bgmSmooth, false);
    loadSong("bgmUnderground.sng", &self->bgmUnderground, false); 
    loadSong("snd1up.sng", &self->snd1up, false);
    loadSong("sndBreak.sng", &self->sndBreak, false);
    loadSong("sndCheckpoint.sng", &self->sndCheckpoint, false);
    loadSong("sndCoin.sng", &self->sndCoin, false); 
    loadSong("sndDie.sng", &self->sndDie, false); 
    loadSong("bgmGameOver.sng", &self->bgmGameOver, false);
    loadSong("sndHit.sng", &self->sndHit, false);
    loadSong("sndHurt.sng", &self->sndHurt, false);
    loadSong("sndJump1.sng", &self->sndJump1, false);
    loadSong("sndJump2.sng", &self->sndJump2, false);
    loadSong("sndLevelClearA.sng", &self->sndLevelClearA, false);
    loadSong("sndLevelClearB.sng", &self->sndLevelClearB, false);
    loadSong("sndLevelClearC.sng", &self->sndLevelClearC, false);
    loadSong("sndLevelClearD.sng", &self->sndLevelClearD, false);
    loadSong("sndLevelClearS.sng", &self->sndLevelClearS, false);
    loadSong("sndMenuConfirm.sng", &self->sndMenuConfirm, false);
    loadSong("sndMenuDeny.sng", &self->sndMenuDeny, false);
    loadSong("sndMenuSelect.sng", &self->sndMenuSelect, false);
    loadSong("sndOuttatime.sng", &self->sndOuttaTime, false);
    loadSong("sndPause.sng", &self->sndPause, false);
    loadSong("sndPowerUp.sng", &self->sndPowerUp, false);
    loadSong("sndSquish.sng", &self->sndSquish, false);
    loadSong("sndTally.sng", &self->sndTally, false);
    loadSong("sndWarp.sng", &self->sndWarp, false);
    loadSong("sndWaveBall.sng", &self->sndWaveBall, false);
}

void freeSoundManager(soundManager_t *self){
    freeSong(&self->bgmCastle);
    freeSong(&self->bgmDemagio);
    freeSong(&self->bgmGameStart);
    freeSong(&self->bgmIntro); 
    freeSong(&self->bgmNameEntry); 
    freeSong(&self->bgmSmooth);
    freeSong(&self->bgmUnderground); 
    freeSong(&self->snd1up);
    freeSong(&self->sndBreak);
    freeSong(&self->sndCheckpoint);
    freeSong(&self->sndCoin); 
    freeSong(&self->sndDie); 
    freeSong(&self->bgmGameOver);
    freeSong(&self->sndHit);
    freeSong(&self->sndHurt);
    freeSong(&self->sndJump1);
    freeSong(&self->sndJump2);
    freeSong(&self->sndLevelClearA);
    freeSong(&self->sndLevelClearB);
    freeSong(&self->sndLevelClearC);
    freeSong(&self->sndLevelClearD);
    freeSong(&self->sndLevelClearS);
    freeSong(&self->sndMenuConfirm);
    freeSong(&self->sndMenuDeny);
    freeSong(&self->sndMenuSelect);
    freeSong(&self->sndOuttaTime);
    freeSong(&self->sndPause);
    freeSong(&self->sndPowerUp);
    freeSong(&self->sndSquish);
    freeSong(&self->sndTally);
    freeSong(&self->sndWarp);
    freeSong(&self->sndWaveBall);
}