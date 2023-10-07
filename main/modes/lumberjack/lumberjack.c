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
static void lumberjackEspNowRecvCb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len,
                                   int8_t rssi);
static void lumberjackEspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status);

static void lumberjackConCb(p2pInfo* p2p, connectionEvt_t evt);
static void lumberjackMsgRxCb(p2pInfo* p2p, const uint8_t* payload, uint8_t len);
static void lumberjackMsgTxCbFn(p2pInfo* p2p, messageStatus_t status, const uint8_t* data, uint8_t len);

static void lumberjackMenuCb(const char*, bool selected, uint32_t settingVal);

static void lumberjackJoinGame(void);

static const char lumberjackName[]   = "Lumber Jack";
static const char lumberjackPanic[]  = "Panic";
static const char lumberjackAttack[] = "Attack";
static const char lumberjackBack[]   = "Back";

// static const char lumberjackNone[]    = "None";
static const char lumberjackRedCharacter[]     = "Character: Red";
static const char lumberjackGreen[]            = "Character: Green";
static const char lumberjackSpecialCharacter[] = "Character: Special";

static const char lumberjackMenuSinglePlayer[]      = "Single Player";
static const char lumberjackMenuMultiPlayerHost[]   = "Multi-Player";
static const char lumberjackMenuMultiPlayerClient[] = "Multi-Player Join";

const char* LUM_TAG = "LUM";

swadgeMode_t lumberjackMode = {
    .modeName                 = lumberjackName,
    .wifiMode                 = ESP_NOW_IMMEDIATE,
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

lumberjack_t* lumberjack = NULL;

static void lumberjackEnterMode(void)
{
    ESP_LOGI(LUM_TAG, "Lumber!");
    // Allocate and clear all memory for the menu mode.
    lumberjack = calloc(1, sizeof(lumberjack_t));

    loadFont("logbook.font", &lumberjack->logbook, false);

    lumberjack->menu                = initMenu(lumberjackName, lumberjackMenuCb);
    lumberjack->menuLogbookRenderer = initMenuLogbookRenderer(&lumberjack->logbook);

    lumberjack->gameMode  = LUMBERJACK_MODE_NONE;
    lumberjack->networked = false;
    lumberjack->host      = false;

    lumberjack->menu = startSubMenu(lumberjack->menu, lumberjackPanic);
    addSingleItemToMenu(lumberjack->menu, lumberjackMenuSinglePlayer);
    addSingleItemToMenu(lumberjack->menu, lumberjackMenuMultiPlayerHost);
    addSingleItemToMenu(lumberjack->menu, lumberjackMenuMultiPlayerClient);
    lumberjack->menu = endSubMenu(lumberjack->menu);

    lumberjack->menu = startSubMenu(lumberjack->menu, lumberjackAttack);
    addSingleItemToMenu(lumberjack->menu, lumberjackMenuSinglePlayer);
    addSingleItemToMenu(lumberjack->menu, lumberjackMenuMultiPlayerHost);
    addSingleItemToMenu(lumberjack->menu, lumberjackMenuMultiPlayerClient);
    lumberjack->menu = endSubMenu(lumberjack->menu);

    if (true) // Ignore this line
    {
        static const char* defaultCharacters[] = {lumberjackRedCharacter, lumberjackGreen};

        addMultiItemToMenu(lumberjack->menu, defaultCharacters, ARRAY_SIZE(defaultCharacters), 0);
    }
    else
    {
        static const char* defaultCharacterswUnlocks[]
            = {lumberjackRedCharacter, lumberjackGreen, lumberjackSpecialCharacter};

        addMultiItemToMenu(lumberjack->menu, defaultCharacterswUnlocks, ARRAY_SIZE(defaultCharacterswUnlocks), 0);
    }

    lumberjack->screen = LUMBERJACK_MENU;

    // Lumberjack. Game 19
    //  Init menu :(

    bzrStop(); // Stop the buzzer?

    // High score stuff?
    // Unlockables ? Save data?
}

static void lumberjackJoinGame(void)
{
    if (lumberjack->gameMode == LUMBERJACK_MODE_PANIC)
    {
        lumberjack->screen = LUMBERJACK_A;
        lumberjackStartGameMode(lumberjack, lumberjack->selected);
        return;
    }

    if (lumberjack->gameMode == LUMBERJACK_MODE_ATTACK)
    {
        lumberjack->screen = LUMBERJACK_B;
        lumberjackStartGameMode(lumberjack, lumberjack->selected);
        return;
    }

    lumberjack->screen = LUMBERJACK_MENU;
}

static void lumberjackExitMode(void)
{
    lumberjackExitGameMode();

    p2pDeinit(&lumberjack->p2p);
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

    // Are we drawing the game here?
}

//==============================================================================
// ESP_NOW
//==============================================================================
static void lumberjackEspNowRecvCb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len,
                                   int8_t rssi)
{
    // ESP_LOGI(LUM_TAG, "Getting: %d", (uint8_t)&data);
    p2pRecvCb(&lumberjack->p2p, esp_now_info->src_addr, data, len, rssi);
}

