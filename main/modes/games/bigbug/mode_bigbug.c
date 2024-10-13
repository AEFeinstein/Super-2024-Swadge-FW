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

    bool isPaused; ///< true if the game is paused, false if it is running

    midiFile_t bgm;  ///< Background music
    midiFile_t hit1; ///< A sound effect

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
static void bb_AudioCallback(uint16_t* samples, uint32_t sampleCnt);
static void bb_BackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);
static void bb_EspNowRecvCb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len, int8_t rssi);
static void bb_EspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status);
static int16_t bb_AdvancedUSB(uint8_t* buffer, uint16_t length, uint8_t isGet);

// big bug logic
// static void bb_LoadScreenDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);
static void bb_DrawScene(void);
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

//==============================================================================
// Variables
//==============================================================================

swadgeMode_t bigbugMode = {.modeName                 = bigbugName,
                           .wifiMode                 = ESP_NOW,
                           .overrideUsb              = false,
                           .usesAccelerometer        = true,
                           .usesThermometer          = true,
                           .overrideSelectBtn        = false,
                           .fnEnterMode              = bb_EnterMode,
                           .fnExitMode               = bb_ExitMode,
                           .fnMainLoop               = bb_MainLoop,
                           .fnAudioCallback          = bb_AudioCallback,
                           .fnBackgroundDrawCallback = bb_BackgroundDrawCallback,
                           .fnEspNowRecvCb           = bb_EspNowRecvCb,
                           .fnEspNowSendCb           = bb_EspNowSendCb,
                           .fnAdvancedUSB            = bb_AdvancedUSB,
                           .fnDacCb                  = NULL};

/// All state information for bigbug mode. This whole struct is calloc()'d and free()'d so that bigbug is only
/// using memory while it is being played
bb_t* bigbug = NULL;

//==============================================================================
// Required Functions
//==============================================================================

static void bb_EnterMode(void)
{
    // Force draw a loading screen
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c123);

    bigbug = heap_caps_calloc(1, sizeof(bb_t), MALLOC_CAP_SPIRAM);

    bb_SetLeds();

    // Load font
    loadFont("ibm_vga8.font", &bigbug->font, false);

    const char loadingStr[] = "Loading...";
    int32_t tWidth          = textWidth(&bigbug->font, loadingStr);
    drawText(&bigbug->font, c542, loadingStr, (TFT_WIDTH - tWidth) / 2, (TFT_HEIGHT - bigbug->font.height) / 2);
    drawDisplayTft(NULL);

    bb_initializeGameData(&(bigbug->gameData), &(bigbug->soundManager));
    bb_initializeTileMap(&(bigbug->gameData.tilemap));
    bb_initializeEntityManager(&(bigbug->gameData.entityManager), &(bigbug->gameData), &(bigbug->soundManager));


    bb_createEntity(&(bigbug->gameData.entityManager), LOOPING_ANIMATION, true, ROCKET_ANIM, 3,
                    (TILE_FIELD_WIDTH / 2) * TILE_SIZE + HALF_TILE + 1, -1000);
    bigbug->gameData.camera.pos.x = ((TILE_FIELD_WIDTH / 2) * TILE_SIZE + HALF_TILE + 1) - TFT_WIDTH / 2;

    bb_initializeEggs(&(bigbug->gameData.entityManager), &(bigbug->gameData.tilemap));

    // Player
    //  bb_createEntity(&(bigbug->gameData.entityManager), NO_ANIMATION, true, GARBOTNIK_FLYING, 1,
    //          5 * 32 + 16,
    //          -110);

    // Set the mode to game mode
    bigbug->screen = BIGBUG_GAME;

    bb_Reset();
}

static void bb_ExitMode(void)
{
    bb_deactivateAllEntities(&bigbug->gameData.entityManager, false, false, false);
    // Free entity manager
    bb_freeEntityManager(&bigbug->gameData.entityManager);
    // Free font
    freeFont(&bigbug->font);
}

