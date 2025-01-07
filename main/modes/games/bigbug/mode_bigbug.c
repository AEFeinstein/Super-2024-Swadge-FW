/**
 * @file mode_bigbug.c
 * @author James Albracht (James A on slack)
 * @brief Big Bug game
 * @date 2024-05-05
 */

//==============================================================================
// Includes
//==============================================================================

#include "gameData_bigbug.h"
#include "mode_bigbug.h"
#include "gameData_bigbug.h"
#include "tilemap_bigbug.h"
#include "worldGen_bigbug.h"
#include "entity_bigbug.h"
#include "entityManager_bigbug.h"
#include "random_bigbug.h"
#include "pathfinding_bigbug.h"
#include "esp_heap_caps.h"
#include "hdw-tft.h"
#include <math.h>
#include "mainMenu.h"

//==============================================================================
// Defines
//==============================================================================

//==============================================================================
// Enums
//==============================================================================

//==============================================================================
// Structs
//==============================================================================

struct bb_t
{
    menu_t* menu; ///< The menu structure
    // bb_screen_t screen; ///< The screen being displayed

    bb_gameData_t gameData;
};

//==============================================================================
// Function Prototypes
//==============================================================================

// required by adam
static void bb_EnterMode(void);
static void bb_EnterModeSkipIntro(void);
static void bb_ExitMode(void);
static void bb_MainLoop(int64_t elapsedUs);
static void bb_BackgroundDrawCallbackBlack(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);
static void bb_BackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);

// big bug logic
// static void bb_LoadScreenDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);
static void bb_DrawScene(void);
static void bb_DrawScene_Radar(void);
static void bb_DrawScene_Radar_Upgrade(void);
static void bb_GameLoop_Radar(int64_t elapsedUs);
static void bb_GameLoop_Radar_Upgrade(int64_t elapsedUs);
static void bb_DrawScene_Garbotnik_Upgrade(void);
static void bb_GameLoop_Garbotnik_Upgrade(int64_t elapsedUs);
static void bb_GameLoop_Loadout_Select(int64_t elapsedUs);
static void bb_GameLoop(int64_t elapsedUs);
static void bb_Reset(void);
static void bb_SetLeds(void);
static void bb_UpdateTileSupport(void);
static void bb_UpdateLEDs(bb_entityManager_t* entityManager);

//==============================================================================
// Strings
//==============================================================================

/* Design Pattern!
 * These strings are all declared 'const' because they do not change, so that they are placed in ROM, not RAM.
 * Lengths are not explicitly given so the compiler can figure it out.
 */
static const char bigbugName[] = "Big Bug";

// A tag for debug printing
const char BB_TAG[] = "BB";

//==============================================================================
// Variables
//==============================================================================

swadgeMode_t bigbugMode = {.modeName                 = bigbugName,
                           .wifiMode                 = NO_WIFI,
                           .overrideUsb              = false,
                           .usesAccelerometer        = true,
                           .usesThermometer          = true,
                           .overrideSelectBtn        = false,
                           .fnAudioCallback          = NULL,
                           .fnEnterMode              = bb_EnterMode, // SkipIntro,
                           .fnExitMode               = bb_ExitMode,
                           .fnMainLoop               = bb_MainLoop,
                           .fnBackgroundDrawCallback = bb_BackgroundDrawCallback,
                           .fnDacCb                  = NULL};

/// All state information for bigbug mode. This whole struct is calloc()'d and heap_caps_free()'d so that bigbug is only
/// using memory while it is being played
bb_t* bigbug = NULL;

/// @brief A heatshrink decoder to use for all WSG loads rather than allocate a new one for each WSG
/// This helps to prevent memory fragmentation in SPIRAM.
/// Note, this is outside the bb_t struct for easy access to loading fuctions without bb_t references
heatshrink_decoder* bb_hsd;
/// @brief A temporary decode space to use for all WSG loads
uint8_t* bb_decodeSpace;

//==============================================================================
// Required Functions
//==============================================================================

static void bb_EnterMode(void)
{
    setFrameRateUs(16667); // 60 FPS

    // Force draw a loading screen
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c123);

    // Allocate memory for the game state
    bigbug = heap_caps_calloc_tag(1, sizeof(bb_t), MALLOC_CAP_SPIRAM, "bigbug");

    // calloc the columns in layers separately to avoid a big alloc
    for (int32_t w = 0; w < TILE_FIELD_WIDTH; w++)
    {
        bigbug->gameData.tilemap.fgTiles[w]
            = heap_caps_calloc_tag(TILE_FIELD_HEIGHT, sizeof(bb_foregroundTileInfo_t), MALLOC_CAP_SPIRAM, "fgTile");
        bigbug->gameData.tilemap.mgTiles[w]
            = heap_caps_calloc_tag(TILE_FIELD_HEIGHT, sizeof(bb_midgroundTileInfo_t), MALLOC_CAP_SPIRAM, "mgTiles");
    }

    // Allocate WSG loading helpers
    bb_hsd = heatshrink_decoder_alloc(256, 8, 4);
    // The largest image is bb_menu2.png, decodes to 99124 bytes
    // 99328 is 1024 * 97
    bb_decodeSpace = heap_caps_malloc_tag(99328, MALLOC_CAP_SPIRAM, "decodeSpace");

    bb_SetLeds();

    // Load font
    loadFont("ibm_vga8.font", &bigbug->gameData.font, false);

    const char loadingStr[] = "Loading...";
    int32_t tWidth          = textWidth(&bigbug->gameData.font, loadingStr);
    drawText(&bigbug->gameData.font, c542, loadingStr, (TFT_WIDTH - tWidth) / 2,
             (TFT_HEIGHT - bigbug->gameData.font.height) / 2);
    drawDisplayTft(NULL);

    bb_initializeGameData(&bigbug->gameData);
    bb_initializeEntityManager(&bigbug->gameData.entityManager, &bigbug->gameData);
    // Shrink after loading initial sprites. The next largest image is ovo_talk0.png, decodes to 67200 bytes
    // 67584 is 1024 * 66
    bb_decodeSpace = heap_caps_realloc(bb_decodeSpace, 67584, MALLOC_CAP_SPIRAM);

    // bb_createEntity(&(bigbug->gameData.entityManager), LOOPING_ANIMATION, true, ROCKET_ANIM, 3,
    //                 (TILE_FIELD_WIDTH / 2) * TILE_SIZE + HALF_TILE + 1, -1000, true);

    bb_entity_t* foreground       = bb_createEntity(&(bigbug->gameData.entityManager), NO_ANIMATION, true, BB_MENU, 1,
                                                    (TILE_FIELD_WIDTH / 2) * TILE_SIZE + HALF_TILE - 1, -2573, true, false);
    foreground->updateFarFunction = &bb_updateFarMenuAndUnload; // This menu will unload menu sprites when it is far.

    foreground->updateFunction = NULL;
    foreground->drawFunction   = &bb_drawMenuForeground;
    bb_destroyEntity(((bb_menuData_t*)foreground->data)->cursor, false);

    bb_createEntity(&(bigbug->gameData.entityManager), NO_ANIMATION, true, BB_MENU, 1,
                    (foreground->pos.x >> DECIMAL_BITS), (foreground->pos.y >> DECIMAL_BITS), false, false);

    bigbug->gameData.entityManager.viewEntity
        = bb_createEntity(&(bigbug->gameData.entityManager), NO_ANIMATION, true, NO_SPRITE_POI, 1,
                          (foreground->pos.x >> DECIMAL_BITS), (foreground->pos.y >> DECIMAL_BITS) - 234, true, false);

    ((bb_goToData*)bigbug->gameData.entityManager.viewEntity->data)->executeOnArrival = &bb_startGarbotnikIntro;

    bigbug->gameData.camera.camera.pos.x = (bigbug->gameData.entityManager.viewEntity->pos.x >> DECIMAL_BITS) - 140;
    bigbug->gameData.camera.camera.pos.y = (bigbug->gameData.entityManager.viewEntity->pos.y >> DECIMAL_BITS) - 120;

    bb_generateWorld(&(bigbug->gameData.tilemap));

    // Player
    //  bb_createEntity(&(bigbug->gameData.entityManager), NO_ANIMATION, true, GARBOTNIK_FLYING, 1,
    //          5 * 32 + 16,
    //          -110);

    bb_setupMidi();
    soundPlayBgm(&bigbug->gameData.bgm, MIDI_BGM);

    bb_Reset();
}

