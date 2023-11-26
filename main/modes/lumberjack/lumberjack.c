//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include <esp_log.h>

#include "mainMenu.h"
#include "menu.h"
#include "mode_ray.h"

// For lumberjack
#include "lumberjack.h"
#include "lumberjackGame.h"

#define LUMBERJACK_VLEN    7
#define LUMBERJACK_VERSION "231121b"

#define DEFAULT_HIGHSCORE     5000
#define DEFAULT_FRAME_RATE_US 40000

#define CHARACTER_RED   0
#define CHARACTER_GREEN 1
#define CHARACTER_GUY   2
#define CHARACTER_CHO   3

static void lumberjackEnterMode(void);
static void lumberjackExitMode(void);
static void lumberjackMainLoop(int64_t elapsedUs);
static void lumberjackMenuLoop(int64_t elapsedUs);
static void lumberjackAudioCallback(uint16_t* samples, uint32_t sampleCnt);
static void lumberjackBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);
static void lumberjackInstructionText(const char* string, paletteColor_t color, int locationX, int locationY);
static void lumberjackEspNowRecvCb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len,
                                   int8_t rssi);
static void lumberjackEspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status);

static void lumberjackConCb(p2pInfo* p2p, connectionEvt_t evt);
static void lumberjackMsgRxCb(p2pInfo* p2p, const uint8_t* payload, uint8_t len);
static void lumberjackMsgTxCbFn(p2pInfo* p2p, messageStatus_t status, const uint8_t* data, uint8_t len);

static void lumberjackMenuCb(const char*, bool selected, uint32_t settingVal);

static void lumberjackLoadSave(void);
static void lumberjackJoinGame(void);
static bool lumberjackChoUnlocked(void);
static bool lumberjackSwadgeGuyUnlocked(void);

const char lumberjackName[]                = "Lumber Jacks";
static const char lumberjackPanic[]        = "Panic";
static const char lumberjackAttack[]       = "Attack";
static const char lumberjackInstructions[] = "How to Play";
static const char lumberjackExit[]         = "Exit";

static char lumberjackRedCharacter[]     = "Character: Red";
static char lumberjackGreenCharacter[]   = "Character: Green";
static char lumberjackSpecialCharacter[] = "Character: Guy";
static char lumberjackChoCharacter[]     = "Character: Cho";

static const char lumberjackMenuSinglePlayer[] = "Single Player";
static const char lumberjackMenuMultiPlayer[]  = "Multi-Player";

static const char lumberjackPlatformerUnlock[] = "pf_scores";
static const char lumberjackChoUnlock[]        = "ray";
const char* LUM_TAG                            = "LUM";
const char LUMBERJACK_SAVE[]                   = "lumberjackdata";

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
    SCORE_MSG,
    WATER_MSG,
    HOST_MSG,
    CHARACTER_MSG,
    BUMP_MSG,
    HIGHSCORE_MSG
} lumberjackMessageType_t;

lumberjack_t* lumberjack = NULL;

