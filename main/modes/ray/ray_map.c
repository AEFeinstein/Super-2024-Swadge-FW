//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include <esp_heap_caps.h>

#include "hdw-spiffs.h"
#include "heatshrink_helper.h"
#include "ray_map.h"
#include "ray_tex_manager.h"
#include "ray_renderer.h"
#include "ray_script.h"

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Load a RMH from ROM to RAM. RMHs placed in the spiffs_image folder
 * before compilation will be automatically flashed to ROM
 *
 * @param name The filename of the RMH to load
 * @param ray The ray_t to load the map into
 * @param pStartX The starting X coordinate for this map
 * @param pStartY The starting Y coordinate for this map
 * @param spiRam true to load to SPI RAM, false to load to normal RAM. SPI RAM is more plentiful but slower to access
 * than normal RAM
 */
void loadRayMap(const char* name, ray_t* ray, q24_8* pStartX, q24_8* pStartY, bool spiRam)
{
    // Pick the allocation type
    uint32_t caps = spiRam ? MALLOC_CAP_SPIRAM : MALLOC_CAP_DEFAULT;

    // Convenience pointers
    rayMap_t* map = &ray->map;

    // Read and decompress the file
    uint32_t decompressedSize = 0;
    uint8_t* fileData         = readHeatshrinkFile(name, &decompressedSize, spiRam);
    uint32_t fileIdx          = 0;

    // Read the width and height
    map->w = fileData[fileIdx++];
    map->h = fileData[fileIdx++];

    // Allocate the tiles, 2D array
    map->tiles = (rayMapCell_t**)heap_caps_calloc(map->w, sizeof(rayMapCell_t*), caps);
    for (uint32_t x = 0; x < map->w; x++)
    {
        map->tiles[x] = (rayMapCell_t*)heap_caps_calloc(map->h, sizeof(rayMapCell_t), caps);
    }

    // Allocate space to track what tiles have been visited
    // TODO save and load this from NVM
    map->visitedTiles = (bool*)heap_caps_calloc(map->w * map->h, sizeof(bool), caps);

    // Read tile data
    for (uint32_t y = 0; y < map->h; y++)
    {
        for (uint32_t x = 0; x < map->w; x++)
        {
            // Each tile has a type and object
            map->tiles[x][y].type     = fileData[fileIdx++];
            map->tiles[x][y].doorOpen = 0;
            rayMapCellType_t type     = fileData[fileIdx++];

            // If the type isn't empty
            if (EMPTY != type)
            {
                // Read the type's ID
                uint8_t id = fileData[fileIdx++];
                // If it's the starting point
                if (type == OBJ_ENEMY_START_POINT)
                {
                    // Save the starting coordinates
                    *pStartX = ADD_FX(TO_FX(x), TO_FX_FRAC(1, 2));
                    *pStartY = ADD_FX(TO_FX(y), TO_FX_FRAC(1, 2));
                }
                // If it's an object
                else if ((type & OBJ) == OBJ)
                {
                    // Allocate a new object
                    if ((type & 0x60) == ENEMY)
                    {
                        rayCreateEnemy(ray, type, id, x, y);
                    }
                    else
                    {
                        // TODO check for persistent health & missile upgrades in the inventory before spawning
                        rayCreateCommonObj(ray, type, id, x, y);
                    }
                }
            }
        }
    }

    // Load Scripts
    loadScripts(ray, &fileData[fileIdx], decompressedSize - fileIdx, caps);

    // Free the file data
    free(fileData);
}

/**
 * @brief Create an enemy
 *
 * @param ray The entire game state
 * @param type The type of enemy to spawn
 * @param id The ID for this enemy
 * @param x The X cell position for this enemy
 * @param y The Y cell position for this enemy
 */
