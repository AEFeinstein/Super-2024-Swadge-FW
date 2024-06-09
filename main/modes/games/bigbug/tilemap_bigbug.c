//==============================================================================
// Includes
//==============================================================================
#include <color_utils.h>
#include "tilemap_bigbug.h"

//==============================================================================
// Function Prototypes
//==============================================================================

//==============================================================================
// Functions
//==============================================================================

void bb_initializeTileMap(bb_tilemap_t* tilemap)
{
    wsg_t levelWsg;       ///< A graphic representing the level data where tiles are pixels.
    loadWsg("levelNew.wsg", &levelWsg, true); // levelWsg only needed for this brief scope.
    bb_loadWsgs(tilemap);

    //Set all the tiles
    for(int i = 0; i < TILE_FIELD_WIDTH; i++){
        for(int j = 0; j < TILE_FIELD_HEIGHT; j++){
            uint32_t rgbCol = paletteToRGB(levelWsg.px[(j * levelWsg.w) + i]);
            //blue value used for foreground tiles
            switch(rgbCol & 255){
                case 0:   //0 in wsg land
                    tilemap->fgTiles[i][j] = 1;
                    break;
                case 153: //3 in wsg land
                    tilemap->fgTiles[i][j] = 4;
                    break;
                case 255: //5 in wsg land.
                    tilemap->fgTiles[i][j] = 10;
                    break;
            }

            //printf("green: %u\n", (rgbCol >> 8) & 255);
            //green value used for midground tiles
            switch((rgbCol >> 8) & 255){
                case 0:   //0 in wsg land
                    tilemap->mgTiles[i][j] = false;
                    break;
                case 255: //5 in wsg land
                    tilemap->mgTiles[i][j] = true;
                    break;
            }

            switch((rgbCol >> 16) & 255){//red value
                //will use red for spawns
                default:
                    break;
            }
        }
    }
}

