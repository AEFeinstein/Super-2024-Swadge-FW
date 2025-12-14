//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include "mgGameData.h"
#include "mgEntityManager.h"
#include "esp_random.h"
#include "hdw-btn.h"
#include "soundFuncs.h"
#include "hdw-nvs.h"

//==============================================================================
// Functions
//==============================================================================
void mg_initializeGameData(mgGameData_t* gameData, mgSoundManager_t* soundManager)
{
    gameData->gameState            = 0;
    gameData->btnState             = 0;
    gameData->score                = 0;
    gameData->lives                = 3;
    gameData->countdown            = 000;
    gameData->level                = 1;
    gameData->frameCount           = 0;
    gameData->coins                = 0;
    gameData->combo                = 0;
    gameData->comboTimer           = 0;
    gameData->initials[0]          = 'A';
    gameData->initials[1]          = 'A';
    gameData->initials[2]          = 'A';
    gameData->rank                 = 5;
    gameData->extraLifeCollected   = false;
    gameData->checkpointSpawnIndex = 0;
    gameData->levelDeaths          = 0;
    gameData->initialHp            = 1;
    gameData->debugMode            = false;
    gameData->continuesUsed        = false;
    gameData->inGameTimer          = 0;
    gameData->soundManager         = soundManager;
    gameData->bgColors             = bgGradientCyan;
}

void mg_initializeGameDataFromTitleScreen(mgGameData_t* gameData)
{
    gameData->gameState            = 0;
    gameData->btnState             = 0;
    gameData->score                = 0;
    gameData->lives                = 3;
    gameData->countdown            = 000;
    gameData->pauseCountdown       = false;
    gameData->frameCount           = 0;
    gameData->coins                = 0;
    gameData->combo                = 0;
    gameData->comboTimer           = 0;
    gameData->extraLifeCollected   = false;
    gameData->checkpointLevel      = 0;
    gameData->checkpointSpawnIndex = 0;
    gameData->levelDeaths          = 0;
    gameData->currentBgm           = 0;
    gameData->changeBgm            = MG_BGM_NO_CHANGE;
    gameData->initialHp            = 1;
    gameData->continuesUsed        = 0; //(gameData->world == 1 && gameData->level == 1) ? false : true;
    gameData->inGameTimer          = 0;
    gameData->bgColors             = bgGradientCyan;
    gameData->customLevel          = false;
    int32_t outVal                 = 0;
    readNvs32(MG_cheatModeNVSKey, &outVal);
    gameData->cheatMode = outVal;
    outVal              = 0;
    readNvs32(MG_abilitiesNVSKey, &outVal);
    gameData->abilities = outVal;

    mg_resetGameDataLeds(gameData);
}

void mg_updateLedsHpMeter(mgEntityManager_t* entityManager, mgGameData_t* gameData)
{
    if (entityManager->playerEntity == NULL)
    {
        return;
    }

    uint8_t hp = entityManager->playerEntity->hp;
    if (hp > 3)
    {
        hp = 3;
    }

    // HP meter led pairs:
    // 3 4
    // 2 5
    // 1 6
    for (int32_t i = 1; i < CONFIG_NUM_LEDS; i++)
    {
        gameData->leds[i].r = 0x80;
        gameData->leds[i].g = 0x00;
        gameData->leds[i].b = 0x00;
    }

    for (int32_t i = 1; i < 1 + hp; i++)
    {
        gameData->leds[i].r = 0x00;
        gameData->leds[i].g = 0x80;

        gameData->leds[7 - i].r = 0x00;
        gameData->leds[7 - i].g = 0x80;
    }

    setLeds(gameData->leds, CONFIG_NUM_LEDS);
}

void mg_scorePoints(mgGameData_t* gameData, uint16_t points)
{
    gameData->combo++;

    uint32_t comboPoints = points * gameData->combo;

    gameData->score += comboPoints;
    gameData->comboScore = comboPoints;

    gameData->comboTimer = (gameData->levelDeaths < 3) ? 240 : 1;
}

void addCoins(mgGameData_t* gameData, uint8_t coins)
{
    gameData->coins += coins;
    if (gameData->coins > 99)
    {
        gameData->lives++;
        soundPlaySfx(&(gameData->soundManager->snd1up), BZR_LEFT);
        gameData->coins = 0;
    }
    else
    {
        soundPlaySfx(&(gameData->soundManager->sndCoin), BZR_LEFT);
    }
}

void updateComboTimer(mgGameData_t* gameData)
{
    gameData->comboTimer--;

    if (gameData->comboTimer < 0)
    {
        gameData->comboTimer = 0;
        gameData->combo      = 0;
    }
}

void mg_resetGameDataLeds(mgGameData_t* gameData)
{
    for (uint8_t i = 0; i < CONFIG_NUM_LEDS; i++)
    {
        gameData->leds[i].r = 0;
        gameData->leds[i].g = 0;
        gameData->leds[i].b = 0;
    }

    setLeds(gameData->leds, CONFIG_NUM_LEDS);
}

