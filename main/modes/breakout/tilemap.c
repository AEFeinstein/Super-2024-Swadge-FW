//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include <esp_log.h>
#include <string.h>
#include <esp_heap_caps.h>

#include "spiffs_wsg.h"
#include "tilemap.h"
#include "leveldef.h"
#include "esp_random.h"

#include "hdw-spiffs.h"

//==============================================================================
// Function Prototypes
//==============================================================================

// bool isInteractive(uint8_t tileId);

//==============================================================================
// Functions
//==============================================================================

void initializeTileMap(tilemap_t* tilemap)
{
    tilemap->mapOffsetX = 0;
    tilemap->mapOffsetY = 0;

    tilemap->tileSpawnEnabled       = true;
    tilemap->executeTileSpawnColumn = -1;
    tilemap->executeTileSpawnRow    = -1;

    tilemap->animationFrame = 0;
    tilemap->animationTimer = 23;

    loadTiles(tilemap);
}

void drawTileMap(tilemap_t* tilemap)
{
    tilemap->animationTimer--;
    if (tilemap->animationTimer < 0)
    {
        tilemap->animationFrame = ((tilemap->animationFrame + 1) % 3);
        tilemap->animationTimer = 23;
    }

    for (uint16_t y = (tilemap->mapOffsetY >> TILE_SIZE_IN_POWERS_OF_2);
         y < (tilemap->mapOffsetY >> TILE_SIZE_IN_POWERS_OF_2) + TILEMAP_DISPLAY_HEIGHT_TILES; y++)
    {
        if (y >= tilemap->mapHeight)
        {
            break;
        }

        for (int32_t x = (tilemap->mapOffsetX >> TILE_SIZE_IN_POWERS_OF_2);
             x < (tilemap->mapOffsetX >> TILE_SIZE_IN_POWERS_OF_2) + TILEMAP_DISPLAY_WIDTH_TILES; x++)
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

            if (tile < TILE_BOUNDARY_1 || tile == TILE_INVISIBLE_BLOCK)
            {
                continue;
            }

            // Test animated tiles
            /*if (tile == 64 || tile == 67)
            {
                tile += tilemap->animationFrame;
            }*/

            // Draw only non-garbage tiles
            if (tile > 0 && tile < 128)
            {
                if (needsTransparency(tile))
                {
                    // drawWsgSimpleFast(&tilemap->tiles[tile - 1], x * TILE_SIZE - tilemap->mapOffsetX, y * TILE_SIZE -
                    // tilemap->mapOffsetY);
                    drawWsgSimple(&tilemap->tiles[tile - 1], x * TILE_SIZE - tilemap->mapOffsetX,
                                  y * TILE_SIZE - tilemap->mapOffsetY);
                }
                else
                {
                    drawWsgTile(&tilemap->tiles[tile - 1], x * TILE_SIZE - tilemap->mapOffsetX,
                                y * TILE_SIZE - tilemap->mapOffsetY);
                }
            }
            else if (tile > 127 && tilemap->tileSpawnEnabled
                     && (tilemap->executeTileSpawnColumn == x || tilemap->executeTileSpawnRow == y
                         || tilemap->executeTileSpawnAll))
            {
                tileSpawnEntity(tilemap, tile - 128, x, y);
            }
        }
    }

    tilemap->executeTileSpawnAll = 0;
}

