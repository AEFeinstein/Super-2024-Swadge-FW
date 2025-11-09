//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include <esp_log.h>
#include <string.h>
#include <esp_heap_caps.h>

#include "fs_wsg.h"
#include "mgTilemap.h"
#include "mgLeveldef.h"
#include "esp_random.h"
#include "hashMap.h"
#include "mgEntitySpawnData.h"
#include "mega_pulse_ex_typedef.h"
#include "hdw-nvs.h"

#include "cnfs.h"

//==============================================================================
// Function Prototypes
//==============================================================================

// bool isInteractive(uint8_t tileId);

//==============================================================================
// Functions
//==============================================================================

void mg_initializeTileMap(mgTilemap_t* tilemap, mgWsgManager_t* wsgManager)
{
    tilemap->mapOffsetX = 0;
    tilemap->mapOffsetY = 0;

    tilemap->tileSpawnEnabled       = false;
    tilemap->executeTileSpawnColumn = -1;
    tilemap->executeTileSpawnRow    = -1;

    tilemap->animationFrame = 0;
    tilemap->animationTimer = 23;

    tilemap->wsgManager           = wsgManager;
    tilemap->map                  = NULL;
    tilemap->entitySpawns         = NULL;
    tilemap->defaultPlayerSpawn   = NULL;
    tilemap->entitySpawnMap.count = 0;
}

void mg_drawTileMap(mgTilemap_t* tilemap)
{
    tilemap->animationTimer--;
    if (tilemap->animationTimer < 0)
    {
        tilemap->animationFrame = ((tilemap->animationFrame + 1) % 3);
        tilemap->animationTimer = 23;
    }

    for (uint16_t y = (tilemap->mapOffsetY >> MG_TILESIZE_IN_POWERS_OF_2);
         y < (tilemap->mapOffsetY >> MG_TILESIZE_IN_POWERS_OF_2) + MG_TILEMAP_DISPLAY_HEIGHT_TILES; y++)
    {
        if (y >= tilemap->mapHeight)
        {
            break;
        }

        for (uint16_t x = (tilemap->mapOffsetX >> MG_TILESIZE_IN_POWERS_OF_2);
             x < (tilemap->mapOffsetX >> MG_TILESIZE_IN_POWERS_OF_2) + MG_TILEMAP_DISPLAY_WIDTH_TILES; x++)
        {
            if (x >= tilemap->mapWidth)
            {
                break;
            }

            uint8_t tile = tilemap->map[(y * tilemap->mapWidth) + x];

            if (tilemap->tileSpawnEnabled
                && (tilemap->executeTileSpawnColumn == x || tilemap->executeTileSpawnRow == y
                    || tilemap->executeTileSpawnAll))
            {
                uint16_t key = ((y << 8) + (x));
                mgEntitySpawnData_t* entitySpawn
                    = hashGetBin(&(tilemap->entitySpawnMap), (const void*)((uintptr_t)key));

                if (entitySpawn != NULL && entitySpawn->spawnable)
                {
                    ESP_LOGE("MAP", "Spawned entity at tile position %i, %i", x, y);
                    mg_hashSpawnEntity(tilemap->entityManager, entitySpawn);
                }
            }

            if (tile < MG_TILE_GRASS)
            {
                continue;
            }

            // Draw only visible tiles
            if (tile > 31)
            {
                if (tilemap->wsgManager->tiles[tile - 32] == NULL)
                {
                    continue;
                }

                if (tilemap->wsgManager->transparencyFunction(tile))
                {
                    // drawWsgSimpleFast(&tilemap->tiles[tile - 32], x * MG_TILESIZE - tilemap->mapOffsetX, y *
                    // MG_TILESIZE - tilemap->mapOffsetY);
                    drawWsgSimple(tilemap->wsgManager->tiles[tile - 32], x * MG_TILESIZE - tilemap->mapOffsetX,
                                  y * MG_TILESIZE - tilemap->mapOffsetY);
                }
                else
                {
                    drawWsgTile(tilemap->wsgManager->tiles[tile - 32], x * MG_TILESIZE - tilemap->mapOffsetX,
                                y * MG_TILESIZE - tilemap->mapOffsetY);
                }
            }
            /*else*/
        }
    }

    tilemap->executeTileSpawnAll = 0;
}

