#include "menu.h"
#include "demoMode.h"

static void demoEnterMode(void);
static void demoExitMode(void);
static void demoMainLoop(int64_t elapsedUs);
static void demoAudioCallback(uint16_t* samples, uint32_t sampleCnt);
static void demoBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);
static void demoEspNowRecvCb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len, int8_t rssi);
static void demoEspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status);
static int16_t demoAdvancedUSB(uint8_t* buffer, uint16_t length, uint8_t isGet);

static void demoConCb(p2pInfo* p2p, connectionEvt_t evt);
static void demoMsgRxCb(p2pInfo* p2p, const uint8_t* payload, uint8_t len);
static void demoMsgTxCbFn(p2pInfo* p2p, messageStatus_t status, const uint8_t* data, uint8_t len);

static void demoMenuCb(const char*, bool selected, uint32_t settingVal);

static const char demoName[]  = "Demo";
static const char demoMenu1[] = "Menu 1";
static const char demoMenu2[] = "Menu 2";
static const char demoMenu3[] = "Menu 3";
static const char demoMenu4[] = "Menu 4";
static const char demoMenu5[] = "Menu 5";
static const char demoMenu6[] = "Menu 6";
static const char demoMenu7[] = "Menu 7";
static const char demoMenu8[] = "Menu 8";

static const char opt1[]            = "opt1";
static const char opt2[]            = "opt2";
static const char opt3[]            = "opt3";
static const char opt4[]            = "opt4";
static const char* const demoOpts[] = {opt1, opt2, opt3, opt4};

static const char demoSubMenu1[] = "SubMenu1";
static const char demoSubMenu2[] = "SubMenu2";

swadgeMode_t demoMode = {
    .modeName                 = demoName,
    .wifiMode                 = ESP_NOW,
    .overrideUsb              = false,
    .usesAccelerometer        = true,
    .usesThermometer          = true,
    .fnEnterMode              = demoEnterMode,
    .fnExitMode               = demoExitMode,
    .fnMainLoop               = demoMainLoop,
    .fnAudioCallback          = demoAudioCallback,
    .fnBackgroundDrawCallback = demoBackgroundDrawCallback,
    .fnEspNowRecvCb           = demoEspNowRecvCb,
    .fnEspNowSendCb           = demoEspNowSendCb,
    .fnAdvancedUSB            = demoAdvancedUSB,
};

typedef struct
{
    font_t ibm;
    wsg_t king_donut;
    song_t ode_to_joy;
    p2pInfo p2p;
    uint16_t sentPackets;
    uint16_t recvPackets;
    connectionEvt_t conStatus;
    menu_t* menu;
} demoVars_t;

demoVars_t* dv;

/**
 * This function is called when this mode is started. It should initialize
 * variables and start the mode.
 */
static void demoEnterMode(void)
{
    dv = calloc(1, sizeof(demoVars_t));
    loadFont("ibm_vga8.font", &dv->ibm, false);
    loadWsg("kid0.wsg", &dv->king_donut, true);
    loadSong("ode.sng", &dv->ode_to_joy, true);

    bzrPlayBgm(&dv->ode_to_joy, BZR_STEREO);
    bzrStop();

    dv->menu = initMenu(demoName, demoMenuCb);
    addSingleItemToMenu(dv->menu, demoMenu1);
    addSingleItemToMenu(dv->menu, demoMenu2);

    dv->menu = startSubMenu(dv->menu, demoSubMenu1);
    addSingleItemToMenu(dv->menu, demoMenu7);

    dv->menu = startSubMenu(dv->menu, demoSubMenu2);
    addSingleItemToMenu(dv->menu, demoMenu8);
    dv->menu = endSubMenu(dv->menu);

    dv->menu = endSubMenu(dv->menu);

    addSingleItemToMenu(dv->menu, demoMenu3);
    addSingleItemToMenu(dv->menu, demoMenu4);
    addMultiItemToMenu(dv->menu, demoOpts, ARRAY_SIZE(demoOpts), 0);
    addSingleItemToMenu(dv->menu, demoMenu5);
    addSingleItemToMenu(dv->menu, demoMenu6);

    dv->conStatus = CON_LOST;

    p2pInitialize(&dv->p2p, 'p', demoConCb, demoMsgRxCb, -70);
    p2pStartConnection(&dv->p2p);

    const char demoKey[]     = "demo_high_score";
    int32_t highScoreToWrite = 99999;
    if (true == writeNvs32(demoKey, highScoreToWrite))
    {
        int32_t highScoreToRead;
        if (true == readNvs32(demoKey, &highScoreToRead))
        {
            printf("High score in NVS is %" PRId32 "\n", highScoreToRead);
        }
    }
}

/**
 * This function is called when the mode is exited. It should free any allocated memory.
 */
