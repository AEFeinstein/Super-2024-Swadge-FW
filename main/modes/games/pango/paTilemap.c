//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include <esp_log.h>
#include <string.h>
#include <esp_heap_caps.h>

#include "fs_wsg.h"
#include "paTilemap.h"
#include "paLeveldef.h"
#include "esp_random.h"

#include "cnfs.h"

//==============================================================================
// Function Prototypes
//==============================================================================

// bool isInteractive(uint8_t tileId);

//==============================================================================
// Functions
//==============================================================================

void pa_initializeTileMap(paTilemap_t* tilemap)
{
    tilemap->mapOffsetX = 0;
    tilemap->mapOffsetY = 0;

    tilemap->tileSpawnEnabled       = false;
    tilemap->executeTileSpawnColumn = -1;
    tilemap->executeTileSpawnRow    = -1;

    tilemap->animationFrame = 0;
    tilemap->animationTimer = 23;

    pa_loadTiles(tilemap);
}

void pa_drawTileMap(paTilemap_t* tilemap)
{
    tilemap->animationTimer--;
    if (tilemap->animationTimer < 0)
    {
        tilemap->animationFrame = ((tilemap->animationFrame + 1) % 3);
        tilemap->animationTimer = 23;
    }

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

            // Test animated tiles
            if (tile == PA_TILE_SPAWN_BLOCK_0 || tile == PA_TILE_BONUS_BLOCK_0)
            {
                tile += tilemap->animationFrame;
            }

            // Draw only non-garbage tiles
            if (tile > 0 && tile < 13)
            {
                if (pa_needsTransparency(tile))
                {
                    // drawWsgSimpleFast(&tilemap->tiles[tile - 32], x * PA_TILE_SIZE - tilemap->mapOffsetX, y *
                    // PA_TILE_SIZE - tilemap->mapOffsetY);
                    drawWsgSimple(&tilemap->tiles[tile - 1], x * PA_TILE_SIZE - tilemap->mapOffsetX,
                                  y * PA_TILE_SIZE - tilemap->mapOffsetY);
                }
                else
                {
                    drawWsgTile(&tilemap->tiles[tile - 1], x * PA_TILE_SIZE - tilemap->mapOffsetX,
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

    /*for (uint16_t i = 0; i < 16; i++)
    {
        tilemap->warps[i].x = buf[2 + width * height + i * 2];
        tilemap->warps[i].y = buf[2 + width * height + i * 2 + 1];
    }*/

    free(buf);

    return true;
}

bool pa_loadTiles(paTilemap_t* tilemap)
{
    // tiles 0 is invisible
    // remember to subtract 1 from tile index before drawing tile
    loadWsg("pa-tile-001.wsg", &tilemap->tiles[0], false);
    loadWsg("pa-tile-002.wsg", &tilemap->tiles[1], false);
    loadWsg("pa-tile-003.wsg", &tilemap->tiles[2], false);
    loadWsg("pa-tile-004.wsg", &tilemap->tiles[3], false);
    loadWsg("pa-tile-005.wsg", &tilemap->tiles[4], false);
    loadWsg("pa-tile-006.wsg", &tilemap->tiles[5], false);
    loadWsg("pa-tile-007.wsg", &tilemap->tiles[6], false);
    loadWsg("pa-tile-008.wsg", &tilemap->tiles[7], false);
    loadWsg("pa-tile-009.wsg", &tilemap->tiles[8], false);
    loadWsg("pa-tile-010.wsg", &tilemap->tiles[9], false);
    loadWsg("pa-tile-011.wsg", &tilemap->tiles[10], false);
    loadWsg("pa-tile-012.wsg", &tilemap->tiles[11], false);
    loadWsg("pa-tile-013.wsg", &tilemap->tiles[12], false);
    loadWsg("pa-tile-014.wsg", &tilemap->tiles[13], false);
    loadWsg("pa-tile-015.wsg", &tilemap->tiles[14], false);

    /*
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
*/
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
        //ty = 0;
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

// bool isInteractive(uint8_t tileId)
// {
//     return tileId > PA_TILE_INVISIBLE_BLOCK && tileId < PA_TILE_BG_GOAL_ZONE;
// }

void pa_unlockScrolling(paTilemap_t* tilemap)
{
    tilemap->minMapOffsetX = 0;
    tilemap->maxMapOffsetX = tilemap->mapWidth * PA_TILE_SIZE - PA_TILE_MAP_DISPLAY_WIDTH_PIXELS;

    tilemap->minMapOffsetY = 0;
    tilemap->maxMapOffsetY = tilemap->mapHeight * PA_TILE_SIZE - PA_TILE_MAP_DISPLAY_HEIGHT_PIXELS;
}

bool pa_needsTransparency(uint8_t tileId)
{
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
            return true;
    }
}

void pa_freeTilemap(paTilemap_t* tilemap)
{
    free(tilemap->map);
    for (uint8_t i = 0; i < PA_TILE_SET_SIZE; i++)
    {
        switch (i)
        {
            // Skip all placeholder tiles, since they reuse other tiles
            //(see pa_loadTiles)
            /*case 10 ... 26:
            case 39 ... 47:
            {
                break;
            }*/
            default:
            {
                freeWsg(&tilemap->tiles[i]);
                break;
            }
        }
    }
}

void pa_generateMaze(paTilemap_t* tilemap)
{
    int32_t tx = 1;
    int32_t ty = 13;
    pa_setTile(tilemap, tx, ty, PA_TILE_EMPTY);

    while(ty > 1){
        tx = 1;
        while(tx < 15){
            if(!pa_getTile(tilemap, tx, ty) && !pa_genPathContinue(tilemap, tx, ty)){
                pa_genMakePath(tilemap, tx, ty);
            }
            tx += 2;
        }
        ty -= 2;
    }
}

bool pa_genPathContinue(paTilemap_t* tilemap, uint32_t x, uint32_t y){
    if(pa_getTile(tilemap, x, y-2)) {
        return false;
    }
    if(pa_getTile(tilemap, x, y+2)) {
        return false;
    }
    if(pa_getTile(tilemap, x+2, y)) {
        return false;
    }
    if(pa_getTile(tilemap, x-2, y)) {
        return false;
    }

    return true;
}

void pa_genMakePath(paTilemap_t* tilemap, uint32_t x, uint32_t y){
    bool done = 0;
    uint32_t nx = x;
    uint32_t ny = y;

    while(!done){
        uint32_t r = esp_random() % 4;
        
        switch(r){
            case 0:
                if(pa_getTile(tilemap, nx, ny-2)) {
                    pa_setTile(tilemap, nx, ny-1, 0);
                    pa_setTile(tilemap, nx, ny-2, 0);
                    ny-=2;
                }
                break;
            case 1:
                if(pa_getTile(tilemap, nx, ny+2)) {
                    pa_setTile(tilemap, nx, ny+1, 0);
                    pa_setTile(tilemap, nx, ny+2, 0);
                    ny+=2;
                }
                break;
            case 2:
                if(pa_getTile(tilemap, nx-2, ny)) {
                    pa_setTile(tilemap, nx-1, ny, 0);
                    pa_setTile(tilemap, nx-2, ny, 0);
                    nx-=2;
                }
                break;
            case 3:
                if(pa_getTile(tilemap, nx+2, ny)) {
                    pa_setTile(tilemap, nx+1, ny, 0);
                    pa_setTile(tilemap, nx+2, ny, 0);
                    nx+=2;
                }
                break;

            
        }

        done = pa_genPathContinue(tilemap, nx, ny);
    }

}

void pa_placeEnemySpawns(paTilemap_t* tilemap){
    int16_t enemySpawnsToPlace = 8;
    int16_t enemiesPlaced = 0;
    bool previouslyPlaced = false;
    int16_t iterations = 0;

    //Place enemy spawn blocks
    while(enemySpawnsToPlace > 0 && iterations < 16){
        for(uint16_t ty = 1; ty < 13; ty++){
            for(uint16_t tx = 1; tx < 15; tx++){
                if(enemySpawnsToPlace <= 0) {
                    break;
                }

                uint8_t t = pa_getTile(tilemap, tx, ty);

                if(t == PA_TILE_BLOCK && !previouslyPlaced && !(esp_random() % 15) ){
                    pa_setTile(tilemap, tx, ty, PA_TILE_SPAWN_BLOCK_0);
                    enemySpawnsToPlace--;
                    enemiesPlaced++;
                    previouslyPlaced = true;
                } else {
                    previouslyPlaced = false;
                }
            }
        }
        iterations++;
    }

    tilemap->entityManager->remainingEnemies = enemiesPlaced;
}