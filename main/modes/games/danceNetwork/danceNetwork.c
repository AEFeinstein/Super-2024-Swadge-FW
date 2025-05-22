#include "danceNetwork.h"

const char danceNetworkName[] = "Alpha Pulse: Dance Network";

static void danceNetworkEnterMode(void);
static void danceNetworkExitMode(void);
static void danceNetworkMainLoop(int64_t elapsedUs);

swadgeMode_t danceNetworkMode = {
    .modeName                 = danceNetworkName, // Assign the name we created here
    .wifiMode                 = NO_WIFI,        // If we want WiFi 
    .overrideUsb              = false,          // Overrides the default USB behavior.
    .usesAccelerometer        = false,          // If we're using motion controls
    .usesThermometer          = false,          // If we're using the internal thermometer
    .overrideSelectBtn        = false,          // The select/Menu button has a default behavior. If you want to override it, you can set this to true but you'll need to re-implement the behavior.
    .fnEnterMode              = danceNetworkEnterMode, // The enter mode function
    .fnExitMode               = danceNetworkExitMode,  // The exit mode function
    .fnMainLoop               = danceNetworkMainLoop,  // The loop function
    .fnAudioCallback          = NULL,           // If the mode uses the microphone
    .fnBackgroundDrawCallback = NULL,           // Draws a section of the display
    .fnEspNowRecvCb           = NULL,           // If using Wifi, add the receive function here
    .fnEspNowSendCb           = NULL,           // If using Wifi, add the send function here
    .fnAdvancedUSB            = NULL, // If using advanced USB things.
};

static void danceNetworkEnterMode(void)
{
}

static void danceNetworkExitMode(void)
{
}

static void danceNetworkMainLoop(int64_t elapsedUs)
{
}