static void bb_EnterModeSkipIntro(void)
{
    setFrameRateUs(16667); // 60 FPS

    // Force draw a loading screen
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c123);

    // Allocate memory for the game state
    bigbug = heap_caps_calloc(1, sizeof(bb_t), MALLOC_CAP_SPIRAM);

    // calloc the columns in layers separately to avoid a big alloc
    for (int32_t w = 0; w < TILE_FIELD_WIDTH; w++)
    {
        bigbug->gameData.tilemap.fgTiles[w]
            = heap_caps_calloc(TILE_FIELD_HEIGHT, sizeof(bb_foregroundTileInfo_t), MALLOC_CAP_SPIRAM);
        bigbug->gameData.tilemap.mgTiles[w]
            = heap_caps_calloc(TILE_FIELD_HEIGHT, sizeof(bb_midgroundTileInfo_t), MALLOC_CAP_SPIRAM);
    }

    // Allocate WSG loading helpers
    bb_hsd = heatshrink_decoder_alloc(256, 8, 4);
    // The largest image is bb_menu2.png, decodes to 99124 bytes
    // 99328 is 1024 * 97
    bb_decodeSpace = heap_caps_malloc(99328, MALLOC_CAP_SPIRAM);

    bb_SetLeds();

    // Load font
    loadFont("ibm_vga8.font", &bigbug->gameData.font, false);

    const char loadingStr[] = "Loading...";
    int32_t tWidth          = textWidth(&bigbug->gameData.font, loadingStr);
    drawText(&bigbug->gameData.font, c542, loadingStr, (TFT_WIDTH - tWidth) / 2,
             (TFT_HEIGHT - bigbug->gameData.font.height) / 2);
    drawDisplayTft(NULL);

    bb_initializeGameData(&bigbug->gameData);
    bb_initializeEntityManager(&bigbug->gameData.entityManager, &bigbug->gameData);
    // Shrink after loading initial sprites. The next largest image is ovo_talk0.png, decodes to 67200 bytes
    // 67584 is 1024 * 66
    bb_decodeSpace = heap_caps_realloc(bb_decodeSpace, 67584, MALLOC_CAP_SPIRAM);

    uint32_t deathDumpsterX = (TILE_FIELD_WIDTH / 2) * TILE_SIZE + HALF_TILE - 1;
    uint32_t deathDumpsterY = -2173;

    // create 3 rockets
    for (int rocketIdx = 0; rocketIdx < 3; rocketIdx++)
    {
        bigbug->gameData.entityManager.boosterEntities[rocketIdx]
            = bb_createEntity(&bigbug->gameData.entityManager, NO_ANIMATION, true, ROCKET_ANIM, 16,
                              deathDumpsterX - 96 + 96 * rocketIdx, deathDumpsterY, false, true);

        // bigbug->gameData.entityManager.boosterEntities[rocketIdx]->updateFunction = NULL;

        if (rocketIdx >= 1)
        {
            bb_rocketData_t* rData = (bb_rocketData_t*)bigbug->gameData.entityManager.boosterEntities[rocketIdx]->data;

            rData->flame = bb_createEntity(
                &(bigbug->gameData.entityManager), LOOPING_ANIMATION, false, FLAME_ANIM, 6,
                bigbug->gameData.entityManager.boosterEntities[rocketIdx]->pos.x >> DECIMAL_BITS,
                bigbug->gameData.entityManager.boosterEntities[rocketIdx]->pos.y >> DECIMAL_BITS, true, false);

            rData->flame->updateFunction = &bb_updateFlame;
        }
        else // rocketIdx == 0
        {
            bigbug->gameData.entityManager.activeBooster = bigbug->gameData.entityManager.boosterEntities[rocketIdx];
            ((bb_rocketData_t*)bigbug->gameData.entityManager.activeBooster->data)->numDonuts = 3;
            bigbug->gameData.entityManager.activeBooster->currentAnimationFrame               = 40;
            bigbug->gameData.entityManager.activeBooster->pos.y                               = 50;
            bigbug->gameData.entityManager.activeBooster->updateFunction                      = bb_updateHeavyFalling;
            bb_entity_t* arm                                                                  = bb_createEntity(
                &bigbug->gameData.entityManager, NO_ANIMATION, true, ATTACHMENT_ARM, 1,
                bigbug->gameData.entityManager.activeBooster->pos.x >> DECIMAL_BITS,
                (bigbug->gameData.entityManager.activeBooster->pos.y >> DECIMAL_BITS) - 33, true, false);
            ((bb_attachmentArmData_t*)arm->data)->rocket = bigbug->gameData.entityManager.activeBooster;

            bb_entity_t* grabbyHand = bb_createEntity(
                &bigbug->gameData.entityManager, LOOPING_ANIMATION, true, BB_GRABBY_HAND, 6,
                bigbug->gameData.entityManager.activeBooster->pos.x >> DECIMAL_BITS,
                (bigbug->gameData.entityManager.activeBooster->pos.y >> DECIMAL_BITS) - 53, true, false);
            ((bb_grabbyHandData_t*)grabbyHand->data)->rocket = bigbug->gameData.entityManager.activeBooster;
        }
    }

    // create the death dumpster
    bigbug->gameData.entityManager.deathDumpster
        = bb_createEntity(&bigbug->gameData.entityManager, NO_ANIMATION, true, BB_DEATH_DUMPSTER, 1, deathDumpsterX,
                          deathDumpsterY, false, true);

    bigbug->gameData.entityManager.viewEntity
        = bb_createEntity(&(bigbug->gameData.entityManager), NO_ANIMATION, true, GARBOTNIK_FLYING, 1,
                          bigbug->gameData.entityManager.activeBooster->pos.x >> DECIMAL_BITS,
                          (bigbug->gameData.entityManager.activeBooster->pos.y >> DECIMAL_BITS) - 90, true, false);

    bb_loadWsgs(&bigbug->gameData.tilemap);

    bigbug->gameData.entityManager.playerEntity = bigbug->gameData.entityManager.viewEntity;

    bb_generateWorld(&(bigbug->gameData.tilemap));

    // Set the mode to game mode
    bigbug->gameData.screen = BIGBUG_GAME;

    bb_setupMidi();
    unloadMidiFile(&bigbug->gameData.bgm);
    loadMidiFile("BigBugExploration.mid", &bigbug->gameData.bgm, true);
    globalMidiPlayerPlaySong(&bigbug->gameData.bgm, MIDI_BGM);

    bb_Reset();
}

void bb_setupMidi(void)
{
    // Setup MIDI
    midiPlayer_t* player = globalMidiPlayerGet(MIDI_BGM);
    midiPlayerReset(player);
    midiGmOn(player);
    midiSetProgram(player, 12, 90); // talking sound effects arbitrarily go on channel 12 and use midi instrument 90.
    midiControlChange(player, 12, MCC_SUSTENUTO_PEDAL, 80);
    midiControlChange(player, 12, MCC_SOUND_RELEASE_TIME, 60);
    player->loop                 = true;
    player->volume               = 16383;
    player->songFinishedCallback = NULL;

    midiPlayer_t* sfx = globalMidiPlayerGet(MIDI_SFX);
    midiPlayerReset(sfx);
    midiGmOn(sfx);

    // midiPlayer_t* sfx = globalMidiPlayerGet(MIDI_SFX);
    // // Turn on the sustain pedal for channel 1
    // midiSustain(sfx, 2, 0x7F);
    // midiSetProgram(sfx, 0, 0);
}

void bb_FreeTilemapData(void)
{
    for (int32_t w = 0; w < TILE_FIELD_WIDTH; w++)
    {
        heap_caps_free(bigbug->gameData.tilemap.fgTiles[w]);
        heap_caps_free(bigbug->gameData.tilemap.mgTiles[w]);
    }

    while (bigbug->gameData.pleaseCheck.first != NULL)
    {
        uint8_t* shiftedVal = (uint8_t*)shift(&bigbug->gameData.pleaseCheck);
        heap_caps_free(shiftedVal);
    }

    while (bigbug->gameData.unsupported.first != NULL)
    {
        uint8_t* shiftedVal = (uint8_t*)shift(&bigbug->gameData.unsupported);
        heap_caps_free(shiftedVal);
    }
}

static void bb_ExitMode(void)
{
    soundStop(true);
    heatshrink_decoder_free(bb_hsd);
    heap_caps_free(bb_decodeSpace);

    // Destroy menu bug, just in case
    bb_destroyEntity(bigbug->gameData.menuBug, false);

    bb_freeGameData(&bigbug->gameData);
    bb_deactivateAllEntities(&bigbug->gameData.entityManager, false);
    // Free entity manager
    bb_freeEntityManager(&bigbug->gameData.entityManager);
    // Free font
    freeFont(&bigbug->gameData.font);

    if (bigbug->gameData.bgm.data)
    {
        unloadMidiFile(&bigbug->gameData.bgm);
    }

    deinitGlobalMidiPlayer();

    bb_freeWsgs(&bigbug->gameData.tilemap);

    bb_FreeTilemapData();

    heap_caps_free(bigbug);

    // heap_caps_free(bb_decodeSpace);
}

static void bb_MainLoop(int64_t elapsedUs)
{
    // Save the elapsed time
    bigbug->gameData.elapsedUs = elapsedUs;

    // Pick what runs and draws depending on the screen being displayed
    switch (bigbug->gameData.screen)
    {
        case BIGBUG_RADAR_SCREEN:
        {
            if (bigbugMode.fnBackgroundDrawCallback == bb_BackgroundDrawCallback)
            {
                bigbugMode.fnBackgroundDrawCallback = bb_BackgroundDrawCallbackBlack;
            }
            bb_GameLoop_Radar(elapsedUs);
            break;
        }
        case BIGBUG_RADAR_UPGRADE_SCREEN:
        {
            if (bigbugMode.fnBackgroundDrawCallback == bb_BackgroundDrawCallback)
            {
                bigbugMode.fnBackgroundDrawCallback = bb_BackgroundDrawCallbackBlack;
            }
            bb_GameLoop_Radar_Upgrade(elapsedUs);
            break;
        }
        case BIGBUG_GARBOTNIK_UPGRADE_SCREEN:
        {
            if (bigbugMode.fnBackgroundDrawCallback == bb_BackgroundDrawCallback)
            {
                bigbugMode.fnBackgroundDrawCallback = bb_BackgroundDrawCallbackBlack;
            }
            bb_GameLoop_Garbotnik_Upgrade(elapsedUs);
            break;
        }
        case BIGBUG_LOADOUT_SELECT_SCREEN:
        {
            if (bigbugMode.fnBackgroundDrawCallback == bb_BackgroundDrawCallback)
            {
                bigbugMode.fnBackgroundDrawCallback = bb_BackgroundDrawCallbackBlack;
            }
            bb_GameLoop_Loadout_Select(elapsedUs);
            break;
        }
        case BIGBUG_GAME:
        {
            // Run the main game loop. This will also process button events
            bb_GameLoop(elapsedUs);
            break;
        }
        case BIGBUG_GAME_PINGING:
        {
            bb_GameLoop(elapsedUs);
            break;
        }
        default:
        {
            break;
        }
    }
}

static void bb_BackgroundDrawCallbackBlack(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
    fillDisplayArea(x, y, x + w, y + h, c000);
}

