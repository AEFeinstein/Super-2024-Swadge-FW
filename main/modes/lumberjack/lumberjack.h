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
    LUMBERJACK_MODE_NONE,
    LUMBERJACK_MODE_PANIC,
    LUMBERJACK_MODE_ATTACK
} lumberjackGameType_t;

typedef enum
{
    LUMBERJACK_GAMESTATE_TITLE,
    LUMBERJACK_GAMESTATE_PLAYING,
    LUMBERJACK_GAMESTATE_WINNING,
    LUMBERJACK_GAMESTATE_GAMEOVER
} lumberjackGameState_t;

typedef struct
{
    menu_t* menu;
    menuLogbookRenderer_t* menuLogbookRenderer;
    font_t logbook;

    uint8_t selected;
    bool networked;
    bool host;

    // The pass throughs
    p2pInfo p2p;
    connectionEvt_t conStatus;
    lumberjackScreen_t screen;
    lumberjackGameType_t gameMode;

} lumberjack_t;

typedef struct
{
    /* data */
    int x;
    int y;
    int type;
    int index;
    int offset;
    int offset_time;

} lumberjackTile_t;

typedef struct
{
    int x;
    int y;
    int type;
    bool active;

} lumberjackAxeBlock_t;

typedef struct
{
    bool loaded;
    bool hasWon;
    font_t arcade;
    lumberjack_t* lumberjackMain;
    menu_t* menu;
    uint16_t btnState; ///<-- The STOLEN! ;)
    lumberjackGameState_t gameState;

    int yOffset;

    int64_t worldTimer;
    int64_t levelTime;
    int64_t transitionTimer;
    int64_t physicsTimer;
    int liquidAnimationFrame;
    int stageAnimationFrame;
    int currentMapHeight;
    int totalEnemyCount;
    int enemyKillCount;
    int spawnTimer;
    int spawnIndex;
    int spawnSide;

    int score;
    int highscore;

    int enemyAttackQueue;

    int waterLevel;
    int waterTimer;
    int waterSpeed;
    int waterDirection;
    int playerSpawnX;
    int playerSpawnY;

    int enemy1Count;
    int enemy2Count;
    int enemy3Count;
    int enemy4Count;
    int enemy5Count;
    int enemy6Count;
    int enemy7Count;
    int enemy8Count;

    wsg_t floorTiles[20];
    wsg_t animationTiles[20];
    wsg_t minicharacters[3];

    wsg_t title;
    wsg_t subtitle_red;
    wsg_t subtitle_green;
    wsg_t subtitle_white;
    wsg_t gameoverSprite;

    lumberjackTile_t* tile;

    wsg_t greenBlockSprite[7];
    wsg_t redBlockSprite[7];
    wsg_t unknownBlockSprite[7];
    wsg_t unusedBlockSprite[7];

    wsg_t enemySprites[23];
    wsg_t playerSprites[18];

    wsg_t ui[6];

    wsg_t alertSprite;

    lumberjackEntity_t* enemy[64];

    lumberjackAxeBlock_t* axeBlocks[8];
    lumberjackEntity_t* localPlayer;
    lumberjackEntity_t* remotePlayer;

    lumberjackGameType_t gameType;

} lumberjackVars_t;

#endif