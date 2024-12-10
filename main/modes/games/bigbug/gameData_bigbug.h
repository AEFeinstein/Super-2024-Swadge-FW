#ifndef _GAMEDATA_BIGBUG_H_
#define _GAMEDATA_BIGBUG_H_

//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>
#include <stdbool.h>
#include <esp_heap_caps.h>
#include "hdw-led.h"
#include "typedef_bigbug.h"
#include "entityManager_bigbug.h"
#include "tilemap_bigbug.h"
#include "palette.h"
#include "linked_list.h"

//==============================================================================
// Constants
//==============================================================================

//==============================================================================
// Structs
//==============================================================================

struct bb_camera_t
{
    rectangle_t camera;
    vec_t velocity; // stored retroactivelly to drive the starfield
};

/**
 * @brief Enum of screens that may be shown in bigbug mode
 */
enum bb_screen_t
{
    BIGBUG_RADAR_SCREEN,
    BIGBUG_GAME,
    BIGBUG_GAME_PINGING, // While game is still playing, but radar is pinging. Don't allow new radar pings.
};

struct bb_radarScreenData_t
{
    vec_t cam;                // x and y offsets for use in the radar (pause) screen.
    uint8_t playerPingRadius; // animates a circle to help the tiny player dot be seen.
};

struct bb_gameData_t
{
    int32_t elapsedUs;

    bool isPaused; ///< true if the game is paused, false if it is running

    uint16_t btnState;     // represents any buttons states
    uint16_t btnDownState; // represents the initial downpresses on each button

    int32_t touchPhi;
    int32_t touchRadius;
    int32_t touchIntensity;
    int32_t isTouched;
    int32_t touchX;
    int32_t touchY;

    midiFile_t bgm; ///< Background music. Midi files take turns loading in and out based on what is currently played.

    midiFile_t sfxBump;    ///< SFX bump into things
    midiFile_t sfxHarpoon; /// SFX harpoon
    midiFile_t sfxDirt;    /// SFT dirt crumbling

    bb_camera_t camera;

    bb_entityManager_t entityManager;

    led_t leds[CONFIG_NUM_LEDS];

    bool debugMode;

    uint8_t changeBgm;
    uint8_t currentBgm;

    bb_tilemap_t tilemap;

    int8_t neighbors[4][2]; // a handy table of left, up, right, and down offsets

    list_t pleaseCheck; // a list of tiles to check if they are supported.
    list_t unsupported; // a list of tiles that flood-fill crumble.

    font_t font;
    font_t tinyNumbers;

    bb_entity_t* menuBug; // Featured entity walking through the main menu.

    bool exit; // Entities can set it to true, and the mode will exit eventually.

    enum bb_screen_t screen; ///< The screen being displayed

    struct bb_radarScreenData_t radar;
};

//==============================================================================
// Functions
//==============================================================================
void bb_initializeGameData(bb_gameData_t* gameData);
void bb_freeGameData(bb_gameData_t* gameData);
void bb_initializeGameDataFromTitleScreen(bb_gameData_t* gameData);
void bb_updateLeds(bb_entityManager_t* entityManager, bb_gameData_t* gameData);
void bb_resetGameDataLeds(bb_gameData_t* gameData);
void bb_updateTouchInput(bb_gameData_t* gameData);

#endif