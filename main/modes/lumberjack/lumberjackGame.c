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

#include "esp_random.h"


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
#define LUMBERJACK_SCREEN_X_MAX         270

#define LUMBERJACK_SCREEN_Y_OFFSET      140
#define LUMBERJACK_SCREEN_Y_MIN         -36

#define LUMBERJACK_MAP_WIDTH            18
#define LUMBERJACK_BLOCK_ANIMATION_MAX  7
#define LUMBERJACK_ROTATE_ANIMATION_MAX 4
#define LUMBERJACK_RESPAWN_PANIC_MIN    3750
#define LUMBERJACK_RESPAWN_ATTACK_MIN   1000
#define LUMBERJACK_RESPAWN_VS_MIN       250
#define LUMBERJACK_UPGRADE_TIMER_OFFSET 250
#define LUMBERJACK_SUBMERGE_TIMER       300
#define LUMBERJACK_WORLD_WRAP           295

#define LUMBERJACK_GHOST_SPAWNTIME_MIN  2000
#define LUMBERJACK_GHOST_SPEED_NORMAL   3
#define LUMBERJACK_GHOST_SPEED_FAST     3
#define LUMBERJACK_GHOST_BOX            28
#define LUMBERJACK_GHOST_ANIMATION      2500

#define LUMBERJACK_BONUS_DISPLAYDURATI  4000

#define LUMBERJACK_WATER_FAST_DRAIN     6
#define LUMBERJACK_WATER_SLOW_DRAIN     3
#define LUMBERJACK_WATER_INCREASE       10

#define LUMBERJACK_ITEM_RESETTIME       2500
#define LUMBERJACK_INVINCIBLE_TIME      7000
#define LUMBERJACK_INVINCIBLE_SPEED     100000
#define LUMBERJACK_COMBO_RESET_TIME     1500

#define LUMBERJACK_TILE_SIZE            16

static lumberjackTile_t* lumberjackGetTile(int x, int y);
static void lumberjackUpdateEntity(lumberjackEntity_t* entity, int64_t elapsedUs);
static bool lumberjackIsCollisionTile(int index);

bool lumberjackLoadLevel(void);
void lumberjackOnLocalPlayerDeath(void);

void DrawTitle(void);
void DrawGame(void);

lumberjackVars_t* lumv;

