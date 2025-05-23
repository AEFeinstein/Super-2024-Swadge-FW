#include "danceNetwork.h"

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

typedef struct {
    wsg_t isoTile;
} tileData_t;

tileData_t* td;

static void dn_EnterMode(void)
{
    td = (tileData_t*)heap_caps_calloc(1, sizeof(tileData_t), MALLOC_CAP_8BIT);
    loadWsg(DN_ISO_TILE_WSG, &td->isoTile, true);
}

static void dn_ExitMode(void)
{
    freeWsg(&td->isoTile);
    free(td);
}

static void dn_MainLoop(int64_t elapsedUs)
{
    dn_drawScene();
}

static void dn_drawScene(void)
{
    dn_drawTiles();
}

static void dn_drawTiles(void)
{
    // Draw the tiles
    for (int y = 0; y < 5; y++)
    {
        for (int x = 0; x < 5; x++)
        {
            int drawX = (TFT_WIDTH >> 1) - (41 >> 1) + (x - y) * (46 >> 1);
            int drawY = (TFT_HEIGHT >> 1) - 15 + (x + y) * (25 >> 1);
            drawWsgSimple(&td->isoTile, drawX, drawY);
        }
    }
}

static void dn_BackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
    // Fill the flat background color
    paletteColor_t* frameBuf = getPxTftFramebuffer();
    memset(&frameBuf[(y * TFT_WIDTH) + x], c212, sizeof(paletteColor_t) * w * h);
}

