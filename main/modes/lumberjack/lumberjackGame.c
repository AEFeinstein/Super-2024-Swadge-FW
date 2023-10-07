//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include <esp_log.h>

#include "swadge2024.h"
#include "lumberjack.h"
#include "lumberjackGame.h"
#include "lumberjackEntity.h"
#include "lumberjackPlayer.h"

//redundancy
#include "hdw-spiffs.h"

#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>

//==============================================================================
// Defines
//==============================================================================

#define LUMBERJACK_TILEANIMATION_SPEED  150500

#define LUMBERJACK_SCREEN_X_OFFSET      299
#define LUMBERJACK_SCREEN_X_MIN         0
#define LUMBERJACK_SCREEN_X_MAX         270

#define LUMBERJACK_SCREEN_Y_OFFSET      140
#define LUMBERJACK_SCREEN_Y_MIN         -16
#define LUMBERJACK_SCREEN_Y_MAX         96

#define LUMBERJACK_MAP_WIDTH            18
#define LUMBERJACK_BLOCK_ANIMATION_MAX  7
#define LUMBERJACK_RESPAWN_TIMER        3750
#define LUMBERJACK_SUBMERGE_TIMER       300


static lumberjackTile_t* lumberjackGetTile(int x, int y);
static void lumberjackUpdateEntity(lumberjackEntity_t* entity, int64_t elapsedUs);
static bool lumberjackIsCollisionTile(int index);

bool lumberjackLoadLevel(int index);
void lumberjackOnLocalPlayerDeath(void);

void DrawTitle(void);
void DrawGame(void);

lumberjackVars_t* lumv;
lumberjackTile_t lumberjackCollisionCheckTiles[32] = {};

