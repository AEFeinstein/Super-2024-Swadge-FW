#include "campIdle.h"
 
const char campModeName[] = "Cozy Camping";
 
static void campEnterMode(void);
static void campExitMode(void);
static void campMainLoop(int64_t elapsedUs);
 
swadgeMode_t campIdleMode = {
    .modeName                 = campModeName,
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = false,
    .usesAccelerometer        = false,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = campEnterMode,
    .fnExitMode               = campExitMode,
    .fnMainLoop               = campMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = NULL,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL,
};
 
static void campEnterMode()
{
}
 
static void campExitMode()
{
}
 
static void campMainLoop(int64_t elapsedUs)
{
}