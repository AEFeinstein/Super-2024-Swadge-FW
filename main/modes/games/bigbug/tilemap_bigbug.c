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
                    tilemap->mgTiles[i][j] = 0;
                    break;
                case 255: //5 in wsg land
                    tilemap->mgTiles[i][j] = tilemap->fgTiles[i][j];
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
    loadWsg("headlampLookup.wsg", &tilemap->headlampWsg, true);//122 x 107 pixels

    loadWsg("baked_Landfill2.wsg", &tilemap->surface1Wsg, true);
    loadWsg("baked_Landfill3.wsg", &tilemap->surface2Wsg, true);
    loadWsg("trash_background.wsg",  &tilemap->bgWsg, true);

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

    // The index of bigbug->fore_s_Wsg is the LURD neighbor info.
    // The value within is the wsg graphic.
    // [3,7,0,4,15,11,12,8,2,6,1,5,14,10,13,9]

    //Midground
    for(int16_t i = 0; i < 120; i++){
        char filename[14];
        snprintf(filename, sizeof(filename), "mid_s_%d.wsg", i);
        loadWsg(filename, &tilemap->mid_s_Wsg[i], true);

        snprintf(filename, sizeof(filename), "mid_m_%d.wsg", i);
        loadWsg(filename, &tilemap->mid_m_Wsg[i], true);

        snprintf(filename, sizeof(filename), "mid_h_%d.wsg", i);
        loadWsg(filename, &tilemap->mid_h_Wsg[i], true);
    }

    //Foreground
    for(int16_t i = 0; i < 216; i++){
        char filename[15];
        snprintf(filename, sizeof(filename), "fore_s_%d.wsg", i);
        loadWsg(filename, &tilemap->mid_s_Wsg[i], true);

        snprintf(filename, sizeof(filename), "fore_m_%d.wsg", i);
        loadWsg(filename, &tilemap->mid_m_Wsg[i], true);

        snprintf(filename, sizeof(filename), "fore_h_%d.wsg", i);
        loadWsg(filename, &tilemap->mid_h_Wsg[i], true);
    }
}