static void demoExitMode(void)
{
    p2pDeinit(&dv->p2p);
    freeWsg(&dv->king_donut);
    freeFont(&dv->ibm);
    freeSong(&dv->ode_to_joy);
    deinitMenu(dv->menu);
    free(dv);
}

/**
 * This function is called from the main loop. It's pretty quick, but the
 * timing may be inconsistent.
 *
 * @param elapsedUs The time elapsed since the last time this function was called. Use this value to determine when
 * it's time to do things
 */
static void demoMainLoop(int64_t elapsedUs)
{
    clearPxTft();

    // Process button events
    buttonEvt_t evt              = {0};
    static uint32_t lastBtnState = 0;
    while (checkButtonQueueWrapper(&evt))
    {
        dv->menu = menuButton(dv->menu, evt);

        // printf("state: %04X, button: %d, down: %s\n", evt.state, evt.button, evt.down ? "down" : "up");
        lastBtnState = evt.state;
        // drawScreen = evt.down;

        if (evt.button == PB_B && evt.down && dv->conStatus == CON_ESTABLISHED)
        {
            printf("Sending packet\n");
            const uint8_t testMsg[] = {0x01, 0x02, 0x03, 0x04};
            p2pSendMsg(&dv->p2p, testMsg, ARRAY_SIZE(testMsg), demoMsgTxCbFn);
        }

        static hid_gamepad_report_t report;
        report.buttons = lastBtnState;
        sendUsbGamepadReport(&report);
    }

    // Fill the display area with a dark cyan
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c123);

    // Shade different areas with different intensities
    for (int i = 0; i < 5; i++)
    {
        shadeDisplayArea(0, (i * TFT_HEIGHT) / 5, TFT_WIDTH / 2, ((i + 1) * TFT_HEIGHT) / 5, i, c321);
    }

    // Draw a red circle
    drawCircle(200, 50, 20, c500);
    // Flood fill the circle with blue
    floodFill(200, 50, c005, 200 - 20, 50 - 20, 200 + 50, 50 + 20);

    // Draw a green rectangle
    drawRect(200, 150, 250, 220, c050);
    // Odd-even fill the rectangle with blue
    oddEvenFill(190, 140, 260, 230, c050, c005);

    char buffer[16];
    snprintf(buffer, sizeof(buffer) - 1, "%" PRIu16, dv->sentPackets);

    fillDisplayArea(15, 200, 45, 220, c544);
    drawText(&dv->ibm, c000, buffer, 20, 205);

    snprintf(buffer, sizeof(buffer) - 1, "%" PRIu16, dv->recvPackets);
    fillDisplayArea(45, 200, 75, 220, c454);
    drawText(&dv->ibm, c000, buffer, 50, 205);

    paletteColor_t statusColor;
    switch (dv->conStatus)
    {
        case CON_STARTED:
            statusColor = c300;
            break;

        case RX_GAME_START_ACK:
            statusColor = c330;
            break;

        case RX_GAME_START_MSG:
            statusColor = c530;
            break;

        case CON_ESTABLISHED:
            statusColor = c050;
            break;

        case CON_LOST:
        default:
            statusColor = c500;
            break;
    }

    fillDisplayArea(15, 180, 75, 200, statusColor);

    // Check for analog touch
    // int32_t centerVal, intensityVal;
    // if (getTouchCentroid(&centerVal, &intensityVal))
    // {
    //     printf("touch center: %" PRId32 ", intensity: %" PRId32 "\n", centerVal, intensityVal);
    // }
    // else
    // {
    //     printf("no touch\n");
    // }

    // // Draw to the display
    // clearPxTft();
    // int numBtns   = 13;
    // int drawWidth = TFT_WIDTH / numBtns;
    // for (int i = 0; i < numBtns; i++)
    // {
    //     if (lastBtnState & (1 << i))
    //     {
    //         for (int w = drawWidth * i; w < drawWidth * (i + 1); w++)
    //         {
    //             for (int h = 0; h < TFT_HEIGHT; h++)
    //             {
    //                 setPxTft(w, h, (i + 5) * 5);
    //             }
    //         }
    //     }
    // }

    // drawLine(92, 92, 200, 200, c555, 0, 0, 0, 1, 1);
    // speedyLine(102, 92, 210, 200, c050);
    static uint16_t rot = 0;
    drawWsg(&dv->king_donut, 100, 10, false, false, rot);
    rot++;
    if (360 == rot)
    {
        rot = 0;
    }

    // int yIdx         = 64;
    // char dbgTxt[256] = {0};
    // sprintf(dbgTxt, "nNotes = %" PRIu32, dv->ode_to_joy.numNotes);
    // drawText(&dv->ibm, c555, dbgTxt, 0, yIdx);
    // yIdx += (dv->ibm.h + 2);

    // int noteIdx = 0;
    // while (yIdx < TFT_HEIGHT)
    // {
    //     sprintf(dbgTxt, "f: %5d, t: %5" PRIu32, dv->ode_to_joy.notes[noteIdx].note,
    //             dv->ode_to_joy.notes[noteIdx].timeMs);
    //     drawText(&dv->ibm, c555, dbgTxt, 0, yIdx);
    //     yIdx += (dv->ibm.h + 2);
    //     noteIdx++;
    // }

    // // Read the temperature
    // printf("%f\n", readTemperatureSensor());

    // // Get the acceleration
    // int16_t a_x, a_y, a_z;
    // accelGetAccelVec(&a_x, &a_y, &a_z);

    // // Set LEDs
    // led_t leds[CONFIG_NUM_LEDS] = {0};
    // for (uint8_t i = 0; i < CONFIG_NUM_LEDS; i++)
    // {
    //     leds[i].r = (255 * ((i + 0) % CONFIG_NUM_LEDS)) / (CONFIG_NUM_LEDS - 1);
    //     leds[i].g = (255 * ((i + 3) % CONFIG_NUM_LEDS)) / (CONFIG_NUM_LEDS - 1);
    //     leds[i].b = (255 * ((i + 6) % CONFIG_NUM_LEDS)) / (CONFIG_NUM_LEDS - 1);
    // }
    // setLeds(leds, CONFIG_NUM_LEDS);
}