static void bb_MainLoop(int64_t elapsedUs)
{
    // Pick what runs and draws depending on the screen being displayed
    switch (bigbug->screen)
    {
        case BIGBUG_MENU:
        {
            // Process button events
            buttonEvt_t evt = {0};
            while (checkButtonQueueWrapper(&evt))
            {
                // Pass button events to the menu
                bigbug->menu = menuButton(bigbug->menu, evt);
            }

            // Draw the menu
            break;
        }
        case BIGBUG_GAME:
        {
            // Run the main game loop. This will also process button events
            bb_GameLoop(elapsedUs);
            break;
        }
    }
}

static void bb_AudioCallback(uint16_t* samples, uint32_t sampleCnt)
{
    // Fill this in
}

static void bb_BackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
    // accelIntegrate(); only needed if using accelerometer for something
    // SETUP_FOR_TURBO(); only needed if drawing individual pixels
    if (bigbug->gameData.camera.pos.y < 100)
    {
        fillDisplayArea(x, y, x + w, y + h, c455);
    }
    else
    {
        fillDisplayArea(x, y, x + w, y + h, c000);
    }
}

static void bb_EspNowRecvCb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len, int8_t rssi)
{
    // Fill this in
}

static void bb_EspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status)
{
    // Fill this in
}

static int16_t bb_AdvancedUSB(uint8_t* buffer, uint16_t length, uint8_t isGet)
{
    // Fill this in
    return 0;
}

//==============================================================================
// Big Bug Functions
//==============================================================================

/**
 * @brief Draw the bigbug field to the TFT
 */
static void bb_DrawScene(void)
{
    if (bigbug->gameData.entityManager.playerEntity != NULL)
    {
        vec_t garbotnikDrawPos = {.x = (bigbug->gameData.entityManager.playerEntity->pos.x >> DECIMAL_BITS)
                                       - bigbug->gameData.camera.pos.x - 18,
                                  .y = (bigbug->gameData.entityManager.playerEntity->pos.y >> DECIMAL_BITS)
                                       - bigbug->gameData.camera.pos.y - 17};
        bb_drawTileMap(&bigbug->gameData.tilemap, &bigbug->gameData.camera, &garbotnikDrawPos,
                       &((bb_garbotnikData_t*)bigbug->gameData.entityManager.playerEntity->data)->yaw, &bigbug->gameData.entityManager);
    }
    else
    {
        bb_drawTileMap(&bigbug->gameData.tilemap, &bigbug->gameData.camera, &(vec_t){0, 0}, &(vec_t){0, 0}, &bigbug->gameData.entityManager);
    }
    bb_drawSolidGround(&bigbug->gameData.tilemap, &bigbug->gameData.camera);

    bb_drawEntities(&bigbug->gameData.entityManager, &bigbug->gameData.camera);
}

/**
 * @brief This function is called periodically and frequently. It runs the actual game, including processing inputs,
 * physics updates and drawing to the display.
 *
 * @param elapsedUs The time that has elapsed since the last call to this function, in microseconds
 */
static void bb_GameLoop(int64_t elapsedUs)
{
    // Save the elapsed time
    bigbug->gameData.elapsedUs = elapsedUs;

    // Always process button events, regardless of control scheme, so the main menu button can be captured
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        // Print the current event
        // printf("state: %04X, button: %d, down: %s\n",
        // evt.state, evt.button, evt.down ? "down" : "up");

        // Save the button state
        bigbug->gameData.btnState = evt.state;

        // Check if the pause button was pressed
        if (evt.down && (PB_START == evt.button))
        {
            // Toggle pause
            bigbug->isPaused = !bigbug->isPaused;
        }
    }

    // If the game is not paused, do game logic
    if (bigbug->isPaused == false)
    {
        bb_updateEntities(&(bigbug->gameData.entityManager), &(bigbug->gameData.camera));

        bb_UpdateTileSupport();

        bb_UpdateLEDs(&(bigbug->gameData.entityManager));
        // bigbugFadeLeds(elapsedUs);
        // bigbugControlCpuPaddle();
    }

    // Draw the field
    bb_DrawScene();

    // printf("FPS: %ld\n", 1000000 / elapsedUs);
}

