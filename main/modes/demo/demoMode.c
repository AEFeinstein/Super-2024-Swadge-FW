#include "demoMode.h"

static void demoEnterMode(void);
static void demoExitMode(void);
static void demoMainLoop(int64_t elapsedUs);
static void demoAudioCallback(uint16_t* samples, uint32_t sampleCnt);
static void demoBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);
static void demoEspNowRecvCb(const uint8_t* mac_addr, const uint8_t* data, uint8_t len, int8_t rssi);
static void demoEspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status);
static int16_t demoAdvancedUSB(uint8_t* buffer, uint16_t length, uint8_t isGet);

static void demoConCb(p2pInfo* p2p, connectionEvt_t evt);
static void demoMsgRxCb(p2pInfo* p2p, const uint8_t* payload, uint8_t len);
static void demoMsgTxCbFn(p2pInfo* p2p, messageStatus_t status, const uint8_t* data, uint8_t len);

swadgeMode_t demoMode = {
    .modeName                 = "Demo",
    .wifiMode                 = ESP_NOW,
    .overrideUsb              = false,
    .fnEnterMode              = demoEnterMode,
    .fnExitMode               = demoExitMode,
    .fnMainLoop               = demoMainLoop,
    .fnAudioCallback          = demoAudioCallback,
    .fnBackgroundDrawCallback = demoBackgroundDrawCallback,
    .fnEspNowRecvCb           = demoEspNowRecvCb,
    .fnEspNowSendCb           = demoEspNowSendCb,
    .fnAdvancedUSB            = NULL,
};

typedef struct
{
    font_t ibm;
    wsg_t king_donut;
    song_t ode_to_joy;
    p2pInfo p2p;
} demoVars_t;

demoVars_t* dv;

/**
 * This function is called when this mode is started. It should initialize
 * variables and start the mode.
 */
static void demoEnterMode(void)
{
    dv = calloc(1, sizeof(demoVars_t));
    loadFont("ibm_vga8.font", &dv->ibm);
    loadWsgSpiRam("kid0.wsg", &dv->king_donut, true);
    loadSngSpiRam("ode.sng", &dv->ode_to_joy, true);

    bzrPlayBgm(&dv->ode_to_joy);

    p2pInitialize(&dv->p2p, 'd', demoConCb, demoMsgRxCb, -70);
    p2pStartConnection(&dv->p2p);

    // const uint8_t testMsg[] = {0x01, 0x02, 0x03, 0x04};
    // p2pSendMsg(&dv->p2p, testMsg, ARRAY_SIZE(testMsg), demoMsgTxCbFn);

    const char demoKey[]     = "demo_high_score";
    int32_t highScoreToWrite = 99999;
    if (true == writeNvs32(demoKey, highScoreToWrite))
    {
        int32_t highScoreToRead;
        if (true == readNvs32(demoKey, &highScoreToRead))
        {
            printf("High score in NVS is %ld\n", highScoreToRead);
        }
    }
}

/**
 * This function is called when the mode is exited. It should free any allocated memory.
 */
static void demoExitMode(void)
{
    freeWsg(&dv->king_donut);
    freeFont(&dv->ibm);
    freeSng(&dv->ode_to_joy);
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
    // Process button events
    buttonEvt_t evt              = {0};
    static uint32_t lastBtnState = 0;
    while (checkButtonQueue(&evt))
    {
        printf("state: %04X, button: %d, down: %s\n", evt.state, evt.button, evt.down ? "down" : "up");
        lastBtnState = evt.state;
        // drawScreen = evt.down;

        static hid_gamepad_report_t report;
        report.buttons = lastBtnState;
        sendUsbGamepadReport(&report);
    }

    // Check for analog touch
    int32_t centerVal, intensityVal;
    if (getTouchCentroid(&centerVal, &intensityVal))
    {
        printf("touch center: %lu, intensity: %lu\n", centerVal, intensityVal);
    }
    else
    {
        printf("no touch\n");
    }

    // Draw to the display
    clearPxTft();
    int numBtns   = 13;
    int drawWidth = TFT_WIDTH / numBtns;
    for (int i = 0; i < numBtns; i++)
    {
        if (lastBtnState & (1 << i))
        {
            for (int w = drawWidth * i; w < drawWidth * (i + 1); w++)
            {
                for (int h = 0; h < TFT_HEIGHT; h++)
                {
                    setPxTft(w, h, (i + 5) * 5);
                }
            }
        }
    }

    drawLine(92, 92, 200, 200, c500, 0, 0, 0, 1, 1);
    speedyLine(102, 92, 210, 200, c050);
    drawWsg(&dv->king_donut, 100, 10, false, false, 0);

    int yIdx         = 64;
    char dbgTxt[256] = {0};
    sprintf(dbgTxt, "nNotes = %" PRIu32, dv->ode_to_joy.numNotes);
    drawText(&dv->ibm, c555, dbgTxt, 0, yIdx);
    yIdx += (dv->ibm.h + 2);

    int noteIdx = 0;
    while (yIdx < TFT_HEIGHT)
    {
        sprintf(dbgTxt, "f: %5d, t: %5" PRIu32, dv->ode_to_joy.notes[noteIdx].note,
                dv->ode_to_joy.notes[noteIdx].timeMs);
        drawText(&dv->ibm, c555, dbgTxt, 0, yIdx);
        yIdx += (dv->ibm.h + 2);
        noteIdx++;
    }

    // Read the temperature
    printf("%f\n", readTemperatureSensor());

    // Get the acceleration
    int16_t a_x, a_y, a_z;
    qma7981_get_accel(&a_x, &a_y, &a_z);

    // Set LEDs
    led_t leds[CONFIG_NUM_LEDS] = {0};
    for (uint8_t i = 0; i < CONFIG_NUM_LEDS; i++)
    {
        leds[i].r = (255 * ((i + 0) % CONFIG_NUM_LEDS)) / (CONFIG_NUM_LEDS - 1);
        leds[i].g = (255 * ((i + 3) % CONFIG_NUM_LEDS)) / (CONFIG_NUM_LEDS - 1);
        leds[i].b = (255 * ((i + 6) % CONFIG_NUM_LEDS)) / (CONFIG_NUM_LEDS - 1);
    }
    setLeds(leds, CONFIG_NUM_LEDS);
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
 * @param mac_addr The MAC address which sent this data
 * @param data     A pointer to the data received
 * @param len      The length of the data received
 * @param rssi     The RSSI for this packet, from 1 (weak) to ~90 (touching)
 */
static void demoEspNowRecvCb(const uint8_t* mac_addr, const uint8_t* data, uint8_t len, int8_t rssi)
{
    p2pRecvCb(&dv->p2p, mac_addr, data, len, rssi);
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
 * if "direction" == 0, that is a "set" or an "OUT" endpoint, where the PC sends the swage data.
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
    // Do something
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
}