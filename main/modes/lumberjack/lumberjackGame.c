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

static lumberjackTile_t* lumberjackGetTile(int x, int y);
static void lumberjackUpdateEntity(lumberjackEntity_t* entity, int64_t elapsedUs);
static bool lumberjackIsCollisionTile(int index);

lumberjackVars_t* lumv;
lumberjackTile_t lumberjackCollisionDebugTiles[32] = {};


void lumberjackStartGameMode(lumberjackGameType_t type)
{
    lumv = calloc(1, sizeof(lumberjackVars_t));
    loadFont("ibm_vga8.font", &lumv->ibm, false);

    bzrStop(); // Stop the buzzer?

    lumv->worldTimer = 0;
    lumv->liquidAnimationFrame = 0;
    lumv->loaded = false;
    lumv->gameType = type;

    ESP_LOGI(LUM_TAG, "Loading floor Tiles");
    loadWsg("bottom_floor1.wsg", &lumv->floorTiles[0], false);
    loadWsg("bottom_floor2.wsg", &lumv->floorTiles[1], false);
    loadWsg("bottom_floor3.wsg", &lumv->floorTiles[2], false);
    loadWsg("bottom_floor4.wsg", &lumv->floorTiles[3], false);
    loadWsg("bottom_floor5.wsg", &lumv->floorTiles[4], false);
    loadWsg("bottom_floor6.wsg", &lumv->floorTiles[5], false);
    loadWsg("bottom_floor7.wsg", &lumv->floorTiles[6], false);
    loadWsg("bottom_floor8.wsg", &lumv->floorTiles[7], false);
    loadWsg("bottom_floor9.wsg", &lumv->floorTiles[8], false);
    loadWsg("bottom_floor10.wsg", &lumv->floorTiles[9], false);
    ESP_LOGI(LUM_TAG, "Loading Animation Tiles");

    loadWsg("water_floor1.wsg", &lumv->animationTiles[0], false);
    loadWsg("water_floor2.wsg", &lumv->animationTiles[1], false);
    loadWsg("water_floor3.wsg", &lumv->animationTiles[2], false);
    loadWsg("water_floor4.wsg", &lumv->animationTiles[3], false);
    loadWsg("water_floor0.wsg", &lumv->animationTiles[4], false);
    loadWsg("water_floor0.wsg", &lumv->animationTiles[5], false);
    loadWsg("water_floor0.wsg", &lumv->animationTiles[6], false);
    loadWsg("water_floor0.wsg", &lumv->animationTiles[7], false);
    loadWsg("water_floor_b1.wsg", &lumv->animationTiles[8], false);
    loadWsg("water_floor_b2.wsg", &lumv->animationTiles[9], false);
    loadWsg("water_floor_b3.wsg", &lumv->animationTiles[10], false);
    loadWsg("water_floor_b4.wsg", &lumv->animationTiles[11], false);
    ESP_LOGI(LUM_TAG, "Loading Characters");
    
    loadWsg("lumbers_red_1.wsg", &lumv->playerSprites[0], true);
    loadWsg("lumbers_red_2.wsg", &lumv->playerSprites[1], true);
    loadWsg("lumbers_red_3.wsg", &lumv->playerSprites[2], true);
    loadWsg("lumbers_red_4.wsg", &lumv->playerSprites[3], false); // These two things break 3 seconds after the game loads
    loadWsg("lumbers_red_5.wsg", &lumv->playerSprites[4], false); // I think the memory is being replaces
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
    
    loadWsg("lumbers_green_1.wsg", &lumv->playerSprites[17], true);
    loadWsg("lumbers_green_2.wsg", &lumv->playerSprites[18], true);
    loadWsg("lumbers_green_3.wsg", &lumv->playerSprites[19], true);
    loadWsg("lumbers_green_4.wsg", &lumv->playerSprites[20], true);
    loadWsg("lumbers_green_5.wsg", &lumv->playerSprites[21], true);
    loadWsg("lumbers_green_6.wsg", &lumv->playerSprites[22], true);
    loadWsg("lumbers_green_7.wsg", &lumv->playerSprites[23], true);
    loadWsg("lumbers_green_8.wsg", &lumv->playerSprites[24], true);
    loadWsg("lumbers_green_9.wsg", &lumv->playerSprites[25], true);
    loadWsg("lumbers_green_10.wsg", &lumv->playerSprites[26], true);
    loadWsg("lumbers_green_11.wsg", &lumv->playerSprites[27], true);
    loadWsg("lumbers_green_12.wsg", &lumv->playerSprites[28], true);
    loadWsg("lumbers_green_13.wsg", &lumv->playerSprites[29], true);
    loadWsg("lumbers_green_14.wsg", &lumv->playerSprites[30], true);
    loadWsg("lumbers_green_15.wsg", &lumv->playerSprites[31], true);
    loadWsg("lumbers_green_16.wsg", &lumv->playerSprites[32], true);
    loadWsg("lumbers_green_17.wsg", &lumv->playerSprites[33], true);
    /*
    loadWsg("secret_swadgeland_1.wsg", &lumv->playerSprites[34], true);
    loadWsg("secret_swadgeland_2.wsg", &lumv->playerSprites[35], true);
    loadWsg("secret_swadgeland_3.wsg", &lumv->playerSprites[36], true);
    loadWsg("secret_swadgeland_4.wsg", &lumv->playerSprites[37], true);
    loadWsg("secret_swadgeland_5.wsg", &lumv->playerSprites[38], true);
    loadWsg("secret_swadgeland_6.wsg", &lumv->playerSprites[39], true);
    loadWsg("secret_swadgeland_7.wsg", &lumv->playerSprites[40], true);
    loadWsg("secret_swadgeland_8.wsg", &lumv->playerSprites[41], true);
    loadWsg("secret_swadgeland_9.wsg", &lumv->playerSprites[42], true);
    loadWsg("secret_swadgeland_10.wsg", &lumv->playerSprites[43], true);
    loadWsg("secret_swadgeland_11.wsg", &lumv->playerSprites[44], true);
    loadWsg("secret_swadgeland_12.wsg", &lumv->playerSprites[45], true);
    loadWsg("secret_swadgeland_13.wsg", &lumv->playerSprites[46], true);
    loadWsg("secret_swadgeland_14.wsg", &lumv->playerSprites[47], true);
    loadWsg("secret_swadgeland_15.wsg", &lumv->playerSprites[48], true);
    loadWsg("secret_swadgeland_16.wsg", &lumv->playerSprites[49], true);
    loadWsg("secret_swadgeland_17.wsg", &lumv->playerSprites[50], true);
    loadWsg("secret_swadgeland_18.wsg", &lumv->playerSprites[51], true);
    loadWsg("secret_swadgeland_19.wsg", &lumv->playerSprites[52], true);
    loadWsg("secret_swadgeland_20.wsg", &lumv->playerSprites[53], true);
    loadWsg("secret_swadgeland_21.wsg", &lumv->playerSprites[54], true);*/
    
    ESP_LOGI(LUM_TAG, "Loading Enemies");
    loadWsg("enemy_a1.wsg", &lumv->enemySprites[0], true);
    loadWsg("enemy_a2.wsg", &lumv->enemySprites[1], true);
    loadWsg("enemy_a3.wsg", &lumv->enemySprites[2], true);
    loadWsg("enemy_a4.wsg", &lumv->enemySprites[3], true);
    loadWsg("enemy_a5.wsg", &lumv->enemySprites[4], true);
    loadWsg("enemy_a6.wsg", &lumv->enemySprites[5], true);
    loadWsg("enemy_a7.wsg", &lumv->enemySprites[6], true);
    loadWsg("enemy_b1.wsg", &lumv->enemySprites[7], true);
    loadWsg("enemy_b2.wsg", &lumv->enemySprites[8], true);
    loadWsg("enemy_b3.wsg", &lumv->enemySprites[9], true);
    loadWsg("enemy_b4.wsg", &lumv->enemySprites[10], true);
    loadWsg("enemy_b5.wsg", &lumv->enemySprites[11], true);
    loadWsg("enemy_b6.wsg", &lumv->enemySprites[12], true);
    loadWsg("enemy_b7.wsg", &lumv->enemySprites[13], true);
    loadWsg("enemy_c1.wsg", &lumv->enemySprites[14], true);
    loadWsg("enemy_c2.wsg", &lumv->enemySprites[15], true);
    loadWsg("enemy_c3.wsg", &lumv->enemySprites[16], true);
    loadWsg("enemy_c4.wsg", &lumv->enemySprites[17], true);
    loadWsg("enemy_c5.wsg", &lumv->enemySprites[18], true);
    loadWsg("enemy_c6.wsg", &lumv->enemySprites[19], true);
    loadWsg("enemy_c7.wsg", &lumv->enemySprites[20], true);

    loadWsg("alert.wsg", &lumv->alertSprite, true); 

    if (lumv->gameType == LUMBERJACK_ATTACK)
    {
        lumberjackSetupLevel(0);
    }
    else if (lumv->gameType == LUMBERJACK_PANIC)
    {
        lumberjackSetupLevel(0);
    }
}

