//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include <esp_log.h>
#include <string.h>
#include <esp_heap_caps.h>

#include "fs_wsg.h"
#include "paTilemap.h"
#include "esp_random.h"

#include "cnfs.h"

//==============================================================================
// Function Prototypes
//==============================================================================

// bool isInteractive(uint8_t tileId);

//==============================================================================
// Functions
//==============================================================================

void pa_initializeTileMap(paTilemap_t* tilemap, paWsgManager_t* wsgManager)
{
    tilemap->mapOffsetX = 0;
    tilemap->mapOffsetY = 0;

    tilemap->tileSpawnEnabled       = false;
    tilemap->executeTileSpawnColumn = -1;
    tilemap->executeTileSpawnRow    = -1;

    tilemap->wsgManager = wsgManager;
}

void pa_drawTileMap(paTilemap_t* tilemap)
{
    for (int32_t y = (tilemap->mapOffsetY >> PA_TILE_SIZE_IN_POWERS_OF_2);
         y < (tilemap->mapOffsetY >> PA_TILE_SIZE_IN_POWERS_OF_2) + PA_TILE_MAP_DISPLAY_HEIGHT_TILES; y++)
    {
        if (y >= tilemap->mapHeight)
        {
            break;
        }

        for (int32_t x = (tilemap->mapOffsetX >> PA_TILE_SIZE_IN_POWERS_OF_2);
             x < (tilemap->mapOffsetX >> PA_TILE_SIZE_IN_POWERS_OF_2) + PA_TILE_MAP_DISPLAY_WIDTH_TILES; x++)
        {
            if (x >= tilemap->mapWidth)
            {
                break;
            }
            else if (x < 0)
            {
                continue;
            }

            uint8_t tile = tilemap->map[(y * tilemap->mapWidth) + x];

            if (tile < PA_TILE_WALL_0 || tile == PA_TILE_INVISIBLE_BLOCK)
            {
                continue;
            }

            // Draw only non-garbage tiles
            if (tile > 0 && tile < 13)
            {
                if (pa_needsTransparency(tile))
                {
                    // drawWsgSimpleFast(&tilemap->tiles[tile - 32], x * PA_TILE_SIZE - tilemap->mapOffsetX, y *
                    // PA_TILE_SIZE - tilemap->mapOffsetY);
                    drawWsgSimple(tilemap->wsgManager->tiles[tile - 1], x * PA_TILE_SIZE - tilemap->mapOffsetX,
                                  y * PA_TILE_SIZE - tilemap->mapOffsetY);
                }
                else
                {
                    drawWsgTile(tilemap->wsgManager->tiles[tile - 1], x * PA_TILE_SIZE - tilemap->mapOffsetX,
                                y * PA_TILE_SIZE - tilemap->mapOffsetY);
                }
            }
            else if (tile > 127 && tilemap->tileSpawnEnabled
                     && (tilemap->executeTileSpawnColumn == x || tilemap->executeTileSpawnRow == y
                         || tilemap->executeTileSpawnAll))
            {
                pa_tileSpawnEntity(tilemap, tile - 128, x, y);
            }
        }
    }

    tilemap->executeTileSpawnAll = 0;
}