void lumberjackStartGameMode(lumberjack_t* main, uint8_t characterIndex)
{
    lumv                 = calloc(1, sizeof(lumberjackVars_t));
    lumv->lumberjackMain = main;

    loadFont("eightbit_atari_grube2.font", &lumv->arcade, false);

    bzrStop(); // Stop the buzzer?

    for (int i = 0; i < ARRAY_SIZE(lumv->axeBlocks); i++)
    {
        lumv->axeBlocks[i] = calloc(1, sizeof(lumberjackAxeBlock_t));
        lumv->axeBlocks[i]->x = -90;
        lumv->axeBlocks[i]->y = -1000;
        lumv->axeBlocks[i]->active = false;
    }

    lumv->gameOver             = false;
    lumv->worldTimer           = 0;
    lumv->liquidAnimationFrame = 0;
    lumv->stageAnimationFrame  = 0;
    lumv->loaded               = false;
    lumv->gameType             = main->gameMode;
    lumv->onTitle              = true;

    ESP_LOGI(LUM_TAG, "Load Title");
    loadWsg("lumbers_title.wsg", &lumv->title, true);

    if (main->screen == LUMBERJACK_A)
    {
        loadWsg("lumbers_title_panic_red.wsg", &lumv->subtitle_red, true);
        loadWsg("lumbers_title_panic_green.wsg", &lumv->subtitle_green, true);
        loadWsg("lumbers_title_panic_white.wsg", &lumv->subtitle_white, true);
    }

    if (main->screen == LUMBERJACK_B)
    {
    
        loadWsg("lumbers_title_attack_red.wsg", &lumv->subtitle_red, true);
        loadWsg("lumbers_title_attack_green.wsg", &lumv->subtitle_green, true);    
        loadWsg("lumbers_title_attack_white.wsg", &lumv->subtitle_white, true);    
    }

    loadWsg("lumbers_game_over.wsg", &lumv->gameoverSprite, true);

    ESP_LOGI(LUM_TAG, "Loading floor Tiles");
    loadWsg("bottom_floor1.wsg", &lumv->floorTiles[0], true);
    loadWsg("bottom_floor2.wsg", &lumv->floorTiles[1], true);
    loadWsg("bottom_floor3.wsg", &lumv->floorTiles[2], true);
    loadWsg("bottom_floor4.wsg", &lumv->floorTiles[3], true);
    loadWsg("bottom_floor5.wsg", &lumv->floorTiles[4], true);
    loadWsg("bottom_floor6.wsg", &lumv->floorTiles[5], true);
    loadWsg("bottom_floor7.wsg", &lumv->floorTiles[6], true);
    loadWsg("bottom_floor8.wsg", &lumv->floorTiles[7], true);
    loadWsg("bottom_floor9.wsg", &lumv->floorTiles[8], true);
    loadWsg("bottom_floor10.wsg", &lumv->floorTiles[9], true);
    ESP_LOGI(LUM_TAG, "Loading Animation Tiles");

    loadWsg("water_floor1.wsg", &lumv->animationTiles[0], true);
    loadWsg("water_floor2.wsg", &lumv->animationTiles[1], true);
    loadWsg("water_floor3.wsg", &lumv->animationTiles[2], true);
    loadWsg("water_floor4.wsg", &lumv->animationTiles[3], true);
    loadWsg("water_floor0.wsg", &lumv->animationTiles[4], true);
    loadWsg("water_floor0.wsg", &lumv->animationTiles[5], true);
    loadWsg("water_floor0.wsg", &lumv->animationTiles[6], true);
    loadWsg("water_floor0.wsg", &lumv->animationTiles[7], true);
    loadWsg("water_floor_b1.wsg", &lumv->animationTiles[8], true);
    loadWsg("water_floor_b2.wsg", &lumv->animationTiles[9], true);
    loadWsg("water_floor_b3.wsg", &lumv->animationTiles[10], true);
    loadWsg("water_floor_b4.wsg", &lumv->animationTiles[11], true);

    ESP_LOGI(LUM_TAG, "Loading Characters");

    ESP_LOGI(LUM_TAG, "*Loading character icons");
    loadWsg("lumbers_red_lives.wsg", &lumv->minicharacters[0], true);
    loadWsg("lumbers_green_lives.wsg", &lumv->minicharacters[1], true);
    loadWsg("secret_swadgeland_lives.wsg", &lumv->minicharacters[2], true);

    ESP_LOGI(LUM_TAG, "*Loading character sprites");
    if (characterIndex == 0)
    {
        loadWsg("lumbers_red_1.wsg", &lumv->playerSprites[0], true);
        loadWsg("lumbers_red_2.wsg", &lumv->playerSprites[1], true);
        loadWsg("lumbers_red_3.wsg", &lumv->playerSprites[2], true);
        loadWsg("lumbers_red_4.wsg", &lumv->playerSprites[3], true); 
        loadWsg("lumbers_red_5.wsg", &lumv->playerSprites[4], true);
        loadWsg("lumbers_red_6.wsg", &lumv->playerSprites[5], true);
        loadWsg("lumbers_red_7.wsg", &lumv->playerSprites[6], true);
        loadWsg("lumbers_red_8.wsg", &lumv->playerSprites[7], true);
        loadWsg("lumbers_red_9.wsg", &lumv->playerSprites[8], true);
        loadWsg("lumbers_red_10.wsg", &lumv->playerSprites[9], true);
        loadWsg("lumbers_red_11.wsg", &lumv->playerSprites[10], true);
        loadWsg("lumbers_red_12.wsg", &lumv->playerSprites[11], true);
        loadWsg("lumbers_red_13.wsg", &lumv->playerSprites[12], true);
        loadWsg("lumbers_red_14.wsg", &lumv->playerSprites[13], true);
        loadWsg("lumbers_red_15.wsg", &lumv->playerSprites[14], true);
        loadWsg("lumbers_red_16.wsg", &lumv->playerSprites[15], true);
        loadWsg("lumbers_red_17.wsg", &lumv->playerSprites[16], true);
        loadWsg("lumbers_red_22.wsg", &lumv->playerSprites[17], true);
    }
    else if (characterIndex == 1)
    {
        loadWsg("lumbers_green_1.wsg", &lumv->playerSprites[0], true);
        loadWsg("lumbers_green_2.wsg", &lumv->playerSprites[1], true);
        loadWsg("lumbers_green_3.wsg", &lumv->playerSprites[2], true);
        loadWsg("lumbers_green_4.wsg", &lumv->playerSprites[3], true);
        loadWsg("lumbers_green_5.wsg", &lumv->playerSprites[4], true);
        loadWsg("lumbers_green_6.wsg", &lumv->playerSprites[5], true);
        loadWsg("lumbers_green_7.wsg", &lumv->playerSprites[6], true);
        loadWsg("lumbers_green_8.wsg", &lumv->playerSprites[7], true);
        loadWsg("lumbers_green_9.wsg", &lumv->playerSprites[8], true);
        loadWsg("lumbers_green_10.wsg", &lumv->playerSprites[9], true);
        loadWsg("lumbers_green_11.wsg", &lumv->playerSprites[10], true);
        loadWsg("lumbers_green_12.wsg", &lumv->playerSprites[11], true);
        loadWsg("lumbers_green_13.wsg", &lumv->playerSprites[12], true);
        loadWsg("lumbers_green_14.wsg", &lumv->playerSprites[13], true);
        loadWsg("lumbers_green_15.wsg", &lumv->playerSprites[14], true);
        loadWsg("lumbers_green_16.wsg", &lumv->playerSprites[15], true);
        loadWsg("lumbers_green_17.wsg", &lumv->playerSprites[16], true);
        loadWsg("lumbers_green_22.wsg", &lumv->playerSprites[17], true);
    }
    else if (characterIndex == 2)
    {

        loadWsg("secret_swadgeland_1.wsg", &lumv->playerSprites[0], true);
        loadWsg("secret_swadgeland_2.wsg", &lumv->playerSprites[1], true);
        loadWsg("secret_swadgeland_3.wsg", &lumv->playerSprites[2], true);
        loadWsg("secret_swadgeland_4.wsg", &lumv->playerSprites[3], true);
        loadWsg("secret_swadgeland_5.wsg", &lumv->playerSprites[4], true);
        loadWsg("secret_swadgeland_6.wsg", &lumv->playerSprites[5], true);
        loadWsg("secret_swadgeland_7.wsg", &lumv->playerSprites[6], true);
        loadWsg("secret_swadgeland_8.wsg", &lumv->playerSprites[7], true);
        loadWsg("secret_swadgeland_9.wsg", &lumv->playerSprites[8], true);
        loadWsg("secret_swadgeland_10.wsg", &lumv->playerSprites[9], true);
        loadWsg("secret_swadgeland_11.wsg", &lumv->playerSprites[10], true);
        loadWsg("secret_swadgeland_12.wsg", &lumv->playerSprites[11], true);
        loadWsg("secret_swadgeland_13.wsg", &lumv->playerSprites[12], true);
        loadWsg("secret_swadgeland_14.wsg", &lumv->playerSprites[13], true);
        loadWsg("secret_swadgeland_15.wsg", &lumv->playerSprites[14], true);
        loadWsg("secret_swadgeland_16.wsg", &lumv->playerSprites[15], true);
        loadWsg("secret_swadgeland_17.wsg", &lumv->playerSprites[16], true);
        loadWsg("secret_swadgeland_22.wsg", &lumv->playerSprites[17], true);
    } 

    ESP_LOGI(LUM_TAG, "Loading Enemies");
    loadWsg("lumbers_enemy_a1.wsg", &lumv->enemySprites[0], true);
    loadWsg("lumbers_enemy_a2.wsg", &lumv->enemySprites[1], true);
    loadWsg("lumbers_enemy_a3.wsg", &lumv->enemySprites[2], true);
    loadWsg("lumbers_enemy_a4.wsg", &lumv->enemySprites[3], true);
    loadWsg("lumbers_enemy_a5.wsg", &lumv->enemySprites[4], true);
    loadWsg("lumbers_enemy_a6.wsg", &lumv->enemySprites[5], true);
    loadWsg("lumbers_enemy_a7.wsg", &lumv->enemySprites[6], true);
    loadWsg("lumbers_enemy_b1.wsg", &lumv->enemySprites[7], true);
    loadWsg("lumbers_enemy_b2.wsg", &lumv->enemySprites[8], true);
    loadWsg("lumbers_enemy_b3.wsg", &lumv->enemySprites[9], true);
    loadWsg("lumbers_enemy_b4.wsg", &lumv->enemySprites[10], true);
    loadWsg("lumbers_enemy_b5.wsg", &lumv->enemySprites[11], true);
    loadWsg("lumbers_enemy_b6.wsg", &lumv->enemySprites[12], true);
    loadWsg("lumbers_enemy_b7.wsg", &lumv->enemySprites[13], true);
    loadWsg("lumbers_enemy_c1.wsg", &lumv->enemySprites[14], true);
    loadWsg("lumbers_enemy_c2.wsg", &lumv->enemySprites[15], true);
    loadWsg("lumbers_enemy_c3.wsg", &lumv->enemySprites[16], true);
    loadWsg("lumbers_enemy_c4.wsg", &lumv->enemySprites[17], true);
    loadWsg("lumbers_enemy_c5.wsg", &lumv->enemySprites[18], true);
    loadWsg("lumbers_enemy_c6.wsg", &lumv->enemySprites[19], true);
    loadWsg("lumbers_enemy_c7.wsg", &lumv->enemySprites[20], true);

    loadWsg("lumbers_green_ax_block1.wsg", &lumv->greenBlockSprite[0], true);
    loadWsg("lumbers_green_ax_block2.wsg", &lumv->greenBlockSprite[1], true);
    loadWsg("lumbers_green_ax_block3.wsg", &lumv->greenBlockSprite[2], true);
    loadWsg("lumbers_green_ax_block4.wsg", &lumv->greenBlockSprite[3], true);
    loadWsg("lumbers_green_ax_block5.wsg", &lumv->greenBlockSprite[4], true);
    loadWsg("lumbers_green_ax_block6.wsg", &lumv->greenBlockSprite[5], true);
    loadWsg("lumbers_green_ax_block7.wsg", &lumv->greenBlockSprite[6], true);

    loadWsg("lumbers_red_ax_block1.wsg", &lumv->redBlockSprite[0], true);
    loadWsg("lumbers_red_ax_block2.wsg", &lumv->redBlockSprite[1], true);
    loadWsg("lumbers_red_ax_block3.wsg", &lumv->redBlockSprite[2], true);
    loadWsg("lumbers_red_ax_block4.wsg", &lumv->redBlockSprite[3], true);
    loadWsg("lumbers_red_ax_block5.wsg", &lumv->redBlockSprite[4], true);
    loadWsg("lumbers_red_ax_block6.wsg", &lumv->redBlockSprite[5], true);
    loadWsg("lumbers_red_ax_block7.wsg", &lumv->redBlockSprite[6], true);

    loadWsg("lumbers_normal_ax_block1.wsg", &lumv->unusedBlockSprite[0], true);
    loadWsg("lumbers_normal_ax_block2.wsg", &lumv->unusedBlockSprite[1], true);
    loadWsg("lumbers_normal_ax_block3.wsg", &lumv->unusedBlockSprite[2], true);
    loadWsg("lumbers_normal_ax_block4.wsg", &lumv->unusedBlockSprite[3], true);
    loadWsg("lumbers_normal_ax_block5.wsg", &lumv->unusedBlockSprite[4], true);
    loadWsg("lumbers_normal_ax_block6.wsg", &lumv->unusedBlockSprite[5], true);
    loadWsg("lumbers_normal_ax_block7.wsg", &lumv->unusedBlockSprite[6], true);

    ESP_LOGI(LUM_TAG, "AXE %d", lumv->axeBlocks[0]->active);

    loadWsg("lumbers_alert.wsg", &lumv->alertSprite, true);

    if (lumv->gameType == LUMBERJACK_MODE_ATTACK)
    {
        lumberjackSetupLevel(characterIndex);
    }
    else if (lumv->gameType == LUMBERJACK_MODE_PANIC)
    {
        lumberjackSetupLevel(characterIndex);
    }

    ESP_LOGI(LUM_TAG, "width %d height %d",TFT_WIDTH, TFT_HEIGHT);
}

