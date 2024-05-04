//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include <esp_log.h>
#include <string.h>
#include <esp_heap_caps.h>

#include "spiffs_wsg.h"
#include "plTilemap.h"
#include "plLeveldef.h"
#include "esp_random.h"

#include "hdw-spiffs.h"

//==============================================================================
// Function Prototypes
//==============================================================================

// bool isInteractive(uint8_t tileId);

//==============================================================================
// Functions
//==============================================================================

void pl_initializeTileMap(plTilemap_t* tilemap)
{
    tilemap->mapOffsetX = 0;
    tilemap->mapOffsetY = 0;

    tilemap->tileSpawnEnabled       = false;
    tilemap->executeTileSpawnColumn = -1;
    tilemap->executeTileSpawnRow    = -1;

    tilemap->animationFrame = 0;
    tilemap->animationTimer = 23;

    pl_loadTiles(tilemap);
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

            if (tile < PL_TILEGRASS)
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
                    drawWsgSimple(&tilemap->tiles[tile - 32], x * PL_TILESIZE - tilemap->mapOffsetX,
                                  y * PL_TILESIZE - tilemap->mapOffsetY);
                }
                else
                {
                    drawWsgTile(&tilemap->tiles[tile - 32], x * PL_TILESIZE - tilemap->mapOffsetX,
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
        free(tilemap->map);
    }

    size_t sz;
    uint8_t* buf = spiffsReadFile(name, &sz, false);

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

    free(buf);

    return true;
}