static void bb_BackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
    // Convenience camera pointer
    vec_t* cameraPos = &bigbug->gameData.camera.camera.pos;

    // Flat background color
    paletteColor_t bgColor;
    if (cameraPos->y >= 400)
    {
        // Deep underground is black
        bgColor = c000;
    }
    else
    {
        // Lookup table of background colors from dark to light
        const paletteColor_t skyColors[] = {
            c000, c001, c011, //
            c111, c112, c122, //
            c222, c223, c233, //
            c333, c334, c344, //
            c444, c445, c455  //
        };
        // get steps from 0 (at -2000) to 14 (at -900)
        int32_t skyStep = (cameraPos->y + 2000) * 14 / 1100;
        skyStep         = CLAMP(skyStep, 0, ARRAY_SIZE(skyColors) - 1);
        bgColor         = skyColors[skyStep];
    }

    // Fill the flat background color
    paletteColor_t* frameBuf = getPxTftFramebuffer();
    memset(&frameBuf[(y * TFT_WIDTH) + x], bgColor, sizeof(paletteColor_t) * w * h);

    // Maybe draw some background
    bb_tilemap_t* tilemap = &bigbug->gameData.tilemap;
    if (tilemap->wsgsLoaded && -906 <= cameraPos->y && cameraPos->y <= 1022)
    {
        // Convenience widths
        int32_t bgWidth = tilemap->surface1Wsg.w;
        int32_t grWidth = tilemap->landfillGradient.w;

        // Calculate furthest background offset
        int32_t offsetY2 = -(cameraPos->y / 3) - 64;

        // Calculate nearer background offset
        int32_t offsetY1 = -(cameraPos->y / 2);

        // Calculate gradient offset (under nearer background)
        int32_t offsetYG = offsetY1 + tilemap->surface1Wsg.h;

        // Get a pointer to the framebuffer for fast pixel setting
        paletteColor_t* fb = &getPxTftFramebuffer()[TFT_WIDTH * y + x];

        // For each row
        for (int32_t iY = y; iY < y + h; iY++)
        {
            // Calculate source row pointers
            int32_t s1row              = (iY - offsetY1);
            bool drawS1row             = (0 <= s1row && s1row < tilemap->surface1Wsg.h);
            paletteColor_t* surface1px = &tilemap->surface1Wsg.px[bgWidth * s1row];

            int32_t s2row              = (iY - offsetY2);
            bool drawS2row             = (0 <= s2row && s2row < tilemap->surface2Wsg.h);
            paletteColor_t* surface2px = &tilemap->surface2Wsg.px[bgWidth * s2row];

            int32_t grRow              = (iY - offsetYG);
            bool drawGrRow             = (0 <= grRow && grRow < tilemap->landfillGradient.h);
            paletteColor_t* gradientPx = &tilemap->landfillGradient.px[grWidth * grRow];

            // For each pixel in the row
            for (int32_t iX = x; iX < x + w; iX++)
            {
                // Draw appropriate pixel
                if (drawS1row && cTransparent != surface1px[iX])
                {
                    *(fb++) = surface1px[iX];
                }
                else if (drawGrRow) // No transparency check
                {
                    *(fb++) = gradientPx[iX % grWidth];
                }
                else if (drawS2row) // No transparency check
                {
                    *(fb++) = surface2px[iX];
                }
                else
                {
                    // Nothing to draw here, advance the pointer
                    fb++;
                }
            }
        }
    }
}

//==============================================================================
// Big Bug Functions
//==============================================================================

static void bb_DrawScene(void)
{
    if ((bigbug->gameData.entityManager.playerEntity != NULL)
        && (GARBOTNIK_DATA == bigbug->gameData.entityManager.playerEntity->dataType))
    {
        vec_t garbotnikDrawPos = {.x = (bigbug->gameData.entityManager.playerEntity->pos.x >> DECIMAL_BITS)
                                       - bigbug->gameData.camera.camera.pos.x - 18,
                                  .y = (bigbug->gameData.entityManager.playerEntity->pos.y >> DECIMAL_BITS)
                                       - bigbug->gameData.camera.camera.pos.y - 17};
        bb_drawTileMap(&bigbug->gameData.tilemap, &bigbug->gameData.camera.camera, &garbotnikDrawPos,
                       &((bb_garbotnikData_t*)bigbug->gameData.entityManager.playerEntity->data)->yaw,
                       &bigbug->gameData.entityManager);
    }
    else
    {
        bb_drawTileMap(&bigbug->gameData.tilemap, &bigbug->gameData.camera.camera, &(vec_t){0, 0}, &(vec_t){0, 0},
                       &bigbug->gameData.entityManager);
    }

    bb_drawEntities(&bigbug->gameData.entityManager, &bigbug->gameData.camera.camera);
    // DRAW_FPS_COUNTER(bigbug->gameData.font);
}

static void bb_DrawScene_Radar(void)
{
    // Draw the tilemap squares
    for (int xIdx = 1; xIdx < TILE_FIELD_WIDTH - 3; xIdx++)
    {
        int16_t yMin = CLAMP(bigbug->gameData.radar.cam.y / 4, 0, TILE_FIELD_HEIGHT - 1);
        int16_t yMax = CLAMP(yMin + FIELD_HEIGHT / 4, 0, TILE_FIELD_HEIGHT);
        for (int yIdx = yMin; yIdx < yMax; yIdx++)
        {
            uint16_t radarTileColor = bigbug->gameData.tilemap.fgTiles[xIdx][yIdx].health > 0 ? c333 : c000;
            if ((bigbug->gameData.radar.upgrades >> BIGBUG_GARBAGE_DENSITY) & 1)
            {
                if (bigbug->gameData.tilemap.fgTiles[xIdx][yIdx].health > 0
                    && bigbug->gameData.tilemap.fgTiles[xIdx][yIdx].health < 4)
                {
                    radarTileColor = c222;
                }
                else if (bigbug->gameData.tilemap.fgTiles[xIdx][yIdx].health > 3
                         && bigbug->gameData.tilemap.fgTiles[xIdx][yIdx].health < 10)
                {
                    radarTileColor = c333;
                }
                else if (bigbug->gameData.tilemap.fgTiles[xIdx][yIdx].health > 9
                         && bigbug->gameData.tilemap.fgTiles[xIdx][yIdx].health < 100)
                {
                    radarTileColor = c444;
                }
                else if (bigbug->gameData.tilemap.fgTiles[xIdx][yIdx].health > 99)
                {
                    radarTileColor = c555;
                }
            }
            drawRectFilled(xIdx * 4 - 4, yIdx * 4 - bigbug->gameData.radar.cam.y, xIdx * 4,
                           yIdx * 4 + 4 - bigbug->gameData.radar.cam.y, radarTileColor);
        }
    }
    // draw eggs (enemies), skeletons (fuel), donuts, hotdogs
    // iterate tilemap embeds
    for (int xIdx = 1; xIdx < TILE_FIELD_WIDTH - 3; xIdx++)
    {
        int16_t yMin = CLAMP(bigbug->gameData.radar.cam.y / 4, 0, TILE_FIELD_HEIGHT - 1);
        int16_t yMax = CLAMP(yMin + FIELD_HEIGHT / 4, 0, TILE_FIELD_HEIGHT);
        for (int yIdx = yMin; yIdx < yMax; yIdx++)
        {
            if ((bigbug->gameData.radar.upgrades >> BIGBUG_ENEMIES) & 1)
            {
                if (bigbug->gameData.tilemap.fgTiles[xIdx][yIdx].embed == EGG_EMBED)
                {
                    drawCircleFilled(xIdx * 4 - 2, yIdx * 4 - bigbug->gameData.radar.cam.y, 1, c500);
                }
            }
            if ((bigbug->gameData.radar.upgrades >> BIGBUG_FUEL) & 1)
            {
                if (bigbug->gameData.tilemap.fgTiles[xIdx][yIdx].embed == SKELETON_EMBED)
                {
                    drawCircleFilled(xIdx * 4 - 2, yIdx * 4 - bigbug->gameData.radar.cam.y, 2, c050);
                }
            }
            if ((bigbug->gameData.radar.upgrades >> BIGBUG_POINTS_OF_INTEREST) & 1)
            {
                if (bigbug->gameData.tilemap.fgTiles[xIdx][yIdx].embed == BB_CAR_WITH_DONUT_EMBED
                    || bigbug->gameData.tilemap.fgTiles[xIdx][yIdx].embed == BB_FOOD_CART_WITH_DONUT_EMBED)
                {
                    drawWsgSimple(&bigbug->gameData.entityManager.sprites[BB_DONUT].frames[0], xIdx * 4 - 15,
                                  yIdx * 4 - bigbug->gameData.radar.cam.y - 6);
                }
                if (bigbug->gameData.tilemap.fgTiles[xIdx][yIdx].embed == BB_CAR_WITH_SWADGE_EMBED
                    || bigbug->gameData.tilemap.fgTiles[xIdx][yIdx].embed == BB_FOOD_CART_WITH_SWADGE_EMBED)
                {
                    drawWsgSimple(&bigbug->gameData.entityManager.sprites[BB_HOTDOG].frames[0], xIdx * 4 - 15,
                                  yIdx * 4 - bigbug->gameData.radar.cam.y - 6);
                }
            }
        }
    }

    // draw fuel, enemies, POIs
    // iterate all cached entities
    node_t* current = bigbug->gameData.entityManager.cachedEntities->first;
    while (current != NULL)
    {
        bb_entity_t* entity = (bb_entity_t*)current->val;
        if ((bigbug->gameData.radar.upgrades >> BIGBUG_ENEMIES) & 1)
        {
            if (entity->dataType == EGG_DATA)
            {
                drawCircleFilled((entity->pos.x >> DECIMAL_BITS) / 8 - 2,
                                 (entity->pos.y >> DECIMAL_BITS) / 8 - bigbug->gameData.radar.cam.y, 1, c500);
            }
            else if (entity->spriteIndex >= 8 && entity->spriteIndex <= 13)
            {
                drawCircleFilled((entity->pos.x >> DECIMAL_BITS) / 8 - 2,
                                 (entity->pos.y >> DECIMAL_BITS) / 8 - bigbug->gameData.radar.cam.y, 2, c530);
            }
        }
        if ((bigbug->gameData.radar.upgrades >> BIGBUG_FUEL) & 1)
        {
            if (entity->spriteIndex == BB_FUEL)
            {
                drawCircleFilled((entity->pos.x >> DECIMAL_BITS) / 8 - 2,
                                 (entity->pos.y >> DECIMAL_BITS) / 8 - bigbug->gameData.radar.cam.y, 2, c050);
            }
        }
        if ((bigbug->gameData.radar.upgrades >> BIGBUG_POINTS_OF_INTEREST) & 1)
        {
            if (entity->dataType == CAR_DATA || entity->dataType == FOOD_CART_DATA)
            {
                bb_PointOfInterestParentData_t* POIData = (bb_PointOfInterestParentData_t*)entity->data;
                if (POIData->reward == BB_DONUT)
                {
                    drawWsgSimple(&bigbug->gameData.entityManager.sprites[BB_DONUT].frames[0],
                                  (entity->pos.x >> DECIMAL_BITS) / 8 - 15,
                                  (entity->pos.y >> DECIMAL_BITS) / 8 - bigbug->gameData.radar.cam.y - 6);
                }
                else if (POIData->reward == BB_SWADGE)
                {
                    drawWsgSimple(&bigbug->gameData.entityManager.sprites[BB_HOTDOG].frames[0],
                                  (entity->pos.x >> DECIMAL_BITS) / 8 - 15,
                                  (entity->pos.y >> DECIMAL_BITS) / 8 - bigbug->gameData.radar.cam.y - 6);
                }
            }
        }

        current = current->next;
    }

    // iterate all active entities
    for (int i = 0; i < MAX_ENTITIES; i++)
    {
        bb_entity_t* entity = &bigbug->gameData.entityManager.entities[i];
        if (entity->active)
        {
            if ((bigbug->gameData.radar.upgrades >> BIGBUG_ENEMIES) & 1)
            {
                if (entity->dataType == EGG_DATA)
                {
                    drawCircleFilled((entity->pos.x >> DECIMAL_BITS) / 8 - 2,
                                     (entity->pos.y >> DECIMAL_BITS) / 8 - bigbug->gameData.radar.cam.y, 1, c500);
                }
                else if (entity->spriteIndex >= 8 && entity->spriteIndex <= 13)
                {
                    drawCircleFilled((entity->pos.x >> DECIMAL_BITS) / 8 - 2,
                                     (entity->pos.y >> DECIMAL_BITS) / 8 - bigbug->gameData.radar.cam.y, 2, c500);
                }
            }
            if ((bigbug->gameData.radar.upgrades >> BIGBUG_FUEL) & 1)
            {
                if (entity->spriteIndex == BB_FUEL)
                {
                    drawCircleFilled((entity->pos.x >> DECIMAL_BITS) / 8 - 2,
                                     (entity->pos.y >> DECIMAL_BITS) / 8 - bigbug->gameData.radar.cam.y, 2, c050);
                }
            }
            if ((bigbug->gameData.radar.upgrades >> BIGBUG_POINTS_OF_INTEREST) & 1)
            {
                if (entity->dataType == CAR_DATA || entity->dataType == FOOD_CART_DATA)
                {
                    bb_PointOfInterestParentData_t* POIData = (bb_PointOfInterestParentData_t*)entity->data;
                    if (POIData->reward == BB_DONUT)
                    {
                        drawWsgSimple(&bigbug->gameData.entityManager.sprites[BB_DONUT].frames[0],
                                      (entity->pos.x >> DECIMAL_BITS) / 8 - 15,
                                      (entity->pos.y >> DECIMAL_BITS) / 8 - bigbug->gameData.radar.cam.y - 6);
                    }
                    else if (POIData->reward == BB_SWADGE)
                    {
                        drawWsgSimple(&bigbug->gameData.entityManager.sprites[BB_HOTDOG].frames[0],
                                      (entity->pos.x >> DECIMAL_BITS) / 8 - 15,
                                      (entity->pos.y >> DECIMAL_BITS) / 8 - bigbug->gameData.radar.cam.y - 6);
                    }
                }
            }
        }
    }

    // draw garbotnik
    vec_t garbotnikPos = (vec_t){0};
    if (bigbug->gameData.entityManager.playerEntity != NULL)
    {
        garbotnikPos = (vec_t){(bigbug->gameData.entityManager.playerEntity->pos.x >> (DECIMAL_BITS + 3)) - 3,
                               (bigbug->gameData.entityManager.playerEntity->pos.y >> (DECIMAL_BITS + 3)) - 3};
    }
    else
    {
        garbotnikPos = (vec_t){(bigbug->gameData.entityManager.activeBooster->pos.x >> (DECIMAL_BITS + 3)) - 3,
                               (bigbug->gameData.entityManager.activeBooster->pos.y >> (DECIMAL_BITS + 3)) - 3};
    }

    drawCircleFilled(garbotnikPos.x, garbotnikPos.y - bigbug->gameData.radar.cam.y, 3, c515);
    // garbotnik's pings
    for (int i = 0; i < 254; i += 51)
    {
        bigbug->gameData.radar.playerPingRadius += i;
        drawCircle(garbotnikPos.x, garbotnikPos.y - bigbug->gameData.radar.cam.y,
                   bigbug->gameData.radar.playerPingRadius, c404);
    }

    // draw camera perimeter
    drawRect(garbotnikPos.x - 17, garbotnikPos.y - 15 - bigbug->gameData.radar.cam.y, garbotnikPos.x + 17,
             garbotnikPos.y + 15 - bigbug->gameData.radar.cam.y, c424);

    if ((bigbug->gameData.radar.upgrades >> BIGBUG_OLD_BOOSTERS) & 1)
    {
        for (int boosterIdx = 0; boosterIdx < 3; boosterIdx++)
        {
            if (bigbug->gameData.entityManager.activeBooster
                != bigbug->gameData.entityManager.boosterEntities[boosterIdx])
            {
                // booster radar rect
                garbotnikPos.x
                    = (bigbug->gameData.entityManager.boosterEntities[boosterIdx]->pos.x >> DECIMAL_BITS) / 8 - 4;
                garbotnikPos.y
                    = (bigbug->gameData.entityManager.boosterEntities[boosterIdx]->pos.y >> DECIMAL_BITS) / 8 - 4;
                drawRectFilled(garbotnikPos.x, garbotnikPos.y - bigbug->gameData.radar.cam.y, garbotnikPos.x + 2,
                               garbotnikPos.y + 8 - bigbug->gameData.radar.cam.y, c452);
            }
        }
    }

    if ((bigbug->gameData.radar.upgrades >> BIGBUG_ACTIVE_BOOSTER) & 1)
    {
        // booster radar rect
        garbotnikPos.x = (bigbug->gameData.entityManager.activeBooster->pos.x >> DECIMAL_BITS) / 8 - 4;
        garbotnikPos.y = (bigbug->gameData.entityManager.activeBooster->pos.y >> DECIMAL_BITS) / 8 - 4;
        drawRectFilled(garbotnikPos.x, garbotnikPos.y - bigbug->gameData.radar.cam.y, garbotnikPos.x + 2,
                       garbotnikPos.y + 8 - bigbug->gameData.radar.cam.y, c124);
    }

    if ((bigbug->gameData.radar.upgrades >> BIGBUG_INFINITE_RANGE) & 1)
    {
        if ((bigbug->gameData.radar.playerPingRadius % 51) < 26)
        {
            drawTriangleOutlined(140, 2, 148, 10, 132, 10, c222, c550);
            drawTriangleOutlined(132, 230, 148, 230, 140, 238, c222, c550);
        }
    }

    // DRAW_FPS_COUNTER(bigbug->gameData.font);
}