bool lumberjackLoadLevel(int index)
{
    char* fname = "lumberjacks_panic_1.bin";

    size_t ms;
    uint8_t *buffer = spiffsReadFile(fname, &ms, false);

    ESP_LOGI(LUM_TAG, "HEIGHT = %d", (int)buffer[0]);
    // Buffer 0 = map height
    lumv->tile = (lumberjackTile_t*)malloc((int)buffer[0] * LUMBERJACK_MAP_WIDTH * sizeof(lumberjackTile_t));
    lumv->currentMapHeight = (int)buffer[0];

    lumv->enemy1Count = (int) buffer[1];
    lumv->enemy2Count = (int) buffer[2];
    lumv->enemy3Count = (int) buffer[3];
    lumv->enemy4Count = (int) buffer[4];
    lumv->enemy5Count = (int) buffer[5];
    lumv->enemy6Count = (int) buffer[6];
    lumv->enemy7Count = (int) buffer[7];
    lumv->enemy8Count = (int) buffer[8];


    lumv->waterSpeed       = (int)buffer[11];
    lumv->waterTimer       = (int)buffer[11];

    for (int i = 0; i < lumv->currentMapHeight * LUMBERJACK_MAP_WIDTH; i++)
    {
        lumv->tile[i].index       = -1;
        lumv->tile[i].x           = i % LUMBERJACK_MAP_WIDTH;
        lumv->tile[i].y           = i / LUMBERJACK_MAP_WIDTH;
        lumv->tile[i].type        = (int)buffer[i+12];
        lumv->tile[i].offset      = 0;
        lumv->tile[i].offset_time = 0;
        
        if (i < 10)
        {
            ESP_LOGI(LUM_TAG, "Tile = %d", lumv->tile[i].x);
        }
        
    }
    
    int offset = (lumv->currentMapHeight * LUMBERJACK_MAP_WIDTH) + 12;
    ESP_LOGI (LUM_TAG, "%d",((int)buffer[offset + 1] + ((int)buffer[offset + 2] << 8)));
    //lumv->tile = &level;
    lumv->playerSpawnX = (int)buffer[offset];
    lumv->playerSpawnY = (int)buffer[offset + 1] + ((int)buffer[offset + 2] << 8);
    

    return true;
}

void lumberjackSetupLevel(int characterIndex)
{
    
    ESP_LOGI(LUM_TAG, "LOADING LEVEL");
    bool levelLoaded = lumberjackLoadLevel(0);
        
    // This all to be loaded externally
    lumv->yOffset          = 0;
    lumv->spawnIndex       = 0;
    lumv->spawnTimer       = 2750;
    lumv->transitionTimer  = 0;
    lumv->spawnSide        = 0;
    lumv->waterLevel       = lumv->currentMapHeight * 16;
    lumv->waterDirection   = -1; // This needs to be taken in to account with the intro timer

    lumv->localPlayer  = calloc(1, sizeof(lumberjackEntity_t));
    lumv->remotePlayer = calloc(1, sizeof(lumberjackEntity_t));
    lumberjackSetupPlayer(lumv->localPlayer, characterIndex);
    lumberjackSpawnPlayer(lumv->localPlayer, lumv->playerSpawnX, lumv->playerSpawnY, 0);

    strcpy(lumv->localPlayer->name, " Dennis"); // If you see this... this name means nothing

    int offset = 0;
    //Load enemies
    for (int eSpawnIndex = 0; eSpawnIndex < lumv->enemy1Count; eSpawnIndex++)
    {
        lumv->enemy[eSpawnIndex] = calloc(1, sizeof(lumberjackEntity_t));
        lumberjackSetupEnemy(lumv->enemy[eSpawnIndex], 0);
    } 

    offset += lumv->enemy1Count;
    for (int eSpawnIndex = 0; eSpawnIndex < lumv->enemy2Count; eSpawnIndex++)
    {
        lumv->enemy[offset + eSpawnIndex] = calloc(1, sizeof(lumberjackEntity_t));
        lumberjackSetupEnemy(lumv->enemy[offset + eSpawnIndex], 0);
    } 

    offset += lumv->enemy2Count;
    for (int eSpawnIndex = 0; eSpawnIndex < lumv->enemy3Count; eSpawnIndex++)
    {
        lumv->enemy[offset + eSpawnIndex] = calloc(1, sizeof(lumberjackEntity_t));
        lumberjackSetupEnemy(lumv->enemy[offset + eSpawnIndex], 0);
    } 

    offset += lumv->enemy3Count;
    for (int eSpawnIndex = 0; eSpawnIndex < lumv->enemy4Count; eSpawnIndex++)
    {
        lumv->enemy[offset + eSpawnIndex] = calloc(1, sizeof(lumberjackEntity_t));
        lumberjackSetupEnemy(lumv->enemy[offset + eSpawnIndex], 0);
    } 

    offset += lumv->enemy4Count;
    for (int eSpawnIndex = 0; eSpawnIndex < lumv->enemy5Count; eSpawnIndex++)
    {
        lumv->enemy[offset + eSpawnIndex] = calloc(1, sizeof(lumberjackEntity_t));
        lumberjackSetupEnemy(lumv->enemy[offset + eSpawnIndex], 0);
    } 

    offset += lumv->enemy5Count;
    for (int eSpawnIndex = 0; eSpawnIndex < lumv->enemy6Count; eSpawnIndex++)
    {
        lumv->enemy[offset + eSpawnIndex] = calloc(1, sizeof(lumberjackEntity_t));
        lumberjackSetupEnemy(lumv->enemy[offset + eSpawnIndex], 0);
    } 

    offset += lumv->enemy6Count;
    for (int eSpawnIndex = 0; eSpawnIndex < lumv->enemy7Count; eSpawnIndex++)
    {
        lumv->enemy[offset + eSpawnIndex] = calloc(1, sizeof(lumberjackEntity_t));
        lumberjackSetupEnemy(lumv->enemy[offset + eSpawnIndex], 0);
    } 

    offset += lumv->enemy7Count;
    for (int eSpawnIndex = 0; eSpawnIndex < lumv->enemy8Count; eSpawnIndex++)
    {
        lumv->enemy[offset + eSpawnIndex] = calloc(1, sizeof(lumberjackEntity_t));
        lumberjackSetupEnemy(lumv->enemy[offset + eSpawnIndex], 0);
    } 

    ESP_LOGI(LUM_TAG, "LOADED");
}

