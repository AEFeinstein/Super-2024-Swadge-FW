//==============================================================================
// Includes
//==============================================================================

#include "ray_warp_screen.h"
#include "ray_map.h"
#include "ray_player.h"

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Draw the background for the warp screen
 *
 * @param x the x coordinate that should be updated
 * @param y the x coordinate that should be updated
 * @param w the width of the rectangle to be updated
 * @param h the height of the rectangle to be updated
 */
void drawWarpBackground(int16_t x, int16_t y, int16_t w, int16_t h)
{
    // Draw black screen
    fillDisplayArea(x, y, x + w, y + h, c000);
}

/**
 * @brief Draw the foreground for the warp screen and run the timer
 *
 * @param ray The entire game state
 * @param elapsedUs The elapsed time since this function was last called
 */
void rayWarpScreenRender(ray_t* ray, uint32_t elapsedUs)
{
    // Draw the starfield
    updateStarfield(&ray->starfield, 32);
    drawStarfield(&ray->starfield);

    // Draw text over the starfield
    // TODO use map destination name
    int16_t tWidth = textWidth(&ray->logbook, "Warping");
    drawText(&ray->logbook, c555, "Warping", (TFT_WIDTH - tWidth) / 2, (TFT_HEIGHT - ray->logbook.height) / 2);

    // Decrement the timer
    ray->warpTimerUs -= elapsedUs;
    // If it expired
    if (ray->warpTimerUs <= 0)
    {
        // Return to the game
        ray->screen = RAY_GAME;
        // Don't warp again
        ray->warpTimerUs = 0;
    }
}

/**
 * @brief Set the warp map and cell destination, but do not warp yet
 *
 * @param ray The entire game state
 * @param mapId The map ID to warp to
 * @param posX The map cell X position to warp to
 * @param posY The map cell Y position to warp to
 */
void setWarpDestination(ray_t* ray, int32_t mapId, int16_t posX, int16_t posY)
{
    // Set the destination
    ray->warpDestMapId = mapId;
    ray->warpDestPosX  = ADD_FX(TO_FX(posX), TO_FX_FRAC(1, 2));
    ray->warpDestPosY  = ADD_FX(TO_FX(posY), TO_FX_FRAC(1, 2));

    // Set the warp timer
    ray->warpTimerUs = 4000000;
}

/**
 * @brief Execute the warp to the destination map and cell, freeing and reloading the map and objects
 *
 * @param ray The entire game state
 */
void warpToDestination(ray_t* ray)
{
    // Initialize the starfield
    initializeStarfield(&ray->starfield, true);

    // Free the scripts
    rayFreeCurrentState(ray);

    // Construct the map name
    char mapName[] = "0.rmh";
    mapName[0]     = '0' + ray->warpDestMapId;
    // Load the new map
    loadRayMap(mapName, ray, false);

    // Set the player position after the map is loaded
    ray->posX = ray->warpDestPosX;
    ray->posY = ray->warpDestPosY;

    // Initialize player angle
    initializePlayer(ray, false);
}
