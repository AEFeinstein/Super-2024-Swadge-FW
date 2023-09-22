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

void initializeTileMap(tilemap_t *tilemap)
{
    tilemap->mapOffsetX = 0;
    tilemap->mapOffsetY = 0;

    tilemap->tileSpawnEnabled = false;
    tilemap->executeTileSpawnColumn = -1;
    tilemap->executeTileSpawnRow = -1;

    tilemap->animationFrame = 0;
    tilemap->animationTimer = 23;

    loadTiles(tilemap);
}

void drawTileMap(tilemap_t *tilemap)
{
    tilemap->animationTimer--;
    if (tilemap->animationTimer < 0)
    {
        tilemap->animationFrame = ((tilemap->animationFrame + 1) % 3);
        tilemap->animationTimer = 23;
    }

    for (uint16_t y = (tilemap->mapOffsetY >> TILE_SIZE_IN_POWERS_OF_2); y < (tilemap->mapOffsetY >> TILE_SIZE_IN_POWERS_OF_2) + TILEMAP_DISPLAY_HEIGHT_TILES; y++)
    {
        if (y >= tilemap->mapHeight)
        {
            break;
        }

        for (uint16_t x = (tilemap->mapOffsetX >> TILE_SIZE_IN_POWERS_OF_2); x < (tilemap->mapOffsetX >> TILE_SIZE_IN_POWERS_OF_2) + TILEMAP_DISPLAY_WIDTH_TILES; x++)
        {
            if (x >= tilemap->mapWidth)
            {
                break;
            }

            uint8_t tile = tilemap->map[(y * tilemap->mapWidth) + x];
            
            if(tile < TILE_GRASS){
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
                if(needsTransparency(tile)){
                    drawWsgSimpleFast(&tilemap->tiles[tile - 32], x * TILE_SIZE - tilemap->mapOffsetX, y * TILE_SIZE - tilemap->mapOffsetY);
                }
                else {
                    drawWsgTile(&tilemap->tiles[tile - 32], x * TILE_SIZE - tilemap->mapOffsetX, y * TILE_SIZE - tilemap->mapOffsetY);
                }
            }
            else if (tile > 127 && tilemap->tileSpawnEnabled && (tilemap->executeTileSpawnColumn == x || tilemap->executeTileSpawnRow == y || tilemap->executeTileSpawnAll))
            {
                tileSpawnEntity(tilemap, tile - 128, x, y);
            }
        }
    }

    tilemap->executeTileSpawnAll = 0;
}

