//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include "gameData.h"
#include "entityManager.h"
//#include "platformer_sounds.h"
#include "esp_random.h"
#include "hdw-btn.h"
#include "touchUtils.h"

//==============================================================================
// Functions
//==============================================================================
 void initializeGameData(gameData_t * gameData){
    gameData->gameState = 0;
    gameData->btnState = 0;
    gameData->score = 0;
    gameData->lives = 3;
    gameData->countdown = 000;

    gameData->level = 1;
    gameData->frameCount = 0;

    gameData->combo = 0;
    //gameData->comboTimer = 0;
    gameData->bgColor = c335;
    gameData->initials[0] = 'A';
    gameData->initials[1] = 'A';
    gameData->initials[2] = 'A';
    gameData->rank = 5;
    /*gameData->extraLifeCollected = false;
    gameData->checkpoint = 0;
    gameData->levelDeaths = 0;
    gameData->initialHp = 1;*/
    gameData->debugMode = false;
    gameData->continuesUsed = false;
    gameData->inGameTimer = 0;

    gameData->playerBombs[0] = NULL;
    gameData->playerBombs[1] = NULL;
    gameData->playerBombs[2] = NULL;

    gameData->playerTimeBombsCount = 0;
    gameData->playerRemoteBombPlaced  = false;
    /*gameData->nextBombToDetonate = 0;
    gameData->nextBombSlot = 0;
    gameData->bombDetonateCooldown = 0;*/
}

 void initializeGameDataFromTitleScreen(gameData_t * gameData){
    gameData->gameState = 0;
    gameData->btnState = 0;
    gameData->score = 0;
    gameData->lives = 3;
    gameData->countdown = 000;
    gameData->frameCount = 0;

    gameData->combo = 0;
    //gameData->comboTimer = 0;
    gameData->bgColor = c000;
    gameData->currentBgm = 0;
    gameData->changeBgm = 0;
    gameData->continuesUsed = (gameData->level == 1) ? false : true;
    gameData->inGameTimer = 0;
    gameData->targetBlocksBroken = 0;

    resetGameDataLeds(gameData);

    gameData->playerTimeBombsCount = 0;
    gameData->playerRemoteBombPlaced = false;
    //gameData->nextBombToDetonate = 0;
    //gameData->nextBombSlot = 0;
    //gameData->bombDetonateCooldown = 0;
}

void updateLedsHpMeter(entityManager_t *entityManager, gameData_t *gameData){
    if(entityManager->playerEntity == NULL){
        return;
    }

    uint8_t hp = 0; //entityManager->playerEntity->hp;
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

void scorePoints(gameData_t * gameData, uint16_t points, int16_t incCombo){
    gameData->combo += incCombo;
    if(gameData->combo < 1){
        gameData->combo = 1;
    }
    
    uint32_t comboPoints = points * gameData->combo;

    gameData->score += comboPoints;
    gameData->comboScore = comboPoints;
    
    //gameData->comboTimer = (gameData->levelDeaths < 3) ? 240: 1;
}

void resetGameDataLeds(gameData_t * gameData)
{
    for(uint8_t i=0;i<CONFIG_NUM_LEDS; i++){
        gameData->leds[i].r = 0;
        gameData->leds[i].g = 0;
        gameData->leds[i].b = 0;
    }

    setLeds(gameData->leds, CONFIG_NUM_LEDS);
}

void updateLedsShowHighScores(gameData_t * gameData){
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

void updateLedsGameOver(gameData_t * gameData){
    if(( (gameData->frameCount) % 10) == 0){
        for (int32_t i = 0; i < 8; i++)
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

void updateLedsLevelClear(gameData_t * gameData){
    if(( (gameData->frameCount) % 10) == 0){
        for (int32_t i = 0; i < 8; i++)
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

void updateLedsGameClear(gameData_t * gameData){
    if(( (gameData->frameCount) % 10) == 0){
        for (int32_t i = 0; i < 8; i++)
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

void updateLedsInGame(gameData_t * gameData){

    for (int32_t i = 0; i < 8; i++)
    {
        if(gameData->leds[i].r > 4){
            gameData->leds[i].r -= 0x02;
        } else if(gameData->leds[i].r <= 4){
            gameData->leds[i].r = 0x00;
        }
        
        if(gameData->leds[i].g > 4){
            gameData->leds[i].g -= 0x02;
        } else if(gameData->leds[i].g <= 4){
            gameData->leds[i].g = 0x00;
        }

        if(gameData->leds[i].b > 4){
            gameData->leds[i].b -= 0x02;
        } else if(gameData->leds[i].b <= 4){
            gameData->leds[i].b = 0x00;
        }

    }

    setLeds(gameData->leds, CONFIG_NUM_LEDS);
}

void updateTouchInput(gameData_t * gameData){
    if(getTouchJoystick(&(gameData->touchPhi), &(gameData->touchRadius), &(gameData->touchIntensity))){
        gameData->isTouched = true;
        getTouchCartesian(gameData->touchPhi, gameData->touchRadius, &(gameData->touchX), &(gameData->touchY));
    } else {
        gameData->isTouched = false;
    }
}