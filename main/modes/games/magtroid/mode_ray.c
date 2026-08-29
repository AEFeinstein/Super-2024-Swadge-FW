//==============================================================================
// Includes
//==============================================================================

#include <esp_heap_caps.h>
#include "mainMenu.h"
#include "mode_ray.h"
#include "ray_map.h"
#include "ray_object.h"
#include "ray_tex_manager.h"
#include "ray_player.h"
#include "ray_dialog.h"
#include "ray_pause.h"
#include "ray_script.h"
#include "ray_warp_screen.h"
#include "ray_death_screen.h"
#include "ray_credits.h"
#include "ray_instrument.h"
#include "2d_renderer.h"
#include "ray_shop_dialog.h"

//==============================================================================
// Function Prototypes
//==============================================================================

static void rayEnterMode(void);
static void rayExitMode(void);
static void rayMainLoop(int64_t elapsedUs);
static void rayBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);
static bool rayMenuCb(const char* label, bool selected, uint32_t settingVal);
static void rayInitMenu(void);
static void rayInstrumentDacCallback(uint8_t* samples, int16_t len);

//==============================================================================
// Const Variables
//==============================================================================

const char rayName[]       = "Magtroid Pocket";
const char rayPlayStr[]    = "Play";
const char rayResetStr[]   = "Reset Data";
const char rayConfirmStr[] = "Really Reset Data";
const char rayCreditsStr[] = "Credits";
const char rayExitStr[]    = "Exit";

/// @brief A list of the map names
const char* const rayMapNames[] = {
    "Station Zero", "Vinegrasp", "Floriss", "Station One", "Mosspire", "Scalderia",
};

/// @brief A list of the map colors, in order
const paletteColor_t rayMapColors[] = {
    c303, // Violet
    c005, // Blue
    c030, // Green
    c550, // Yellow
    c530, // Orange
    c500, // Red
};

/// @brief The songs to play, must be in map order
const cnfsFileIdx_t songFiles[]
    = {BASE_0_MID, JUNGLE_0_MID, CAVE_0_MID, BASE_1_MID, JUNGLE_1_MID, CAVE_1_MID, RAY_BOSS_MID};

/// @brief The NVS key to save and load player data
const char RAY_NVS_KEY[] = "ray";
// The NVS key to save and load visited tiles
const char* const RAY_NVS_VISITED_KEYS[] = {"rv0", "rv1", "rv2", "rv3", "rv4", "rv5"};

/// @brief The NVS key to unlock Sip on the menu
const char MAGTROID_UNLOCK_KEY[] = "zip_unlock";

//==============================================================================
// Variables
//==============================================================================

swadgeMode_t rayMode = {
    .modeName                 = rayName,
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = true, // Not actually doing anything, just don't init USB
    .usesAccelerometer        = false,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = rayEnterMode,
    .fnExitMode               = rayExitMode,
    .fnMainLoop               = rayMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = rayBackgroundDrawCallback,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL,
    .fnDacCb                  = NULL,
};

ray_t* ray;

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Enter the Ray mode and initialize everything
 */
