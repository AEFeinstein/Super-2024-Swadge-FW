//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include <esp_log.h>

// For lumberjack
#include "lumberjack.h"
#include "lumberjackEntity.h"
#include "lumberjackPlayer.h"


static void lumberjackEnterMode(void);
static void lumberjackExitMode(void);
static void lumberjackMainLoop(int64_t elapsedUs);
static void lumberjackMenuLoop(int64_t elapsedUs);
static void lumberjackGameLoop(int64_t elapsedUs);
static void lumberjackAudioCallback(uint16_t* samples, uint32_t sampleCnt);
static void lumberjackBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);
static void lumberjackEspNowRecvCb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len, int8_t rssi);
static void lumberjackEspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status);

static void lumberjackConCb(p2pInfo* p2p, connectionEvt_t evt);
static void lumberjackMsgRxCb(p2pInfo* p2p, const uint8_t* payload, uint8_t len);
static void lumberjackMsgTxCbFn(p2pInfo* p2p, messageStatus_t status, const uint8_t* data, uint8_t len);
static void lumberjackDoControls(void);
static void lumberjackTileMap(void);
static void lumberjackUpdate(int64_t elapseUs);

static void lumberjackUpdateEntity(lumberjackEntity_t* entity, int64_t elapsedUs);

static void restartLevel();


//static void lumberjackMenuCb(const char*, bool selected, uint32_t settingVal);

const static char* LUM_TAG = "BMN";

static const char lumberjackName[] = "Lumber Jack";
lumberjackTile_t lumberjackCollisionDebugTiles[8] = {};

static void lumberjackSetupLevel(int index);
static lumberjackTile_t* lumberjackGetTile(int x, int y);
static bool lumberjackIsCollisionTile(int index);
static void lumberjackDetectBump(lumberjackTile_t* tile);

swadgeMode_t lumberjackMode = {
    .modeName                 = lumberjackName,
    .wifiMode                 = ESP_NOW,
    .overrideUsb              = false,
    .usesAccelerometer        = false,
    .usesThermometer          = false,
    .fnEnterMode              = lumberjackEnterMode,
    .fnExitMode               = lumberjackExitMode,
    .fnMainLoop               = lumberjackMainLoop,
    .fnAudioCallback          = lumberjackAudioCallback,
    .fnBackgroundDrawCallback = lumberjackBackgroundDrawCallback,
    .fnEspNowRecvCb           = lumberjackEspNowRecvCb,
    .fnEspNowSendCb           = lumberjackEspNowSendCb,
    .fnAdvancedUSB            = NULL,
};
lumberjackVars_t* lumv;

static void lumberjackEnterMode(void)
{
    lumv = calloc(1, sizeof(lumberjackVars_t));
    loadFont("ibm_vga8.font", &lumv->ibm, false);

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

    //Init menu :(

    //bzrStop(); // Stop the buzzer?
    p2pInitialize(&lumv->p2p, 'd', lumberjackConCb, lumberjackMsgRxCb, -70);
    p2pStartConnection(&lumv->p2p);

    const uint8_t testMsg[] = {0x01, 0x02, 0x03, 0x04};
    p2pSendMsg(&lumv->p2p, testMsg, ARRAY_SIZE(testMsg), lumberjackMsgTxCbFn);

    lumv->worldTimer = 0;
    lumv->liquidAnimationFrame = 0;
    //High score stuff?

    //Setup first level
    lumberjackSetupLevel(0);

}

