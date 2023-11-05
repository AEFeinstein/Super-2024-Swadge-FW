//==============================================================================
// Includes
//==============================================================================

#include <esp_heap_caps.h>
#include "mainMenu.h"
#include "mode_ray.h"
#include "ray_map.h"
#include "ray_renderer.h"
#include "ray_object.h"
#include "ray_tex_manager.h"
#include "ray_player.h"
#include "ray_dialog.h"
#include "ray_pause.h"
#include "ray_script.h"
#include "ray_warp_screen.h"
#include "ray_death_screen.h"

//==============================================================================
// Function Prototypes
//==============================================================================

static void rayEnterMode(void);
static void rayExitMode(void);
static void rayMainLoop(int64_t elapsedUs);
static void rayBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);
static void rayMenuCb(const char* label, bool selected, uint32_t settingVal);

//==============================================================================
// Const Variables
//==============================================================================

const char rayName[]       = "Magtroid Pocket";
const char rayPlayStr[]    = "Play";
const char rayResetStr[]   = "Reset Data";
const char rayConfirmStr[] = "Really Reset Data";
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
const char* const songFiles[]
    = {"base_0.sng", "jungle_0.sng", "cave_0.sng", "base_1.sng", "jungle_1.sng", "cave_1.sng", "ray_boss.sng"};

/// @brief The NVS key to save and load player data
const char RAY_NVS_KEY[] = "ray";
// The NVS key to save and load visited tiles
const char* const RAY_NVS_VISITED_KEYS[] = {"rv0", "rv1", "rv2", "rv3", "rv4", "rv5"};

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
    ray = calloc(1, sizeof(ray_t));

    // Initialize the menu
    ray->menu = initMenu(rayName, rayMenuCb);
    addSingleItemToMenu(ray->menu, rayPlayStr);
    ray->menu = startSubMenu(ray->menu, rayResetStr);
    addSingleItemToMenu(ray->menu, rayConfirmStr);
    ray->menu = endSubMenu(ray->menu);
    addSingleItemToMenu(ray->menu, rayExitStr);

    // Load fonts
    loadFont("logbook.font", &ray->logbook, true);
    loadFont("ibm_vga8.font", &ray->ibm, true);

    // Initialize a renderer
    ray->renderer = initMenuLogbookRenderer(&ray->logbook);

    // Force draw a loading screen
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c100);
    const char loadingStr[] = "Loading...";
    int32_t tWidth          = textWidth(&ray->logbook, loadingStr);
    drawText(&ray->logbook, c542, loadingStr, (TFT_WIDTH - tWidth) / 2, (TFT_HEIGHT - ray->logbook.height) / 2);
    drawDisplayTft(NULL);

    // Initialize texture manager and environment textures
    loadEnvTextures(ray);

    // Load songs
    for (int32_t sIdx = 0; sIdx < ARRAY_SIZE(songFiles); sIdx++)
    {
        loadSong(songFiles[sIdx], &ray->songs[sIdx], true);
    }

    // Set the menu as the screen
    ray->screen = RAY_MENU;

    // Start a blink for dialog and pause and such
    ray->blinkTimer = BLINK_US;

    // Turn off LEDs
    led_t leds[CONFIG_NUM_LEDS] = {0};
    setLeds(leds, CONFIG_NUM_LEDS);

    // Set frame rate to 60 FPS
    setFrameRateUs(16666);
}

/**
 * @brief Exit the ray mode and free all allocated memory
 */
static void rayExitMode(void)
{
    // Free menu
    deinitMenu(ray->menu);
    deinitMenuLogbookRenderer(ray->renderer);

    // Free map, scripts, enemies, scenery, bullets, etc.
    rayFreeCurrentState(ray);

    // Free the textures
    freeAllTex(ray);

    // Free songs
    for (int32_t sIdx = 0; sIdx < ARRAY_SIZE(songFiles); sIdx++)
    {
        freeSong(&ray->songs[sIdx]);
    }

    // Free the font
    freeFont(&ray->ibm);
    freeFont(&ray->logbook);

    // Free the game state
    free(ray);
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
    // Head bob
    ray->posZ     = 0;
    ray->bobTimer = 0;
    ray->bobCount = 0;
    // Strafe and lock
    ray->isStrafing  = false;
    ray->targetedObj = NULL;
    // Player timers
    ray->lavaTimer      = 0;
    ray->chargeTimer    = 0;
    ray->pRotationTimer = 0;
    // Dialog variables
    ray->dialogText     = NULL;
    ray->nextDialogText = NULL;
    ray->dialogPortrait = NULL;
    ray->btnLockoutUs   = 0;
    // Pause menu variables
    ray->blinkTimer = 0;
    ray->blink      = false;
    // Item rotation
    ray->itemRotateTimer  = 0;
    ray->itemRotateDeg    = 0;
    ray->itemRotateMirror = false;

    // Set invalid IDs for all bullets
    for (uint16_t objIdx = 0; objIdx < MAX_RAY_BULLETS; objIdx++)
    {
        ray->bullets[objIdx].c.id = -1;
    }

    // Empty all lists
    rayEnemy_t* poppedEnemy = NULL;
    while (NULL != (poppedEnemy = pop(&cRay->enemies)))
    {
        free(poppedEnemy);
    }

    rayObjCommon_t* poppedItem = NULL;
    while (NULL != (poppedItem = pop(&cRay->items)))
    {
        free(poppedItem);
    }

    rayObjCommon_t* poppedScenery = NULL;
    while (NULL != (poppedScenery = pop(&cRay->scenery)))
    {
        free(poppedScenery);
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
    if (0 < ray->blinkTimer)
    {
        ray->blinkTimer -= elapsedUs;
        if (0 >= ray->blinkTimer)
        {
            ray->blink      = !ray->blink;
            ray->blinkTimer = BLINK_US;
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
            drawMenuLogbook(ray->menu, ray->renderer, elapsedUs);
            break;
        }
        case RAY_GAME:
        {
            // Render everything! This must be done first, to draw over the floor and ceiling,
            // which were drawn in the background callback, before updating any positions or directions
            // Draw the walls after floor & ceiling
            castWalls(ray);
            // Draw sprites after walls
            rayObjCommon_t* centeredEnemy = castSprites(ray);
            // Draw the HUD after sprites
            drawHud(ray);

            // Run timers for head-bob, doors, etc.
            runEnvTimers(ray, elapsedUs);

            // Check buttons for the player and move player accordingly
            rayPlayerCheckButtons(ray, centeredEnemy, elapsedUs);

            // Check the joystick for the player and update loadout accordingly
            rayPlayerCheckJoystick(ray, elapsedUs);

            // Check for lava damage
            rayPlayerCheckLava(ray, elapsedUs);

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
                ray->screen = RAY_WARP_SCREEN;
                // Do the warp in the background
                warpToDestination(ray);
            }

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
        {
            // Do nothing
            break;
        }
        case RAY_GAME:
        {
            // Draw a portion of the background
            castFloorCeiling(ray, y, y + h);
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
static void rayMenuCb(const char* label, bool selected, uint32_t settingVal)
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
        else if (label == rayExitStr)
        {
            switchToSwadgeMode(&mainMenuMode);
        }
    }
}

/**
 * @brief Start the game from the menu
 */
void rayStartGame(void)
{
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
    }

    // Save a backup of the player state to restore in case of death
    memcpy(&ray->p_backup, &ray->p, sizeof(rayPlayer_t));

    // Mark the starting tile as visited
    markTileVisited(&ray->map, FROM_FX(ray->p.posX), FROM_FX(ray->p.posY));

    // Set the initial screen
    ray->screen = RAY_GAME;
}
