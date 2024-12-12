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
    font_t font;  ///< The font used in the menu and game
    // bb_screen_t screen; ///< The screen being displayed

    bb_gameData_t gameData;

    led_t ledL;           ///< The left LED color
    led_t ledR;           ///< The right LED color
    int32_t ledFadeTimer; ///< The timer to fade LEDs
};

//==============================================================================
// Function Prototypes
//==============================================================================

// required by adam
static void bb_EnterMode(void);
static void bb_EnterModeSkipIntro(void);
static void bb_ExitMode(void);
static void bb_MainLoop(int64_t elapsedUs);
static void bb_BackgroundDrawCallbackRadar(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);
static void bb_BackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);

// big bug logic
// static void bb_LoadScreenDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);
static void bb_DrawScene(void);
static void bb_DrawScene_Radar(void);
static void bb_DrawScene_Radar_Upgrade(void);
static void bb_GameLoop_Radar(int64_t elapsedUs);
static void bb_GameLoop_Radar_Upgrade(int64_t elapsedUs);
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
                           .fnEnterMode              = bb_EnterModeSkipIntro,
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
    bb_hsd         = heatshrink_decoder_alloc(256, 8, 4);
    bb_decodeSpace = heap_caps_malloc(102404, MALLOC_CAP_SPIRAM);

    bb_SetLeds();

    // Load font
    loadFont("ibm_vga8.font", &bigbug->font, false);

    const char loadingStr[] = "Loading...";
    int32_t tWidth          = textWidth(&bigbug->font, loadingStr);
    drawText(&bigbug->font, c542, loadingStr, (TFT_WIDTH - tWidth) / 2, (TFT_HEIGHT - bigbug->font.height) / 2);
    drawDisplayTft(NULL);

    bb_initializeGameData(&bigbug->gameData);
    bb_initializeEntityManager(&bigbug->gameData.entityManager, &bigbug->gameData);

    // bb_createEntity(&(bigbug->gameData.entityManager), LOOPING_ANIMATION, true, ROCKET_ANIM, 3,
    //                 (TILE_FIELD_WIDTH / 2) * TILE_SIZE + HALF_TILE + 1, -1000, true);

    bb_entity_t* foreground       = bb_createEntity(&(bigbug->gameData.entityManager), NO_ANIMATION, true, BB_MENU, 1,
                                                    (TILE_FIELD_WIDTH / 2) * TILE_SIZE + HALF_TILE - 1, -5146, true, false);
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
    bb_hsd         = heatshrink_decoder_alloc(256, 8, 4);
    bb_decodeSpace = heap_caps_malloc(102404, MALLOC_CAP_SPIRAM);

    bb_SetLeds();

    // Load font
    loadFont("ibm_vga8.font", &bigbug->font, false);

    const char loadingStr[] = "Loading...";
    int32_t tWidth          = textWidth(&bigbug->font, loadingStr);
    drawText(&bigbug->font, c542, loadingStr, (TFT_WIDTH - tWidth) / 2, (TFT_HEIGHT - bigbug->font.height) / 2);
    drawDisplayTft(NULL);

    bb_initializeGameData(&bigbug->gameData);
    bb_initializeEntityManager(&bigbug->gameData.entityManager, &bigbug->gameData);

    // create the death dumpster
    bigbug->gameData.entityManager.deathDumpster
        = bb_createEntity(&bigbug->gameData.entityManager, NO_ANIMATION, true, BB_DEATH_DUMPSTER, 1,
                          (TILE_FIELD_WIDTH / 2) * TILE_SIZE + HALF_TILE - 1, -4746, true, false);

    // create 3 rockets
    for (int rocketIdx = 0; rocketIdx < 3; rocketIdx++)
    {
        bigbug->gameData.entityManager.boosterEntities[rocketIdx] = bb_createEntity(
            &bigbug->gameData.entityManager, NO_ANIMATION, true, ROCKET_ANIM, 8,
            (bigbug->gameData.entityManager.deathDumpster->pos.x >> DECIMAL_BITS) - 96 + 96 * rocketIdx,
            (bigbug->gameData.entityManager.deathDumpster->pos.y >> DECIMAL_BITS) + 375, true, false);

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
            bigbug->gameData.entityManager.activeBooster->currentAnimationFrame = 40;
            bigbug->gameData.entityManager.activeBooster->pos.y                 = 50;
            bigbug->gameData.entityManager.activeBooster->updateFunction        = bb_updateHeavyFalling;
            bb_entity_t* arm                                                    = bb_createEntity(
                &bigbug->gameData.entityManager, NO_ANIMATION, true, ATTACHMENT_ARM, 1,
                bigbug->gameData.entityManager.activeBooster->pos.x >> DECIMAL_BITS,
                (bigbug->gameData.entityManager.activeBooster->pos.y >> DECIMAL_BITS) - 33, false, false);
            ((bb_attachmentArmData_t*)arm->data)->rocket = bigbug->gameData.entityManager.activeBooster;

            bb_entity_t* grabbyHand = bb_createEntity(
                &bigbug->gameData.entityManager, LOOPING_ANIMATION, true, BB_GRABBY_HAND, 5,
                bigbug->gameData.entityManager.activeBooster->pos.x >> DECIMAL_BITS,
                (bigbug->gameData.entityManager.activeBooster->pos.y >> DECIMAL_BITS) - 53, false, false);
            ((bb_grabbyHandData_t*)grabbyHand->data)->rocket = bigbug->gameData.entityManager.activeBooster;
        }
    }

    bb_loadWsgs(&bigbug->gameData.tilemap);

    bigbug->gameData.entityManager.viewEntity
        = bb_createEntity(&(bigbug->gameData.entityManager), NO_ANIMATION, true, GARBOTNIK_FLYING, 1,
                          bigbug->gameData.entityManager.activeBooster->pos.x >> DECIMAL_BITS,
                          (bigbug->gameData.entityManager.activeBooster->pos.y >> DECIMAL_BITS) - 90, true, false);

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