static void rayEnterMode(void)
{
    // Allocate memory
    ray = heap_caps_calloc(1, sizeof(ray_t), MALLOC_CAP_8BIT);

    // Load fonts
    loadFont(LOGBOOK_FONT, &ray->logbook, true);
    loadFont(IBM_VGA_8_FONT, &ray->ibm, true);

    // Initialize the menu
    rayInitMenu();

    // Force draw a loading screen
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c100);
    const char loadingStr[] = "Loading...";
    int32_t tWidth          = textWidth(&ray->logbook, loadingStr);
    drawText(&ray->logbook, c542, loadingStr, (TFT_WIDTH - tWidth) / 2, (TFT_HEIGHT - ray->logbook.height) / 2);
    drawDisplayTft(NULL);

    // Initialize texture manager and environment textures
    loadEnvTextures(ray);

    // Set BGM to loop
    globalMidiPlayerGet(MIDI_BGM)->loop = true;

    // Load songs
    for (int32_t sIdx = 0; sIdx < ARRAY_SIZE(songFiles); sIdx++)
    {
        loadMidiFile(songFiles[sIdx], &ray->songs[sIdx], false);
    }

    // Load SFX
    loadMidiFile(R_DOOR_OPEN_MID, &ray->sfx_door_open, false);
    loadMidiFile(R_E_DAMAGE_MID, &ray->sfx_e_damage, false);
    loadMidiFile(R_E_FREEZE_MID, &ray->sfx_e_freeze, false);
    loadMidiFile(R_P_CHARGE_MID, &ray->sfx_p_charge, false);
    loadMidiFile(R_P_DAMAGE_MID, &ray->sfx_p_damage, false);
    loadMidiFile(R_P_SHOOT_MID, &ray->sfx_p_shoot, false);
    loadMidiFile(R_E_BLOCK_MID, &ray->sfx_e_block, false);
    loadMidiFile(R_E_DEAD_MID, &ray->sfx_e_dead, false);
    loadMidiFile(R_ITEM_GET_MID, &ray->sfx_item_get, false);
    loadMidiFile(R_P_CHARGE_MID, &ray->sfx_p_charge_start, false);
    loadMidiFile(R_P_MISSILE_MID, &ray->sfx_p_missile, false);
    loadMidiFile(R_P_ICE_MID, &ray->sfx_p_ice, false);
    loadMidiFile(R_P_XRAY_MID, &ray->sfx_p_xray, false);
    loadMidiFile(R_WARP_MID, &ray->sfx_warp, false);
    loadMidiFile(R_LAVA_DMG_MID, &ray->sfx_lava_dmg, false);
    loadMidiFile(R_HEALTH_MID, &ray->sfx_health, false);
    loadMidiFile(R_GAME_OVER_MID, &ray->sfx_game_over, false);

    // Set the menu as the screen
    raySwitchToScreen(RAY_MENU);

    // Start a blink for dialog and pause and such
    ray->blinkTimer = BLINK_US;

    // Initialize instrument oscillators
    swSynthInitOscillator(&ray->lOsc, SHAPE_SINE, 440, 0);
    swSynthInitOscillator(&ray->rOsc, SHAPE_SINE, 440, 0);
    ray->oscillators[0] = &ray->lOsc;
    ray->oscillators[1] = &ray->rOsc;

    // Turn off LEDs
    led_t leds[CONFIG_NUM_LEDS] = {0};
    setLeds(leds, CONFIG_NUM_LEDS);

    // Set frame rate to 30 FPS
    setFrameRateUs(33333);
}

/**
 * @brief Exit the ray mode and free all allocated memory
 */
static void rayExitMode(void)
{
    // Free menu
    deinitMenu(ray->menu);
    deinitMenuZorldoRenderer(ray->renderer);

    // Free map, scripts, enemies, scenery, bullets, etc.
    rayFreeCurrentState(ray);

    // Free the textures
    freeAllTex(ray);

    // Free songs
    for (int32_t sIdx = 0; sIdx < ARRAY_SIZE(songFiles); sIdx++)
    {
        unloadMidiFile(&ray->songs[sIdx]);
    }

    // Free SFX
    unloadMidiFile(&ray->sfx_door_open);
    unloadMidiFile(&ray->sfx_e_damage);
    unloadMidiFile(&ray->sfx_e_freeze);
    unloadMidiFile(&ray->sfx_p_charge);
    unloadMidiFile(&ray->sfx_p_damage);
    unloadMidiFile(&ray->sfx_p_shoot);
    unloadMidiFile(&ray->sfx_e_block);
    unloadMidiFile(&ray->sfx_e_dead);
    unloadMidiFile(&ray->sfx_item_get);
    unloadMidiFile(&ray->sfx_p_charge_start);
    unloadMidiFile(&ray->sfx_p_missile);
    unloadMidiFile(&ray->sfx_p_ice);
    unloadMidiFile(&ray->sfx_p_xray);
    unloadMidiFile(&ray->sfx_warp);
    unloadMidiFile(&ray->sfx_lava_dmg);
    unloadMidiFile(&ray->sfx_health);
    unloadMidiFile(&ray->sfx_game_over);

    // Free the font
    freeFont(&ray->ibm);
    freeFont(&ray->logbook);

    // Free the game state
    heap_caps_free(ray);
}

/**
 * @brief Free allocated memory for the current state, but not assets like textures, fonts, or songs
 *
 * @param cRay The entire game state
 */
