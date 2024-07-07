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
    loadWsg("s1g16.wsg", &tilemap->s1Wsg[16], true);
    loadWsg("s1g17.wsg", &tilemap->s1Wsg[17], true);
    loadWsg("s1g18.wsg", &tilemap->s1Wsg[18], true);
    loadWsg("s1g19.wsg", &tilemap->s1Wsg[19], true);
    loadWsg("s1g20.wsg", &tilemap->s1Wsg[20], true);
    loadWsg("s1g21.wsg", &tilemap->s1Wsg[21], true);
    loadWsg("s1g22.wsg", &tilemap->s1Wsg[22], true);
    loadWsg("s1g23.wsg", &tilemap->s1Wsg[23], true);
    loadWsg("s1g24.wsg", &tilemap->s1Wsg[24], true);
    loadWsg("s1g25.wsg", &tilemap->s1Wsg[25], true);
    loadWsg("s1g26.wsg", &tilemap->s1Wsg[26], true);
    loadWsg("s1g27.wsg", &tilemap->s1Wsg[27], true);
    loadWsg("s1g28.wsg", &tilemap->s1Wsg[28], true);
    loadWsg("s1g29.wsg", &tilemap->s1Wsg[29], true);
    loadWsg("s1g30.wsg", &tilemap->s1Wsg[30], true);
    loadWsg("s1g31.wsg", &tilemap->s1Wsg[31], true);

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
    loadWsg("m1g16.wsg", &tilemap->m1Wsg[16], true);
    loadWsg("m1g17.wsg", &tilemap->m1Wsg[17], true);
    loadWsg("m1g18.wsg", &tilemap->m1Wsg[18], true);
    loadWsg("m1g19.wsg", &tilemap->m1Wsg[19], true);
    loadWsg("m1g20.wsg", &tilemap->m1Wsg[20], true);
    loadWsg("m1g21.wsg", &tilemap->m1Wsg[21], true);
    loadWsg("m1g22.wsg", &tilemap->m1Wsg[22], true);
    loadWsg("m1g23.wsg", &tilemap->m1Wsg[23], true);
    loadWsg("m1g24.wsg", &tilemap->m1Wsg[24], true);
    loadWsg("m1g25.wsg", &tilemap->m1Wsg[25], true);
    loadWsg("m1g26.wsg", &tilemap->m1Wsg[26], true);
    loadWsg("m1g27.wsg", &tilemap->m1Wsg[27], true);
    loadWsg("m1g28.wsg", &tilemap->m1Wsg[28], true);
    loadWsg("m1g29.wsg", &tilemap->m1Wsg[29], true);
    loadWsg("m1g30.wsg", &tilemap->m1Wsg[30], true);
    loadWsg("m1g31.wsg", &tilemap->m1Wsg[31], true);

    //hard
    loadWsg("h1g3.wsg",  &tilemap->h1Wsg[0],  true);
    loadWsg("h1g7.wsg",  &tilemap->h1Wsg[1],  true);
    loadWsg("h1g0.wsg",  &tilemap->h1Wsg[2],  true);
    loadWsg("h1g4.wsg",  &tilemap->h1Wsg[3],  true);
    loadWsg("h1g15.wsg", &tilemap->h1Wsg[4],  true);
    loadWsg("h1g11.wsg", &tilemap->h1Wsg[5],  true);
    loadWsg("h1g12.wsg", &tilemap->h1Wsg[6],  true);
    loadWsg("h1g8.wsg",  &tilemap->h1Wsg[7],  true);
    loadWsg("h1g2.wsg",  &tilemap->h1Wsg[8],  true);
    loadWsg("h1g6.wsg",  &tilemap->h1Wsg[9],  true);
    loadWsg("h1g1.wsg",  &tilemap->h1Wsg[10], true);
    loadWsg("h1g5.wsg",  &tilemap->h1Wsg[11], true);
    loadWsg("h1g14.wsg", &tilemap->h1Wsg[12], true);
    loadWsg("h1g10.wsg", &tilemap->h1Wsg[13], true);
    loadWsg("h1g13.wsg", &tilemap->h1Wsg[14], true);
    loadWsg("h1g9.wsg",  &tilemap->h1Wsg[15], true);
    //hard corners
    loadWsg("h1g16.wsg", &tilemap->h1Wsg[16], true);
    loadWsg("h1g17.wsg", &tilemap->h1Wsg[17], true);
    loadWsg("h1g18.wsg", &tilemap->h1Wsg[18], true);
    loadWsg("h1g19.wsg", &tilemap->h1Wsg[19], true);
    loadWsg("h1g20.wsg", &tilemap->h1Wsg[20], true);
    loadWsg("h1g21.wsg", &tilemap->h1Wsg[21], true);
    loadWsg("h1g22.wsg", &tilemap->h1Wsg[22], true);
    loadWsg("h1g23.wsg", &tilemap->h1Wsg[23], true);
    loadWsg("h1g24.wsg", &tilemap->h1Wsg[24], true);
    loadWsg("h1g25.wsg", &tilemap->h1Wsg[25], true);
    loadWsg("h1g26.wsg", &tilemap->h1Wsg[26], true);
    loadWsg("h1g27.wsg", &tilemap->h1Wsg[27], true);
    loadWsg("h1g28.wsg", &tilemap->h1Wsg[28], true);
    loadWsg("h1g29.wsg", &tilemap->h1Wsg[29], true);
    loadWsg("h1g30.wsg", &tilemap->h1Wsg[30], true);
    loadWsg("h1g31.wsg", &tilemap->h1Wsg[31], true);

    //Midground
    loadWsg("mg3.wsg",  &tilemap->mg1Wsg[0],  true);
    loadWsg("mg7.wsg",  &tilemap->mg1Wsg[1],  true);
    loadWsg("mg0.wsg",  &tilemap->mg1Wsg[2],  true);
    loadWsg("mg4.wsg",  &tilemap->mg1Wsg[3],  true);
    loadWsg("mg15.wsg", &tilemap->mg1Wsg[4],  true);
    loadWsg("mg11.wsg", &tilemap->mg1Wsg[5],  true);
    loadWsg("mg12.wsg", &tilemap->mg1Wsg[6],  true);
    loadWsg("mg8.wsg",  &tilemap->mg1Wsg[7],  true);
    loadWsg("mg2.wsg",  &tilemap->mg1Wsg[8],  true);
    loadWsg("mg6.wsg",  &tilemap->mg1Wsg[9],  true);
    loadWsg("mg1.wsg",  &tilemap->mg1Wsg[10], true);
    loadWsg("mg5.wsg",  &tilemap->mg1Wsg[11], true);
    loadWsg("mg14.wsg", &tilemap->mg1Wsg[12], true);
    loadWsg("mg10.wsg", &tilemap->mg1Wsg[13], true);
    loadWsg("mg13.wsg", &tilemap->mg1Wsg[14], true);
    loadWsg("mg9.wsg",  &tilemap->mg1Wsg[15], true);
    //Midground corners
    loadWsg("mg016.wsg", &tilemap->mg1Wsg[16], true);
    loadWsg("mg017.wsg", &tilemap->mg1Wsg[17], true);
    loadWsg("mg018.wsg", &tilemap->mg1Wsg[18], true);
    loadWsg("mg019.wsg", &tilemap->mg1Wsg[19], true);
    loadWsg("mg020.wsg", &tilemap->mg1Wsg[20], true);
    loadWsg("mg021.wsg", &tilemap->mg1Wsg[21], true);
    loadWsg("mg022.wsg", &tilemap->mg1Wsg[22], true);
    loadWsg("mg023.wsg", &tilemap->mg1Wsg[23], true);
    loadWsg("mg024.wsg", &tilemap->mg1Wsg[24], true);
    loadWsg("mg025.wsg", &tilemap->mg1Wsg[25], true);
    loadWsg("mg026.wsg", &tilemap->mg1Wsg[26], true);
    loadWsg("mg027.wsg", &tilemap->mg1Wsg[27], true);
    loadWsg("mg028.wsg", &tilemap->mg1Wsg[28], true);
    loadWsg("mg029.wsg", &tilemap->mg1Wsg[29], true);
    loadWsg("mg030.wsg", &tilemap->mg1Wsg[30], true);
    loadWsg("mg031.wsg", &tilemap->mg1Wsg[31], true);

    loadWsg("baked_Landfill2.wsg", &tilemap->surface1Wsg, true);
    loadWsg("baked_Landfill3.wsg", &tilemap->surface2Wsg, true);
    loadWsg("trash_background.wsg",  &tilemap->bgWsg, true);
}

