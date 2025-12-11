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
    wsg_t logo;
} finder_t;

//==============================================================================
// Const variables
//==============================================================================

static const char findingFacesModeName[]                 = "Finding Faces";


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

    initShapes();

    loadWsg(PROTOMEN_SMALL_WHITE_WSG, &finder->logo, true);
}
static void findingMainLoop(int64_t elapsedUs)
{
    drawWsg(&finder->logo, 65, 80, false, false, 0);
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {}
}
static void findingExitMode(void)
{
    heap_caps_free(finder);
}