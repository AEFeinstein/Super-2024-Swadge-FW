// TODO menu visible on side with sub menus
// TODO swadgesona display on background as we edit
// TODO save swadgesona to nvs

#include "swadgesonaCreator.h"
#include "swadgesona.h"

const char swadgesonaCreatorName[] = "Swadgesona Creator";

static void enterMode(void);
static void exitMode(void);
static void runMode(int64_t elapsedUs);
static void draw(void);

typedef struct
{
    /* data */
    wsg_t test; // file type for images is wsg
    bool isDisplaying;
    int8_t index; // number for position in menu
    swadgesona_t swadgesona;
    swadgesonaData_t ssd;
} swadgesonaCreatorMode_t;

swadgeMode_t swadgesonaCreatorMode = {
    .modeName                 = swadgesonaCreatorName,
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

static swadgesonaCreatorMode_t* sc; // initializing

static void enterMode(void)
{
    sc = (swadgesonaCreatorMode_t*)heap_caps_calloc(1, sizeof(swadgesonaCreatorMode_t), MALLOC_CAP_8BIT);
    loadWsg(MIDI_WSG, &sc->test, true);
    initSwadgesonaDraw(&sc->ssd);
}

static void exitMode(void)
{
    freeSwadgesonaDraw(&sc->ssd);
    freeWsg(&sc->test);
    heap_caps_free(sc);
}

static void runMode(int64_t elapsedUs) // microseconds since the last time it looped
{
    // This is where things go state machine/vending machine
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        if (evt.down)
        {
            if (evt.button == PB_A)
            {
            }
            else if (evt.button == PB_B)
            {
            }
            else if (evt.button == PB_UP)
            {
                sc->index -= 1;
                if(sc->index<0){
                    sc->index=4;
                }
            }
            else if (evt.button == PB_DOWN)
            {
                sc->index += 1;
                if(sc->index>=5){
                    sc->index=0;
                }
            }
            else if (evt.button == PB_LEFT)
            {
            }
            else if (evt.button == PB_RIGHT)
            {
            }
        }
    }
    sc->swadgesona.skin=sc->index;

    draw(); // does not require something in the paren
}

static void draw()
{
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c222);
    if (sc->isDisplaying)
    {
        drawWsgSimple(&sc->test, 120, 120);
    }
    drawSwadgesona(&sc->ssd, &sc->swadgesona, (TFT_WIDTH - (64 * 4)) >> 1, 32, 4);
}
    