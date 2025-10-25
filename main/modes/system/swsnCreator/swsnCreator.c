//==============================================================================
// Includes
//==============================================================================

#include "swsnCreator.h"
#include "mainMenu.h"
#include "swadgesona.h"

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

static const cnfsFileIdx_t tabImages[]
    = {OPEN_TAB_LEFT_WSG, OPEN_TAB_RIGHT_WSG,   SWSN_SKIN_WSG, SWSN_EYEBROW_WSG, SWSN_EYE_WSG,     SWSN_EAR_WSG,
       SWSN_MOUTH_WSG,    SWSN_FACIAL_HAIR_WSG, SWSN_HAIR_WSG, SWSN_HATS_WSG,    SWSN_GLASSES_WSG, SWSN_SHIRT_WSG};

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

typedef enum
{
    // Body
    SKIN,
    EYEBROWS,
    EYES,
    MOUTH,
    EARS,
    // Equipment
    BODY_MODS,
    HAIR,
    HAT,
    GLASSES,
    CLOTHES,
    // Exit
    EXIT,
    NUM_CREATOR_OPTIONS
} creatorSelections_t;

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

    // Creator
    wsg_t* tabSprs;
    swadgesona_t activeSona;
    int selection; // Which body part we're on

} swsnCreatorData_t;

//==============================================================================
// Function declarations
//==============================================================================

static void swsnEnterMode(void);
static void swsnExitMode(void);
static void swsnLoop(int64_t elapsedUs);

static void swsnMenuCb(const char* label, bool selected, uint32_t settingVal);

static void drawTab(int y, int scale, bool flip, int labelIdx);
static void drawCreator(void);

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

static void swsnEnterMode(void)
{
    scd           = (swsnCreatorData_t*)heap_caps_calloc(1, sizeof(swsnCreatorData_t), MALLOC_CAP_8BIT);
    scd->menu     = initMenu(sonaMenuName, swsnMenuCb);
    scd->renderer = initMenuMegaRenderer(NULL, NULL, NULL);

    scd->tabSprs = heap_caps_calloc(ARRAY_SIZE(tabImages), sizeof(wsg_t), MALLOC_CAP_8BIT);
    for (int idx = 0; idx < ARRAY_SIZE(tabImages); idx++)
    {
        loadWsg(tabImages[idx], &scd->tabSprs[idx], true);
    }

    // Setup menu options
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

static void swsnExitMode(void)
{
    deinitMenuMegaRenderer(scd->renderer);
    deinitMenu(scd->menu);
    for (int idx = 0; idx < ARRAY_SIZE(tabImages); idx++)
    {
        freeWsg(&scd->tabSprs[idx]);
    }
    heap_caps_free(scd->tabSprs);
    free(scd);
}

static void swsnLoop(int64_t elapsedUs)
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
            while (checkButtonQueueWrapper(&evt))
            {
                if (evt.down)
                {
                    if (evt.button & PB_UP)
                    {
                        scd->selection--;
                    }
                }
            }
            // Apply the data

            // Draw the creator
            drawCreator();
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

static void swsnMenuCb(const char* label, bool selected, uint32_t settingVal)
{
    if (selected)
    {
        if (label == sonaSlotUninitialized)
        {
            // Load the creator
            generateRandomSwadgesona(&scd->activeSona);
            scd->state = CREATING;
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

static void drawTab(int y, int scale, bool flip, int labelIdx)
{
    int x = 0;
    wsg_t* tab = &scd->tabSprs[0];
    if (flip)
    {
        tab    = &scd->tabSprs[1];
        x = TFT_WIDTH -  scd->tabSprs[1].w * scale;
    }
    drawWsgSimpleScaled(tab, x, y, scale, scale);
    drawWsgSimpleScaled(&scd->tabSprs[labelIdx], x, y, scale, scale);
}

static void drawCreator(void)
{
    // Background
    clearPxTft();
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c445);

    // Draw the swadgesona face
    // TODO: slide to avoid tab panels
    drawWsgSimpleScaled(&scd->activeSona.image, (TFT_WIDTH - (scd->activeSona.image.w * 3)) >> 1, TFT_HEIGHT - 192, 3,
                        3);

    // Draw the tabs
    for (int idx = 0; idx < NUM_CREATOR_OPTIONS - 1; idx++)
    {
        drawTab(20 + (idx % 5) * 36, 3, (idx > 4), idx + 2);
    }
}