static void lumberjackEnterMode(void)
{
    // Allocate and clear all memory for the menu mode.
    lumberjack = calloc(1, sizeof(lumberjack_t));

    lumberjackLoadSave();

    loadFont("logbook.font", &lumberjack->logbook, false);
    loadFont("eightbit_atari_grube2.font", &lumberjack->arcade, false);
    lumberjack->menu                = initMenu(lumberjackName, lumberjackMenuCb);
    lumberjack->menuLogbookRenderer = initMenuLogbookRenderer(&lumberjack->logbook);

    lumberjack->gameMode  = LUMBERJACK_MODE_NONE;
    lumberjack->networked = false;
    lumberjack->host      = false;

    lumberjack->menu = startSubMenu(lumberjack->menu, lumberjackPanic);
    addSingleItemToMenu(lumberjack->menu, lumberjackMenuSinglePlayer);
    addSingleItemToMenu(lumberjack->menu, lumberjackMenuMultiPlayer);
    addSingleItemToMenu(lumberjack->menu, lumberjackInstructions);
    lumberjack->menu = endSubMenu(lumberjack->menu);

    lumberjack->menu = startSubMenu(lumberjack->menu, lumberjackAttack);
    addSingleItemToMenu(lumberjack->menu, lumberjackMenuSinglePlayer);
    addSingleItemToMenu(lumberjack->menu, lumberjackMenuMultiPlayer);
    addSingleItemToMenu(lumberjack->menu, lumberjackInstructions);
    lumberjack->menu = endSubMenu(lumberjack->menu);

    int characters = 2;

    if (lumberjack->save.choUnlocked)
    {
        characters++;
    }

    if (lumberjack->save.swadgeGuyUnlocked)
    {
        characters++;
    }

    lumberjack->charactersArray = calloc(characters, sizeof(char*));

    lumberjack->charactersArray[0] = lumberjackRedCharacter;
    lumberjack->charactersArray[1] = lumberjackGreenCharacter;

    int index           = 2;
    int selectedDefault = lumberjack->save.character;

    if (lumberjack->save.swadgeGuyUnlocked)
    {
        if (lumberjack->save.character == CHARACTER_GUY)
        {
            selectedDefault = index;
        }
        lumberjack->charactersArray[index++] = lumberjackSpecialCharacter;
    }

    if (lumberjack->save.choUnlocked)
    {
        if (lumberjack->save.character == CHARACTER_CHO)
        {
            selectedDefault = index;
        }
        lumberjack->charactersArray[index++] = lumberjackChoCharacter;
    }

    addMultiItemToMenu(lumberjack->menu, lumberjack->charactersArray, characters, selectedDefault);
    addSingleItemToMenu(lumberjack->menu, lumberjackExit);

    lumberjack->screen = LUMBERJACK_MENU;
    // Turn off LEDs
}

static void lumberjackLoadSave(void)
{
    size_t len = sizeof(lumberjackUnlock_t);

    readNvsBlob(LUMBERJACK_SAVE, &lumberjack->save, &len);
    if (lumberjack->save.choUnlocked == false)
    {
        lumberjack->save.choUnlocked = lumberjackChoUnlocked();
    }

    if (lumberjack->save.swadgeGuyUnlocked == false)
    {
        lumberjack->save.swadgeGuyUnlocked = lumberjackSwadgeGuyUnlocked();
    }
    // writeNvsBlob(breakoutNvsKey_scores, &(self->highScores), size);

    int highscore = lumberjack->save.highScore;

    if (highscore < DEFAULT_HIGHSCORE)
    {
        lumberjack->save.highScore = DEFAULT_HIGHSCORE;
    }

    if (lumberjack->save.attackHighScore < DEFAULT_HIGHSCORE)
    {
        lumberjack->save.attackHighScore = DEFAULT_HIGHSCORE;
    }

    if (lumberjack->save.panicHighScore < DEFAULT_HIGHSCORE)
    {
        lumberjack->save.panicHighScore = DEFAULT_HIGHSCORE;
    }

    if (lumberjack->save.character == CHARACTER_GREEN)
    {
        lumberjack->selected      = CHARACTER_GREEN;
        lumberjack->playerColor.r = 0;
        lumberjack->playerColor.g = 255;
        lumberjack->playerColor.b = 0;
    }
    else if (lumberjack->save.character == CHARACTER_CHO)
    {
        lumberjack->selected      = CHARACTER_CHO;
        lumberjack->playerColor.r = 0;
        lumberjack->playerColor.g = 255;
        lumberjack->playerColor.b = 255;
    }
    else if (lumberjack->save.character == CHARACTER_GUY)
    {
        lumberjack->selected      = CHARACTER_GUY;
        lumberjack->playerColor.r = 0;
        lumberjack->playerColor.g = 255;
        lumberjack->playerColor.b = 255;
    }
    else
    {
        lumberjack->selected      = CHARACTER_RED;
        lumberjack->playerColor.r = 255;
        lumberjack->playerColor.g = 0;
        lumberjack->playerColor.b = 0;
    }

    lumberjack->save.character = lumberjack->selected;

    writeNvsBlob(LUMBERJACK_SAVE, &lumberjack->save, len);
}

void lumberjackSaveSave(void)
{
    ESP_LOGI(LUM_TAG, "Saving");
    size_t len = sizeof(lumberjackUnlock_t);
    writeNvsBlob(LUMBERJACK_SAVE, &lumberjack->save, len);
}

