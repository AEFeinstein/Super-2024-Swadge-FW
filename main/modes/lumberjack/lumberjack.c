//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include <esp_log.h>

// For lumberjack
#include "lumberjack.h"
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

//static void lumberjackMenuCb(const char*, bool selected, uint32_t settingVal);

static const char lumberjackName[] = "Lumber Jack";
lumberjackTile_t lumberjackCollisionDebugTiles[8] = {};

static void lumberjackSetupLevel(int index);
static lumberjackTile_t* lumberjackGetTile(int x, int y);
static bool lumberjackIsCollisionTile(int index);


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

    lumv->localPlayer = calloc (1, sizeof(lumberjackHero_t));
    lumberjackSetupPlayer(lumv->localPlayer, 3);
    lumberjackSpawnPlayer(lumv->localPlayer, 94, 0, 0);

    for (int i = 0; i < 400; i++)
    {
        lumv->tiles[i] = 0;
    }
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
        lumv->tiles[i] = level[i];
        lumv->anim[i] = ani[i]; 
    }

    
}



static void lumberjackExitMode(void)
{    
    p2pDeinit(&lumv->p2p);

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
    lumberjackDoControls();

    //Clear cruft
    for (int i = 0; i < 8; i++)
    {
        lumberjackCollisionDebugTiles[i].type = -1;
        lumberjackCollisionDebugTiles[i].x = -1;
        lumberjackCollisionDebugTiles[i].y = -1;
        lumberjackCollisionDebugTiles[i].collision = -1;
        lumberjackCollisionDebugTiles[i].index = -1;
    }  

    if (lumv->localPlayer->onGround && !lumv->localPlayer->jumpReady)
    {
        if (lumv->localPlayer->jumpPressed == false)
        {
            lumv->localPlayer->jumpReady = true;
        }
    }

    //Check physics
    bool playerOnGround = false;

    //World wrap
    lumv->localPlayer->x %= 295;
    if (lumv->localPlayer->x < -20)
    {
        lumv->localPlayer->x += 295;
    }

    //Check jumping first
    if (lumv->localPlayer->jumpPressed)
    {
        
        if (lumv->localPlayer->onGround && lumv->localPlayer->jumpReady)
        {
            //Check if player CAN jump
            lumv->localPlayer->jumpReady = false; 
            lumv->localPlayer->jumping = true;
            lumv->localPlayer->vy = -15;
            lumv->localPlayer->jumpTimer = 225000;
            lumv->localPlayer->onGround = false;
        }
        else if (lumv->localPlayer->jumping)
        {
            lumv->localPlayer->vy -= 6;
        }
    }

    if (lumv->localPlayer->jumpTimer > 0)
    {
        lumv->localPlayer->jumpTimer -= elapsedUs;
        if (lumv->localPlayer->jumpTimer <= 0)
        {
           lumv->localPlayer->jumpTimer  = 0;
           lumv->localPlayer->jumping = false;
        }
    } 

    if (lumv->localPlayer->jumping == false)
        lumv->localPlayer->vy += 6; //Fix gravity

    if (lumv->localPlayer->h > 0)
    {
        if (lumv->localPlayer->onGround)
            lumv->localPlayer->vx += 5;
        else
            lumv->localPlayer->vx += 8;

    }
    else if (lumv->localPlayer->h < 0)
    {
        if (lumv->localPlayer->onGround)
            lumv->localPlayer->vx -= 5;
        else
            lumv->localPlayer->vx -= 8;
    }
    else
    {
        if (lumv->localPlayer->onGround)
            lumv->localPlayer->vx *= .1;
    }

    if (lumv->localPlayer->vx > 15) lumv->localPlayer->vx = 15;
    if (lumv->localPlayer->vx < -15) lumv->localPlayer->vx = -15;
    if (lumv->localPlayer->vy < -30) lumv->localPlayer->vy = -30;
    if (lumv->localPlayer->vy > 16) lumv->localPlayer->vy = 16;
   
    //Physics test
    int destinationX = lumv->localPlayer->x + (lumv->localPlayer->vx * elapsedUs) / 100000;
    int destinationY = lumv->localPlayer->y + (lumv->localPlayer->vy * elapsedUs) / 100000;

    //Collision
    if (lumv->localPlayer->vx < 0)
    {
        lumberjackTile_t* tileA = lumberjackGetTile(destinationX + 0, lumv->localPlayer->y + 2);
        lumberjackTile_t* tileB = lumberjackGetTile(destinationX + 0, lumv->localPlayer->y + 31);

        if (lumberjackIsCollisionTile(tileA->type) || lumberjackIsCollisionTile(tileB->type))
        {
            destinationX = lumv->localPlayer->x;
            lumv->localPlayer->vx = 0;
            
        }
    }
    else if (lumv->localPlayer->vx > 0)
    {
        lumberjackTile_t* tileA = lumberjackGetTile(destinationX + 24, lumv->localPlayer->y + 2);
        lumberjackTile_t* tileB = lumberjackGetTile(destinationX + 24, lumv->localPlayer->y + 31);

        if (lumberjackIsCollisionTile(tileA->type) || lumberjackIsCollisionTile(tileB->type))
        {
            destinationX = lumv->localPlayer->x;
            lumv->localPlayer->vx = 0;
        }
    }

    if (lumv->localPlayer->vy < 0)
    {
        lumberjackTile_t* tileA = lumberjackGetTile(destinationX, destinationY);
        lumberjackTile_t* tileB = lumberjackGetTile(destinationX + 16, destinationY);

        if (lumberjackIsCollisionTile(tileA->type) || lumberjackIsCollisionTile(tileB->type))
        {
            destinationY = ((tileA->y + 1) * 16);
            lumv->localPlayer->jumpTimer = 0;
            lumv->localPlayer->jumping = false;
            lumv->localPlayer->vy = 0;
        }
    }
    else if (lumv->localPlayer->vy > 0)
    {

        lumberjackTile_t* tileA = lumberjackGetTile(destinationX, destinationY + 31);
        lumberjackTile_t* tileB = lumberjackGetTile(destinationX + 16, destinationY + 31);

        if (lumberjackIsCollisionTile(tileA->type) || lumberjackIsCollisionTile(tileB->type))
        {
            destinationY = ((tileA->y - 2) * 16);
            lumv->localPlayer->vy = 0;
            playerOnGround = true;
        }
    }

    lumv->localPlayer->onGround = playerOnGround;
    lumv->localPlayer->x = destinationX;
    lumv->localPlayer->y = destinationY;
    
    lumv->worldTimer += elapsedUs;

    if (lumv->worldTimer > 150500)
    {
        lumv->worldTimer -= 150500;
        lumv->liquidAnimationFrame++;
        lumv->liquidAnimationFrame %=4;
    }

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

    int currentFrame = lumberjackGetPlayerAnimation(lumv->localPlayer);
 
    drawWsg(&lumv->localPlayer->frames[currentFrame], lumv->localPlayer->x - 4, lumv->localPlayer->y - lumv->yOffset, lumv->localPlayer->flipped, false, 0);    
    if (lumv->localPlayer->x > 270)
    {
        drawWsg(&lumv->localPlayer->frames[currentFrame], lumv->localPlayer->x - 299, lumv->localPlayer->y - lumv->yOffset, lumv->localPlayer->flipped, false, 0);
    }

    //Debug
    char debug[16] ={0};        
    snprintf(debug, sizeof(debug), "CellX: %d %d", lumv->localPlayer->x, lumv->localPlayer->y);

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

static void lumberjackTileMap()
{
    for (int y = 0; y < 21; y++)
    {
        for (int x = 0; x < 18; x++)
        {
            int tileIndex = lumv->tiles[ (y * 18) + x];
            int animIndex = lumv->anim[(y * 18) + x];

            if ((y * 16) - lumv->yOffset >= -16)
            {
                if (animIndex > 0 && animIndex < 4)
                {
                    drawWsgSimple(&lumv->animationTiles[((animIndex - 1) * 4) + (lumv->liquidAnimationFrame % 4)], x * 16, (y * 16) - lumv->yOffset + 8);
                }

                if (tileIndex > 0 && tileIndex < 13)
                {
                    if (tileIndex < 11)
                    {
                        drawWsgSimple(&lumv->floorTiles[tileIndex - 1],x * 16, (y * 16) - lumv->yOffset);
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
        lumv->localPlayer->h = -1;
        buttonPressed = true;
    }
    else if (lumv->btnState & PB_RIGHT)
    {
        lumv->localPlayer->flipped = false;
        lumv->localPlayer->state = 2;
        lumv->localPlayer->h = 1;

        buttonPressed = true;
    }
    else
    {
        lumv->localPlayer->h = 0;
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
    ;
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

    for (int i = 0; i < 8; i++ )
    {
        if (lumberjackCollisionDebugTiles[i].type == -1)
        {
            if (lumberjackCollisionDebugTiles[i].index == (ty * 18) + tx)
                return &lumberjackCollisionDebugTiles[i];

            lumberjackCollisionDebugTiles[i].index = (ty * 18) + tx;
            lumberjackCollisionDebugTiles[i].x = tx;
            lumberjackCollisionDebugTiles[i].y = ty;
            lumberjackCollisionDebugTiles[i].type = lumv->tiles[(ty * 18) + tx];

            return &lumberjackCollisionDebugTiles[i];
        }
    }
    
    return NULL;
}

static bool lumberjackIsCollisionTile(int index)
{
    if (index == 0 || index == 1 || index == 6 || index == 5 || index == 10)
        return false;


    return true;
}


// static void lumberjackMenuCb(const char* label, bool selected, uint32_t settingVal)
// {
//     printf("");
// }