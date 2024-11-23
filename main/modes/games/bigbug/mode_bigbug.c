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

/**
 * @brief Enum of screens that may be shown in bigbug mode
 */
typedef enum
{
    BIGBUG_MENU,
    BIGBUG_GAME,
} bb_screen_t;

//==============================================================================
// Structs
//==============================================================================

struct bb_t
{
    menu_t* menu;       ///< The menu structure
    font_t font;        ///< The font used in the menu and game
    bb_screen_t screen; ///< The screen being displayed

    bb_gameData_t gameData;
    bb_soundManager_t soundManager;

    led_t ledL;           ///< The left LED color
    led_t ledR;           ///< The right LED color
    int32_t ledFadeTimer; ///< The timer to fade LEDs
};

//==============================================================================
// Function Prototypes
//==============================================================================

// required by adam
static void bb_EnterMode(void);
static void bb_ExitMode(void);
static void bb_MainLoop(int64_t elapsedUs);
static void bb_BackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);

// big bug logic
// static void bb_LoadScreenDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);
static void bb_DrawScene(void);
static void bb_GameLoop(int64_t elapsedUs);
static void bb_Reset(void);
static void bb_SetLeds(void);
static void bb_UpdateTileSupport(void);
static void bb_UpdateLEDs(bb_entityManager_t* entityManager);
static void bb_GarbotniksHomeMusicCb(void);

//==============================================================================
// Strings
//==============================================================================

/* Design Pattern!
 * These strings are all declared 'const' because they do not change, so that they are placed in ROM, not RAM.
 * Lengths are not explicitly given so the compiler can figure it out.
 */
static const char bigbugName[] = "Big Bug";

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
                           .fnEnterMode              = bb_EnterMode,
                           .fnExitMode               = bb_ExitMode,
                           .fnMainLoop               = bb_MainLoop,
                           .fnBackgroundDrawCallback = bb_BackgroundDrawCallback,
                           .fnDacCb                  = NULL};

/// All state information for bigbug mode. This whole struct is calloc()'d and FREE_DBG()'d so that bigbug is only
/// using memory while it is being played
bb_t* bigbug = NULL;

//==============================================================================
// Required Functions
//==============================================================================

static void bb_EnterMode(void)
{
    setFrameRateUs(16667); // 60 FPS

    // Force draw a loading screen
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c123);

    bigbug = HEAP_CAPS_CALLOC_DBG(1, sizeof(bb_t), MALLOC_CAP_SPIRAM);

    bb_SetLeds();

    // Load font
    loadFont("ibm_vga8.font", &bigbug->font, false);


    const char loadingStr[] = "Loading...";
    int32_t tWidth          = textWidth(&bigbug->font, loadingStr);
    drawText(&bigbug->font, c542, loadingStr, (TFT_WIDTH - tWidth) / 2, (TFT_HEIGHT - bigbug->font.height) / 2);
    drawDisplayTft(NULL);

    bb_initializeGameData(&bigbug->gameData, &bigbug->soundManager);
    bb_initializeEntityManager(&bigbug->gameData.entityManager, &bigbug->gameData, &bigbug->soundManager);
    bb_initializeTileMap(&bigbug->gameData.tilemap);

    // bb_createEntity(&(bigbug->gameData.entityManager), LOOPING_ANIMATION, true, ROCKET_ANIM, 3,
    //                 (TILE_FIELD_WIDTH / 2) * TILE_SIZE + HALF_TILE + 1, -1000, true);

    bb_entity_t* foreground    = bb_createEntity(&(bigbug->gameData.entityManager), NO_ANIMATION, true, BB_MENU, 1,
                                                 (TILE_FIELD_WIDTH / 2) * TILE_SIZE + HALF_TILE - 1, -5146, true, false);

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

    bb_initializeEggs(&(bigbug->gameData.entityManager), &(bigbug->gameData.tilemap));

    // Player
    //  bb_createEntity(&(bigbug->gameData.entityManager), NO_ANIMATION, true, GARBOTNIK_FLYING, 1,
    //          5 * 32 + 16,
    //          -110);

    // Set the mode to game mode
    bigbug->screen = BIGBUG_GAME;

    //play the music!
    midiPlayer_t* player = globalMidiPlayerGet(MIDI_BGM);
    midiGmOn(player);
    midiPlayer_t* sfx = globalMidiPlayerGet(MIDI_SFX);
    midiGmOn(sfx);
    soundPlayBgmCb(&bigbug->gameData.garbotniksHome, MIDI_BGM, bb_GarbotniksHomeMusicCb);
    midiSetProgram(player, 12, 90);//talking sound effects arbitrarily go on channel 12 and use midi instrument 90.
    midiControlChange(player, 12, MCC_SUSTENUTO_PEDAL, 80);
    midiControlChange(player, 12, MCC_SOUND_RELEASE_TIME , 60);

    // midiPlayer_t* sfx = globalMidiPlayerGet(MIDI_SFX);
    // // Turn on the sustain pedal for channel 1
    // midiSustain(sfx, 2, 0x7F);
    // midiSetProgram(sfx, 0, 0);


    bb_Reset();
}