static void lumberjackJoinGame(void)
{
    if (lumberjack->gameMode == LUMBERJACK_MODE_PANIC)
    {
        lumberjack->screen         = LUMBERJACK_A;
        lumberjack->save.highScore = lumberjack->save.panicHighScore;
        lumberjackStartGameMode(lumberjack, lumberjack->selected);
        return;
    }

    if (lumberjack->gameMode == LUMBERJACK_MODE_ATTACK)
    {
        lumberjack->screen         = LUMBERJACK_B;
        lumberjack->save.highScore = lumberjack->save.attackHighScore;
        lumberjackStartGameMode(lumberjack, lumberjack->selected);
        return;
    }

    lumberjack->screen = LUMBERJACK_MENU;
}

static void lumberjackExitMode(void)
{
    lumberjackUnloadLevel();
    lumberjackExitGameMode();
    free(lumberjack->charactersArray);
    p2pDeinit(&lumberjack->p2p);
    freeFont(&lumberjack->logbook);
    freeFont(&lumberjack->arcade);
    deinitMenu(lumberjack->menu);
    deinitMenuLogbookRenderer(lumberjack->menuLogbookRenderer);
    free(lumberjack);

    lumberjack = NULL;
}

static void lumberjackMainLoop(int64_t elapsedUs)
{
    switch (lumberjack->screen)
    {
        case LUMBERJACK_MENU:
        {
            lumberjackMenuLoop(elapsedUs);
            for (int ledIdx = 0; ledIdx < ARRAY_SIZE(lumberjack->menuLogbookRenderer->leds); ledIdx++)
            {
                lumberjack->menuLogbookRenderer->ledTimers[ledIdx].periodUs = 1;
                lumberjack->menuLogbookRenderer->ledTimers[ledIdx].timerUs  = 0;
                lumberjack->menuLogbookRenderer->ledTimers[ledIdx].brightness
                    = 255; // lumberjack->menuLogbookRenderer->ledTimers[ledIdx].maxBrightness;

                lumberjack->menuLogbookRenderer->leds[ledIdx].r = lumberjack->playerColor.r;
                lumberjack->menuLogbookRenderer->leds[ledIdx].g = lumberjack->playerColor.g;
                lumberjack->menuLogbookRenderer->leds[ledIdx].b = lumberjack->playerColor.b;
            }

            setLeds(lumberjack->menuLogbookRenderer->leds, CONFIG_NUM_LEDS);

            break;
        }
        case LUMBERJACK_A:
        case LUMBERJACK_B:
        {
            lumberjackGameLoop(elapsedUs);
            break;
        }
    }

    if (lumberjack->connectionConfirmed > 0 && lumberjack->conStatus == CON_ESTABLISHED)
    {
        lumberjack->connectionConfirmed -= elapsedUs / 1000;

        if (lumberjack->connectionConfirmed > 0)
        {
            return;
        }

        lumberjack->connectionConfirmed      = 0;
        uint8_t payload[1 + LUMBERJACK_VLEN] = {VERSION_MSG};
        memcpy(&payload[1], LUMBERJACK_VERSION, LUMBERJACK_VLEN);

        p2pSendMsg(&lumberjack->p2p, payload, sizeof(payload), lumberjackMsgTxCbFn);

        // high score test
        uint8_t highscorePayload[4] = {HIGHSCORE_MSG};
        int score                   = (lumberjack->gameMode == LUMBERJACK_MODE_PANIC) ? lumberjack->save.panicHighScore
                                                                                      : lumberjack->save.attackHighScore;

        highscorePayload[3] = score >> 16 & 0xFF;
        highscorePayload[2] = score >> 8 & 0xFF;
        highscorePayload[1] = score & 0xFF;

        ESP_LOGI(LUM_TAG, "Sending %d", score);
        p2pSendMsg(&lumberjack->p2p, highscorePayload, sizeof(highscorePayload), lumberjackMsgTxCbFn);
    }
}