void lumberjackSetupLevel(int index)
{

    lumv->yOffset = 0;
    lumv->currentMapHeight = 21;
    lumv->lives = 3;
    lumv->spawnIndex = 0;
    lumv->spawnTimer = 2750;
    lumv->spawnSide = 0;
    

    lumv->localPlayer = calloc (1, sizeof(lumberjackEntity_t));
    lumberjackSetupPlayer(lumv->localPlayer, 0);
    lumberjackSpawnPlayer(lumv->localPlayer, 94, 0, 0);
    
    //snprintf(lumv->localPlayer->name, sizeof(lumv->localPlayer->name), "Player");
    strcpy(lumv->localPlayer->name," Dennis"); //If you see this... this name means nothing

    for (int i = 0; i < 2; i++)
    {
        
        lumv->enemy[i] = calloc (1, sizeof(lumberjackEntity_t));
        lumberjackSetupEnemy(lumv->enemy[i], 0);

        sprintf(lumv->enemy[i]->name, "Enemy %d", i);


    }

    const uint8_t level[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    4, 4, 4, 4, 4, 4, 4, 0, 0, 0, 0, 4, 4, 4, 4, 4, 4, 4,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 4, 4, 3, 3, 4, 4, 0, 0, 0, 0, 0, 0,
    4, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 4, 4,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    4, 4, 4, 4, 4, 4, 0, 0, 0, 0, 0, 0, 4, 4, 4, 4, 4, 4,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 5, 0, 0, 0,
    0, 0, 6, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 9, 10, 0, 0, 0,
    };
    
    const uint8_t ani[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 1, 3, 1,
    2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2,
    };

    for (int i = 0; i < ARRAY_SIZE(ani); i++)
    {
        lumv->anim[i] = ani[i]; 

        lumv->tile[i].x = i % 18;
        lumv->tile[i].y = i / 18;
        lumv->tile[i].type = level[i];
    }

    ESP_LOGI(LUM_TAG, "LOADED");
}

