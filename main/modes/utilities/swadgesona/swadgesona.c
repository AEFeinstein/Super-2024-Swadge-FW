#include "swadgesona.h"


const char swadgesonaName[] = "Swadgesona";



static void enterMode (void);
static void exitMode (void);
static void runMode (int64_t elapsedUs);

typedef struct 
{
    /* data */
}swadgesona_t;


swadgeMode_t swadgesonaMode = {
    .modeName                 = swadgesonaName,
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = false,
    .usesAccelerometer        = false,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = enterMode,
    .fnExitMode               = exitMode,
    .fnMainLoop               = runMode,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = NULL,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL,
};

static swadgesona_t* SS;

static void enterMode (void) {
    SS = (swadgesona_t*)heap_caps_calloc(1,sizeof(swadgesona_t), MALLOC_CAP_8BIT);
};

static void exitMode (void) {
    heap_caps_free(SS);
}

static void runMode (int64_t elapsedUs){
    //This is where things go state machine/vending machine

    
}

