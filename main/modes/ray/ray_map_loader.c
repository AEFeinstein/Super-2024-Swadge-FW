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

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Load a RMH from ROM to RAM. RMHs placed in the spiffs_image folder
 * before compilation will be automatically flashed to ROM
 *
 * @param name The filename of the RMH to load
 * @param map The map to load into
 * @param objs TODO
 * @param startX Where the starting X coordinate is loaded into
 * @param startY Where the starting Y coordinate is loaded into
 * @param spiRam true to load to SPI RAM, false to load to normal RAM. SPI RAM is more plentiful but slower to access
 * than normal RAM
 */
void loadRayMap(const char* name, rayMap_t* map, rayObj_t* objs, int32_t* startX, int32_t* startY, bool spiRam)
{
    // Pick the allocation type
    uint32_t caps = spiRam ? MALLOC_CAP_SPIRAM : MALLOC_CAP_DEFAULT;

    // Read and decompress the file
    uint32_t decompressedSize = 0;
    uint8_t* fileData         = readHeatshrinkFile(name, &decompressedSize, spiRam);
    uint32_t fileIdx          = 0;

    // Read the width and height
    map->w = fileData[fileIdx++];
    map->h = fileData[fileIdx++];

    // Allocate the tiles, 2D array
    map->tiles = (rayMapCellType_t**)heap_caps_calloc(map->w, sizeof(rayMapCellType_t*), caps);
    for (uint16_t x = 0; x < map->w; x++)
    {
        map->tiles[x] = (rayMapCellType_t*)heap_caps_calloc(map->h, sizeof(rayMapCellType_t), caps);
    }

    int16_t objIdx = 0;

    // Read tile data
    for (uint16_t y = 0; y < map->h; y++)
    {
        for (uint16_t x = 0; x < map->w; x++)
        {
            // Each tile has a type and object
            map->tiles[x][y]     = fileData[fileIdx++];
            rayMapCellType_t obj = fileData[fileIdx++];

            // If the object isn't empty
            if (EMPTY != obj)
            {
                // Read the object's ID
                uint8_t id = fileData[fileIdx++];
                switch (obj)
                {
                    case EMPTY:
                    case BG_WALL:
                    case BG_DOOR:
                    case BG_FLOOR:
                    case BG_CEILING:
                    case OBJ_DELETE:
                    {
                        // Not really objects
                        break;
                    }
                    case OBJ_START_POINT:
                    {
                        // Save the starting coordinates
                        *startX = x;
                        *startY = y;
                        break;
                    }
                    case OBJ_ENEMY_DRAGON:
                    case OBJ_ENEMY_SKELETON:
                    case OBJ_ENEMY_KNIGHT:
                    case OBJ_ENEMY_GOLEM:
                    case OBJ_OBELISK:
                    case OBJ_GUN:
                    {
                        objs[objIdx].sprite = getTexture(obj);
                        objs[objIdx].dist   = 0;
                        objs[objIdx].posX   = TO_FX(x);
                        objs[objIdx].posY   = TO_FX(y);
                        objs[objIdx].velX   = TO_FX(0);
                        objs[objIdx].velY   = TO_FX(0);
                        objs[objIdx].radius = DIV_FX(TO_FX(objs[objIdx].sprite->w), TO_FX(64)); // each cell is 64px
                        objs[objIdx].type   = obj;
                        objs[objIdx].id     = id;
                        objIdx++;
                        // TODO save these object types or something
                        break;
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
