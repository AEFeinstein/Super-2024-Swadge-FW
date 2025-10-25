//==============================================================================
// Includes
//==============================================================================

#include "swsnCreator.h"
#include "mainMenu.h"

//==============================================================================
// Define
//==============================================================================

#define MAX_SWSN_SLOTS 14

//==============================================================================
// Consts
//==============================================================================

const char sonaModeName[]                 = "Swadgesona Creator";
static const char sonaMenuName[]          = "Sona Creator";
static const char sonaSlotUninitialized[] = "Uninitialized";

static const char* const menuOptions[] = {
    "Create!",
    "View",
    "Exit",
};

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    MENU,
    CREATING,
    NAMING,
    VIEWING,
} swsnCreatorState_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    // Main
    swsnCreatorState_t state;

    // Menu
    menu_t* menu;
    menuMegaRenderer_t* renderer;

} swsnCreatorData_t;

//==============================================================================
// Function declarations
//==============================================================================

void swsnEnterMode(void);
void swsnExitMode(void);
void swsnLoop(int64_t elapsedUs);

void swsnMenuCb(const char* label, bool selected, uint32_t settingVal);

//==============================================================================
// Variables
//==============================================================================

swadgeMode_t swsnCreatorMode = {
    .modeName          = sonaModeName,
    .wifiMode          = NO_WIFI,
    .usesAccelerometer = true,
    .fnEnterMode       = swsnEnterMode,
    .fnExitMode        = swsnExitMode,
    .fnMainLoop        = swsnLoop,
};

swsnCreatorData_t* scd;

//==============================================================================
// Functions
//==============================================================================

void swsnEnterMode(void)
{
    scd           = (swsnCreatorData_t*)heap_caps_calloc(1, sizeof(swsnCreatorData_t), MALLOC_CAP_8BIT);
    scd->menu     = initMenu(sonaMenuName, swsnMenuCb);
    scd->renderer = initMenuMegaRenderer(NULL, NULL, NULL);

    // Setup menu options
    // 1 - Create a swadgesona
    //   - Multiple slots (submenu)
    // 2 - View saved swadgesonas
    // 3 - Exit
    scd->menu = startSubMenu(scd->menu, menuOptions[0]);
    for (int16_t idx = 0; idx < MAX_SWSN_SLOTS; idx++)
    {
        // TODO: Load swsn name from NVS when slot is used
        addSingleItemToMenu(scd->menu, sonaSlotUninitialized);
    }
    scd->menu = endSubMenu(scd->menu);
    addSingleItemToMenu(scd->menu, menuOptions[1]);
    addSingleItemToMenu(scd->menu, menuOptions[2]);
}

void swsnExitMode(void)
{
    deinitMenuMegaRenderer(scd->renderer);
    deinitMenu(scd->menu);
    free(scd);
}

void swsnLoop(int64_t elapsedUs)
{
    buttonEvt_t evt;
    switch (scd->state)
    {
        case MENU:
        {
            while (checkButtonQueueWrapper(&evt))
            {
                scd->menu = menuButton(scd->menu, evt);
            }
            drawMenuMega(scd->menu, scd->renderer, elapsedUs);
            break;
        }
        case VIEWING:
        {
            break;
        }
        case CREATING:
        {
            break;
        }
        case NAMING:
        {
            break;
        }
        default:
        {
            break;
        }
    }
}

void swsnMenuCb(const char* label, bool selected, uint32_t settingVal)
{
    if (selected)
    {
        if (label == sonaSlotUninitialized)
        {
            // Load the creator
        }
        else if (label == menuOptions[1])
        {
            // Enter the viewer
        }
        else if (label == menuOptions[2])
        {
            // Exit the mode
            switchToSwadgeMode(&mainMenuMode);
        }
    }
}