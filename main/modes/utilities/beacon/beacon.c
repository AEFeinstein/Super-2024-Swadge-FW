#include "beacon.h"

void beaconEnterMode(void);
void beaconExitMode(void);
void beaconMainLoop(int64_t elapsedUs);
void beaconEspNowRecvCb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len, int8_t rssi);
void beaconEspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status);
void beaconMenuCb(const char* label, bool selected, uint32_t value);

const char beaconName[] = "Beacon";
const char rxLabel[]    = "Receive";
const char txLabel[]    = "Transmit";

swadgeMode_t beaconMode = {
    .modeName                 = beaconName,
    .wifiMode                 = ESP_NOW,
    .overrideUsb              = false,
    .usesAccelerometer        = false,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = beaconEnterMode,
    .fnExitMode               = beaconExitMode,
    .fnMainLoop               = beaconMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = NULL,
    .fnEspNowRecvCb           = beaconEspNowRecvCb,
    .fnEspNowSendCb           = beaconEspNowSendCb,
    .fnAdvancedUSB            = NULL,
    .fnDacCb                  = NULL,
};

typedef enum
{
    BEACON_IDLE,
    BEACON_TRANSMIT,
    BEACON_RECEIVE,
} beaconState_t;

typedef struct
{
    menu_t* menu;
    menuManiaRenderer_t* renderer;
    beaconState_t state;
    int32_t txTimer;
    list_t receivedData;
    font_t ibm;
} beacon_t;

static beacon_t* b;

/**
 * @brief This function is called when this mode is started. It should initialize variables and start the mode.
 */
void beaconEnterMode(void)
{
    b           = heap_caps_calloc(1, sizeof(beacon_t), MALLOC_CAP_8BIT);
    b->menu     = initMenu(beaconMode.modeName, beaconMenuCb);
    b->renderer = initMenuManiaRenderer(NULL, NULL, NULL);

    // Load fonts
    loadFont("ibm_vga8.font", &b->ibm, true);

    addSingleItemToMenu(b->menu, txLabel);
    addSingleItemToMenu(b->menu, rxLabel);
    return;
}

/**
 * @brief This function is called when the mode is exited. It should free any allocated memory.
 */
void beaconExitMode(void)
{
    deinitMenuManiaRenderer(b->renderer);
    deinitMenu(b->menu);

    freeFont(&b->ibm);

    void* val;
    while (NULL != (val = pop(&b->receivedData)))
    {
        heap_caps_free(val);
    }
    heap_caps_free(b);
    return;
}

/**
 * @brief This function is called from the main loop. It's pretty quick, but the timing may be inconsistent.
 *
 * @param elapsedUs The time elapsed since the last time this function was called. Use this value to determine when
 * it's time to do things
 */
void beaconMainLoop(int64_t elapsedUs)
{
    // Process button events
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        switch (b->state)
        {
            case BEACON_IDLE:
            {
                b->menu = menuButton(b->menu, evt);
                break;
            }
            case BEACON_TRANSMIT:
            case BEACON_RECEIVE:
            {
                if (evt.down)
                {
                    b->state = BEACON_IDLE;
                }
                break;
            }
        }
    }

    switch (b->state)
    {
        case BEACON_IDLE:
        {
            drawMenuMania(b->menu, b->renderer, elapsedUs);
            break;
        }
        case BEACON_TRANSMIT:
        {
            fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c100);
            drawText(&b->ibm, c555, "Transmit", 40, 40);
            if (BEACON_TRANSMIT == b->state)
            {
                RUN_TIMER_EVERY(b->txTimer, 1000000, elapsedUs, { espNowSend(beaconName, sizeof(beaconName)); });
            }
            break;
        }
        case BEACON_RECEIVE:
        {
            fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c001);
            void* rxStr;
            node_t* node = b->receivedData.first;
            int32_t yIdx = 0;
            while (node && yIdx < TFT_HEIGHT)
            {
                drawText(&b->ibm, c555, node->val, 40, yIdx);
                yIdx += 4 + b->ibm.height;
                node = node->next;
            }
            break;
        }
    }
}

/**
 * @brief This function is called whenever an ESP-NOW packet is received.
 *
 * @param esp_now_info Information about the transmission, including The MAC addresses
 * @param data     A pointer to the data received
 * @param len      The length of the data received
 * @param rssi     The RSSI for this packet, from 1 (weak) to ~90 (touching)
 */
void beaconEspNowRecvCb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len, int8_t rssi)
{
    uint8_t* savedCopy = heap_caps_calloc(1, len, MALLOC_CAP_8BIT);
    memcpy(savedCopy, data, len);
    push(&b->receivedData, savedCopy);
    return;
}

/**
 * @brief This function is called whenever an ESP-NOW packet is sent. It is just a status callback whether or not
 * the packet was actually sent. This will be called after calling espNowSend().
 *
 * @param mac_addr The MAC address which the data was sent to
 * @param status   The status of the transmission
 */
void beaconEspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status)
{
    return;
}

void beaconMenuCb(const char* label, bool selected, uint32_t value)
{
    if (selected)
    {
        if (txLabel == label)
        {
            b->state = BEACON_TRANSMIT;
        }
        else if (rxLabel == label)
        {
            b->state = BEACON_RECEIVE;
        }
    }
}