void lumberjackStartGameMode(lumberjack_t* main, uint8_t characterIndex)
{
    lumv                   = calloc(1, sizeof(lumberjackVars_t));
    lumv->lumberjackMain   = main;
    lumv->localPlayerType  = characterIndex;
    lumv->score            = 0;
    lumv->highscore        = 5000;

    loadFont("eightbit_atari_grube2.font", &lumv->arcade, false);

    bzrStop(true); // Stop the buzzer?

    for (int i = 0; i < ARRAY_SIZE(lumv->axeBlocks); i++)
    {
        lumv->axeBlocks[i] = calloc(1, sizeof(lumberjackAxeBlock_t));
        lumv->axeBlocks[i]->x = -90;
        lumv->axeBlocks[i]->y = -1000;
        lumv->axeBlocks[i]->active = false;
    }

    lumv->ghost                = NULL;
    lumv->worldTimer           = 0;
    lumv->liquidAnimationFrame = 0;
    lumv->stageAnimationFrame  = 0;
    lumv->loaded               = false;
    lumv->gameType             = main->gameMode;
    lumv->gameState            = LUMBERJACK_GAMESTATE_TITLE;
    lumv->levelIndex           = 0;
    lumv->lives                = 3;
    lumv->gameReady            = !(main->networked);
    lumv->invincibleColor      = c000;

    lumv->itemBlockAnimationTime = 0;
    lumv->itemBlockItemFrame = 0;
    lumv->itemBlockIndex     = -1;
    lumv->itemBlockItemAnimation = 0;

    //ESP_LOGI(LUM_TAG, "Load Title");
    loadWsg("lumbers_title.wsg", &lumv->title, true);

    if (main->screen == LUMBERJACK_A)
    {
        loadWsg("lumbers_title_panic_red.wsg", &lumv->subtitle_red, true);
        loadWsg("lumbers_title_panic_green.wsg", &lumv->subtitle_green, true);
        loadWsg("lumbers_title_panic_white.wsg", &lumv->subtitle_white, true);
        lumv->enemySpawnTime        = LUMBERJACK_RESPAWN_PANIC_MIN;
        lumv->enemyReceiveSpawnTime = LUMBERJACK_RESPAWN_VS_MIN;
        lumv->enemySpawnAmount      = 1;
    }

    if (main->screen == LUMBERJACK_B)
    {
    
        loadWsg("lumbers_title_attack_red.wsg", &lumv->subtitle_red, true);
        loadWsg("lumbers_title_attack_green.wsg", &lumv->subtitle_green, true);    
        loadWsg("lumbers_title_attack_white.wsg", &lumv->subtitle_white, true);    
        lumv->enemySpawnTime        = LUMBERJACK_RESPAWN_ATTACK_MIN;
        lumv->enemyReceiveSpawnTime = LUMBERJACK_RESPAWN_VS_MIN;
        lumv->enemySpawnAmount      = 4;
    }

    loadWsg("lumbers_game_over.wsg", &lumv->gameoverSprite, true);
    if (lumv->lumberjackMain->networked) 
    {
        loadWsg("lumbers_game_over_win.wsg", &lumv->gamewinSprite, true);
        loadWsg("lumbers_conn_lost.wsg", &lumv->connectLostSprite, true);
    }

    //ESP_LOGI(LUM_TAG, "Loading floor Tiles");
    loadWsg("lumbers_bottom_floor1.wsg", &lumv->floorTiles[0], true);
    loadWsg("lumbers_bottom_floor2.wsg", &lumv->floorTiles[1], true);
    loadWsg("lumbers_bottom_floor3.wsg", &lumv->floorTiles[2], true);
    loadWsg("lumbers_bottom_floor4.wsg", &lumv->floorTiles[3], true);
    loadWsg("lumbers_bottom_floor5.wsg", &lumv->floorTiles[4], true);
    loadWsg("lumbers_bottom_floor6.wsg", &lumv->floorTiles[5], true);
    loadWsg("lumbers_bottom_floor7.wsg", &lumv->floorTiles[6], true);
    loadWsg("lumbers_bottom_floor8.wsg", &lumv->floorTiles[7], true);
    loadWsg("lumbers_bottom_floor9.wsg", &lumv->floorTiles[8], true);
    loadWsg("lumbers_bottom_floor10.wsg", &lumv->floorTiles[9], true);
    loadWsg("lumbers_itemused_block.wsg", &lumv->floorTiles[10], true);
    loadWsg("lumbers_rtile_1.wsg", &lumv->floorTiles[11], true);
    //ESP_LOGI(LUM_TAG, "Loading Animation Tiles");

    loadWsg("lumbers_water_floor1.wsg", &lumv->animationTiles[0], true);
    loadWsg("lumbers_water_floor2.wsg", &lumv->animationTiles[1], true);
    loadWsg("lumbers_water_floor3.wsg", &lumv->animationTiles[2], true);
    loadWsg("lumbers_water_floor4.wsg", &lumv->animationTiles[3], true);
    loadWsg("lumbers_water_floor0.wsg", &lumv->animationTiles[4], true);
    loadWsg("lumbers_water_floor0.wsg", &lumv->animationTiles[5], true);
    loadWsg("lumbers_water_floor0.wsg", &lumv->animationTiles[6], true);
    loadWsg("lumbers_water_floor0.wsg", &lumv->animationTiles[7], true);
    loadWsg("lumbers_water_floor_b1.wsg", &lumv->animationTiles[8], true);
    loadWsg("lumbers_water_floor_b2.wsg", &lumv->animationTiles[9], true);
    loadWsg("lumbers_water_floor_b3.wsg", &lumv->animationTiles[10], true);
    loadWsg("lumbers_water_floor_b4.wsg", &lumv->animationTiles[11], true);
    loadWsg("lumbers_rtile_1.wsg", &lumv->animationTiles[12], true);
    loadWsg("lumbers_rtile_2.wsg", &lumv->animationTiles[13], true);
    loadWsg("lumbers_rtile_3.wsg", &lumv->animationTiles[14], true);
    loadWsg("lumbers_rtile_4.wsg", &lumv->animationTiles[15], true);
    


    //ESP_LOGI(LUM_TAG, "Loading Characters");

    //ESP_LOGI(LUM_TAG, "*Loading character icons");
    loadWsg("lumbers_red_lives.wsg", &lumv->minicharacters[0], true);
    loadWsg("lumbers_green_lives.wsg", &lumv->minicharacters[1], true);
    loadWsg("lumbers_secret_swadgeland_lives.wsg", &lumv->minicharacters[2], true);
    loadWsg("lumbers_cho_lives.wsg", &lumv->minicharacters[3], true);

    //ESP_LOGI(LUM_TAG, "*Loading character sprites");
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
        lumv->invincibleColor      = c500;

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
        lumv->invincibleColor      = c050;

    }
    else if (characterIndex == 2)
    {

        loadWsg("lumbers_secret_swadgeland_1.wsg", &lumv->playerSprites[0], true);
        loadWsg("lumbers_secret_swadgeland_2.wsg", &lumv->playerSprites[1], true);
        loadWsg("lumbers_secret_swadgeland_3.wsg", &lumv->playerSprites[2], true);
        loadWsg("lumbers_secret_swadgeland_4.wsg", &lumv->playerSprites[3], true);
        loadWsg("lumbers_secret_swadgeland_5.wsg", &lumv->playerSprites[4], true);
        loadWsg("lumbers_secret_swadgeland_6.wsg", &lumv->playerSprites[5], true);
        loadWsg("lumbers_secret_swadgeland_7.wsg", &lumv->playerSprites[6], true);
        loadWsg("lumbers_secret_swadgeland_8.wsg", &lumv->playerSprites[7], true);
        loadWsg("lumbers_secret_swadgeland_9.wsg", &lumv->playerSprites[8], true);
        loadWsg("lumbers_secret_swadgeland_10.wsg", &lumv->playerSprites[9], true);
        loadWsg("lumbers_secret_swadgeland_11.wsg", &lumv->playerSprites[10], true);
        loadWsg("lumbers_secret_swadgeland_12.wsg", &lumv->playerSprites[11], true);
        loadWsg("lumbers_secret_swadgeland_13.wsg", &lumv->playerSprites[12], true);
        loadWsg("lumbers_secret_swadgeland_14.wsg", &lumv->playerSprites[13], true);
        loadWsg("lumbers_secret_swadgeland_15.wsg", &lumv->playerSprites[14], true);
        loadWsg("lumbers_secret_swadgeland_16.wsg", &lumv->playerSprites[15], true);
        loadWsg("lumbers_secret_swadgeland_17.wsg", &lumv->playerSprites[16], true);
        loadWsg("lumbers_secret_swadgeland_22.wsg", &lumv->playerSprites[17], true);
        lumv->invincibleColor      = c305;

    } 
    else if (characterIndex == 3)
    {
        loadWsg("lumbers_cho_1.wsg", &lumv->playerSprites[0], true);
        loadWsg("lumbers_cho_2.wsg", &lumv->playerSprites[1], true);
        loadWsg("lumbers_cho_3.wsg", &lumv->playerSprites[2], true);
        loadWsg("lumbers_cho_4.wsg", &lumv->playerSprites[3], true);
        loadWsg("lumbers_cho_5.wsg", &lumv->playerSprites[4], true);
        loadWsg("lumbers_cho_6.wsg", &lumv->playerSprites[5], true);
        loadWsg("lumbers_cho_7.wsg", &lumv->playerSprites[6], true);
        loadWsg("lumbers_cho_8.wsg", &lumv->playerSprites[7], true);
        loadWsg("lumbers_cho_9.wsg", &lumv->playerSprites[8], true);
        loadWsg("lumbers_cho_10.wsg", &lumv->playerSprites[9], true);
        loadWsg("lumbers_cho_11.wsg", &lumv->playerSprites[10], true);
        loadWsg("lumbers_cho_12.wsg", &lumv->playerSprites[11], true);
        loadWsg("lumbers_cho_13.wsg", &lumv->playerSprites[12], true);
        loadWsg("lumbers_cho_14.wsg", &lumv->playerSprites[13], true);
        loadWsg("lumbers_cho_15.wsg", &lumv->playerSprites[14], true);
        loadWsg("lumbers_cho_16.wsg", &lumv->playerSprites[15], true);
        loadWsg("lumbers_cho_17.wsg", &lumv->playerSprites[16], true);
        loadWsg("lumbers_cho_18.wsg", &lumv->playerSprites[17], true);
        lumv->invincibleColor      = c103;

    } 

    //ESP_LOGI(LUM_TAG, "Loading Enemies");
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
    loadWsg("lumbers_enemy_d1.wsg", &lumv->enemySprites[21], true);
    loadWsg("lumbers_enemy_d2.wsg", &lumv->enemySprites[22], true);
    loadWsg("lumbers_enemy_e1.wsg", &lumv->enemySprites[23], true);
    loadWsg("lumbers_enemy_e2.wsg", &lumv->enemySprites[24], true);
    loadWsg("lumbers_enemy_e3.wsg", &lumv->enemySprites[25], true);
    loadWsg("lumbers_enemy_e4.wsg", &lumv->enemySprites[26], true);
    loadWsg("lumbers_enemy_e5.wsg", &lumv->enemySprites[27], true);
    loadWsg("lumbers_enemy_e6.wsg", &lumv->enemySprites[28], true);
    loadWsg("lumbers_enemy_e7.wsg", &lumv->enemySprites[29], true);
    loadWsg("lumbers_enemy_f1.wsg", &lumv->enemySprites[30], true);
    loadWsg("lumbers_enemy_f2.wsg", &lumv->enemySprites[31], true);
    loadWsg("lumbers_enemy_f3.wsg", &lumv->enemySprites[32], true);
    loadWsg("lumbers_enemy_f4.wsg", &lumv->enemySprites[33], true);
    loadWsg("lumbers_enemy_f5.wsg", &lumv->enemySprites[34], true);
    loadWsg("lumbers_enemy_f6.wsg", &lumv->enemySprites[35], true);
    loadWsg("lumbers_enemy_f7.wsg", &lumv->enemySprites[36], true);

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

    loadWsg("lumbers_bonus_0.wsg", &lumv->bonusSprites[0], true);
    loadWsg("lumbers_bonus_1.wsg", &lumv->bonusSprites[1], true);
    loadWsg("lumbers_bonus_2.wsg", &lumv->bonusSprites[2], true);
    loadWsg("lumbers_bonus_3.wsg", &lumv->bonusSprites[3], true);
    loadWsg("lumbers_bonus_4.wsg", &lumv->bonusSprites[4], true);
    loadWsg("lumbers_bonus_5.wsg", &lumv->bonusSprites[5], true);
    loadWsg("lumbers_bonus_6.wsg", &lumv->bonusSprites[6], true);
    loadWsg("lumbers_bonus_7.wsg", &lumv->bonusSprites[7], true);
    loadWsg("lumbers_bonus_8.wsg", &lumv->bonusSprites[8], true);
    loadWsg("lumbers_bonus_9.wsg", &lumv->bonusSprites[9], true);
    loadWsg("lumbers_bonus_x.wsg", &lumv->bonusSprites[10], true);
    

    loadWsg("lumbers_item_ui.wsg", &lumv->ui[0], true);
    loadWsg("lumbers_alert.wsg", &lumv->alertSprite, true);

    for (int i = 0; i < ARRAY_SIZE(lumv->bonusDisplay); i++)
    {
        lumv->bonusDisplay[i] = calloc(1, sizeof(lumberjackBonus_t));
    }
    lumv->activeBonusIndex = 0;


    if (lumv->gameType == LUMBERJACK_MODE_ATTACK)
    {
        loadWsg("lumbers_items_grapes1.wsg", &lumv->itemIcons[0], true);
        loadWsg("lumbers_items_grapes2.wsg", &lumv->itemIcons[1], true);
        loadWsg("lumbers_items_grapes3.wsg", &lumv->itemIcons[2], true);
        loadWsg("lumbers_items_grapes4.wsg", &lumv->itemIcons[3], true);
        loadWsg("lumbers_items_grapes5.wsg", &lumv->itemIcons[4], true);
        loadWsg("lumbers_items_grapes6.wsg", &lumv->itemIcons[5], true);
        loadWsg("lumbers_items_orange1.wsg", &lumv->itemIcons[6], true);
        loadWsg("lumbers_items_orange2.wsg", &lumv->itemIcons[7], true);
        loadWsg("lumbers_items_orange3.wsg", &lumv->itemIcons[8], true);
        loadWsg("lumbers_items_orange4.wsg", &lumv->itemIcons[9], true);
        loadWsg("lumbers_items_orange5.wsg", &lumv->itemIcons[10], true);
        loadWsg("lumbers_items_orange6.wsg", &lumv->itemIcons[11], true);
        loadWsg("lumbers_items_pear1.wsg", &lumv->itemIcons[12], true);
        loadWsg("lumbers_items_pear2.wsg", &lumv->itemIcons[13], true);
        loadWsg("lumbers_items_pear3.wsg", &lumv->itemIcons[14], true);
        loadWsg("lumbers_items_pear4.wsg", &lumv->itemIcons[15], true);
        loadWsg("lumbers_items_pear5.wsg", &lumv->itemIcons[16], true);
        loadWsg("lumbers_items_pear6.wsg", &lumv->itemIcons[17], true);

    }

}