/**
 * @brief TODO use this somewhere
 */
void restartLevel(void)
{
    lumv->localPlayer->lives--;
    lumberjackRespawn(lumv->localPlayer);
}

void lumberjackTitleLoop(int64_t elapsedUs)
{

    // Update State
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        lumv->btnState = evt.state;
    }

    if (lumv->btnState & PB_A)
    {
        lumv->onTitle = false;
    }

    //Update Animation
    lumv->worldTimer += elapsedUs;

    if (lumv->worldTimer > LUMBERJACK_TILEANIMATION_SPEED)
    {
        lumv->worldTimer -= LUMBERJACK_TILEANIMATION_SPEED;
        lumv->liquidAnimationFrame++;
        lumv->liquidAnimationFrame %= 4;
        lumv->stageAnimationFrame++;
    }



    DrawTitle();
}

void lumberjackGameLoop(int64_t elapsedUs)
{

    if (lumv->onTitle == true)
    {
        lumberjackTitleLoop(elapsedUs);
        return;
    }

    baseMode(elapsedUs);

    if (lumv->gameOver)
    {
        lumv->transitionTimer -= elapsedUs/10000;

        if (lumv->transitionTimer <= 0)
        {
            lumv->transitionTimer = 0;       
            switchToSwadgeMode(&lumberjackMode);                 
        }
    }

    //if panic mode do water
    if (lumv->gameOver == false && lumv->gameType == LUMBERJACK_MODE_PANIC)
    {
        lumv->waterTimer -= elapsedUs/10000;
        if (lumv->waterTimer < 0)
        {
            lumv->waterTimer += lumv->waterSpeed;
            lumv->waterLevel += lumv->waterDirection;

            if (lumv->waterLevel > lumv->currentMapHeight * 16)
            {
                lumv->waterLevel     = lumv->currentMapHeight * 16;
                lumv->waterTimer     = lumv->waterSpeed * 10;
                lumv->waterDirection = -1;
            }

            if (lumv->waterLevel < 0)
            {
                lumv->waterLevel = 0;
            }
            
        }

        // If networked
        if (lumv->lumberjackMain->networked && lumv->lumberjackMain->conStatus == CON_ESTABLISHED)
            lumberjackUpdateLocation(lumv->localPlayer->x, lumv->localPlayer->y, lumv->localPlayer->drawFrame);
    }
    DrawGame();
}

void lumberjackUpdateRemote(int remoteX, int remoteY, int remoteFrame)
{
    lumv->remotePlayer->x         = remoteX;
    lumv->remotePlayer->y         = remoteY;
    lumv->remotePlayer->drawFrame = remoteFrame;
    lumv->remotePlayer->active    = true;
}