void rayCreateEnemy(ray_t* ray, rayMapCellType_t type, int32_t id, int32_t x, int32_t y)
{
    // Allocate the enemy
    rayEnemy_t* newObj = (rayEnemy_t*)heap_caps_calloc(1, sizeof(rayEnemy_t), MALLOC_CAP_SPIRAM);

    // Copy enemy data from the template (sprite indices, type)
    memcpy(newObj, &(ray->eTemplates[type - OBJ_ENEMY_NORMAL]), sizeof(rayEnemy_t));

    // Set ID
    newObj->c.id = id;

    // Set spatial values
    newObj->c.posX   = TO_FX(x) + TO_FX_FRAC(1, 2);
    newObj->c.posY   = TO_FX(y) + TO_FX_FRAC(1, 2);
    newObj->c.radius = TO_FX_FRAC(newObj->c.sprite->w, 2 * TEX_WIDTH);

    // Add it to the linked list
    push(&ray->enemies, newObj);
}

/**
 * @brief Create an object, either scenery or item
 *
 * @param ray The entire game state
 * @param type The type of object to spawn
 * @param id The ID for this object
 * @param x The X cell position for this object
 * @param y The Y cell position for this object
 */
void rayCreateCommonObj(ray_t* ray, rayMapCellType_t type, int32_t id, int32_t x, int32_t y)
{
    rayObjCommon_t* newObj = (rayObjCommon_t*)heap_caps_calloc(1, sizeof(rayObjCommon_t), MALLOC_CAP_SPIRAM);

    // Set type, sprite and ID
    newObj->type   = type;
    newObj->sprite = getTexByType(ray, type);
    newObj->id     = id;

    // Set spatial values
    newObj->posX   = TO_FX(x) + TO_FX_FRAC(1, 2);
    newObj->posY   = TO_FX(y) + TO_FX_FRAC(1, 2);
    newObj->radius = TO_FX_FRAC(newObj->sprite->w, 2 * TEX_WIDTH);

    // Add it to the linked list
    if ((type & 0x60) == ITEM)
    {
        push(&ray->items, newObj);
    }
    else
    {
        push(&ray->scenery, newObj);
    }
}

/**
 * @brief Free an allocated ::rayMap_t
 *
 * @param map the ::rayMap_t to free
 */
void freeRayMap(rayMap_t* map)
{
    // Free each column
    for (uint32_t x = 0; x < map->w; x++)
    {
        free(map->tiles[x]);
    }
    // Free the pointers
    free(map->tiles);
    // Free visited tiles too
    free(map->visitedTiles);
}

/**
 * @brief Check if a cell is currently passable
 *
 * @param cell The cell type to check
 * @return true if the cell can be passed through, false if it cannot
 */
bool isPassableCell(rayMapCell_t* cell)
{
    if (CELL_IS_TYPE(cell->type, BG | WALL))
    {
        // Never pass through walls
        return false;
    }
    else if (CELL_IS_TYPE(cell->type, BG | DOOR))
    {
        // Only pass through open doors
        return (TO_FX(1) == cell->doorOpen);
    }
    else
    {
        // Always pass through everything else
        return true;
    }
}

/**
 * @brief Mark a tile, and surrounding tiles, as visited on the map.
 * Visited tiles are drawn in the pause menu
 *
 * @param map The map to mark tiles visited for
 * @param x The X coordinate of the tile that was visited
 * @param y The Y coordinate of the tile that was visited
 */
void markTileVisited(rayMap_t* map, int16_t x, int16_t y)
{
    // Find in-bounds loop indices
    int16_t minX = MAX(0, x - 1);
    int16_t maxX = MIN(map->w - 1, x + 1);
    int16_t minY = MAX(0, y - 1);
    int16_t maxY = MIN(map->h - 1, y + 1);

    // For a 3x3 grid (inbounds)
    for (int16_t yIdx = minY; yIdx <= maxY; yIdx++)
    {
        for (int16_t xIdx = minX; xIdx <= maxX; xIdx++)
        {
            // Mark these cells as visited
            map->visitedTiles[(yIdx * map->w) + xIdx] = true;
        }
    }
}
