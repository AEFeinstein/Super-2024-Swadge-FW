#ifndef _LUMBERJACK_MODE_H_
#define _LUMBERJACK_MODE_H_

#include "swadge2024.h"

#include "lumberjackEntity.h"
#include "lumberjackPlayer.h"

extern const char* LUM_TAG;
extern swadgeMode_t lumberjackMode;

typedef enum
{
    LUMBERJACK_MENU,
    LUMBERJACK_A,
    LUMBERJACK_B,
} lumberjackScreen_t;

typedef enum
{
    LUMBERJACK_NONE,
    LUMBERJACK_PANIC,
    LUMBERJACK_ATTACK
} lumberjackGameType_t;

typedef struct
{
    menu_t* menu;
    menuLogbookRenderer_t* menuLogbookRenderer;
    font_t ibm;
    font_t logbook;

    // The pass throughs
    p2pInfo p2p;
    connectionEvt_t conStatus;
    lumberjackScreen_t screen;

} lumberjack_t;

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
    bool loaded;
    font_t ibm;
    lumberjack_t* lumberjackMain;
    menu_t* menu;
    uint16_t btnState; ///<-- The STOLEN! ;)

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

    wsg_t enemySprites[21];
    wsg_t playerSprites[54];

    wsg_t alertSprite;

    wsg_t slowload[400];

    lumberjackEntity_t* enemy[8];

    lumberjackEntity_t* localPlayer;
    lumberjackEntity_t* remotePlayer;

    lumberjackGameType_t gameType;

} lumberjackVars_t;

#endif