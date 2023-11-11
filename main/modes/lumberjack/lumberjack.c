//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include <esp_log.h>

#include "menu.h"

// For lumberjack
#include "lumberjack.h"
#include "lumberjackGame.h"

#define LUMBERJACK_VLEN             7
#define LUMBERJACK_VERSION          "231111a"


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
static bool lumberjackChoUnlocked(void);
static bool lumberjackSpecialUnlocked(void);

static const char lumberjackName[]   = "Lumber Jack";
static const char lumberjackPanic[]  = "Panic";
static const char lumberjackAttack[] = "Attack";
static const char lumberjackBack[]   = "Back";

// static const char lumberjackNone[]    = "None";
static char lumberjackRedCharacter[]          = "Character: Red";
static char lumberjackGreenCharacter[]        = "Character: Green";
static char lumberjackSpecialCharacter[]      = "Character: Guy";
static char lumberjackChoCharacter[]          = "Character: Cho";

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
    .overrideSelectBtn        = false,
    .fnEnterMode              = lumberjackEnterMode,
    .fnExitMode               = lumberjackExitMode,
    .fnMainLoop               = lumberjackMainLoop,
    .fnAudioCallback          = lumberjackAudioCallback,
    .fnBackgroundDrawCallback = lumberjackBackgroundDrawCallback,
    .fnEspNowRecvCb           = lumberjackEspNowRecvCb,
    .fnEspNowSendCb           = lumberjackEspNowSendCb,
    .fnAdvancedUSB            = NULL,
};

typedef enum
{
    VERSION_MSG,
    READY_MSG,
    ATTACK_MSG,
    DEATH_MSG,
    SCORE_MSG
} lumberjackMessageType_t;

lumberjack_t* lumberjack = NULL;

static void lumberjackEnterMode(void)
{
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

    int characters = 2;

    if (lumberjackChoUnlocked())
    {
        characters++;
    }

    if (lumberjackSpecialUnlocked())
    {
        characters++;
    }

    const char** charactersArray = calloc(characters, sizeof(char*));

    charactersArray[0] = lumberjackRedCharacter;
    charactersArray[1] = lumberjackGreenCharacter;

    int index = 2;

    if (lumberjackChoUnlocked())
    {
        charactersArray[index++] = lumberjackChoCharacter;
    }

    if (lumberjackSpecialUnlocked())
    {
        charactersArray[index++] = lumberjackSpecialCharacter;
    }

    addMultiItemToMenu(lumberjack->menu, charactersArray, characters, 0);
 
    lumberjack->screen = LUMBERJACK_MENU;

    // Lumberjack. Game 19
    //  Init menu :(

    bzrStop(true); // Stop the buzzer?

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
// UNLOCKS
//==============================================================================

static bool lumberjackChoUnlocked()
{
    return true;
}

static bool lumberjackSpecialUnlocked()
{
    return true;
}

//==============================================================================
// ESP_NOW
//==============================================================================
static void lumberjackEspNowRecvCb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len,
                                   int8_t rssi)
{
    //ESP_LOGI(LUM_TAG, "Getting: %d %d", len, rssi);
    for (int i = 0; i < len; i++)
    {
        //ESP_LOGI(LUM_TAG, "data %d) %d", i, data[i]);
    }
    p2pRecvCb(&lumberjack->p2p, esp_now_info->src_addr, (const uint8_t*)data, len, rssi);
}

static void lumberjackEspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status)
{
    ESP_LOGI(LUM_TAG, "STATUS %d", status);
    p2pSendCb(&lumberjack->p2p, mac_addr, status);
}

static void lumberjackConCb(p2pInfo* p2p, connectionEvt_t evt)
{
    // Do anything
    if (evt == CON_ESTABLISHED)
    {
        ESP_LOGI(LUM_TAG, "LumberJack.Net ready! %d", (int)p2pGetPlayOrder(p2p));

        uint8_t payload[1 + LUMBERJACK_VLEN] = {VERSION_MSG};
        memcpy(&payload[1], LUMBERJACK_VERSION, LUMBERJACK_VLEN);

        /*
        if (GOING_FIRST == p2pGetPlayOrder(p2p))
        {
            const uint8_t testMsg[] = {0x01, 0x02, 0x03, 0x04};
            p2pSendMsg(&lumberjack->p2p, testMsg, ARRAY_SIZE(testMsg), lumberjackMsgTxCbFn);
        }
        else
        {
            const uint8_t testMsg[] = {0x01, 0x02, 0x03, 0x04};
            p2pSendMsg(&lumberjack->p2p, testMsg, ARRAY_SIZE(testMsg), lumberjackMsgTxCbFn);                    
        }*/

        p2pSendMsg(&lumberjack->p2p, payload, sizeof(payload), lumberjackMsgTxCbFn);
        lumberjackGameReady();
    }

    if (evt == CON_LOST)
    {
        // Do we attempt to get it back?
        ESP_LOGI(LUM_TAG, "We lost connection!");
    }

    lumberjack->conStatus = evt;
}

