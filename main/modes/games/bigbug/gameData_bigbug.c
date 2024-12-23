//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include "gameData_bigbug.h"
#include "entityManager_bigbug.h"
#include "esp_random.h"
#include "hdw-btn.h"
#include "touchUtils.h"
#include "soundFuncs.h"

//==============================================================================
// Functions
//==============================================================================
void bb_initializeGameData(bb_gameData_t* gameData)
{
    // Set the mode to game mode
    gameData->screen = BIGBUG_GAME;

    gameData->debugMode = false;

    gameData->GarbotnikStat_fireTime = 200;
    gameData->GarbotnikStat_diggingStrength = 1;
    gameData->GarbotnikStat_fuelConsumptionRate = 4;

    loadMidiFile("BigBug_Dr.Garbotniks Home.mid", &gameData->bgm, true);
    // loadMidiFile("BigBugExploration.mid", &gameData->bgm, true);
    // loadMidiFile("Big Bug Hurry up.mid", &gameData->hurryUp, true);
    // loadMidiFile("BigBug_Dr.Garbotniks Home.mid", &gameData->garbotniksHome, true);
    // loadMidiFile("BigBug_Space Travel.mid", &gameData->spaceTravel, true);

    loadMidiFile("Bump.mid", &gameData->sfxBump, true);
    loadMidiFile("Harpoon.mid", &gameData->sfxHarpoon, true);
    loadMidiFile("Dirt_Breaking.mid", &gameData->sfxDirt, true);
    loadMidiFile("BigBug - Egg 2.mid", &gameData->sfxEgg, true);
    loadMidiFile("BigBug - Egg 1.mid", &gameData->sfxDamage, true);
    loadMidiFile("r_item_get.mid", &gameData->sfxCollection, true);
    loadMidiFile("r_p_ice.mid", &gameData->sfxTether, true);
    loadMidiFile("r_health.mid", &gameData->sfxHealth, true);

    gameData->neighbors[0][0] = -1; // left  neighbor x offset
    gameData->neighbors[0][1] = 0;  // left  neighbor y offset
    gameData->neighbors[1][0] = 0;  // up    neighbor x offset
    gameData->neighbors[1][1] = -1; // up    neighbor y offset
    gameData->neighbors[2][0] = 1;  // right neighbor x offset
    gameData->neighbors[2][1] = 0;  // right neighbor y offset
    gameData->neighbors[3][0] = 0;  // down  neighbor x offset
    gameData->neighbors[3][1] = 1;  // down  neighbor y offset

    // Load font
    loadFont("ibm_vga8.font", &gameData->font, false);
    loadFont("tiny_numbers.font", &gameData->tinyNumbersFont, false);
    loadFont("seven_segment.font", &gameData->sevenSegmentFont, false);

    memset(&gameData->pleaseCheck, 0, sizeof(list_t));
    memset(&gameData->unsupported, 0, sizeof(list_t));

    // Palette setup
    wsgPaletteReset(&gameData->damagePalette);
    for (int color = 0; color < 215; color++)
    {
        uint32_t rgbCol = paletteToRGB((paletteColor_t)color);
        // don't modify blue channel
        //  int16_t newChannelColor = (rgbCol >> 16) & 255;
        //  newChannelColor += 51;
        //  if(newChannelColor > 255)
        //  {
        //      newChannelColor = 255;
        //  }
        //  rgbCol = (rgbCol & 0x00FFFF) | (newChannelColor << 16);

        // decrement green by 51
        int16_t newChannelColor = (rgbCol >> 8) & 255;
        newChannelColor -= 51;
        if (newChannelColor < 0)
        {
            newChannelColor = 0;
        }
        rgbCol = (rgbCol & 0xFF00FF) | (newChannelColor << 8);

        // increment red by 102
        newChannelColor = rgbCol & 255;
        newChannelColor += 102;
        if (newChannelColor > 255)
        {
            newChannelColor = 255;
        }
        rgbCol = (rgbCol & 0x00FFFF) | newChannelColor;
        wsgPaletteSet(&gameData->damagePalette, (paletteColor_t)color, RGBtoPalette(rgbCol));
    }
}

void bb_freeGameData(bb_gameData_t* gameData)
{
    unloadMidiFile(&gameData->bgm);

    unloadMidiFile(&gameData->sfxBump);
    unloadMidiFile(&gameData->sfxHarpoon);
    unloadMidiFile(&gameData->sfxDirt);
    unloadMidiFile(&gameData->sfxEgg);
    unloadMidiFile(&gameData->sfxDamage);
    unloadMidiFile(&gameData->sfxCollection);
    unloadMidiFile(&gameData->sfxTether);
    unloadMidiFile(&gameData->sfxHealth);
    freeFont(&gameData->font);
    freeFont(&gameData->tinyNumbersFont);
    freeFont(&gameData->sevenSegmentFont);
    freeFont(&gameData->cgFont);
    freeFont(&gameData->cgThinFont);
    while (gameData->unsupported.first)
    {
        heap_caps_free(shift(&gameData->unsupported));
    }
    while (gameData->pleaseCheck.first)
    {
        heap_caps_free(shift(&gameData->unsupported));
    }
}

void bb_initializeGameDataFromTitleScreen(bb_gameData_t* gameData)
{
    gameData->currentBgm = 0;
    gameData->changeBgm  = 0;

    bb_resetGameDataLeds(gameData);
}

void bb_updateLeds(bb_entityManager_t* entityManager, bb_gameData_t* gameData)
{
    if (entityManager->playerEntity == NULL)
    {
        return;
    }

    for (int32_t i = 1; i < 7; i++)
    {
        gameData->leds[i].r = 0x80;
        gameData->leds[i].g = 0x00;
        gameData->leds[i].b = 0x00;
    }

    setLeds(gameData->leds, CONFIG_NUM_LEDS);
}

void bb_resetGameDataLeds(bb_gameData_t* gameData)
{
    for (uint8_t i = 0; i < CONFIG_NUM_LEDS; i++)
    {
        gameData->leds[i].r = 0;
        gameData->leds[i].g = 0;
        gameData->leds[i].b = 0;
    }

    setLeds(gameData->leds, CONFIG_NUM_LEDS);
}

void bb_updateTouchInput(bb_gameData_t* gameData)
{
    if (getTouchJoystick(&(gameData->touchPhi), &(gameData->touchRadius), &(gameData->touchIntensity)))
    {
        gameData->isTouched = true;
        getTouchCartesian(gameData->touchPhi, gameData->touchRadius, &(gameData->touchX), &(gameData->touchY));
    }
    else
    {
        gameData->isTouched = false;
    }
}