static void lumberjackMenuLoop(int64_t elapsedUs)
{
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        if (lumberjack->instructions)
        {
            if (evt.state & PB_B)
            {
                lumberjack->instructions = false;
            }
        }
        else
        {
            lumberjack->menu = menuButton(lumberjack->menu, evt);
        }
    }

    drawMenuLogbook(lumberjack->menu, lumberjack->menuLogbookRenderer, elapsedUs);

    if (lumberjack->instructions)
    {
        fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c145);

        const char* closeText = "Press B to close";
        int16_t cWidthH       = textWidth(&lumberjack->arcade, closeText);
        int16_t dOffset       = 15;
        int16_t dyOffset      = 50;
        int16_t iWidth        = 10;

        if (lumberjack->gameMode == LUMBERJACK_MODE_PANIC)
        {
            const char* titleText = "Panic";
            int16_t tWidthH       = textWidth(&lumberjack->arcade, titleText);
            lumberjackInstructionText(titleText, c555, (TFT_WIDTH - tWidthH) / 2, 15);

            const char* ins = "Press A to jump\nAvoid upright baddies\nHit baddies from under\nThen kick them "
                              "off\nDuck DOWN to avoid ghost\nAxe blocks lowers water\nDon't drown";
            dOffset         = 15;
            dyOffset        = 49;
            drawTextWordWrap(&lumberjack->arcade, c000, ins, &dOffset, &dyOffset, TFT_WIDTH - iWidth,
                             TFT_HEIGHT - dOffset);

            dOffset  = 15;
            dyOffset = 52;
            drawTextWordWrap(&lumberjack->arcade, c000, ins, &dOffset, &dyOffset, TFT_WIDTH - iWidth,
                             TFT_HEIGHT - dOffset);

            dOffset  = 14;
            dyOffset = 50;
            drawTextWordWrap(&lumberjack->arcade, c000, ins, &dOffset, &dyOffset, TFT_WIDTH - iWidth,
                             TFT_HEIGHT - dOffset);

            dOffset  = 16;
            dyOffset = 50;
            drawTextWordWrap(&lumberjack->arcade, c000, ins, &dOffset, &dyOffset, TFT_WIDTH - iWidth + 1,
                             TFT_HEIGHT - dOffset);

            dOffset  = 15;
            dyOffset = 50;
            drawTextWordWrap(&lumberjack->arcade, c355, ins, &dOffset, &dyOffset, TFT_WIDTH - iWidth,
                             TFT_HEIGHT - dOffset);
        }
        else if (lumberjack->gameMode == LUMBERJACK_MODE_ATTACK)
        {
            const char* titleText = "Attack";
            int16_t tWidthH       = textWidth(&lumberjack->arcade, titleText);
            lumberjackInstructionText(titleText, c555, (TFT_WIDTH - tWidthH) / 2, 15);

            const char* ins = "Press A to jump\nAvoid upright baddies\nHit baddies from under\nThen kick them "
                              "off\nDuck DOWN to avoid ghost\nAxe blocks give item\nPress B uses item\n  Upgrange: "
                              "Harder baddies\n  Impearvious: Invincible\n  Grapes O' Wrath: Flip baddies";

            dOffset  = 15;
            dyOffset = 49;
            drawTextWordWrap(&lumberjack->arcade, c000, ins, &dOffset, &dyOffset, TFT_WIDTH - iWidth,
                             TFT_HEIGHT - dOffset);

            dOffset  = 15;
            dyOffset = 52;
            drawTextWordWrap(&lumberjack->arcade, c000, ins, &dOffset, &dyOffset, TFT_WIDTH - iWidth,
                             TFT_HEIGHT - dOffset);

            dOffset  = 14;
            dyOffset = 50;
            drawTextWordWrap(&lumberjack->arcade, c000, ins, &dOffset, &dyOffset, TFT_WIDTH - iWidth,
                             TFT_HEIGHT - dOffset);

            dOffset  = 16;
            dyOffset = 50;
            drawTextWordWrap(&lumberjack->arcade, c000, ins, &dOffset, &dyOffset, TFT_WIDTH - iWidth + 1,
                             TFT_HEIGHT - dOffset);

            dOffset  = 15;
            dyOffset = 50;
            drawTextWordWrap(&lumberjack->arcade, c355, ins, &dOffset, &dyOffset, TFT_WIDTH - iWidth,
                             TFT_HEIGHT - dOffset);
        }

        dOffset  = 15;
        dyOffset = 50;

        // dOffset = 20;
        // drawText(&lumberjack->arcade, c555, closeText, (TFT_WIDTH-cWidthH)/2,(TFT_HEIGHT - dOffset - 5));
        lumberjackInstructionText(closeText, c555, (TFT_WIDTH - cWidthH) / 2, (TFT_HEIGHT - dOffset - 5));
    }
}