void mg_updateLedsShowHighScores(mgGameData_t* gameData)
{
    if (((gameData->frameCount) % 10) == 0)
    {
        for (int32_t i = 0; i < CONFIG_NUM_LEDS; i++)
        {
            if (((gameData->frameCount >> 4) % CONFIG_NUM_LEDS) == i)
            {
                gameData->leds[i].r = 0xF0;
                gameData->leds[i].g = 0xF0;
                gameData->leds[i].b = 0x00;
            }

            if (gameData->leds[i].r > 0)
            {
                gameData->leds[i].r -= 0x05;
            }

            if (gameData->leds[i].g > 0)
            {
                gameData->leds[i].g -= 0x10;
            }

            if (gameData->leds[i].b > 0)
            {
                gameData->leds[i].b = 0x00;
            }
        }
    }
    setLeds(gameData->leds, CONFIG_NUM_LEDS);
}

void mg_updateLedsGameOver(mgGameData_t* gameData)
{
    if (((gameData->frameCount) % 10) == 0)
    {
        for (int32_t i = 0; i < CONFIG_NUM_LEDS; i++)
        {
            if (((gameData->frameCount >> 4) % CONFIG_NUM_LEDS) == i)
            {
                gameData->leds[i].r = 0xF0;
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

void mg_updateLedsLevelClear(mgGameData_t* gameData)
{
    if (((gameData->frameCount) % 10) == 0)
    {
        for (int32_t i = 0; i < CONFIG_NUM_LEDS; i++)
        {
            if (((gameData->frameCount >> 4) % CONFIG_NUM_LEDS) == i)
            {
                gameData->leds[i].g = (esp_random() % 24) * (10);
                gameData->leds[i].b = (esp_random() % 24) * (10);
            }

            if (gameData->leds[i].r > 0)
            {
                gameData->leds[i].r -= 0x10;
            }

            if (gameData->leds[i].g > 0)
            {
                gameData->leds[i].g -= 0x10;
            }

            if (gameData->leds[i].b > 0)
            {
                gameData->leds[i].b -= 0x10;
            }
        }
    }
    setLeds(gameData->leds, CONFIG_NUM_LEDS);
}

void mg_updateLedsGameClear(mgGameData_t* gameData)
{
    if (((gameData->frameCount) % 10) == 0)
    {
        for (int32_t i = 0; i < CONFIG_NUM_LEDS; i++)
        {
            if (((gameData->frameCount >> 4) % CONFIG_NUM_LEDS) == i)
            {
                gameData->leds[i].r = (esp_random() % 24) * (10);
                gameData->leds[i].g = (esp_random() % 24) * (10);
                gameData->leds[i].b = (esp_random() % 24) * (10);
            }

            if (gameData->leds[i].r > 0)
            {
                gameData->leds[i].r -= 0x10;
            }

            if (gameData->leds[i].g > 0)
            {
                gameData->leds[i].g -= 0x10;
            }

            if (gameData->leds[i].b > 0)
            {
                gameData->leds[i].b -= 0x10;
            }
        }
    }
    setLeds(gameData->leds, CONFIG_NUM_LEDS);
}

void mg_updateLeds(mgEntityManager_t* entityManager)
{
    if (entityManager->playerEntity == NULL)
    {
        return;
    }

    mgEntity_t* playerEntity = entityManager->playerEntity;
    mgGameData_t* gameData   = playerEntity->gameData;

    for (int32_t i = 0; i < CONFIG_NUM_LEDS; i++)
    {
        gameData->leds[i].b = 0x20 + abs(playerEntity->shotsFired << 1);

        if ((playerEntity->shotsFired <= -63) && (((gameData->frameCount >> 3) % 7) == i))
        {
            gameData->leds[i].g = 0xFF;
        }
        else
        {
            gameData->leds[i].g = 0x30 + abs(playerEntity->shotsFired << 1);
        }

        if (playerEntity->hp < 7)
        {
            gameData->leds[i].r = (gameData->frameCount << 2);
        }
        else if (playerEntity->hp < 13)
        {
            gameData->leds[i].r = (gameData->frameCount << 1);
        }
        else
        {
            gameData->leds[i].r = 0;
        }
    }

    setLeds(gameData->leds, CONFIG_NUM_LEDS);
}

void mg_updateLedsDead(mgGameData_t* gameData)
{
    for (int32_t i = 0; i < CONFIG_NUM_LEDS; i++)
    {
        if (gameData->frameCount < 16)
        {
            gameData->leds[i].r += 0x0F;
            gameData->leds[i].g += 0x0F;
            gameData->leds[i].b += 0x0F;
        }
        else
        {
            if (gameData->leds[i].r > 0x02)
            {
                gameData->leds[i].r -= 0x02;
            }
            else
            {
                gameData->leds[i].r -= 0;
            }

            if (gameData->leds[i].g > 0x05)
            {
                gameData->leds[i].g -= 0x05;
            }
            else
            {
                gameData->leds[i].g -= 0;
            }

            if (gameData->leds[i].b > 0x20)
            {
                gameData->leds[i].b -= 0x20;
            }
            else
            {
                gameData->leds[i].b = 0;
            }
        }
    }

    setLeds(gameData->leds, CONFIG_NUM_LEDS);
}