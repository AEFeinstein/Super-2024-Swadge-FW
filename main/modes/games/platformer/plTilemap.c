//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include <esp_log.h>
#include <string.h>
#include <esp_heap_caps.h>

#include "fs_wsg.h"
#include "plTilemap.h"
#include "plLeveldef.h"
#include "esp_random.h"

#include "cnfs.h"

//==============================================================================
// Function Prototypes
//==============================================================================

// bool isInteractive(uint8_t tileId);

//==============================================================================
// Functions
//==============================================================================

void pl_initializeTileMap(plTilemap_t* tilemap, plWsgManager_t* wsgManager)
{
    tilemap->mapOffsetX = 0;
    tilemap->mapOffsetY = 0;

    tilemap->tileSpawnEnabled       = false;
    tilemap->executeTileSpawnColumn = -1;
    tilemap->executeTileSpawnRow    = -1;

    tilemap->animationFrame = 0;
    tilemap->animationTimer = 23;

    tilemap->wsgManager = wsgManager;
}

void pl_drawTileMap(plTilemap_t* tilemap)
{
    tilemap->animationTimer--;
    if (tilemap->animationTimer < 0)
    {
        tilemap->animationFrame = ((tilemap->animationFrame + 1) % 3);
        tilemap->animationTimer = 23;
    }

    for (uint16_t y = (tilemap->mapOffsetY >> PL_TILESIZE_IN_POWERS_OF_2);
         y < (tilemap->mapOffsetY >> PL_TILESIZE_IN_POWERS_OF_2) + PL_TILEMAP_DISPLAY_HEIGHT_TILES; y++)
    {
        if (y >= tilemap->mapHeight)
        {
            break;
        }

        for (uint16_t x = (tilemap->mapOffsetX >> PL_TILESIZE_IN_POWERS_OF_2);
             x < (tilemap->mapOffsetX >> PL_TILESIZE_IN_POWERS_OF_2) + PL_TILEMAP_DISPLAY_WIDTH_TILES; x++)
        {
            if (x >= tilemap->mapWidth)
            {
                break;
            }

            uint8_t tile = tilemap->map[(y * tilemap->mapWidth) + x];

            if (tile < PL_TILE_GRASS)
            {
                continue;
            }

            // Test animated tiles
            if (tile == 64 || tile == 67)
            {
                tile += tilemap->animationFrame;
            }

            // Draw only non-garbage tiles
            if (tile > 31 && tile < 104)
            {
                if (pl_needsTransparency(tile))
                {
                    // drawWsgSimpleFast(&tilemap->tiles[tile - 32], x * PL_TILESIZE - tilemap->mapOffsetX, y *
                    // PL_TILESIZE - tilemap->mapOffsetY);
                    drawWsgSimple(tilemap->wsgManager->tiles[tile - 32], x * PL_TILESIZE - tilemap->mapOffsetX,
                                  y * PL_TILESIZE - tilemap->mapOffsetY);
                }
                else
                {
                    drawWsgTile(tilemap->wsgManager->tiles[tile - 32], x * PL_TILESIZE - tilemap->mapOffsetX,
                                y * PL_TILESIZE - tilemap->mapOffsetY);
                }
            }
            else if (tile > 127 && tilemap->tileSpawnEnabled
                     && (tilemap->executeTileSpawnColumn == x || tilemap->executeTileSpawnRow == y
                         || tilemap->executeTileSpawnAll))
            {
                pl_tileSpawnEntity(tilemap, tile - 128, x, y);
            }
        }
    }

    tilemap->executeTileSpawnAll = 0;
}

void pl_scrollTileMap(plTilemap_t* tilemap, int16_t x, int16_t y)
{
    if (x != 0)
    {
        uint8_t oldTx       = tilemap->mapOffsetX >> PL_TILESIZE_IN_POWERS_OF_2;
        tilemap->mapOffsetX = CLAMP(tilemap->mapOffsetX + x, tilemap->minMapOffsetX, tilemap->maxMapOffsetX);
        uint8_t newTx       = tilemap->mapOffsetX >> PL_TILESIZE_IN_POWERS_OF_2;

        if (newTx > oldTx)
        {
            tilemap->executeTileSpawnColumn = oldTx + PL_TILEMAP_DISPLAY_WIDTH_TILES;
        }
        else if (newTx < oldTx)
        {
            tilemap->executeTileSpawnColumn = newTx;
        }
        else
        {
            tilemap->executeTileSpawnColumn = -1;
        }
    }

    if (y != 0)
    {
        uint8_t oldTy       = tilemap->mapOffsetY >> PL_TILESIZE_IN_POWERS_OF_2;
        tilemap->mapOffsetY = CLAMP(tilemap->mapOffsetY + y, tilemap->minMapOffsetY, tilemap->maxMapOffsetY);
        uint8_t newTy       = tilemap->mapOffsetY >> PL_TILESIZE_IN_POWERS_OF_2;

        if (newTy > oldTy)
        {
            tilemap->executeTileSpawnRow = oldTy + PL_TILEMAP_DISPLAY_HEIGHT_TILES;
        }
        else if (newTy < oldTy)
        {
            tilemap->executeTileSpawnRow = newTy;
        }
        else
        {
            tilemap->executeTileSpawnRow = -1;
        }
    }
}