static void bb_Reset(void)
{
    printf("The width is: %d\n", FIELD_WIDTH);
    printf("The height is: %d\n", FIELD_HEIGHT);

    bigbug->gameData.camera.width  = FIELD_WIDTH;
    bigbug->gameData.camera.height = FIELD_HEIGHT;
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
    if (bigbug->gameData.unsupported.first != NULL)
    {
        for (int i = 0; i < 50; i++) // arbitrarily large loop to get to the dirt tiles.
        {
            // remove the first item from the list
            uint32_t* shiftedVal = shift(&bigbug->gameData.unsupported);
            // check that it's still dirt, because a previous pass may have crumbled it.
            if (bigbug->gameData.tilemap.fgTiles[shiftedVal[0]][shiftedVal[1]].health > 0)
            {
                // set it to air
                bigbug->gameData.tilemap.fgTiles[shiftedVal[0]][shiftedVal[1]].health = 0;
                // create a crumble animation
                bb_createEntity(&(bigbug->gameData.entityManager), ONESHOT_ANIMATION, false, CRUMBLE_ANIM, 1,
                                shiftedVal[0] * 32 + 16, shiftedVal[1] * 32 + 16);

                // queue neighbors for crumbling
                for (uint8_t neighborIdx = 0; neighborIdx < 4;
                     neighborIdx++) // neighborIdx 0 thru 3 is Left, Up, Right, Down
                {
                    if ((int32_t)shiftedVal[0] + bigbug->gameData.neighbors[neighborIdx][0] >= 0
                        && (int32_t)shiftedVal[0] + bigbug->gameData.neighbors[neighborIdx][0] < TILE_FIELD_WIDTH
                        && (int32_t)shiftedVal[1] + bigbug->gameData.neighbors[neighborIdx][1] >= 0
                        && (int32_t)shiftedVal[1] + bigbug->gameData.neighbors[neighborIdx][1] < TILE_FIELD_HEIGHT)
                    {
                        // TODO where is this free()'d?
                        uint32_t* val = heap_caps_calloc(2, sizeof(uint32_t), MALLOC_CAP_SPIRAM);
                        val[0]        = shiftedVal[0] + bigbug->gameData.neighbors[neighborIdx][0];
                        val[1]        = shiftedVal[1] + bigbug->gameData.neighbors[neighborIdx][1];

                        push(&bigbug->gameData.unsupported, (void*)val);
                    }
                }
                break;
            }
        }
    }
}

static void bb_UpdateLEDs(bb_entityManager_t* entityManager)
{
    if (entityManager->playerEntity != NULL)
    {
        int16_t squishedFuel = (((bb_garbotnikData_t*)entityManager->playerEntity->data))->fuel
                               / (60000000 / 0b11111111);
        // Set the LEDs to a display fuel level
        led_t leds[CONFIG_NUM_LEDS] = {0};
        for (uint8_t i = 0; i < CONFIG_NUM_LEDS; i++)
        {
            leds[i].r = 0;
            if (squishedFuel >= (i + 1) * 255 / CONFIG_NUM_LEDS)
            {
                leds[i].g = 255;
            }
            else if (squishedFuel < (i) * 255 / CONFIG_NUM_LEDS)
            {
                leds[i].g = 0;
            }
            else
            {
                leds[i].g = ((squishedFuel - i * 255 / CONFIG_NUM_LEDS) * 255) / (255 / CONFIG_NUM_LEDS);
            }
            leds[i].b = 0;
        }
        setLeds(leds, CONFIG_NUM_LEDS);
    }
}