void scrollTileMap(tilemap_t *tilemap, int16_t x, int16_t y)
{
    if (x != 0)
    {
        uint8_t oldTx = tilemap->mapOffsetX >> TILE_SIZE_IN_POWERS_OF_2;
        tilemap->mapOffsetX = CLAMP(tilemap->mapOffsetX + x, tilemap->minMapOffsetX, tilemap->maxMapOffsetX);
        uint8_t newTx = tilemap->mapOffsetX >> TILE_SIZE_IN_POWERS_OF_2;

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
        uint8_t oldTy = tilemap->mapOffsetY >> TILE_SIZE_IN_POWERS_OF_2;
        tilemap->mapOffsetY = CLAMP(tilemap->mapOffsetY + y, tilemap->minMapOffsetY, tilemap->maxMapOffsetY);
        uint8_t newTy = tilemap->mapOffsetY >> TILE_SIZE_IN_POWERS_OF_2;

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

bool loadMapFromFile(tilemap_t *tilemap, const char *name)
{
    if (tilemap->map != NULL)
    {
        free(tilemap->map);
    }

    size_t sz;
    uint8_t *buf = spiffsReadFile(name, &sz, false);
    
    if (NULL == buf)
    {
        ESP_LOGE("MAP", "Failed to read %s", name);
        return false;
    }

    uint8_t width = buf[0];
    uint8_t height = buf[1];

    tilemap->map = (uint8_t *)heap_caps_calloc(width * height, sizeof(uint8_t), MALLOC_CAP_SPIRAM);
    memcpy(tilemap->map, &buf[2], width * height);

    tilemap->mapWidth = width;
    tilemap->mapHeight = height;

    tilemap->minMapOffsetX = 0;
    tilemap->maxMapOffsetX = width * TILE_SIZE - TILEMAP_DISPLAY_WIDTH_PIXELS;

    tilemap->minMapOffsetY = 0;
    tilemap->maxMapOffsetY = height * TILE_SIZE - TILEMAP_DISPLAY_HEIGHT_PIXELS;

    for(uint16_t i=0; i<16; i++){
        tilemap->warps[i].x = buf[2 + width * height + i * 2];
        tilemap->warps[i].y = buf[2 + width * height + i * 2 + 1];
    }

    free(buf);

    return true;
}

bool loadTiles(tilemap_t *tilemap)
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

void tileSpawnEntity(tilemap_t *tilemap, uint8_t objectIndex, uint8_t tx, uint8_t ty)
{
    entity_t *entityCreated = createEntity(tilemap->entityManager, objectIndex, (tx << TILE_SIZE_IN_POWERS_OF_2) + 8, (ty << TILE_SIZE_IN_POWERS_OF_2) + 8);

    if (entityCreated != NULL)
    {
        entityCreated->homeTileX = tx;
        entityCreated->homeTileY = ty;
        tilemap->map[ty * tilemap->mapWidth + tx] = 0;
    }
}

uint8_t getTile(tilemap_t *tilemap, uint8_t tx, uint8_t ty)
{
    // ty = CLAMP(ty, 0, tilemap->mapHeight - 1);

    if (/*ty < 0 ||*/ ty >= tilemap->mapHeight)
    {
        ty = 0;
        //return 0;
    }

    if (/*tx < 0 ||*/ tx >= tilemap->mapWidth)
    {
        return 1;
    }

    return tilemap->map[ty * tilemap->mapWidth + tx];
}

void setTile(tilemap_t *tilemap, uint8_t tx, uint8_t ty, uint8_t newTileId)
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
    case TILE_EMPTY ... TILE_UNUSED_29:
        return false;
        break;
    case TILE_INVISIBLE_BLOCK ... TILE_METAL_PIPE_V:
        return true;
        break;
    case TILE_BOUNCE_BLOCK:
        return false;
        break;
    case TILE_DIRT_PATH ... TILE_CONTAINER_3:
        return true;
        break;
    default:
        return false;
    }
}

// bool isInteractive(uint8_t tileId)
// {
//     return tileId > TILE_INVISIBLE_BLOCK && tileId < TILE_BG_GOAL_ZONE;
// }

void unlockScrolling(tilemap_t *tilemap){
    tilemap->minMapOffsetX = 0;
    tilemap->maxMapOffsetX = tilemap->mapWidth * TILE_SIZE - TILEMAP_DISPLAY_WIDTH_PIXELS;

    tilemap->minMapOffsetY = 0;
    tilemap->maxMapOffsetY = tilemap->mapHeight * TILE_SIZE - TILEMAP_DISPLAY_HEIGHT_PIXELS;
}

bool needsTransparency(uint8_t tileId){
    switch(tileId) {
        case TILE_BOUNCE_BLOCK:
        case TILE_GIRDER:
        case TILE_CONTAINER_1 ... TILE_CONTAINER_3:
        case TILE_COIN_1 ... TILE_COIN_3:
        case TILE_LADDER:
        case TILE_BG_GOAL_ZONE ... TILE_BG_CLOUD_D:
            return true;
        case TILE_BG_CLOUD:
            return false;
        case TILE_BG_TALL_GRASS ... TILE_BG_MOUNTAIN_R:
            return true;
        case TILE_BG_MOUNTAIN ... TILE_BG_METAL:
            return false;
        case TILE_BG_CHAINS:
            return true;
        case TILE_BG_WALL:
            return false;
        default:
            return false;
    }
}

void freeTilemap(tilemap_t *tilemap){
    free(tilemap->map);
    for(uint8_t i=0; i<TILESET_SIZE; i++){
        switch(i){
            //Skip all placeholder tiles, since they reuse other tiles
            //(see loadTiles)
            case 10 ... 26:
            case 39 ... 47:
            {
                break;
            }
            default: {
                freeWsg(&tilemap->tiles[i]);
                break;
            }
        }
    }


}