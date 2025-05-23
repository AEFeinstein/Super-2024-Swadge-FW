#include "danceNetwork.h"
#include "dn_typedef.h"

const char danceNetworkName[] = "Alpha Pulse: Dance Network";

static void dn_EnterMode(void);
static void dn_ExitMode(void);
static void dn_MainLoop(int64_t elapsedUs);
static void dn_drawScene(void);
static void dn_drawTiles(void);
static void dn_BackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);

swadgeMode_t danceNetworkMode = {
    .modeName                 = danceNetworkName, // Assign the name we created here
    .wifiMode                 = NO_WIFI,        // If we want WiFi 
    .overrideUsb              = false,          // Overrides the default USB behavior.
    .usesAccelerometer        = false,          // If we're using motion controls
    .usesThermometer          = false,          // If we're using the internal thermometer
    .overrideSelectBtn        = false,          // The select/Menu button has a default behavior. If you want to override it, you can set this to true but you'll need to re-implement the behavior.
    .fnEnterMode              = dn_EnterMode, // The enter mode function
    .fnExitMode               = dn_ExitMode,  // The exit mode function
    .fnMainLoop               = dn_MainLoop,  // The loop function
    .fnAudioCallback          = NULL,           // If the mode uses the microphone
    .fnBackgroundDrawCallback = dn_BackgroundDrawCallback,           // Draws a section of the display
    .fnEspNowRecvCb           = NULL,           // If using Wifi, add the receive function here
    .fnEspNowSendCb           = NULL,           // If using Wifi, add the send function here
    .fnAdvancedUSB            = NULL, // If using advanced USB things.
};

typedef struct{
    wsg_t groundTile;
} sprites_t;

typedef struct{
    uint16_t yOffset;
    int16_t yVel;
} tileData_t;

typedef struct {
    sprites_t sprites;
    tileData_t tiles[BOARD_SIZE][BOARD_SIZE];
    uint8_t selection[2];//x and y indices of the selected tile
} gameData_t;

gameData_t* gameData;

static void dn_EnterMode(void)
{
    gameData = (gameData_t*)heap_caps_calloc(1, sizeof(gameData_t), MALLOC_CAP_8BIT);
    for (int y = 0; y < BOARD_SIZE; y++)
    {
        for (int x = 0; x < BOARD_SIZE; x++)
        {
            gameData->tiles[y][x].yOffset = ((TFT_HEIGHT >> 2)-15) << DECIMAL_BITS;
        }
    }
    gameData->selection[0] = 2;
    gameData->selection[1] = 2;
    gameData->tiles[gameData->selection[0]][gameData->selection[1]].yOffset = (TFT_HEIGHT >> 2) << DECIMAL_BITS;
    loadWsg(DN_GROUND_TILE_WSG, &gameData->sprites.groundTile, true);
}

static void dn_ExitMode(void)
{
    freeWsg(&gameData->sprites.groundTile);
    free(gameData);
}

