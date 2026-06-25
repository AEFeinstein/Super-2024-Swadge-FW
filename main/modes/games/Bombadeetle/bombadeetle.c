//==============================================================================
// Includes
//==============================================================================
#include "bombadeetle.h"

const char bombadeetleModeName[] = "Bombadeetle";

static void bombadeetleEnterMode(void);
static void bombadeetleExitMode(void);
static void bombadeetleMainLoop(int64_t elapsedUs);

swadgeMode_t bombadeetleMode = {
    .modeName                   = bombadeetleModeName,
    .wifiMode                   = NO_WIFI,

    .overrideUsb                = false,
    .usesAccelerometer          = false,
    .usesThermometer            = false,
    .overrideSelectBtn          = false,
    .fnEnterMode                = bombadeetleEnterMode,
    .fnExitMode                 = bombadeetleExitMode,
    .fnMainLoop                 = bombadeetleMainLoop,
    .fnAudioCallback            = NULL,
    .fnBackgroundDrawCallback   = NULL,
    .fnEspNowRecvCb             = NULL,
    .fnEspNowSendCb             = NULL,
    .fnAdvancedUSB              = NULL,
};

static void bombadeetleEnterMode()
{

}

static void bombadeetleExitMode()
{

}

static void bombadeetleMainLoop(int64_t elapsedUs)
{

}