bool pl_loadMapFromFile(plTilemap_t* tilemap, const char* name)
{
    if (tilemap->map != NULL)
    {
        heap_caps_free(tilemap->map);
    }

    size_t sz;
    uint8_t* buf = cnfsReadFile(name, &sz, false);

    if (NULL == buf)
    {
        ESP_LOGE("MAP", "Failed to read %s", name);
        return false;
    }

    uint8_t width  = buf[0];
    uint8_t height = buf[1];

    tilemap->map = (uint8_t*)heap_caps_calloc(width * height, sizeof(uint8_t), MALLOC_CAP_SPIRAM);
    memcpy(tilemap->map, &buf[2], width * height);

    tilemap->mapWidth  = width;
    tilemap->mapHeight = height;

    tilemap->minMapOffsetX = 0;
    tilemap->maxMapOffsetX = width * PL_TILESIZE - PL_TILEMAP_DISPLAY_WIDTH_PIXELS;

    tilemap->minMapOffsetY = 0;
    tilemap->maxMapOffsetY = height * PL_TILESIZE - PL_TILEMAP_DISPLAY_HEIGHT_PIXELS;

    for (uint16_t i = 0; i < 16; i++)
    {
        tilemap->warps[i].x = buf[2 + width * height + i * 2];
        tilemap->warps[i].y = buf[2 + width * height + i * 2 + 1];
    }

    heap_caps_free(buf);

    return true;
}

void pl_tileSpawnEntity(plTilemap_t* tilemap, uint8_t objectIndex, uint8_t tx, uint8_t ty)
{
    plEntity_t* entityCreated
        = pl_createEntity(tilemap->entityManager, objectIndex, (tx << PL_TILESIZE_IN_POWERS_OF_2) + 8,
                          (ty << PL_TILESIZE_IN_POWERS_OF_2) + 8);

    if (entityCreated != NULL)
    {
        entityCreated->homeTileX                  = tx;
        entityCreated->homeTileY                  = ty;
        tilemap->map[ty * tilemap->mapWidth + tx] = 0;
    }
}

uint8_t pl_getTile(plTilemap_t* tilemap, uint8_t tx, uint8_t ty)
{
    // ty = CLAMP(ty, 0, tilemap->mapHeight - 1);

    if (/*ty < 0 ||*/ ty >= tilemap->mapHeight)
    {
        ty = 0;
        // return 0;
    }

    if (/*tx < 0 ||*/ tx >= tilemap->mapWidth)
    {
        return 1;
    }

    return tilemap->map[ty * tilemap->mapWidth + tx];
}

void pl_setTile(plTilemap_t* tilemap, uint8_t tx, uint8_t ty, uint8_t newTileId)
{
    // ty = CLAMP(ty, 0, tilemap->mapHeight - 1);

    if (ty >= tilemap->mapHeight || tx >= tilemap->mapWidth)
    {
        return;
    }

    tilemap->map[ty * tilemap->mapWidth + tx] = newTileId;
}

bool pl_isSolid(uint8_t tileId)
{
    switch (tileId)
    {
        case PL_TILE_EMPTY ... PL_TILE_UNUSED_29:
            return false;
            break;
        case PL_TILE_INVISIBLE_BLOCK ... PL_TILE_METAL_PIPE_V:
            return true;
            break;
        case PL_TILE_BOUNCE_BLOCK:
            return false;
            break;
        case PL_TILE_DIRT_PATH ... PL_TILE_CONTAINER_3:
            return true;
            break;
        default:
            return false;
    }
}

// bool isInteractive(uint8_t tileId)
// {
//     return tileId > PL_TILEINVISIBLE_BLOCK && tileId < PL_TILEBG_GOAL_ZONE;
// }

void pl_unlockScrolling(plTilemap_t* tilemap)
{
    tilemap->minMapOffsetX = 0;
    tilemap->maxMapOffsetX = tilemap->mapWidth * PL_TILESIZE - PL_TILEMAP_DISPLAY_WIDTH_PIXELS;

    tilemap->minMapOffsetY = 0;
    tilemap->maxMapOffsetY = tilemap->mapHeight * PL_TILESIZE - PL_TILEMAP_DISPLAY_HEIGHT_PIXELS;
}

bool pl_needsTransparency(uint8_t tileId)
{
    switch (tileId)
    {
        case PL_TILE_BOUNCE_BLOCK:
        case PL_TILEGIRDER:
        case PL_TILE_CONTAINER_1 ... PL_TILE_CONTAINER_3:
        case PL_TILE_COIN_1 ... PL_TILE_COIN_3:
        case PL_TILE_LADDER:
        case PL_TILE_BG_GOAL_ZONE ... PL_TILE_BG_CLOUD_D:
            return true;
        case PL_TILE_BG_CLOUD:
            return false;
        case PL_TILE_BG_TALL_GRASS ... PL_TILE_BG_MOUNTAIN_R:
            return true;
        case PL_TILE_BG_MOUNTAIN ... PL_TILE_BG_METAL:
            return false;
        case PL_TILE_BG_CHAINS:
            return true;
        case PL_TILE_BG_WALL:
            return false;
        default:
            return false;
    }
}

void pl_freeTilemap(plTilemap_t* tilemap)
{
    heap_caps_free(tilemap->map);
}