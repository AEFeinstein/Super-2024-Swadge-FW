#include "heyListen.h"
const char heyListenModeName[] = "Hey Listen";

static void heyListenEnterMode(void);
static void heyListenExitMode(void);
static void heyListenMainLoop(int64_t elapsedUs);

swadgeMode_t heyListenMode = {
    .modeName                 = heyListenModeName,  // Assign the name we created here
    .wifiMode                 = false,         // If we want WiFi. WiFi is expensive computationally/battery-wise, so disable 
                                                 // it if you're not going to use it.
    .overrideUsb              = false,           // Overrides the default USB behavior. This is helpful for the game controller 
                                                 // mode but unlikely to be useful for your game.
    .usesAccelerometer        = false,           // If we're using motion controls
    .usesThermometer          = false,           // If we're using the internal thermometer
    .overrideSelectBtn        = false,           // The select/Menu button has a default behavior. If you want to override it, 
                                                 // you can set this to true but you'll need to re-implement the 
                                                 // 'return to main menu' behavior.
    .fnEnterMode              = heyListenEnterMode, // The enter mode function
    .fnExitMode               = heyListenExitMode,  // The exit mode function
    .fnMainLoop               = heyListenMainLoop,  // The loop function
    .fnAudioCallback          = NULL,            // If the mode uses the microphone
    .fnBackgroundDrawCallback = NULL,            // Draws a section of the display
    .fnEspNowRecvCb           = NULL,            // If using Wifi, add the receive function here
    .fnEspNowSendCb           = NULL,            // If using Wifi, add the send function here
    .fnAdvancedUSB            = NULL,            // If using advanced USB things.
};


static void heyListenEnterMode()
{
}

static void heyListenExitMode()
{
}

static void heyListenMainLoop(int64_t elapsedUs)
{
}