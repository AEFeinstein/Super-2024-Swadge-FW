//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include <esp_log.h>
#include <string.h>
#include <esp_heap_caps.h>

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
            
            if(tile < TILE_BOUNDARY_1){
                continue;
            }

            // Test animated tiles
            /*if (tile == 64 || tile == 67)
            {
                tile += tilemap->animationFrame;
            }*/

            // Draw only non-garbage tiles
            if (tile > 0 /*&& tile < 104*/)
            {
                if(needsTransparency(tile)){
                    drawWsgSimpleFast(&tilemap->tiles[tile - 1], x * TILE_SIZE - tilemap->mapOffsetX, y * TILE_SIZE - tilemap->mapOffsetY);
                }
                else {
                    drawWsgTile(&tilemap->tiles[tile - 1], x * TILE_SIZE - tilemap->mapOffsetX, y * TILE_SIZE - tilemap->mapOffsetY);
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
    uint8_t *buf = spiffsReadFile(name, &buf, &sz);
    
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
    // tiles 0 is invisible
    // remember to subtract 1 from tile index before drawing tile
    loadWsg("brkTile001.wsg", &tilemap->tiles[0]);
    loadWsg("brkTile002.wsg", &tilemap->tiles[1]);
    loadWsg("brkTile003.wsg", &tilemap->tiles[2]);
    tilemap->tiles[4] = tilemap->tiles[0];
    tilemap->tiles[5] = tilemap->tiles[0];
    tilemap->tiles[6] = tilemap->tiles[0];
    tilemap->tiles[7] = tilemap->tiles[0];
    tilemap->tiles[8] = tilemap->tiles[0];
    tilemap->tiles[9] = tilemap->tiles[0];
    tilemap->tiles[10] = tilemap->tiles[0];
    tilemap->tiles[11] = tilemap->tiles[0];
    tilemap->tiles[12] = tilemap->tiles[0];
    tilemap->tiles[13] = tilemap->tiles[0];
    tilemap->tiles[14] = tilemap->tiles[0];
    tilemap->tiles[15] = tilemap->tiles[0];
    loadWsg("brkTile016.wsg", &tilemap->tiles[16]);
    loadWsg("brkTile017.wsg", &tilemap->tiles[17]);
    loadWsg("brkTile018.wsg", &tilemap->tiles[18]);
    loadWsg("brkTile019.wsg", &tilemap->tiles[19]);
    loadWsg("brkTile020.wsg", &tilemap->tiles[20]);
    loadWsg("brkTile021.wsg", &tilemap->tiles[21]);
    loadWsg("brkTile022.wsg", &tilemap->tiles[22]);
    loadWsg("brkTile023.wsg", &tilemap->tiles[23]);
    loadWsg("brkTile024.wsg", &tilemap->tiles[24]);
    loadWsg("brkTile025.wsg", &tilemap->tiles[25]);
    loadWsg("brkTile026.wsg", &tilemap->tiles[26]);
    loadWsg("brkTile027.wsg", &tilemap->tiles[27]);

    tilemap->tiles[28] = tilemap->tiles[0];
    tilemap->tiles[29] = tilemap->tiles[0];
    tilemap->tiles[30] = tilemap->tiles[0];
    tilemap->tiles[31] = tilemap->tiles[0];


    loadWsg("brkTile032.wsg", &tilemap->tiles[32]);
    loadWsg("brkTile033.wsg", &tilemap->tiles[33]);
    loadWsg("brkTile034.wsg", &tilemap->tiles[34]);
    loadWsg("brkTile035.wsg", &tilemap->tiles[35]);
    loadWsg("brkTile036.wsg", &tilemap->tiles[36]);
    loadWsg("brkTile037.wsg", &tilemap->tiles[37]);
    loadWsg("brkTile038.wsg", &tilemap->tiles[38]);
    loadWsg("brkTile039.wsg", &tilemap->tiles[39]);
    loadWsg("brkTile040.wsg", &tilemap->tiles[40]);
    loadWsg("brkTile041.wsg", &tilemap->tiles[41]);
    loadWsg("brkTile042.wsg", &tilemap->tiles[42]);
    loadWsg("brkTile043.wsg", &tilemap->tiles[43]);
    loadWsg("brkTile044.wsg", &tilemap->tiles[44]);
    loadWsg("brkTile045.wsg", &tilemap->tiles[45]);
    loadWsg("brkTile046.wsg", &tilemap->tiles[46]);
    loadWsg("brkTile047.wsg", &tilemap->tiles[47]);
    loadWsg("brkTile048.wsg", &tilemap->tiles[48]);
    loadWsg("brkTile049.wsg", &tilemap->tiles[49]);
    loadWsg("brkTile050.wsg", &tilemap->tiles[50]);
    loadWsg("brkTile051.wsg", &tilemap->tiles[51]);
    loadWsg("brkTile052.wsg", &tilemap->tiles[52]);
    loadWsg("brkTile053.wsg", &tilemap->tiles[53]);
    loadWsg("brkTile054.wsg", &tilemap->tiles[54]);
    loadWsg("brkTile055.wsg", &tilemap->tiles[55]);
    loadWsg("brkTile056.wsg", &tilemap->tiles[56]);
    loadWsg("brkTile057.wsg", &tilemap->tiles[57]);
    loadWsg("brkTile058.wsg", &tilemap->tiles[58]);
    loadWsg("brkTile059.wsg", &tilemap->tiles[59]);
    loadWsg("brkTile060.wsg", &tilemap->tiles[60]);
    loadWsg("brkTile061.wsg", &tilemap->tiles[61]);
    loadWsg("brkTile062.wsg", &tilemap->tiles[62]);
    loadWsg("brkTile063.wsg", &tilemap->tiles[63]);
    loadWsg("brkTile064.wsg", &tilemap->tiles[64]);
    loadWsg("brkTile065.wsg", &tilemap->tiles[65]);
    loadWsg("brkTile066.wsg", &tilemap->tiles[66]);
    loadWsg("brkTile067.wsg", &tilemap->tiles[67]);
    loadWsg("brkTile068.wsg", &tilemap->tiles[68]);
    loadWsg("brkTile069.wsg", &tilemap->tiles[69]);
    loadWsg("brkTile070.wsg", &tilemap->tiles[70]);
    loadWsg("brkTile071.wsg", &tilemap->tiles[71]);
    loadWsg("brkTile072.wsg", &tilemap->tiles[72]);
    loadWsg("brkTile073.wsg", &tilemap->tiles[73]);
    loadWsg("brkTile074.wsg", &tilemap->tiles[74]);
    loadWsg("brkTile075.wsg", &tilemap->tiles[75]);
    loadWsg("brkTile076.wsg", &tilemap->tiles[76]);
    loadWsg("brkTile077.wsg", &tilemap->tiles[77]);
    loadWsg("brkTile078.wsg", &tilemap->tiles[78]);
    loadWsg("brkTile079.wsg", &tilemap->tiles[79]);
    loadWsg("brkTile080.wsg", &tilemap->tiles[80]);
    loadWsg("brkTile081.wsg", &tilemap->tiles[81]);
    loadWsg("brkTile082.wsg", &tilemap->tiles[82]);
    loadWsg("brkTile083.wsg", &tilemap->tiles[83]);
    loadWsg("brkTile084.wsg", &tilemap->tiles[84]);
    loadWsg("brkTile085.wsg", &tilemap->tiles[85]);
    loadWsg("brkTile086.wsg", &tilemap->tiles[86]);
    loadWsg("brkTile087.wsg", &tilemap->tiles[87]);
    loadWsg("brkTile088.wsg", &tilemap->tiles[88]);
    loadWsg("brkTile089.wsg", &tilemap->tiles[89]);
    loadWsg("brkTile090.wsg", &tilemap->tiles[90]);
    loadWsg("brkTile091.wsg", &tilemap->tiles[91]);
    loadWsg("brkTile092.wsg", &tilemap->tiles[92]);
    loadWsg("brkTile093.wsg", &tilemap->tiles[93]);
    loadWsg("brkTile094.wsg", &tilemap->tiles[94]);
    loadWsg("brkTile095.wsg", &tilemap->tiles[95]);
    loadWsg("brkTile096.wsg", &tilemap->tiles[96]);
    loadWsg("brkTile097.wsg", &tilemap->tiles[97]);
    loadWsg("brkTile098.wsg", &tilemap->tiles[98]);
    loadWsg("brkTile099.wsg", &tilemap->tiles[99]);
    loadWsg("brkTile100.wsg", &tilemap->tiles[100]);
    loadWsg("brkTile101.wsg", &tilemap->tiles[101]);
    loadWsg("brkTile102.wsg", &tilemap->tiles[102]);
    loadWsg("brkTile103.wsg", &tilemap->tiles[103]);
    loadWsg("brkTile104.wsg", &tilemap->tiles[104]);
    loadWsg("brkTile105.wsg", &tilemap->tiles[105]);
    loadWsg("brkTile106.wsg", &tilemap->tiles[106]);
    loadWsg("brkTile107.wsg", &tilemap->tiles[107]);
    loadWsg("brkTile108.wsg", &tilemap->tiles[108]);
    loadWsg("brkTile109.wsg", &tilemap->tiles[109]);
    loadWsg("brkTile110.wsg", &tilemap->tiles[110]);
    loadWsg("brkTile111.wsg", &tilemap->tiles[111]);
    loadWsg("brkTile112.wsg", &tilemap->tiles[112]);
    loadWsg("brkTile113.wsg", &tilemap->tiles[113]);
    loadWsg("brkTile114.wsg", &tilemap->tiles[114]);
    loadWsg("brkTile115.wsg", &tilemap->tiles[115]);
    loadWsg("brkTile116.wsg", &tilemap->tiles[116]);
    loadWsg("brkTile117.wsg", &tilemap->tiles[117]);
    loadWsg("brkTile118.wsg", &tilemap->tiles[118]);
    loadWsg("brkTile119.wsg", &tilemap->tiles[119]);
    loadWsg("brkTile120.wsg", &tilemap->tiles[120]);
    loadWsg("brkTile121.wsg", &tilemap->tiles[121]);
    loadWsg("brkTile122.wsg", &tilemap->tiles[122]);
    loadWsg("brkTile123.wsg", &tilemap->tiles[123]);
    loadWsg("brkTile124.wsg", &tilemap->tiles[124]);
    loadWsg("brkTile125.wsg", &tilemap->tiles[125]);
    loadWsg("brkTile126.wsg", &tilemap->tiles[126]);
    loadWsg("brkTile127.wsg", &tilemap->tiles[127]);


    


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
        case TILE_BOUNDARY_1 ... TILE_UNUSED_31:
            return false;
        case TILE_BLOCK_2x1_RED_L ... TILE_UNUSED_127:
            return true;
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
            case TILE_UNUSED_4 ... TILE_UNUSED_F:
            case TILE_UNUSED_28 ... TILE_UNUSED_31:
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