bool lumberjackLoadLevel()
{
    char* fname = "lumberjacks_panic_vs.bin";
    
    lumv->upgrade = 0;
    if (lumv->gameType == LUMBERJACK_MODE_PANIC)
    {

        if (!lumv->lumberjackMain->networked)
        {
            char* levelName[] =
            {
                "lumberjacks_panic_1.bin",
                "lumberjacks_panic_2.bin",
                "lumberjacks_panic_3.bin",
                "lumberjacks_panic_4.bin",
                "lumberjacks_panic_5.bin",
                "lumberjacks_panic_6.bin",
                "lumberjacks_panic_7.bin",
                "lumberjacks_panic_10.bin",
            };
            
            fname = levelName[lumv->levelIndex % ARRAY_SIZE(levelName)];

            lumv->upgrade = (int)(lumv->levelIndex / ARRAY_SIZE(levelName));
        }
        else 
        {
            fname = "lumberjacks_panic_vs.bin";
        }
        
        lumv->enemySpawnAmount      = 1;    
    }

    if (lumv->gameType == LUMBERJACK_MODE_ATTACK)
    {
        if (!lumv->lumberjackMain->networked)
        {
            char* attackLevelName[] = 
            {
                "lumberjacks_attack_1.bin",
                "lumberjacks_attack_2.bin",
                "lumberjacks_attack_3.bin",
                "lumberjacks_attack_4.bin",
                "lumberjacks_attack_5.bin",
            };

            fname = attackLevelName[lumv->levelIndex % ARRAY_SIZE(attackLevelName)];
            lumv->upgrade = (int)(lumv->levelIndex / ARRAY_SIZE(attackLevelName));
        }
        else 
        {
            fname = "lumberjacks_attack_vs.bin";
        }

        for (int i = 0; i < 8; i++)
        {
            lumv->attackQueue[i] = 0;
            lumv->receiveQueue[i] = 0;
        }
        
        lumv->enemySpawnAmount      = 4;    
        lumv->itemBlockIndex        = -1;
    }


    size_t ms;
    uint8_t *buffer = spiffsReadFile(fname, &ms, false);

    ESP_LOGI(LUM_TAG, "Level = %s", fname);
    // Buffer 0 = map height
    lumv->tile             = (lumberjackTile_t*)malloc((int)buffer[0] * LUMBERJACK_MAP_WIDTH * sizeof(lumberjackTile_t));
    lumv->currentMapHeight = (int)buffer[0];
    lumv->levelTime        = 0;
    lumv->itemBlockTime    = 0;
    lumv->itemBlockReady   = true;

    lumv->enemy1Count      = (int) buffer[1];
    lumv->enemy2Count      = (int) buffer[2];
    lumv->enemy3Count      = (int) buffer[3];
    lumv->enemy4Count      = (int) buffer[4];
    lumv->enemy5Count      = (int) buffer[5];
    lumv->enemy6Count      = (int) buffer[6];
    lumv->enemy7Count      = (int) buffer[7];
    lumv->enemy8Count      = (int) buffer[8];
    
    if (lumv->enemy4Count > 1) lumv->enemy4Count = 1; //Only one ghost

    lumv->totalEnemyCount  = lumv->enemy1Count; 
    lumv->totalEnemyCount += lumv->enemy2Count; 
    lumv->totalEnemyCount += lumv->enemy3Count; 
    //Ghost don't count
    lumv->totalEnemyCount += lumv->enemy5Count; 
    lumv->totalEnemyCount += lumv->enemy6Count; 
    lumv->totalEnemyCount += lumv->enemy7Count; 
    lumv->totalEnemyCount += lumv->enemy8Count; 


    if (lumv->upgrade == 1)
    {
        lumv->enemy3Count += lumv->enemy2Count;
        lumv->enemy2Count = lumv->enemy1Count;
        lumv->enemy1Count = 0; // 
    }
    else if (lumv->upgrade >= 2)
    {
        lumv->enemy3Count += lumv->enemy2Count + lumv->enemy1Count;
        lumv->enemy2Count = 0;
        lumv->enemy1Count = 0;
    }

    if (lumv->enemy4Count > 0)
    {
        lumv->ghostSpawnTime   = (int)buffer[9] * 1000;

        if (lumv->ghostSpawnTime < LUMBERJACK_GHOST_SPAWNTIME_MIN) lumv->ghostSpawnTime = LUMBERJACK_GHOST_SPAWNTIME_MIN; //Make sure if the ghost is active at least 10 seconds must past before spawning

    }

    lumv->waterSpeed       = (int)buffer[11];
    lumv->waterTimer       = (int)buffer[11];

    for (int i = 0; i < lumv->currentMapHeight * LUMBERJACK_MAP_WIDTH; i++)
    {
        lumv->tile[i].index       = i;
        lumv->tile[i].x           = i % LUMBERJACK_MAP_WIDTH;
        lumv->tile[i].y           = i / LUMBERJACK_MAP_WIDTH;
        lumv->tile[i].type        = (int)buffer[i+12];
        lumv->tile[i].offset      = 0;
        lumv->tile[i].offset_time = 0;
                
    }


    for (int i = 0; i < ARRAY_SIZE(lumv->bonusDisplay); i++)
    {
        lumv->bonusDisplay[i]->active = false;
    }
    
    int offset = (lumv->currentMapHeight * LUMBERJACK_MAP_WIDTH) + 12;
    ESP_LOGI (LUM_TAG, "%d total enemies",lumv->totalEnemyCount);
    ESP_LOGI (LUM_TAG, "%d ", lumv->currentMapHeight* LUMBERJACK_TILE_SIZE);

    lumv->playerSpawnX = (int)buffer[offset];
    lumv->playerSpawnY = (int)buffer[offset + 1] + ((int)buffer[offset + 2] << 8);
    //lumv->yOffset = lumv->playerSpawnY - LUMBERJACK_SCREEN_Y_OFFSET;

    ESP_LOGI(LUM_TAG, "SPAWN %d %d", lumv->playerSpawnY, buffer[offset]);

    free(buffer);    

    return true;
}