static void lumberjackMsgRxCb(p2pInfo* p2p, const uint8_t* payload, uint8_t len)
{
    // Do anything
    ESP_LOGI(LUM_TAG, "Ya boi got something! %d", (uint8_t)payload[0]);

    if (len > 0)
    {
        if (payload[0] == VERSION_MSG)
        {
            ESP_LOGI(LUM_TAG, "Version received!");
            if (memcmp(&payload[1], LUMBERJACK_VERSION, LUMBERJACK_VLEN))
            {
                ESP_LOGI("LUM_TAG", "We're in!");
            }
        }

        if (payload[0] == READY_MSG)
        {
            printf ("Test message");
            lumberjackPlayGame();
        }

        if (payload[0] == ATTACK_MSG)
        {
            ESP_LOGI(LUM_TAG, "Incoming attack!");
            lumberjackOnReceiveAttack(payload, len);
        }

        if (payload[0] == SCORE_MSG)
        {
            lumberjackOnReceiveScore(payload);
        }

        if (payload[0] == DEATH_MSG)
        {
            ESP_LOGI(LUM_TAG, "Playher died!");
            lumberjackOnReceiveDeath(payload[1] != 0x00);
        }

        if (payload[0] == 0x19)
        {
            int locX      = (int)payload[1] << 0 | (uint32_t)payload[2] << 8;
            int locY      = (int)payload[3] << 0 | (uint32_t)payload[4] << 8;
            uint8_t frame = (uint8_t)payload[5];
            printf("Got %d,%d %d|", locX, locY, frame);

        }
    }


    printf("Received %d %d!\n", *payload, len);
}

void lumberjackSendGo(void)
{
    if (lumberjack->networked)
    {
        const uint8_t msg[] = {READY_MSG};
        p2pSendMsg(&lumberjack->p2p, msg, ARRAY_SIZE(msg), lumberjackMsgTxCbFn);
    }
}

void lumberjackSendAttack(uint8_t* number)
{
    if (lumberjack->networked)
    {
        uint8_t payload[9] = {ATTACK_MSG};
        memcpy(&payload[1], number, 8);
        for (int i = 0; i < ARRAY_SIZE(payload); i++)
        {
            ESP_LOGI(LUM_TAG, "SENDING %d) %d", i, payload[i]);
        }
        p2pSendMsg(&lumberjack->p2p, payload, ARRAY_SIZE(payload), lumberjackMsgTxCbFn);
    }
}

void lumberjackSendScore(int score)
{
    if (lumberjack->networked)
    {

        uint8_t payload[4] = {SCORE_MSG};

        payload[3] = score >> 16 & 0xFF;
        payload[2] = score >> 8  & 0xFF;
        payload[1] = score       & 0xFF;
        p2pSendMsg(&lumberjack->p2p, payload, ARRAY_SIZE(payload), lumberjackMsgTxCbFn);
    }
}

void lumberjackSendDeath(bool gameover)
{
    if (lumberjack->networked)
    {
        const uint8_t payload[2] = {DEATH_MSG, (gameover ? 0x01 : 0x00)};
        p2pSendMsg(&lumberjack->p2p, payload, ARRAY_SIZE(payload), lumberjackMsgTxCbFn);
    }
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
    if (selected)
    {
        if (lumberjack->screen != LUMBERJACK_MENU)
        {
            return;
        }

        if (label == lumberjackPanic)
        {
            ESP_LOGI(LUM_TAG, "Panic");
            lumberjack->gameMode = LUMBERJACK_MODE_PANIC;
        }
        else if (label == lumberjackAttack)
        {
            ESP_LOGI(LUM_TAG, "Attack");
            lumberjack->gameMode = LUMBERJACK_MODE_ATTACK;
        }

        if (label == lumberjackMenuMultiPlayerHost)
        {
            p2pInitialize(&lumberjack->p2p,(lumberjack->gameMode == LUMBERJACK_MODE_PANIC ? 0x13 : 0x15), lumberjackConCb, lumberjackMsgRxCb, -70);
            p2pSetAsymmetric(&lumberjack->p2p, (lumberjack->gameMode == LUMBERJACK_MODE_PANIC ? 0x14 : 0x16));
            p2pStartConnection(&lumberjack->p2p);
            lumberjack->networked = true;
            lumberjack->host      = true;
            lumberjackJoinGame();
        }
        else if (label == lumberjackMenuMultiPlayerClient)
        {
            p2pInitialize(&lumberjack->p2p, (lumberjack->gameMode == LUMBERJACK_MODE_PANIC ? 0x14 : 0x16), lumberjackConCb, lumberjackMsgRxCb, -70);
            p2pSetAsymmetric(&lumberjack->p2p, (lumberjack->gameMode == LUMBERJACK_MODE_PANIC ? 0x13 : 0x15));
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
        else if (label == lumberjackGreenCharacter)
        {
            lumberjack->selected = 1;
        }
        else if (label == lumberjackSpecialCharacter)
        {
            lumberjack->selected = 2;
        }
        else if (label == lumberjackChoCharacter)
        {
            lumberjack->selected = 3;
        }

        if (label == lumberjackBack)
        {
            //.switchToSwadgeMode(&mainMenuMode);
        }
    }
    else
    {

        if (label == lumberjackPanic)
        {
            ESP_LOGI(LUM_TAG, "Panic");
            lumberjack->gameMode = LUMBERJACK_MODE_PANIC;
        }
        else if (label == lumberjackAttack)
        {
            ESP_LOGI(LUM_TAG, "Attack");
            lumberjack->gameMode = LUMBERJACK_MODE_ATTACK;
        }
        
        if (label == lumberjackRedCharacter)
        {
            lumberjack->selected = 0;
        }
        else if (label == lumberjackGreenCharacter)
        {
            lumberjack->selected = 1;
        }
        else if (label == lumberjackSpecialCharacter)
        {
            lumberjack->selected = 2;
        }else if (label == lumberjackChoCharacter)
        {
            lumberjack->selected = 3;

        }
    }
}