static void bb_DrawScene_Radar_Upgrade(void)
{
    drawRect(1, 1, 279, 239, c132);
    drawRect(3, 3, 277, 237, c141);
    drawCircleQuadrants(238, 41, 40, false, false, false, true, c132);
    drawCircleQuadrants(236, 43, 40, false, false, false, true, c141);

    drawCircleQuadrants(41, 41, 40, false, false, true, false, c132);
    drawCircleQuadrants(43, 43, 40, false, false, true, false, c141);

    drawCircleQuadrants(41, 198, 40, false, true, false, false, c132);
    drawCircleQuadrants(43, 196, 40, false, true, false, false, c141);

    drawCircleQuadrants(238, 198, 40, true, false, false, false, c132);
    drawCircleQuadrants(236, 196, 40, true, false, false, false, c141);

    drawText(&bigbug->gameData.sevenSegmentFont, c141, "bugnology", 10, 40);
    if (bigbug->gameData.radar.choices[0] == BIGBUG_REFILL_AMMO)
    {
        drawText(&bigbug->gameData.font, c301, "No radar upgrades available...", 10, 120);
    }
    else
    {
        drawText(&bigbug->gameData.font, c132, "Tune radar for:", 10, 120);
        drawText(&bigbug->gameData.font, c122, "(pick one)", 138, 120);
    }

    drawCircleFilledQuadrants(29, 166 + bigbug->gameData.radar.playerPingRadius * 20, 10, bb_randomInt(0, 8),
                              bb_randomInt(0, 1), bb_randomInt(-8, 1), bb_randomInt(-20, 1), c132);

    for (int i = 0; i < 3; i++)
    {
        if (bigbug->gameData.radar.choices[i] == -1)
        {
            break;
        }
        switch (bigbug->gameData.radar.choices[i])
        {
            case BIGBUG_GARBAGE_DENSITY:
            {
                drawText(&bigbug->gameData.font, i == bigbug->gameData.radar.playerPingRadius ? c141 : c132,
                         "garbage density", 45, 160 + i * 20);
                break;
            }
            case BIGBUG_INFINITE_RANGE:
            {
                drawText(&bigbug->gameData.font, i == bigbug->gameData.radar.playerPingRadius ? c141 : c132,
                         "infinite range", 45, 160 + i * 20);
                break;
            }
            case BIGBUG_FUEL:
            {
                drawText(&bigbug->gameData.font, i == bigbug->gameData.radar.playerPingRadius ? c141 : c132,
                         "fuel locations", 45, 160 + i * 20);
                break;
            }
            case BIGBUG_ENEMIES:
            {
                drawText(&bigbug->gameData.font, i == bigbug->gameData.radar.playerPingRadius ? c141 : c132,
                         "enemy locations", 45, 160 + i * 20);
                break;
            }
            case BIGBUG_ACTIVE_BOOSTER:
            {
                drawText(&bigbug->gameData.font, i == bigbug->gameData.radar.playerPingRadius ? c141 : c132,
                         "booster location", 45, 160 + i * 20);
                break;
            }
            case BIGBUG_OLD_BOOSTERS:
            {
                drawText(&bigbug->gameData.font, i == bigbug->gameData.radar.playerPingRadius ? c141 : c132,
                         "old booster locations", 45, 160 + i * 20);
                break;
            }
            case BIGBUG_POINTS_OF_INTEREST:
            {
                drawText(&bigbug->gameData.font, i == bigbug->gameData.radar.playerPingRadius ? c141 : c132,
                         "points of interest", 45, 160 + i * 20);
                break;
            }
            case BIGBUG_REFILL_AMMO:
            {
                drawText(&bigbug->gameData.font, i == bigbug->gameData.radar.playerPingRadius ? c141 : c132,
                         "refill ammo", 45, 160 + i * 20);
                break;
            }
            default:
            {
                break;
            }
        }
    }
}