/**
 * This function is called whenever audio samples are read from the
 * microphone (ADC) and are ready for processing. Samples are read at 8KHz
 * This cannot be used at the same time as fnBatteryCallback
 *
 * @param samples A pointer to 12 bit audio samples
 * @param sampleCnt The number of samples read
 */
static void demoAudioCallback(uint16_t* samples, uint32_t sampleCnt)
{
    ;
}

/**
 * This function is called when the display driver wishes to update a
 * section of the display.
 *
 * @param disp The display to draw to
 * @param x the x coordiante that should be updated
 * @param y the x coordiante that should be updated
 * @param w the width of the rectangle to be updated
 * @param h the height of the rectangle to be updated
 * @param up update number
 * @param numUp update number denominator
 */
static void demoBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
    fillDisplayArea(x, y, x + w, y + h, c555);
}

/**
 * This function is called whenever an ESP-NOW packet is received.
 *
 * @param esp_now_info Information about the transmission, including The MAC addresses
 * @param data     A pointer to the data received
 * @param len      The length of the data received
 * @param rssi     The RSSI for this packet, from 1 (weak) to ~90 (touching)
 */
static void demoEspNowRecvCb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len, int8_t rssi)
{
    p2pRecvCb(&dv->p2p, esp_now_info->src_addr, data, len, rssi);
}

/**
 * This function is called whenever an ESP-NOW packet is sent.
 * It is just a status callback whether or not the packet was actually sent.
 * This will be called after calling espNowSend()
 *
 * @param mac_addr The MAC address which the data was sent to
 * @param status   The status of the transmission
 */
static void demoEspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status)
{
    p2pSendCb(&dv->p2p, mac_addr, status);
}

/**
 * Advanced USB Functionality, for hooking existing advanced_usb interface.
 * if "direction" == 1, that is a "get" or an "IN" endpoint, which means Swadge->PC
 * if "direction" == 0, that is a "set" or an "OUT" endpoint, where the PC sends the Swadge data.
 *
 * @param buffer
 * @param length
 * @param isGet
 * @return
 */
static int16_t demoAdvancedUSB(uint8_t* buffer, uint16_t length, uint8_t isGet)
{
    return 0;
}

/**
 * @brief This callback delivers connection statuses to the Swadge mode
 *
 * @param p2p The p2pInfo
 * @param evt The connection event
 */
static void demoConCb(p2pInfo* p2p, connectionEvt_t evt)
{
    dv->conStatus = evt;
}

/**
 * @brief This callback delivers received p2p packets to the Swadge mode
 *
 * @param p2p The p2pInfo
 * @param payload The data that was received
 * @param len The length of the data that was received
 */
static void demoMsgRxCb(p2pInfo* p2p, const uint8_t* payload, uint8_t len)
{
    // Do something
    dv->recvPackets++;
}

/**
 * @brief This callback delivers acknowledge status for transmitted messages to the Swadge mode
 *
 * @param p2p The p2pInfo
 * @param status The status of the transmission
 * @param data The data that was transmitted
 * @param len The length of the data that was transmitted
 */
static void demoMsgTxCbFn(p2pInfo* p2p, messageStatus_t status, const uint8_t* data, uint8_t len)
{
    // Do something
    dv->sentPackets++;
}

/**
 * @brief Callback for when menu items are selected
 *
 * @param label The menu item that was selected or moved to
 * @param selected true if the item was selected, false if it was moved to
 * @param settingVal The value of the setting, if the menu item is a settings item
 */
static void demoMenuCb(const char* label, bool selected, uint32_t settingVal)
{
    printf("%s %s\n", label, selected ? "selected" : "scrolled to");
}
