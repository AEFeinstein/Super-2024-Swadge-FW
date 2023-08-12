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

    int64_t worldTimer;

    wsg_t floorTiles[20];
    
    lumberjackHero_t* localPlayer;
    lumberjackHero_t* remotePlayer;

} lumberjackVars_t;



#endif