/**
 * @brief This function is called periodically and frequently. It runs the actual game, including processing inputs,
 * physics updates and drawing to the display.
 *
 * @param elapsedUs The time that has elapsed since the last call to this function, in microseconds
 */
static void bb_GameLoop_Radar(int64_t elapsedUs)
{
    bigbug->gameData.btnDownState = 0b0;
    // Always process button events, regardless of control scheme, so the main menu button can be captured
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        // Print the current event
        // ESP_LOGD(BB_TAG,"state: %04X, button: %d, down: %s\n", evt.state, evt.button, evt.down ? "down" : "up");

        // Save the button state
        bigbug->gameData.btnState = evt.state;

        // Check if the pause button was pressed
        if (evt.down)
        {
            bigbug->gameData.btnDownState += evt.button;
            // PB_UP     = 0x0001, //!< The up button's bit
            // PB_DOWN   = 0x0002, //!< The down button's bit
            // PB_LEFT   = 0x0004, //!< The left button's bit
            // PB_RIGHT  = 0x0008, //!< The right button's bit
            // PB_A      = 0x0010, //!< The A button's bit
            // PB_B      = 0x0020, //!< The B button's bit
            // PB_START  = 0x0040, //!< The start button's bit
            // PB_SELECT = 0x0080, //!< The select button's bit
        }
    }

    if ((bigbug->gameData.radar.upgrades >> BIGBUG_INFINITE_RANGE) & 1)
    {
        if (bigbug->gameData.btnState & 1) // up
        {
            bigbug->gameData.radar.cam.y--;
        }
        if ((bigbug->gameData.btnState >> 1) & 1) // down
        {
            bigbug->gameData.radar.cam.y++;
        }
    }

    if ((bigbug->gameData.btnDownState >> 6) & 1) // start
    {
        bigbugMode.fnBackgroundDrawCallback = bb_BackgroundDrawCallback;
        bigbug->gameData.screen             = BIGBUG_GAME;
    }

    bigbug->gameData.radar.playerPingRadius += 3;

    bb_DrawScene_Radar();
}

static void bb_DrawScene_Garbotnik_Upgrade(void)
{
    // Draw the upgrade options
    drawText(&bigbug->gameData.font, c441, "Choose a swadge upgrade:", 45, 20);

    // vertical line always the same
    drawLineFast(29, 0, 29, 239, c331);

    // left side of a horizontal line
    drawLineFast(0, 65 + bigbug->gameData.radar.playerPingRadius * 30, 43,
                 65 + bigbug->gameData.radar.playerPingRadius * 30, c331);

    // a PLUS graphic
    drawRectFilled(21, 64 + bigbug->gameData.radar.playerPingRadius * 30, 38,
                   67 + bigbug->gameData.radar.playerPingRadius * 30, c545);
    drawRectFilled(28, 56 + bigbug->gameData.radar.playerPingRadius * 30, 31,
                   73 + bigbug->gameData.radar.playerPingRadius * 30, c545);

    uint16_t tWidth = 0;
    for (int i = 0; i < 3; i++)
    {
        if (bigbug->gameData.garbotnikUpgrade.choices[i] == -1)
        {
            break;
        }
        char upgradeText[32];
        char detailText[32];
        switch (bigbug->gameData.garbotnikUpgrade.choices[i])
        {
            case GARBOTNIK_REDUCED_FUEL_CONSUMPTION:
            {
                strcpy(upgradeText, "reduced fuel consumption");
                snprintf(detailText, sizeof(detailText), "%d -> %d", bigbug->gameData.GarbotnikStat_fuelConsumptionRate,
                         bigbug->gameData.GarbotnikStat_fuelConsumptionRate - 1);
                break;
            }
            case GARBOTNIK_FASTER_FIRE_RATE:
            {
                strcpy(upgradeText, "faster fire rate");
                snprintf(detailText, sizeof(detailText), "cooldown: %d -> %d", bigbug->gameData.GarbotnikStat_fireTime,
                         bigbug->gameData.GarbotnikStat_fireTime > 75 ? bigbug->gameData.GarbotnikStat_fireTime - 50
                                                                      : 25);
                break;
            }
            case GARBOTNIK_MORE_DIGGING_STRENGTH:
            {
                strcpy(upgradeText, "more digging strength");
                snprintf(detailText, sizeof(detailText), "%d -> %d", bigbug->gameData.GarbotnikStat_diggingStrength,
                         bigbug->gameData.GarbotnikStat_diggingStrength + 1);
                break;
            }
            case GARBOTNIK_MORE_TOW_CABLES:
            {
                strcpy(upgradeText, "more tow cables");
                snprintf(detailText, sizeof(detailText), "%d -> %d", bigbug->gameData.GarbotnikStat_maxTowCables,
                         bigbug->gameData.GarbotnikStat_maxTowCables + 2);
                break;
            }
            case GARBOTNIK_INCREASE_MAX_AMMO:
            {
                strcpy(upgradeText, "increase max ammo");
                snprintf(detailText, sizeof(detailText), "%d -> %d", bigbug->gameData.GarbotnikStat_maxHarpoons,
                         bigbug->gameData.GarbotnikStat_maxHarpoons + 25);
                break;
            }
            case GARBOTNIK_MORE_CHOICES:
            {
                strcpy(upgradeText, "more choices");
                strcpy(detailText, "3rd choice at future upgrades");
                break;
            }
            case GARBOTNIK_BUG_WHISPERER:
            {
                strcpy(upgradeText, "bug whisperer");
                strcpy(detailText, "towed bugs won't shoot back");
                break;
            }
            default:
            {
                break;
            }
        }
        if (i == bigbug->gameData.radar.playerPingRadius)
        {
            tWidth = textWidth(&bigbug->gameData.font, upgradeText);
        }
        drawText(&bigbug->gameData.font, i == bigbug->gameData.radar.playerPingRadius ? c515 : c304, upgradeText, 45,
                 60 + i * 30);
        drawText(&bigbug->gameData.font, i == bigbug->gameData.radar.playerPingRadius ? c225 : c113, detailText, 45,
                 72 + i * 30);
    }
    // draw the right side of the horizonal line
    drawLineFast(45 + tWidth + 2, 65 + bigbug->gameData.radar.playerPingRadius * 30, 279,
                 65 + bigbug->gameData.radar.playerPingRadius * 30, c331);
}