bool pl_loadTiles(plTilemap_t* tilemap)
{
    // tiles 0-31 are invisible tiles;
    // remember to subtract 32 from tile index before drawing tile
    loadWsg("tile032.wsg", &tilemap->tiles[0], false);
    loadWsg("tile033.wsg", &tilemap->tiles[1], false);
    loadWsg("tile034.wsg", &tilemap->tiles[2], false);
    loadWsg("tile035.wsg", &tilemap->tiles[3], false);
    loadWsg("tile036.wsg", &tilemap->tiles[4], false);
    loadWsg("tile037.wsg", &tilemap->tiles[5], false);
    loadWsg("tile038.wsg", &tilemap->tiles[6], false);
    loadWsg("tile039.wsg", &tilemap->tiles[7], false);
    loadWsg("tile040.wsg", &tilemap->tiles[8], false);
    loadWsg("tile041.wsg", &tilemap->tiles[9], false);

    tilemap->tiles[10] = tilemap->tiles[0];
    tilemap->tiles[11] = tilemap->tiles[0];
    tilemap->tiles[12] = tilemap->tiles[0];
    tilemap->tiles[13] = tilemap->tiles[0];
    tilemap->tiles[14] = tilemap->tiles[0];
    tilemap->tiles[15] = tilemap->tiles[0];
    tilemap->tiles[16] = tilemap->tiles[0];
    tilemap->tiles[17] = tilemap->tiles[0];
    tilemap->tiles[18] = tilemap->tiles[0];
    tilemap->tiles[19] = tilemap->tiles[0];
    tilemap->tiles[20] = tilemap->tiles[0];
    tilemap->tiles[21] = tilemap->tiles[0];
    tilemap->tiles[22] = tilemap->tiles[0];
    tilemap->tiles[23] = tilemap->tiles[0];
    tilemap->tiles[24] = tilemap->tiles[0];
    tilemap->tiles[25] = tilemap->tiles[0];
    tilemap->tiles[26] = tilemap->tiles[0];

    loadWsg("tile059.wsg", &tilemap->tiles[27], false);
    loadWsg("tile060.wsg", &tilemap->tiles[28], false);
    loadWsg("tile061.wsg", &tilemap->tiles[29], false);
    loadWsg("tile062.wsg", &tilemap->tiles[30], false);
    loadWsg("tile063.wsg", &tilemap->tiles[31], false);
    loadWsg("tile064.wsg", &tilemap->tiles[32], false);
    loadWsg("tile065.wsg", &tilemap->tiles[33], false);
    loadWsg("tile066.wsg", &tilemap->tiles[34], false);
    loadWsg("tile067.wsg", &tilemap->tiles[35], false);
    loadWsg("tile068.wsg", &tilemap->tiles[36], false);
    loadWsg("tile069.wsg", &tilemap->tiles[37], false);
    loadWsg("tile070.wsg", &tilemap->tiles[38], false);

    tilemap->tiles[39] = tilemap->tiles[0];
    tilemap->tiles[40] = tilemap->tiles[0];
    tilemap->tiles[41] = tilemap->tiles[0];
    tilemap->tiles[42] = tilemap->tiles[0];
    tilemap->tiles[43] = tilemap->tiles[0];
    tilemap->tiles[44] = tilemap->tiles[0];
    tilemap->tiles[45] = tilemap->tiles[0];
    tilemap->tiles[46] = tilemap->tiles[0];
    tilemap->tiles[47] = tilemap->tiles[0];

    loadWsg("tile080.wsg", &tilemap->tiles[48], false);
    loadWsg("tile081.wsg", &tilemap->tiles[49], false);
    loadWsg("tile082.wsg", &tilemap->tiles[50], false);
    loadWsg("tile083.wsg", &tilemap->tiles[51], false);
    loadWsg("tile084.wsg", &tilemap->tiles[52], false);
    loadWsg("tile085.wsg", &tilemap->tiles[53], false);
    loadWsg("tile086.wsg", &tilemap->tiles[54], false);
    loadWsg("tile087.wsg", &tilemap->tiles[55], false);
    loadWsg("tile088.wsg", &tilemap->tiles[56], false);
    loadWsg("tile089.wsg", &tilemap->tiles[57], false);
    loadWsg("tile090.wsg", &tilemap->tiles[58], false);
    loadWsg("tile091.wsg", &tilemap->tiles[59], false);
    loadWsg("tile092.wsg", &tilemap->tiles[60], false);
    loadWsg("tile093.wsg", &tilemap->tiles[61], false);
    loadWsg("tile094.wsg", &tilemap->tiles[62], false);
    loadWsg("tile095.wsg", &tilemap->tiles[63], false);
    loadWsg("tile096.wsg", &tilemap->tiles[64], false);
    loadWsg("tile097.wsg", &tilemap->tiles[65], false);
    loadWsg("tile098.wsg", &tilemap->tiles[66], false);
    loadWsg("tile099.wsg", &tilemap->tiles[67], false);
    loadWsg("tile100.wsg", &tilemap->tiles[68], false);
    loadWsg("tile101.wsg", &tilemap->tiles[69], false);
    loadWsg("tile102.wsg", &tilemap->tiles[70], false);
    loadWsg("tile103.wsg", &tilemap->tiles[71], false);

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
        case PL_TILEEMPTY ... PL_TILEUNUSED_29:
            return false;
            break;
        case PL_TILEINVISIBLE_BLOCK ... PL_TILEMETAL_PIPE_V:
            return true;
            break;
        case PL_TILEBOUNCE_BLOCK:
            return false;
            break;
        case PL_TILEDIRT_PATH ... PL_TILECONTAINER_3:
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
        case PL_TILEBOUNCE_BLOCK:
        case PL_TILEGIRDER:
        case PL_TILECONTAINER_1 ... PL_TILECONTAINER_3:
        case PL_TILECOIN_1 ... PL_TILECOIN_3:
        case PL_TILELADDER:
        case PL_TILEBG_GOAL_ZONE ... PL_TILEBG_CLOUD_D:
            return true;
        case PL_TILEBG_CLOUD:
            return false;
        case PL_TILEBG_TALL_GRASS ... PL_TILEBG_MOUNTAIN_R:
            return true;
        case PL_TILEBG_MOUNTAIN ... PL_TILEBG_METAL:
            return false;
        case PL_TILEBG_CHAINS:
            return true;
        case PL_TILEBG_WALL:
            return false;
        default:
            return false;
    }
}

void pl_freeTilemap(plTilemap_t* tilemap)
{
    free(tilemap->map);
    for (uint8_t i = 0; i < PL_TILESET_SIZE; i++)
    {
        switch (i)
        {
            // Skip all placeholder tiles, since they reuse other tiles
            //(see pl_loadTiles)
            case 10 ... 26:
            case 39 ... 47:
            {
                break;
            }
            default:
            {
                freeWsg(&tilemap->tiles[i]);
                break;
            }
        }
    }
}