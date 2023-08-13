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

void lumberjackSetupLevel(int index);

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
    ESP_LOGI("LUM", "Hy %d", index);

    lumv->yOffset = 0;

    lumv->localPlayer = calloc (1, sizeof(lumberjackHero_t));
    lumberjackSetupPlayer(lumv->localPlayer, 0);
    lumberjackSpawnPlayer(lumv->localPlayer, 94, 0, 0);

    // lumv->remotePlayer = calloc (1, sizeof(lumberjackHero_t));
    // lumberjackSetupPlayer(lumv->remotePlayer, 1);
    // lumberjackSpawnPlayer(lumv->remotePlayer, 100, 160, 1);
    memcpy(lumv->tiles ,(int[]) {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    4, 4, 4, 4, 4, 4, 4, 0, 0, 0, 0, 4, 4, 4, 4, 4, 4, 4,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 4, 4, 4, 4, 4, 4, 0, 0, 0, 0, 0, 0,
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
    0, 0, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 10, 0, 0, 0,
    }, sizeof lumv->tiles);

    memcpy(lumv->anim, (int[]){
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
    }, sizeof * lumv->anim);
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
  

    //Check physics
   //push down on player
    bool playerOnGround = false;
    
    //Take player's current y and shift down one pixel
    int checkPoint = lumv->localPlayer->y + 25;
    
    int tileCheck = 0;
    
    if (lumv->localPlayer->h > 0)
    {
        lumv->localPlayer->x += 5;
    }
    else if (lumv->localPlayer->h < 0)
    {
        lumv->localPlayer->x -= 5;
    }

    //World wrap
    lumv->localPlayer->x %= 295;
    if (lumv->localPlayer->x < -20)
    {
        lumv->localPlayer->x += 295;
    }
    
    //Crude
    lumv->localPlayer->tx = (int)(lumv->localPlayer->x / 16);

    if (lumv->localPlayer->tx < 0) lumv->localPlayer->tx = 0;
    if (lumv->localPlayer->tx > 18) lumv->localPlayer->tx = 18;

    lumv->localPlayer->ty = (int)(lumv->localPlayer->y / 16);

    if (lumv->localPlayer->ty < 0) lumv->localPlayer->ty = 0;
    if (lumv->localPlayer->ty > 18) lumv->localPlayer->ty = 18; //Kill player


    //(lumv->localPlayer->ty * 20)+lumv->localPlayer->tx
    //Do two checks
    int tx1,tx2,ty;
    for (int yMod = 0; yMod < 8; yMod++)
    {

        tx1 = (int)((lumv->localPlayer->x ) / 16);
        tx2 = (int)((lumv->localPlayer->x + 16) / 16);

        if (tx1 < 0) tx1 = 0;
        if (tx1 > 18) tx1 = 18;
        if (tx2 < 0) tx2 = 0;
        if (tx2 > 18) tx2 = 18;
        ty = (int)((lumv->localPlayer->y+48) / 16);//(int)((lumv->localPlayer->ty) / 16);

        if (ty < 0) ty = 0;
        if (ty > 20) ty = 20;// This is going to cause problems later

        int tileCheck1 = lumv->tiles[(ty * 18) + tx1];
        int tileCheck2 = lumv->tiles[(ty * 18) + tx2];

        playerOnGround = (tileCheck1 != 0 && tileCheck1 != 1 && tileCheck1 != 6 && tileCheck1 != 5 && tileCheck1 != 10);
        
        if (playerOnGround == false)
        {
            playerOnGround = (tileCheck2 != 0 && tileCheck2 != 1 && tileCheck2 != 6 && tileCheck2 != 5 && tileCheck2 != 10);
        }

        if (playerOnGround == false)
        {
            lumv->localPlayer->y++;
        }
        else
        {
            break;
        }

    }

    lumv->localPlayer->onGround = playerOnGround;

    //ESP_LOGI("LUM", "%d %d %d %d", tx1, tx2, lumv->localPlayer->y, ty);
    
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

    lumv->yOffset = lumv->localPlayer->y - 16;
    if (lumv->yOffset < 16) lumv->yOffset = 16;
    if (lumv->yOffset > 96) lumv->yOffset = 96;

    //Draw section
    fillDisplayArea ( 0,0, 280,256,c145); 	

    //Redraw bottom
    lumberjackTileMap();

    int currentFrame = lumberjackGetPlayerAnimation(lumv->localPlayer);
    //drawWsg(&lumv->remotePlayer->frames[lumv->remotePlayer->currentFrame], lumv->remotePlayer->x, lumv->remotePlayer->y, lumv->remotePlayer->flipped, false, 0);
    drawWsg(&lumv->localPlayer->frames[currentFrame], lumv->localPlayer->x, lumv->localPlayer->y - lumv->yOffset, lumv->localPlayer->flipped, false, 0);

    if (lumv->localPlayer->x > 270)
    {
        drawWsg(&lumv->localPlayer->frames[currentFrame], lumv->localPlayer->x - 295, lumv->localPlayer->y - lumv->yOffset, lumv->localPlayer->flipped, false, 0);
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
                    drawWsgSimple(&lumv->animationTiles[((animIndex - 1) * 4) + (lumv->liquidAnimationFrame % 4)],-4 + x * 16, (y * 16) - lumv->yOffset + 8);
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

// static void lumberjackMenuCb(const char* label, bool selected, uint32_t settingVal)
// {
//     printf("");
// }