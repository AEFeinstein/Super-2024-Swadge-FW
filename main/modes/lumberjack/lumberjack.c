//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include <esp_log.h>

#include "menu.h"

// For lumberjack
#include "lumberjack.h"
#include "lumberjackGame.h"

static void lumberjackEnterMode(void);
static void lumberjackExitMode(void);
static void lumberjackMainLoop(int64_t elapsedUs);
static void lumberjackMenuLoop(int64_t elapsedUs);
static void lumberjackAudioCallback(uint16_t* samples, uint32_t sampleCnt);
static void lumberjackBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);
static void lumberjackEspNowRecvCb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len, int8_t rssi);
static void lumberjackEspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status);

static void lumberjackConCb(p2pInfo* p2p, connectionEvt_t evt);
static void lumberjackMsgRxCb(p2pInfo* p2p, const uint8_t* payload, uint8_t len);
static void lumberjackMsgTxCbFn(p2pInfo* p2p, messageStatus_t status, const uint8_t* data, uint8_t len);


static void lumberjackMenuCb(const char*, bool selected, uint32_t settingVal);


static const char lumberjackName[] = "Lumberjack";
static const char lumberjackPanic[] = "Panic";
static const char lumberjackAttack[] = "Attack";
static const char lumberjackBack[] = "Back";



swadgeMode_t lumberjackMode = {
    .modeName                 = lumberjackName,
    .wifiMode                 = ESP_NOW_IMMEDIATE,
    .overrideUsb              = false,
    .usesAccelerometer        = false,
    .usesThermometer          = false,
    .fnEnterMode              = lumberjackEnterMode,
    .fnExitMode               = lumberjackExitMode,
    .fnMainLoop               = lumberjackMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = lumberjackBackgroundDrawCallback,
    .fnEspNowRecvCb           = lumberjackEspNowRecvCb,
    .fnEspNowSendCb           = lumberjackEspNowSendCb,
    .fnAdvancedUSB            = NULL,
};

lumberjack_t* lumberjack = NULL;

static void lumberjackEnterMode(void)
{
    // Allocate and clear all memory for the menu mode.
    lumberjack = calloc(1, sizeof(lumberjack_t));

    loadFont("ibm_vga8.font", &lumberjack->ibm, false);
    loadFont("logbook.font", &lumberjack->logbook, false);

    lumberjack->menu = initMenu(lumberjackName, lumberjackMenuCb);
    lumberjack->menuLogbookRenderer = initMenuLogbookRenderer(&lumberjack->logbook);

    addSingleItemToMenu(lumberjack->menu, lumberjackPanic);
    addSingleItemToMenu(lumberjack->menu, lumberjackAttack);

    lumberjack->screen = LUMBERJACK_MENU;
    
    p2pInitialize(&lumberjack->p2p, 0x13, lumberjackConCb, lumberjackMsgRxCb, -70);
    p2pStartConnection(&lumberjack->p2p);
    //Init menu :(

    bzrStop(); // Stop the buzzer?
    

    
    //High score stuff?

    //Setup first level

    lumberjack->screen = LUMBERJACK_A;
    lumberjackStartGameMode(LUMBERJACK_PANIC);

    
}



static void lumberjackExitMode(void)
{    
    p2pDeinit(&lumberjack->p2p);
    freeFont(&lumberjack->ibm);
    freeFont(&lumberjack->logbook);
    deinitMenu(lumberjack->menu);
    free(lumberjack);

 
}

static void lumberjackMainLoop(int64_t elapsedUs)
{

    switch (lumberjack->screen)
    {
        case LUMBERJACK_MENU:
        {
            ESP_LOGI(LUM_TAG, "Menu");
            lumberjackMenuLoop(elapsedUs);
            break;
        }
        case LUMBERJACK_A:
        case LUMBERJACK_B:
        {
            lumberjackGameLoop(elapsedUs);
            break;
        }
    }
}


static void lumberjackMenuLoop(int64_t elapsedUs)
{
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        lumberjack->menu = menuButton(lumberjack->menu, evt);
    }

    drawMenuLogbook(lumberjack->menu, lumberjack->menuLogbookRenderer, elapsedUs);
    
}


static void lumberjackAudioCallback(uint16_t* samples, uint32_t sampleCnt)
{
    
}

static void lumberjackBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
    fillDisplayArea(x, y, x + w, y + h, c145);

    //Are we drawing the game here?
}

static void lumberjackEspNowRecvCb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len, int8_t rssi)
{
}

static void lumberjackEspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status)
{
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


static void lumberjackMenuCb(const char* label, bool selected, uint32_t settingVal)
{
    ESP_LOGI("LUM", "Info ");
    if (selected)
    {
        if (label == lumberjackPanic)
        {
            ESP_LOGI("LUM", "Panic");   
            lumberjack->screen = LUMBERJACK_A;
            lumberjackStartGameMode(LUMBERJACK_PANIC);
        }
        else if (label == lumberjackAttack)
        {
            ESP_LOGI(LUM_TAG, "Attack");
            lumberjack->screen = LUMBERJACK_B;
            lumberjackStartGameMode(LUMBERJACK_ATTACK);
        }
        else if (label == lumberjackBack)
        {
            //.switchToSwadgeMode(&mainMenuMode);
        }
    }
}