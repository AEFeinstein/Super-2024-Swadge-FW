#include "swadgesona.h"

#define MAX_MARKINGS 2
#define MAX_STR_LEN  17

const char swadgesonaName[] = "Swadgesona";

static void enterMode(void);
static void exitMode(void);
static void runMode(int64_t elapsedUs);
static void draw(void);

typedef enum
{
    SWADGESONA_SMALL,
    SWADGESONA_MED,
    SWADGESONA_TALL,
} swadgesonaHeights_t;

typedef struct
{
    // body features
    int8_t skinColor;
    swadgesonaHeights_t height;
    paletteColor_t shirt;

    // facial features
    int8_t headShape; // basically chin shapes
    int8_t hairStyle;
    paletteColor_t hairColor; // use for all hair and brows
    paletteColor_t hairHighlight;
    paletteColor_t hairShadow;
    int8_t eyeBrowShape;
    int8_t eyeShape;
    paletteColor_t eyeColor;
    int8_t noseShape;
    int8_t mouth; // TODO; might add color here later
    int8_t bodyMarkings[MAX_MARKINGS];

    char name[MAX_STR_LEN];

} swadgesona_t;

typedef struct
{
    /* data */
    wsg_t test; // file type for images is wsg
    bool isDisplaying;

} swadgesonaMode_t;

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

static swadgesonaMode_t* SS;

static void enterMode(void)
{
    SS = (swadgesonaMode_t*)heap_caps_calloc(1, sizeof(swadgesonaMode_t), MALLOC_CAP_8BIT);
    loadWsg("midi.wsg", &SS->test, true);
}

static void exitMode(void)
{
    freeWsg(&SS->test);
    heap_caps_free(SS);
}

static void runMode(int64_t elapsedUs) // microseconds since the last time it looped
{
    // This is where things go state machine/vending machine
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        if(evt.down && evt.button & PB_A){ // && means both ! means false
            SS->isDisplaying = !SS->isDisplaying;
        }
    }

    draw(); // does not require something in the paren
}

static void draw()
{
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c222);
    if(SS->isDisplaying){
        drawWsgSimple(&SS->test, 120, 120);
    }
    
}