void bb_drawTileMap(bb_tilemap_t* tilemap, rectangle_t* camera, vec_t* garbotnikPos)
{
    // font_t ibm;
    // loadFont("ibm_vga8.font", &ibm, false);

 

    int32_t offsetX1 = (camera->pos.x/3) % 400;
    int32_t offsetX2 = (camera->pos.x/2) % 400;

    offsetX1 = (offsetX1 < 0) ? offsetX1 + 400 : offsetX1;
    offsetX2 = (offsetX2 < 0) ? offsetX2 + 400 : offsetX2;

    // printf("camera y: %d\n", camera->pos.y);

    //draws background
    if(camera->pos.y < 170 && camera->pos.y > -907)
    {
        for ( int32_t x = -1; x <= TFT_WIDTH / 400 + 1; x++){
            drawWsgSimple(&tilemap->surface2Wsg,
                        x * 400 - offsetX1,
                        -64-camera->pos.y/3);
        }
    }

    //draws the closer background
    if(camera->pos.y < 1014 && camera->pos.y > -480){
        for ( int32_t x = -1; x <= TFT_WIDTH / 400 + 1; x++){
            drawWsgSimple(&tilemap->surface1Wsg,
                        x * 400 - offsetX2,
                        -camera->pos.y/2);
        }
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

        int32_t brightness;

        for (int32_t i = iStart; i <= iEnd; i++){
            for (int32_t j = jStart; j <= jEnd; j++){
                vec_t tilePos = {
                    .x = i  * TILE_SIZE - camera->pos.x,
                    .y = j  * TILE_SIZE - camera->pos.y
                };
                             
                // Draw midground  tiles
                if(tilemap->mgTiles[i][j] > 0)
                {
                    wsg_t (*wsgMidgroundArrayPtr)[120] = bb_GetMidgroundWsgArrForCoord(tilemap, i, j);

                    //sprite_idx LURD order.
                    int8_t sprite_idx = 8 * ((i-1 < 0) ? 0 : (tilemap->mgTiles[i-1][j]>0)) +
                                        4 * ((j-1 < 0) ? 0 : (tilemap->mgTiles[i][j-1]>0)) +
                                        2 * ((i+1 > TILE_FIELD_WIDTH - 1) ? 0 : (tilemap->mgTiles[i+1][j]>0)) +
                                        1 * ((j+1 > TILE_FIELD_HEIGHT - 1) ? 0 : (tilemap->mgTiles[i][j+1])>0);
                    //corner_info represents up_left, up_right, down_left, down_right dirt presence (remember >0 is dirt).
                    int8_t corner_info = 8 * ((i-1 < 0) ? 0 : (j-1 < 0) ? 0 : (tilemap->mgTiles[i-1][j-1]>0)) +
                                         4 * ((i+1 > TILE_FIELD_WIDTH - 1) ? 0 : (j-1 < 0) ? 0 : (tilemap->mgTiles[i+1][j-1]>0)) +
                                         2 * ((i-1 < 0) ? 0 : (j+1 > TILE_FIELD_HEIGHT - 1) ? 0 : (tilemap->mgTiles[i-1][j+1]>0)) +
                                         1 * ((i+1 > TILE_FIELD_WIDTH - 1) ? 0 : (j+1 > TILE_FIELD_HEIGHT - 1) ? 0 : (tilemap->mgTiles[i+1][j+1])>0);

                    // Top Left
                    // 0 11xx 1xxx
                    // 4 10xx xxxx
                    // 8 01xx xxxx
                    // 12 00xx xxxx
                    // 16 11xx 0xxx
                    vec_t lookup = {tilePos.x + 8 - garbotnikPos->x + tilemap->headlampWsg.w,
                                    tilePos.y - garbotnikPos->y + tilemap->headlampWsg.h};
                    lookup = divVec2d(lookup, 2);
                    brightness = 5 - j/5;
                    if (brightness < 0){
                        brightness = 0;
                    }
                    //if within bounds of the headlamp texture...
                    if(lookup.x > 0 && lookup.x < 121 && lookup.y > 0 && lookup.y < 106){
                        uint32_t rgbCol = paletteToRGB(tilemap->headlampWsg.px[(lookup.y * tilemap->headlampWsg.w) + lookup.x]);
                        brightness += ((rgbCol >> 16) & 0xFF)/51; // >> 16 & 0xFF gets red channel
                    }
                    if(brightness > 5){
                        brightness = 5;
                    }
                    // printf("red: %d\n", red);
                    
                    switch(sprite_idx & 0b1100){
                        case 0b1100://0 16
                            switch(corner_info & 0b1000){
                                case 0b1000://0
                                    drawWsgSimple(&(*wsgMidgroundArrayPtr)[20*brightness+0], tilePos.x, tilePos.y);
                                    break;
                                default: //0b0000 16
                                    drawWsgSimple(&(*wsgMidgroundArrayPtr)[20*brightness+16], tilePos.x, tilePos.y);
                                    break;
                            }
                            break;
                        case 0b1000://4
                            drawWsgSimple(&(*wsgMidgroundArrayPtr)[20*brightness+4], tilePos.x, tilePos.y);
                            break;
                        case 0b0100://8
                            drawWsgSimple(&(*wsgMidgroundArrayPtr)[20*brightness+8], tilePos.x, tilePos.y);
                            break;
                        default: //0b0000:12
                            drawWsgSimple(&(*wsgMidgroundArrayPtr)[20*brightness+12], tilePos.x, tilePos.y);
                            break;
                    }

                    // Top Right
                    // 1 x11x x1xx
                    // 5 x01x xxxx
                    // 9 x10x xxxx
                    // 13 x00x xxxx 
                    // 17 x11x x0xx
                    lookup.x += 8;
                    brightness = 5 - j/5;
                    if (brightness < 0){
                        brightness = 0;
                    }
                    //if within bounds of the headlamp texture...
                    if(lookup.x > 0 && lookup.x < 121 && lookup.y > 0 && lookup.y < 106){
                        uint32_t rgbCol = paletteToRGB(tilemap->headlampWsg.px[(lookup.y * tilemap->headlampWsg.w) + lookup.x]);
                        brightness += ((rgbCol >> 16) & 255)/51; // red channel
                    }
                    if(brightness > 5){
                        brightness = 5;
                    }
                    switch(sprite_idx & 0b110){
                        case 0b110://1 17
                            switch(corner_info & 0b0100){
                                case 0b0100://1
                                    drawWsgSimple(&(*wsgMidgroundArrayPtr)[20*brightness+1], tilePos.x + HALF_TILE, tilePos.y);
                                    break;
                                default: //0b0000 17
                                    drawWsgSimple(&(*wsgMidgroundArrayPtr)[20*brightness+17], tilePos.x + HALF_TILE, tilePos.y);
                                    break;
                            }
                            break;
                        case 0b010://5
                            drawWsgSimple(&(*wsgMidgroundArrayPtr)[20*brightness+5], tilePos.x + HALF_TILE, tilePos.y);
                            break;
                        case 0b100://9
                            drawWsgSimple(&(*wsgMidgroundArrayPtr)[20*brightness+9], tilePos.x + HALF_TILE, tilePos.y);
                            break;
                        default: //0b0000:13
                            drawWsgSimple(&(*wsgMidgroundArrayPtr)[20*brightness+13], tilePos.x + HALF_TILE, tilePos.y);
                            break;
                    }

                    // Bottom Left
                    // 2 1xx1 xx1x
                    // 6 1xx0 xxxx
                    // 10 0xx1 xxxx
                    // 14 0xx0 xxxx 
                    // 18 1xx1 xx0x
                    lookup.x -= 8;
                    lookup.y += 8;
                    brightness = 5 - j/5;
                    if (brightness < 0){
                        brightness = 0;
                    }
                    //if within bounds of the headlamp texture...
                    if(lookup.x > 0 && lookup.x < 121 && lookup.y > 0 && lookup.y < 106){
                        uint32_t rgbCol = paletteToRGB(tilemap->headlampWsg.px[(lookup.y * tilemap->headlampWsg.w) + lookup.x]);
                        brightness += ((rgbCol >> 16) & 255)/51; // red channel
                    }
                    if(brightness > 5){
                        brightness = 5;
                    }
                    switch(sprite_idx & 0b1001){
                        case 0b1001://2 18
                            switch(corner_info & 0b0010){
                                case 0b0010://2
                                    drawWsgSimple(&(*wsgMidgroundArrayPtr)[20*brightness+2], tilePos.x, tilePos.y + HALF_TILE);
                                    break;
                                default: //0b0000 18
                                    drawWsgSimple(&(*wsgMidgroundArrayPtr)[20*brightness+18], tilePos.x, tilePos.y + HALF_TILE);
                                    break;
                            }
                            break;
                        case 0b1000://6
                            drawWsgSimple(&(*wsgMidgroundArrayPtr)[20*brightness+6], tilePos.x, tilePos.y + HALF_TILE);
                            break;
                        case 0b0001://10
                            drawWsgSimple(&(*wsgMidgroundArrayPtr)[20*brightness+10], tilePos.x, tilePos.y + HALF_TILE);
                            break;
                        default: //0b0000:14
                            drawWsgSimple(&(*wsgMidgroundArrayPtr)[20*brightness+14], tilePos.x, tilePos.y + HALF_TILE);
                            break;
                    }

                    // Bottom Right
                    // 3 xx11 xxx1
                    // 7 xx10 xxxx
                    // 11 xx01 xxxx
                    // 15 xx00 xxxx 
                    // 19 xx11 xxx0
                    lookup.x += 8;
                    brightness = 5 - j/5;
                    if (brightness < 0){
                        brightness = 0;
                    }
                    //if within bounds of the headlamp texture...
                    if(lookup.x > 0 && lookup.x < 121 && lookup.y > 0 && lookup.y < 106){
                        uint32_t rgbCol = paletteToRGB(tilemap->headlampWsg.px[(lookup.y * tilemap->headlampWsg.w) + lookup.x]);
                        brightness += ((rgbCol >> 16) & 255)/51; // red channel
                    }
                    if(brightness > 5){
                        brightness = 5;
                    }
                    switch(sprite_idx & 0b0011){
                        case 0b11://3 19
                            switch(corner_info & 0b1){
                                case 0b1://3
                                    drawWsgSimple(&(*wsgMidgroundArrayPtr)[20*brightness+3], tilePos.x + HALF_TILE, tilePos.y + HALF_TILE);
                                    break;
                                default: //0b0000 19
                                    drawWsgSimple(&(*wsgMidgroundArrayPtr)[20*brightness+19], tilePos.x + HALF_TILE, tilePos.y + HALF_TILE);
                                    break;
                            }
                            break;
                        case 0b10://7
                            drawWsgSimple(&(*wsgMidgroundArrayPtr)[20*brightness+7], tilePos.x + HALF_TILE, tilePos.y + HALF_TILE);
                            break;
                        case 0b01://11
                            drawWsgSimple(&(*wsgMidgroundArrayPtr)[20*brightness+11], tilePos.x + HALF_TILE, tilePos.y + HALF_TILE);
                            break;
                        default: //0b0000:15
                            drawWsgSimple(&(*wsgMidgroundArrayPtr)[20*brightness+15], tilePos.x + HALF_TILE, tilePos.y + HALF_TILE);
                            break;
                    }
                    // char snum[4];
                    // sprintf(snum, "%d", 20*brightness);
                    // drawText(&ibm, c555, snum, tilePos.x, tilePos.y);
                }
                



// Top Left      V
// 00RD 0...   (0,0),  (2,1),  (0,2),  (2,3), #convex corners
// 1100 1..0   (14,0), (12,1), (6,2),  (4,3), #opposite convex corners
// 10RD ....   (1,0),  (1,1),  (1,2),  (1,3), #horizontal light
// 01RD ....   (11,0), (11,1), (11,2), (11,3),#vertical light
// 1110 1...   (13,0), (13,1), (5,2),  (5,3), #horizontal shadow
// 1101 1...   (10,0), (8,1),  (10,2), (8,3), #vertical shadow
// 1111 0...   (19,0), (17,1), (18,2), (16,3),#concave corners
// 1111 10..   (17,0), (16,1), (19,2), (18,3),#left of concave corners
// 1111 1.0.   (18,0), (19,1), (16,2), (17,3),#right of concave corners
// 1111 1110   (16,0), (18,1), (17,2), (19,3) #opposite concave corners


// Top Right             V
// L00D .0...  (0,0),  (2,1),  (0,2),  (2,3), #convex corners
// 0110 .10.   (14,0), (12,1), (6,2),  (4,3), #opposite convex corners
// L01D ....   (1,0),  (1,1),  (1,2),  (1,3), #horizontal light
// L10D ....   (11,0), (11,1), (11,2), (11,3),#vertical light
// 1110 .1..   (13,0), (13,1), (5,2),  (5,3), #horizontal shadow
// 0111 .1..   (10,0), (8,1),  (10,2), (8,3), #vertical shadow
// 1111 .0...  (19,0), (17,1), (18,2), (16,3),#concave corners
// 1111 .1.0   (17,0), (16,1), (19,2), (18,3),#left of concave corners
// 1111 01..   (18,0), (19,1), (16,2), (17,3),#right of concave corners
// 1111 1101   (16,0), (18,1), (17,2), (19,3) #opposite concave corners

// Bottom Left                   V
// 0UR0 ..0.   (0,0),  (2,1),  (0,2),  (2,3), #convex corners
// 0110 .01.   (14,0), (12,1), (6,2),  (4,3), #opposite convex corners
// 1UR0 ....   (1,0),  (1,1),  (1,2),  (1,3), #horizontal light
// 0UR1 ....   (11,0), (11,1), (11,2), (11,3),#vertical light
// 1011 ..1.   (13,0), (13,1), (5,2),  (5,3), #horizontal shadow
// 1101 ..1.   (10,0), (8,1),  (10,2), (8,3), #vertical shadow
// 1111 ..0.   (19,0), (17,1), (18,2), (16,3),#concave corners
// 1111 0.1.   (17,0), (16,1), (19,2), (18,3),#left of concave corners
// 1111 ..10   (18,0), (19,1), (16,2), (17,3),#right of concave corners
// 1111 1011   (16,0), (18,1), (17,2), (19,3) #opposite concave corners


// Bottom Right                          V
// LU00 ...0   (0,0),  (2,1),  (0,2),  (2,3), #convex corners
// 0011 0..1   (14,0), (12,1), (6,2),  (4,3), #opposite convex corners
// LU10 ....   (1,0),  (1,1),  (1,2),  (1,3), #horizontal light
// LU01 ....   (11,0), (11,1), (11,2), (11,3),#vertical light
// 1011 ...1   (13,0), (13,1), (5,2),  (5,3), #horizontal shadow
// 0111 ...1   (10,0), (8,1),  (10,2), (8,3), #vertical shadow
// 1111 ...0   (19,0), (17,1), (18,2), (16,3),#concave corners
// 1111 ..01   (17,0), (16,1), (19,2), (18,3),#left of concave corners
// 1111 .0.1   (18,0), (19,1), (16,2), (17,3),#right of concave corners
// 1111 0111   (16,0), (18,1), (17,2), (19,3) #opposite concave corners






                // Draw foreground tiles
                if(tilemap->fgTiles[i][j] >= 1){
                    wsg_t (*wsgForegroundArrayPtr)[216] = bb_GetForegroundWsgArrForCoord(tilemap, i, j);
                    
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

                    vec_t lookup = {tilePos.x + 8 - garbotnikPos->x + tilemap->headlampWsg.w,
                                    tilePos.y - garbotnikPos->y + tilemap->headlampWsg.h};
                    lookup = divVec2d(lookup, 2);
                    brightness = 0;
                    //if within bounds of the headlamp texture...
                    if(lookup.x > 0 && lookup.x < 121 && lookup.y > 0 && lookup.y < 106){
                        uint32_t rgbCol = paletteToRGB(tilemap->headlampWsg.px[(lookup.y * tilemap->headlampWsg.w) + lookup.x]);
                        brightness += ((rgbCol >> 8) & 0xFF)/51; // >> 8 & 0xFF gets green channel
                    }
                    // printf("red: %d\n", red);
                    
                    switch(sprite_idx & 0b1100){
                        case 0b1100://0 16
                            switch(corner_info & 0b1000){
                                case 0b1000://0
                                    drawWsgSimple(&(*wsgForegroundArrayPtr)[20*brightness+0], tilePos.x, tilePos.y);
                                    break;
                                default: //0b0000 16
                                    drawWsgSimple(&(*wsgForegroundArrayPtr)[20*brightness+16], tilePos.x, tilePos.y);
                                    break;
                            }
                            break;
                        case 0b1000://4
                            drawWsgSimple(&(*wsgForegroundArrayPtr)[20*brightness+4], tilePos.x, tilePos.y);
                            break;
                        case 0b0100://8
                            drawWsgSimple(&(*wsgForegroundArrayPtr)[20*brightness+8], tilePos.x, tilePos.y);
                            break;
                        default: //0b0000:12
                            drawWsgSimple(&(*wsgForegroundArrayPtr)[20*brightness+12], tilePos.x, tilePos.y);
                            break;
                    }

                    // Top Right
                    // 1 x11x x1xx
                    // 5 x01x xxxx
                    // 9 x10x xxxx
                    // 13 x00x xxxx 
                    // 17 x11x x0xx
                    lookup.x += 8;
                    brightness = 5 - j/5;
                    if (brightness < 0){
                        brightness = 0;
                    }
                    //if within bounds of the headlamp texture...
                    if(lookup.x > 0 && lookup.x < 121 && lookup.y > 0 && lookup.y < 106){
                        uint32_t rgbCol = paletteToRGB(tilemap->headlampWsg.px[(lookup.y * tilemap->headlampWsg.w) + lookup.x]);
                        brightness += ((rgbCol >> 16) & 255)/51; // red channel
                    }
                    if(brightness > 5){
                        brightness = 5;
                    }
                    switch(sprite_idx & 0b110){
                        case 0b110://1 17
                            switch(corner_info & 0b0100){
                                case 0b0100://1
                                    drawWsgSimple(&(*wsgMidgroundArrayPtr)[20*brightness+1], tilePos.x + HALF_TILE, tilePos.y);
                                    break;
                                default: //0b0000 17
                                    drawWsgSimple(&(*wsgMidgroundArrayPtr)[20*brightness+17], tilePos.x + HALF_TILE, tilePos.y);
                                    break;
                            }
                            break;
                        case 0b010://5
                            drawWsgSimple(&(*wsgMidgroundArrayPtr)[20*brightness+5], tilePos.x + HALF_TILE, tilePos.y);
                            break;
                        case 0b100://9
                            drawWsgSimple(&(*wsgMidgroundArrayPtr)[20*brightness+9], tilePos.x + HALF_TILE, tilePos.y);
                            break;
                        default: //0b0000:13
                            drawWsgSimple(&(*wsgMidgroundArrayPtr)[20*brightness+13], tilePos.x + HALF_TILE, tilePos.y);
                            break;
                    }

                    // Bottom Left
                    // 2 1xx1 xx1x
                    // 6 1xx0 xxxx
                    // 10 0xx1 xxxx
                    // 14 0xx0 xxxx 
                    // 18 1xx1 xx0x
                    lookup.x -= 8;
                    lookup.y += 8;
                    brightness = 5 - j/5;
                    if (brightness < 0){
                        brightness = 0;
                    }
                    //if within bounds of the headlamp texture...
                    if(lookup.x > 0 && lookup.x < 121 && lookup.y > 0 && lookup.y < 106){
                        uint32_t rgbCol = paletteToRGB(tilemap->headlampWsg.px[(lookup.y * tilemap->headlampWsg.w) + lookup.x]);
                        brightness += ((rgbCol >> 16) & 255)/51; // red channel
                    }
                    if(brightness > 5){
                        brightness = 5;
                    }
                    switch(sprite_idx & 0b1001){
                        case 0b1001://2 18
                            switch(corner_info & 0b0010){
                                case 0b0010://2
                                    drawWsgSimple(&(*wsgMidgroundArrayPtr)[20*brightness+2], tilePos.x, tilePos.y + HALF_TILE);
                                    break;
                                default: //0b0000 18
                                    drawWsgSimple(&(*wsgMidgroundArrayPtr)[20*brightness+18], tilePos.x, tilePos.y + HALF_TILE);
                                    break;
                            }
                            break;
                        case 0b1000://6
                            drawWsgSimple(&(*wsgMidgroundArrayPtr)[20*brightness+6], tilePos.x, tilePos.y + HALF_TILE);
                            break;
                        case 0b0001://10
                            drawWsgSimple(&(*wsgMidgroundArrayPtr)[20*brightness+10], tilePos.x, tilePos.y + HALF_TILE);
                            break;
                        default: //0b0000:14
                            drawWsgSimple(&(*wsgMidgroundArrayPtr)[20*brightness+14], tilePos.x, tilePos.y + HALF_TILE);
                            break;
                    }

                    // Bottom Right
                    // 3 xx11 xxx1
                    // 7 xx10 xxxx
                    // 11 xx01 xxxx
                    // 15 xx00 xxxx 
                    // 19 xx11 xxx0
                    lookup.x += 8;
                    brightness = 5 - j/5;
                    if (brightness < 0){
                        brightness = 0;
                    }
                    //if within bounds of the headlamp texture...
                    if(lookup.x > 0 && lookup.x < 121 && lookup.y > 0 && lookup.y < 106){
                        uint32_t rgbCol = paletteToRGB(tilemap->headlampWsg.px[(lookup.y * tilemap->headlampWsg.w) + lookup.x]);
                        brightness += ((rgbCol >> 16) & 255)/51; // red channel
                    }
                    if(brightness > 5){
                        brightness = 5;
                    }
                    switch(sprite_idx & 0b0011){
                        case 0b11://3 19
                            switch(corner_info & 0b1){
                                case 0b1://3
                                    drawWsgSimple(&(*wsgMidgroundArrayPtr)[20*brightness+3], tilePos.x + HALF_TILE, tilePos.y + HALF_TILE);
                                    break;
                                default: //0b0000 19
                                    drawWsgSimple(&(*wsgMidgroundArrayPtr)[20*brightness+19], tilePos.x + HALF_TILE, tilePos.y + HALF_TILE);
                                    break;
                            }
                            break;
                        case 0b10://7
                            drawWsgSimple(&(*wsgMidgroundArrayPtr)[20*brightness+7], tilePos.x + HALF_TILE, tilePos.y + HALF_TILE);
                            break;
                        case 0b01://11
                            drawWsgSimple(&(*wsgMidgroundArrayPtr)[20*brightness+11], tilePos.x + HALF_TILE, tilePos.y + HALF_TILE);
                            break;
                        default: //0b0000:15
                            drawWsgSimple(&(*wsgMidgroundArrayPtr)[20*brightness+15], tilePos.x + HALF_TILE, tilePos.y + HALF_TILE);
                            break;
                    }
                    // char snum[4];
                    // sprintf(snum, "%d", 20*brightness);
                    // drawText(&ibm, c555, snum, tilePos.x, tilePos.y);
                }
            }
        }
    }
    // freeFont(&ibm);
}

wsg_t (*bb_GetMidgroundWsgArrForCoord(bb_tilemap_t* tilemap, const uint32_t i, const uint32_t j))[120]
{
    if(tilemap->mgTiles[i][j]>4){
        return &tilemap->mid_h_Wsg;
    }
    else if(tilemap->mgTiles[i][j]>1){
        return &tilemap->mid_m_Wsg;
    }
    return &tilemap->mid_s_Wsg;
}

wsg_t (*bb_GetForegroundWsgArrForCoord(bb_tilemap_t* tilemap, const uint32_t i, const uint32_t j))[216]
{
    if(tilemap->fgTiles[i][j]>4){
        return &tilemap->fore_h_Wsg;
    }
    else if(tilemap->fgTiles[i][j]>1){
        return &tilemap->fore_m_Wsg;
    }
    return &tilemap->fore_s_Wsg;
}