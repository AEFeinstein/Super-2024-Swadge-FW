//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include "paGameData.h"
#include "paTables.h"
#include "paEntityManager.h"
#include "esp_random.h"
#include "hdw-btn.h"
#include "soundFuncs.h"

//==============================================================================
// Functions
//==============================================================================
void pa_initializeGameData(paGameData_t* gameData, paSoundManager_t* soundManager)
{
    gameData->gameState       = 0;
    gameData->btnState        = 0;
    gameData->score           = 0;
    gameData->extraLifeScore  = 10000;
    gameData->lives           = 3;
    gameData->levelTime       = 000;
    gameData->level           = 1;
    gameData->frameCount      = 0;
    gameData->bgColor         = c000;
    gameData->initials[0]     = 'A';
    gameData->initials[1]     = 'A';
    gameData->initials[2]     = 'A';
    gameData->rank            = 5;
    gameData->debugMode       = false;
    gameData->continuesUsed   = false;
    gameData->inGameTimer     = 0;
    gameData->soundManager    = soundManager;
    gameData->playerCharacter = PA_PLAYER_CHARACTER_PANGO;
}

void pa_initializeGameDataFromTitleScreen(paGameData_t* gameData)
{
    gameData->gameState                = 0;
    gameData->btnState                 = 0;
    gameData->score                    = 0;
    gameData->extraLifeScore           = 10000;
    gameData->lives                    = 3;
    gameData->levelTime                = 000;
    gameData->frameCount               = 0;
    gameData->bgColor                  = c000;
    gameData->currentBgm               = 0;
    gameData->changeBgm                = 0;
    gameData->continuesUsed            = (gameData->level == 1) ? false : true;
    gameData->inGameTimer              = 0;
    gameData->firstBonusItemDispensed  = false;
    gameData->secondBonusItemDispensed = false;

    pa_resetGameDataLeds(gameData);
}

void pa_scorePoints(paGameData_t* gameData, uint16_t points)
{
    gameData->score += points;

    if (gameData->score >= gameData->extraLifeScore)
    {
        gameData->lives++;
        gameData->extraLifeScore += 10000; //(gameData->extraLifeScore + 2000);
        gameData->leds[ledRemap[0]].r = 0xFF;
        gameData->leds[ledRemap[0]].g = 0xFF;
        gameData->leds[ledRemap[0]].b = 0xFF;
        soundPlaySfx(&(gameData->soundManager->snd1up), MIDI_SFX);
    }
}

void pa_resetGameDataLeds(paGameData_t* gameData)
{
    for (uint8_t i = 0; i < CONFIG_NUM_LEDS; i++)
    {
        gameData->leds[i].r = 0;
        gameData->leds[i].g = 0;
        gameData->leds[i].b = 0;
    }

    setLeds(gameData->leds, CONFIG_NUM_LEDS);
}

void pa_updateLedsShowHighScores(paGameData_t* gameData)
{
    if (((gameData->frameCount) % 10) == 0)
    {
        for (int32_t i = 0; i < CONFIG_NUM_LEDS; i++)
        {
            if (((gameData->frameCount >> 4) % CONFIG_NUM_LEDS) == i)
            {
                gameData->leds[i].r = 0xFF;
                gameData->leds[i].g = 0xFF;
                gameData->leds[i].b = 0xFF;
            }

            if (gameData->leds[i].r > 0x05)
            {
                gameData->leds[i].r -= 0x05;
            } else {
                gameData->leds[i].r -= 0;
            }

            if (gameData->leds[i].g > 0x10)
            {
                gameData->leds[i].g -= 0x10;
            }else {
                gameData->leds[i].g -= 0;
            }

            if (gameData->leds[i].b > 0x40)
            {
                gameData->leds[i].b -= 0x40;
            }else {
                gameData->leds[i].b = 0;
            }
        }
    }
    setLeds(gameData->leds, CONFIG_NUM_LEDS);
}

void pa_updateLedsGameOver(paGameData_t* gameData)
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

void pa_updateLedsLevelClear(paGameData_t* gameData)
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

void pa_updateLedsGameClear(paGameData_t* gameData)
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

void pa_updateLedsInGame(paGameData_t* gameData){
    if (((gameData->frameCount) % 4) == 0)
    {
        for (int32_t i = 0; i < CONFIG_NUM_LEDS; i++)
        {
            uint8_t mappedLed = ledRemap[i];
            uint8_t nextLed = ledRemap[(i + 1) % CONFIG_NUM_LEDS];

            if (gameData->leds[mappedLed].r > 0xF)
            {
                gameData->leds[mappedLed].r -= 0x10;

                if((gameData->leds[mappedLed].r >> 4) == 0xC && (i < CONFIG_NUM_LEDS-1)){
                    gameData->leds[nextLed].r = 0xF0;
                }
            } else {
                gameData->leds[mappedLed].r = 0;
            }

            if (gameData->leds[mappedLed].g > 0xF)
            {
                gameData->leds[mappedLed].g -= 0x10;

                if((gameData->leds[mappedLed].g >> 4) == 0xC && (i < CONFIG_NUM_LEDS-1)){
                    gameData->leds[nextLed].g = 0xF0;
                }
            } else {
                gameData->leds[mappedLed].g = 0;
            }

            if (gameData->leds[mappedLed].b > 0xF)
            {
                gameData->leds[mappedLed].b -= 0x10;

                if((gameData->leds[mappedLed].b >> 4) == 0xC && (i < CONFIG_NUM_LEDS-1)){
                gameData->leds[nextLed].b = 0xF0;
                }
            } else {
                gameData->leds[mappedLed].b = 0;
            }
        }
    }
    setLeds(gameData->leds, CONFIG_NUM_LEDS);
}

void pa_fadeLeds(paGameData_t* gameData){
    if (((gameData->frameCount) % 10) == 0)
    {
        for (int32_t i = 0; i < CONFIG_NUM_LEDS; i++)
        {
            if (gameData->leds[i].r > 0xF)
            {
                gameData->leds[i].r -= 0x10;
            } else {
                gameData->leds[i].r = 0;
            }

            if (gameData->leds[i].g > 0xF)
            {
                gameData->leds[i].g -= 0x10;
            } else {
                gameData->leds[i].g = 0;
            }

            if (gameData->leds[i].b > 0xF)
            {
                gameData->leds[i].b -= 0x10;
            } else {
                gameData->leds[i].b = 0;
            }
        }
    }
    setLeds(gameData->leds, CONFIG_NUM_LEDS);
}