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
#include "ray_map_loader.h"
#include "ray_tex_manager.h"

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
    q24_8* startX = &ray->posX;
    q24_8* startY = &ray->posY;

    // Read and decompress the file
    uint32_t decompressedSize = 0;
    uint8_t* fileData         = readHeatshrinkFile(name, &decompressedSize, spiRam);
    uint32_t fileIdx          = 0;

    // Read the width and height
    map->w = fileData[fileIdx++];
    map->h = fileData[fileIdx++];

    // Allocate the tiles, 2D array
    map->tiles = (rayMapCell_t**)heap_caps_calloc(map->w, sizeof(rayMapCell_t*), caps);
    for (uint16_t x = 0; x < map->w; x++)
    {
        map->tiles[x] = (rayMapCell_t*)heap_caps_calloc(map->h, sizeof(rayMapCell_t), caps);
    }

    // Read tile data
    for (uint16_t y = 0; y < map->h; y++)
    {
        for (uint16_t x = 0; x < map->w; x++)
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
                    *startX = x;
                    *startY = y;
                }
                // If it's an object
                else if ((type & OBJ) == OBJ)
                {
                    // Allocate a new object
                    if ((type & ENEMY) == ENEMY)
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
                        newObj->c.radius = DIV_FX(TO_FX(newObj->c.sprite->w), TO_FX(64)); // each cell is 64px

                        // Add it to the linked list
                        push(&ray->enemies, newObj);
                    }
                    else
                    {
                        rayScenery_t* newObj
                            = (rayScenery_t*)heap_caps_calloc(1, sizeof(rayScenery_t), MALLOC_CAP_SPIRAM);

                        // Set type, sprite and ID
                        newObj->c.type   = type;
                        newObj->c.sprite = getTexByType(ray, type);
                        newObj->c.id     = id;

                        // Set spatial values
                        newObj->c.posX   = TO_FX(x) + TO_FX_FRAC(1, 2);
                        newObj->c.posY   = TO_FX(y) + TO_FX_FRAC(1, 2);
                        newObj->c.radius = DIV_FX(TO_FX(newObj->c.sprite->w), TO_FX(64)); // each cell is 64px

                        // Add it to the linked list
                        push(&ray->scenery, newObj);
                    }
                }
            }
        }
    }

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
    for (uint16_t x = 0; x < map->w; x++)
    {
        free(map->tiles[x]);
    }
    // Free the pointers
    free(map->tiles);
}