void rayFreeCurrentState(ray_t* cRay)
{
    // Zero and NULL the door timer
    ray->doorTimer = 0;
    // Dialog variables
    ray->dialogText     = NULL;
    ray->nextDialogText = NULL;
    ray->dialogPortrait = NULL;
    ray->btnLockoutUs   = 0;
    // Pause menu variables
    ray->blinkTimer = 0;
    ray->blink      = false;

    // Set invalid IDs for all bullets
    for (uint16_t objIdx = 0; objIdx < MAX_RAY_BULLETS; objIdx++)
    {
        ray->bullets[objIdx].c.id = -1;
    }

    // Empty all lists
    rayEnemy_t* poppedEnemy = NULL;
    while (NULL != (poppedEnemy = pop(&cRay->enemies)))
    {
        heap_caps_free(poppedEnemy);
    }

    rayObjCommon_t* poppedItem = NULL;
    while (NULL != (poppedItem = pop(&cRay->items)))
    {
        heap_caps_free(poppedItem);
    }

    rayObjCommon_t* poppedScenery = NULL;
    while (NULL != (poppedScenery = pop(&cRay->scenery)))
    {
        heap_caps_free(poppedScenery);
    }

    // Free the scripts
    freeScripts(cRay);
    // Free the map
    freeRayMap(&cRay->map);
}

/**
 * @brief This function is called from the main loop. It does everything
 *
 * @param elapsedUs The time elapsed since the last time this function was called.
 */
static void rayMainLoop(int64_t elapsedUs)
{
    // Run a button lockout, regardless of mode
    if (0 < ray->btnLockoutUs)
    {
        ray->btnLockoutUs -= elapsedUs;
        if (0 >= ray->btnLockoutUs)
        {
            ray->btnLockoutUs = 0;
            // Restart blinks when the lockout ends
            ray->blink      = true;
            ray->blinkTimer = BLINK_US;
        }
    }

    // Run a timer to blink things
    ray->blinkTimer -= elapsedUs;
    while (0 >= ray->blinkTimer)
    {
        ray->blink = !ray->blink;
        ray->blinkTimer += BLINK_US;
    }

    // 0 is red
    // 85 is green
    // 170 is blue
    // If the LED is not at the target hue
    if (ray->ledHue != ray->targetLedHue)
    {
        ray->ledTimer -= elapsedUs;
        while (0 >= ray->ledTimer)
        {
            ray->ledTimer += 7812;

            // Find the decision point to either increment or decrement to the target
            int32_t decisionPoint = (ray->targetLedHue + 128) % 256;
            bool shouldIncrement  = false;
            if (decisionPoint < ray->targetLedHue)
            {
                if ((decisionPoint < ray->ledHue) && (ray->ledHue < ray->targetLedHue))
                {
                    shouldIncrement = true;
                }
                else
                {
                    shouldIncrement = false;
                }
            }
            else
            {
                if ((ray->targetLedHue < ray->ledHue) && (ray->ledHue < decisionPoint))
                {
                    shouldIncrement = false;
                }
                else
                {
                    shouldIncrement = true;
                }
            }

            // Adjust the hue with wraparound
            if (shouldIncrement)
            {
                ray->ledHue = (ray->ledHue + 1) % 255;
            }
            else
            {
                if (0 == ray->ledHue)
                {
                    ray->ledHue = 255;
                }
                else
                {
                    ray->ledHue--;
                }
            }
        }
    }

    switch (ray->screen)
    {
        case RAY_MENU:
        {
            // Check buttons
            buttonEvt_t evt = {0};
            while (checkButtonQueueWrapper(&evt))
            {
                ray->menu = menuButton(ray->menu, evt);
                if (ray->wasReset)
                {
                    ray->wasReset = false;
                    // Return up one level
                    ray->menu = ray->menu->parentMenu;
                }
            }
            // Draw the menu
            drawMenuZorldo(ray->menu, ray->renderer, elapsedUs);
            break;
        }
        case RAY_GAME:
        {
            drawForeground2d(ray, elapsedUs);

            // Only run this code when the camera is settled
            if (ray->camera.x == ray->cameraTarget.x && ray->camera.y == ray->cameraTarget.y)
            {
                // Check buttons for the player and move player accordingly
                rayPlayerCheckButtons(ray, elapsedUs);

                // Check the joystick for the player and update loadout accordingly
                rayPlayerCheckJoystick(ray, elapsedUs);

                // Check for floor effects
                rayPlayerCheckFloorEffect(ray, elapsedUs);

                // Move objects including enemies and bullets
                moveRayObjects(ray, elapsedUs);

                // Check for collisions between the moved player, enemies, and bullets
                checkRayCollisions(ray);

                // Check for time-based scripts
                checkScriptTime(ray, elapsedUs);

                // If the warp timer is active
                if (ray->warpTimerUs > 0)
                {
                    // Switch to showing the warp screen
                    raySwitchToScreen(RAY_WARP_SCREEN);
                    // Do the warp in the background
                    warpToDestination(ray);
                }
                // Show credits if it's in the game state and it should
                // It may switch to the dialog state while credits are pending
                else if ((RAY_GAME == ray->screen) && (ray->shouldShowCredits))
                {
                    ray->shouldShowCredits = false;
                    rayShowCredits(ray);
                }
            }
            break;
        }
        case RAY_INSTRUMENT:
        {
            // Check buttons
            rayInstrumentCheckButtons(ray);
            // Draw game foreground
            drawForeground2d(ray, elapsedUs);
            // Draw instrument UI on top of that
            rayInstrumentRender(ray, elapsedUs);
            break;
        }
        case RAY_SHOP_DIALOG:
        {
            // Draw game foreground
            drawForeground2d(ray, elapsedUs);
            // Render dialog on top of that
            rayShopDialogRender(ray, elapsedUs);
            // Check buttons
            rayShopDialogCheckButtons(ray);
            break;
        }
        case RAY_DIALOG:
        {
            // Render first
            rayDialogRender(ray, elapsedUs);
            // Then check buttons
            rayDialogCheckButtons(ray);
            break;
        }
        case RAY_PAUSE:
        {
            // Render first
            rayPauseRender(ray, elapsedUs);
            // Then check buttons
            rayPauseCheckButtons(ray);
            break;
        }
        case RAY_WARP_SCREEN:
        {
            // Only render
            rayWarpScreenRender(ray, elapsedUs);
            break;
        }
        case RAY_DEATH_SCREEN:
        {
            rayDeathScreenRender(ray, elapsedUs);
            break;
        }
        case RAY_CREDITS:
        {
            rayCreditsRender(ray, elapsedUs);
            break;
        }
    }
}