/**
 * @brief TODO use this somewhere
 */
void restartLevel(void)
{
    lumberjackRespawn(lumv->localPlayer);
}


void lumberjackGameLoop(int64_t elapsedUs)
{

    baseMode(elapsedUs);
}

void baseMode(int64_t elapsedUs)
{
    //Update State
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        lumv->btnState = evt.state;
    }

    // return;

    //Check Controls
    if (lumv->localPlayer->state != LUMBERJACK_DEAD && lumv->localPlayer->state != LUMBERJACK_OFFSCREEN)
    {
        lumberjackDoControls();
    }   

    for (int i = 0; i < ARRAY_SIZE(lumv->enemy); i++)
    {
        if (lumv->enemy[i] == NULL) continue;

        lumberjackDoEnemyControls(lumv->enemy[i]);
    }

    //Clear cruft
    lumberjackUpdate(elapsedUs);

    //Check spawn
    lumberjackSpawnCheck(elapsedUs);

    for (int i = 0; i < ARRAY_SIZE(lumberjackCollisionDebugTiles); i++)
    {
        lumberjackCollisionDebugTiles[i].type = -1;
        lumberjackCollisionDebugTiles[i].x = -1;
        lumberjackCollisionDebugTiles[i].y = -1;
        lumberjackCollisionDebugTiles[i].collision = -1;
        lumberjackCollisionDebugTiles[i].index = -1;
        lumberjackCollisionDebugTiles[i].offset = 0;
        lumberjackCollisionDebugTiles[i].offset_time = 0;
    }  

    if (lumv->localPlayer->onGround && !lumv->localPlayer->jumpReady)
    {
        if (lumv->localPlayer->jumpPressed == false)
        {
            lumv->localPlayer->jumpReady = true;
        }
    }

    //Check physics
    
    //Enemy
    for (int eIdx = 0; eIdx < ARRAY_SIZE(lumv->enemy); eIdx++)
    {
        lumberjackEntity_t* enemy = lumv->enemy[eIdx]; 
        if (enemy == NULL || enemy->ready == true) continue;

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
                //Hopefully the enemy isn't dead off screen.
                // lumberjackResetEnemy(enemy);
                //ESP_LOGI(LUM_TAG, "FLIP IT!");
                enemy->state = LUMBERJACK_RUN;

               enemy->direction = (enemy->flipped) ? -1 : 1;
                
                enemy->showAlert = false;
                enemy->vy = -10;

                lumberjackUpdateEnemy(enemy, enemy->type + 1);
            }
        }


        lumberjackUpdateEntity(enemy,elapsedUs);
        lumberjackUpdatePlayerCollision(lumv->localPlayer);
        
        for (int oeIdx = 0; oeIdx < ARRAY_SIZE(lumv->enemy); oeIdx++)
        {
            if (lumv->enemy[oeIdx] == NULL) continue;

            lumberjackUpdateEnemyCollision(lumv->enemy[oeIdx]);
        }
    }

    //Player
    if (lumv->localPlayer->ready)
    {
        lumv->localPlayer->respawn -= elapsedUs / 10000;

        if (lumv->localPlayer->respawn <= 0 && lumv->lives > 0)
        {
            //Respawn player
            lumv->localPlayer->respawn = 0;
            lumberjackRespawn(lumv->localPlayer);

        }

    } else if (lumv->localPlayer->state != LUMBERJACK_OFFSCREEN && lumv->localPlayer->state != LUMBERJACK_VICTORY)
    {
        lumberjackUpdateEntity(lumv->localPlayer, elapsedUs);

        //
        for (int i = 0; i < ARRAY_SIZE(lumv->enemy); i++)
        {
            lumberjackEntity_t* enemy = lumv->enemy[i];

            if (enemy == NULL || lumv->localPlayer->state == LUMBERJACK_DEAD) continue;

            if (enemy->state != LUMBERJACK_DEAD && enemy->state != LUMBERJACK_OFFSCREEN) 
            {
                //DO AABB checking
                if (checkCollision(lumv->localPlayer, enemy))
                {
                    //ESP_LOGI(LUM_TAG, "Collision");

                    if (enemy->state == LUMBERJACK_BUMPED || enemy->state == LUMBERJACK_BUMPED_IDLE)
                    {
                        enemy->state = LUMBERJACK_DEAD; 
                        enemy->vy = -30;
                        if (lumv->localPlayer->vx != 0)
                        {
                            enemy->direction = abs(lumv->localPlayer->vx)/lumv->localPlayer->vx;
                        }
                        else
                        {
                            enemy->direction = 0;
                        }
                        enemy->vx = enemy->direction * 10;
                        enemy->active = false;
                        
                    }
                    else 
                    {
                        //Kill player
                        //ESP_LOGI(LUM_TAG, "KILL PLAYER");
                        lumv->localPlayer->state = LUMBERJACK_DEAD;
                        lumv->localPlayer->vy = -20;
                        lumv->localPlayer->active = false;
                        lumv->localPlayer->jumping = false;
                        lumv->localPlayer->jumpTimer = 0;

                    }
                }
            }
        }
    }
    
    lumv->worldTimer += elapsedUs;

    if (lumv->worldTimer > 150500)
    {
        lumv->worldTimer -= 150500;
        lumv->liquidAnimationFrame++;
        lumv->liquidAnimationFrame %=4;
    }


    //Update animation
    //Enemy Animation
    for (int i = 0; i < ARRAY_SIZE(lumv->enemy); i++)
    {
        if (lumv->enemy[i] == NULL) continue;

        lumv->enemy[i]->timerFrameUpdate += elapsedUs;
        if (lumv->enemy[i]->timerFrameUpdate > lumv->enemy[i]->animationSpeed)
        {
            lumv->enemy[i]->currentFrame++;
            lumv->enemy[i]->timerFrameUpdate = 0;//;
        }
    }

    //Player
    lumv->localPlayer->timerFrameUpdate += elapsedUs;
    if (lumv->localPlayer->timerFrameUpdate > lumv->localPlayer->animationSpeed)
    {
        lumv->localPlayer->currentFrame++;
        lumv->localPlayer->timerFrameUpdate = 0;//;
    }

    lumv->yOffset = lumv->localPlayer->y - 140;
    if (lumv->yOffset < -16) lumv->yOffset = -16;
    if (lumv->yOffset > 96) lumv->yOffset = 96;

    //Draw section
    //fillDisplayArea ( 0,0, 280,256, c145); 	

    //Redraw bottom
    lumberjackTileMap();

    //Draw enemies

    
    for (int i = 0; i < ARRAY_SIZE(lumv->enemy); i++)
    {
        if (lumv->enemy[i] == NULL || lumv->enemy[i]->ready) continue;
        lumberjackEntity_t* enemy = lumv->enemy[i];

        int eFrame = lumberjackGetEnemyAnimation(enemy);

        drawWsg(&lumv->enemySprites[enemy->spriteOffset + eFrame], enemy->x, enemy->y - lumv->yOffset, enemy->flipped, false, 0);       
    
        if (enemy->x > 270)
        {
           drawWsg(&lumv->enemySprites[enemy->spriteOffset + eFrame], enemy->x - 299, enemy->y - lumv->yOffset, enemy->flipped, false, 0);
        }

        if (enemy->showAlert)
        {
            //Fix the magic number :(
            drawWsg(&lumv->alertSprite, enemy->x + 6, enemy->y - 26 - lumv->yOffset, enemy->flipped, false, 0);       
        }
    
    }


    int currentFrame = lumberjackGetPlayerAnimation(lumv->localPlayer);
 
    if (lumv->localPlayer->state == LUMBERJACK_DEAD)
    {
        //ESP_LOGI(LUM_TAG, "DEAD %d", currentFrame);
    }


    //ESP_LOGI(LUM_TAG, "Err %d %ld %d", lumv->playerSprites[3], elapsedUs, lumv->localPlayer->y - lumv->yOffset);
    

    //drawRect(lumv->localPlayer->x - 4, lumv->localPlayer->y - lumv->yOffset, lumv->localPlayer->x + 32, lumv->localPlayer->y - lumv->yOffset + 32, c050);
    //drawWsgSimple(&lumv->playerSprites[lumv->localPlayer->spriteOffset + currentFrame], lumv->localPlayer->x - 4, lumv->localPlayer->y - lumv->yOffset);

    //This is where it breaks. When it tries to play frame 3 or 4 it crashes.
    drawWsg(&lumv->playerSprites[lumv->localPlayer->spriteOffset + currentFrame], lumv->localPlayer->x - 4, lumv->localPlayer->y - lumv->yOffset, lumv->localPlayer->flipped, false, 0);    

    if (lumv->localPlayer->x > 270)
    {
        drawWsg(&lumv->playerSprites[lumv->localPlayer->spriteOffset + currentFrame], lumv->localPlayer->x - 299, lumv->localPlayer->y - lumv->yOffset, lumv->localPlayer->flipped, false, 0);
    }

    //Debug
    char debug[20] ={0};        
    snprintf(debug, sizeof(debug), "CellX: %d %d %d", lumv->enemy[0]->x, lumv->enemy[1]->x, lumv->spawnTimer);

    drawText(&lumv->ibm, c000, debug, 16, 16);

    if (lumv->localPlayer->jumpPressed)
    {
        drawText(&lumv->ibm, c555, "A", 16, 32);
    }   
    else
    {
        drawText(&lumv->ibm, c000, "A", 16, 32);
    }

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
            for(int i = 0; i < ARRAY_SIZE(lumv->enemy); i++)
            {
                if (lumv->enemy[i] == NULL) continue;

                if (lumv->enemy[i]->ready && lumv->enemy[i]->state != LUMBERJACK_DEAD)
                {
                    lumv->spawnSide++;
                    lumv->spawnSide %= 2;

                    lumv->spawnTimer = 750;

                    lumberjackRespawnEnemy(lumv->enemy[i], lumv->spawnSide);
                    spawnReady = false;
                    //break;
                }
            }

            if (spawnReady)
            {
                //No one was spawned.
                lumv->spawnTimer = 500;
            }
        }

    }

    

}