static void dn_MainLoop(int64_t elapsedUs)
{
    buttonEvt_t evt;
    while(checkButtonQueueWrapper(&evt))
    {
        if(evt.down)
        {
            if(evt.button == PB_UP && gameData->selection[1] > 0)
            {
                gameData->selection[1]--;
                gameData->tiles[gameData->selection[1]][gameData->selection[0]].yOffset = (TFT_HEIGHT >> 2) << DECIMAL_BITS;
                gameData->tiles[gameData->selection[1]][gameData->selection[0]].yVel = 1000;
            }
            else if(evt.button == PB_DOWN && gameData->selection[1] < BOARD_SIZE - 1)
            {
                gameData->selection[1]++;
                gameData->tiles[gameData->selection[1]][gameData->selection[0]].yOffset = (TFT_HEIGHT >> 2) << DECIMAL_BITS;
                gameData->tiles[gameData->selection[1]][gameData->selection[0]].yVel = 1000;
            }
            else if(evt.button == PB_LEFT && gameData->selection[0] > 0)
            {
                gameData->selection[0]--;
                gameData->tiles[gameData->selection[1]][gameData->selection[0]].yOffset = (TFT_HEIGHT >> 2) << DECIMAL_BITS;
                gameData->tiles[gameData->selection[1]][gameData->selection[0]].yVel = 1000;
            }
            else if(evt.button == PB_RIGHT && gameData->selection[0] < BOARD_SIZE - 1)
            {
                gameData->selection[0]++;
                gameData->tiles[gameData->selection[1]][gameData->selection[0]].yOffset = (TFT_HEIGHT >> 2) << DECIMAL_BITS;
                gameData->tiles[gameData->selection[1]][gameData->selection[0]].yVel = 1000;
            }
        }
    }

    //perform hooke's law on neighboring tiles
    for (int y = 0; y < BOARD_SIZE; y++)
    {
        for (int x = 0; x < BOARD_SIZE; x++)
        {
            // Get the current tile
            tileData_t* tile = &gameData->tiles[y][x];
            int8_t dampen = 3;
            if(x == gameData->selection[0] && y == gameData->selection[1])
            {
                //the selected tile approaches a particular offset
                tile->yVel += (((int16_t)(((TFT_HEIGHT >> 2) << DECIMAL_BITS) - tile->yOffset)) / 3);
            }
            else
            {
                //all unselected tiles approach neighboring tiles
                if (y > gameData->selection[1])
                {
                    tile->yVel += (((int16_t)(gameData->tiles[y - 1][x].yOffset - tile->yOffset)) / 2);
                    dampen += y - gameData->selection[1];
                }
                if (y < gameData->selection[1])
                {
                    tile->yVel += (((int16_t)(gameData->tiles[y + 1][x].yOffset - tile->yOffset)) / 2);
                    dampen += gameData->selection[1] - y;
                }
                if (x > gameData->selection[0])
                {
                    tile->yVel += (((int16_t)(gameData->tiles[y][x - 1].yOffset - tile->yOffset)) / 2);
                    dampen += x - gameData->selection[0];
                }
                if (x < gameData->selection[0])
                {
                    tile->yVel += (((int16_t)(gameData->tiles[y][x + 1].yOffset - tile->yOffset)) / 2);
                    dampen += gameData->selection[0] - x;
                }
            }

            tile->yVel /= dampen;

            // Update position with smaller time step
            uint16_t newYOffset = tile->yOffset + tile->yVel * (elapsedUs >> 14);
            // If the the yOffset would wrap around
            if(((tile->yOffset & 0x8000) && !(newYOffset & 0x8000) && tile->yVel > 0) ||
                (!(tile->yOffset & 0x8000) && (newYOffset & 0x8000) && tile->yVel < 0))
            {
                //print a message
                ESP_LOGI("Dance Network", "Tile %d,%d yOffset hit the limit", x, y);
                // Set yVel to 0
                tile->yVel = 0;
            }
            else{
                tile->yOffset  = newYOffset;
            }
        }
    }
    dn_drawScene();
}

static void dn_drawScene(void)
{
    dn_drawTiles();
}

static void dn_drawTiles(void)
{
    // Draw the tiles
    for (int y = 0; y < BOARD_SIZE; y++)
    {
        for (int x = 0; x < BOARD_SIZE; x++)
        {
            int drawX = (TFT_WIDTH >> 1) - (41 >> 1) + (x - y) * (46 >> 1);
            int drawY = (TFT_HEIGHT >> 1) + (TFT_HEIGHT >> 2) - 15 - (gameData->tiles[y][x].yOffset >> DECIMAL_BITS) + (x + y) * (25 >> 1);
            drawWsgSimple(&gameData->sprites.groundTile, drawX, drawY);
        }
    }
}

static void dn_BackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
    // Fill the flat background color
    paletteColor_t* frameBuf = getPxTftFramebuffer();
    memset(&frameBuf[(y * TFT_WIDTH) + x], c212, sizeof(paletteColor_t) * w * h);
}

