#ifndef _LUMBERJACK_MODE_H_
#define _LUMBERJACK_MODE_H_

#include "swadge2024.h"

#include "lumberjackEntity.h"
#include "lumberjackPlayer.h"
 
extern swadgeMode_t lumberjackMode;


typedef struct 
{
    /* data */
    int x;
    int y;
    int collision;
    int type;
    int index;
    int offset;
    int offset_time;

} lumberjackTile_t;

typedef struct
{
    font_t ibm;
    p2pInfo p2p;
    menu_t* menu;
    uint16_t btnState;      ///<-- The STOLEN! ;)

    int yOffset;
    int lives;

    int64_t worldTimer;
    int64_t physicsTimer;
    int liquidAnimationFrame;
    int currentMapHeight;
    int spawnTimer;
    int spawnIndex;
    int spawnSide;

    wsg_t floorTiles[20];
    wsg_t animationTiles[20];

    lumberjackTile_t tile[400];
    uint8_t anim[400];

    wsg_t playerSprites[63];
    wsg_t enemySprites[21];

    wsg_t alertSprite;

    lumberjackEntity_t* enemy[8];
    
    lumberjackEntity_t* localPlayer;
    lumberjackEntity_t* remotePlayer;

} lumberjackVars_t;



#endif