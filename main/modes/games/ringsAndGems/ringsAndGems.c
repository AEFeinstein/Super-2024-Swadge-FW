//==============================================================================
// Includes
//==============================================================================

#include "ringsAndGems.h"

//==============================================================================
// Enums
//==============================================================================

typedef enum __attribute__((packed))
{
    RGS_MENU,
    RGS_PLACING_PIECE,
    RGS_WAITING,
} ragGameState_t;

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
} ragPiece_t;

typedef enum __attribute__((packed))
{
    MSG_SELECT_PIECE,
    MSG_MOVE_CURSOR,
    MSG_PLACE_PIECE,
} ragMsgType_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    ragPiece_t game[3][3];
    ragPiece_t winner;
} ragSubgame_t;

typedef struct
{
    ragGameState_t state;
    ragSubgame_t subgames[3][3];
    vec_t cursor;
    vec_t selectedSubgame;
    ragCursorMode_t cursorMode;
    wsg_t piece_x_big;
    wsg_t piece_x_small;
    wsg_t piece_o_big;
    wsg_t piece_o_small;
    p2pInfo p2p;
    ragPiece_t p1Piece;
    ragPiece_t p2Piece;
} ringsAndGems_t;

typedef struct
{
    ragMsgType_t type;
    ragPiece_t piece;
} ragMsgSelectPiece_t;

typedef struct
{
    ragMsgType_t type;
    ragCursorMode_t cursorMode;
    vec_t selectedSubgame;
    vec_t cursor;
} ragMsgMoveCursor_t;

typedef struct
{
    ragMsgType_t type;
    vec_t selectedSubgame;
    vec_t selectedCell;
} ragMsgPlacePiece_t;

//==============================================================================
// Function Prototypes
//==============================================================================

static void ragEnterMode(void);
static void ragExitMode(void);
static void ragMainLoop(int64_t elapsedUs);

static void ragEspNowRecvCb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len, int8_t rssi);
static void ragEspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status);
static void ragConCb(p2pInfo* p2p, connectionEvt_t evt);
static void ragMsgRxCb(p2pInfo* p2p, const uint8_t* payload, uint8_t len);
static void ragMsgTxCbFn(p2pInfo* p2p, messageStatus_t status, const uint8_t* data, uint8_t len);

static void ragHandleInput(void);
static void ragDrawGame(void);

static void ragDrawGrid(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t m, paletteColor_t color);

//==============================================================================
// Variables
//==============================================================================

// It's good practice to declare immutable strings as const so they get placed in ROM, not RAM
static const char ragName[] = "Rings and Gems";

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

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief TODO
 *
 */
static void ragEnterMode(void)
{
    // Allocate memory for the mode
    rag = calloc(1, sizeof(ringsAndGems_t));

    rag->cursorMode = SELECT_SUBGAME;

    loadWsg("x_small.wsg", &rag->piece_x_small, true);
    loadWsg("x_large.wsg", &rag->piece_x_big, true);
    loadWsg("o_small.wsg", &rag->piece_o_small, true);
    loadWsg("o_large.wsg", &rag->piece_o_big, true);

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
    // Free memory
    freeWsg(&rag->piece_x_small);
    freeWsg(&rag->piece_x_big);
    freeWsg(&rag->piece_o_small);
    freeWsg(&rag->piece_o_big);

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
    // Handle inputs
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        switch (rag->state)
        {
            case RGS_MENU:
            {
                // TODO menu button inputs
                break;
            }
            case RGS_PLACING_PIECE:
            {
                // Move the cursor
                ragHandleInput();
                break;
            }
            default:
            case RGS_WAITING:
            {
                // Do nothing
                break;
            }
        }
    }

    // Draw to the TFT
    switch (rag->state)
    {
        default:
        case RGS_MENU:
        {
            clearPxTft();
            break;
        }
        case RGS_PLACING_PIECE:
        case RGS_WAITING:
        {
            ragDrawGame();
            break;
        }
    }
}
/**
 * @brief TODO
 *
 */