/**
 * @brief This function is called when the display driver wishes to update a section of the background
 * This is used to draw the first layer: the floor and ceiling
 *
 * @param x the x coordinate that should be updated
 * @param y the x coordinate that should be updated
 * @param w the width of the rectangle to be updated
 * @param h the height of the rectangle to be updated
 * @param up update number, ignored
 * @param numUp update number denominator, ignored
 */
static void rayBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
    switch (ray->screen)
    {
        case RAY_MENU:
        case RAY_DIALOG:
        case RAY_DEATH_SCREEN:
        case RAY_PAUSE:
        case RAY_CREDITS:
        {
            // Do nothing
            break;
        }
        case RAY_SHOP_DIALOG:
        case RAY_INSTRUMENT:
        case RAY_GAME:
        {
            drawBackground2d(ray, y, y + h);
            break;
        }
        case RAY_WARP_SCREEN:
        {
            drawWarpBackground(x, y, w, h);
            break;
        }
    }
}

/**
 * @brief Handle menu callbacks
 *
 * @param label The menu option selected or scrolled to
 * @param selected if the menu option was selected
 * @param settingVal The setting value, if this is a setting
 */
static bool rayMenuCb(const char* label, bool selected, uint32_t settingVal)
{
    if (selected)
    {
        if (label == rayPlayStr)
        {
            rayStartGame();
        }
        else if (label == rayConfirmStr)
        {
            // Wipe NVM
            eraseNvsKey(RAY_NVS_KEY);
            for (int16_t kIdx = 0; kIdx < ARRAY_SIZE(RAY_NVS_VISITED_KEYS); kIdx++)
            {
                eraseNvsKey(RAY_NVS_VISITED_KEYS[kIdx]);
            }
            // Return up one menu
            ray->wasReset = true;
        }
        else if (label == rayCreditsStr)
        {
            rayShowCredits(ray);
        }
        else if (label == rayExitStr)
        {
            switchToSwadgeMode(&mainMenuMode);
        }
    }
    return false;
}

