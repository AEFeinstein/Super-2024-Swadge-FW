#include "ringsAndGems.h"

// It's good practice to declare immutable strings as const so they get placed in ROM, not RAM
static const char ragName[] = "Rings and Gems";

static void ragEnterMode(void);
static void ragExitMode(void);
static void ragMainLoop(int64_t elapsedUs);

static void ragEspNowRecvCb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len, int8_t rssi);
static void ragEspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status);
static void ragConCb(p2pInfo* p2p, connectionEvt_t evt);
static void ragMsgRxCb(p2pInfo* p2p, const uint8_t* payload, uint8_t len);
static void ragMsgTxCbFn(p2pInfo* p2p, messageStatus_t status, const uint8_t* data, uint8_t len);

static void ragDrawGrid(int16_t x0, int16_t y0, int16_t x1, int16_t y1, paletteColor_t color);

typedef enum __attribute__((packed))
{
    RAG_EMPTY,
    RAG_RING,
    RAG_GEM,
} ragCell_t;

typedef struct
{
    p2pInfo p2p;
    ragCell_t grid[3][3][3][3];
} ringsAndGems_t;

swadgeMode_t ragMode = {
    .modeName                 = ragName,
    .wifiMode                 = ESP_NOW,
    .overrideUsb              = false,
    .usesAccelerometer        = false,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = ragEnterMode,
    .fnExitMode               = ragExitMode,
    .fnMainLoop               = ragMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = NULL,
    .fnEspNowRecvCb           = ragEspNowRecvCb,
    .fnEspNowSendCb           = ragEspNowSendCb,
    .fnAdvancedUSB            = NULL,
    .fnDacCb                  = NULL,
};

ringsAndGems_t* rag;

/**
 * @brief TODO
 *
 */
static void ragEnterMode(void)
{
    // Allocate memory for the mode
    rag = calloc(1, sizeof(ringsAndGems_t));

    // Initialize and start p2p
    p2pInitialize(&rag->p2p, 0x25, ragConCb, ragMsgRxCb, -70);
    p2pStartConnection(&rag->p2p);
}

/**
 * @brief TODO
 *
 */
static void ragExitMode(void)
{
    // Deinitialize p2p
    p2pDeinit(&rag->p2p);

    // Free memory
    free(rag);
}

/**
 * @brief TODO
 *
 * @param elapsedUs
 */
static void ragMainLoop(int64_t elapsedUs)
{
    // Check for buttons
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        // Do something?
    }

    // Clear before drawing
    clearPxTft();

    // Fidn the size of a subgrid
    int16_t subgameSize = TFT_HEIGHT / 3;

    // Draw main grid
    ragDrawGrid((TFT_WIDTH - TFT_HEIGHT) / 2, 0, TFT_WIDTH - (TFT_WIDTH - TFT_HEIGHT) / 2, TFT_HEIGHT, c555);

    // Draw subgrids
    for (int16_t y = 0; y < 3; y++)
    {
        for (int16_t x = 0; x < 3; x++)
        {
            ragDrawGrid((TFT_WIDTH - TFT_HEIGHT) / 2 + (x * subgameSize) + 3, y * subgameSize + 3, //
                        (TFT_WIDTH - TFT_HEIGHT) / 2 + ((x + 1) * subgameSize) - 3, (y + 1) * subgameSize - 3, c500);
        }
    }
}

/**
 * @brief TODO
 *
 * @param x0
 * @param y0
 * @param x1
 * @param y1
 * @param color
 */
static void ragDrawGrid(int16_t x0, int16_t y0, int16_t x1, int16_t y1, paletteColor_t color)
{
    int16_t width  = x1 - x0;
    int16_t height = y1 - y0;

    int16_t cellWidth  = (width - 2) / 3;
    int16_t cellHeight = (height - 2) / 3;

    // Horizontal lines
    drawLineFast(x0, y0 + cellHeight, x1, y0 + cellHeight, color);
    drawLineFast(x0, y0 + (2 * cellHeight) + 1, x1, y0 + (2 * cellHeight) + 1, color);

    // Vertical lines
    drawLineFast(x0 + cellWidth, y0, x0 + cellWidth, y1, color);
    drawLineFast(x0 + (2 * cellWidth) + 1, y0, x0 + (2 * cellWidth) + 1, y1, color);
}

/**
 * @brief TODO
 *
 * @param esp_now_info
 * @param data
 * @param len
 * @param rssi
 */
static void ragEspNowRecvCb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len, int8_t rssi)
{
    // Pass to p2p
    p2pRecvCb(&rag->p2p, esp_now_info->src_addr, (const uint8_t*)data, len, rssi);
}

/**
 * @brief TODO
 *
 * @param mac_addr
 * @param status
 */
static void ragEspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status)
{
    // Pass to p2p
    p2pSendCb(&rag->p2p, mac_addr, status);
}

/**
 * @brief TODO
 *
 * @param p2p
 * @param evt
 */
static void ragConCb(p2pInfo* p2p, connectionEvt_t evt)
{
    // TODO
}

/**
 * @brief TODO
 *
 * @param p2p
 * @param payload
 * @param len
 */
static void ragMsgRxCb(p2pInfo* p2p, const uint8_t* payload, uint8_t len)
{
    // TODO
}

/**
 * @brief TODO
 *
 * @param p2p
 * @param status
 * @param data
 * @param len
 */
static void ragMsgTxCbFn(p2pInfo* p2p, messageStatus_t status, const uint8_t* data, uint8_t len)
{
    // TODO
}