void baseMode(int64_t elapsedUs)
{
    // Ignore the first frame because everything was loading
    // Here we might want to do something like say "On first frame loaded do stuff"
    if (lumv->loaded == false)
    {
        lumv->loaded = true;

        ESP_LOGI(LUM_TAG, "Load Time %ld", (long)elapsedUs);

        // If networked, send "Loaded complete!" ?

        return;
    }

    // Update State
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        lumv->btnState = evt.state;
    }

    // return;

    // Check Controls
    if (lumv->localPlayer->state != LUMBERJACK_DEAD && lumv->localPlayer->state != LUMBERJACK_UNSPAWNED)
    {
        bool attackThisFrame = lumv->localPlayer->attackPressed;
        lumberjackDoControls();

        if (!attackThisFrame && lumv->localPlayer->attackPressed)
        {
            ESP_LOGI(LUM_TAG, "Attack this frame!");
            lumberjackSendAttack(0);
        }
    }

    if (!lumv->gameOver)
    {
        for (int enemyIndex = 0; enemyIndex < ARRAY_SIZE(lumv->enemy); enemyIndex++)
        {
            if (lumv->enemy[enemyIndex] == NULL)
                continue;

            lumberjackDoEnemyControls(lumv->enemy[enemyIndex]);
        }
    }

    // Clear cruft
    lumberjackUpdate(elapsedUs);

    // Check spawn
    lumberjackSpawnCheck(elapsedUs);

    for (int colTileIndex = 0; colTileIndex < ARRAY_SIZE(lumberjackCollisionCheckTiles); colTileIndex++)
    {
        lumberjackCollisionCheckTiles[colTileIndex].type        = -1;
        lumberjackCollisionCheckTiles[colTileIndex].x           = -1;
        lumberjackCollisionCheckTiles[colTileIndex].y           = -1;
        lumberjackCollisionCheckTiles[colTileIndex].index       = -1;
        lumberjackCollisionCheckTiles[colTileIndex].offset      = 0;
        lumberjackCollisionCheckTiles[colTileIndex].offset_time = 0;
    }

    if (lumv->localPlayer->onGround && !lumv->localPlayer->jumpReady)
    {
        if (lumv->localPlayer->jumpPressed == false)
        {
            lumv->localPlayer->jumpReady = true;
        }
    }

    // Check physics
    lumberjackUpdatePlayerCollision(lumv->localPlayer);

    if (!lumv->gameOver)
    {
        // Enemy
        for (int eIdx = 0; eIdx < ARRAY_SIZE(lumv->enemy); eIdx++)
        {
            lumberjackEntity_t* enemy = lumv->enemy[eIdx];
            if (enemy == NULL || enemy->ready == true)
                continue;

            enemy->showAlert = false;
            if (enemy->state == LUMBERJACK_BUMPED_IDLE)
            {
                enemy->respawn -= elapsedUs / 10000;

                enemy->showAlert = enemy->respawn < 200;
                if (enemy->showAlert)
                {
                    enemy->upgrading = true;
                }

                if (enemy->respawn <= 0)
                {
                    // Hopefully the enemy isn't dead off screen.
                    enemy->state = LUMBERJACK_RUN;

                    enemy->direction = (enemy->flipped) ? -1 : 1;

                    enemy->showAlert = false;
                    enemy->vy        = -10;

                    lumberjackUpdateEnemy(enemy, enemy->type + 1);
                }
            }

            lumberjackUpdateEntity(enemy, elapsedUs);

            for (int oeIdx = 0; oeIdx < ARRAY_SIZE(lumv->enemy); oeIdx++)
            {
                if (lumv->enemy[oeIdx] == NULL)
                    continue;

                lumberjackUpdateEnemyCollision(lumv->enemy[oeIdx]);
            }
        }
    }

    // Player
    if (lumv->localPlayer->ready)
    {
        lumv->localPlayer->respawn -= elapsedUs / 10000;

        if (lumv->localPlayer->respawn <= 0 && lumv->localPlayer->lives > 0)
        {
            ESP_LOGI(LUM_TAG, "RESPAWN PLAYER!");
            // Respawn player
            lumv->localPlayer->respawn = 0;
            lumv->localPlayer->lives--;
            lumberjackRespawn(lumv->localPlayer);
        }
    }
    else if (lumv->localPlayer->state != LUMBERJACK_OFFSCREEN && lumv->localPlayer->state != LUMBERJACK_VICTORY)
    {
        lumberjackUpdateEntity(lumv->localPlayer, elapsedUs);

        //
        for (int enemyIndex = 0; enemyIndex < ARRAY_SIZE(lumv->enemy); enemyIndex++)
        {
            lumberjackEntity_t* enemy = lumv->enemy[enemyIndex];

            if (enemy == NULL || lumv->localPlayer->state == LUMBERJACK_DEAD || lumv->localPlayer->state == LUMBERJACK_INVINCIBLE)
                continue;

            if (enemy->state != LUMBERJACK_DEAD && enemy->state != LUMBERJACK_OFFSCREEN)
            {
                // DO AABB checking
                if (checkCollision(lumv->localPlayer, enemy))
                {
                    if (enemy->state == LUMBERJACK_BUMPED || enemy->state == LUMBERJACK_BUMPED_IDLE)
                    {
                        enemy->state = LUMBERJACK_DEAD; // Enemy Death
                        enemy->vy    = -30;
                        if (lumv->localPlayer->vx != 0)
                        {
                            enemy->direction = abs(lumv->localPlayer->vx) / lumv->localPlayer->vx;
                        }
                        else
                        {
                            enemy->direction = 0;
                        }
                        enemy->vx     = enemy->direction * 10;
                        enemy->active = false;
                    }
                    else
                    {
                        // Kill player
                        // ESP_LOGI(LUM_TAG, "KILL PLAYER");
                        lumberjackOnLocalPlayerDeath();
                    }
                }
            }
        }
    }

    lumv->worldTimer += elapsedUs;

    if (lumv->worldTimer > LUMBERJACK_TILEANIMATION_SPEED)
    {
        lumv->worldTimer -= LUMBERJACK_TILEANIMATION_SPEED;
        lumv->liquidAnimationFrame++;
        lumv->liquidAnimationFrame %= 4;
        lumv->stageAnimationFrame++;
    }

    //if Panic mode... check to see if player's head is under water.
    if (lumv->localPlayer->y > lumv->waterLevel && lumv->localPlayer->state != LUMBERJACK_DEAD && lumv->localPlayer->state != LUMBERJACK_OFFSCREEN)
    {
        //ESP_LOGW(LUM_TAG, "UNDER WATER! %d", lumv->localPlayer->submergedTimer);
        lumv->localPlayer->submergedTimer -= elapsedUs / 10000;

        if (lumv->localPlayer->submergedTimer <= 0)
        {
            lumv->localPlayer->submergedTimer = 0;
            lumberjackOnLocalPlayerDeath();
        }
    }
    else
    {
        lumv->localPlayer->submergedTimer = LUMBERJACK_SUBMERGE_TIMER;
    }

    // Update animation
    if (!lumv->gameOver)
    {        
        // Enemy Animation
        for (int enemyIndex = 0; enemyIndex < ARRAY_SIZE(lumv->enemy); enemyIndex++)
        {
            if (lumv->enemy[enemyIndex] == NULL)
                continue;

            lumv->enemy[enemyIndex]->timerFrameUpdate += elapsedUs;
            if (lumv->enemy[enemyIndex]->timerFrameUpdate > lumv->enemy[enemyIndex]->animationSpeed)
            {
                lumv->enemy[enemyIndex]->currentFrame++;
                lumv->enemy[enemyIndex]->timerFrameUpdate = 0; //;
            }
        }
    }

    // Player
    lumv->localPlayer->timerFrameUpdate += elapsedUs;
    if (lumv->localPlayer->timerFrameUpdate > lumv->localPlayer->animationSpeed)
    {
        lumv->localPlayer->currentFrame++;
        lumv->localPlayer->timerFrameUpdate = 0; //;
    }

    lumv->yOffset = lumv->localPlayer->y - LUMBERJACK_SCREEN_Y_OFFSET;
    if (lumv->yOffset < LUMBERJACK_SCREEN_Y_MIN)
        lumv->yOffset = LUMBERJACK_SCREEN_Y_MIN;
    if (lumv->yOffset > LUMBERJACK_SCREEN_Y_MAX)
        lumv->yOffset = LUMBERJACK_SCREEN_Y_MAX;
}

void lumberjackOnLocalPlayerDeath(void)
{
    lumv->localPlayer->state     = LUMBERJACK_DEAD;
    lumv->localPlayer->vy        = -20;
    lumv->localPlayer->active    = false;
    lumv->localPlayer->jumping   = false;
    lumv->localPlayer->jumpTimer = 0;

    if (lumv->localPlayer->lives <= 0)
    {
        ESP_LOGI(LUM_TAG, "Game over!");
        lumv->gameOver = true;
        lumv->transitionTimer = 400;
    }
}

void DrawTitle(void)
{
    drawWsgSimple(&lumv->title, (TFT_WIDTH / 2) - 51, (TFT_HEIGHT / 2) - 48);
    
    if (lumv->localPlayer->type == 0)
    {
        drawWsgSimple(&lumv->subtitle_red, (TFT_WIDTH/2)- 36, (TFT_HEIGHT/2) -9);
    }
    else if (lumv->localPlayer->type == 1)
    {
        drawWsgSimple(&lumv->subtitle_green, (TFT_WIDTH/2)- 36, (TFT_HEIGHT/2) -9);
    }
    
    for (int i = 0; i < LUMBERJACK_MAP_WIDTH; i++)
    {
        drawWsgSimple(&lumv->floorTiles[1], i * 16, 208);
        drawWsgSimple(&lumv->floorTiles[7], i * 16, 224);
    }

    drawWsgSimple(&lumv->unusedBlockSprite[lumv->stageAnimationFrame % LUMBERJACK_BLOCK_ANIMATION_MAX], 8.5 * 16, 208 - 64);
}
 