static void lumberjackEspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status)
{
    p2pSendCb(&lumberjack->p2p, mac_addr, status);
}

static void lumberjackConCb(p2pInfo* p2p, connectionEvt_t evt)
{
    // Do anything
    if (evt == CON_ESTABLISHED)
    {
        ESP_LOGI(LUM_TAG, "LumberJack.Net ready! %d", (int)p2pGetPlayOrder(p2p));

        if (GOING_FIRST == p2pGetPlayOrder(p2p))
        {
            ESP_LOGI(LUM_TAG, "HOST?");
            const uint8_t testMsg[] = {0x01, 0x02, 0x03, 0x04};

            p2pSendMsg(&lumberjack->p2p, testMsg, ARRAY_SIZE(testMsg), lumberjackMsgTxCbFn);
        }
    }

    if (evt == CON_LOST)
    {
        // Do we attempt to get it back?
        ESP_LOGW(LUM_TAG, "We lost connection!");
    }

    lumberjack->conStatus = evt;
}

static void lumberjackMsgRxCb(p2pInfo* p2p, const uint8_t* payload, uint8_t len)
{
    // Do anything

    if (len > 1)
    {
        if (payload[0] == 0x19)
        {
            int locX      = (int)payload[1] << 0 | (uint32_t)payload[2] << 8;
            int locY      = (int)payload[3] << 0 | (uint32_t)payload[4] << 8;
            uint8_t frame = (uint8_t)payload[5];
            printf("Got %d,%d %d|", locX, locY, frame);

            lumberjackUpdateRemote(locX, locY, frame);
        }
    }

    printf("Received %d %d!\n", *payload, len);
}

void lumberjackSendAttack(int number)
{
    if (lumberjack->networked)
    {
        const uint8_t testMsg[] = {0x13};
        p2pSendMsg(&lumberjack->p2p, testMsg, ARRAY_SIZE(testMsg), lumberjackMsgTxCbFn);
    }
}

void lumberjackUpdateLocation(int ghostX, int ghostY, int frame)
{
    const uint8_t locationMessage[6]
        = {0x19, (uint8_t)(ghostX >> 0), (uint8_t)(ghostX >> 8), (uint8_t)(ghostY >> 0), (uint8_t)(ghostY >> 8), frame};
    p2pSendMsg(&lumberjack->p2p, locationMessage, ARRAY_SIZE(locationMessage), lumberjackMsgTxCbFn);
}

/**
 * @brief TODO use this somewhere
 *
 * @param p2p
 * @param status
 * @param data
 * @param len
 */
static void lumberjackMsgTxCbFn(p2pInfo* p2p, messageStatus_t status, const uint8_t* data, uint8_t len)
{
}

static void lumberjackMenuCb(const char* label, bool selected, uint32_t settingVal)
{
    ESP_LOGI(LUM_TAG, "Info ");
    if (selected)
    {
        if (label == lumberjackPanic)
        {
            ESP_LOGI(LUM_TAG, "Panic");
            lumberjack->gameMode = LUMBERJACK_MODE_PANIC;
            // lumberjack->screen = LUMBERJACK_A;
            // lumberjackStartGameMode(LUMBERJACK_MODE_PANIC, lumberjack->selected);
        }
        else if (label == lumberjackAttack)
        {
            ESP_LOGI(LUM_TAG, "Attack");
            lumberjack->gameMode = LUMBERJACK_MODE_ATTACK;

            // lumberjack->screen = LUMBERJACK_B;
            // lumberjackStartGameMode(LUMBERJACK_MODE_ATTACK, lumberjack->selected);
        }

        if (label == lumberjackMenuMultiPlayerHost)
        {
            p2pInitialize(&lumberjack->p2p, 0x13, lumberjackConCb, lumberjackMsgRxCb, -70);
            p2pStartConnection(&lumberjack->p2p);
            lumberjack->networked = true;
            lumberjack->host      = true;
            lumberjackJoinGame();
        }
        else if (label == lumberjackMenuMultiPlayerClient)
        {
            p2pInitialize(&lumberjack->p2p, 0x13, lumberjackConCb, lumberjackMsgRxCb, -70);
            p2pStartConnection(&lumberjack->p2p);
            lumberjack->networked = true;
            lumberjack->host      = false;
            lumberjackJoinGame();
        }
        else if (label == lumberjackMenuSinglePlayer)
        {
            lumberjack->networked = false;
            lumberjack->host      = false;
            lumberjackJoinGame();
        }

        if (label == lumberjackRedCharacter)
        {
            lumberjack->selected = 0;
        }
        else if (label == lumberjackGreen)
        {
            lumberjack->selected = 1;
        }
        else if (label == lumberjackSpecialCharacter)
        {
            lumberjack->selected = 2;
        }

        if (label == lumberjackBack)
        {
            //.switchToSwadgeMode(&mainMenuMode);
        }
    }
    else
    {
        if (label == lumberjackRedCharacter)
        {
            lumberjack->selected = 0;
        }
        else if (label == lumberjackGreen)
        {
            lumberjack->selected = 1;
        }
        else if (label == lumberjackSpecialCharacter)
        {
            lumberjack->selected = 2;
        }
    }
}