void bb_drawTileMap(bb_tilemap_t* tilemap, rectangle_t* camera)
{
    int32_t offsetX1 = (camera->pos.x/3) % 256;
    int32_t offsetX2 = (camera->pos.x/2) % 256;

    offsetX1 = (offsetX1 < 0) ? offsetX1 + 256 : offsetX1;
    offsetX2 = (offsetX2 < 0) ? offsetX2 + 256 : offsetX2;

    //draws background
    for ( int32_t x = -1; x <= TFT_WIDTH / 256 + 1; x++){
        //needs more optimizaiton? FIX ME!!!
        drawWsgSimple(&tilemap->surface2Wsg,
                    x * 256 - offsetX1,
                    -64-camera->pos.y/3);
    }

    for ( int32_t x = -1; x <= TFT_WIDTH / 256 + 1; x++){
        //needs more optimizaiton? FIX ME!!!
        drawWsgSimple(&tilemap->surface1Wsg,
                    x * 256 - offsetX2,
                    -camera->pos.y/2);
    }


    //setting up variables to draw midground & foreground
    //printf("camera x: %d\n", (bigbug->camera.pos.x >> DECIMAL_BITS));
    //printf("width: %d\n", FIELD_WIDTH);
    int16_t iStart = camera->pos.x / TILE_SIZE;
    int16_t iEnd = iStart + TFT_WIDTH / TILE_SIZE + 1;
    int16_t jStart = camera->pos.y / TILE_SIZE;
    int16_t jEnd = jStart + TFT_HEIGHT / TILE_SIZE + 1;

    iStart -= (camera->pos.x < 0);
    iEnd -= (camera->pos.x < 0);
    if (camera->pos.x  + FIELD_WIDTH < 0){
        iEnd = -1;
    }

    jStart -= (camera->pos.y < 0);
    jEnd -= (camera->pos.y < 0);
    if (camera->pos.y + FIELD_HEIGHT< 0){
        jEnd = -1;
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
                                                i * TILE_SIZE - camera->pos.x,
                                                j * TILE_SIZE - camera->pos.y,
                                                2, 2);
                                    break;
                            }
                            break;
                        default:
                            drawWsgSimpleScaled(&tilemap->mg1Wsg[sprite_idx],
                                                i * TILE_SIZE - camera->pos.x,
                                                j * TILE_SIZE - camera->pos.y,
                                                1, 1);
                    }
                    if((sprite_idx & 0b1100) == 0b1100 && !(corner_info & 0b1000)){
                        //FIX ME!!!!
                        //see about getting wsg for coord once
                        drawWsgSimpleScaled(&tilemap->mg1Wsg[28], i  * TILE_SIZE - camera->pos.x, j * TILE_SIZE - camera->pos.y, 1, 1);
                    }
                    if((sprite_idx & 0b0110) == 0b0110 && !(corner_info & 0b0100)){
                        drawWsgSimpleScaled(&tilemap->mg1Wsg[21], i  * TILE_SIZE - camera->pos.x + HALF_TILE, j * TILE_SIZE - camera->pos.y,      1, 1);
                    }
                    if((sprite_idx & 0b0011) == 0b0011 && !(corner_info & 0b0001)){
                        drawWsgSimpleScaled(&tilemap->mg1Wsg[19], i  * TILE_SIZE - camera->pos.x + HALF_TILE, j * TILE_SIZE - camera->pos.y + HALF_TILE, 1, 1);
                    }
                    if((sprite_idx & 0b1001) == 0b1001 && !(corner_info & 0b0010)){
                        drawWsgSimpleScaled(&tilemap->mg1Wsg[26], i  * TILE_SIZE - camera->pos.x,      j * TILE_SIZE - camera->pos.y + HALF_TILE, 1, 1);
                    }
                }
                
                // Draw foreground tiles
                if(tilemap->fgTiles[i][j] >= 1){
                    //drawWsgTile(&bigbug->dirtWsg, i * TILE_SIZE - bigbug->camera.pos.x, j * TILE_SIZE - bigbug->camera.pos.y);
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
                                                i * TILE_SIZE - camera->pos.x,
                                                j * TILE_SIZE - camera->pos.y,
                                                1, 1);
                                    break;
                            }
                            break;
                        default:
                            drawWsgSimpleScaled(&(*bb_GetWsgArrForForegroundCoord(tilemap, i,j))[sprite_idx],
                                                i * TILE_SIZE - camera->pos.x,
                                                j * TILE_SIZE - camera->pos.y,
                                                1, 1);
                    }
                    if((sprite_idx & 0b1100) == 0b1100 && !(corner_info & 0b1000)){
                        //FIX ME!!!!
                        //see about getting wsg for coord once
                        drawWsgSimpleScaled(&(*bb_GetWsgArrForForegroundCoord(tilemap, i,j))[28], i  * TILE_SIZE - camera->pos.x, j * TILE_SIZE - camera->pos.y, 1, 1);
                    }
                    if((sprite_idx & 0b0110) == 0b0110 && !(corner_info & 0b0100)){
                        wsg_t (*tileset)[32] = bb_GetWsgArrForForegroundCoord(tilemap, i, j);

                        drawWsgSimpleScaled(&(*tileset)[21], i  * TILE_SIZE - camera->pos.x + HALF_TILE, j * TILE_SIZE - camera->pos.y,      1, 1);
                    }
                    if((sprite_idx & 0b0011) == 0b0011 && !(corner_info & 0b0001)){
                        wsg_t (*tileset)[32] = bb_GetWsgArrForForegroundCoord(tilemap, i, j);

                        drawWsgSimpleScaled(&(*tileset)[19], i  * TILE_SIZE - camera->pos.x + HALF_TILE, j * TILE_SIZE - camera->pos.y + HALF_TILE, 1, 1);
                    }
                    if((sprite_idx & 0b1001) == 0b1001 && !(corner_info & 0b0010)){
                        wsg_t (*tileset)[32] = bb_GetWsgArrForForegroundCoord(tilemap, i, j);

                        drawWsgSimpleScaled(&(*tileset)[26], i  * TILE_SIZE - camera->pos.x,      j * TILE_SIZE - camera->pos.y + HALF_TILE, 1, 1);
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
            .x = i  * TILE_SIZE - camera->pos.x,
            .y = j * TILE_SIZE - camera->pos.y
        };

    wsg_t (*tileset)[32] = bb_GetWsgArrForForegroundCoord(tilemap, i, j);

    drawWsgSimpleScaled(&(*tileset)[idx_arr[0]+16], tilePos.x,      tilePos.y,      1, 1);
    drawWsgSimpleScaled(&(*tileset)[idx_arr[1]+16], tilePos.x + HALF_TILE, tilePos.y,      1, 1);
    drawWsgSimpleScaled(&(*tileset)[idx_arr[2]+16], tilePos.x,      tilePos.y + HALF_TILE, 1, 1);
    drawWsgSimpleScaled(&(*tileset)[idx_arr[3]+16], tilePos.x + HALF_TILE, tilePos.y + HALF_TILE, 1, 1);
}

/**
 * @brief Piece together a corner tile and draw it.
 */
void bb_DrawMidgroundCornerTile(bb_tilemap_t* tilemap, rectangle_t* camera, const uint8_t* idx_arr, const uint32_t i, const uint32_t j)
{
    vec_t tilePos = {
            .x = i  * TILE_SIZE - camera->pos.x,
            .y = j * TILE_SIZE - camera->pos.y
        };

    drawWsgSimpleScaled(&tilemap->mg1Wsg[idx_arr[0]+16], tilePos.x,      tilePos.y,      1, 1);
    drawWsgSimpleScaled(&tilemap->mg1Wsg[idx_arr[1]+16], tilePos.x + HALF_TILE, tilePos.y,      1, 1);
    drawWsgSimpleScaled(&tilemap->mg1Wsg[idx_arr[2]+16], tilePos.x,      tilePos.y + HALF_TILE, 1, 1);
    drawWsgSimpleScaled(&tilemap->mg1Wsg[idx_arr[3]+16], tilePos.x + HALF_TILE, tilePos.y + HALF_TILE, 1, 1);
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