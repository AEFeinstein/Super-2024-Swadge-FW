#ifndef _LUMBERJACK_MODE_H_
#define _LUMBERJACK_MODE_H_

#include "swadge2024.h"

#include "lumberjackPlayer.h"
 
extern swadgeMode_t lumberjackMode;



typedef struct
{
    font_t ibm;
    p2pInfo p2p;
    menu_t* menu;
    uint16_t btnState;      ///<-- The STOLEN! ;)

    int yOffset;

    int64_t worldTimer;
    int64_t physicsTimer;
    int liquidAnimationFrame;

    wsg_t floorTiles[20];
    wsg_t animationTiles[20];

    int tiles[378];
    int anim[378];
    
    lumberjackHero_t* localPlayer;
    lumberjackHero_t* remotePlayer;

} lumberjackVars_t;



#endif