static void bb_ExitMode(void)
{
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
    
    unloadMidiFile(&bigbug->gameData.hurryUp);

    unloadMidiFile(&bigbug->gameData.garbotniksHome);
    deinitGlobalMidiPlayer();

    bb_freeWsgs(&bigbug->gameData.tilemap);

    FREE_DBG(bigbug);
}

static void bb_MainLoop(int64_t elapsedUs)
{
    // Save the elapsed time
    bigbug->gameData.elapsedUs = elapsedUs;

    // Pick what runs and draws depending on the screen being displayed
    switch (bigbug->screen)
    {
        case BIGBUG_MENU:
        {
            break;
        }
        case BIGBUG_GAME:
        {
            // Run the main game loop. This will also process button events
            bb_GameLoop(elapsedUs);
            break;
        }
        default:
        {
            break;
        }
    }
}

static void bb_BackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
    // accelIntegrate(); only needed if using accelerometer for something
    // SETUP_FOR_TURBO(); only needed if drawing individual pixels
    if (bigbug->gameData.camera.camera.pos.y >= 400)
    {
        fillDisplayArea(x, y, x + w, y + h, c000);
    }
    else if (bigbug->gameData.camera.camera.pos.y < -1200)
    {
        // get steps from 0 (at -4000) to 14 (at -1200)
        int32_t currentStep = bigbug->gameData.camera.camera.pos.y/200+20;
        if(currentStep < 0)
        {
            currentStep = 0;
        }
        else if(currentStep > 14)
        {
            currentStep = 14;
        }
        // Initialize r, g, b to 4, 5, 5
        uint8_t r = 0;
        uint8_t g = 0;
        uint8_t b = 0;

        // Apply decrements based on current step
        for (int i = 0; i < currentStep; i++)
        {
            if (r == g && g == b)
            {
                b++;
            }
            else if (r == g)
            {
                g++;
            }
            else
            {
                r++;
            }
        }
        fillDisplayArea(x, y, x + w, y + h, (paletteColor_t)(r * 36 + g * 6 + b));
    }
    else
    {
        fillDisplayArea(x, y, x + w, y + h, c455);
    }
}

//==============================================================================
// Big Bug Functions
//==============================================================================

/**
 * @brief Draw the bigbug field to the TFT
 */

static void bb_GarbotniksHomeMusicCb()
{
    soundPlayBgmCb(&bigbug->gameData.garbotniksHome, MIDI_BGM, bb_GarbotniksHomeMusicCb);
    midiPlayer_t* player = globalMidiPlayerGet(MIDI_BGM);
    midiSetProgram(player, 12, 90);//talking sound effects arbitrarily go on channel 12 and use midi instrument 90.
    midiControlChange(player, 12, MCC_SUSTENUTO_PEDAL, 80);
    midiControlChange(player, 12, MCC_SOUND_RELEASE_TIME , 60);
    midiGmOn(player);
}

