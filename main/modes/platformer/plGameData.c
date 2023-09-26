//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include "plGameData.h"
#include "plEntityManager.h"
#include "esp_random.h"
#include "hdw-btn.h"
#include "hdw-bzr.h"

//==============================================================================
// Functions
//==============================================================================
 void pl_initializeGameData(plGameData_t * gameData, plSoundManager_t * soundManager){
    gameData->gameState = 0;
    gameData->btnState = 0;
    gameData->score = 0;
    gameData->lives = 3;
    gameData->countdown = 000;
    gameData->world = 1;
    gameData->level = 1;
    gameData->frameCount = 0;
    gameData->coins = 0;
    gameData->combo = 0;
    gameData->comboTimer = 0;
    gameData->bgColor = c335;
    gameData->initials[0] = 'A';
    gameData->initials[1] = 'A';
    gameData->initials[2] = 'A';
    gameData->rank = 5;
    gameData->extraLifeCollected = false;
    gameData->checkpoint = 0;
    gameData->levelDeaths = 0;
    gameData->initialHp = 1;
    gameData->debugMode = false;
    gameData->continuesUsed = false;
    gameData->inGameTimer = 0;
    gameData->soundManager = soundManager;
}

 void pl_initializeGameDataFromTitleScreen(plGameData_t * gameData){
    gameData->gameState = 0;
    gameData->btnState = 0;
    gameData->score = 0;
    gameData->lives = 3;
    gameData->countdown = 000;
    gameData->frameCount = 0;
    gameData->coins = 0;
    gameData->combo = 0;
    gameData->comboTimer = 0;
    gameData->bgColor = c000;
    gameData->extraLifeCollected = false;
    gameData->checkpoint = 0;
    gameData->levelDeaths = 0;
    gameData->currentBgm = 0;
    gameData->changeBgm = 0;
    gameData->initialHp = 1;
    gameData->continuesUsed = (gameData->world == 1 && gameData->level == 1) ? false : true;
    gameData->inGameTimer = 0;

    pl_resetGameDataLeds(gameData);
}

void pl_updateLedsHpMeter(plEntityManager_t *entityManager, plGameData_t *gameData){
    if(entityManager->playerEntity == NULL){
        return;
    }

    uint8_t hp = entityManager->playerEntity->hp;
    if(hp > 3){
        hp = 3;
    }

    //HP meter led pairs:
    //3 4
    //2 5
    //1 6
    for (int32_t i = 1; i < 7; i++)
    {
        gameData->leds[i].r = 0x80;
        gameData->leds[i].g = 0x00;
        gameData->leds[i].b = 0x00;
    }

    for (int32_t i = 1; i < 1+hp; i++)
    {
        gameData->leds[i].r = 0x00;
        gameData->leds[i].g = 0x80;

        gameData->leds[7-i].r = 0x00;
        gameData->leds[7-i].g = 0x80;
    }

    setLeds(gameData->leds, CONFIG_NUM_LEDS);
}

void pl_scorePoints(plGameData_t * gameData, uint16_t points){
    gameData->combo++;
    
    uint32_t comboPoints = points * gameData->combo;

    gameData->score += comboPoints;
    gameData->comboScore = comboPoints;
    
    gameData->comboTimer = (gameData->levelDeaths < 3) ? 240: 1;
}

void addCoins(plGameData_t * gameData, uint8_t coins){
    gameData->coins+=coins;
    if(gameData->coins > 99){
        gameData->lives++;
        bzrPlaySfx(&(gameData->soundManager->snd1up), BZR_LEFT);
        gameData->coins = 0;
    } else {
        bzrPlaySfx(&(gameData->soundManager->sndCoin), BZR_LEFT);
    }
}

void updateComboTimer(plGameData_t * gameData){
    gameData->comboTimer--;

    if(gameData->comboTimer < 0){
        gameData->comboTimer = 0;
        gameData->combo = 0;
    }
};

void pl_resetGameDataLeds(plGameData_t * gameData)
{
    for(uint8_t i=0;i<CONFIG_NUM_LEDS; i++){
        gameData->leds[i].r = 0;
        gameData->leds[i].g = 0;
        gameData->leds[i].b = 0;
    }

    setLeds(gameData->leds, CONFIG_NUM_LEDS);
}

void pl_updateLedsShowHighScores(plGameData_t * gameData){
    if(( (gameData->frameCount) % 10) == 0){
        for (int32_t i = 0; i < 8; i++)
        {
        
            if(( (gameData->frameCount >> 4) % CONFIG_NUM_LEDS) == i) {
                gameData->leds[i].r =  0xF0;
                gameData->leds[i].g = 0xF0;
                gameData->leds[i].b = 0x00;
            }

            if(gameData->leds[i].r > 0){
                gameData->leds[i].r -= 0x05;
            }
            
            if(gameData->leds[i].g > 0){
                gameData->leds[i].g -= 0x10;
            }

            if(gameData->leds[i].b > 0){
                gameData->leds[i].b = 0x00;
            }
            
        }
    }
    setLeds(gameData->leds, CONFIG_NUM_LEDS);
}

void pl_updateLedsGameOver(plGameData_t * gameData){
    if(( (gameData->frameCount) % 10) == 0){
        for (int32_t i = 0; i < CONFIG_NUM_LEDS; i++)
        {
        
            if(( (gameData->frameCount >> 4) % CONFIG_NUM_LEDS) == i) {
                gameData->leds[i].r =  0xF0;
                gameData->leds[i].g = 0x00;
                gameData->leds[i].b = 0x00;
            }

            gameData->leds[i].r -= 0x10;
            gameData->leds[i].g = 0x00;
            gameData->leds[i].b = 0x00;
        }
    }
    setLeds(gameData->leds, CONFIG_NUM_LEDS);
}

void pl_updateLedsLevelClear(plGameData_t * gameData){
    if(( (gameData->frameCount) % 10) == 0){
        for (int32_t i = 0; i < CONFIG_NUM_LEDS; i++)
        {
        
            if(( (gameData->frameCount >> 4) % CONFIG_NUM_LEDS) == i) {
                gameData->leds[i].g = (esp_random() % 24) * (10);
                gameData->leds[i].b = (esp_random() % 24) * (10);
            }

            if(gameData->leds[i].r > 0){
                gameData->leds[i].r -= 0x10;
            }
            
            if(gameData->leds[i].g > 0){
                gameData->leds[i].g -= 0x10;
            }

            if(gameData->leds[i].b > 0){
                gameData->leds[i].b -= 0x10;
            }
        }
    }
    setLeds(gameData->leds, CONFIG_NUM_LEDS);
}

void pl_updateLedsGameClear(plGameData_t * gameData){
    if(( (gameData->frameCount) % 10) == 0){
        for (int32_t i = 0; i < CONFIG_NUM_LEDS; i++)
        {
        
            if(( (gameData->frameCount >> 4) % CONFIG_NUM_LEDS) == i) {
                gameData->leds[i].r = (esp_random() % 24) * (10);
                gameData->leds[i].g = (esp_random() % 24) * (10);
                gameData->leds[i].b = (esp_random() % 24) * (10);
            }

            if(gameData->leds[i].r > 0){
                gameData->leds[i].r -= 0x10;
            }
            
            if(gameData->leds[i].g > 0){
                gameData->leds[i].g -= 0x10;
            }

            if(gameData->leds[i].b > 0){
                gameData->leds[i].b -= 0x10;
            }
        }
    }
    setLeds(gameData->leds, CONFIG_NUM_LEDS);
}
