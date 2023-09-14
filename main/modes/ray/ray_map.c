//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include <esp_log.h>
#include <esp_heap_caps.h>

#include "hdw-spiffs.h"
#include "heatshrink_helper.h"
#include "ray_map.h"
#include "ray_tex_manager.h"
#include "ray_renderer.h"

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Load a RMH from ROM to RAM. RMHs placed in the spiffs_image folder
 * before compilation will be automatically flashed to ROM
 *
 * @param name The filename of the RMH to load
 * @param ray The ray_t to load the map into
 * @param spiRam true to load to SPI RAM, false to load to normal RAM. SPI RAM is more plentiful but slower to access
 * than normal RAM
 */
void loadRayMap(const char* name, ray_t* ray, bool spiRam)
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
                    ray->posX = ADD_FX(TO_FX(x), TO_FX_FRAC(1, 2));
                    ray->posY = ADD_FX(TO_FX(y), TO_FX_FRAC(1, 2));
                }
                // If it's an object
                else if ((type & OBJ) == OBJ)
                {
                    // Allocate a new object
                    if ((type & 0x60) == ENEMY)
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
                    else
                    {
                        // TODO check for persistent health & missile upgrades in the inventory before spawning
                        rayObjCommon_t* newObj
                            = (rayObjCommon_t*)heap_caps_calloc(1, sizeof(rayObjCommon_t), MALLOC_CAP_SPIRAM);

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
                }
            }
        }
    }

    // TODO load rules!!

    // Free the file data
    free(fileData);
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