static void bb_DrawScene(void)
{
    if ((bigbug->gameData.entityManager.playerEntity != NULL) && 
        (GARBOTNIK_DATA == bigbug->gameData.entityManager.playerEntity->dataType))
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

/**
 * @brief This function is called periodically and frequently. It runs the actual game, including processing inputs,
 * physics updates and drawing to the display.
 *
 * @param elapsedUs The time that has elapsed since the last call to this function, in microseconds
 */
static void bb_GameLoop(int64_t elapsedUs)
{
    if(bigbug->gameData.exit)
    {
        bb_ExitMode();
    }
    bigbug->gameData.btnDownState = 0b0;
    // Always process button events, regardless of control scheme, so the main menu button can be captured
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        // Print the current event
        //printf("state: %04X, button: %d, down: %s\n", evt.state, evt.button, evt.down ? "down" : "up");

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
            if (evt.button == PB_START)
            {
                // Toggle pause
                bigbug->gameData.isPaused = !bigbug->gameData.isPaused;
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

    // printf("FPS: %ld\n", 1000000 / elapsedUs);
}

static void bb_Reset(void)
{
    printf("The width is: %d\n", FIELD_WIDTH);
    printf("The height is: %d\n", FIELD_HEIGHT);

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
    while(bigbug->gameData.pleaseCheck.first != NULL)
    {
        uint8_t* shiftedVal = (uint8_t*)shift(&bigbug->gameData.pleaseCheck);
        if((shiftedVal[2] ? bigbug->gameData.tilemap.fgTiles[shiftedVal[0]][shiftedVal[1]].health : bigbug->gameData.tilemap.mgTiles[shiftedVal[0]][shiftedVal[1]].health) > 0)
        {
            bigbug->gameData.toggleIteration = !bigbug->gameData.toggleIteration;
            //pathfind
            if(!(shiftedVal[2] ? 
                pathfindToPerimeter(&bigbug->gameData.tilemap.fgTiles[shiftedVal[0]][shiftedVal[1]], &bigbug->gameData.tilemap, bigbug->gameData.toggleIteration):
                pathfindToPerimeter(&bigbug->gameData.tilemap.mgTiles[shiftedVal[0]][shiftedVal[1]], &bigbug->gameData.tilemap, bigbug->gameData.toggleIteration)))
            {
                //trigger a cascading collapse
                uint8_t* val = HEAP_CAPS_CALLOC_DBG(3,sizeof(uint8_t), MALLOC_CAP_SPIRAM);
                memcpy(val, shiftedVal, 3 * sizeof(uint8_t));
                push(&bigbug->gameData.unsupported, (void*)val);
            }
            FREE_DBG(shiftedVal);
            break;
        }
        FREE_DBG(shiftedVal);
    }

    if (bigbug->gameData.unsupported.first != NULL && bb_randomInt(1,10) == 1)//making it happen randomly slowly crumble sounds and looks nicer.
    {
        for (int i = 0; i < 50; i++) // arbitrarily large loop to get to the dirt tiles.
        {
            if(bigbug->gameData.unsupported.first == NULL)
            {
                break;
            }
            // remove the first item from the list
            uint8_t* shiftedVal = (uint8_t*)shift(&bigbug->gameData.unsupported);
            // check that it's still dirt, because a previous pass may have crumbled it.
            if ((shiftedVal[2] ? bigbug->gameData.tilemap.fgTiles[shiftedVal[0]][shiftedVal[1]].health : bigbug->gameData.tilemap.mgTiles[shiftedVal[0]][shiftedVal[1]].health) > 0)
            {
                // set it to air
                if(shiftedVal[2])
                {
                    bigbug->gameData.tilemap.fgTiles[shiftedVal[0]][shiftedVal[1]].health = 0;
                }
                else
                {
                    bigbug->gameData.tilemap.mgTiles[shiftedVal[0]][shiftedVal[1]].health = 0;
                }
                
                // create a crumble animation
                bb_crumbleDirt(bigbug->gameData.entityManager.playerEntity, bb_randomInt(2,5), shiftedVal[0], shiftedVal[1], true);

                // queue neighbors for crumbling
                for (uint8_t neighborIdx = 0; neighborIdx < 4;
                     neighborIdx++) // neighborIdx 0 thru 3 is Left, Up, Right, Down
                {
                    if ((uint8_t)shiftedVal[0] + bigbug->gameData.neighbors[neighborIdx][0] >= 0
                        && (uint8_t)shiftedVal[0] + bigbug->gameData.neighbors[neighborIdx][0] < TILE_FIELD_WIDTH
                        && (uint8_t)shiftedVal[1] + bigbug->gameData.neighbors[neighborIdx][1] >= 0
                        && (uint8_t)shiftedVal[1] + bigbug->gameData.neighbors[neighborIdx][1] < TILE_FIELD_HEIGHT)
                    {
                        // TODO where is this FREE_DBG()'d?
                        // should be done now.
                        uint8_t* val = HEAP_CAPS_CALLOC_DBG(3, sizeof(uint8_t), MALLOC_CAP_SPIRAM);
                        val[0]        = shiftedVal[0] + bigbug->gameData.neighbors[neighborIdx][0];
                        val[1]        = shiftedVal[1] + bigbug->gameData.neighbors[neighborIdx][1];
                        val[2]        = shiftedVal[2];
                        push(&bigbug->gameData.unsupported, (void*)val);
                    }
                }
                uint8_t* val = HEAP_CAPS_CALLOC_DBG(3, sizeof(uint8_t), MALLOC_CAP_SPIRAM);
                val[0] = shiftedVal[0];
                val[1] = shiftedVal[1];
                val[2] = !shiftedVal[2];
                push(&bigbug->gameData.unsupported, (void*)val);

                FREE_DBG(shiftedVal);
                break;
            }
            FREE_DBG(shiftedVal);
        }
    }
}

static void bb_UpdateLEDs(bb_entityManager_t* entityManager)
{
    if ((entityManager->playerEntity != NULL) && (GARBOTNIK_DATA == entityManager->playerEntity->dataType))
    {
        int32_t fuel = ((bb_garbotnikData_t*)entityManager->playerEntity->data)->fuel;
        // Set the LEDs to a display fuel level
        // printf("timer %d\n", fuel);
        led_t leds[CONFIG_NUM_LEDS] = {0};
        int32_t ledChunk            = 60000 / CONFIG_NUM_LEDS;
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
                leds[i].g = ((fuel - i * ledChunk) * 60000) / ledChunk;
            }
            leds[i].b = 0;
        }
        setLeds(leds, CONFIG_NUM_LEDS);
    }
}