void pa_scrollTileMap(paTilemap_t* tilemap, int16_t x, int16_t y)
{
    if (x != 0)
    {
        uint8_t oldTx       = tilemap->mapOffsetX >> PA_TILE_SIZE_IN_POWERS_OF_2;
        tilemap->mapOffsetX = CLAMP(tilemap->mapOffsetX + x, tilemap->minMapOffsetX, tilemap->maxMapOffsetX);
        uint8_t newTx       = tilemap->mapOffsetX >> PA_TILE_SIZE_IN_POWERS_OF_2;

        if (newTx > oldTx)
        {
            tilemap->executeTileSpawnColumn = oldTx + PA_TILE_MAP_DISPLAY_WIDTH_TILES;
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
        uint8_t oldTy       = tilemap->mapOffsetY >> PA_TILE_SIZE_IN_POWERS_OF_2;
        tilemap->mapOffsetY = CLAMP(tilemap->mapOffsetY + y, tilemap->minMapOffsetY, tilemap->maxMapOffsetY);
        uint8_t newTy       = tilemap->mapOffsetY >> PA_TILE_SIZE_IN_POWERS_OF_2;

        if (newTy > oldTy)
        {
            tilemap->executeTileSpawnRow = oldTy + PA_TILE_MAP_DISPLAY_HEIGHT_TILES;
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

bool pa_loadMapFromFile(paTilemap_t* tilemap, const char* name)
{
    if (tilemap->map != NULL)
    {
        free(tilemap->map);
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
    tilemap->maxMapOffsetX = width * PA_TILE_SIZE - PA_TILE_MAP_DISPLAY_WIDTH_PIXELS;

    tilemap->minMapOffsetY = 0;
    tilemap->maxMapOffsetY = height * PA_TILE_SIZE - PA_TILE_MAP_DISPLAY_HEIGHT_PIXELS;

    free(buf);

    return true;
}

void pa_tileSpawnEntity(paTilemap_t* tilemap, uint8_t objectIndex, uint8_t tx, uint8_t ty)
{
    paEntity_t* entityCreated
        = pa_createEntity(tilemap->entityManager, objectIndex, (tx << PA_TILE_SIZE_IN_POWERS_OF_2) + 8,
                          (ty << PA_TILE_SIZE_IN_POWERS_OF_2) + 8);

    if (entityCreated != NULL)
    {
        entityCreated->homeTileX                  = tx;
        entityCreated->homeTileY                  = ty;
        tilemap->map[ty * tilemap->mapWidth + tx] = 0;
    }
}

uint8_t pa_getTile(paTilemap_t* tilemap, uint8_t tx, uint8_t ty)
{
    // ty = CLAMP(ty, 0, tilemap->mapHeight - 1);

    if (/*ty < 0 ||*/ ty >= tilemap->mapHeight)
    {
        // ty = 0;
        return 0;
    }

    if (/*tx < 0 ||*/ tx >= tilemap->mapWidth)
    {
        return 0;
    }

    return tilemap->map[ty * tilemap->mapWidth + tx];
}

void pa_setTile(paTilemap_t* tilemap, uint8_t tx, uint8_t ty, uint8_t newTileId)
{
    // ty = CLAMP(ty, 0, tilemap->mapHeight - 1);

    if (ty >= tilemap->mapHeight || tx >= tilemap->mapWidth)
    {
        return;
    }

    tilemap->map[ty * tilemap->mapWidth + tx] = newTileId;
}

bool pa_isSolid(uint8_t tileId)
{
    switch (tileId)
    {
        case PA_TILE_EMPTY:
            return false;
            break;
        default:
            return true;
    }
}

void pa_unlockScrolling(paTilemap_t* tilemap)
{
    tilemap->minMapOffsetX = 0;
    tilemap->maxMapOffsetX = tilemap->mapWidth * PA_TILE_SIZE - PA_TILE_MAP_DISPLAY_WIDTH_PIXELS;

    tilemap->minMapOffsetY = 0;
    tilemap->maxMapOffsetY = tilemap->mapHeight * PA_TILE_SIZE - PA_TILE_MAP_DISPLAY_HEIGHT_PIXELS;
}

bool pa_needsTransparency(uint8_t tileId)
{
    // TODO
    // Currently, all tiles need transparency.
    // So get rid of this then?

    switch (tileId)
    {
        /*case PA_TILE_BOUNCE_BLOCK:
        case PA_TILE_GIRDER:
        case PA_TILE_CONTAINER_1 ... PA_TILE_CONTAINER_3:
        case PA_TILE_COIN_1 ... PA_TILE_COIN_3:
        case PA_TILE_LADDER:
        case PA_TILE_BG_GOAL_ZONE ... PA_TILE_BG_CLOUD_D:
            return true;
        case PA_TILE_BG_CLOUD:
            return false;
        case PA_TILE_BG_TALL_GRASS ... PA_TILE_BG_MOUNTAIN_R:
            return true;
        case PA_TILE_BG_MOUNTAIN ... PA_TILE_BG_METAL:
            return false;
        case PA_TILE_BG_CHAINS:
            return true;
        case PA_TILE_BG_WALL:
            return false;*/
        default:
            return false;
    }
}

void pa_freeTilemap(paTilemap_t* tilemap)
{
    free(tilemap->map);
}

void pa_generateMaze(paTilemap_t* tilemap)
{
    tilemap->entityManager->gameData->remainingBlocks = 195;
    int32_t tx = 1;
    int32_t ty = 13;
    
    pa_setTile(tilemap, tx, ty, PA_TILE_EMPTY);
    tilemap->entityManager->gameData->remainingBlocks--;

    while (ty > 1)
    {
        tx = 1;
        while (tx < 15)
        {
            if (!pa_getTile(tilemap, tx, ty) && !pa_genPathContinue(tilemap, tx, ty))
            {
                pa_genMakePath(tilemap, tx, ty);
            }
            tx += 2;
        }
        ty -= 2;
    }

    tilemap->entityManager->gameData->levelBlocks = tilemap->entityManager->gameData->remainingBlocks;
    tilemap->entityManager->gameData->firstBonusItemDispenseThreshold = tilemap->entityManager->gameData->levelBlocks - (tilemap->entityManager->gameData->levelBlocks >> 2);
    tilemap->entityManager->gameData->secondBonusItemDispenseThreshold = tilemap->entityManager->gameData->levelBlocks - 3*(tilemap->entityManager->gameData->levelBlocks >> 2);}


bool pa_genPathContinue(paTilemap_t* tilemap, uint32_t x, uint32_t y)
{
    if (pa_getTile(tilemap, x, y - 2))
    {
        return false;
    }
    if (pa_getTile(tilemap, x, y + 2))
    {
        return false;
    }
    if (pa_getTile(tilemap, x + 2, y))
    {
        return false;
    }
    if (pa_getTile(tilemap, x - 2, y))
    {
        return false;
    }

    return true;
}

void pa_genMakePath(paTilemap_t* tilemap, uint32_t x, uint32_t y)
{
    bool done   = 0;
    uint32_t nx = x;
    uint32_t ny = y;

    while (!done)
    {
        uint32_t r = esp_random() % 4;

        switch (r)
        {
            case 0:
                if (pa_getTile(tilemap, nx, ny - 2))
                {
                    pa_setTile(tilemap, nx, ny - 1, PA_TILE_EMPTY);
                    pa_setTile(tilemap, nx, ny - 2, PA_TILE_EMPTY);
                    ny -= 2;
                    tilemap->entityManager->gameData->remainingBlocks-=2;
                }
                break;
            case 1:
                if (pa_getTile(tilemap, nx, ny + 2))
                {
                    pa_setTile(tilemap, nx, ny + 1, PA_TILE_EMPTY);
                    pa_setTile(tilemap, nx, ny + 2, PA_TILE_EMPTY);
                    ny += 2;
                    tilemap->entityManager->gameData->remainingBlocks-=2;
                }
                break;
            case 2:
                if (pa_getTile(tilemap, nx - 2, ny))
                {
                    pa_setTile(tilemap, nx - 1, ny, PA_TILE_EMPTY);
                    pa_setTile(tilemap, nx - 2, ny, PA_TILE_EMPTY);
                    nx -= 2;
                    tilemap->entityManager->gameData->remainingBlocks-=2;
                }
                break;
            case 3:
                if (pa_getTile(tilemap, nx + 2, ny))
                {
                    pa_setTile(tilemap, nx + 1, ny, PA_TILE_EMPTY);
                    pa_setTile(tilemap, nx + 2, ny, PA_TILE_EMPTY);
                    nx += 2;
                    tilemap->entityManager->gameData->remainingBlocks-=2;
                }
                break;
        }

        done = pa_genPathContinue(tilemap, nx, ny);
    }
}

void pa_placeEnemySpawns(paTilemap_t* tilemap)
{
    int16_t enemySpawnsToPlace = tilemap->entityManager->gameData->remainingEnemies;
    int16_t enemiesPlaced      = 0;
    bool previouslyPlaced      = false;
    int16_t iterations         = 0;

    // Place enemy spawn blocks
    while (enemySpawnsToPlace > 0 && iterations < 16)
    {
        for (uint16_t ty = 1; ty < 13; ty++)
        {
            for (uint16_t tx = 1; tx < 15; tx++)
            {
                if (enemySpawnsToPlace <= 0)
                {
                    break;
                }

                uint8_t t = pa_getTile(tilemap, tx, ty);

                if (t == PA_TILE_BLOCK && !previouslyPlaced && !(esp_random() % 15))
                {
                    pa_setTile(tilemap, tx, ty, PA_TILE_SPAWN_BLOCK_0);
                    enemySpawnsToPlace--;
                    enemiesPlaced++;
                    previouslyPlaced = true;
                }
                else
                {
                    previouslyPlaced = false;
                }
            }
        }
        iterations++;
    }

    tilemap->entityManager->gameData->remainingEnemies = enemiesPlaced;
}