static void ragHandleInput(void)
{
    // Check for buttons
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        // Do something?
        if (evt.down)
        {
            bool cursorMoved = false;
            switch (evt.button)
            {
                case PB_UP:
                {
                    cursorMoved = true;
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
                    cursorMoved   = true;
                    rag->cursor.y = (rag->cursor.y + 1) % 3;
                    break;
                }
                case PB_LEFT:
                {
                    cursorMoved = true;
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
                    cursorMoved   = true;
                    rag->cursor.x = (rag->cursor.x + 1) % 3;
                    break;
                }
                case PB_A:
                {
                    if (SELECT_SUBGAME == rag->cursorMode)
                    {
                        cursorMoved          = true;
                        rag->selectedSubgame = rag->cursor;
                        rag->cursor.x        = 1;
                        rag->cursor.y        = 1;
                        rag->cursorMode      = SELECT_CELL;
                    }
                    else if (SELECT_CELL == rag->cursorMode)
                    {
                        // Place the piece
                        rag->subgames[rag->selectedSubgame.x][rag->selectedSubgame.y].game[rag->cursor.x][rag->cursor.y]
                            = (GOING_FIRST == p2pGetPlayOrder(&rag->p2p)) ? rag->p1Piece : rag->p2Piece;

                        // Send move to the other swadge
                        ragMsgPlacePiece_t place = {
                            .type            = MSG_PLACE_PIECE,
                            .selectedSubgame = rag->selectedSubgame,
                            .selectedCell    = rag->cursor,
                        };
                        p2pSendMsg(&rag->p2p, (const uint8_t*)&place, sizeof(place), ragMsgTxCbFn);

                        // Switch to waiting
                        rag->state = RGS_WAITING;
                    }
                    break;
                }
                case PB_B:
                {
                    if (SELECT_CELL == rag->cursorMode)
                    {
                        cursorMoved     = true;
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

            // Send cursor movement to the other Swadge
            if (cursorMoved)
            {
                // Send cursor type to other swadge
                ragMsgMoveCursor_t move = {
                    .type            = MSG_MOVE_CURSOR,
                    .cursorMode      = rag->cursorMode,
                    .selectedSubgame = rag->selectedSubgame,
                    .cursor          = rag->cursor,
                };
                p2pSendMsg(&rag->p2p, (const uint8_t*)&move, sizeof(move), ragMsgTxCbFn);
            }
        }
    }
}

/**
 * @brief TODO
 *
 */
static void ragDrawGame(void)
{
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
            // Get this subgame's rectangle
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

            // Check if the subgame has a winner
            switch (rag->subgames[subX][subY].winner)
            {
                case RAG_RING:
                {
                    // Draw big winner sprite
                    drawWsgSimple(&rag->piece_x_big, sX0, sY0);
                    break;
                }
                case RAG_GEM:
                {
                    // Draw big winner sprite
                    drawWsgSimple(&rag->piece_o_big, sX0, sY0);
                    break;
                }
                default:
                case RAG_EMPTY:
                {
                    // Draw the subgame. For each cell
                    for (int cellY = 0; cellY < 3; cellY++)
                    {
                        for (int cellX = 0; cellX < 3; cellX++)
                        {
                            // Get the location for this cell
                            int16_t cX0 = sX0 + (cellX * cellSize);
                            int16_t cY0 = sY0 + (cellY * cellSize);

                            // Draw sprites
                            switch (rag->subgames[subX][subY].game[cellX][cellY])
                            {
                                default:
                                case RAG_EMPTY:
                                {
                                    break;
                                }
                                case RAG_RING:
                                {
                                    drawWsgSimple(&rag->piece_x_small, cX0, cY0);
                                    break;
                                }
                                case RAG_GEM:
                                {
                                    drawWsgSimple(&rag->piece_o_small, cX0, cY0);
                                    break;
                                }
                            }

                            // If selected, draw the cursor on this cell
                            if (SELECT_CELL == rag->cursorMode &&                                   //
                                rag->selectedSubgame.x == subX && rag->selectedSubgame.y == subY && //
                                rag->cursor.x == cellX && rag->cursor.y == cellY)
                            {
                                // Get the other rectangle coordinates
                                int16_t cX1 = cX0 + cellSize - 1;
                                int16_t cY1 = cY0 + cellSize - 1;
                                // Draw the cursor
                                drawRect(cX0, cY0, cX1, cY1, c005);
                            }
                        }
                    }
                    break;
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
    int16_t cellWidth  = (x1 - x0) / 3;
    int16_t cellHeight = (y1 - y0) / 3;

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
    // TODO handle connection states and disconnection
    switch (evt)
    {
        case CON_STARTED:
        {
            break;
        }
        case RX_GAME_START_ACK:
        {
            break;
        }
        case RX_GAME_START_MSG:
        {
            break;
        }
        case CON_ESTABLISHED:
        {
            // If going first
            if (GOING_FIRST == p2pGetPlayOrder(p2p))
            {
                // Set own piece type
                rag->p1Piece = RAG_RING;

                // Send piece type to other swadge
                ragMsgSelectPiece_t sel = {
                    .type  = MSG_SELECT_PIECE,
                    .piece = rag->p1Piece,
                };
                p2pSendMsg(&rag->p2p, (const uint8_t*)&sel, sizeof(sel), ragMsgTxCbFn);
            }
            break;
        }
        case CON_LOST:
        {
            break;
        }
    }
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
    // Make sure there is a type to switch on
    if (len < 1)
    {
        return;
    }

    // TODO
    switch (payload[0])
    {
        case MSG_SELECT_PIECE:
        {
            if (len == sizeof(ragMsgSelectPiece_t))
            {
                const ragMsgSelectPiece_t* rxSel = (const ragMsgSelectPiece_t*)payload;
                // TODO record other piece

                // If this is the second player
                if (GOING_SECOND == p2pGetPlayOrder(&rag->p2p))
                {
                    // Save p1's piece
                    rag->p1Piece = rxSel->piece;

                    // Send p2's piece to p1
                    rag->p2Piece = RAG_GEM;

                    // Send sprite selection to other swadge
                    ragMsgSelectPiece_t txSel = {
                        .type  = MSG_SELECT_PIECE,
                        .piece = rag->p2Piece,
                    };
                    p2pSendMsg(&rag->p2p, (const uint8_t*)&txSel, sizeof(txSel), ragMsgTxCbFn);

                    // Wait for p1 to make the first move
                    rag->state = RGS_WAITING;

                    // Debug
                    printf("%d, %d, %d\n", p2pGetPlayOrder(&rag->p2p), rag->p1Piece, rag->p2Piece);
                }
                else
                {
                    // Received piece type
                    rag->p2Piece = rxSel->piece;

                    // Make the first move
                    rag->state = RGS_PLACING_PIECE;

                    // Debug
                    printf("%d, %d, %d\n", p2pGetPlayOrder(&rag->p2p), rag->p1Piece, rag->p2Piece);
                }
            }
            break;
        }
        case MSG_MOVE_CURSOR:
        {
            // Length check
            if (len == sizeof(ragMsgMoveCursor_t))
            {
                // Move the cursor
                const ragMsgMoveCursor_t* move = (const ragMsgMoveCursor_t*)payload;
                rag->cursorMode                = move->cursorMode;
                rag->selectedSubgame           = move->selectedSubgame;
                rag->cursor                    = move->cursor;

                // No state transition
            }
            break;
        }
        case MSG_PLACE_PIECE:
        {
            // Length check
            if (len == sizeof(ragMsgPlacePiece_t))
            {
                // Place the piece
                const ragMsgPlacePiece_t* place = (const ragMsgPlacePiece_t*)payload;
                rag->subgames[place->selectedSubgame.x][place->selectedSubgame.y]
                    .game[place->selectedCell.x][place->selectedCell.y]
                    = (GOING_FIRST == p2pGetPlayOrder(&rag->p2p)) ? rag->p2Piece : rag->p1Piece;

                // Transition state to placing a piece
                rag->state = RGS_PLACING_PIECE;
            }
            break;
        }
    }
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