void bb_loadWsgs(bb_tilemap_t* tilemap)
{
    // TILE MAP shenanigans explained:
    // neigbhbors in LURD order (Left, Up, Down, Right) 1 if dirt, 0 if not
    // bin  dec  wsg
    // LURD
    // 0010 2    0
    // 1010 10   1
    // 1000 8    2
    // 0000 0    3

    // 0011 3    4
    // 1011 11   5
    // 1001 9    6
    // 0001 1    7

    // 0111 7    8
    // 1111 15   9
    // 1101 13   10
    // 0101 5    11

    // 0110 6    12
    // 1110 14   13
    // 1100 12   14
    // 0100 4    15

    // The index of bigbug->s1Wsg is the LURD neighbor info.
    // The value within is the wsg graphic.
    // [3,7,0,4,15,11,12,8,2,6,1,5,14,10,13,9]
    //soft
    loadWsg("s1g3.wsg",  &tilemap->s1Wsg[0],  true);
    loadWsg("s1g7.wsg",  &tilemap->s1Wsg[1],  true);
    loadWsg("s1g0.wsg",  &tilemap->s1Wsg[2],  true);
    loadWsg("s1g4.wsg",  &tilemap->s1Wsg[3],  true);
    loadWsg("s1g15.wsg", &tilemap->s1Wsg[4],  true);
    loadWsg("s1g11.wsg", &tilemap->s1Wsg[5],  true);
    loadWsg("s1g12.wsg", &tilemap->s1Wsg[6],  true);
    loadWsg("s1g8.wsg",  &tilemap->s1Wsg[7],  true);
    loadWsg("s1g2.wsg",  &tilemap->s1Wsg[8],  true);
    loadWsg("s1g6.wsg",  &tilemap->s1Wsg[9],  true);
    loadWsg("s1g1.wsg",  &tilemap->s1Wsg[10], true);
    loadWsg("s1g5.wsg",  &tilemap->s1Wsg[11], true);
    loadWsg("s1g14.wsg", &tilemap->s1Wsg[12], true);
    loadWsg("s1g10.wsg", &tilemap->s1Wsg[13], true);
    loadWsg("s1g13.wsg", &tilemap->s1Wsg[14], true);
    loadWsg("s1g9.wsg",  &tilemap->s1Wsg[15], true);
    //soft corners
    loadWsg("s1g016.wsg", &tilemap->s1Wsg[16], true);
    loadWsg("s1g017.wsg", &tilemap->s1Wsg[17], true);
    loadWsg("s1g018.wsg", &tilemap->s1Wsg[18], true);
    loadWsg("s1g019.wsg", &tilemap->s1Wsg[19], true);
    loadWsg("s1g020.wsg", &tilemap->s1Wsg[20], true);
    loadWsg("s1g021.wsg", &tilemap->s1Wsg[21], true);
    loadWsg("s1g022.wsg", &tilemap->s1Wsg[22], true);
    loadWsg("s1g023.wsg", &tilemap->s1Wsg[23], true);
    loadWsg("s1g024.wsg", &tilemap->s1Wsg[24], true);
    loadWsg("s1g025.wsg", &tilemap->s1Wsg[25], true);
    loadWsg("s1g026.wsg", &tilemap->s1Wsg[26], true);
    loadWsg("s1g027.wsg", &tilemap->s1Wsg[27], true);
    loadWsg("s1g028.wsg", &tilemap->s1Wsg[28], true);
    loadWsg("s1g029.wsg", &tilemap->s1Wsg[29], true);
    loadWsg("s1g030.wsg", &tilemap->s1Wsg[30], true);
    loadWsg("s1g031.wsg", &tilemap->s1Wsg[31], true);

    //medium
    loadWsg("m1g3.wsg",  &tilemap->m1Wsg[0],  true);
    loadWsg("m1g7.wsg",  &tilemap->m1Wsg[1],  true);
    loadWsg("m1g0.wsg",  &tilemap->m1Wsg[2],  true);
    loadWsg("m1g4.wsg",  &tilemap->m1Wsg[3],  true);
    loadWsg("m1g15.wsg", &tilemap->m1Wsg[4],  true);
    loadWsg("m1g11.wsg", &tilemap->m1Wsg[5],  true);
    loadWsg("m1g12.wsg", &tilemap->m1Wsg[6],  true);
    loadWsg("m1g8.wsg",  &tilemap->m1Wsg[7],  true);
    loadWsg("m1g2.wsg",  &tilemap->m1Wsg[8],  true);
    loadWsg("m1g6.wsg",  &tilemap->m1Wsg[9],  true);
    loadWsg("m1g1.wsg",  &tilemap->m1Wsg[10], true);
    loadWsg("m1g5.wsg",  &tilemap->m1Wsg[11], true);
    loadWsg("m1g14.wsg", &tilemap->m1Wsg[12], true);
    loadWsg("m1g10.wsg", &tilemap->m1Wsg[13], true);
    loadWsg("m1g13.wsg", &tilemap->m1Wsg[14], true);
    loadWsg("m1g9.wsg",  &tilemap->m1Wsg[15], true);
    //medium corners
    loadWsg("m1g016.wsg", &tilemap->m1Wsg[16], true);
    loadWsg("m1g017.wsg", &tilemap->m1Wsg[17], true);
    loadWsg("m1g018.wsg", &tilemap->m1Wsg[18], true);
    loadWsg("m1g019.wsg", &tilemap->m1Wsg[19], true);
    loadWsg("m1g020.wsg", &tilemap->m1Wsg[20], true);
    loadWsg("m1g021.wsg", &tilemap->m1Wsg[21], true);
    loadWsg("m1g022.wsg", &tilemap->m1Wsg[22], true);
    loadWsg("m1g023.wsg", &tilemap->m1Wsg[23], true);
    loadWsg("m1g024.wsg", &tilemap->m1Wsg[24], true);
    loadWsg("m1g025.wsg", &tilemap->m1Wsg[25], true);
    loadWsg("m1g026.wsg", &tilemap->m1Wsg[26], true);
    loadWsg("m1g027.wsg", &tilemap->m1Wsg[27], true);
    loadWsg("m1g028.wsg", &tilemap->m1Wsg[28], true);
    loadWsg("m1g029.wsg", &tilemap->m1Wsg[29], true);
    loadWsg("m1g030.wsg", &tilemap->m1Wsg[30], true);
    loadWsg("m1g031.wsg", &tilemap->m1Wsg[31], true);

    //hard (I think I like the despeckled variant better)
    loadWsg("d_h1g3.wsg",  &tilemap->h1Wsg[0],  true);
    loadWsg("d_h1g7.wsg",  &tilemap->h1Wsg[1],  true);
    loadWsg("d_h1g0.wsg",  &tilemap->h1Wsg[2],  true);
    loadWsg("d_h1g4.wsg",  &tilemap->h1Wsg[3],  true);
    loadWsg("d_h1g15.wsg", &tilemap->h1Wsg[4],  true);
    loadWsg("d_h1g11.wsg", &tilemap->h1Wsg[5],  true);
    loadWsg("d_h1g12.wsg", &tilemap->h1Wsg[6],  true);
    loadWsg("d_h1g8.wsg",  &tilemap->h1Wsg[7],  true);
    loadWsg("d_h1g2.wsg",  &tilemap->h1Wsg[8],  true);
    loadWsg("d_h1g6.wsg",  &tilemap->h1Wsg[9],  true);
    loadWsg("d_h1g1.wsg",  &tilemap->h1Wsg[10], true);
    loadWsg("d_h1g5.wsg",  &tilemap->h1Wsg[11], true);
    loadWsg("d_h1g14.wsg", &tilemap->h1Wsg[12], true);
    loadWsg("d_h1g10.wsg", &tilemap->h1Wsg[13], true);
    loadWsg("d_h1g13.wsg", &tilemap->h1Wsg[14], true);
    loadWsg("d_h1g9.wsg",  &tilemap->h1Wsg[15], true);
    //hard corners
    loadWsg("d_h1g016.wsg", &tilemap->h1Wsg[16], true);
    loadWsg("d_h1g017.wsg", &tilemap->h1Wsg[17], true);
    loadWsg("d_h1g018.wsg", &tilemap->h1Wsg[18], true);
    loadWsg("d_h1g019.wsg", &tilemap->h1Wsg[19], true);
    loadWsg("d_h1g020.wsg", &tilemap->h1Wsg[20], true);
    loadWsg("d_h1g021.wsg", &tilemap->h1Wsg[21], true);
    loadWsg("d_h1g022.wsg", &tilemap->h1Wsg[22], true);
    loadWsg("d_h1g023.wsg", &tilemap->h1Wsg[23], true);
    loadWsg("d_h1g024.wsg", &tilemap->h1Wsg[24], true);
    loadWsg("d_h1g025.wsg", &tilemap->h1Wsg[25], true);
    loadWsg("d_h1g026.wsg", &tilemap->h1Wsg[26], true);
    loadWsg("d_h1g027.wsg", &tilemap->h1Wsg[27], true);
    loadWsg("d_h1g028.wsg", &tilemap->h1Wsg[28], true);
    loadWsg("d_h1g029.wsg", &tilemap->h1Wsg[29], true);
    loadWsg("d_h1g030.wsg", &tilemap->h1Wsg[30], true);
    loadWsg("d_h1g031.wsg", &tilemap->h1Wsg[31], true);

    //Midground
    loadWsg("mg1_3.wsg",  &tilemap->mg1Wsg[0],  true);
    loadWsg("mg1_7.wsg",  &tilemap->mg1Wsg[1],  true);
    loadWsg("mg1_0.wsg",  &tilemap->mg1Wsg[2],  true);
    loadWsg("mg1_4.wsg",  &tilemap->mg1Wsg[3],  true);
    loadWsg("mg1_15.wsg", &tilemap->mg1Wsg[4],  true);
    loadWsg("mg1_11.wsg", &tilemap->mg1Wsg[5],  true);
    loadWsg("mg1_12.wsg", &tilemap->mg1Wsg[6],  true);
    loadWsg("mg1_8.wsg",  &tilemap->mg1Wsg[7],  true);
    loadWsg("mg1_2.wsg",  &tilemap->mg1Wsg[8],  true);
    loadWsg("mg1_6.wsg",  &tilemap->mg1Wsg[9],  true);
    loadWsg("mg1_1.wsg",  &tilemap->mg1Wsg[10], true);
    loadWsg("mg1_5.wsg",  &tilemap->mg1Wsg[11], true);
    loadWsg("mg1_14.wsg", &tilemap->mg1Wsg[12], true);
    loadWsg("mg1_10.wsg", &tilemap->mg1Wsg[13], true);
    loadWsg("mg1_13.wsg", &tilemap->mg1Wsg[14], true);
    loadWsg("mg1_9.wsg",  &tilemap->mg1Wsg[15], true);
    //Midground corners
    loadWsg("mg1_016.wsg", &tilemap->mg1Wsg[16], true);
    loadWsg("mg1_017.wsg", &tilemap->mg1Wsg[17], true);
    loadWsg("mg1_018.wsg", &tilemap->mg1Wsg[18], true);
    loadWsg("mg1_019.wsg", &tilemap->mg1Wsg[19], true);
    loadWsg("mg1_020.wsg", &tilemap->mg1Wsg[20], true);
    loadWsg("mg1_021.wsg", &tilemap->mg1Wsg[21], true);
    loadWsg("mg1_022.wsg", &tilemap->mg1Wsg[22], true);
    loadWsg("mg1_023.wsg", &tilemap->mg1Wsg[23], true);
    loadWsg("mg1_024.wsg", &tilemap->mg1Wsg[24], true);
    loadWsg("mg1_025.wsg", &tilemap->mg1Wsg[25], true);
    loadWsg("mg1_026.wsg", &tilemap->mg1Wsg[26], true);
    loadWsg("mg1_027.wsg", &tilemap->mg1Wsg[27], true);
    loadWsg("mg1_028.wsg", &tilemap->mg1Wsg[28], true);
    loadWsg("mg1_029.wsg", &tilemap->mg1Wsg[29], true);
    loadWsg("mg1_030.wsg", &tilemap->mg1Wsg[30], true);
    loadWsg("mg1_031.wsg", &tilemap->mg1Wsg[31], true);

    loadWsg("dumpSurface_small.wsg", &tilemap->surfaceWsg, true);
    loadWsg("trash_background.wsg",  &tilemap->bgWsg, true);
}