static void bb_ExitMode(void)
{
    heatshrink_decoder_free(bb_hsd);
    heap_caps_free(bb_decodeSpace);

    // Destroy menu bug, just in case
    bb_destroyEntity(bigbug->gameData.menuBug, false);

    bb_freeGameData(&bigbug->gameData);
    bb_deactivateAllEntities(&bigbug->gameData.entityManager, false);
    // Free entity manager
    bb_freeEntityManager(&bigbug->gameData.entityManager);
    // Free font
    freeFont(&bigbug->font);

    soundStop(true);
    unloadMidiFile(&bigbug->gameData.bgm);

    deinitGlobalMidiPlayer();

    bb_freeWsgs(&bigbug->gameData.tilemap);

    for (int32_t w = 0; w < TILE_FIELD_WIDTH; w++)
    {
        heap_caps_free(bigbug->gameData.tilemap.fgTiles[w]);
        heap_caps_free(bigbug->gameData.tilemap.mgTiles[w]);
    }

    heap_caps_free(bigbug);
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
                bigbugMode.fnBackgroundDrawCallback = bb_BackgroundDrawCallbackRadar;
            }
            bb_GameLoop_Radar(elapsedUs);
            break;
        }
        case BIGBUG_RADAR_UPGRADE_SCREEN:
        {
            if (bigbugMode.fnBackgroundDrawCallback == bb_BackgroundDrawCallback)
            {
                bigbugMode.fnBackgroundDrawCallback = bb_BackgroundDrawCallbackRadar;
            }
            bb_GameLoop_Radar_Upgrade(elapsedUs);
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

static void bb_BackgroundDrawCallbackRadar(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
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
        // get steps from 0 (at -4000) to 14 (at -1200)
        int32_t skyStep = cameraPos->y / 200 + 20;
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
        int32_t offsetX2 = ((cameraPos->x / 3) % bgWidth);
        int32_t offsetY2 = -(cameraPos->y / 3) - 64;

        // Calculate nearer background offset
        int32_t offsetX1 = ((cameraPos->x / 2) % bgWidth);
        int32_t offsetY1 = -(cameraPos->y / 2);

        // Calculate gradient offset (under nearer background)
        int32_t offsetXG = offsetX1 % grWidth;
        int32_t offsetYG = offsetY1 + tilemap->surface1Wsg.h;

        // setup fast pixel drawing with TURBO_SET_PIXEL()
        SETUP_FOR_TURBO();

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

            // Start indices at X offsets
            int32_t s1idx = offsetX1;
            int32_t s2idx = offsetX2;
            int32_t gridx = offsetXG;

            // For each pixel in the row
            for (int32_t iX = x; iX < x + w; iX++)
            {
                // Draw appropriate pixel
                if (drawS1row && cTransparent != surface1px[s1idx])
                {
                    TURBO_SET_PIXEL(iX, iY, surface1px[s1idx]);
                }
                else if (drawGrRow) // No transparency check
                {
                    TURBO_SET_PIXEL(iX, iY, gradientPx[gridx]);
                }
                else if (drawS2row) // No transparency check
                {
                    TURBO_SET_PIXEL(iX, iY, surface2px[s2idx]);
                }

                // Increment with modulo
                s1idx = (s1idx + 1) % bgWidth;
                s2idx = (s2idx + 1) % bgWidth;
                gridx = (gridx + 1) % grWidth;
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
    DRAW_FPS_COUNTER(bigbug->font);
}

static void bb_DrawScene_Radar(void)
{
    for (int xIdx = 1; xIdx < TILE_FIELD_WIDTH - 3; xIdx++)
    {
        for (int yIdx = bigbug->gameData.radar.cam.y / 4 < 0 ? 0 : bigbug->gameData.radar.cam.y / 4;
             yIdx < (bigbug->gameData.radar.cam.y / 4 < 0 ? FIELD_HEIGHT / 4
                                                          : bigbug->gameData.radar.cam.y / 4 + FIELD_HEIGHT / 4);
             yIdx++)
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
    // garbotnik
    vec_t garbotnikPos = (vec_t){(bigbug->gameData.entityManager.playerEntity->pos.x >> DECIMAL_BITS) / 8 - 3,
                                 (bigbug->gameData.entityManager.playerEntity->pos.y >> DECIMAL_BITS) / 8 - 3};

    drawCircleFilled(garbotnikPos.x, garbotnikPos.y - bigbug->gameData.radar.cam.y, 3, c515);
    // garbotnik pings
    for (int i = 0; i < 254; i += 51)
    {
        bigbug->gameData.radar.playerPingRadius += i;
        drawCircle(garbotnikPos.x, garbotnikPos.y - bigbug->gameData.radar.cam.y,
                   bigbug->gameData.radar.playerPingRadius, c404);
    }

    if ((bigbug->gameData.radar.upgrades >> BIGBUG_OLD_BOOSTERS) & 1)
    {
        for (int boosterIdx = 0; boosterIdx < 4; boosterIdx++)
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
                               garbotnikPos.y + 8 - bigbug->gameData.radar.cam.y, c430);
            }
        }
    }

    if ((bigbug->gameData.radar.upgrades >> BIGBUG_ACTIVE_BOOSTER) & 1)
    {
        // booster radar rect
        garbotnikPos.x = (bigbug->gameData.entityManager.activeBooster->pos.x >> DECIMAL_BITS) / 8 - 4;
        garbotnikPos.y = (bigbug->gameData.entityManager.activeBooster->pos.y >> DECIMAL_BITS) / 8 - 4;
        drawRectFilled(garbotnikPos.x, garbotnikPos.y - bigbug->gameData.radar.cam.y, garbotnikPos.x + 2,
                       garbotnikPos.y + 8 - bigbug->gameData.radar.cam.y, c250);
    }

    if ((bigbug->gameData.radar.upgrades >> BIGBUG_INFINITE_RANGE) & 1)
    {
        if ((bigbug->gameData.radar.playerPingRadius % 51) < 26)
        {
            drawTriangleOutlined(140, 2, 148, 10, 132, 10, c222, c555);
            drawTriangleOutlined(132, 230, 148, 230, 140, 238, c222, c555);
        }
    }

    DRAW_FPS_COUNTER(bigbug->font);
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

    drawText(&bigbug->gameData.sevenSegment, c141, "bugnology", 10, 40);
    if (bigbug->gameData.radar.choices[0] == BIGBUG_REFILL_AMMO)
    {
        drawText(&bigbug->gameData.font, c301, "No radar upgrades available...", 10, 120);
    }
    else
    {
        drawText(&bigbug->gameData.font, c132, "Tune radar for:", 10, 120);
        drawText(&bigbug->gameData.font, c122, "(pick one)", 138, 120);
    }

    drawCircleFilledQuadrants(29, 166 + bigbug->gameData.radar.playerPingRadius * 30, 10, bb_randomInt(0, 8),
                              bb_randomInt(0, 1), bb_randomInt(-8, 1), bb_randomInt(-20, 1), c132);

    for (int i = 0; i < 2; i++)
    {
        switch (bigbug->gameData.radar.choices[i])
        {
            case BIGBUG_GARBAGE_DENSITY:
                drawText(&bigbug->gameData.font, i == bigbug->gameData.radar.playerPingRadius ? c141 : c132,
                         "garbage density", 45, 160 + i * 30);
                break;
            case BIGBUG_INFINITE_RANGE:
                drawText(&bigbug->gameData.font, i == bigbug->gameData.radar.playerPingRadius ? c141 : c132,
                         "infinite range", 45, 160 + i * 30);
                break;
            case BIGBUG_FUEL:
                drawText(&bigbug->gameData.font, i == bigbug->gameData.radar.playerPingRadius ? c141 : c132,
                         "fuel locations", 45, 160 + i * 30);
                drawText(&bigbug->gameData.font, c122, "not implemented", 45, 172 + i * 30);
                break;
            case BIGBUG_ENEMIES:
                drawText(&bigbug->gameData.font, i == bigbug->gameData.radar.playerPingRadius ? c141 : c132,
                         "enemy locations", 45, 160 + i * 30);
                drawText(&bigbug->gameData.font, c122, "not implemented", 45, 172 + i * 30);
                break;
            case BIGBUG_ACTIVE_BOOSTER:
                drawText(&bigbug->gameData.font, i == bigbug->gameData.radar.playerPingRadius ? c141 : c132,
                         "booster location", 45, 160 + i * 30);
                break;
            case BIGBUG_OLD_BOOSTERS:
                drawText(&bigbug->gameData.font, i == bigbug->gameData.radar.playerPingRadius ? c141 : c132,
                         "old booster locations", 45, 160 + i * 30);
                break;
            case BIGBUG_POINTS_OF_INTEREST:
                drawText(&bigbug->gameData.font, i == bigbug->gameData.radar.playerPingRadius ? c141 : c132,
                         "more points of interest", 45, 160 + i * 30);
                drawText(&bigbug->gameData.font, c122, "not implemented", 45, 172 + i * 30);
                break;
            case BIGBUG_REFILL_AMMO:
                drawText(&bigbug->gameData.font, i == bigbug->gameData.radar.playerPingRadius ? c141 : c132,
                         "refill ammo", 45, 160 + i * 30);
                break;
            default:
                break;
        }
    }

    // no upgrades left...
    // refill ammo
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
                        gData->numHarpoons = 250;
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
        bb_ExitMode();
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
    for (uint8_t i = 0; i < CONFIG_NUM_LEDS / 2; i++)
    {
        leds[i]                         = bigbug->ledL;
        leds[i + (CONFIG_NUM_LEDS / 2)] = bigbug->ledR;
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

                // create a crumble animation
                bb_crumbleDirt(&bigbug->gameData.entityManager.entities[MAX_ENTITIES-1], bb_randomInt(2, 5), shiftedVal[0],
                               shiftedVal[1], true);

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
        int32_t fuel = ((bb_garbotnikData_t*)entityManager->playerEntity->data)->fuel;
        // Set the LEDs to a display fuel level
        // ESP_LOGD(BB_TAG,"timer %d\n", fuel);
        led_t leds[CONFIG_NUM_LEDS] = {0};
        int32_t ledChunk            = 180000 / CONFIG_NUM_LEDS;
        for (uint8_t i = 0; i < CONFIG_NUM_LEDS; i++)
        {
            leds[i].r = 0;
            if (fuel >= (i + 1) * ledChunk)
            {
                leds[i].g = 255;
            }
            else if (fuel < i * ledChunk)
            {
                leds[i].g = 0;
            }
            else
            {
                leds[i].g = ((fuel - i * ledChunk) * 255) / ledChunk;
            }
            leds[i].b = 0;
        }
        setLeds(leds, CONFIG_NUM_LEDS);
    }
}