static void lumberjackInstructionText(const char* string, paletteColor_t color, int locationX, int locationY)
{
    drawText(&lumberjack->arcade, c000, string, locationX, locationY - 1);
    drawText(&lumberjack->arcade, c000, string, locationX, locationY + 2);
    drawText(&lumberjack->arcade, c000, string, locationX - 1, locationY);
    drawText(&lumberjack->arcade, c000, string, locationX + 1, locationY);
    drawText(&lumberjack->arcade, color, string, locationX, locationY);

    // drawTextWordWrap(&lumberjack->arcade, c555, string, &dOffset, &dOffset, TFT_WIDTH - dOffset, TFT_HEIGHT -
    // dOffset);
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
    rayPlayer_t cho;
    size_t size = sizeof(rayPlayer_t);

    return readNvsBlob(lumberjackChoUnlock, &cho, &(size));
}

static bool lumberjackSwadgeGuyUnlocked()
{
    pfHighScores_t highScores;
    size_t size = sizeof(pfHighScores_t);
    // Try reading the value

    return readNvsBlob(lumberjackPlatformerUnlock, &(highScores), &(size));
}

//==============================================================================
// ESP_NOW
//==============================================================================
static void lumberjackEspNowRecvCb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len,
                                   int8_t rssi)
{
    /*ESP_LOGD(LUM_TAG, "Getting: %d %d", len, rssi);
    for (int i = 0; i < len; i++)
    {
        ESP_LOGD(LUM_TAG, "data %d) %d", i, data[i]);
    }*/
    p2pRecvCb(&lumberjack->p2p, esp_now_info->src_addr, (const uint8_t*)data, len, rssi);
}

static void lumberjackEspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status)
{
    // ESP_LOGD(LUM_TAG, "STATUS %d", status);
    p2pSendCb(&lumberjack->p2p, mac_addr, status);
}

static void lumberjackConCb(p2pInfo* p2p, connectionEvt_t evt)
{
    switch (evt)
    {
        case CON_STARTED:
        case RX_GAME_START_ACK:
        case RX_GAME_START_MSG:
        {
            // TODO, optional, update menu to indicate connection progress
            ESP_LOGI(LUM_TAG, "LumberJack.Net ack! ");
            lumberjackQualityCheck();
            break;
        }
        case CON_ESTABLISHED:
        {
            int index = (int)p2pGetPlayOrder(p2p);
            ESP_LOGI(LUM_TAG, "LumberJack.Net ready! %d", index);
            lumberjack->host                     = (GOING_FIRST == index);
            uint8_t payload[1 + LUMBERJACK_VLEN] = {VERSION_MSG};
            memcpy(&payload[1], LUMBERJACK_VERSION, LUMBERJACK_VLEN);

            p2pSendMsg(&lumberjack->p2p, payload, sizeof(payload), lumberjackMsgTxCbFn);
            lumberjack->connectionConfirmed = 2000;

            lumberjackGameReady();
            break;
        }
        case CON_LOST:
        {
            // TODO drop back to main menu or show an error or something, its not recoverable
            ESP_LOGI(LUM_TAG, "We lost connection!");

            lumberjack->connLost = true;
            break;
        }
    }

    lumberjack->conStatus = evt;
}