static void bb_DrawScene_Loadout_Select(void)
{
    uint8_t primingEffectY = 160 - log((double)(bigbug->gameData.loadoutScreenData->primingEffect - 1)) * 80;
    drawRectFilled(0, primingEffectY, TFT_WIDTH, primingEffectY + 10, c451);

    char donuts[15];
    uint8_t numDonuts = ((bb_rocketData_t*)bigbug->gameData.entityManager.activeBooster->data)->numDonuts;
    snprintf(donuts, sizeof(donuts), "%d donut%s", numDonuts, numDonuts == 1 ? "" : "s");
    uint8_t donutLeft
        = bigbug->gameData.entityManager.sprites[BB_DONUT].frames[0].w + 2 + textWidth(&bigbug->gameData.font, donuts);
    donutLeft = (TFT_WIDTH >> 1) - (donutLeft >> 1);
    drawWsgSimple(&bigbug->gameData.entityManager.sprites[BB_DONUT].frames[0], donutLeft, 4);
    drawText(&bigbug->gameData.font, numDonuts > 0 ? c555 : c500, donuts,
             donutLeft + bigbug->gameData.entityManager.sprites[BB_DONUT].frames[0].w + 2, 16);

    uint16_t tWidth = textWidth(&bigbug->gameData.sevenSegmentFont, "loadout");
    drawText(&bigbug->gameData.sevenSegmentFont, c225, "loadout", (TFT_WIDTH >> 1) - (tWidth >> 1), 30);

    drawLine(15, 84, 125, 84, bigbug->gameData.radar.playerPingRadius == 0 ? c550 : c222, 0);
    drawLine(15, 123, 125, 123, bigbug->gameData.radar.playerPingRadius == 0 ? c550 : c222, 0);
    drawLine(15, 84, 15, 123, bigbug->gameData.radar.playerPingRadius == 0 ? c550 : c222, 13);
    drawLine(125, 84, 125, 123, bigbug->gameData.radar.playerPingRadius == 0 ? c550 : c222, 13);
    if (bigbug->gameData.loadout.primaryWileIdx != 255)
    {
        drawText(&bigbug->gameData.font, bigbug->gameData.radar.playerPingRadius == 0 ? c555 : c333,
                 bigbug->gameData.loadout.allWiles[bigbug->gameData.loadout.primaryWileIdx].name, 20, 89);
        snprintf(donuts, sizeof(donuts), "cooldown: %ds",
                 bigbug->gameData.loadout.allWiles[bigbug->gameData.loadout.primaryWileIdx].cooldown);
        drawText(&bigbug->gameData.font, bigbug->gameData.radar.playerPingRadius == 0 ? c345 : c333, donuts, 20, 110);
    }
    else
    {
        tWidth = textWidth(&bigbug->gameData.font, "none");
        drawText(&bigbug->gameData.font, bigbug->gameData.radar.playerPingRadius == 0 ? c555 : c333, "none",
                 (TFT_WIDTH >> 2) - (tWidth >> 1), 98);
    }
    tWidth = textWidth(&bigbug->gameData.font, "primary wile");
    drawText(&bigbug->gameData.font, bigbug->gameData.radar.playerPingRadius == 0 ? c555 : c333, "primary wile",
             (TFT_WIDTH >> 2) - (tWidth >> 1), 126);

    drawLine(155, 84, 265, 84, bigbug->gameData.radar.playerPingRadius == 1 ? c550 : c222, 0);
    drawLine(155, 123, 265, 123, bigbug->gameData.radar.playerPingRadius == 1 ? c550 : c222, 0);
    drawLine(155, 84, 155, 123, bigbug->gameData.radar.playerPingRadius == 1 ? c550 : c222, 13);
    drawLine(265, 84, 265, 123, bigbug->gameData.radar.playerPingRadius == 1 ? c550 : c222, 13);
    if (bigbug->gameData.loadout.secondaryWileIdx != 255)
    {
        drawText(&bigbug->gameData.font, bigbug->gameData.radar.playerPingRadius == 1 ? c555 : c333,
                 bigbug->gameData.loadout.allWiles[bigbug->gameData.loadout.secondaryWileIdx].name, 160, 89);
        snprintf(donuts, sizeof(donuts), "cooldown: %ds",
                 bigbug->gameData.loadout.allWiles[bigbug->gameData.loadout.secondaryWileIdx].cooldown);
        drawText(&bigbug->gameData.font, bigbug->gameData.radar.playerPingRadius == 0 ? c345 : c333, donuts, 160, 110);
    }
    else
    {
        tWidth = textWidth(&bigbug->gameData.font, "none");
        drawText(&bigbug->gameData.font, bigbug->gameData.radar.playerPingRadius == 1 ? c555 : c333, "none",
                 (TFT_WIDTH >> 2) * 3 - (tWidth >> 1), 98);
    }
    tWidth = textWidth(&bigbug->gameData.font, "secondary wile");
    drawText(&bigbug->gameData.font, bigbug->gameData.radar.playerPingRadius == 1 ? c555 : c333, "secondary wile",
             (TFT_WIDTH >> 1) + (TFT_WIDTH >> 2) - (tWidth >> 1), 126);

    if (bigbug->gameData.loadoutScreenData->blinkTimer > 126 && bigbug->gameData.radar.playerPingRadius == 2)
    {
        drawTriangleOutlined(25, 188, 15, 178, 25, 168, c222, c555);
        drawTriangleOutlined(255, 188, 265, 178, 255, 168, c222, c555);
    }

    drawRectScaled(10, 50, 84, 70, bigbug->gameData.radar.playerPingRadius == 2 ? c550 : c222, 0, 0, 3, 3);

    drawText(&bigbug->gameData.font, bigbug->gameData.radar.playerPingRadius == 2 ? c555 : c333,
             bigbug->gameData.loadout.allWiles[bigbug->gameData.loadoutScreenData->selectedWile].name, 35, 155);

    if (bigbug->gameData.loadout.allWiles[bigbug->gameData.loadoutScreenData->selectedWile].purchased)
    {
        drawTextMarquee(&bigbug->gameData.font, bigbug->gameData.radar.playerPingRadius == 2 ? c325 : c225,
                        bigbug->gameData.loadout.allWiles[bigbug->gameData.loadoutScreenData->selectedWile].description,
                        35, 170, 247, &bigbug->gameData.loadoutScreenData->marqueeTimer);
        drawText(&bigbug->gameData.font, bigbug->gameData.radar.playerPingRadius == 2 ? c555 : c333, "hold", 35, 188);
        drawText(&bigbug->gameData.font, bigbug->gameData.radar.playerPingRadius == 2 ? c500 : c333, "B", 73, 188);
        drawText(&bigbug->gameData.font, bigbug->gameData.radar.playerPingRadius == 2 ? c555 : c333, "+", 86, 188);
        // draw the call sequence
        for (int i = 0; i < 5; i++)
        {
            enum bb_direction_t dir
                = bigbug->gameData.loadout.allWiles[bigbug->gameData.loadoutScreenData->selectedWile].callSequence[i];
            if (dir == BB_NONE)
            {
                break;
            }
            int32_t rotation = 180 + dir * 90;
            rotation         = rotation % 360;
            drawWsg(
                &bigbug->gameData.entityManager.sprites[BB_ARROW].frames[bigbug->gameData.radar.playerPingRadius == 2],
                96 + i * 30, 182, false, false, rotation);
        }
    }
    else
    {
        // draw lock
        drawTriangleOutlined(205, 155, 246, 155, 246, 196, c215, c215);
        drawCircleFilled(237, 164, 5, c000);
        drawCircleFilled(237, 164, 3, c215);
        drawRectFilled(238, 163, 243, 165, c215);
        drawRectFilled(229, 165, 245, 178, c000);
        drawRectFilled(236, 171, 238, 173, c215);

        snprintf(donuts, sizeof(donuts), "pay %d donut%s",
                 bigbug->gameData.loadout.allWiles[bigbug->gameData.loadoutScreenData->selectedWile].cost,
                 bigbug->gameData.loadout.allWiles[bigbug->gameData.loadoutScreenData->selectedWile].cost == 1 ? ""
                                                                                                               : "s");
        tWidth                 = textWidth(&bigbug->gameData.font, donuts);
        paletteColor_t textCol = c222;
        if (bigbug->gameData.radar.playerPingRadius == 2)
        {
            if (bigbug->gameData.loadout.allWiles[bigbug->gameData.loadoutScreenData->selectedWile].cost <= numDonuts)
            {
                textCol = c550;
            }
            else
            {
                textCol = c500;
            }
        }
        drawText(&bigbug->gameData.font, textCol, donuts, (TFT_WIDTH >> 1) - (tWidth >> 1), 180);
    }

    drawRectFilled(0, primingEffectY + 10, TFT_WIDTH, primingEffectY + 20, c451);
    drawRectFilled(0, primingEffectY + 20, TFT_WIDTH, TFT_HEIGHT, c151);

    drawRect(1, 1, 279, 239, c303);
    drawRect(3, 3, 277, 237, c440);
    drawCircleQuadrants(238, 41, 40, false, false, false, true, c303);
    drawCircleQuadrants(236, 43, 40, false, false, false, true, c440);

    drawCircleQuadrants(41, 41, 40, false, false, true, false, c303);
    drawCircleQuadrants(43, 43, 40, false, false, true, false, c440);

    drawCircleQuadrants(41, 198, 40, false, true, false, false, c303);
    drawCircleQuadrants(43, 196, 40, false, true, false, false, c440);

    drawCircleQuadrants(238, 198, 40, true, false, false, false, c303);
    drawCircleQuadrants(236, 196, 40, true, false, false, false, c440);

    drawRectFilled(65, 217, 215, 234, bigbug->gameData.radar.playerPingRadius == 3 ? c550 : c222);
    drawCircleFilled(65, 225, 8, bigbug->gameData.radar.playerPingRadius == 3 ? c550 : c222);
    drawCircleFilled(215, 225, 8, bigbug->gameData.radar.playerPingRadius == 3 ? c550 : c222);
    tWidth                 = textWidth(&bigbug->gameData.font, "prime the trash pod");
    paletteColor_t textCol = c000;
    if (bigbug->gameData.radar.playerPingRadius == 3 && bigbug->gameData.loadoutScreenData->primingEffect > 10
        && bb_randomInt(0, 1))
    {
        textCol = c303;
    }
    drawText(&bigbug->gameData.font, textCol, "prime the trash pod", (TFT_WIDTH >> 1) - (tWidth >> 1), 220);
}

static void bb_GameLoop_Garbotnik_Upgrade(int64_t elapsedUs)
{
    bigbug->gameData.btnDownState = 0b0;
    // Always process button events, regardless of control scheme, so the main menu button can be captured
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        // Save the button state
        bigbug->gameData.btnState = evt.state;

        if (evt.down)
        {
            bigbug->gameData.btnDownState += evt.button;
            if (evt.button == PB_UP)
            {
                bigbug->gameData.radar.playerPingRadius--; // using it as a selection idx in this screen to save space.
            }
            else if (evt.button == PB_DOWN)
            {
                bigbug->gameData.radar.playerPingRadius++; // using it as a selection idx in this screen to save space.
            }
            else if (evt.button == PB_A)
            {
                switch (bigbug->gameData.garbotnikUpgrade.choices[bigbug->gameData.radar.playerPingRadius])
                {
                    case GARBOTNIK_FASTER_FIRE_RATE:
                    {
                        bigbug->gameData.GarbotnikStat_fireTime = bigbug->gameData.GarbotnikStat_fireTime > 75
                                                                      ? bigbug->gameData.GarbotnikStat_fireTime - 50
                                                                      : 25;
                        if (bigbug->gameData.GarbotnikStat_fireTime == 25)
                        {
                            // fireTime is maxed out. Take it out of the pool.
                            bigbug->gameData.garbotnikUpgrade.upgrades
                                = bigbug->gameData.garbotnikUpgrade.upgrades | (1 << GARBOTNIK_FASTER_FIRE_RATE);
                        }
                        break;
                    }
                    case GARBOTNIK_MORE_DIGGING_STRENGTH:
                    {
                        bigbug->gameData.GarbotnikStat_diggingStrength++;
                        break;
                    }
                    case GARBOTNIK_REDUCED_FUEL_CONSUMPTION:
                    {
                        bigbug->gameData.GarbotnikStat_fuelConsumptionRate--;
                        break;
                    }
                    case GARBOTNIK_MORE_TOW_CABLES:
                    {
                        bigbug->gameData.GarbotnikStat_maxTowCables += 3;
                        break;
                    }
                    case GARBOTNIK_INCREASE_MAX_AMMO:
                    {
                        bigbug->gameData.GarbotnikStat_maxHarpoons += 50;
                        if (bigbug->gameData.GarbotnikStat_maxHarpoons == 250)
                        {
                            // max ammo is maxed out. Take it out of the pool.
                            bigbug->gameData.garbotnikUpgrade.upgrades
                                = bigbug->gameData.garbotnikUpgrade.upgrades | (1 << GARBOTNIK_INCREASE_MAX_AMMO);
                        }
                        break;
                    }
                    case GARBOTNIK_MORE_CHOICES:
                    {
                        bigbug->gameData.garbotnikUpgrade.upgrades
                            = bigbug->gameData.garbotnikUpgrade.upgrades | (1 << GARBOTNIK_MORE_CHOICES);
                        break;
                    }
                    case GARBOTNIK_BUG_WHISPERER:
                    {
                        bigbug->gameData.garbotnikUpgrade.upgrades
                            = bigbug->gameData.garbotnikUpgrade.upgrades | (1 << GARBOTNIK_BUG_WHISPERER);
                        break;
                    }
                    default:
                    {
                        break;
                    }
                }
                bigbugMode.fnBackgroundDrawCallback = bb_BackgroundDrawCallback;
                bigbug->gameData.screen             = BIGBUG_GAME;
            }
        }
    }

    // keep the selection wrapped in range of available choices.
    uint8_t numChoices
        = 1 + (uint8_t)(bigbug->gameData.radar.choices[1] > -1) + (uint8_t)(bigbug->gameData.radar.choices[2] > -1);
    bigbug->gameData.radar.playerPingRadius
        = (bigbug->gameData.radar.playerPingRadius % numChoices + numChoices) % numChoices;

    bb_DrawScene_Garbotnik_Upgrade();
}

