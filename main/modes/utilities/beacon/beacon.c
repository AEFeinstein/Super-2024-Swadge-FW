//==============================================================================
// Includes
//==============================================================================

#include "beacon.h"

//==============================================================================
// Defines
//==============================================================================

#define MAC_LEN 6

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    BEACON_IDLE,
    BEACON_TRANSMIT,
    BEACON_RECEIVE,
} beaconState_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    uint8_t* data;
    int16_t len;
    int8_t rssi;
    uint8_t src_addr[MAC_LEN];
    uint8_t des_addr[MAC_LEN];
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

//==============================================================================
// Function Declarations
//==============================================================================

void beaconEnterMode(void);
void beaconExitMode(void);
void beaconMainLoop(int64_t elapsedUs);
void beaconEspNowRecvCb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len, int8_t rssi);
void beaconEspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status);
void beaconMenuCb(const char* label, bool selected, uint32_t value);

//==============================================================================
// Constant Variables
//==============================================================================

const char beaconName[]  = "Beacon";
const char rxLabel[]     = "Receive";
const char txLabel[]     = "Transmit";
const char* broadcasts[] = {
    "BCST0", "BCST1", "BCST2", "BCST3", "BCST4",
};

//==============================================================================
// Variables
//==============================================================================

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

static beacon_t* b;

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief This function is called when this mode is started. It should initialize variables and start the mode.
 */
void beaconEnterMode(void)
{
    // Allocate memory
    b = heap_caps_calloc(1, sizeof(beacon_t), MALLOC_CAP_8BIT);

    // Load fonts
    loadFont("ibm_vga8.font", &b->ibm, true);

    // Setup menu
    b->renderer = initMenuManiaRenderer(NULL, NULL, NULL);
    b->menu     = initMenu(beaconMode.modeName, beaconMenuCb);

    b->menu = startSubMenu(b->menu, txLabel);
    for (int32_t i = 0; i < ARRAY_SIZE(broadcasts); i++)
    {
        addSingleItemToMenu(b->menu, broadcasts[i]);
    }
    b->menu = endSubMenu(b->menu);

    addSingleItemToMenu(b->menu, rxLabel);
}

/**
 * @brief This function is called when the mode is exited. It should free any allocated memory.
 */
void beaconExitMode(void)
{
    // Free font
    freeFont(&b->ibm);

    // Deinit menu
    deinitMenuManiaRenderer(b->renderer);
    deinitMenu(b->menu);

    // Clear list
    beaconRxData_t* val;
    while (NULL != (val = pop(&b->receivedData)))
    {
        heap_caps_free(val->data);
        heap_caps_free(val);
    }

    // Free everything
    heap_caps_free(b);
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
                // Pass buttons to the menu
                b->menu = menuButton(b->menu, evt);
                break;
            }
            case BEACON_TRANSMIT:
            case BEACON_RECEIVE:
            {
                if (evt.down)
                {
                    // exit on button press
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
            // Slightly red background
            fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c100);

            // Transmit every second
            paletteColor_t textColor = c555;
            RUN_TIMER_EVERY(b->txTimer, 1000000, elapsedUs, {
                espNowSend(b->txData, b->txLen);
                textColor = c050;
            });

            // Draw text
            drawText(&b->ibm, textColor, "Transmit", 40, 40);
            break;
        }
        case BEACON_RECEIVE:
        {
            // Slightly blue background
            fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c001);

            // Draw received data to the screen, starting with last node
            node_t* node = b->receivedData.last;
            int32_t yIdx = 4;
            while (node && yIdx < TFT_HEIGHT)
            {
                // Get data from list
                beaconRxData_t* rxData = node->val;

                // Print header to string
                char str[128] = {0};
                int32_t sIdx  = 0;
                sIdx          = snprintf(str, sizeof(str) - 1, "[%3d] ", rxData->rssi);

                // Print data to string
                for (int16_t dIdx = 0; dIdx < rxData->len; dIdx++)
                {
                    sIdx += snprintf(&str[sIdx], sizeof(str) - sIdx - 1, "%02X ", rxData->data[dIdx]);
                }

                // Draw string
                drawText(&b->ibm, c555, str, 40, yIdx);
                yIdx += 4 + b->ibm.height;

                // Work backwards
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
    memcpy(rxData->src_addr, esp_now_info->src_addr, MAC_LEN);
    memcpy(rxData->des_addr, esp_now_info->des_addr, MAC_LEN);

    // Save RSSI
    rxData->rssi = rssi;

    // Push into list
    push(&b->receivedData, rxData);
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
    // Do nothing, don't care about TX status
    return;
}

/**
 * @brief Menu callback function
 *
 * @param label The menu item that was triggered
 * @param selected True if the item was selected, false if it was moved to
 * @param value Unused
 */
void beaconMenuCb(const char* label, bool selected, uint32_t value)
{
    // If the menu item was selected
    if (selected)
    {
        // If this was one of the broadcasts
        for (int32_t i = 0; i < ARRAY_SIZE(broadcasts); i++)
        {
            if (broadcasts[i] == label)
            {
                // Set transmit mode
                b->state  = BEACON_TRANSMIT;
                b->txData = label;
                b->txLen  = strlen(label);
                return;
            }
        }

        if (rxLabel == label)
        {
            // Set receive mode
            b->state = BEACON_RECEIVE;
        }
    }
}