void DrawGame(void)
{
    // Draw section
    // Redraw bottom
    lumberjackTileMap();

    // Draw enemies

    for (int enemyIndex = 0; enemyIndex < ARRAY_SIZE(lumv->enemy); enemyIndex++)
    {
        if (lumv->enemy[enemyIndex] == NULL || lumv->enemy[enemyIndex]->ready)
            continue;
        lumberjackEntity_t* enemy = lumv->enemy[enemyIndex];

        int eFrame = lumberjackGetEnemyAnimation(enemy);

        drawWsg(&lumv->enemySprites[enemy->spriteOffset + eFrame], enemy->x, enemy->y - lumv->yOffset, enemy->flipped,
                false, 0);

        if (enemy->x > LUMBERJACK_SCREEN_X_MAX)
        {
            drawWsg(&lumv->enemySprites[enemy->spriteOffset + eFrame], enemy->x - LUMBERJACK_SCREEN_X_OFFSET,
                    enemy->y - lumv->yOffset, enemy->flipped, false, 0);
        }

        if (enemy->showAlert)
        {
            // Fix the magic number :(
            drawWsg(&lumv->alertSprite, enemy->x + 6, enemy->y - 26 - lumv->yOffset, enemy->flipped, false, 0);
        }
    }

    int currentFrame = lumberjackGetPlayerAnimation(lumv->localPlayer);

    if (lumv->localPlayer->state == LUMBERJACK_DEAD)
    {
        // ESP_LOGI(LUM_TAG, "DEAD %d", currentFrame);
        
        // If panic mode 
        lumv->waterDirection = 3;
        lumv->waterTimer = lumv->waterSpeed;
        lumv->waterLevel += lumv->waterDirection;

    }

    if (lumv->localPlayer->submergedTimer != LUMBERJACK_SUBMERGE_TIMER && ((lumv->localPlayer->submergedTimer/10) % 3) != 0)
    {
        drawWsg(&lumv->alertSprite, lumv->localPlayer->x + 6, lumv->localPlayer->y - 26 - lumv->yOffset, false, false, 0);
    }

    lumv->localPlayer->drawFrame = currentFrame;

    // This is where it breaks. When it tries to play frame 3 or 4 it crashes.
    drawWsg(&lumv->playerSprites[lumv->localPlayer->drawFrame], lumv->localPlayer->x - 4,
            lumv->localPlayer->y - lumv->yOffset, lumv->localPlayer->flipped, false, 0);

    if (lumv->localPlayer->x > LUMBERJACK_SCREEN_X_MAX)
    {
        drawWsg(&lumv->playerSprites[lumv->localPlayer->drawFrame], lumv->localPlayer->x - LUMBERJACK_SCREEN_X_OFFSET,
                lumv->localPlayer->y - lumv->yOffset, lumv->localPlayer->flipped, false, 0);
    }

    if (lumv->remotePlayer->active)
    {
        drawWsg(&lumv->playerSprites[lumv->remotePlayer->drawFrame], lumv->remotePlayer->x - 4,
                lumv->remotePlayer->y - lumv->yOffset, false, false, 0);
    }

    //If playing panic mode draw water

    lumberjackDrawWaterLevel();

    for (int i = 0; i < lumv->localPlayer->lives; i++)
    {
        int icon = lumv->localPlayer->type;

        if (icon > ARRAY_SIZE(lumv->minicharacters))
        {
            icon = 0;
        }
        drawWsgSimple(&lumv->minicharacters[icon], (i * 14) + 22, 32);
    }

    if (lumv->gameOver)
    {
        drawWsgSimple(&lumv->gameoverSprite, (TFT_WIDTH/2) -72, (TFT_HEIGHT/2) - 9);
    }

    char* score_display = "000";

    drawText(&lumv->arcade, c000, score_display, 26, 18);
    drawText(&lumv->arcade, c000, score_display, 26, 15);
    drawText(&lumv->arcade, c000, score_display, 25, 16);
    drawText(&lumv->arcade, c000, score_display, 27, 16);
    drawText(&lumv->arcade, c555, score_display, 26, 16);
    // Debug

    char debug[20] = {0};
    /*
    snprintf(debug, sizeof(debug), "Debug: %d %d %d", lumv->localPlayer->x, lumv->localPlayer->y,
             lumv->localPlayer->cH);

    drawText(&lumv->arcade, c000, debug, 16, 16);

    // drawRect(lumv->localPlayer->cX, lumv->localPlayer->cY - lumv->yOffset, lumv->localPlayer->cX +
    // lumv->localPlayer->cW, lumv->localPlayer->cY - lumv->yOffset + lumv->localPlayer->cH, c050);

    if (lumv->localPlayer->jumpPressed)
    {
        drawText(&lumv->arcade, c555, "A", 16, 32);
    }
    else
    {
        drawText(&lumv->arcade, c000, "A", 16, 32);
    }

    if (lumv->localPlayer->attackPressed)
    {
        drawText(&lumv->arcade, c555, "B", 48, 32);
    }
    else
    {
        drawText(&lumv->arcade, c000, "B", 48, 32);
    }
    */
}

void lumberjackSpawnCheck(int64_t elapseUs)
{
    if (lumv->spawnTimer >= 0)
    {
        bool spawnReady = true;

        float elapse = (elapseUs / 1000);
        lumv->spawnTimer -= elapse;

        if (lumv->spawnTimer < 0)
        {
            for (int enemyIndex = 0; enemyIndex < ARRAY_SIZE(lumv->enemy); enemyIndex++)
            {
                if (lumv->enemy[enemyIndex] == NULL)
                    continue;

                if (lumv->enemy[enemyIndex]->ready && lumv->enemy[enemyIndex]->state != LUMBERJACK_DEAD)
                {
                    lumv->spawnSide++;
                    lumv->spawnSide %= 2;

                    lumv->spawnTimer = LUMBERJACK_RESPAWN_TIMER;

                    lumberjackRespawnEnemy(lumv->enemy[enemyIndex], lumv->spawnSide);
                    spawnReady = false;
                    break;
                }
            }

            if (spawnReady)
            {
                // No one was spawned.
                lumv->spawnTimer = 500;
            }
        }
    }
}