static void bb_GameLoop_Loadout_Select(int64_t elapsedUs)
{
    bigbug->gameData.loadoutScreenData->blinkTimer += elapsedUs >> 12;
    if (bigbug->gameData.radar.playerPingRadius == 2)
    {
        bigbug->gameData.loadoutScreenData->marqueeTimer += elapsedUs;
    }
    bigbug->gameData.btnDownState = 0b0;
    // Always process button events, regardless of control scheme, so the main menu button can be captured
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        // Save the button state
        bigbug->gameData.btnState = evt.state;

        if (evt.down)
        {
            bigbug->gameData.btnDownState += evt.button;
            if (evt.button == PB_UP)
            {
                if (bigbug->gameData.radar.playerPingRadius == 1)
                {
                    bigbug->gameData.radar.playerPingRadius -= 2;
                }
                else
                {
                    bigbug->gameData.radar
                        .playerPingRadius--; // using it as a selection idx in this screen to save space.
                    if (bigbug->gameData.radar.playerPingRadius != 2)
                    {
                        bigbug->gameData.loadoutScreenData->marqueeTimer = 0;
                    }
                }
            }
            else if (evt.button == PB_DOWN)
            {
                if (bigbug->gameData.radar.playerPingRadius == 0)
                {
                    bigbug->gameData.radar.playerPingRadius += 2;
                }
                else
                {
                    bigbug->gameData.radar
                        .playerPingRadius++; // using it as a selection idx in this screen to save space.
                }
                if (bigbug->gameData.radar.playerPingRadius != 2)
                {
                    bigbug->gameData.loadoutScreenData->marqueeTimer = 0;
                }
            }
            else if (evt.button == PB_LEFT)
            {
                if (bigbug->gameData.radar.playerPingRadius == 1)
                {
                    bigbug->gameData.radar.playerPingRadius--;
                }
                else if (bigbug->gameData.radar.playerPingRadius == 2)
                {
                    bigbug->gameData.loadoutScreenData->selectedWile--;
                    if (bigbug->gameData.loadoutScreenData->selectedWile > 6)
                    {
                        bigbug->gameData.loadoutScreenData->selectedWile = 6;
                    }
                }
                bigbug->gameData.loadoutScreenData->marqueeTimer = 0;
            }
            else if (evt.button == PB_RIGHT)
            {
                if (bigbug->gameData.radar.playerPingRadius == 0)
                {
                    bigbug->gameData.radar.playerPingRadius++;
                }
                else if (bigbug->gameData.radar.playerPingRadius == 2)
                {
                    bigbug->gameData.loadoutScreenData->selectedWile++;
                    if (bigbug->gameData.loadoutScreenData->selectedWile > 6)
                    {
                        bigbug->gameData.loadoutScreenData->selectedWile = 0;
                    }
                }
                bigbug->gameData.loadoutScreenData->marqueeTimer = 0;
            }
            else if (evt.button == PB_A)
            {
                if (bigbug->gameData.radar.playerPingRadius == 0)
                {
                    if (bigbug->gameData.loadout.primaryWileIdx != 255)
                    {
                        bigbug->gameData.loadout.primaryWileIdx = 255;
                    }
                    else
                    {
                        bigbug->gameData.radar.playerPingRadius = 2;
                    }
                }
                else if (bigbug->gameData.radar.playerPingRadius == 1)
                {
                    if (bigbug->gameData.loadout.secondaryWileIdx != 255)
                    {
                        bigbug->gameData.loadout.secondaryWileIdx = 255;
                    }
                    else
                    {
                        bigbug->gameData.radar.playerPingRadius = 2;
                    }
                }
                else if (bigbug->gameData.radar.playerPingRadius == 2)
                {
                    if (!bigbug->gameData.loadout.allWiles[bigbug->gameData.loadoutScreenData->selectedWile].purchased)
                    {
                        if (bigbug->gameData.loadout.allWiles[bigbug->gameData.loadoutScreenData->selectedWile].cost
                            <= ((bb_rocketData_t*)bigbug->gameData.entityManager.activeBooster->data)->numDonuts)
                        {
                            ((bb_rocketData_t*)bigbug->gameData.entityManager.activeBooster->data)->numDonuts
                                -= bigbug->gameData.loadout.allWiles[bigbug->gameData.loadoutScreenData->selectedWile]
                                       .cost;
                            bigbug->gameData.loadout.allWiles[bigbug->gameData.loadoutScreenData->selectedWile]
                                .purchased
                                = true;
                        }
                    }
                    else
                    {
                        if (bigbug->gameData.loadout.primaryWileIdx == 255
                            && bigbug->gameData.loadout.secondaryWileIdx
                                   != bigbug->gameData.loadoutScreenData->selectedWile)
                        {
                            // nothing was selected yet
                            bigbug->gameData.loadout.primaryWileIdx = bigbug->gameData.loadoutScreenData->selectedWile;
                        }
                        else if (bigbug->gameData.loadout.secondaryWileIdx == 255
                                 && bigbug->gameData.loadout.primaryWileIdx
                                        != bigbug->gameData.loadoutScreenData->selectedWile)
                        {
                            // nothing was selected yet
                            bigbug->gameData.loadout.secondaryWileIdx
                                = bigbug->gameData.loadoutScreenData->selectedWile;
                        }
                    }
                }
            }
        }
    }

    // keep the selections wrapped in range of available choices.
    bigbug->gameData.radar.playerPingRadius = (bigbug->gameData.radar.playerPingRadius % 4 + 4) % 4;

    // if 3rd option is selected and 'a' button is held
    if (bigbug->gameData.radar.playerPingRadius == 3 && ((bigbug->gameData.btnState & PB_A) >> 4))
    {
        if (bigbug->gameData.loadoutScreenData->primingEffect >= 185)
        {
            heap_caps_free(bigbug->gameData.loadoutScreenData);
            bigbug->gameData.loadoutScreenData  = NULL;
            bigbugMode.fnBackgroundDrawCallback = bb_BackgroundDrawCallback;
            bigbug->gameData.screen             = BIGBUG_GAME;
            return;
        }
        bigbug->gameData.loadoutScreenData->primingEffect += elapsedUs >> 13;
        // printf("priming effect: %d\n",bigbug->gameData.loadoutScreenData->primingEffect);
    }

    if (bigbug->gameData.loadoutScreenData->primingEffect > 10)
    {
        bigbug->gameData.loadoutScreenData->primingEffect -= elapsedUs >> 14;
    }
    else
    {
        bigbug->gameData.loadoutScreenData->primingEffect = 10;
    }

    bb_DrawScene_Loadout_Select();
}

static void bb_GameLoop_Radar_Upgrade(int64_t elapsedUs)
{
    bigbug->gameData.btnDownState = 0b0;
    // Always process button events, regardless of control scheme, so the main menu button can be captured
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        // Print the current event
        // ESP_LOGD(BB_TAG,"state: %04X, button: %d, down: %s\n", evt.state, evt.button, evt.down ? "down" : "up");

        // Save the button state
        bigbug->gameData.btnState = evt.state;

        // Check if the pause button was pressed
        if (evt.down)
        {
            bigbug->gameData.btnDownState += evt.button;
            // PB_UP     = 0x0001, //!< The up button's bit
            // PB_DOWN   = 0x0002, //!< The down button's bit
            // PB_LEFT   = 0x0004, //!< The left button's bit
            // PB_RIGHT  = 0x0008, //!< The right button's bit
            // PB_A      = 0x0010, //!< The A button's bit
            // PB_B      = 0x0020, //!< The B button's bit
            // PB_START  = 0x0040, //!< The start button's bit
            // PB_SELECT = 0x0080, //!< The select button's bit
            if (evt.button == PB_UP)
            {
                bigbug->gameData.radar.playerPingRadius--; // using it as a selection idx in this screen to save space.
            }
            else if (evt.button == PB_DOWN)
            {
                bigbug->gameData.radar.playerPingRadius++; // using it as a selection idx in this screen to save space.
            }
            else if (evt.button == PB_A)
            {
                if (bigbug->gameData.radar.choices[bigbug->gameData.radar.playerPingRadius] == BIGBUG_REFILL_AMMO)
                {
                    if (bigbug->gameData.entityManager.playerEntity->dataType == GARBOTNIK_DATA)
                    {
                        bb_garbotnikData_t* gData
                            = (bb_garbotnikData_t*)bigbug->gameData.entityManager.playerEntity->data;
                        gData->numHarpoons = bigbug->gameData.GarbotnikStat_maxHarpoons;
                    }
                }
                else
                {
                    bigbug->gameData.radar.upgrades
                        += 1 << bigbug->gameData.radar.choices[bigbug->gameData.radar.playerPingRadius];
                }
                bigbugMode.fnBackgroundDrawCallback = bb_BackgroundDrawCallback;
                bigbug->gameData.screen             = BIGBUG_GAME;
            }
        }
    }

    // keep the selection wrapped in range of available choices.
    uint8_t numChoices = 1 + (uint8_t)(bigbug->gameData.radar.choices[1] > -1);
    bigbug->gameData.radar.playerPingRadius
        = (bigbug->gameData.radar.playerPingRadius % numChoices + numChoices) % numChoices;

    bb_DrawScene_Radar_Upgrade();
}

/**
 * @brief This function is called periodically and frequently. It runs the actual game, including processing inputs,
 * physics updates and drawing to the display.
 *
 * @param elapsedUs The time that has elapsed since the last call to this function, in microseconds
 */