void lumberjackSetupLevel(int characterIndex)
{    
    lumv->enemyKillCount       = 0;
    lumv->totalEnemyCount      = 0;
    lumv->comboTime            = 0;
    lumv->comboAmount          = 0;
    lumv->hasWon               = false;
    lumv->wakeupSignal         = 0;
    lumv->lastResponseSignal   = 1000;

    ESP_LOGI(LUM_TAG, "LOADING LEVEL");
    lumberjackLoadLevel();
        
    // This all to be loaded externally
    lumv->yOffset          = 0;
    lumv->spawnIndex       = 0;
    lumv->spawnTimer       = 2750;
    lumv->transitionTimer  = 0;
    lumv->spawnSide        = 0;
    lumv->waterLevel       = lumv->currentMapHeight * LUMBERJACK_TILE_SIZE;
    lumv->waterDirection   = -1; // This needs to be taken in to account with the intro timer

    lumv->localPlayer  = calloc(1, sizeof(lumberjackEntity_t));
    lumv->localPlayer->scoreValue       = 0;
    lumv->invincibleTimer               = LUMBERJACK_INVINCIBLE_TIME;

    lumberjackSetupPlayer(lumv->localPlayer, characterIndex);
    lumberjackSpawnPlayer(lumv->localPlayer, lumv->playerSpawnX, lumv->playerSpawnY, 0);

    int offset = 0;

    //Load enemies

    // START ENEMY 1
    if (false == lumv->lumberjackMain->networked)
    {
        for (int eSpawnIndex = 0; eSpawnIndex < lumv->enemy1Count; eSpawnIndex++)
        {
            lumv->enemy[eSpawnIndex] = calloc(1, sizeof(lumberjackEntity_t));
            lumberjackSetupEnemy(lumv->enemy[eSpawnIndex], 0);
        } 
        offset += lumv->enemy1Count;
    }
    else
    {
        for (int eSpawnIndex = 0; eSpawnIndex < 4; eSpawnIndex++)
        {
            lumv->enemy[eSpawnIndex] = calloc(1, sizeof(lumberjackEntity_t));
            lumberjackSetupEnemy(lumv->enemy[eSpawnIndex], 0);

            if (eSpawnIndex >= lumv->enemy1Count)
            {
                lumv->enemy[eSpawnIndex]->queueable = false;
            }
        }

        offset += 4;
    }

    
    if (false == lumv->lumberjackMain->networked)
    {
        for (int eSpawnIndex = 0; eSpawnIndex < lumv->enemy2Count; eSpawnIndex++)
        {
            lumv->enemy[offset + eSpawnIndex] = calloc(1, sizeof(lumberjackEntity_t));
            lumberjackSetupEnemy(lumv->enemy[offset + eSpawnIndex], 1);
        }

        offset += lumv->enemy2Count;
    } 
    else
    {
        for (int eSpawnIndex = 0; eSpawnIndex < 8; eSpawnIndex++)
        {
            lumv->enemy[offset + eSpawnIndex] = calloc(1, sizeof(lumberjackEntity_t));
            lumberjackSetupEnemy(lumv->enemy[offset + eSpawnIndex], 1);

            if (eSpawnIndex >= lumv->enemy2Count)
            {
                lumv->enemy[offset + eSpawnIndex]->queueable = false;
            }
        }

        offset += 8;
    }


    
    if (false == lumv->lumberjackMain->networked)
    {
        for (int eSpawnIndex = 0; eSpawnIndex < lumv->enemy3Count; eSpawnIndex++)
        {
            lumv->enemy[offset + eSpawnIndex] = calloc(1, sizeof(lumberjackEntity_t));
            lumberjackSetupEnemy(lumv->enemy[offset + eSpawnIndex], 2);
        } 
        
        offset += lumv->enemy3Count;
    }
    else
    {
        for (int eSpawnIndex = 0; eSpawnIndex < 8; eSpawnIndex++)
        {
            lumv->enemy[offset + eSpawnIndex] = calloc(1, sizeof(lumberjackEntity_t));
            lumberjackSetupEnemy(lumv->enemy[offset + eSpawnIndex], 2);

            if (eSpawnIndex >= lumv->enemy3Count)
            {
                lumv->enemy[offset + eSpawnIndex]->queueable = false;
            }
        }

        offset += 8;

    }
    
    
    //END ENEMY 1



    //GHOST
    if (lumv->enemy4Count > 0)
    {
        lumv->enemy4Count = 1;
        lumv->ghost = calloc(1, sizeof(lumberjackGhost_t));
        lumv->ghost->spawnTime = lumv->ghostSpawnTime ;
    }

    offset += lumv->enemy4Count;
    for (int eSpawnIndex = 0; eSpawnIndex < lumv->enemy5Count; eSpawnIndex++)
    {
        lumv->enemy[offset + eSpawnIndex] = calloc(1, sizeof(lumberjackEntity_t));
        lumberjackSetupEnemy(lumv->enemy[offset + eSpawnIndex], 4);
    } 

    offset += lumv->enemy5Count;
    for (int eSpawnIndex = 0; eSpawnIndex < lumv->enemy6Count; eSpawnIndex++)
    {
        lumv->enemy[offset + eSpawnIndex] = calloc(1, sizeof(lumberjackEntity_t));
        lumberjackSetupEnemy(lumv->enemy[offset + eSpawnIndex], 5);
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

    //Ghost is separate for reasons

    ESP_LOGI(LUM_TAG, "LOADED");
}

void lumberjackUnloadLevel(void)
{
    free(lumv->tile);

    //Unload previous enemies
    for (int i = 0; i < ARRAY_SIZE(lumv->enemy); i++)
    {
        free(lumv->enemy[i]);
        lumv->enemy[i] = NULL;
    }

    free(lumv->ghost);    
    lumv->ghost = NULL;
}

/**
 * @brief TODO use this somewhere
 */
void restartLevel(void)
{
    lumv->lives--;
    lumberjackRespawn(lumv->localPlayer, lumv->playerSpawnX, lumv->playerSpawnY);

}

void lumberjackTitleLoop(int64_t elapsedUs)
{

    // Update State
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        lumv->btnState = evt.state;
    }

    if (lumv->lumberjackMain->networked)
    {
        if (lumv->btnState & PB_A && lumv->gameReady && lumv->lumberjackMain->host && lumv->lumberjackMain->conStatus == CON_ESTABLISHED) // And Game Ready!
        {
            lumberjackPlayGame();      
            lumberjackSendGo();
        }
        lumv->highscore = 0;

    } else if (lumv->btnState & PB_A && lumv->gameReady) // And Game Ready!
    {
        lumberjackPlayGame();      

        if (lumv->lumberjackMain->networked && lumv->lumberjackMain->conStatus == CON_ESTABLISHED)
        {
            lumberjackSendGo();
        }
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

void lumberjackPlayGame()
{
    lumv->gameState = LUMBERJACK_GAMESTATE_PLAYING;
    lumberjackSetupLevel(lumv->localPlayerType); 
}

void lumberjackGameReady(void)
{
    lumv->gameReady = true;
}

void lumberjackGameLoop(int64_t elapsedUs)
{

    if (lumv->gameState == LUMBERJACK_GAMESTATE_TITLE)
    {
        lumberjackTitleLoop(elapsedUs);
        return;
    }

    if (lumv->gameState == LUMBERJACK_GAMESTATE_WINNING)
    {
        lumv->levelTime += elapsedUs / 1000;

        if (lumv->ghost != NULL && lumv->ghost->active)
            lumv->ghost->currentFrame = 0;

        if (lumv->levelTime > 3000)
        {
            ESP_LOGI(LUM_TAG, "Next level");
            lumberjackUnloadLevel();

            lumberjackSetupLevel(lumv->localPlayerType);

            lumv->gameState = LUMBERJACK_GAMESTATE_PLAYING;
        }

    }

    baseMode(elapsedUs);

    if (lumv->lumberjackMain->networked && lumv->gameState == LUMBERJACK_GAMESTATE_PLAYING)
    {
        //Check for connection lost
        lumv->wakeupSignal -= elapsedUs/10000;
        lumv->lastResponseSignal -= elapsedUs/10000;

        if (lumv->wakeupSignal < 0)
        {
            lumv->wakeupSignal = 100;
            lumberjackSendScore(lumv->score); //Send score just to ping

        }

        if (lumv->lastResponseSignal <= 0)
        {
            ESP_LOGI(LUM_TAG, "Connection lost!");
            lumv->gameState = LUMBERJACK_GAMESTATE_GAMEOVER;
            lumv->transitionTimer = 500;
            lumv->localPlayer->state = LUMBERJACK_DEAD;
        }
    }

    if (lumv->gameState == LUMBERJACK_GAMESTATE_GAMEOVER)
    {
        lumv->transitionTimer -= elapsedUs/10000;

        if (lumv->transitionTimer <= 0)
        {
            lumv->transitionTimer = 0;       
            switchToSwadgeMode(&lumberjackMode);                 
        }
    }

    if (lumv->gameState == LUMBERJACK_GAMESTATE_PLAYING && lumv->gameType == LUMBERJACK_MODE_ATTACK)
    {
        if (lumv->itemBlockTime > 0 && lumv->itemBlockIndex == -1)
        {
            lumv->itemBlockTime -= elapsedUs/10000;

            if (lumv->itemBlockTime <= 0)
            {
                lumv->itemBlockReady = true;
                lumv->itemBlockTime = 0;
            }
        }
        
    }

    //if panic mode do water
    if (lumv->gameState == LUMBERJACK_GAMESTATE_PLAYING && lumv->gameType == LUMBERJACK_MODE_PANIC)
    {
        if (lumv->itemBlockTime > 0)
        {
            lumv->itemBlockTime -= elapsedUs/10000;
            
            if (lumv->itemBlockTime <= 0)
            {
                lumv->itemBlockReady = true;
                lumv->itemBlockTime = 0;
                //Play note?
            }
        }

        lumv->waterTimer -= elapsedUs/10000;
        if (lumv->waterTimer < 0)
        {
            lumv->waterTimer += lumv->waterSpeed;
            lumv->waterLevel += lumv->waterDirection * (lumv->upgrade + 1);

            if (lumv->waterLevel > lumv->currentMapHeight * LUMBERJACK_TILE_SIZE)
            {
                lumv->waterLevel     = lumv->currentMapHeight * LUMBERJACK_TILE_SIZE;
                lumv->waterTimer     = lumv->waterSpeed * LUMBERJACK_WATER_INCREASE; // Magic number
                lumv->waterDirection = -1;
            }

            if (lumv->waterLevel < 0)
            {
                lumv->waterLevel = 0;
            }
            
        }

    }

    //if attack mode animate item
    if (lumv->gameState == LUMBERJACK_GAMESTATE_PLAYING && lumv->gameType == LUMBERJACK_MODE_ATTACK)
    {
        if (lumv->itemBlockIndex != -1)
        {
            lumv->itemBlockAnimationTime += elapsedUs;

            if (lumv->itemBlockAnimationTime > 80000)
            {
                lumv->itemBlockAnimationTime -= 80000;
                lumv->itemBlockItemAnimation++;
                lumv->itemBlockItemAnimation %= 6;

                lumv->itemBlockItemFrame = (lumv->itemBlockIndex * 6) + lumv->itemBlockItemAnimation;
                
            }
        }
        
    }

    DrawGame();
}

void baseMode(int64_t elapsedUs)
{
    // Ignore the first frame because everything was loading
    // Here we might want to do something like say "On first frame loaded do stuff"
    if (lumv->loaded == false)
    {
        lumv->loaded = true;

        //ESP_LOGI(LUM_TAG, "Load Time %ld", (long)elapsedUs);

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
    if (lumv->gameState == LUMBERJACK_GAMESTATE_PLAYING)
    {
        lumv->levelTime += elapsedUs / 1000;

        if (lumv->comboTime > 0)
        {
            lumv->comboTime -= elapsedUs / 1000;
            
            if (lumv->comboTime < 0)
            {
                lumv->comboTime = 0;
                ESP_LOGI(LUM_TAG, "Combo end! %d", lumv->comboAmount);               


                if (lumv->lumberjackMain->networked && lumv->comboAmount > 0)
                {
                    
                    lumberjackSendAttack(lumv->attackQueue);

                    for (int i = 0; i < ARRAY_SIZE(lumv->attackQueue); i++)
                    {
                        lumv->attackQueue[i] = 0;
                    }
                }

                lumv->comboAmount = 0;
            }

        }

        // Check Controls
        if (lumv->localPlayer->state != LUMBERJACK_DEAD && lumv->localPlayer->state != LUMBERJACK_UNSPAWNED)
        {
            bool attackThisFrame = lumv->localPlayer->attackPressed;
            lumberjackDoControls();

            if (!attackThisFrame && lumv->localPlayer->attackPressed)
            {
                ESP_LOGI(LUM_TAG, "Attack this frame!");
                int x = 0;
                for (int i = 0; i < 32; i ++)
                {
                    if (lumv->enemy[i] != NULL && lumv->enemy[i]->type == 2 && lumv->enemy[i]->queueable)
                    {
                        x++;
                    }
                }
                ESP_LOGI(LUM_TAG, "%d", x);

                //lumberjackSendAttack(lumv->attackQueue);

                if (lumv->gameType == LUMBERJACK_MODE_ATTACK && lumv->itemBlockIndex != -1)
                {
                    if (lumv->itemBlockIndex == 0)
                    {
                        if (lumv->enemy4Count > 0)
                        {
                            lumv->ghost->spawnTime = lumv->ghostSpawnTime ;
                            lumv->ghost->x = 400;                               
                        }

                        for (int i = 0; i < ARRAY_SIZE(lumv->enemy); i++)
                        {
                            lumberjackEntity_t* enemy = lumv->enemy[i];

                            if (enemy == NULL || enemy->state == LUMBERJACK_DEAD) continue;


                            enemy->vy = -20;
                            enemy->onGround = false;
                            if (enemy->state == LUMBERJACK_BUMPED || enemy->state == LUMBERJACK_BUMPED_IDLE)
                            {
                                enemy->state = LUMBERJACK_RUN;
                                enemy->direction = enemy->flipped ? 1 : -1; // Go opposite direction
                                enemy->flipped = !enemy->flipped;
                            }
                            else
                            {
                                enemy->direction = 0;
                                enemy->state = LUMBERJACK_BUMPED;
                            }

                        }
                    }

                    if (lumv->itemBlockIndex == 1)
                    {
                        for (int i = 0; i < ARRAY_SIZE(lumv->enemy); i++)
                        {
                            lumberjackEntity_t* enemy = lumv->enemy[i];

                            if (enemy == NULL || enemy->state == LUMBERJACK_DEAD) continue;

                            if (enemy->active)
                            {
                                enemy->vy = -20;
                                lumberjackUpdateEnemy(enemy, enemy->type + 1);
                            }
                        }

                        if (lumv->lumberjackMain->networked)
                        {
                            lumberjackSendBump();
                        }
                    }

                    if (lumv->itemBlockIndex >= 2)
                    {
                        ESP_LOGI(LUM_TAG, "2" ); //Pear - Invincible
                        lumv->invincibleTimer = LUMBERJACK_INVINCIBLE_TIME;
                    }

                    lumv->itemBlockIndex = -1;
                }
            }
        }

        for (int enemyIndex = 0; enemyIndex < ARRAY_SIZE(lumv->enemy); enemyIndex++)
        {
            if (lumv->enemy[enemyIndex] == NULL)
                continue;

        }

        //Do Ghost Controls
        if (lumv->ghost != NULL && lumv->ghost->active)
        {
            if (lumv->localPlayer->state != LUMBERJACK_DEAD)
            {
                lumv->ghost->x += LUMBERJACK_GHOST_SPEED_NORMAL * -lumv->ghost->startSide *  elapsedUs / 100000.0; 
            }
            else
            {
                lumv->ghost->x += LUMBERJACK_GHOST_SPEED_FAST * -lumv->ghost->startSide *  elapsedUs / 100000.0; 
            }
           
            if (lumv->localPlayer->y < lumv->ghost->y)
            {
                lumv->ghost->y -= 1;

            }
            if (lumv->localPlayer->y > lumv->ghost->y)
            {
                lumv->ghost->y += 1;
            }

        }

        if (lumv->localPlayer->onGround && lumv->hasWon)
        {
            lumv->localPlayer->flipped = 1;
            if (lumv->lumberjackMain->networked)
            {
                lumv->localPlayer->state = LUMBERJACK_VICTORY;
                lumv->gameState = LUMBERJACK_GAMESTATE_GAMEOVER;
                lumv->transitionTimer = 400;
            }
            else
            {
                lumv->gameState          = LUMBERJACK_GAMESTATE_WINNING;
                lumv->localPlayer->state = LUMBERJACK_VICTORY;
                lumv->levelTime          = 0;
                lumv->levelIndex++;
            }


        }
    }

    // Clear cruft
    lumberjackUpdate(elapsedUs);

    // Check spawn
    bool spawnedEnemy = false;
    //if (false == lumv->hasWon) spawnedEnemy = lumberjackSpawnCheck(elapsedUs);
    if (lumv->gameState == LUMBERJACK_GAMESTATE_PLAYING && !lumv->hasWon)
    {
        spawnedEnemy = lumberjackSpawnCheck(elapsedUs);
    }


    if (spawnedEnemy)
    {
        lumv->enemySpawnAmount --;
        if (lumv->enemySpawnAmount <= 0)
        {
            lumv->spawnSide++;
            lumv->spawnSide %= 2;
            if (lumv->gameType == LUMBERJACK_MODE_PANIC)
            {
                lumv->enemySpawnAmount = 1;
            }
            if (lumv->gameType == LUMBERJACK_MODE_ATTACK)
            {
                lumv->enemySpawnAmount = 4;
                lumv->spawnTimer = LUMBERJACK_RESPAWN_PANIC_MIN; 
            }
        }
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

    if (lumv->gameState == LUMBERJACK_GAMESTATE_PLAYING)
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


            if (enemy->direction != 0 && enemy->onGround)
            {

                lumberjackTile_t* facingTile = lumberjackGetTile(enemy->x + (enemy->direction > 0 ? 28 : -8) , enemy->y + 2);

                if (facingTile != NULL && facingTile->type != 0 &&  lumberjackIsCollisionTile(facingTile))
                {
                    enemy->direction = enemy->flipped ? 1 : -1; // Go opposite direction
                    enemy->flipped = !enemy->flipped; //Debug
                    enemy->vx = (enemy->direction == 1) ? enemy->maxVX : -enemy->maxVX;
                }
            }

            lumberjackUpdateEnemyCollision(enemy);
        }
    
        // Player
        if (lumv->localPlayer->ready)
        {
            lumv->localPlayer->respawn -= elapsedUs / 10000;

            if (lumv->localPlayer->respawn <= 0 && lumv->lives > 0)
            {
                ESP_LOGI(LUM_TAG, "RESPAWN PLAYER!");
                // Respawn player
                lumv->localPlayer->respawn = 0;
                lumv->lives--;
                lumv->invincibleTimer = LUMBERJACK_INVINCIBLE_TIME;
                lumberjackRespawn(lumv->localPlayer, lumv->playerSpawnX, lumv->playerSpawnY);
            }
        }
        else if (lumv->localPlayer->state != LUMBERJACK_OFFSCREEN && lumv->localPlayer->state != LUMBERJACK_VICTORY)
        {
            lumberjackUpdateEntity(lumv->localPlayer, elapsedUs);

            //
            for (int enemyIndex = 0; enemyIndex < ARRAY_SIZE(lumv->enemy); enemyIndex++)
            {
                lumberjackEntity_t* enemy = lumv->enemy[enemyIndex];

                if (enemy == NULL || lumv->localPlayer->state == LUMBERJACK_DEAD)
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

                            if (lumv->comboTime > 0)
                            {
                                ESP_LOGI(LUM_TAG, "COMBO!");
                                ++lumv->comboAmount;
                                
                                lumv->activeBonusIndex++;
                                lumv->activeBonusIndex %= ARRAY_SIZE(lumv->bonusDisplay);
                                lumberjackBonus_t* currentBonus = lumv->bonusDisplay[lumv->activeBonusIndex];

                                currentBonus->x = enemy->x;
                                currentBonus->y = enemy->y;
                                currentBonus->active = true;
                                currentBonus->time = 0;
                                currentBonus->bonusAmount = lumv->comboAmount + 1;

                                
                                if (lumv->lumberjackMain->networked)
                                {
                                    lumv->attackQueue[enemy->type]++;
                                }
                            }

                            if (lumv->lumberjackMain->networked)
                            {
                                lumv->attackQueue[enemy->type]++;
                            }

                            lumv->score += enemy->scoreValue * (lumv->comboAmount + 1);

                            if (lumv->lumberjackMain->networked)
                            {
                                lumberjackSendScore(lumv->score);
                            }
                            else
                            {
                                if (lumv->score > lumv->highscore)
                                {
                                    lumv->highscore = lumv->score;
                                }
                            }

                            lumv->comboTime = LUMBERJACK_COMBO_RESET_TIME;


                            //if game mode is single player decide if you're going to clear the level
                            lumv->enemyKillCount ++;

                            if (lumv->enemyKillCount >= lumv->totalEnemyCount)
                            {
                                //And game mode == blah
                                if (lumv->lumberjackMain->networked)
                                {
                                    for (int i = 0; i < ARRAY_SIZE(lumv->enemy); i++)
                                    {
                                        if (lumv->enemy[i] == NULL || lumv->enemy[i]->state != LUMBERJACK_DEAD || lumv->enemy[i]->queueable == false) continue;

                                        lumv->enemy[i]->state = LUMBERJACK_RUN;
                                        if (lumv->enemy[i]->active == false && lumv->enemy[i]->state == LUMBERJACK_OFFSCREEN) 
                                        {
                                            lumberjackResetEnemy(lumv->enemy[i]);
                                            lumberjackUpdateEnemy(lumv->enemy[i], lumv->enemy[i]->type+1);
                                        }
                                    }

                                    lumv->enemyKillCount = 0;

                                    for (int i = 0; i < ARRAY_SIZE(lumv->enemy); i++)
                                    {
                                        if (lumv->enemy[i] == NULL) continue;
                                        if (lumv->enemy[i]->queueable == false)
                                        {
                                            lumv->enemy[i]->queueable = true;
                                            lumv->totalEnemyCount++;
                                            break;
                                        }

                                    }
                                }
                                else
                                {
                                    lumv->hasWon = true;
                                }
                            }

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
                        else if (lumv->invincibleTimer <= 0)
                        {
                            // Kill player
                            // ESP_LOGI(LUM_TAG, "KILL PLAYER");
                            lumberjackOnLocalPlayerDeath();
                        }
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

    //Check invincibility timer
    if (lumv->invincibleTimer > 0)
    {
        lumv->invincibleTimer -= elapsedUs / 1000;
        lumv->invincibleFlicker -= elapsedUs/1000;

        if (lumv->invincibleFlicker <= 0)
        {
            lumv->invincibleFlicker = 250;
            lumv->invincibleFlickerOn = !lumv->invincibleFlickerOn;
        }
    }

    // Update animation
    if (lumv->gameState == LUMBERJACK_GAMESTATE_PLAYING)
    {
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

    if (NULL != lumv->ghost && lumv->ghost->active && 
        lumv->gameState != LUMBERJACK_GAMESTATE_GAMEOVER)
    {
        lumv->ghost->timerFrameUpdate += elapsedUs;
        if (lumv->ghost->timerFrameUpdate > LUMBERJACK_GHOST_ANIMATION)
        {
            lumv->ghost->timerFrameUpdate -= LUMBERJACK_GHOST_ANIMATION;
            lumv->ghost->currentFrame++;
            lumv->ghost->currentFrame %= 4;
        }
    }

    lumv->yOffset = lumv->localPlayer->y - LUMBERJACK_SCREEN_Y_OFFSET;
    if (lumv->yOffset < LUMBERJACK_SCREEN_Y_MIN)
        lumv->yOffset = LUMBERJACK_SCREEN_Y_MIN;

    //
    if (lumv->yOffset > (lumv->currentMapHeight * LUMBERJACK_TILE_SIZE) - TFT_HEIGHT)
        lumv->yOffset = (lumv->currentMapHeight * LUMBERJACK_TILE_SIZE) - TFT_HEIGHT;


    for (int i = 0; i < ARRAY_SIZE(lumv->bonusDisplay); i++)
    {
        lumberjackBonus_t* currentBonus = lumv->bonusDisplay[i];
        if (false == currentBonus->active ) continue;

        currentBonus->time += elapsedUs / 1000;
        if (currentBonus->time > LUMBERJACK_BONUS_DISPLAYDURATI)
        {
            currentBonus->active = false;
        }
    }

}

void lumberjackOnLocalPlayerDeath(void)
{
    lumv->localPlayer->state     = LUMBERJACK_DEAD;
    lumv->localPlayer->vy        = -20;
    lumv->localPlayer->active    = false;
    lumv->localPlayer->jumping   = false;
    lumv->localPlayer->jumpTimer = 0;
    lumv->comboAmount            = 0;
    lumv->comboTime              = 0;

    if (lumv->lives <= 0)
    {
        if (lumv->ghost != NULL && lumv->ghost->active)
            lumv->ghost->currentFrame = 0;

        lumv->gameState = LUMBERJACK_GAMESTATE_GAMEOVER;
        lumv->transitionTimer = 400;
    }

    if (lumv->lumberjackMain->networked)
    {
        lumberjackSendDeath(lumv->lives <= 0);

    }
}



void DrawTitle(void)
{
    drawWsgSimple(&lumv->title, (TFT_WIDTH / 2) - 51, (TFT_HEIGHT / 2) - 48);
    
    if (lumv->localPlayerType == 0)
    {
        drawWsgSimple(&lumv->subtitle_red, (TFT_WIDTH/2)- 36, (TFT_HEIGHT/2) -9);
    }
    else if (lumv->localPlayerType == 1)
    {
        drawWsgSimple(&lumv->subtitle_green, (TFT_WIDTH/2)- 36, (TFT_HEIGHT/2) -9);
    }
    else
    {
        drawWsgSimple(&lumv->subtitle_white, (TFT_WIDTH/2)- 36, (TFT_HEIGHT/2) -9);
        
    }
    
    for (int i = 0; i < LUMBERJACK_MAP_WIDTH; i++)
    {
        drawWsgSimple(&lumv->floorTiles[1], i * LUMBERJACK_TILE_SIZE, 208);
        drawWsgSimple(&lumv->floorTiles[7], i * LUMBERJACK_TILE_SIZE, 224);
    }

    if (lumv->gameReady)
    {
        //Press A To Start
        lumberjackTitleDisplayText(((lumv->lumberjackMain->host || !lumv->lumberjackMain->networked ) ? "Press A to start": "Host must start"), (TFT_WIDTH/2) - 70, 180);
    }
    else if (lumv->lumberjackMain->networked && lumv->lumberjackMain->host)
    {
        lumberjackTitleDisplayText("Waiting for client", (TFT_WIDTH/2) - 80, 180);
    }
    else if (lumv->lumberjackMain->networked && !lumv->lumberjackMain->host)
    {
        lumberjackTitleDisplayText("Looking for host", (TFT_WIDTH/2) - 80, 180);
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
        if (lumv->gameType == LUMBERJACK_MODE_PANIC)
        {
            lumv->waterDirection = LUMBERJACK_WATER_SLOW_DRAIN;
            lumv->waterTimer = lumv->waterSpeed;
            lumv->waterLevel += lumv->waterDirection;
        }

    }

    if (lumv->localPlayer->submergedTimer != LUMBERJACK_SUBMERGE_TIMER && ((lumv->localPlayer->submergedTimer/10) % 3) != 0)
    {
        drawWsg(&lumv->alertSprite, lumv->localPlayer->x + 6, lumv->localPlayer->y - 26 - lumv->yOffset, false, false, 0);
    }

    lumv->localPlayer->drawFrame = currentFrame;

    // This is where it breaks. When it tries to play frame 3 or 4 it crashes.
    if (lumv->invincibleTimer <= 0 || !lumv->invincibleFlickerOn)
    {


    drawWsg(&lumv->playerSprites[lumv->localPlayer->drawFrame], lumv->localPlayer->x - 4,
            lumv->localPlayer->y - lumv->yOffset, lumv->localPlayer->flipped, false, 0);
    }
    else
    {

    drawSolidWsg(&lumv->playerSprites[lumv->localPlayer->drawFrame], lumv->localPlayer->x - 4,
            lumv->localPlayer->y - lumv->yOffset, lumv->localPlayer->flipped, false, lumv->invincibleColor);
    }
    

    if (lumv->localPlayer->x > LUMBERJACK_SCREEN_X_MAX)
    {
        drawWsg(&lumv->playerSprites[lumv->localPlayer->drawFrame], lumv->localPlayer->x - LUMBERJACK_SCREEN_X_OFFSET,
                lumv->localPlayer->y - lumv->yOffset, lumv->localPlayer->flipped, false, 0);
    }

    //This is where we draw the ghost
    if (NULL != lumv->ghost && true == lumv->ghost->active && lumv->ghost->currentFrame % 2  == 0)
    {
        if (lumv->ghost->currentFrame == 2)
            drawWsg(&lumv->enemySprites[21], lumv->ghost->x, lumv->ghost->y  - lumv->yOffset, (lumv->ghost->startSide == 1), false, 0);
        else
            drawWsg(&lumv->enemySprites[22], lumv->ghost->x, lumv->ghost->y  - lumv->yOffset, (lumv->ghost->startSide == 1), false, 0);
    }


    if (lumv->gameType == LUMBERJACK_MODE_ATTACK)
    {
        drawWsgSimple(&lumv->ui[0], (TFT_WIDTH / 2) - 12, 6);

        if (lumv->itemBlockIndex != -1)
        {
            drawWsgSimple(&lumv->itemIcons[lumv->itemBlockItemFrame], (TFT_WIDTH / 2) - 8, 10);
        }
    }
    //If playing panic mode draw water

    lumberjackDrawWaterLevel();

    for (int i = 0; i < lumv->lives; i++)
    {
        int icon = lumv->localPlayerType;

        if (icon > ARRAY_SIZE(lumv->minicharacters))
        {
            icon = 0;
        }
        drawWsgSimple(&lumv->minicharacters[icon], (i * 14) + 22, 32);
    }

    if (lumv->gameState == LUMBERJACK_GAMESTATE_GAMEOVER)
    {
        if (lumv->lumberjackMain->networked && lumv->lastResponseSignal <= 0)
        {
            
            drawWsgSimple(&lumv->connectLostSprite, (TFT_WIDTH/2) -72, (TFT_HEIGHT/2) - 9);

        } else if (lumv->hasWon)
        {
            ESP_LOGI(LUM_TAG, "%d lrs", lumv->lastResponseSignal);
            drawWsgSimple(&lumv->gamewinSprite, (TFT_WIDTH/2) -72, (TFT_HEIGHT/2) - 9);
        }
        else
        {
            drawWsgSimple(&lumv->gameoverSprite, (TFT_WIDTH/2) -72, (TFT_HEIGHT/2) - 9);
        }
    }

    //
    lumberjackScoreDisplay(lumv->score, 26);
    lumberjackScoreDisplay(lumv->highscore, 206);

    if (lumv->gameState == LUMBERJACK_GAMESTATE_PLAYING && lumv->levelTime < 5000)
    {
        char level_display[20] = {0};

        if (lumv->levelIndex < 9)
        {
            snprintf(level_display, sizeof(level_display), "Level 0%d", (lumv->levelIndex + 1));
        }
        else
        {
            snprintf(level_display, sizeof(level_display), "Level %d", (lumv->levelIndex + 1));
        }

        drawText(&lumv->arcade, c000, level_display, (TFT_WIDTH/2) - 36, TFT_HEIGHT - 18);
        drawText(&lumv->arcade, c000, level_display, (TFT_WIDTH/2) - 36, TFT_HEIGHT - 21);
        drawText(&lumv->arcade, c000, level_display, (TFT_WIDTH/2) - 36 - 1, TFT_HEIGHT - 20);
        drawText(&lumv->arcade, c000, level_display, (TFT_WIDTH/2) - 36 + 1, TFT_HEIGHT - 20);
        drawText(&lumv->arcade, c555, level_display, (TFT_WIDTH/2) - 36, TFT_HEIGHT - 20);
    }

    for (int i = 0; i < ARRAY_SIZE(lumv->bonusDisplay); i++)
    {
        lumberjackBonus_t* currentBonus = lumv->bonusDisplay[i];
        if (false == currentBonus->active ) continue;

        float offsetTime = (((float)currentBonus->time)/LUMBERJACK_BONUS_DISPLAYDURATI) + .9f;

        if (offsetTime > 1) offsetTime = 1;

        drawWsgSimple(&lumv->bonusSprites[10], currentBonus->x, currentBonus->y - lumv->yOffset - (20 - (20 * offsetTime)));
        if (currentBonus->bonusAmount < 10)
        {
            drawWsgSimple(&lumv->bonusSprites[currentBonus->bonusAmount], currentBonus->x+8, currentBonus->y - lumv->yOffset - (20 - (20 * offsetTime)));
        }
        else
        {
            int tenDigit = (int)(currentBonus->bonusAmount / 10);
            if (tenDigit > 9) tenDigit = 9;
            int oneDigit = currentBonus->bonusAmount  % 10;

            drawWsgSimple(&lumv->bonusSprites[tenDigit], currentBonus->x+8, currentBonus->y - lumv->yOffset - (20 - (20 * offsetTime)));
            drawWsgSimple(&lumv->bonusSprites[oneDigit], currentBonus->x+16, currentBonus->y - lumv->yOffset - (20 - (20 * offsetTime)));



        }

        //currentBonus->time += ;
        //currentBonus->bonusAmount = lumv->comboAmount;
    }
    

    //drawRect()
    // Debug

    /*
    char debug[20] = {0};
    snprintf(debug, sizeof(debug), "Debug: %d %d %d", lumv->localPlayer->x, lumv->localPlayer->y,
             lumv->localPlayer->cH);

    drawText(&lumv->arcade, c000, debug, 16, 16);

    drawRect(lumv->localPlayer->cX, lumv->localPlayer->cY - lumv->yOffset, lumv->localPlayer->cX +
     lumv->localPlayer->cW, lumv->localPlayer->cY - lumv->yOffset + lumv->localPlayer->cH, c050);

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

    if (lumv->localPlayer->onGround)
    {
        drawText(&lumv->arcade, c555, "G", 80, 32);
    }
    else
    {
        drawText(&lumv->arcade, c000, "G", 80, 32);
    } //lumv->spawnTimer

    snprintf(debug, sizeof(debug), "%d", lumv->spawnTimer);
    drawText(&lumv->arcade, c555, debug,  122, 32);
    */
}

void lumberjackTitleDisplayText(char* string, int locationX, int locationY)
{

    drawText(&lumv->arcade, c000, string, locationX, locationY + 2);
    drawText(&lumv->arcade, c000, string, locationX, locationY - 1);
    drawText(&lumv->arcade, c000, string, locationX - 1, locationY);
    drawText(&lumv->arcade, c000, string, locationX + 1, locationY);
    drawText(&lumv->arcade, c555, string, locationX, locationY);
}

void lumberjackScoreDisplay(int score, int locationX)
{
    char score_display[20] = {0};

    if (score > 0)
    {
        snprintf(score_display, sizeof(score_display), "%d", score);
    }
    else
    {
        snprintf(score_display, sizeof(score_display), "000");
    }

    drawText(&lumv->arcade, c000, score_display, locationX, 18);
    drawText(&lumv->arcade, c000, score_display, locationX, 15);
    drawText(&lumv->arcade, c000, score_display, locationX - 1, 16);
    drawText(&lumv->arcade, c000, score_display, locationX + 1, 16);
    drawText(&lumv->arcade, c555, score_display, locationX, 16);
}

bool lumberjackSpawnCheck(int64_t elapseUs)
{
    float elapse = (elapseUs / 1000);
    if (lumv->ghost != NULL && lumv->ghost->active == false)
    {
        lumv->ghost->spawnTime -= elapse;

        if (lumv->ghost->spawnTime <= 0)
        {
            lumv->ghost->spawnTime = lumv->ghostSpawnTime;

            lumv->ghost->active = true;
            
            if (esp_random() % 2 > 0)
            {
                lumv->ghost->startSide = -1;
                lumv->ghost->x = -5;
            }
            else
            {
                lumv->ghost->startSide = 1;
                lumv->ghost->x = 280;
            }

            lumv->ghost->y = lumv->localPlayer->y;

            if (esp_random() % 2 > 0)
            {
                lumv->ghost->y -= 32;
            }
            else
            {
                lumv->ghost->y += 32;
            }
        }

    }

    bool spawnReady = false;
    if (lumv->spawnTimer >= 0)
    {

        lumv->spawnTimer -= elapse;

        if (lumv->spawnTimer < 0)
        {
            for (int enemyIndex = 0; enemyIndex < ARRAY_SIZE(lumv->enemy); enemyIndex++)
            {
                if (lumv->enemy[enemyIndex] == NULL || lumv->enemy[enemyIndex]->queueable == false)
                    continue;

                if (lumv->enemy[enemyIndex]->ready && lumv->enemy[enemyIndex]->state != LUMBERJACK_DEAD)
                {

                    //lumv->spawnTimer = LUMBERJACK_RESPAWN_TIMER - (lumv->upgrade * LUMBERJACK_UPGRADE_TIMER_OFFSET);
                    lumv->spawnTimer = lumv->enemySpawnTime;

                    /*
                    if (lumv->spawnTimer < lumv->enemySpawnTime)
                    {

                        if ((lumv->lumberjackMain->networked && lumv->enemyAttackQueue > 0))
                        {
                            lumv->spawnTimer = lumv->enemyReceiveSpawnTime;

                            if (lumv->enemyAttackQueue > 0)
                            {
                                lumv->enemyAttackQueue--;
                            }
                        }
                        else
                        {
                            lumv->spawnTimer = LUMBERJACK_RESPAWN_PANIC_MIN;
                        }
                    }*/

                    lumberjackRespawnEnemy(lumv->enemy[enemyIndex], lumv->spawnSide);
                    spawnReady = true;
                    break;
                }
            }

            if (!spawnReady)
            {
                // No one was spawned.
                lumv->spawnTimer = 500;
            }
        }
    }

    return spawnReady;
}

void lumberjackQualityCheck()
{
    lumv->wakeupSignal = 100;
    lumv->lastResponseSignal = 1000;
}

void lumberjackOnReceiveAttack(const uint8_t* attack, int len)
{
    ESP_LOGI(LUM_TAG, "Being attacked %ld", sizeof(attack));
    if (lumv->gameType == LUMBERJACK_MODE_PANIC)
    {

        lumv->waterDirection = -1; //Even if it is draining
        lumv->waterSpeed -= 5;
        if (lumv->waterSpeed < 0) 
        {
            lumv->waterSpeed = 0;
        }
    }

    if (lumv->gameType == LUMBERJACK_MODE_ATTACK)
    {
        for (int i = 1; i < len; i++)
        {
            ESP_LOGI(LUM_TAG, "Attacky! %d) %d", i, attack[i]);
            int rem = attack[i];
            for (int n = 0; n < ARRAY_SIZE(lumv->enemy); n++)
            {
                if (rem <= 0) break;

                if (lumv->enemy[n] == NULL || lumv->enemy[n]->queueable) continue;

                if (lumv->enemy[n]->type  == i - 1)
                {
                    lumv->enemy[n]->queueable = true;
                    rem--;
                }
            }
        }

        lumv->totalEnemyCount = 0;
        for (int i = 0; i < ARRAY_SIZE(lumv->enemy); i++)
        {
            if (lumv->enemy[i] == NULL) continue;

            if (lumv->enemy[i]->queueable)
            {
                lumv->totalEnemyCount++;
            }
        }
    }
}

void lumberjackOnReceiveScore(const uint8_t* score)
{
    int locX      = (int)score[1] << 0 | (uint32_t)score[2] << 8 | (uint32_t)score[3] << 16;
    ESP_LOGI(LUM_TAG, "SCORE: %d", locX);
    lumv->highscore = locX;
}

void lumberjackOnReceiveBump(void)
{
    ESP_LOGI(LUM_TAG, "BUMP!");
    if (lumv->localPlayer->onGround)
    {
        lumv->localPlayer->vy = -20; //HERE

        if (lumv->localPlayer->state == LUMBERJACK_DUCK)
        {
            lumv->localPlayer->vy = -40;
            lumv->localPlayer->state = LUMBERJACK_BUMPED;
            lumv->invincibleTimer = 0;

            if (lumv->ghost != NULL && lumv->ghost->active)
            {
                if (lumv->localPlayer->x < lumv->ghost->x + LUMBERJACK_GHOST_BOX && lumv->localPlayer->x + lumv->localPlayer->width > lumv->ghost->x
                && lumv->localPlayer->y < lumv->ghost->y + LUMBERJACK_GHOST_BOX && lumv->localPlayer->y + lumv->localPlayer->height > lumv->ghost->y
                )
                {
                    lumberjackOnLocalPlayerDeath();
                }

            }
        }

        
    }

    lumv->itemBlockIndex = -1;
}

void lumberjackOnReceiveDeath(bool gameover)
{
    ESP_LOGI(LUM_TAG, "Player died!  Haha %d", (gameover));

    if (gameover)
    {
        lumv->hasWon = true;

        for (int i = 0; i < ARRAY_SIZE(lumv->enemy); i++ )
        {
            if (lumv->enemy[i] == NULL) continue;

            lumv->enemy[i]->state = LUMBERJACK_DEAD;
        }

    }
}

void lumberjackAttackCheck(int64_t elapseUs)
{
}

static void lumberjackUpdateEntity(lumberjackEntity_t* entity, int64_t elapsedUs)
{
    bool onGround = false;

    // World wrap
    entity->x %= LUMBERJACK_WORLD_WRAP;
    if (entity->x < -20)
    {
        entity->x += LUMBERJACK_WORLD_WRAP;
    }

    if (entity->state == LUMBERJACK_BUMPED)
    {
        entity->vy -= 2;
        entity->respawn = 1500 - (lumv->upgrade * LUMBERJACK_UPGRADE_TIMER_OFFSET);

        if (entity->respawn < LUMBERJACK_UPGRADE_TIMER_OFFSET)
        {
            entity->respawn = LUMBERJACK_UPGRADE_TIMER_OFFSET;
        }



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

        if ((tileA != NULL && lumberjackIsCollisionTile(tileA->type)) || (tileB != NULL && lumberjackIsCollisionTile(tileB->type)))
        {
            destinationY      = ((tileA->y + 1) * LUMBERJACK_TILE_SIZE);
            entity->jumpTimer = 0;
            entity->jumping   = false;
            entity->vy        = 0;

            if (tileA != NULL && lumberjackIsCollisionTile(tileA->type))
            {
                if (tileA->type != 11 || lumv->itemBlockReady == true)
                {
                    //ESP_LOGI(LUM_TAG, "BUMP A %d", tileA->index);
                    lumv->tile[tileA->index].offset      = 10;
                    lumv->tile[tileA->index].offset_time = 100;

                
                    if (tileA->type == 12)
                    {
                        lumv->tile[tileA->index].type = 13;
                        lumv->tile[tileA->index].offset      *= 10;
                        lumv->tile[tileA->index].offset_time *= 10;
                    }

                    if (lumv->tile[tileA->index].type != 13) lumberjackDetectBump(tileA);
                    if (tileA->type == 11)
                    {
                        lumberjackUseBlock();
                    }
                
                }

            }

            if (tileB!= NULL && lumberjackIsCollisionTile(tileB->type))
            {
                
                if (tileB->type != 11 || lumv->itemBlockReady == true)
                {


                    lumv->tile[tileB->index].offset      = 10;
                    lumv->tile[tileB->index].offset_time = 100;
                    if (tileB->type == 12)
                    {
                        lumv->tile[tileB->index].type = 13;
                        lumv->tile[tileB->index].offset      *= 10;
                        lumv->tile[tileB->index].offset_time *= 10;
                    }

                    if (lumv->tile[tileB->index].type != 13) lumberjackDetectBump(tileB);
                    if (tileB->type == 11)
                    {
                        lumberjackUseBlock();                        
                    }
                }

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
            destinationY = ((tileA->y - entity->tileHeight) * LUMBERJACK_TILE_SIZE);
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

    if (entity->y > lumv->currentMapHeight * LUMBERJACK_TILE_SIZE)
    {
        entity->y      = lumv->currentMapHeight * LUMBERJACK_TILE_SIZE;
        entity->active = false;
        if (entity->state == LUMBERJACK_DEAD)
        {
            //
            if (entity == lumv->localPlayer)
            {
                if (entity->respawn == 0)
                {
                    // ESP_LOGI(LUM_TAG, "DEAD & hit the ground %d", entity->respawn);
                    entity->respawn = LUMBERJACK_UPGRADE_TIMER_OFFSET;
                    entity->ready   = true;
                    return;
                }
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
        if (entity->state != LUMBERJACK_DEAD && !lumv->hasWon)
            entity->state = LUMBERJACK_OFFSCREEN;

        if (entity->state != LUMBERJACK_DEAD && entity->state != LUMBERJACK_OFFSCREEN && lumv->hasWon)
        {
            lumv->gameState          = LUMBERJACK_GAMESTATE_WINNING;
            lumv->localPlayer->state = LUMBERJACK_VICTORY;
            lumv->levelTime          = 0;
            lumv->levelIndex++;
        }
    }

    if (entity == lumv->localPlayer && lumv->ghost != NULL && lumv->ghost->active)
    {
        if (lumv->localPlayer->x < lumv->ghost->x + LUMBERJACK_GHOST_BOX && lumv->localPlayer->x + lumv->localPlayer->width > lumv->ghost->x
        && lumv->localPlayer->y < lumv->ghost->y + LUMBERJACK_GHOST_BOX && lumv->localPlayer->y + lumv->localPlayer->height > lumv->ghost->y
        && lumv->localPlayer->state != LUMBERJACK_DEAD && lumv->localPlayer->state != LUMBERJACK_DUCK && lumv->invincibleTimer <= 0)
        {
            lumberjackOnLocalPlayerDeath();
        }

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

                if (lumv->tile[tileIndex].type == 13 && lumv->tile[tileIndex].offset <= 0)
                {
                    lumv->tile[tileIndex].type = 12;
                }
            }
        }
    }

    if (lumv->ghost != NULL && lumv->ghost->active)
    {
        if ((lumv->ghost->x < -28 && lumv->ghost->startSide > 0) || (lumv->ghost->x > 290 && lumv->ghost->startSide < 0 ))
        {
            ESP_LOGI(LUM_TAG, "Remove ghost!");
            lumv->ghost->active = false;
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

            if ((y * LUMBERJACK_TILE_SIZE) - lumv->yOffset >= -16)
            {
                if (tileIndex > 0 && tileIndex < 15)
                {
                    if (tileIndex < 11)
                    {
                        drawWsgSimple(&lumv->floorTiles[tileIndex - 1], x * LUMBERJACK_TILE_SIZE, (y * LUMBERJACK_TILE_SIZE) - lumv->yOffset - offset);
                    }
                    else if (tileIndex == 11)
                    {
                        if (lumv->itemBlockReady)
                        {
                            drawWsgSimple(&lumv->unusedBlockSprite[lumv->stageAnimationFrame % LUMBERJACK_BLOCK_ANIMATION_MAX], x * LUMBERJACK_TILE_SIZE, (y * LUMBERJACK_TILE_SIZE) - lumv->yOffset - offset);
                        }
                        else
                        {
                            drawWsgSimple(&lumv->floorTiles[tileIndex - 1], x * LUMBERJACK_TILE_SIZE, (y * LUMBERJACK_TILE_SIZE) - lumv->yOffset - offset);
                        }
                    }
                    else if (tileIndex == 12)
                    {
                        drawWsgSimple(&lumv->animationTiles[12], x * LUMBERJACK_TILE_SIZE, (y * LUMBERJACK_TILE_SIZE) - lumv->yOffset);
                    }
                    else if (tileIndex == 13)
                    {
                        drawWsgSimple(&lumv->animationTiles[12 + (offset % LUMBERJACK_ROTATE_ANIMATION_MAX)], x * LUMBERJACK_TILE_SIZE, (y * LUMBERJACK_TILE_SIZE) - lumv->yOffset);
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
        drawWsgSimple(&lumv->animationTiles[lumv->liquidAnimationFrame], i * LUMBERJACK_TILE_SIZE, lumv->waterLevel - lumv->yOffset);
    }

    fillDisplayArea(0, lumv->waterLevel + 16 - lumv->yOffset, TFT_WIDTH, ((lumv->currentMapHeight+1) * LUMBERJACK_TILE_SIZE)- lumv->yOffset , c134);

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
    int tx = (int)x / LUMBERJACK_TILE_SIZE;
    int ty = (int)y / LUMBERJACK_TILE_SIZE;

    if (tx < 0)
        tx = 17;
    if (tx > 17)
        tx = 0;

    if (ty < 0)
    {
        return NULL;
    }
    if (ty >= lumv->currentMapHeight)
    {
        return NULL;
    }   

    return &lumv->tile[(ty * LUMBERJACK_MAP_WIDTH) + tx];

}

static bool lumberjackIsCollisionTile(int index)
{
    if (index == 0 || index == 1 || index == 6 || index == 5 || index == 10 || index == 13)
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
            int tx  = ((enemy->x - 8) / LUMBERJACK_TILE_SIZE);
            int ty  = ((enemy->y) / LUMBERJACK_TILE_SIZE) + 1;
            int tx2 = ((enemy->x + 8) / LUMBERJACK_TILE_SIZE);

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
                    enemy->direction = enemy->flipped ? 1 : -1; // Go opposite direction
                    enemy->flipped = !enemy->flipped;
                }
                else
                {
                    enemy->direction = 0;
                    enemy->state     = LUMBERJACK_BUMPED;
            
                    if (enemy->type == 4)
                    {
                        lumberjackUpdateEnemy(enemy, enemy->type + 1);
                        enemy->state = LUMBERJACK_RUN;
                        enemy->direction = enemy->flipped ? 1 : -1; // Go opposite direction
                        enemy->flipped = !enemy->flipped;
                    }
                }
            }
        }
    }
}

void lumberjackUseBlock()
{
    lumv->itemBlockReady = false;
    ESP_LOGI(LUM_TAG, "JACKED");

    if (lumv->gameType == LUMBERJACK_MODE_PANIC)
    {
        
        lumv->waterDirection = LUMBERJACK_WATER_FAST_DRAIN;
        lumv->waterTimer = lumv->waterSpeed;
        lumv->waterLevel += lumv->waterDirection;

        lumv->itemBlockTime = LUMBERJACK_ITEM_RESETTIME;

        if (lumv->lumberjackMain->networked)
        {
            //Increase speed of opponent's water!
            lumberjackSendAttack(lumv->attackQueue);

            for (int i = 0; i < ARRAY_SIZE(lumv->attackQueue); i++)
            {
                lumv->attackQueue[i] = 0;
            }
        }
    }

    if (lumv->gameType == LUMBERJACK_MODE_ATTACK)
    {
        lumv->itemBlockIndex = esp_random() % 3;
        lumv->itemBlockTime = LUMBERJACK_ITEM_RESETTIME;

        if (lumv->itemBlockIndex == 0)
        {

        }
        else if (lumv->itemBlockIndex == 1)
        {

        }
        else {

        }

    }
}

/// CODE ++

/**
 * @brief Draw solid color of a WSG to the display
 *
 * @param wsg  The WSG to draw to the display
 * @param xOff The x offset to draw the WSG at
 * @param yOff The y offset to draw the WSG at
 * @param flipLR true to flip the image across the Y axis
 * @param flipUD true to flip the image across the X axis
 * @param color The color we're sending instead
 */
void drawSolidWsg(const wsg_t* wsg, int16_t xOff, int16_t yOff, bool flipLR, bool flipUD, uint8_t inColor)
{

    if (NULL == wsg->px)
    {
        return;
    }

    // Draw the image's pixels (no rotation or transformation)
    uint32_t w         = TFT_WIDTH;
    paletteColor_t* px = getPxTftFramebuffer();

    uint16_t wsgw = wsg->w;
    uint16_t wsgh = wsg->h;

    int32_t xstart = 0;
    int16_t xend   = wsgw;
    int32_t xinc   = 1;

    // Reflect over Y axis?
    if (flipLR)
    {
        xstart = wsgw - 1;
        xend   = -1;
        xinc   = -1;
    }

    if (xOff < 0)
    {
        if (xinc > 0)
        {
            xstart -= xOff;
            if (xstart >= xend)
            {
                return;
            }
        }
        else
        {
            xstart += xOff;
            if (xend >= xstart)
            {
                return;
            }
        }
        xOff = 0;
    }

    if (xOff + wsgw > w)
    {
        int32_t peelBack = (xOff + wsgw) - w;
        if (xinc > 0)
        {
            xend -= peelBack;
            if (xstart >= xend)
            {
                return;
            }
        }
        else
        {
            xend += peelBack;
            if (xend >= xstart)
            {
                return;
            }
        }
    }

    for (int16_t srcY = 0; srcY < wsgh; srcY++)
    {
        int32_t usey = srcY;

        // Reflect over X axis?
        if (flipUD)
        {
            usey = wsgh - 1 - usey;
        }

        const paletteColor_t* linein = &wsg->px[usey * wsgw];

        // Transform this pixel's draw location as necessary
        uint32_t dstY = srcY + yOff;

        // It is too complicated to detect both directions and backoff correctly, so we just do this here.
        // It does slow things down a "tiny" bit.  People in the future could optimze out this check.
        if (dstY >= TFT_HEIGHT)
        {
            continue;
        }

        int32_t lineOffset = dstY * w;
        int32_t dstx       = xOff + lineOffset;

        for (int32_t srcX = xstart; srcX != xend; srcX += xinc)
        {
            // Draw if not transparent
            uint8_t color = linein[srcX];
            if (cTransparent != color)
            {
                px[dstx] = inColor;
            }
            dstx++;
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

    if (lumv->lumberjackMain->networked) 
    {
     freeWsg(&lumv->gamewinSprite);
     freeWsg(&lumv->connectLostSprite);

    }   
    

    // Free the bonus
    for (int i = 0; i < ARRAY_SIZE(lumv->bonusDisplay); i++)
    {
        freeWsg(&lumv->bonusDisplay[i]);
    }

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

    for (int i = 0; i < ARRAY_SIZE(lumv->axeBlocks); i++)
    {
        free(lumv->axeBlocks[i]);
    }


    if (lumv->gameType == LUMBERJACK_MODE_ATTACK)
    {
        for (int i = 0; i < ARRAY_SIZE(lumv->itemIcons); i++)
        {
            freeWsg(&lumv->itemIcons[i]);
        }
    }

    freeFont(&lumv->arcade);
    free(lumv);
}