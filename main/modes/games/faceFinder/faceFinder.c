//==============================================================================
// Includes
//==============================================================================
#include "faceFinder.h"
//#include "mainMenu.h"
//#include "settingsManager.h"
//#include "textEntry.h"

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    font_t* ibm;
    wsg_t pointer;

    wsg_t faces[7];
    int64_t timer;
    int32_t stage;

    vec_t pointerCoords; //x and y are int32 allowing us to turn 2billies into 280 X_pixels
    bool pointingRight;
    bool pointingLeft;
    bool pointingUp;
    bool pointingDown;
} finder_t;

//==============================================================================
// Const variables
//==============================================================================

static const char findingFacesModeName[] = "Finding Faces";
static const int32_t TimePerLevel = 20000000; //20 seconds per stage, additive

//==============================================================================
// Function Definitions
//==============================================================================
static void findingEnterMode(void);
static void findingExitMode(void);
static void findingMainLoop(int64_t elapsedUs);


//==============================================================================
// Variables
//==============================================================================
swadgeMode_t findingFacesMode = {
    .modeName                = findingFacesModeName,
    .wifiMode                = NO_WIFI,
    .overrideUsb             = false,
    .usesAccelerometer       = false,
    .usesThermometer         = false,
    .overrideSelectBtn       = false,
    .fnEnterMode             = findingEnterMode,
    .fnExitMode              = findingExitMode,
    .fnMainLoop              = findingMainLoop,
    //.trophyData              = &findingTrophyData,
    .fnBackgroundDrawCallback = NULL,
};
finder_t* finder;

//==============================================================================
// Functions
//==============================================================================

static void findingEnterMode(void)
{
    finder = heap_caps_calloc(1, sizeof(finder_t), MALLOC_CAP_8BIT);
    finder->ibm = getSysFont();
    finder->stage=0;
    finder->timer = TimePerLevel;
    finder->pointingDown=false;
    finder->pointingUp=false;
    finder->pointingLeft=false;
    finder->pointingRight=false;
    finder->pointerCoords = {
        .x = 0,
        .y=0,
    };

    initShapes();

    loadWsg(SWSN_POINTER_NO_GLOVE_NO_LOVE_WSG, &finder->pointer, true);
    loadWsg(FINDER_BATTRICE_WSG, &finder->faces[0], true);
    loadWsg(FINDER_BIGMA_WSG, &finder->faces[1], true);
    loadWsg(FINDER_DUSTCAP_WSG, &finder->faces[2], true);
    loadWsg(FINDER_GARBOTNIK_WSG, &finder->faces[3], true);
    loadWsg(FINDER_KINETIC_DONUT_WSG, &finder->faces[4], true);
    loadWsg(FINDER_PULSE_WSG, &finder->faces[5], true);
    loadWsg(FINDER_SAWTOOTH_WSG, &finder->faces[6], true);
}
static void findingMainLoop(int64_t elapsedUs)
{
    drawRectFilled(0,0,280,240,c234);
    drawWsgSimpleScaled(&finder->faces[finder->stage], 65, 80, 2,2);


    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        if (evt.down)
        {
            switch (evt.button)
            {
                case PB_A:
                {
                    finder->stage = (finder->stage + 1) % 7;
                    break;
                }
            }
        }
    }
}
static void findingExitMode(void)
{
    heap_caps_free(finder);
}