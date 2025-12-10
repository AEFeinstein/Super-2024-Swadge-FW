//==============================================================================
// Includes
//==============================================================================
#include "findingFaces.h"
//#include "mainMenu.h"
//#include "settingsManager.h"
//#include "textEntry.h"

const char findingFacesModeName[]                 = "Finding Faces";
//static const char* menuOptions[] = {
   // "New Game", "Continue", "Exit",};

static void findingEnterMode(void);
static void findingExitMode(void);
static void findingMainLoop(int64_t elapsedUs);

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

static void findingEnterMode(void)
{

}
static void findingMainLoop(int64_t elapsedUs)
{

}
static void findingExitMode(void)
{

}