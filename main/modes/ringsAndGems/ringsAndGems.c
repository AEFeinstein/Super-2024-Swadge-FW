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

static void ragDrawGrid(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t m, paletteColor_t color);

typedef enum __attribute__((packed))
{
    NO_CURSOR,
    SELECT_SUBGAME,
    SELECT_CELL,
} ragCursorMode_t;

typedef enum __attribute__((packed))
{
    RAG_EMPTY,
    RAG_RING,
    RAG_GEM,
} ragCell_t;

typedef struct
{
    ragCell_t game[3][3];
    ragCell_t winner;
} ragSubgame_t;

typedef struct
{
    p2pInfo p2p;
    ragSubgame_t subgames[3][3];
    vec_t cursor;
    vec_t selectedSubgame;
    ragCursorMode_t cursorMode;
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

    rag->cursorMode = SELECT_SUBGAME;

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
        if (evt.down)
        {
            switch (evt.button)
            {
                case PB_UP:
                {
                    if (0 == rag->cursor.y)
                    {
                        rag->cursor.y = 2;
                    }
                    else
                    {
                        rag->cursor.y--;
                    }
                    break;
                }
                case PB_DOWN:
                {
                    rag->cursor.y = (rag->cursor.y + 1) % 3;
                    break;
                }
                case PB_LEFT:
                {
                    if (0 == rag->cursor.x)
                    {
                        rag->cursor.x = 2;
                    }
                    else
                    {
                        rag->cursor.x--;
                    }
                    break;
                }
                case PB_RIGHT:
                {
                    rag->cursor.x = (rag->cursor.x + 1) % 3;
                    break;
                }
                case PB_A:
                {
                    if (SELECT_SUBGAME == rag->cursorMode)
                    {
                        rag->selectedSubgame = rag->cursor;
                        rag->cursor.x        = 1;
                        rag->cursor.y        = 1;
                        rag->cursorMode      = SELECT_CELL;
                    }
                    else if (SELECT_CELL == rag->cursorMode)
                    {
                        // TODO place marker, pass the turn
                    }
                    break;
                }
                case PB_B:
                {
                    if (SELECT_CELL == rag->cursorMode)
                    {
                        rag->cursor     = rag->selectedSubgame;
                        rag->cursorMode = SELECT_SUBGAME;
                    }
                    break;
                }
                default:
                {
                    break;
                }
            }
        }
    }

    // Clear before drawing
    clearPxTft();

    // Calculate the game size based on the largest possible cell size
    int16_t gameSize    = MIN(TFT_WIDTH, TFT_HEIGHT);
    int16_t cellSize    = gameSize / 9;
    int16_t subgameSize = cellSize * 3;
    gameSize            = cellSize * 9;

    // Center the game on the screen
    int16_t gameOffsetX = (TFT_WIDTH - gameSize) / 2;
    int16_t gameOffsetY = (TFT_HEIGHT - gameSize) / 2;

    // Draw the main gridlines
    ragDrawGrid(gameOffsetX, gameOffsetY, gameOffsetX + gameSize - 1, gameOffsetY + gameSize - 1, 0, c300);

    // For each subgame
    for (int subY = 0; subY < 3; subY++)
    {
        for (int subX = 0; subX < 3; subX++)
        {
            int16_t sX0 = gameOffsetX + (subX * subgameSize);
            int16_t sY0 = gameOffsetY + (subY * subgameSize);
            int16_t sX1 = sX0 + subgameSize - 1;
            int16_t sY1 = sY0 + subgameSize - 1;
            // Draw the subgame grid lines
            ragDrawGrid(sX0, sY0, sX1, sY1, 4, c330);

            // If selected, draw the cursor on this subgame
            if (SELECT_SUBGAME == rag->cursorMode && //
                rag->cursor.x == subX && rag->cursor.y == subY)
            {
                drawRect(sX0, sY0, sX1, sY1, c005);
            }

            // For each cell
            for (int cellY = 0; cellY < 3; cellY++)
            {
                for (int cellX = 0; cellX < 3; cellX++)
                {
                    int16_t cX0 = sX0 + (cellX * cellSize);
                    int16_t cY0 = sY0 + (cellY * cellSize);
                    int16_t cX1 = cX0 + cellSize - 1;
                    int16_t cY1 = cY0 + cellSize - 1;
                    // Draw sprites
                    // fillDisplayArea(cX0, cY0, cX1, cY1, c002);

                    // If selected, draw the cursor on this cell
                    if (SELECT_CELL == rag->cursorMode &&                                   //
                        rag->selectedSubgame.x == subX && rag->selectedSubgame.y == subY && //
                        rag->cursor.x == cellX && rag->cursor.y == cellY)
                    {
                        drawRect(cX0, cY0, cX1, cY1, c005);
                    }
                }
            }
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
 * @param m
 * @param color
 */
static void ragDrawGrid(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t m, paletteColor_t color)
{
    int16_t width  = x1 - x0;
    int16_t height = y1 - y0;

    int16_t cellWidth  = width / 3;
    int16_t cellHeight = height / 3;

    // Horizontal lines
    drawLineFast(x0 + m, y0 + cellHeight, //
                 x1 - 1 - m, y0 + cellHeight, color);
    drawLineFast(x0 + m, y0 + (2 * cellHeight) + 1, //
                 x1 - 1 - m, y0 + (2 * cellHeight) + 1, color);

    // Vertical lines
    drawLineFast(x0 + cellWidth, y0 + m, //
                 x0 + cellWidth, y1 - 1 - m, color);
    drawLineFast(x0 + (2 * cellWidth) + 1, y0 + m, //
                 x0 + (2 * cellWidth) + 1, y1 - 1 - m, color);
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
