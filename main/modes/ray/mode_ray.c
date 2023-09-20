//==============================================================================
// Includes
//==============================================================================

#include <esp_heap_caps.h>
#include "mode_ray.h"
#include "ray_map.h"
#include "ray_renderer.h"
#include "ray_object.h"
#include "ray_tex_manager.h"
#include "ray_player.h"
#include "ray_dialog.h"
#include "ray_pause.h"
#include "ray_script.h"

//==============================================================================
// Function Prototypes
//==============================================================================

static void rayEnterMode(void);
static void rayExitMode(void);
static void rayMainLoop(int64_t elapsedUs);
static void rayBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);

//==============================================================================
// Const Variables
//==============================================================================

const char rayName[] = "Magtroid Pocket";

//==============================================================================
// Variables
//==============================================================================

swadgeMode_t rayMode = {
    .modeName                 = rayName,
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = true, // Not actually doing anything, just don't init USB
    .usesAccelerometer        = false,
    .usesThermometer          = false,
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

    // Set invalid IDs for all bullets
    for (uint16_t objIdx = 0; objIdx < MAX_RAY_BULLETS; objIdx++)
    {
        ray->bullets[objIdx].c.id = -1;
    }
    // lists of enemies, items, and scenery are already cleared

    loadFont("ibm_vga8.font", &ray->ibm, true);

    // Initialize texture manager and environment textures
    loadEnvTextures(ray);

    // Initialize enemy templates and textures
    initEnemyTemplates(ray);

    // Load the map and object data
    loadRayMap("demo.rmh", ray, false);

    // Set initial position and direction, centered on the tile
    initializePlayer(ray);

    // Mark the starting tile as visited
    markTileVisited(&ray->map, FROM_FX(ray->posX), FROM_FX(ray->posY));

    // Turn off LEDs
    led_t leds[CONFIG_NUM_LEDS] = {0};
    setLeds(leds, CONFIG_NUM_LEDS);

    // Set the initial screen
    ray->screen = RAY_GAME;
}

/**
 * @brief Exit the ray mode and free all allocated memory
 */
static void rayExitMode(void)
{
    // Empty all lists
    rayEnemy_t* poppedEnemy = NULL;
    while (NULL != (poppedEnemy = pop(&ray->enemies)))
    {
        free(poppedEnemy);
    }

    rayObjCommon_t* poppedItem = NULL;
    while (NULL != (poppedItem = pop(&ray->items)))
    {
        free(poppedItem);
    }

    rayObjCommon_t* poppedScenery = NULL;
    while (NULL != (poppedScenery = pop(&ray->scenery)))
    {
        free(poppedScenery);
    }

    // Free the scripts
    freeScripts(ray);
    // Free the map
    freeRayMap(&ray->map);
    // Free the textures
    freeAllTex(ray);

    // Free the font
    freeFont(&ray->ibm);

    // Free the game state
    free(ray);
}

/**
 * @brief This function is called from the main loop. It does everything
 *
 * @param elapsedUs The time elapsed since the last time this function was called.
 */
static void rayMainLoop(int64_t elapsedUs)
{
    switch (ray->screen)
    {
        case RAY_MENU:
        {
            break;
        }
        case RAY_GAME:
        {
            // Render everything! This must be done first, to draw over the floor and ceiling,
            // which were drawn in the background callback, before updating any positions or directions
            // Draw the walls after floor & ceiling
            castWalls(ray);
            // Draw sprites after walls
            rayObjCommon_t* centeredSprite = castSprites(ray);
            // Draw the HUD after sprites
            drawHud(ray);

            // Run timers for head-bob, doors, etc.
            runEnvTimers(ray, elapsedUs);

            // Check buttons for the player and move player accordingly
            rayPlayerCheckButtons(ray, centeredSprite, elapsedUs);

            // Check the joystick for the player and update loadout accordingly
            rayPlayerCheckJoystick(ray, elapsedUs);

            // Check for lava damage
            rayPlayerCheckLava(ray, elapsedUs);

            // Move objects including enemies and bullets
            moveRayObjects(ray, elapsedUs);

            // Check for collisions between the moved player, enemies, and bullets
            checkRayCollisions(ray);
            break;
        }
        case RAY_DIALOG:
        {
            // Render first
            rayDialogRender(ray);
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
        case RAY_PAUSE:
        {
            // Draw a black background
            fillDisplayArea(x, y, x + w, y + h, c000);
        }
    }
}
