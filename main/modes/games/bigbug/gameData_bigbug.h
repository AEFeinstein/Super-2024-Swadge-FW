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
#include "wsgPalette.h"

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
    BIGBUG_RADAR_UPGRADE_SCREEN,
    BIGBUG_GAME,
    BIGBUG_GAME_PINGING, // While game is still playing, but radar is pinging. Don't allow new radar pings.
    BIGBUG_GARBOTNIK_UPGRADE_SCREEN,
};

enum bb_radarUpgrade_t
{
    BIGBUG_GARBAGE_DENSITY,
    BIGBUG_INFINITE_RANGE,
    BIGBUG_FUEL,
    BIGBUG_ENEMIES,
    BIGBUG_ACTIVE_BOOSTER,
    BIGBUG_OLD_BOOSTERS,
    BIGBUG_POINTS_OF_INTEREST,
    BIGBUG_REFILL_AMMO,
};

struct bb_radarScreenData_t
{
    vec_t cam;                // x and y offsets for use in the radar (pause) screen.
    uint8_t playerPingRadius; // animates a circle to help the tiny player dot be seen.
    uint8_t upgrades;         // the radar upgrade bools are bitpacked into this
                              // 0b1       garbage density  1 << 0
                              // 0b10      infinite range   1 << 1
                              // 0b100     fuel             1 << 2
                              // 0b1000    enemies          1 << 3
                              // 0b10000   active booster   1 << 4
                              // 0b100000  old boosters     1 << 5
                              // 0b1000000 more points of interest 1 << 6
                              // 0b10000000 refill ammo     1 << 7
    int8_t choices[2];        // the choices presented to the player. -1 means no choice.
};

enum bb_garbotnikUpgrade_t
{
    GARBOTNIK_REDUCED_FUEL_CONSUMPTION,
    GARBOTNIK_FASTER_FIRE_RATE,
    GARBOTNIK_MORE_DIGGING_STRENGTH,
};

struct bb_garbotnikUpgradeScreenData_t
{
    uint8_t upgrades;   // the garbotnik upgrade bools are bitpacked into this
                        // 0b1       reduced fuel consumption  1 << 0
                        // 0b10      faster fire rate          1 << 1
                        // 0b100     more digging strength     1 << 2
    uint8_t choices[2]; // the choices presented to the player. -1 means no choice.
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
    font_t sevenSegment;

    bb_entity_t* menuBug; // Featured entity walking through the main menu.

    bool exit; // Entities can set it to true, and the mode will exit eventually.

    enum bb_screen_t screen; ///< The screen being displayed

    struct bb_radarScreenData_t radar;
    struct bb_garbotnikUpgradeScreenData_t garbotnikUpgrade;

    uint8_t carFightState; ///< 0 means no active car fight, greater than 0 means number of kills remaining

    wsgPalette_t damagePalette; ///< More vibrant reddish colors to use when something takes damage.
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