void lumberjackSetupLevel(int index)
{

    lumv->yOffset = 0;
    lumv->currentMapHeight = 21;
    lumv->lives = 3;

    lumv->localPlayer = calloc (1, sizeof(lumberjackEntity_t));
    lumberjackSetupPlayer(lumv->localPlayer, 3);
    lumberjackSpawnPlayer(lumv->localPlayer, 94, 0, 0);

    lumv->enemy[0] = calloc (1, sizeof(lumberjackEntity_t));
    lumberjackSetupEnemy(lumv->enemy[0], 0);

    uint8_t level[] = {
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
    
    uint8_t ani[] = {
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

    for (int i = 0; i < 378; i++)
    {
        lumv->anim[i] = ani[i]; 

        lumv->tile[i].x = i % 18;
        lumv->tile[i].y = i / 18;
        lumv->tile[i].type = level[i];
    }

    
}

static void restartLevel()
{
    lumberjackRespawn(lumv->localPlayer);
}


static void lumberjackExitMode(void)
{    
    p2pDeinit(&lumv->p2p);

    //Free the enemies
    for (int i =0; i < 8; i++)
    {
        lumberjackUnloadEnemy(lumv->enemy[i]);
    }

    //Free the tiles
    freeFont(&lumv->ibm);
    free(lumv);
}

static void lumberjackMainLoop(int64_t elapsedUs)
{
    lumberjackGameLoop(elapsedUs);
}

static void lumberjackMenuLoop(int64_t elapsedUs)
{

}

static void lumberjackGameLoop(int64_t elapsedUs)
{
    
    //Update State
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        lumv->btnState = evt.state;
    }

    //Check Controls
    if (lumv->localPlayer->state != LUMBERJACK_DEAD && lumv->localPlayer->state != LUMBERJACK_OFFSCREEN)
    {
        lumberjackDoControls();
    }   

    //Clear cruft
    lumberjackUpdate(elapsedUs);

    for (int i = 0; i < 8; i++)
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
    for (int i = 0; i < 8; i++)
    {
        if (lumv->enemy[i] == NULL) continue;

        lumberjackUpdateEntity(lumv->enemy[i],elapsedUs);
    }

    //Player
    if (lumv->localPlayer->state == LUMBERJACK_OFFSCREEN)
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
        for (int i = 0; i < 8; i++)
        {
            lumberjackEntity_t* enemy = lumv->enemy[i];

            if (enemy == NULL || lumv->localPlayer->state == LUMBERJACK_DEAD) continue;

            if (enemy->state != LUMBERJACK_DEAD && enemy->state != LUMBERJACK_OFFSCREEN) 
            {
                //DO AABB checking
                if (lumv->localPlayer->x < enemy->x + enemy->width &&
                lumv->localPlayer->x + lumv->localPlayer->width > enemy->x &&
                lumv->localPlayer->y < enemy->y + enemy->height &&
                lumv->localPlayer->y + lumv->localPlayer->height > enemy->y)
                {
                    ESP_LOGI(LUM_TAG, "Collision");

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
                        ESP_LOGI(LUM_TAG, "KILL PLAYER");
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
    //Enemy Aniamtion
    for (int i = 0; i < 8; i++)
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
    fillDisplayArea ( 0,0, 280,256,c145); 	

    //Redraw bottom
    lumberjackTileMap();

    //Draw enemies
    for (int i = 0; i < 8; i++)
    {
        if (lumv->enemy[i] == NULL) continue;
        int eFrame = lumberjackGetEnemyAnimation(lumv->enemy[i]);

        drawWsg(&lumv->enemy[i]->frames[eFrame], lumv->enemy[i]->x, lumv->enemy[i]->y - lumv->yOffset, false, false, 0);
         if (lumv->enemy[i]->x > 270)
        {
            drawWsg(&lumv->enemy[i]->frames[eFrame], lumv->enemy[i]->x - 299, lumv->enemy[i]->y - lumv->yOffset, lumv->enemy[i]->flipped, false, 0);
        }
    }


    int currentFrame = lumberjackGetPlayerAnimation(lumv->localPlayer);
 
    if (lumv->localPlayer->state == LUMBERJACK_DEAD)
    {
        ESP_LOGI("LLL", "DEAD %d", currentFrame);
    }

    drawWsg(&lumv->localPlayer->frames[currentFrame], lumv->localPlayer->x - 4, lumv->localPlayer->y - lumv->yOffset, lumv->localPlayer->flipped, false, 0);    
    if (lumv->localPlayer->x > 270)
    {
        drawWsg(&lumv->localPlayer->frames[currentFrame], lumv->localPlayer->x - 299, lumv->localPlayer->y - lumv->yOffset, lumv->localPlayer->flipped, false, 0);
    }

    //Debug
    char debug[20] ={0};        
    snprintf(debug, sizeof(debug), "CellX: %d %d %d", lumv->localPlayer->state, lumv->localPlayer->y, lumv->localPlayer->respawn);

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

        if (entity->vx > entity->maxVX) entity->vx = entity->maxVX;
        if (entity->vx < -entity->maxVX) entity->vx = -entity->maxVX;
        if (entity->vy < -30) entity->vy = -30;
        if (entity->vy > 16) entity->vy = 16;
    }
   

    int destinationX = entity->x + (entity->vx * elapsedUs) / 100000;
    int destinationY = entity->y + (entity->vy * elapsedUs) / 100000;

    if (entity->vx < 0 && entity->active)
    {
        lumberjackTile_t* tileA = lumberjackGetTile(destinationX + 0, entity->y + 2);
        lumberjackTile_t* tileB = lumberjackGetTile(destinationX + 0, entity->y + entity->height);

        if (lumberjackIsCollisionTile(tileA->type) || lumberjackIsCollisionTile(tileB->type))
        {
            destinationX = entity->x;
            entity->vx = 0;
            
        }
    }
    else if (entity->vx > 0 && entity->active)
    {
        lumberjackTile_t* tileA = lumberjackGetTile(destinationX + 24, entity->y + 2);
        lumberjackTile_t* tileB = lumberjackGetTile(destinationX + 24, entity->y + entity->height);

        if (lumberjackIsCollisionTile(tileA->type) || lumberjackIsCollisionTile(tileB->type))
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
                ESP_LOGI(LUM_TAG, "Pop that head! A %d",tileA->index);
                lumv->tile[tileA->index].offset = 10;
                lumv->tile[tileA->index].offset_time = 100;

                lumberjackDetectBump(tileA);
            }

            if (lumberjackIsCollisionTile(tileB->type))
            {
                ESP_LOGI(LUM_TAG, "Pop that head! B %d", tileB->index);
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

        if (lumberjackIsCollisionTile(tileA->type) || lumberjackIsCollisionTile(tileB->type))
        {
            destinationY = ((tileA->y - entity->tileHeight) * 16);
            entity->vy = 0;
            onGround = true;
        }
    }

    entity->onGround = onGround;
    entity->x = destinationX;
    entity->y = destinationY;

    if (entity->y > 350)
    {
        entity->y = 350;
        entity->active = false;
        if (entity->state == LUMBERJACK_DEAD)
        {
            //
            ESP_LOGI(LUM_TAG, "DEAD & hit the ground %d", entity->respawn);

            if (entity == lumv->localPlayer && entity->respawn == 0)
            {
                entity->respawn = 250;
                return;
            }
        }
        
        if (entity->state != LUMBERJACK_OFFSCREEN && entity->state != LUMBERJACK_DEAD)
        {
            entity->respawn = 500;

            if (entity == lumv->localPlayer)
            {
                ESP_LOGI(LUM_TAG, "PLAYER DIED %d", lumv->localPlayer->state);
                lumv->localPlayer->state = LUMBERJACK_DEAD;
                return;
            }
        }
        entity->state = LUMBERJACK_OFFSCREEN;
    }
}

static void lumberjackUpdate(int64_t elapseUs)
{
    for (int i = 0; i < 400; i++)
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

static void lumberjackTileMap()
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

static void lumberjackDoControls()
{
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
            lumv->localPlayer->state = 3;
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

    if (lumv->btnState & PB_B)
    {
        lumv->enemy[0]->flying = true;
    }

    if (buttonPressed == false)
    {
        lumv->localPlayer->state = 1; //Do a ton of other checks here
    }

    if (lumv->localPlayer->state != previousState)
    {
        //lumv->localPlayer->currentFrame = 0;
        lumv->localPlayer->timerFrameUpdate = 0;
    }
}

static void lumberjackAudioCallback(uint16_t* samples, uint32_t sampleCnt)
{
    
}

static void lumberjackBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
    fillDisplayArea(x, y, x + w, y + h, c555);
}

static void lumberjackEspNowRecvCb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len, int8_t rssi)
{
    p2pRecvCb(&lumv->p2p, esp_now_info->src_addr, data, len, rssi);
}

static void lumberjackEspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status)
{
    p2pSendCb(&lumv->p2p, mac_addr, status);
}


static void lumberjackConCb(p2pInfo* p2p, connectionEvt_t evt)
{
    //Do anything
}

static void lumberjackMsgRxCb(p2pInfo* p2p, const uint8_t* payload, uint8_t len)
{
    //Do anything
}

static void lumberjackMsgTxCbFn(p2pInfo* p2p, messageStatus_t status, const uint8_t* data, uint8_t len)
{

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

    int test = -1;
    for (int i = 0; i < 8; i++ )
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
        test = i;
    }

    ESP_LOGI("TU","NO TILE at %d %d!", test, ty);
    return NULL;
}

static bool lumberjackIsCollisionTile(int index)
{
    if (index == 0 || index == 1 || index == 6 || index == 5 || index == 10)
        return false;


    return true;
}

static void lumberjackDetectBump(lumberjackTile_t* tile)
{
    if (&lumv->localPlayer->state != LUMBERJACK_BUMPED && &lumv->localPlayer->state != LUMBERJACK_DEAD && &lumv->localPlayer->state != LUMBERJACK_BUMPED)
    {
        //TODO put in bump the player
    }

    for(int i = 0; i < 8; i++)
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

            ESP_LOGI(LUM_TAG, "Ground %d %d", (ty * 18) + tx , (ty * 18) + tx2);
            
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


                ESP_LOGI(LUM_TAG, "BUMPED!");
                
            }
            if (tileA != NULL)
                ESP_LOGI(LUM_TAG, "Tile A %d %d", tileA->index, tile->index);

            if (tileB != NULL)
                ESP_LOGI(LUM_TAG, "Tile B %d %d", tileB->index, tile->index);

        }
        else
        {
            ESP_LOGI(LUM_TAG, "Not ground!");
        }
    }
}

// static void lumberjackMenuCb(const char* label, bool selected, uint32_t settingVal)
// {
//     printf("");
// }