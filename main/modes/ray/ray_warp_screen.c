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

    // Draw text over the starfield with the destination name
    static const char warpText[] = "Warping to";
    int16_t tWidth               = textWidth(&ray->logbook, warpText);
    drawText(&ray->logbook, c555, warpText, (TFT_WIDTH - tWidth) / 2, (TFT_HEIGHT / 2) - ray->logbook.height - 4);
    tWidth = textWidth(&ray->logbook, rayMapNames[ray->warpDestMapId]);
    drawText(&ray->logbook, c555, rayMapNames[ray->warpDestMapId], (TFT_WIDTH - tWidth) / 2, (TFT_HEIGHT / 2) + 4);

    // Decrement the timer
    ray->warpTimerUs -= elapsedUs;
    // If it expired
    if (ray->warpTimerUs <= 0)
    {
        // Return to the game
        ray->screen = RAY_GAME;
        // Don't warp again
        ray->warpTimerUs = 0;
        // Play music
        bzrPlayBgm(&ray->songs[ray->p.mapId], BZR_STEREO);
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

    // Stop BGM when warping
    bzrStop(true);
}

/**
 * @brief Execute the warp to the destination map and cell, freeing and reloading the map and objects
 *
 * @param ray The entire game state
 */
void warpToDestination(ray_t* ray)
{
    // Save the current map's visited tiles
    raySaveVisitedTiles(ray);

    // Initialize the starfield
    initializeStarfield(&ray->starfield, true);

    // Free the scripts
    rayFreeCurrentState(ray);

    // Load the new map
    q24_8 pStartX = 0, pStartY = 0;
    loadRayMap(ray->warpDestMapId, ray, &pStartX, &pStartY, true);

    // Stop BGM when warping
    bzrStop(true);

    // Set the map ID
    ray->p.mapId                     = ray->warpDestMapId;
    ray->p.mapsVisited[ray->p.mapId] = true;

    // Set the player position after the map is loaded
    ray->p.posX = ray->warpDestPosX;
    ray->p.posY = ray->warpDestPosY;

    // Initialize player angle
    if (ray->p.posX < TO_FX(ray->map.w) / 2)
    {
        // On the left side, look right
        ray->p.dirX = TO_FX(1);
        ray->p.dirY = TO_FX(0);
    }
    else
    {
        // On the right side, look left
        ray->p.dirX = -TO_FX(1);
        ray->p.dirY = TO_FX(0);
    }
    ray->planeX = -MUL_FX(TO_FX(2) / 3, ray->p.dirY);
    ray->planeY = MUL_FX(TO_FX(2) / 3, ray->p.dirX);

    // Give the player one missile after warping to not get stuck behind doors
    if (ray->p.i.missileLoadOut)
    {
        if (0 == ray->p.i.numMissiles)
        {
            ray->p.i.numMissiles++;
        }
    }

    // Save after warping
    raySavePlayer(ray);

    // Save a backup of the player state to restore in case of death
    memcpy(&ray->p_backup, &ray->p, sizeof(rayPlayer_t));

    // Mark the starting tile as visited
    markTileVisited(&ray->map, FROM_FX(ray->p.posX), FROM_FX(ray->p.posY));
}