void scrollTileMap(tilemap_t* tilemap, int16_t x, int16_t y)
{
    if (x != 0)
    {
        uint8_t oldTx       = tilemap->mapOffsetX >> TILE_SIZE_IN_POWERS_OF_2;
        tilemap->mapOffsetX = CLAMP(tilemap->mapOffsetX + x, tilemap->minMapOffsetX, tilemap->maxMapOffsetX);
        uint8_t newTx       = tilemap->mapOffsetX >> TILE_SIZE_IN_POWERS_OF_2;

        if (newTx > oldTx)
        {
            tilemap->executeTileSpawnColumn = oldTx + TILEMAP_DISPLAY_WIDTH_TILES;
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
        uint8_t oldTy       = tilemap->mapOffsetY >> TILE_SIZE_IN_POWERS_OF_2;
        tilemap->mapOffsetY = CLAMP(tilemap->mapOffsetY + y, tilemap->minMapOffsetY, tilemap->maxMapOffsetY);
        uint8_t newTy       = tilemap->mapOffsetY >> TILE_SIZE_IN_POWERS_OF_2;

        if (newTy > oldTy)
        {
            tilemap->executeTileSpawnRow = oldTy + TILEMAP_DISPLAY_HEIGHT_TILES;
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

bool loadMapFromFile(tilemap_t* tilemap, const char* name)
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
    tilemap->maxMapOffsetX = width * TILE_SIZE - TILEMAP_DISPLAY_WIDTH_PIXELS;

    tilemap->minMapOffsetY = 0;
    tilemap->maxMapOffsetY = height * TILE_SIZE - TILEMAP_DISPLAY_HEIGHT_PIXELS;

    memcpy(&(tilemap->totalTargetBlocks), &buf[2 + width * height], 2);

    free(buf);

    return true;
}

bool loadTiles(tilemap_t* tilemap)
{
    // tiles 0 is invisible
    // remember to subtract 1 from tile index before drawing tile
    loadWsg("brkTile001.wsg", &tilemap->tiles[0], false);
    loadWsg("brkTile002.wsg", &tilemap->tiles[1], false);
    loadWsg("brkTile003.wsg", &tilemap->tiles[2], false);
    loadWsg("brkTile004.wsg", &tilemap->tiles[3], false);
    loadWsg("brkTile005.wsg", &tilemap->tiles[4], false);
    loadWsg("brkTile006.wsg", &tilemap->tiles[5], false);
    loadWsg("brkTile007.wsg", &tilemap->tiles[6], false);
    tilemap->tiles[7]  = tilemap->tiles[0];
    tilemap->tiles[8]  = tilemap->tiles[0];
    tilemap->tiles[9]  = tilemap->tiles[0];
    tilemap->tiles[10] = tilemap->tiles[0];
    tilemap->tiles[11] = tilemap->tiles[0];
    tilemap->tiles[12] = tilemap->tiles[0];
    tilemap->tiles[13] = tilemap->tiles[0];
    tilemap->tiles[14] = tilemap->tiles[0];
    loadWsg("brkTile016.wsg", &tilemap->tiles[15], false);
    loadWsg("brkTile017.wsg", &tilemap->tiles[16], false);
    loadWsg("brkTile018.wsg", &tilemap->tiles[17], false);
    loadWsg("brkTile019.wsg", &tilemap->tiles[18], false);
    loadWsg("brkTile020.wsg", &tilemap->tiles[19], false);
    loadWsg("brkTile021.wsg", &tilemap->tiles[20], false);
    loadWsg("brkTile022.wsg", &tilemap->tiles[21], false);
    loadWsg("brkTile023.wsg", &tilemap->tiles[22], false);
    loadWsg("brkTile024.wsg", &tilemap->tiles[23], false);
    loadWsg("brkTile025.wsg", &tilemap->tiles[24], false);
    loadWsg("brkTile026.wsg", &tilemap->tiles[25], false);
    loadWsg("brkTile027.wsg", &tilemap->tiles[26], false);
    loadWsg("brkTile028.wsg", &tilemap->tiles[27], false);

    tilemap->tiles[28] = tilemap->tiles[0];
    tilemap->tiles[29] = tilemap->tiles[0];
    tilemap->tiles[30] = tilemap->tiles[0];

    loadWsg("brkTile032.wsg", &tilemap->tiles[31], false);
    loadWsg("brkTile033.wsg", &tilemap->tiles[32], false);
    loadWsg("brkTile034.wsg", &tilemap->tiles[33], false);
    loadWsg("brkTile035.wsg", &tilemap->tiles[34], false);
    loadWsg("brkTile036.wsg", &tilemap->tiles[35], false);
    loadWsg("brkTile037.wsg", &tilemap->tiles[36], false);
    loadWsg("brkTile038.wsg", &tilemap->tiles[37], false);
    loadWsg("brkTile039.wsg", &tilemap->tiles[38], false);
    loadWsg("brkTile040.wsg", &tilemap->tiles[39], false);
    loadWsg("brkTile041.wsg", &tilemap->tiles[40], false);
    loadWsg("brkTile042.wsg", &tilemap->tiles[41], false);
    loadWsg("brkTile043.wsg", &tilemap->tiles[42], false);
    loadWsg("brkTile044.wsg", &tilemap->tiles[43], false);
    loadWsg("brkTile045.wsg", &tilemap->tiles[44], false);
    loadWsg("brkTile046.wsg", &tilemap->tiles[45], false);
    loadWsg("brkTile047.wsg", &tilemap->tiles[46], false);
    loadWsg("brkTile048.wsg", &tilemap->tiles[47], false);
    loadWsg("brkTile049.wsg", &tilemap->tiles[48], false);
    loadWsg("brkTile050.wsg", &tilemap->tiles[49], false);
    loadWsg("brkTile051.wsg", &tilemap->tiles[50], false);
    loadWsg("brkTile052.wsg", &tilemap->tiles[51], false);
    loadWsg("brkTile053.wsg", &tilemap->tiles[52], false);
    loadWsg("brkTile054.wsg", &tilemap->tiles[53], false);
    loadWsg("brkTile055.wsg", &tilemap->tiles[54], false);
    loadWsg("brkTile056.wsg", &tilemap->tiles[55], false);
    loadWsg("brkTile057.wsg", &tilemap->tiles[56], false);

    tilemap->tiles[57] = tilemap->tiles[0];
    tilemap->tiles[58] = tilemap->tiles[0];
    tilemap->tiles[59] = tilemap->tiles[0];
    tilemap->tiles[60] = tilemap->tiles[0];
    tilemap->tiles[61] = tilemap->tiles[0];
    tilemap->tiles[62] = tilemap->tiles[0];

    loadWsg("brkTile064.wsg", &tilemap->tiles[63], false);
    loadWsg("brkTile065.wsg", &tilemap->tiles[64], false);
    loadWsg("brkTile066.wsg", &tilemap->tiles[65], false);
    loadWsg("brkTile067.wsg", &tilemap->tiles[66], false);
    loadWsg("brkTile068.wsg", &tilemap->tiles[67], false);
    loadWsg("brkTile069.wsg", &tilemap->tiles[68], false);
    loadWsg("brkTile070.wsg", &tilemap->tiles[69], false);
    loadWsg("brkTile071.wsg", &tilemap->tiles[70], false);
    loadWsg("brkTile072.wsg", &tilemap->tiles[71], false);
    loadWsg("brkTile073.wsg", &tilemap->tiles[72], false);
    loadWsg("brkTile074.wsg", &tilemap->tiles[73], false);
    loadWsg("brkTile075.wsg", &tilemap->tiles[74], false);
    loadWsg("brkTile076.wsg", &tilemap->tiles[75], false);
    loadWsg("brkTile077.wsg", &tilemap->tiles[76], false);
    loadWsg("brkTile078.wsg", &tilemap->tiles[77], false);
    loadWsg("brkTile079.wsg", &tilemap->tiles[78], false);
    loadWsg("brkTile080.wsg", &tilemap->tiles[79], false);
    loadWsg("brkTile081.wsg", &tilemap->tiles[80], false);
    loadWsg("brkTile082.wsg", &tilemap->tiles[81], false);
    loadWsg("brkTile083.wsg", &tilemap->tiles[82], false);
    loadWsg("brkTile084.wsg", &tilemap->tiles[83], false);
    loadWsg("brkTile085.wsg", &tilemap->tiles[84], false);
    loadWsg("brkTile086.wsg", &tilemap->tiles[85], false);
    loadWsg("brkTile087.wsg", &tilemap->tiles[86], false);
    loadWsg("brkTile088.wsg", &tilemap->tiles[87], false);
    loadWsg("brkTile089.wsg", &tilemap->tiles[88], false);
    loadWsg("brkTile090.wsg", &tilemap->tiles[89], false);
    loadWsg("brkTile091.wsg", &tilemap->tiles[90], false);
    loadWsg("brkTile092.wsg", &tilemap->tiles[91], false);
    loadWsg("brkTile093.wsg", &tilemap->tiles[92], false);
    loadWsg("brkTile094.wsg", &tilemap->tiles[93], false);
    loadWsg("brkTile095.wsg", &tilemap->tiles[94], false);
    loadWsg("brkTile096.wsg", &tilemap->tiles[95], false);
    loadWsg("brkTile097.wsg", &tilemap->tiles[96], false);
    loadWsg("brkTile098.wsg", &tilemap->tiles[97], false);
    loadWsg("brkTile099.wsg", &tilemap->tiles[98], false);
    loadWsg("brkTile100.wsg", &tilemap->tiles[99], false);
    loadWsg("brkTile101.wsg", &tilemap->tiles[100], false);
    loadWsg("brkTile102.wsg", &tilemap->tiles[101], false);
    loadWsg("brkTile103.wsg", &tilemap->tiles[102], false);
    loadWsg("brkTile104.wsg", &tilemap->tiles[103], false);
    loadWsg("brkTile105.wsg", &tilemap->tiles[104], false);
    loadWsg("brkTile106.wsg", &tilemap->tiles[105], false);
    loadWsg("brkTile107.wsg", &tilemap->tiles[106], false);
    loadWsg("brkTile108.wsg", &tilemap->tiles[107], false);
    loadWsg("brkTile109.wsg", &tilemap->tiles[108], false);
    loadWsg("brkTile110.wsg", &tilemap->tiles[109], false);
    loadWsg("brkTile111.wsg", &tilemap->tiles[110], false);
    loadWsg("brkTile112.wsg", &tilemap->tiles[111], false);
    loadWsg("brkTile113.wsg", &tilemap->tiles[112], false);
    loadWsg("brkTile114.wsg", &tilemap->tiles[113], false);
    loadWsg("brkTile115.wsg", &tilemap->tiles[114], false);
    loadWsg("brkTile116.wsg", &tilemap->tiles[115], false);
    loadWsg("brkTile117.wsg", &tilemap->tiles[116], false);
    loadWsg("brkTile118.wsg", &tilemap->tiles[117], false);
    loadWsg("brkTile119.wsg", &tilemap->tiles[118], false);
    loadWsg("brkTile120.wsg", &tilemap->tiles[119], false);
    loadWsg("brkTile121.wsg", &tilemap->tiles[120], false);
    loadWsg("brkTile122.wsg", &tilemap->tiles[121], false);
    loadWsg("brkTile123.wsg", &tilemap->tiles[122], false);
    loadWsg("brkTile124.wsg", &tilemap->tiles[123], false);
    loadWsg("brkTile125.wsg", &tilemap->tiles[124], false);
    loadWsg("brkTile126.wsg", &tilemap->tiles[125], false);
    loadWsg("brkTile127.wsg", &tilemap->tiles[126], false);

    return true;
}

void tileSpawnEntity(tilemap_t* tilemap, uint8_t objectIndex, uint8_t tx, uint8_t ty)
{
    entity_t* entityCreated = createEntity(tilemap->entityManager, objectIndex, (tx << TILE_SIZE_IN_POWERS_OF_2) + 4,
                                           (ty << TILE_SIZE_IN_POWERS_OF_2) + 4);

    if (entityCreated != NULL)
    {
        entityCreated->homeTileX                  = tx;
        entityCreated->homeTileY                  = ty;
        tilemap->map[ty * tilemap->mapWidth + tx] = 0;
    }
}

uint8_t getTile(tilemap_t* tilemap, uint8_t tx, uint8_t ty)
{
    // ty = CLAMP(ty, 0, tilemap->mapHeight - 1);

    if (/*ty < 0 ||*/ ty >= tilemap->mapHeight)
    {
        ty = 0;
        // return 0;
    }

    if (/*tx < 0 ||*/ tx >= tilemap->mapWidth)
    {
        return 0;
    }

    return tilemap->map[ty * tilemap->mapWidth + tx];
}

void setTile(tilemap_t* tilemap, uint8_t tx, uint8_t ty, uint8_t newTileId)
{
    // ty = CLAMP(ty, 0, tilemap->mapHeight - 1);

    if (ty >= tilemap->mapHeight || tx >= tilemap->mapWidth)
    {
        return;
    }

    tilemap->map[ty * tilemap->mapWidth + tx] = newTileId;
}

bool isSolid(uint8_t tileId)
{
    switch (tileId)
    {
        case TILE_EMPTY:
            return false;
            break;
        case TILE_BOUNDARY_1 ... TILE_UNUSED_127:
            return true;
            break;
        default:
            return false;
    }
}

bool isBlock(uint8_t tileId)
{
    switch (tileId)
    {
        case TILE_BLOCK_1x1_RED ... TILE_BLOCK_2x2_BLACK_DR:
            return true;
        default:
            return false;
    }

    return false;
}

// bool isInteractive(uint8_t tileId)
// {
//     return tileId > TILE_INVISIBLE_BLOCK && tileId < TILE_BG_GOAL_ZONE;
// }

void unlockScrolling(tilemap_t* tilemap)
{
    tilemap->minMapOffsetX = 0;
    tilemap->maxMapOffsetX = tilemap->mapWidth * TILE_SIZE - TILEMAP_DISPLAY_WIDTH_PIXELS;

    tilemap->minMapOffsetY = 0;
    tilemap->maxMapOffsetY = tilemap->mapHeight * TILE_SIZE - TILEMAP_DISPLAY_HEIGHT_PIXELS;
}

bool needsTransparency(uint8_t tileId)
{
    switch (tileId)
    {
        case TILE_BOUNDARY_1 ... TILE_UNUSED_31:
            return false;
        case TILE_BLOCK_2x1_RED_L ... TILE_UNUSED_127:
            return true;
        default:
            return false;
    }
}

void freeTilemap(tilemap_t* tilemap)
{
    free(tilemap->map);
    for (uint8_t i = 0; i < 127; i++)
    {
        switch (i)
        {
            // Skip all placeholder tiles, since they reuse other tiles
            //(see loadTiles)
            case 3 ... 14:
            case 27 ... 30:
            case 55 ... 62:
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

void forceTileSpawnEntitiesWithinView(tilemap_t* tilemap)
{
    for (uint16_t y = (tilemap->mapOffsetY >> TILE_SIZE_IN_POWERS_OF_2);
         y < (tilemap->mapOffsetY >> TILE_SIZE_IN_POWERS_OF_2) + TILEMAP_DISPLAY_HEIGHT_TILES; y++)
    {
        if (y >= tilemap->mapHeight)
        {
            break;
        }

        for (int32_t x = (tilemap->mapOffsetX >> TILE_SIZE_IN_POWERS_OF_2);
             x < (tilemap->mapOffsetX >> TILE_SIZE_IN_POWERS_OF_2) + TILEMAP_DISPLAY_WIDTH_TILES; x++)
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

            if (tile < 128)
            {
                continue;
            }
            else if (tilemap->tileSpawnEnabled)
            {
                tileSpawnEntity(tilemap, tile - 128, x, y);
            }
        }
    }
}