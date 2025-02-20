#include "beacon.h"

void beaconEnterMode(void);
void beaconExitMode(void);
void beaconMainLoop(int64_t elapsedUs);
void beaconEspNowRecvCb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len, int8_t rssi);
void beaconEspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status);
void beaconMenuCb(const char* label, bool selected, uint32_t value);
static esp_now_recv_info_t* copyEspNowRecvInfo(const esp_now_recv_info_t* info);

const char beaconName[]  = "Beacon";
const char rxLabel[]     = "Receive";
const char txLabel[]     = "Transmit";
const char* broadcasts[] = {
    "BCST0", "BCST1", "BCST2", "BCST3", "BCST4",
};

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
    uint8_t* data;
    int16_t len;
    int8_t rssi;
    esp_now_recv_info_t* rxInfo;
} beaconRxData_t;

typedef struct
{
    menu_t* menu;
    menuManiaRenderer_t* renderer;
    beaconState_t state;
    int32_t txTimer;
    list_t receivedData;
    font_t ibm;
    const char* txData;
    int16_t txLen;
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

    b->menu = startSubMenu(b->menu, txLabel);
    for (int32_t i = 0; i < ARRAY_SIZE(broadcasts); i++)
    {
        addSingleItemToMenu(b->menu, broadcasts[i]);
    }
    b->menu = endSubMenu(b->menu);

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

    beaconRxData_t* val;
    while (NULL != (val = pop(&b->receivedData)))
    {
        heap_caps_free(val->rxInfo->des_addr);
        heap_caps_free(val->rxInfo->src_addr);
        heap_caps_free(val->rxInfo->rx_ctrl);
        heap_caps_free(val->rxInfo);
        heap_caps_free(val->data);
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
                RUN_TIMER_EVERY(b->txTimer, 1000000, elapsedUs, { espNowSend(b->txData, b->txLen); });
            }
            break;
        }
        case BEACON_RECEIVE:
        {
            fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c001);

            node_t* node = b->receivedData.last;
            int32_t yIdx = 4;
            while (node && yIdx < TFT_HEIGHT)
            {
                beaconRxData_t* rxData = node->val;

                char str[128] = {0};
                int32_t sIdx  = 0;
                sIdx = snprintf(str, sizeof(str) - 1, "[%3d,%3d] ", rxData->rxInfo->rx_ctrl->rssi, rxData->rssi);

                for (int16_t dIdx = 0; dIdx < rxData->len; dIdx++)
                {
                    sIdx += snprintf(&str[sIdx], sizeof(str) - sIdx - 1, "%02X ", rxData->data[dIdx]);
                }
                drawText(&b->ibm, c555, str, 40, yIdx);
                yIdx += 4 + b->ibm.height;
                node = node->prev;
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
    // Allocate object for list
    beaconRxData_t* rxData = heap_caps_calloc(1, sizeof(beaconRxData_t), MALLOC_CAP_8BIT);

    // Save payload in list object
    rxData->len  = len;
    rxData->data = heap_caps_calloc(1, len, MALLOC_CAP_8BIT);
    memcpy(rxData->data, data, len);

    // Save metadata
    rxData->rxInfo = copyEspNowRecvInfo(esp_now_info);

    // Save RSSI
    rxData->rssi = rssi;

    // Push into list
    push(&b->receivedData, rxData);
}

/**
 * @brief TODO
 *
 * @param info
 * @return esp_now_recv_info_t*
 */
static esp_now_recv_info_t* copyEspNowRecvInfo(const esp_now_recv_info_t* info)
{
#define MAC_LEN 6
    esp_now_recv_info_t* copy = heap_caps_calloc(1, sizeof(esp_now_recv_info_t), MALLOC_CAP_8BIT);

    copy->des_addr = heap_caps_calloc(MAC_LEN, sizeof(uint8_t), MALLOC_CAP_8BIT);
    memcpy(copy->des_addr, info->des_addr, MAC_LEN * sizeof(uint8_t));

    copy->src_addr = heap_caps_calloc(MAC_LEN, sizeof(uint8_t), MALLOC_CAP_8BIT);
    memcpy(copy->src_addr, info->src_addr, MAC_LEN * sizeof(uint8_t));

    copy->rx_ctrl = heap_caps_calloc(1, sizeof(wifi_pkt_rx_ctrl_t), MALLOC_CAP_8BIT);
    memcpy(copy->rx_ctrl, info->rx_ctrl, sizeof(wifi_pkt_rx_ctrl_t));

    return copy;
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
        for (int32_t i = 0; i < ARRAY_SIZE(broadcasts); i++)
        {
            if (broadcasts[i] == label)
            {
                b->state  = BEACON_TRANSMIT;
                b->txData = label;
                b->txLen  = strlen(label);
                return;
            }
        }

        if (rxLabel == label)
        {
            b->state = BEACON_RECEIVE;
        }
    }
}