static void lumberjackMsgRxCb(p2pInfo* p2p, const uint8_t* payload, uint8_t len)
{
    // Do anything
    // ESP_LOGI(LUM_TAG, "Ya boi got something! %d", (uint8_t)payload[0]);

    lumberjackQualityCheck();

    if (len > 0)
    {
        if (payload[0] == VERSION_MSG)
        {
            ESP_LOGD(LUM_TAG, "Version received!");
            if (memcmp(&payload[1], LUMBERJACK_VERSION, LUMBERJACK_VLEN))
            {
                ESP_LOGD("LUM_TAG", "We're in!");
            }
        }

        if (payload[0] == READY_MSG)
        {
            ESP_LOGD(LUM_TAG, "Test message");
            lumberjackPlayGame();
        }

        if (payload[0] == ATTACK_MSG)
        {
            ESP_LOGD(LUM_TAG, "Incoming attack!");
            lumberjackOnReceiveAttack(payload, len);
        }

        if (payload[0] == SCORE_MSG)
        {
            lumberjackOnReceiveScore(payload);
        }

        if (payload[0] == HIGHSCORE_MSG)
        {
            ESP_LOGI(LUM_TAG, "High score incoming");
            lumberjackOnReceiveHighScore(payload);
        }

        if (payload[0] == CHARACTER_MSG)
        {
            ESP_LOGI(LUM_TAG, "CH");
            lumberjackOnReceiveCharacter(payload[1]);
        }

        if (payload[0] == BUMP_MSG)
        {
            lumberjackOnReceiveBump();
        }

        if (payload[0] == DEATH_MSG)
        {
            lumberjackOnReceiveDeath(payload[1]);
        }

        if (payload[0] == HOST_MSG)
        {
            if (lumberjack->host == false)
            {
                lumberjack->host = true;
            }
        }

        /*
        if (payload[0] == 0x19)
        {
            //int locX      = (int)payload[1] << 0 | (uint32_t)payload[2] << 8;
            //int locY      = (int)payload[3] << 0 | (uint32_t)payload[4] << 8;
            //uint8_t frame = (uint8_t)payload[5];
            //ESP_LOGD(LUM_TAG,"Got %d,%d %d|", locX, locY, frame);

        }*/
    }

    ESP_LOGD(LUM_TAG, "Received %d %d!\n", *payload, len);
}

void lumberjackSendGo(void)
{
    if (lumberjack->networked)
    {
        const uint8_t msg[] = {READY_MSG};
        p2pSendMsg(&lumberjack->p2p, msg, ARRAY_SIZE(msg), lumberjackMsgTxCbFn);
    }
}

void lumberjackSendHostRequest(void)
{
    if (lumberjack->networked)
    {
        const uint8_t msg[] = {HOST_MSG};
        p2pSendMsg(&lumberjack->p2p, msg, ARRAY_SIZE(msg), lumberjackMsgTxCbFn);
    }
}

void lumberjackSendAttack(uint8_t* number)
{
    if (lumberjack->networked)
    {
        uint8_t payload[9] = {ATTACK_MSG};
        memcpy(&payload[1], number, 8);

        p2pSendMsg(&lumberjack->p2p, payload, ARRAY_SIZE(payload), lumberjackMsgTxCbFn);
    }
}

void lumberjackSendBump(void)
{
    if (lumberjack->networked)
    {
        uint8_t payload[1] = {BUMP_MSG};
        p2pSendMsg(&lumberjack->p2p, payload, ARRAY_SIZE(payload), lumberjackMsgTxCbFn);
    }
}

void lumberjackSendScore(int score)
{
    if (lumberjack->networked)
    {
        uint8_t payload[4] = {SCORE_MSG};

        payload[3] = score >> 16 & 0xFF;
        payload[2] = score >> 8 & 0xFF;
        payload[1] = score & 0xFF;
        p2pSendMsg(&lumberjack->p2p, payload, ARRAY_SIZE(payload), lumberjackMsgTxCbFn);
    }
}

void lumberjackSendCharacter(uint8_t character)
{
    if (lumberjack->networked)
    {
        uint8_t payload[2] = {CHARACTER_MSG, character};
        p2pSendMsg(&lumberjack->p2p, payload, ARRAY_SIZE(payload), lumberjackMsgTxCbFn);
    }
}

void lumberjackSendDeath(uint8_t lives)
{
    if (lumberjack->networked)
    {
        const uint8_t payload[2] = {DEATH_MSG, lives};
        p2pSendMsg(&lumberjack->p2p, payload, ARRAY_SIZE(payload), lumberjackMsgTxCbFn);
    }
}