static void lumberjackUpdateEntity(lumberjackEntity_t* entity, int64_t elapsedUs)
{
    bool onGround = false;

    // World wrap
    entity->x %= 295;
    if (entity->x < -20)
    {
        entity->x += 295;
    }

    if (entity->state == LUMBERJACK_BUMPED)
    {
        entity->vy -= 2;
        entity->respawn = 1500;

        if (entity->vy >= 0)
        {
            entity->state = LUMBERJACK_BUMPED_IDLE;
        }
    }

    // Check jumping first
    if (entity->jumpPressed && entity->active)
    {
        if (entity->onGround && entity->jumpReady)
        {
            // Check if player CAN jump
            entity->jumpReady = false;
            entity->jumping   = true;
            entity->vy        = -15;
            entity->jumpTimer = 225000;
            entity->onGround  = false;
        }
        else if (entity->jumping)
        {
            entity->vy -= 6;
        }
    }

    if (entity->jumpTimer > 0 && entity->active)
    {
        entity->jumpTimer -= elapsedUs;
        if (entity->jumpTimer <= 0)
        {
            entity->jumpTimer = 0;
            entity->jumping   = false;
        }
    }

    if (entity->jumping == false && entity->flying == false)
        entity->vy += 6; // Fix gravity

    if (entity->active)
    {
        if (entity->direction > 0 && entity->state != LUMBERJACK_DUCK)
        {
            if (entity->onGround)
                entity->vx += 5;
            else
                entity->vx += 8;
        }
        else if (entity->direction < 0 && entity->state != LUMBERJACK_DUCK)
        {
            if (entity->onGround)
                entity->vx -= 5;
            else
                entity->vx -= 8;
        }
        else
        {
            if (entity->onGround)
                entity->vx *= .1;
        }
    }

    if (entity->vx > entity->maxVX)
        entity->vx = entity->maxVX;
    if (entity->vx < -entity->maxVX)
        entity->vx = -entity->maxVX;
    if (entity->vy < -30)
        entity->vy = -30;
    if (entity->vy > 16)
        entity->vy = 16;

    float elapsed    = elapsedUs / 100000.0;
    int destinationX = entity->x + (int)(entity->vx * elapsed);
    int destinationY = entity->y + (entity->vy * elapsed);

    if (entity->vx < 0 && entity->active)
    {
        lumberjackTile_t* tileA = lumberjackGetTile(destinationX + 0, entity->y + 2);
        lumberjackTile_t* tileB = lumberjackGetTile(destinationX + 0, entity->y + entity->height);

        if ((tileA != NULL && lumberjackIsCollisionTile(tileA->type))
            || (tileB != NULL && lumberjackIsCollisionTile(tileB->type)))
        {
            destinationX = entity->x;
            entity->vx   = 0;
        }
    }
    else if (entity->vx > 0 && entity->active)
    {
        lumberjackTile_t* tileA = lumberjackGetTile(destinationX + 24, entity->y + 2);
        lumberjackTile_t* tileB = lumberjackGetTile(destinationX + 24, entity->y + entity->height);

        if ((tileA != NULL && lumberjackIsCollisionTile(tileA->type))
            || (tileB != NULL && lumberjackIsCollisionTile(tileB->type)))
        {
            destinationX = entity->x;
            entity->vx   = 0;
        }
    }

    if (entity->vy < 0 && entity->active)
    {
        lumberjackTile_t* tileA = lumberjackGetTile(destinationX, destinationY);
        lumberjackTile_t* tileB = lumberjackGetTile(destinationX + 16, destinationY);

        if (lumberjackIsCollisionTile(tileA->type) || lumberjackIsCollisionTile(tileB->type))
        {
            destinationY      = ((tileA->y + 1) * 16);
            entity->jumpTimer = 0;
            entity->jumping   = false;
            entity->vy        = 0;

            if (lumberjackIsCollisionTile(tileA->type))
            {
                lumv->tile[tileA->index].offset      = 10;
                lumv->tile[tileA->index].offset_time = 100;

                lumberjackDetectBump(tileA);
            }

            if (lumberjackIsCollisionTile(tileB->type))
            {
                lumberjackDetectBump(tileB);

                lumv->tile[tileB->index].offset      = 10;
                lumv->tile[tileB->index].offset_time = 100;
            }
        }
    }
    else if (entity->vy > 0 && entity->active)
    {
        lumberjackTile_t* tileA = lumberjackGetTile(destinationX, destinationY + entity->height);
        lumberjackTile_t* tileB = lumberjackGetTile(destinationX + 16, destinationY + entity->height);

        if ((tileA != NULL && lumberjackIsCollisionTile(tileA->type))
            || (tileB != NULL && lumberjackIsCollisionTile(tileB->type)))
        {
            destinationY = ((tileA->y - entity->tileHeight) * 16);
            entity->vy   = 0;
            onGround     = true;
        }
    }

    entity->onGround = onGround;

    if (entity->vx > entity->maxVX)
    {
        // ESP_LOGI(LUM_TAG, "ERROR");
    }

    entity->x = destinationX;
    entity->y = destinationY;

    if (entity->y > 350)
    {
        entity->y      = 350;
        entity->active = false;
        if (entity->state == LUMBERJACK_DEAD)
        {
            //
            if (entity == lumv->localPlayer)
            {
                if (entity->respawn == 0)
                {
                    // ESP_LOGI(LUM_TAG, "DEAD & hit the ground %d", entity->respawn);
                    entity->respawn = 250;
                    entity->ready   = true;
                    return;
                }
            }
            else
            {
                // Entity is not local player
            }
        }

        if (entity->state != LUMBERJACK_OFFSCREEN && entity->state != LUMBERJACK_DEAD)
        {
            if (entity == lumv->localPlayer)
            {
                entity->respawn          = 500;
                entity->ready            = true;
                lumberjackOnLocalPlayerDeath();
                return;
            }
            else
            {
                entity->active = false;
                entity->ready  = true;
            }
        }
        if (entity->state != LUMBERJACK_DEAD)
            entity->state = LUMBERJACK_OFFSCREEN;
    }
}

void lumberjackUpdate(int64_t elapseUs)
{
    for (int tileIndex = 0; tileIndex < lumv->currentMapHeight * LUMBERJACK_MAP_WIDTH; tileIndex++)
    {
        if (lumv->tile[tileIndex].offset > 0)
        {
            lumv->tile[tileIndex].offset_time -= elapseUs;
            if (lumv->tile[tileIndex].offset_time < 0)
            {
                lumv->tile[tileIndex].offset_time += 2500;
                lumv->tile[tileIndex].offset--;
            }
        }
    }
}

void lumberjackTileMap(void)
{
    for (int y = 0; y < lumv->currentMapHeight; y++)
    {
        for (int x = 0; x < LUMBERJACK_MAP_WIDTH; x++)
        {
            int index     = (y * LUMBERJACK_MAP_WIDTH) + x;

            int tileIndex = lumv->tile[index].type;
            int offset    = lumv->tile[index].offset;

            if ((y * 16) - lumv->yOffset >= -16)
            {
                if (tileIndex > 0 && tileIndex < 13)
                {
                    if (tileIndex < 11)
                    {
                        drawWsgSimple(&lumv->floorTiles[tileIndex - 1], x * 16, (y * 16) - lumv->yOffset - offset);
                    }
                }
            }
        }
    }

}

void lumberjackDrawWaterLevel(void)
{
    //If GameMode is Panic... draw the water
    for (int i = 0; i < LUMBERJACK_MAP_WIDTH; i++)
    {
        drawWsgSimple(&lumv->animationTiles[lumv->liquidAnimationFrame], i * 16, lumv->waterLevel - lumv->yOffset);
    }

    fillDisplayArea(0, lumv->waterLevel + 16 - lumv->yOffset, TFT_WIDTH, ((lumv->currentMapHeight+1) * 16)- lumv->yOffset , c134);

}