void bb_drawTileMap(bb_tilemap_t* tilemap, rectangle_t* camera)
{
    int32_t offsetX = (camera->pos.x/2) % 256;
    int32_t offsetY = (camera->pos.y/2) % 256;

    offsetX = (offsetX < 0) ? offsetX + 256 : offsetX;
    offsetY = (offsetX < 0) ? offsetY + 256 : offsetY;

    //draws background
    for ( int32_t x = -1; x <= TFT_WIDTH / 256 + 1; x++){
        //needs more optimizaiton? FIX ME!!!
        drawWsgSimple(&tilemap->surfaceWsg,
                    x * 256 - offsetX,
                    -128-camera->pos.y/2);
        for ( int32_t y = -1; y <= TFT_HEIGHT / 256 + 1; y++){
            if(camera->pos.y/2 + y * 256 - offsetY > -1){
                drawWsgSimple(&tilemap->bgWsg,
                    x * 256 - offsetX,
                    y * 256 - offsetY);
            }
        }
    }


    //setting up variables to draw midground & foreground
    //printf("camera x: %d\n", (bigbug->camera.pos.x >> DECIMAL_BITS));
    //printf("width: %d\n", FIELD_WIDTH);
    int16_t iStart = camera->pos.x / 64;
    int16_t iEnd = iStart + 5;
    int16_t jStart = camera->pos.y / 64;
    int16_t jEnd = jStart + 4;
    if (camera->pos.x < 0){
        iStart -= 1;
        if (camera->pos.x  + FIELD_WIDTH < 0){
            iEnd -= 1;
        }
    }
    if (camera->pos.y < 0){
        jStart -= 1;
        if (camera->pos.y + FIELD_HEIGHT< 0){
            jEnd -= 1;
        }
    }

    
    if(iEnd >= 0 && iStart < TILE_FIELD_WIDTH && jEnd >= 0 && jStart < TILE_FIELD_HEIGHT){
        if(0 > iStart){
            iStart = 0;
        }
        if(TILE_FIELD_WIDTH - 1 < iEnd){
            iEnd = TILE_FIELD_WIDTH - 1;
        }
        if(0 > jStart){
            jStart = 0;
        }
        if(TILE_FIELD_HEIGHT - 1 < jEnd){
            jEnd = TILE_FIELD_HEIGHT - 1;
        }

        // printf("iStart: %d\n", iStart);
        // printf("iEnd: %d\n", iEnd);
        // printf("jStart: %d\n", jStart);
        // printf("jEnd: %d\n", jEnd);

        for (int32_t i = iStart; i <= iEnd; i++){
            for (int32_t j = jStart; j <= jEnd; j++){
                // Draw midground  tiles
                if(tilemap->mgTiles[i][j] == true)
                {
                    //sprite_idx LURD order.
                    uint8_t sprite_idx = 8 * ((i-1 < 0) ? 0 : (tilemap->mgTiles[i-1][j]>0)) +
                                         4 * ((j-1 < 0) ? 0 : (tilemap->mgTiles[i][j-1]>0)) +
                                         2 * ((i+1 > TILE_FIELD_WIDTH - 1) ? 0 : (tilemap->mgTiles[i+1][j]>0)) +
                                         1 * ((j+1 > TILE_FIELD_HEIGHT - 1) ? 0 : (tilemap->mgTiles[i][j+1])>0);
                    //corner_info represents up_left, up_right, down_left, down_right dirt presence (remember >0 is dirt).
                    uint8_t corner_info = 8 * ((i-1 < 0) ? 0 : (j-1 < 0) ? 0 : (tilemap->mgTiles[i-1][j-1]>0)) +
                                         4 * ((i+1 > TILE_FIELD_WIDTH - 1) ? 0 : (j-1 < 0) ? 0 : (tilemap->mgTiles[i+1][j-1]>0)) +
                                         2 * ((i-1 < 0) ? 0 : (j+1 > TILE_FIELD_HEIGHT - 1) ? 0 : (tilemap->mgTiles[i-1][j+1]>0)) +
                                         1 * ((i+1 > TILE_FIELD_WIDTH - 1) ? 0 : (j+1 > TILE_FIELD_HEIGHT - 1) ? 0 : (tilemap->mgTiles[i+1][j+1])>0);
                    
                    switch(sprite_idx){
                        case 15:
                            //This case has dirt left, up, right, and down. This figures out if there is some diagonal air though.
                            
                            switch(corner_info){
                                case 0: //0000
                                    bb_DrawMidgroundCornerTile(tilemap, camera, (uint8_t[]){12, 5, 10, 3},
                                    i,
                                    j);
                                    break;
                                case 1: //0001
                                    bb_DrawMidgroundCornerTile(tilemap, camera, (uint8_t[]){12, 5, 10, 11},
                                    i, 
                                    j);
                                    break;
                                case 2:  //0010
                                    bb_DrawMidgroundCornerTile(tilemap, camera, (uint8_t[]){12, 5, 2, 3},
                                    i,
                                    j);
                                    break;
                                case 3:  //0011
                                    bb_DrawMidgroundCornerTile(tilemap, camera, (uint8_t[]){12, 5, 14, 7},
                                    i,
                                    j);
                                    break;
                                case 4:  //0100
                                    bb_DrawMidgroundCornerTile(tilemap, camera, (uint8_t[]){12, 13, 10, 3},
                                    i,
                                    j);
                                    break;
                                case 5:  //0101
                                    bb_DrawMidgroundCornerTile(tilemap, camera, (uint8_t[]){12, 13, 10, 11},
                                    i,
                                    j);
                                    break;
                                case 6:  //0110
                                    bb_DrawMidgroundCornerTile(tilemap, camera, (uint8_t[]){12, 1, 14, 3},
                                    i,
                                    j);
                                    break;
                                case 7:  //0111
                                    bb_DrawMidgroundCornerTile(tilemap, camera, (uint8_t[]){12, 13, 14, 15},
                                    i,
                                    j);
                                    break;
                                case 8:  //1000
                                    bb_DrawMidgroundCornerTile(tilemap, camera, (uint8_t[]){4, 5, 10, 3},
                                    i,
                                    j);
                                    break;
                                case 9:  //1001
                                    bb_DrawMidgroundCornerTile(tilemap, camera, (uint8_t[]){4, 5, 10, 11},
                                    i,
                                    j);
                                    break;
                                case 10: //1010
                                    bb_DrawMidgroundCornerTile(tilemap, camera, (uint8_t[]){4, 5, 2, 3},
                                    i,
                                    j);
                                    break;
                                case 11: //1011
                                    bb_DrawMidgroundCornerTile(tilemap, camera, (uint8_t[]){4, 5, 6, 7},
                                    i,
                                    j);
                                    break;
                                case 12: //1100
                                    bb_DrawMidgroundCornerTile(tilemap, camera, (uint8_t[]){8, 1, 10, 3},
                                    i,
                                    j);
                                    break;
                                case 13: //1101
                                    bb_DrawMidgroundCornerTile(tilemap, camera, (uint8_t[]){8, 9, 10, 11},
                                    i,
                                    j);
                                    break;
                                case 14: //1110
                                    bb_DrawMidgroundCornerTile(tilemap, camera, (uint8_t[]){0, 1, 2, 3},
                                    i,
                                    j);
                                    break;
                                case 15: //1111
                                    drawWsgSimpleScaled(&tilemap->mg1Wsg[15],
                                                i * 64 - camera->pos.x,
                                                j * 64 - camera->pos.y,
                                                2, 2);
                                    break;
                            }
                            break;
                        default:
                            drawWsgSimpleScaled(&tilemap->mg1Wsg[sprite_idx],
                                                i * 64 - camera->pos.x,
                                                j * 64 - camera->pos.y,
                                                2,
                                                2);
                    }
                    if((sprite_idx & 0b1100) == 0b1100 && !(corner_info & 0b1000)){
                        //FIX ME!!!!
                        //see about getting wsg for coord once
                        drawWsgSimpleScaled(&tilemap->mg1Wsg[28], i  * 64 - camera->pos.x, j * 64 - camera->pos.y,2, 2);
                    }
                    if((sprite_idx & 0b0110) == 0b0110 && !(corner_info & 0b0100)){
                        drawWsgSimpleScaled(&tilemap->mg1Wsg[21], i  * 64 - camera->pos.x + 32, j * 64 - camera->pos.y,      2, 2);
                    }
                    if((sprite_idx & 0b0011) == 0b0011 && !(corner_info & 0b0001)){
                        drawWsgSimpleScaled(&tilemap->mg1Wsg[19], i  * 64 - camera->pos.x + 32, j * 64 - camera->pos.y + 32, 2, 2);
                    }
                    if((sprite_idx & 0b1001) == 0b1001 && !(corner_info & 0b0010)){
                        drawWsgSimpleScaled(&tilemap->mg1Wsg[26], i  * 64 - camera->pos.x,      j * 64 - camera->pos.y + 32, 2, 2);
                    }
                }
                
                // Draw foreground tiles
                if(tilemap->fgTiles[i][j] >= 1){
                    //drawWsgTile(&bigbug->dirtWsg, i * 64 - bigbug->camera.pos.x, j * 64 - bigbug->camera.pos.y);
                    //sprite_idx LURD order.
                    uint8_t sprite_idx = 8 * ((i-1 < 0) ? 0 : (tilemap->fgTiles[i-1][j]>0)) +
                                    4 * ((j-1 < 0) ? 0 : (tilemap->fgTiles[i][j-1]>0)) +
                                    2 * ((i+1 > TILE_FIELD_WIDTH - 1) ? 0 : (tilemap->fgTiles[i+1][j]>0)) +
                                    1 * ((j+1 > TILE_FIELD_HEIGHT - 1) ? 0 : (tilemap->fgTiles[i][j+1])>0);
                    //corner_info represents up_left, up_right, down_left, down_right dirt presence (remember >0 is dirt).
                    uint8_t corner_info = 8 * ((i-1 < 0) ? 0 : (j-1 < 0) ? 0 : (tilemap->fgTiles[i-1][j-1]>0)) +
                                    4 * ((i+1 > TILE_FIELD_WIDTH - 1) ? 0 : (j-1 < 0) ? 0 : (tilemap->fgTiles[i+1][j-1]>0)) +
                                    2 * ((i-1 < 0) ? 0 : (j+1 > TILE_FIELD_HEIGHT - 1) ? 0 : (tilemap->fgTiles[i-1][j+1]>0)) +
                                    1 * ((i+1 > TILE_FIELD_WIDTH - 1) ? 0 : (j+1 > TILE_FIELD_HEIGHT - 1) ? 0 : (tilemap->fgTiles[i+1][j+1])>0);
                    switch(sprite_idx){
                        case 15:
                            //This case has dirt left, up, right, and down. This figures out if there is some diagonal air though.
                            
                            switch(corner_info){
                                case 0: //0000
                                    bb_DrawForegroundCornerTile(tilemap, camera, (uint8_t[]){12, 5, 10, 3},
                                    i,
                                    j);
                                    break;
                                case 1: //0001
                                    bb_DrawForegroundCornerTile(tilemap, camera, (uint8_t[]){12, 5, 10, 11},
                                    i, 
                                    j);
                                    break;
                                case 2:  //0010
                                    bb_DrawForegroundCornerTile(tilemap, camera, (uint8_t[]){12, 5, 2, 3},
                                    i,
                                    j);
                                    break;
                                case 3:  //0011
                                    bb_DrawForegroundCornerTile(tilemap, camera, (uint8_t[]){12, 5, 14, 7},
                                    i,
                                    j);
                                    break;
                                case 4:  //0100
                                    bb_DrawForegroundCornerTile(tilemap, camera, (uint8_t[]){12, 13, 10, 3},
                                    i,
                                    j);
                                    break;
                                case 5:  //0101
                                    bb_DrawForegroundCornerTile(tilemap, camera, (uint8_t[]){12, 13, 10, 11},
                                    i,
                                    j);
                                    break;
                                case 6:  //0110
                                    bb_DrawForegroundCornerTile(tilemap, camera, (uint8_t[]){12, 1, 14, 3},
                                    i,
                                    j);
                                    break;
                                case 7:  //0111
                                    bb_DrawForegroundCornerTile(tilemap, camera, (uint8_t[]){12, 13, 14, 15},
                                    i,
                                    j);
                                    break;
                                case 8:  //1000
                                    bb_DrawForegroundCornerTile(tilemap, camera, (uint8_t[]){4, 5, 10, 3},
                                    i,
                                    j);
                                    break;
                                case 9:  //1001
                                    bb_DrawForegroundCornerTile(tilemap, camera, (uint8_t[]){4, 5, 10, 11},
                                    i,
                                    j);
                                    break;
                                case 10: //1010
                                    bb_DrawForegroundCornerTile(tilemap, camera, (uint8_t[]){4, 5, 2, 3},
                                    i,
                                    j);
                                    break;
                                case 11: //1011
                                    bb_DrawForegroundCornerTile(tilemap, camera, (uint8_t[]){4, 5, 6, 7},
                                    i,
                                    j);
                                    break;
                                case 12: //1100
                                    bb_DrawForegroundCornerTile(tilemap, camera, (uint8_t[]){8, 1, 10, 3},
                                    i,
                                    j);
                                    break;
                                case 13: //1101
                                    bb_DrawForegroundCornerTile(tilemap, camera, (uint8_t[]){8, 9, 10, 11},
                                    i,
                                    j);
                                    break;
                                case 14: //1110
                                    bb_DrawForegroundCornerTile(tilemap, camera, (uint8_t[]){0, 1, 2, 3},
                                    i,
                                    j);
                                    break;
                                case 15: //1111
                                    drawWsgSimpleScaled(&tilemap->m1Wsg[15],
                                                i * 64 - camera->pos.x,
                                                j * 64 - camera->pos.y,
                                                2, 2);
                                    break;
                            }
                            break;
                        default:
                            drawWsgSimpleScaled(&(*bb_GetWsgArrForForegroundCoord(tilemap, i,j))[sprite_idx],
                                                i * 64 - camera->pos.x,
                                                j * 64 - camera->pos.y,
                                                2,
                                                2);
                    }
                    if((sprite_idx & 0b1100) == 0b1100 && !(corner_info & 0b1000)){
                        //FIX ME!!!!
                        //see about getting wsg for coord once
                        drawWsgSimpleScaled(&(*bb_GetWsgArrForForegroundCoord(tilemap, i,j))[28], i  * 64 - camera->pos.x, j * 64 - camera->pos.y,2, 2);
                    }
                    if((sprite_idx & 0b0110) == 0b0110 && !(corner_info & 0b0100)){
                        wsg_t (*tileset)[32] = bb_GetWsgArrForForegroundCoord(tilemap, i, j);

                        drawWsgSimpleScaled(&(*tileset)[21], i  * 64 - camera->pos.x + 32, j * 64 - camera->pos.y,      2, 2);
                    }
                    if((sprite_idx & 0b0011) == 0b0011 && !(corner_info & 0b0001)){
                        wsg_t (*tileset)[32] = bb_GetWsgArrForForegroundCoord(tilemap, i, j);

                        drawWsgSimpleScaled(&(*tileset)[19], i  * 64 - camera->pos.x + 32, j * 64 - camera->pos.y + 32, 2, 2);
                    }
                    if((sprite_idx & 0b1001) == 0b1001 && !(corner_info & 0b0010)){
                        wsg_t (*tileset)[32] = bb_GetWsgArrForForegroundCoord(tilemap, i, j);

                        drawWsgSimpleScaled(&(*tileset)[26], i  * 64 - camera->pos.x,      j * 64 - camera->pos.y + 32, 2, 2);
                    }
                }
            }
        }
    }
}

