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
    int highScore;
    int attackHighScore;
    int panicHighScore;
    bool swadgeGuyUnlocked;
    bool choUnlocked;
}lumberjackUnlock_t;

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
    lumberjackUnlock_t save;

    const char** charactersArray;
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
    int x;
    int y;
    int time;
    bool active;
    int bonusAmount;
} lumberjackBonus_t;

typedef struct
{
    bool levelMusic;
    bool loaded;
    bool hasWon;
    bool gameReady;
    font_t arcade;
    lumberjack_t* lumberjackMain;
    menu_t* menu;
    uint16_t btnState; ///<-- The STOLEN! ;)
    lumberjackGameState_t gameState;

    int yOffset;
    int levelIndex;
    int upgrade;
    int resume;
    int lives;

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
    int localPlayerType;

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

    int enemySpawnTime;
    int enemyReceiveSpawnTime;
    int enemySpawnAmount;

    int comboTime;
    int comboAmount;

    uint8_t attackQueue[8];

    int wakeupSignal;
    int lastResponseSignal;

    wsg_t floorTiles[20];
    wsg_t animationTiles[20];
    wsg_t minicharacters[4];
    wsg_t itemIcons[18];

    lumberjackBonus_t* bonusDisplay[16];
    uint8_t activeBonusIndex;

    wsg_t title;
    wsg_t subtitle_red;
    wsg_t subtitle_green;
    wsg_t subtitle_white;
    wsg_t gameoverSprite;
    wsg_t gamewinSprite;
    wsg_t connectLostSprite;

    lumberjackTile_t* tile;

    wsg_t greenBlockSprite[7];
    wsg_t redBlockSprite[7];
    wsg_t unknownBlockSprite[7];
    wsg_t unusedBlockSprite[7];

    wsg_t enemySprites[37];
    wsg_t playerSprites[18];
    wsg_t bonusSprites[11];

    wsg_t ui[6];

    wsg_t alertSprite;

    lumberjackEntity_t* enemy[32];

    lumberjackAxeBlock_t* axeBlocks[8];
    lumberjackEntity_t* localPlayer;

    //Ghost
    lumberjackGhost_t* ghost;
    int ghostSpawnTime;

    //Item Block
    int itemBlockTime;
    bool itemBlockReady;
    int itemBlockIndex;
    int itemBlockItemAnimation;
    int itemBlockAnimationTime;
    int itemBlockItemFrame;

    uint8_t invincibleColor;
    int invincibleTimer;
    int invincibleFlicker;
    bool invincibleFlickerOn;
    
    lumberjackGameType_t gameType;

    //sounds
    song_t sfx_item_get;
    song_t sfx_item_use;
    song_t sfx_jump;
    song_t sfx_bump;
    song_t sfx_flip;
    song_t sfx_player_death;
    song_t sfx_enemy_death;

    song_t song_theme;
    song_t song_respawn;
    song_t song_gameover;
    song_t song_title;

} lumberjackVars_t;

#endif