/**
 * @brief This is called after transmitting a p2p packet and receiving (or not receiving) an ack
 *
 * @param p2p
 * @param status
 * @param data
 * @param len
 */
static void lumberjackMsgTxCbFn(p2pInfo* p2p, messageStatus_t status, const uint8_t* data, uint8_t len)
{
    switch (status)
    {
        case MSG_ACKED:
        {
            // All good
            break;
        }
        case MSG_FAILED:
        {
            // TODO figure out what to do if a message fails to transmit (i.e. no ack). Retry?
            ESP_LOGI(LUM_TAG, "Failed?");
            break;
        }
    }
}

void lumberjackInitp2p()
{
    p2pDeinit(&lumberjack->p2p);
    lumberjack->conStatus = CON_LOST;
    p2pInitialize(&lumberjack->p2p, (lumberjack->gameMode == LUMBERJACK_MODE_PANIC ? 0x13 : 0x15), lumberjackConCb,
                  lumberjackMsgRxCb, -70);
    p2pStartConnection(&lumberjack->p2p);
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
            lumberjack->gameMode = LUMBERJACK_MODE_PANIC;
        }
        else if (label == lumberjackAttack)
        {
            lumberjack->gameMode = LUMBERJACK_MODE_ATTACK;
        }

        if (label == lumberjackInstructions)
        {
            if (lumberjack->gameMode == LUMBERJACK_MODE_PANIC)
            {
                lumberjack->instructions = true;
            }
            else if (lumberjack->gameMode == LUMBERJACK_MODE_ATTACK)
            {
                lumberjack->instructions = true;
            }

            // There was a 3rd game mode planned but... I don't want to take any chances
        }

        if (label == lumberjackMenuMultiPlayer)
        {
            lumberjack->networked = true;
            // lumberjack->host is filled in after connection is established
            lumberjackJoinGame();
            // Start p2p after loading sprites, which takes time
        }
        else if (label == lumberjackMenuSinglePlayer)
        {
            lumberjack->networked = false;
            lumberjack->host      = false;
            lumberjackJoinGame();
        }
        else if (label == lumberjackExit)
        {
            lumberjackSaveSave();
            switchToSwadgeMode(&mainMenuMode);
        }
    }
    else
    {
        bool characterChange = false;
        if (label == lumberjackPanic)
        {
            lumberjack->gameMode = LUMBERJACK_MODE_PANIC;
        }
        else if (label == lumberjackAttack)
        {
            lumberjack->gameMode = LUMBERJACK_MODE_ATTACK;
        }

        if (label == lumberjackRedCharacter)
        {
            lumberjack->selected      = 0;
            lumberjack->playerColor.r = 255;
            lumberjack->playerColor.g = 0;
            lumberjack->playerColor.b = 0;

            characterChange = true;
        }
        else if (label == lumberjackGreenCharacter)
        {
            lumberjack->selected      = 1;
            lumberjack->playerColor.r = 0;
            lumberjack->playerColor.g = 255;
            lumberjack->playerColor.b = 0;
            characterChange           = true;
        }
        else if (label == lumberjackSpecialCharacter)
        {
            lumberjack->selected      = CHARACTER_GUY;
            lumberjack->playerColor.r = 0;
            lumberjack->playerColor.g = 255;
            lumberjack->playerColor.b = 255;
            characterChange           = true;
        }
        else if (label == lumberjackChoCharacter)
        {
            lumberjack->selected      = CHARACTER_CHO;
            lumberjack->playerColor.r = 0;
            lumberjack->playerColor.g = 255;
            lumberjack->playerColor.b = 255;
            characterChange           = true;
        }

        if (characterChange)
        {
            for (int ledIdx = 0; ledIdx < ARRAY_SIZE(lumberjack->menuLogbookRenderer->leds); ledIdx++)
            {
                lumberjack->menuLogbookRenderer->leds[ledIdx].r = lumberjack->playerColor.r;
                lumberjack->menuLogbookRenderer->leds[ledIdx].g = lumberjack->playerColor.g;
                lumberjack->menuLogbookRenderer->leds[ledIdx].b = lumberjack->playerColor.b;
            }
            lumberjack->save.character = lumberjack->selected;
        }
    }
}