void mg_scrollTileMap(mgTilemap_t* tilemap, int16_t x, int16_t y)
{
    if (x != 0)
    {
        uint8_t oldTx       = tilemap->mapOffsetX >> MG_TILESIZE_IN_POWERS_OF_2;
        tilemap->mapOffsetX = CLAMP(tilemap->mapOffsetX + x, tilemap->minMapOffsetX, tilemap->maxMapOffsetX);
        uint8_t newTx       = tilemap->mapOffsetX >> MG_TILESIZE_IN_POWERS_OF_2;

        if (newTx > oldTx)
        {
            tilemap->executeTileSpawnColumn = oldTx + MG_TILEMAP_DISPLAY_WIDTH_TILES;
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
        uint8_t oldTy       = tilemap->mapOffsetY >> MG_TILESIZE_IN_POWERS_OF_2;
        tilemap->mapOffsetY = CLAMP(tilemap->mapOffsetY + y, tilemap->minMapOffsetY, tilemap->maxMapOffsetY);
        uint8_t newTy       = tilemap->mapOffsetY >> MG_TILESIZE_IN_POWERS_OF_2;

        if (newTy > oldTy)
        {
            tilemap->executeTileSpawnRow = oldTy + MG_TILEMAP_DISPLAY_HEIGHT_TILES;
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

bool mg_loadMapFromFile(mgTilemap_t* tilemap, cnfsFileIdx_t name)
{
    if (tilemap->entitySpawns != NULL)
    {
        heap_caps_free(tilemap->entitySpawns);
    }

    if (tilemap->entitySpawnMap.count > 0)
    {
        hashDeinit(&(tilemap->entitySpawnMap));
    }

    if (tilemap->map != NULL)
    {
        heap_caps_free(tilemap->map);
    }

    size_t sz;
    uint8_t* buf = NULL;
    ESP_LOGE("MAP", "Loading %i", name);

    if (name == -69)
    {
        if (readNvsBlob("user_level", NULL, &sz))
        {
            buf = heap_caps_malloc(sz, MALLOC_CAP_8BIT);

            if (NULL != buf)
            {
                if (readNvsBlob("user_level", buf, &sz))
                {
                    ESP_LOGE("MAP", "Loading user level...");
                }
            }
        }
    }
    else
    {
        buf = cnfsReadFile(name, &sz, false);
    }

    if (NULL == buf)
    {
        ESP_LOGE("MAP", "Failed to read %i", name);
        return false;
    }

    uint8_t width  = buf[0];
    uint8_t height = buf[1];

    tilemap->map = (uint8_t*)heap_caps_calloc(width * height, sizeof(uint8_t), MALLOC_CAP_SPIRAM);
    memcpy(tilemap->map, &buf[2], width * height);

    tilemap->mapWidth  = width;
    tilemap->mapHeight = height;

    tilemap->minMapOffsetX = 0;
    tilemap->maxMapOffsetX = width * MG_TILESIZE - MG_TILEMAP_DISPLAY_WIDTH_PIXELS;

    tilemap->minMapOffsetY = 0;
    tilemap->maxMapOffsetY = height * MG_TILESIZE - MG_TILEMAP_DISPLAY_HEIGHT_PIXELS;

    /*for (uint16_t i = 0; i < 16; i++)
    {
        tilemap->warps[i].x = buf[2 + width * height + i * 2];
        tilemap->warps[i].y = buf[2 + width * height + i * 2 + 1];
    }*/

    uint32_t iterator        = 2 + (width * height);
    uint16_t numEntitySpawns = (buf[iterator + 1] << 8) + buf[iterator];
    iterator += 2;

    if (numEntitySpawns > 0)
    {
        tilemap->entitySpawns
            = (mgEntitySpawnData_t*)heap_caps_calloc(numEntitySpawns, sizeof(mgEntitySpawnData_t), MALLOC_CAP_SPIRAM);

        hashInitBin(&(tilemap->entitySpawnMap), numEntitySpawns + (numEntitySpawns >> 1), hashInt, intsEq);
        uint16_t subiterator = 0;

        for (uint32_t i = iterator; i < iterator + (numEntitySpawns * 16); i += 16)
        {
            mgEntitySpawnData_t* entitySpawn = &(tilemap->entitySpawns[subiterator]);
            entitySpawn->spawnable           = true;
            entitySpawn->respawnable         = true;
            entitySpawn->type                = buf[i];
            entitySpawn->tx                  = buf[i + 1];
            entitySpawn->ty                  = buf[i + 2];
            entitySpawn->xOffsetInPixels     = buf[i + 3];
            entitySpawn->yOffsetInPixels     = buf[i + 4];
            entitySpawn->flags               = buf[i + 5];
            entitySpawn->spriteRotateAngle   = (buf[i + 7] << 8) + buf[i + 6];
            entitySpawn->special2            = buf[i + 8];
            entitySpawn->special3            = buf[i + 9];
            entitySpawn->special4            = buf[i + 10];
            entitySpawn->special5            = buf[i + 11];
            entitySpawn->special6            = buf[i + 12];
            entitySpawn->special7            = buf[i + 13];

            uint16_t linkedEntitySpawnIndex = (buf[i + 15] << 8) + buf[i + 14];

            ESP_LOGE("TEST", "Entity #%i: type %i", subiterator, entitySpawn->type);
            ESP_LOGE("TEST", "specials 2-7: %i %i %i %i %i %i",
                     entitySpawn->special2, entitySpawn->special3, entitySpawn->special4, entitySpawn->special5,
                     entitySpawn->special6, entitySpawn->special7);

            if (linkedEntitySpawnIndex == 0xffff)
            {
                entitySpawn->linkedEntitySpawn = NULL;
                ESP_LOGE("TEST", "Linked entity not found for %i, %i", subiterator, linkedEntitySpawnIndex);
            }
            else
            {
                entitySpawn->linkedEntitySpawn = &(tilemap->entitySpawns[linkedEntitySpawnIndex]);
                ESP_LOGE("TEST", "Linked entity found for %i, %i", subiterator, linkedEntitySpawnIndex);
            }

            uint16_t key = (entitySpawn->ty << 8) + (entitySpawn->tx);
            hashPutBin(&(tilemap->entitySpawnMap), (const void*)((uintptr_t)key), (void*)entitySpawn);

            switch (entitySpawn->type)
            {
                case ENTITY_PLAYER:
                    entitySpawn->spawnable      = false;
                    entitySpawn->respawnable    = false;
                    tilemap->defaultPlayerSpawn = entitySpawn;
                    break;
                case ENTITY_WARP_EXIT_FLOOR:
                case ENTITY_WARP_EXIT_WALL:
                    entitySpawn->spawnable   = false;
                    entitySpawn->respawnable = false;
                    break;
                default:
                    break;
            }

            subiterator++;
        }
    }
    else
    {
        tilemap->entitySpawns = NULL;
    }

    heap_caps_free(buf);

    return true;
}

void mg_tileSpawnEntity(mgTilemap_t* tilemap, uint8_t objectIndex, uint8_t tx, uint8_t ty)
{
    mgEntity_t* entityCreated
        = mg_createEntity(tilemap->entityManager, objectIndex, (tx << MG_TILESIZE_IN_POWERS_OF_2) + 8,
                          (ty << MG_TILESIZE_IN_POWERS_OF_2) + 8);

    if (entityCreated != NULL)
    {
        entityCreated->homeTileX                  = tx;
        entityCreated->homeTileY                  = ty;
        tilemap->map[ty * tilemap->mapWidth + tx] = 0;
    }
}

void mg_hashSpawnEntity(mgEntityManager_t* entityManager, mgEntitySpawnData_t* entitySpawnData)
{
    mgEntity_t* entityCreated
        = mg_createEntity(entityManager, entitySpawnData->type,
                          (entitySpawnData->tx << MG_TILESIZE_IN_POWERS_OF_2) + entitySpawnData->xOffsetInPixels,
                          (entitySpawnData->ty << MG_TILESIZE_IN_POWERS_OF_2) + entitySpawnData->yOffsetInPixels);

    if (entityCreated != NULL)
    {
        entityCreated->spriteFlipHorizontal = entitySpawnData->flags & 0b1;
        entityCreated->spriteFlipVertical   = entitySpawnData->flags & 0b10;
        entityCreated->spriteRotateAngle    = entitySpawnData->spriteRotateAngle;

        entityCreated->spawnData = entitySpawnData;

        if (entitySpawnData->linkedEntitySpawn != NULL)
        {
            entityCreated->linkedEntity = entitySpawnData->linkedEntitySpawn->spawnedEntity;
        }

        entitySpawnData->spawnedEntity = entityCreated;

        entitySpawnData->spawnable = false;
    }
}

uint8_t mg_getTile(mgTilemap_t* tilemap, uint8_t tx, uint8_t ty)
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

void mg_setTile(mgTilemap_t* tilemap, uint8_t tx, uint8_t ty, uint8_t newTileId)
{
    // ty = CLAMP(ty, 0, tilemap->mapHeight - 1);

    if (ty >= tilemap->mapHeight || tx >= tilemap->mapWidth)
    {
        return;
    }

    tilemap->map[ty * tilemap->mapWidth + tx] = newTileId;
}

bool mg_isSolid(uint8_t tileId)
{
    switch (tileId)
    {
        case MG_TILE_EMPTY ... MG_TILE_UNUSED_29:
            return false;
            break;
        case MG_TILE_INVISIBLE_BLOCK ... MG_TILE_SOLID_VISIBLE_INTERACTIVE_9F:
            return true;
            break;
        default:
            return false;
    }
}

// bool isInteractive(uint8_t tileId)
// {
//     return tileId > MG_TILEINVISIBLE_BLOCK && tileId < MG_TILEBG_GOAL_ZONE;
// }

void mg_unlockScrolling(mgTilemap_t* tilemap)
{
    tilemap->minMapOffsetX = 0;
    tilemap->maxMapOffsetX = tilemap->mapWidth * MG_TILESIZE - MG_TILEMAP_DISPLAY_WIDTH_PIXELS;

    tilemap->minMapOffsetY = 0;
    tilemap->maxMapOffsetY = tilemap->mapHeight * MG_TILESIZE - MG_TILEMAP_DISPLAY_HEIGHT_PIXELS;
}

bool mg_needsTransparency(uint8_t tileId)
{
    switch (tileId)
    {
        case MG_TILE_BOUNCE_BLOCK:
        case MG_TILEGIRDER:
        case MG_TILE_CONTAINER_1 ... MG_TILE_CONTAINER_3:
        case MG_TILE_COIN_1 ... MG_TILE_COIN_3:
        case MG_TILE_LADDER:
            // case MG_TILE_BG_GOAL_ZONE ... MG_TILE_BG_CLOUD_D:
            return true;
        case MG_TILE_BG_CLOUD:
            return false;
        case MG_TILE_BG_TALL_GRASS ... MG_TILE_BG_MOUNTAIN_R:
            return true;
        case MG_TILE_BG_MOUNTAIN ... MG_TILE_BG_METAL:
            return false;
        case MG_TILE_BG_CHAINS:
            return true;
        case MG_TILE_BG_WALL:
            return false;
        default:
            return false;
    }
}

void mg_freeTilemap(mgTilemap_t* tilemap)
{
    heap_caps_free(tilemap->map);
}