/**
 * @brief Piece together a corner tile and draw it.
 */
void bb_DrawForegroundCornerTile(bb_tilemap_t* tilemap, rectangle_t* camera, const uint8_t* idx_arr, const uint32_t i, const uint32_t j)
{
    vec_t tilePos = {
            .x = i  * 64 - camera->pos.x,
            .y = j * 64 - camera->pos.y
        };

    wsg_t (*tileset)[32] = bb_GetWsgArrForForegroundCoord(tilemap, i, j);

    drawWsgSimpleScaled(&(*tileset)[idx_arr[0]+16], tilePos.x,      tilePos.y,      2, 2);
    drawWsgSimpleScaled(&(*tileset)[idx_arr[1]+16], tilePos.x + 32, tilePos.y,      2, 2);
    drawWsgSimpleScaled(&(*tileset)[idx_arr[2]+16], tilePos.x,      tilePos.y + 32, 2, 2);
    drawWsgSimpleScaled(&(*tileset)[idx_arr[3]+16], tilePos.x + 32, tilePos.y + 32, 2, 2);
}

/**
 * @brief Piece together a corner tile and draw it.
 */
void bb_DrawMidgroundCornerTile(bb_tilemap_t* tilemap, rectangle_t* camera, const uint8_t* idx_arr, const uint32_t i, const uint32_t j)
{
    vec_t tilePos = {
            .x = i  * 64 - camera->pos.x,
            .y = j * 64 - camera->pos.y
        };

    drawWsgSimpleScaled(&tilemap->mg1Wsg[idx_arr[0]+16], tilePos.x,      tilePos.y,      2, 2);
    drawWsgSimpleScaled(&tilemap->mg1Wsg[idx_arr[1]+16], tilePos.x + 32, tilePos.y,      2, 2);
    drawWsgSimpleScaled(&tilemap->mg1Wsg[idx_arr[2]+16], tilePos.x,      tilePos.y + 32, 2, 2);
    drawWsgSimpleScaled(&tilemap->mg1Wsg[idx_arr[3]+16], tilePos.x + 32, tilePos.y + 32, 2, 2);
}

wsg_t (*bb_GetWsgArrForForegroundCoord(bb_tilemap_t* tilemap, const uint32_t i, const uint32_t j))[32]
{
    if(tilemap->fgTiles[i][j]>4){
        return &tilemap->h1Wsg;
    }
    else if(tilemap->fgTiles[i][j]>1){
        return &tilemap->m1Wsg;
    }
    return &tilemap->s1Wsg;
}