static void lumberjackUpdateEntity(lumberjackEntity_t* entity, int64_t elapsedUs)
{
    bool onGround = false;

    //World wrap
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


    //Check jumping first
    if (entity->jumpPressed && entity->active)
    {        
        if (entity->onGround && entity->jumpReady)
        {
            //Check if player CAN jump
            entity->jumpReady = false; 
            entity->jumping = true;
            entity->vy = -15;
            entity->jumpTimer = 225000;
            entity->onGround = false;
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
           entity->jumpTimer  = 0;
           entity->jumping = false;
        }
    } 

    if (entity->jumping == false && entity->flying == false)
        entity->vy += 6; //Fix gravity

    if (entity->active)
    {
        if (entity->direction > 0)
        {
            if (entity->onGround)
                entity->vx += 5;
            else
                entity->vx += 8;

        }
        else if (entity->direction < 0)
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


    if (entity->vx > entity->maxVX) entity->vx = entity->maxVX;
    if (entity->vx < -entity->maxVX) entity->vx = -entity->maxVX;
    if (entity->vy < -30) entity->vy = -30;
    if (entity->vy > 16) entity->vy = 16;

    float elapsed = elapsedUs / 100000.0;
    int destinationX = entity->x + (int)(entity->vx * elapsed);
    int destinationY = entity->y + (entity->vy * elapsed);

    if (entity->vx < 0 && entity->active)
    {
        lumberjackTile_t* tileA = lumberjackGetTile(destinationX + 0, entity->y + 2);
        lumberjackTile_t* tileB = lumberjackGetTile(destinationX + 0, entity->y + entity->height);

        if ((tileA != NULL && lumberjackIsCollisionTile(tileA->type)) || (tileB != NULL && lumberjackIsCollisionTile(tileB->type)))
        {
            destinationX = entity->x;
            entity->vx = 0;
            
        }
    }
    else if (entity->vx > 0 && entity->active)
    {
        lumberjackTile_t* tileA = lumberjackGetTile(destinationX + 24, entity->y + 2);
        lumberjackTile_t* tileB = lumberjackGetTile(destinationX + 24, entity->y + entity->height);

        if ((tileA != NULL && lumberjackIsCollisionTile(tileA->type)) || (tileB != NULL && lumberjackIsCollisionTile(tileB->type)))
        {
            destinationX = entity->x;
            entity->vx = 0;
        }
    }

    if (entity->vy < 0 && entity->active)
    {
        lumberjackTile_t* tileA = lumberjackGetTile(destinationX, destinationY);
        lumberjackTile_t* tileB = lumberjackGetTile(destinationX + 16, destinationY);

        if (lumberjackIsCollisionTile(tileA->type) || lumberjackIsCollisionTile(tileB->type))
        {
            destinationY = ((tileA->y + 1) * 16);
            entity->jumpTimer = 0;
            entity->jumping = false;
            entity->vy = 0;

            if (lumberjackIsCollisionTile(tileA->type))
            {
                lumv->tile[tileA->index].offset = 10;
                lumv->tile[tileA->index].offset_time = 100;

                lumberjackDetectBump(tileA);
            }

            if (lumberjackIsCollisionTile(tileB->type))
            {
                lumberjackDetectBump(tileB);

                lumv->tile[tileB->index].offset = 10;
                lumv->tile[tileB->index].offset_time = 100;
            }
        }
    }
    else if (entity->vy > 0 && entity->active)
    {

        lumberjackTile_t* tileA = lumberjackGetTile(destinationX, destinationY + entity->height);
        lumberjackTile_t* tileB = lumberjackGetTile(destinationX + 16, destinationY + entity->height);

        if ((tileA != NULL && lumberjackIsCollisionTile(tileA->type)) || (tileB != NULL && lumberjackIsCollisionTile(tileB->type)))
        {
            destinationY = ((tileA->y - entity->tileHeight) * 16);
            entity->vy = 0;
            onGround = true;
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
        entity->y = 350;
        entity->active = false;
        if (entity->state == LUMBERJACK_DEAD)
        {
            //
            if (entity == lumv->localPlayer)
            {
                if (entity->respawn == 0)
                {
                    //ESP_LOGI(LUM_TAG, "DEAD & hit the ground %d", entity->respawn);
                    entity->respawn = 250;
                    entity->ready = true;
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
                entity->respawn = 500;
                entity->ready = true;
                lumv->localPlayer->state = LUMBERJACK_DEAD;
                return;
            }
            else
            {
                entity->active = false;
                entity->ready = true;
            }
        }
        if (entity->state != LUMBERJACK_DEAD) entity->state = LUMBERJACK_OFFSCREEN;
    }
}

void lumberjackUpdate(int64_t elapseUs)
{
    for (int i = 0; i < ARRAY_SIZE(lumv->tile); i++)
    {
        if (lumv->tile[i].offset > 0)
        {
            // ESP_LOGI(LUM_TAG, "Update %d",lumv->tile[i].offset);

            lumv->tile[i].offset_time -= elapseUs;
            if (lumv->tile[i].offset_time < 0)
            {
                lumv->tile[i].offset_time += 2500;
                lumv->tile[i].offset--;
            }
        }
    }
}

void lumberjackTileMap(void)
{
    for (int y = 0; y < 21; y++)
    {
        for (int x = 0; x < 18; x++)
        {
            int index = (y * 18) + x;
            int tileIndex = lumv->tile[ index].type;
            int animIndex = lumv->anim[index];
            int offset = lumv->tile[index].offset;

            if ((y * 16) - lumv->yOffset >= -16)
            {
                if (animIndex > 0 && animIndex < 4)
                {
                    drawWsgSimple(&lumv->animationTiles[((animIndex - 1) * 4) + (lumv->liquidAnimationFrame % 4)], x * 16, (y * 16) - lumv->yOffset + 8 - offset);
                }

                if (tileIndex > 0 && tileIndex < 13)
                {
                    if (tileIndex < 11)
                    {
                        drawWsgSimple(&lumv->floorTiles[tileIndex - 1],x * 16, (y * 16) - lumv->yOffset  - offset);
                    }
                }
            }

        }
    }
}

void lumberjackDoControls(void)
{
    lumv->localPlayer->cW = 15;
    int previousState = lumv->localPlayer->state;
    bool buttonPressed = false;
    if (lumv->btnState & PB_LEFT)
    {
        lumv->localPlayer->flipped = true;
        lumv->localPlayer->state = 2;
        lumv->localPlayer->direction = -1;
        buttonPressed = true;
    }
    else if (lumv->btnState & PB_RIGHT)
    {
        lumv->localPlayer->flipped = false;
        lumv->localPlayer->state = 2;
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

    //TODO: This is sloppy Troy
    if (lumv->btnState & PB_A)
    {
        lumv->localPlayer->jumpPressed = true;
    }
    else
    {
        lumv->localPlayer->jumpPressed = false;
    }


    if (buttonPressed == false && lumv->localPlayer->active)
    {
        lumv->localPlayer->state = 1; //Do a ton of other checks here
    }

    if (lumv->localPlayer->state != previousState)
    {
        //lumv->localPlayer->currentFrame = 0;
        lumv->localPlayer->timerFrameUpdate = 0;
    }
}


static lumberjackTile_t* lumberjackGetTile(int x, int y)
{    
    int tx = (int)x/16;
    int ty = (int)y/16;

    if (tx < 0)
        tx = 17;
    if (tx > 17)
        tx = 0;
    
    if (ty < 0) ty = 0;
    if (ty > lumv->currentMapHeight) ty = lumv->currentMapHeight;

    // int test = -1;
    for (int i = 0; i < 32; i++ )
    {
        if (lumberjackCollisionDebugTiles[i].type == -1)
        {
            if (lumberjackCollisionDebugTiles[i].index == (ty * 18) + tx)
                return &lumberjackCollisionDebugTiles[i];

            lumberjackCollisionDebugTiles[i].index = (ty * 18) + tx;
            lumberjackCollisionDebugTiles[i].x = tx;
            lumberjackCollisionDebugTiles[i].y = ty;
            lumberjackCollisionDebugTiles[i].type = lumv->tile[(ty * 18) + tx].type;

            return &lumberjackCollisionDebugTiles[i];
        }
        // test = i;
    }

    //ESP_LOGI(LUM_TAG,"NO TILE at %d %d!", test, ty);
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
    if (lumv->localPlayer->state != LUMBERJACK_BUMPED &&
        lumv->localPlayer->state != LUMBERJACK_DEAD)
    {
        //TODO put in bump the player
    }

    for(int i = 0; i < ARRAY_SIZE(lumv->enemy); i++)
    {
        lumberjackEntity_t* enemy = lumv->enemy[i];
        if (enemy == NULL) continue;

        if (enemy->onGround || enemy->flying)
        {
            int tx = ((enemy->x - 8) / 16);
            int ty = ((enemy->y) / 16) + 1;
            int tx2 = ((enemy->x + 8) / 16);

            if (tx < 0) tx = 0;
            if (tx > 17) tx = 17;
            if (tx2 < 0) tx2 = 0;
            if (tx2 > 17) tx2 = 17;
            if (ty < 0) ty = 0;
            if (ty > lumv->currentMapHeight) ty = lumv->currentMapHeight;

            lumberjackTile_t* tileA = &lumv->tile[(ty * 18) + tx];
            lumberjackTile_t* tileB = &lumv->tile[(ty * 18) + tx2];

            
            if ((tileA != NULL && (ty * 18) + tx == tile->index) || (tileB != NULL && (ty * 18) + tx2 == tile->index))
            {
                enemy->vy = -20;
                enemy->onGround = false;

                if (enemy->state == LUMBERJACK_BUMPED_IDLE)
                {
                    enemy->state = LUMBERJACK_RUN;
                    enemy->direction = enemy->flipped ? -1 : 1;
                }
                else
                {
                    enemy->direction = 0;
                    enemy->state = LUMBERJACK_BUMPED;                    
                }


              //  ESP_LOGI(LUM_TAG, "BUMPED!");
                
            }

            /**
            if (tileA != NULL)
                ESP_LOGI(LUM_TAG, "Tile A %d %d", tileA->index, tile->index);

            if (tileB != NULL)
                ESP_LOGI(LUM_TAG, "Tile B %d %d", tileB->index, tile->index);
                */

        }
        else
        {
        //    ESP_LOGI(LUM_TAG, "Not ground!");
        }
    }
}

void lumberjackExitGameMode(void)
{
    printf("FREE");
    p2pDeinit(&lumv->p2p);
   //** FREE THE SPRITES **//
    //Free the enemies
    for (int i =0; i < ARRAY_SIZE(lumv->enemySprites); i++)
    {
        freeWsg(&lumv->enemySprites[i]);
    }


    //Free the players
    for (int i =0; i < ARRAY_SIZE(lumv->playerSprites); i++)
    {
        freeWsg(&lumv->playerSprites[i]);
    }

    //Free the tiles
    for (int i = 0; i < ARRAY_SIZE(lumv->animationTiles); i++)
    {
        freeWsg(&lumv->animationTiles[i]);
    }

    freeWsg(&lumv->alertSprite);
    
    freeFont(&lumv->ibm);
    free(lumv);
}