void lumberjackDoControls(void)
{
    lumv->localPlayer->cW = 15;
    int previousState     = lumv->localPlayer->state;
    bool buttonPressed    = false;
    if (lumv->btnState & PB_LEFT)
    {
        lumv->localPlayer->flipped   = true;
        lumv->localPlayer->state     = LUMBERJACK_RUN;
        lumv->localPlayer->direction = -1;
        buttonPressed                = true;
    }
    else if (lumv->btnState & PB_RIGHT)
    {
        lumv->localPlayer->flipped   = false;
        lumv->localPlayer->state     = LUMBERJACK_RUN;
        lumv->localPlayer->direction = 1;

        buttonPressed = true;
    }
    else
    {
        lumv->localPlayer->direction = 0;
    }

    if (lumv->btnState & PB_DOWN)
    {
        buttonPressed = true;
        if (lumv->localPlayer->onGround)
        {
            lumv->localPlayer->state = LUMBERJACK_DUCK;

            lumv->localPlayer->cW = 15;
            lumv->localPlayer->cH = 15;
        }
    }
    else if (lumv->btnState & PB_UP)
    {
        buttonPressed = true;
    }

    // TODO: This is sloppy Troy
    if (lumv->btnState & PB_A)
    {
        lumv->localPlayer->jumpPressed = true;
    }
    else
    {
        lumv->localPlayer->jumpPressed = false;
    }

    // TODO: This is sloppy too Troy
    if (lumv->btnState & PB_B)
    {
        lumv->localPlayer->attackPressed = true;
    }
    else
    {
        lumv->localPlayer->attackPressed = false;
    }

    if (buttonPressed == false && lumv->localPlayer->active)
    {
        if (lumv->localPlayer->state != LUMBERJACK_INVINCIBLE)
            lumv->localPlayer->state = LUMBERJACK_IDLE; // Do a ton of other checks here
    }

    if (lumv->localPlayer->state != previousState)
    {
        // lumv->localPlayer->currentFrame = 0;
        lumv->localPlayer->timerFrameUpdate = 0;
    }
}

static lumberjackTile_t* lumberjackGetTile(int x, int y)
{
    int tx = (int)x / 16;
    int ty = (int)y / 16;

    if (tx < 0)
        tx = 17;
    if (tx > 17)
        tx = 0;

    if (ty < 0)
        ty = 0;
    if (ty > lumv->currentMapHeight)
        ty = lumv->currentMapHeight;

    // int test = -1;
    for (int colTileIndex = 0; colTileIndex < 32; colTileIndex++)
    {
        if ((ty * LUMBERJACK_MAP_WIDTH) + tx >= lumv->currentMapHeight * LUMBERJACK_MAP_WIDTH)
        {
            //Don't calculate if entity is going off screen
            continue;
        }

        if (lumberjackCollisionCheckTiles[colTileIndex].type == -1)
        {
            if (lumberjackCollisionCheckTiles[colTileIndex].index == (ty * LUMBERJACK_MAP_WIDTH) + tx)
                return &lumberjackCollisionCheckTiles[colTileIndex];

            lumberjackCollisionCheckTiles[colTileIndex].index = (ty * LUMBERJACK_MAP_WIDTH) + tx;
            lumberjackCollisionCheckTiles[colTileIndex].x     = tx;
            lumberjackCollisionCheckTiles[colTileIndex].y     = ty;
            lumberjackCollisionCheckTiles[colTileIndex].type  = lumv->tile[(ty * LUMBERJACK_MAP_WIDTH) + tx].type;

            return &lumberjackCollisionCheckTiles[colTileIndex];
        }
        // test = i;
    }

    // ESP_LOGI(LUM_TAG,"NO TILE at %d %d!", test, ty);
    return NULL;
}

static bool lumberjackIsCollisionTile(int index)
{
    if (index == 0 || index == 1 || index == 6 || index == 5 || index == 10)
        return false;

    return true;
}

void lumberjackDetectBump(lumberjackTile_t* tile)
{
    if (lumv->localPlayer->state != LUMBERJACK_BUMPED && lumv->localPlayer->state != LUMBERJACK_DEAD)
    {
        // TODO put in bump the player
    }

    for (int enemyIndex = 0; enemyIndex < ARRAY_SIZE(lumv->enemy); enemyIndex++)
    {
        lumberjackEntity_t* enemy = lumv->enemy[enemyIndex];
        if (enemy == NULL)
            continue;

        if (enemy->onGround || enemy->flying)
        {
            int tx  = ((enemy->x - 8) / 16);
            int ty  = ((enemy->y) / 16) + 1;
            int tx2 = ((enemy->x + 8) / 16);

            if (tx < 0)
                tx = 0;
            if (tx > 17)
                tx = 17;
            if (tx2 < 0)
                tx2 = 0;
            if (tx2 > 17)
                tx2 = 17;
            if (ty < 0)
                ty = 0;
            if (ty > lumv->currentMapHeight)
                ty = lumv->currentMapHeight;

            lumberjackTile_t* tileA = &lumv->tile[(ty * LUMBERJACK_MAP_WIDTH) + tx];
            lumberjackTile_t* tileB = &lumv->tile[(ty * LUMBERJACK_MAP_WIDTH) + tx2];

            if ((tileA != NULL && (ty * LUMBERJACK_MAP_WIDTH) + tx == tile->index) || (tileB != NULL && (ty * LUMBERJACK_MAP_WIDTH) + tx2 == tile->index))
            {
                enemy->vy       = -20;
                enemy->onGround = false;

                if (enemy->state == LUMBERJACK_BUMPED_IDLE)
                {
                    enemy->state     = LUMBERJACK_RUN;
                    enemy->direction = enemy->flipped ? -1 : 1;
                }
                else
                {
                    enemy->direction = 0;
                    enemy->state     = LUMBERJACK_BUMPED;
                }
            }
        }
    }
}

///
///

void lumberjackExitGameMode(void)
{
    // Everything crashes if you don't load it first
    if (lumv == NULL)
        return;

    //** FREE THE SPRITES **//

    freeWsg(&lumv->gameoverSprite);

    // Free the enemies
    for (int i = 0; i < ARRAY_SIZE(lumv->enemySprites); i++)
    {
        freeWsg(&lumv->enemySprites[i]);
    }

    // Free the players
    for (int i = 0; i < ARRAY_SIZE(lumv->playerSprites); i++)
    {
        freeWsg(&lumv->playerSprites[i]);
    }

    // Free the tiles
    for (int i = 0; i < ARRAY_SIZE(lumv->animationTiles); i++)
    {
        freeWsg(&lumv->animationTiles[i]);
    }

    //Free the axes
    for (int i = 0; i < ARRAY_SIZE(lumv->greenBlockSprite); i++)
    {
        freeWsg(&lumv->greenBlockSprite[i]);
    }

    //Free the axes
    for (int i = 0; i < ARRAY_SIZE(lumv->redBlockSprite); i++)
    {
        freeWsg(&lumv->redBlockSprite[i]);
    }

    //Free the axes
    for (int i = 0; i < ARRAY_SIZE(lumv->unusedBlockSprite); i++)
    {
        freeWsg(&lumv->unusedBlockSprite[i]);
    }

    freeWsg(&lumv->alertSprite);

    freeFont(&lumv->arcade);
    free(lumv);
}