/**
 * @brief Start the game from the menu
 */
void rayStartGame(void)
{
    // Stop the buzzer to not interfere with loading data
    globalMidiPlayerStop(true);

    // Clear all lists
    rayFreeCurrentState(ray);

    // Load player data from NVM
    bool initFromScratch = initializePlayer(ray);

    // Load the map and object data
    q24_8 pStartX = 0, pStartY = 0;
    loadRayMap(ray->p.mapId, ray, &pStartX, &pStartY, true);

    // If the player was initialized from scratch
    if (initFromScratch)
    {
        // Set the starting position from the map
        ray->p.posX = pStartX;
        ray->p.posY = pStartY;

        // Assuming the starting position is safe
        ray->ps.lastGoodCell.x = FROM_FX(ray->p.posX);
        ray->ps.lastGoodCell.y = FROM_FX(ray->p.posY);

        // Save the starting position
        raySavePlayer(ray);
    }

    // Mark the starting tile as visited
    markTileVisited(&ray->map, FROM_FX(ray->p.posX), FROM_FX(ray->p.posY));

    // Set the default hue (blue)
    ray->targetLedHue = 170;

    // Set the initial screen
    raySwitchToScreen(RAY_GAME);

    // Check script from entering the initial cell
    checkScriptEnter(ray, FROM_FX(pStartX), FROM_FX(pStartY));
}

/**
 * @brief Switch to a new ray screen, initialize as necessary
 *
 * @param newScreen The new screen to switch to
 */
void raySwitchToScreen(rayScreen_t newScreen)
{
    // Set the new screen
    ray->screen = newScreen;

    // Assume MIDI output
    rayMode.fnDacCb = NULL;

    // Initialize depending on the screen
    switch (newScreen)
    {
        case RAY_MENU:
        {
            globalMidiPlayerStop(true);
            // Reinit menu
            rayInitMenu();
            break;
        }
        case RAY_INSTRUMENT:
        {
            ray->is.lTouchZone  = -1;
            ray->is.rTouchZone  = -1;
            ray->is.notesActive = 0;
            ray->is.noteHistory = 0xFFFFFFFF;
            globalMidiPlayerPauseAll();

            rayMode.fnDacCb = rayInstrumentDacCallback;
            break;
        }
        case RAY_GAME:
        case RAY_SHOP_DIALOG:
        case RAY_DIALOG:
        case RAY_PAUSE:
        case RAY_WARP_SCREEN:
        case RAY_DEATH_SCREEN:
        case RAY_CREDITS:
        default:
        {
            break;
        }
    }
}

/**
 * @brief Initialize the main menu
 */
static void rayInitMenu(void)
{
    // Tear down old menu, if it exists
    if (NULL != ray->renderer)
    {
        deinitMenuZorldoRenderer(ray->renderer);
    }
    if (NULL != ray->menu)
    {
        deinitMenu(ray->menu);
    }

    // Initialize new one
    ray->menu = initMenu(rayName, rayMenuCb);
    addSingleItemToMenu(ray->menu, rayPlayStr);
    ray->menu = startSubMenu(ray->menu, rayResetStr);
    addSingleItemToMenu(ray->menu, rayConfirmStr);
    ray->menu = endSubMenu(ray->menu);

    // Only show credits if the game was beaten
    int32_t magtroidUnlocked = false;
    readNvs32(MAGTROID_UNLOCK_KEY, &magtroidUnlocked);
    if (magtroidUnlocked)
    {
        addSingleItemToMenu(ray->menu, rayCreditsStr);
    }

    addSingleItemToMenu(ray->menu, rayExitStr);

    // Initialize a renderer
    ray->renderer = initMenuZorldoRenderer(NULL, NULL);
}

/**
 * @brief A callback which requests DAC samples from the application
 *
 * @param samples A buffer to fill with 8 bit unsigned DAC samples
 * @param len The length of the buffer to fill
 */
static void rayInstrumentDacCallback(uint8_t* samples, int16_t len)
{
    while (len--)
    {
        *samples++ = swSynthMixOscillators(ray->oscillators, ARRAY_SIZE(ray->oscillators));
    }
}

/**
 * @brief Get all of the game state. This can be called from within callbacks that can't take ray_t* as an argument
 *
 * @return The entire game state
 */
ray_t* getRayState(void)
{
    return ray;
}
