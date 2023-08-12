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
static void lumberjackAudioCallback(uint16_t* samples, uint32_t sampleCnt);
static void lumberjackBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);
static void lumberjackEspNowRecvCb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len, int8_t rssi);
static void lumberjackEspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status);
static int16_t lumberjackAdvancedUSB(uint8_t* buffer, uint16_t length, uint8_t isGet);

static void lumberjackConCb(p2pInfo* p2p, connectionEvt_t evt);
static void lumberjackMsgRxCb(p2pInfo* p2p, const uint8_t* payload, uint8_t len);
static void lumberjackMsgTxCbFn(p2pInfo* p2p, messageStatus_t status, const uint8_t* data, uint8_t len);

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
    .fnAdvancedUSB            = lumberjackAdvancedUSB,
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

    //Init menu :(

    //bzrStop(); // Stop the buzzer?
    p2pInitialize(&lumv->p2p, 'd', lumberjackConCb, lumberjackMsgRxCb, -70);
    p2pStartConnection(&lumv->p2p);

    const uint8_t testMsg[] = {0x01, 0x02, 0x03, 0x04};
    p2pSendMsg(&lumv->p2p, testMsg, ARRAY_SIZE(testMsg), lumberjackMsgTxCbFn);

    lumv->worldTimer = 0;

    //High score stuff?

    //Setup first level
    lumberjackSetupLevel(0);
}

void lumberjackSetupLevel(int index)
{
    ESP_LOGI("LUM", "Hi");


    lumv->localPlayer = calloc (1, sizeof(lumberjackHero_t));
    lumberjackSetupPlayer(lumv->localPlayer, 0);
    lumberjackSpawnPlayer(lumv->localPlayer, 150, 160, 0);

    // lumv->remotePlayer = calloc (1, sizeof(lumberjackHero_t));
    // lumberjackSetupPlayer(lumv->remotePlayer, 1);
    // lumberjackSpawnPlayer(lumv->remotePlayer, 100, 160, 1);
    
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
    //Check Controls
    //lumberjackDoControls();
  
    //update animations
    lumv->worldTimer += elapsedUs;
    if (lumv->worldTimer > 150500)
    {
        lumv->worldTimer -= 150500;
    }

    // lumv->remotePlayer->timerFrameUpdate += elapsedUs;

    // if (lumv->remotePlayer->timerFrameUpdate > 150500)
    // {
    //     lumv->remotePlayer->currentFrame++;
    //     lumv->remotePlayer->currentFrame %= 3;
    //     lumv->remotePlayer->timerFrameUpdate -= 150500;
    // }

    lumv->localPlayer->timerFrameUpdate += elapsedUs;

    if (lumv->localPlayer->timerFrameUpdate > lumv->localPlayer->animationSpeed)
    {
        lumv->localPlayer->currentFrame++;
        lumv->localPlayer->timerFrameUpdate -= lumv->localPlayer->animationSpeed;
    }


    //Draw section
    fillDisplayArea ( 0,0, 300,256,c145); 	

    //Redraw bottom
    drawWsgSimple(&lumv->floorTiles[0], 16, 208);
    drawWsgSimple(&lumv->floorTiles[5], 16, 224);
    for (int i = 0; i < 13; i++)
    {
        drawWsgSimple(&lumv->floorTiles[1], 32 + (i * 16), 208);
        drawWsgSimple(&lumv->floorTiles[7], 32 + (i * 16), 224);
    }

    drawWsgSimple(&lumv->floorTiles[2], 240, 176);
    drawWsgSimple(&lumv->floorTiles[3], 240, 192);
    drawWsgSimple(&lumv->floorTiles[4], 240, 208);
    drawWsgSimple(&lumv->floorTiles[9], 240, 224);

    int currentFrame = lumberjackGetPlayerAnimation(lumv->localPlayer);
    ESP_LOGI("LUM", "%ld %d", sizeof(int), currentFrame);
    //drawWsg(&lumv->remotePlayer->frames[lumv->remotePlayer->currentFrame], lumv->remotePlayer->x, lumv->remotePlayer->y, lumv->remotePlayer->flipped, false, 0);
    drawWsg(&lumv->localPlayer->frames[currentFrame], lumv->localPlayer->x, lumv->localPlayer->y, lumv->localPlayer->flipped, false, 0);
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

static int16_t lumberjackAdvancedUSB(uint8_t* buffer, uint16_t length, uint8_t isGet)
{
    return 0;
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