static void bb_GameLoop(int64_t elapsedUs)
{
    if (bigbug->gameData.exit)
    {
        switchToSwadgeMode(&mainMenuMode);
        return;
    }
    bigbug->gameData.btnDownState = 0b0;
    // Always process button events, regardless of control scheme, so the main menu button can be captured
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        // Print the current event
        // ESP_LOGD(BB_TAG,"state: %04X, button: %d, down: %s\n", evt.state, evt.button, evt.down ? "down" : "up");

        // Save the button state
        bigbug->gameData.btnState = evt.state;

        // Check if the pause button was pressed
        if (evt.down)
        {
            bigbug->gameData.btnDownState += evt.button;
            // PB_UP     = 0x0001, //!< The up button's bit
            // PB_DOWN   = 0x0002, //!< The down button's bit
            // PB_LEFT   = 0x0004, //!< The left button's bit
            // PB_RIGHT  = 0x0008, //!< The right button's bit
            // PB_A      = 0x0010, //!< The A button's bit
            // PB_B      = 0x0020, //!< The B button's bit
            // PB_START  = 0x0040, //!< The start button's bit
            // PB_SELECT = 0x0080, //!< The select button's bit
            if (evt.button == PB_START && !bigbug->gameData.isPaused && bigbug->gameData.screen == BIGBUG_GAME
                && bigbug->gameData.entityManager.playerEntity != NULL)
            {
                bb_entity_t* radarPing
                    = bb_createEntity(&bigbug->gameData.entityManager, NO_ANIMATION, true, BB_RADAR_PING, 1,
                                      bigbug->gameData.entityManager.playerEntity->pos.x >> DECIMAL_BITS,
                                      bigbug->gameData.entityManager.playerEntity->pos.y >> DECIMAL_BITS, true, false);
                bigbug->gameData.screen    = BIGBUG_GAME_PINGING;
                bb_radarPingData_t* rpData = (bb_radarPingData_t*)radarPing->data;
                rpData->color              = c415;
                rpData->executeAfterPing   = &bb_openMap;
            }
        }
    }

    bb_updateEntities(&(bigbug->gameData.entityManager), &(bigbug->gameData.camera));

    // If the game is not paused, do game logic
    if (bigbug->gameData.isPaused == false)
    {
        bb_UpdateTileSupport();

        bb_UpdateLEDs(&(bigbug->gameData.entityManager));
        // bigbugFadeLeds(elapsedUs);
        // bigbugControlCpuPaddle();
    }

    bb_DrawScene();

    // ESP_LOGD(BB_TAG,"FPS: %ld\n", 1000000 / elapsedUs);
}

static void bb_Reset(void)
{
    ESP_LOGD(BB_TAG, "The width is: %d\n", FIELD_WIDTH);
    ESP_LOGD(BB_TAG, "The height is: %d\n", FIELD_HEIGHT);

    bigbug->gameData.camera.camera.width  = FIELD_WIDTH;
    bigbug->gameData.camera.camera.height = FIELD_HEIGHT;
}

/**
 * @brief Set the LEDs
 */
static void bb_SetLeds(void)
{
    // Create an array for all LEDs
    led_t leds[CONFIG_NUM_LEDS];
    // Copy the LED colors for left and right to the whole array
    for (uint8_t i = 0; i < CONFIG_NUM_LEDS; i++)
    {
        leds[i].r = 0;
        leds[i].g = 0;
        leds[i].b = 0;
    }
    // Set the LED output
    setLeds(leds, CONFIG_NUM_LEDS);
}

/**
 * @brief Finds unsupported dirt over many frames and crumbles it.
 */
static void bb_UpdateTileSupport(void)
{
    while (bigbug->gameData.pleaseCheck.first != NULL)
    {
        uint8_t* shiftedVal = (uint8_t*)shift(&bigbug->gameData.pleaseCheck);
        if ((shiftedVal[2] ? bigbug->gameData.tilemap.fgTiles[shiftedVal[0]][shiftedVal[1]].health
                           : bigbug->gameData.tilemap.mgTiles[shiftedVal[0]][shiftedVal[1]].health)
            > 0)
        {
            // pathfind
            if (!(shiftedVal[2] ? pathfindToPerimeter(
                      (bb_midgroundTileInfo_t*)&bigbug->gameData.tilemap.fgTiles[shiftedVal[0]][shiftedVal[1]],
                      &bigbug->gameData.tilemap)
                                : pathfindToPerimeter(&bigbug->gameData.tilemap.mgTiles[shiftedVal[0]][shiftedVal[1]],
                                                      &bigbug->gameData.tilemap)))
            {
                // trigger a cascading collapse
                uint8_t* val = heap_caps_calloc(3, sizeof(uint8_t), MALLOC_CAP_SPIRAM);
                memcpy(val, shiftedVal, 3 * sizeof(uint8_t));
                push(&bigbug->gameData.unsupported, (void*)val);
            }
            heap_caps_free(shiftedVal);
            break;
        }
        heap_caps_free(shiftedVal);
    }

    if (bigbug->gameData.unsupported.first != NULL
        && bb_randomInt(1, 4) == 1) // making it happen randomly slowly makes crumble sound and looks nicer.
    {
        for (int i = 0; i < 50; i++) // arbitrarily large loop to get to the dirt tiles.
        {
            if (bigbug->gameData.unsupported.first == NULL)
            {
                break;
            }
            // remove the first item from the list
            uint8_t* shiftedVal = (uint8_t*)shift(&bigbug->gameData.unsupported);
            // check that it's still dirt, because a previous pass may have crumbled it.
            if ((shiftedVal[2] ? bigbug->gameData.tilemap.fgTiles[shiftedVal[0]][shiftedVal[1]].health
                               : bigbug->gameData.tilemap.mgTiles[shiftedVal[0]][shiftedVal[1]].health)
                > 0)
            {
                // set it to air
                if (shiftedVal[2])
                {
                    bigbug->gameData.tilemap.fgTiles[shiftedVal[0]][shiftedVal[1]].health = 0;
                }
                else
                {
                    bigbug->gameData.tilemap.mgTiles[shiftedVal[0]][shiftedVal[1]].health = 0;
                }

                if (bigbug->gameData.entityManager.activeEntities < MAX_ENTITIES)
                {
                    // create a crumble animation
                    bb_crumbleDirt(&bigbug->gameData, bb_randomInt(2, 5), shiftedVal[0], shiftedVal[1], true, false);
                }

                // queue neighbors for crumbling
                for (uint8_t neighborIdx = 0; neighborIdx < 4;
                     neighborIdx++) // neighborIdx 0 thru 3 is Left, Up, Right, Down
                {
                    if ((uint8_t)shiftedVal[0] + bigbug->gameData.neighbors[neighborIdx][0] >= 0
                        && (uint8_t)shiftedVal[0] + bigbug->gameData.neighbors[neighborIdx][0] < TILE_FIELD_WIDTH
                        && (uint8_t)shiftedVal[1] + bigbug->gameData.neighbors[neighborIdx][1] >= 0
                        && (uint8_t)shiftedVal[1] + bigbug->gameData.neighbors[neighborIdx][1] < TILE_FIELD_HEIGHT)
                    {
                        // TODO where is this heap_caps_free()'d?
                        // should be done now.
                        uint8_t* val = heap_caps_calloc(3, sizeof(uint8_t), MALLOC_CAP_SPIRAM);
                        val[0]       = shiftedVal[0] + bigbug->gameData.neighbors[neighborIdx][0];
                        val[1]       = shiftedVal[1] + bigbug->gameData.neighbors[neighborIdx][1];
                        val[2]       = shiftedVal[2];
                        push(&bigbug->gameData.unsupported, (void*)val);
                    }
                }
                uint8_t* val = heap_caps_calloc(3, sizeof(uint8_t), MALLOC_CAP_SPIRAM);
                val[0]       = shiftedVal[0];
                val[1]       = shiftedVal[1];
                val[2]       = !shiftedVal[2];
                push(&bigbug->gameData.unsupported, (void*)val);

                heap_caps_free(shiftedVal);
                break;
            }
            heap_caps_free(shiftedVal);
        }
    }
}

static void bb_UpdateLEDs(bb_entityManager_t* entityManager)
{
    if ((entityManager->playerEntity != NULL) && (GARBOTNIK_DATA == entityManager->playerEntity->dataType))
    {
        const uint8_t condimentIndices[] = {5, 4, 6, 7, 0, 8};
        int32_t fuel                     = ((bb_garbotnikData_t*)entityManager->playerEntity->data)->fuel;
        uint8_t colorStyle               = fuel / 60000;

        // Set the LEDs to display fuel level
        // ESP_LOGD(BB_TAG,"timer %d\n", fuel);
        led_t leds[CONFIG_NUM_LEDS] = {0};
        int32_t ledChunk            = 60000 / 6;
        for (uint8_t i = 0; i < 6; i++)
        {
            if (fuel - colorStyle * 60000 >= (i + 1) * ledChunk)
            {
                switch (colorStyle)
                {
                    case 0:
                    {
                        leds[condimentIndices[i]].r = 255;
                        leds[condimentIndices[i]].g = 0;
                        break;
                    }
                    case 1:
                    {
                        leds[condimentIndices[i]].r = 255;
                        leds[condimentIndices[i]].g = 255;
                        break;
                    }
                    default: // 2
                    {
                        leds[condimentIndices[i]].r = 0;
                        leds[condimentIndices[i]].g = 255;
                        break;
                    }
                }
            }
            else if (fuel - colorStyle * 60000 < i * ledChunk)
            {
                switch (colorStyle)
                {
                    case 0:
                    {
                        leds[condimentIndices[i]].r = 0;
                        leds[condimentIndices[i]].g = 0;
                        break;
                    }
                    case 1:
                    {
                        leds[condimentIndices[i]].r = 255;
                        leds[condimentIndices[i]].g = 0;
                        break;
                    }
                    default: // 2
                    {
                        leds[condimentIndices[i]].r = 255;
                        leds[condimentIndices[i]].g = 255;
                        break;
                    }
                }
            }
            else
            {
                switch (colorStyle)
                {
                    case 0:
                    {
                        leds[condimentIndices[i]].r = ((fuel - i * ledChunk) * 255) / ledChunk;
                        leds[condimentIndices[i]].g = 0;
                        break;
                    }
                    case 1:
                    {
                        leds[condimentIndices[i]].r = 255;
                        leds[condimentIndices[i]].g = ((fuel - 60000 - i * ledChunk) * 255) / ledChunk;
                        break;
                    }
                    default: // 2
                    {
                        leds[condimentIndices[i]].r = 255 - ((fuel - 120000 - i * ledChunk) * 255) / ledChunk;
                        leds[condimentIndices[i]].g = 255;
                        break;
                    }
                }
            }
            leds[condimentIndices[i]].b = 0;
        }

        for (int topLED = 1; topLED < 4; topLED++)
        {
            if (fuel < 20000)
            {
                leds[topLED].r = (fuel % 500) * 51 / 100;
                if ((fuel / 500) % 2 == 1)
                {
                    leds[topLED].r = 255 - leds[topLED].r;
                }
            }
            else if (fuel < 60000)
            {
                leds[topLED].r = 0;
            }
        }

        setLeds(leds, CONFIG_NUM_LEDS);
    }
    else
    {
        led_t leds[CONFIG_NUM_LEDS] = {0};
        setLeds(leds, CONFIG_NUM_LEDS);
    }
}
