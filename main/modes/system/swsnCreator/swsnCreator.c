//==============================================================================
// Includes
//==============================================================================

#include "swsnCreator.h"
#include "mainMenu.h"
#include "swadgesona.h"

//==============================================================================
// Define
//==============================================================================

#define MAX_SWSN_SLOTS    14
#define SLIDE_TIME_AMOUNT (1000 * 500)
#define NUM_PIXELS        100
#define STEP              (SLIDE_TIME_AMOUNT / NUM_PIXELS)

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
    BODY_PART,
    SLIDING,
    PANEL_OPEN,
} creatorStates_t;

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
    creatorStates_t cState;
    wsg_t* tabSprs;
    swadgesona_t activeSona;
    int selection; // Which body part we're on
    bool out;
    int64_t animTimer;
} swsnCreatorData_t;

//==============================================================================
// Function declarations
//==============================================================================

static void swsnEnterMode(void);
static void swsnExitMode(void);
static void swsnLoop(int64_t elapsedUs);

static bool swsnMenuCb(const char* label, bool selected, uint32_t settingVal);

static bool slideTab(int selected, bool out, uint64_t elapsedUs);
static void drawTab(int xOffset, int y, int scale, bool flip, int labelIdx, bool selected);
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
            switch (scd->cState)
            {
                case BODY_PART:
                {
                    while (checkButtonQueueWrapper(&evt))
                    {
                        if (evt.down)
                        {
                            if (evt.button & PB_UP)
                            {
                                scd->selection--;
                                if (scd->selection < 0)
                                {
                                    scd->selection = NUM_CREATOR_OPTIONS - 1;
                                }
                            }
                            else if (evt.button & PB_DOWN)
                            {
                                scd->selection++;
                                if (scd->selection >= NUM_CREATOR_OPTIONS)
                                {
                                    scd->selection = 0;
                                }
                            }
                            else if (evt.button & PB_LEFT && scd->selection > 4 && scd->selection < 10)
                            {
                                scd->selection -= 5;
                            }
                            else if (evt.button & PB_RIGHT && scd->selection < 5)
                            {
                                scd->selection += 5;
                            }
                            else if (evt.button & PB_A)
                            {
                                scd->out    = true;
                                scd->cState = SLIDING;
                            }
                        }
                    }
                    // Draw the creator
                    drawCreator();
                    break;
                }
                case SLIDING:
                {
                    while (checkButtonQueueWrapper(&evt))
                    {
                        if (evt.down)
                        {
                            if (scd->out)
                            {
                                scd->cState = PANEL_OPEN;
                            }
                            else
                            {
                                scd->cState = BODY_PART;
                            }
                        }
                    }
                    if (slideTab(scd->selection, scd->out, elapsedUs))
                    {
                        if (scd->out)
                        {
                            scd->cState = PANEL_OPEN;
                        }
                        else
                        {
                            scd->cState = BODY_PART;
                        }
                    }
                    break;
                }
                case PANEL_OPEN:
                {
                    scd->out    = false;
                    scd->cState = SLIDING;
                    break;
                }
                default:
                {
                    break;
                }
            }
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

static bool swsnMenuCb(const char* label, bool selected, uint32_t settingVal)
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
    return false;
}

static bool slideTab(int selected, bool out, uint64_t elapsedUs)
{
    // Update variables
    scd->animTimer += elapsedUs;
    // Change direction based selection
    bool leftSide = scd->selection < 5;

    // Draw BG
    clearPxTft();
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c445);

    // Draw swadgesona
    int64_t steps = scd->animTimer;
    int x         = (TFT_WIDTH - (scd->activeSona.image.w * 3)) >> 1;
    int offset    = 0;
    if (leftSide)
    {
        if (!out)
        {
            x += NUM_PIXELS;
            offset += NUM_PIXELS;
        }
        while (steps > STEP)
        {
            steps -= STEP;
            x      = (out) ? x + 1 : x - 1;
            offset = (out) ? offset + 1 : offset - 1;
        }
        drawWsgSimpleScaled(&scd->activeSona.image, x, TFT_HEIGHT - 192, 3, 3);
        for (int idx = 0; idx < NUM_CREATOR_OPTIONS - 1; idx++)
        {
            if (idx == scd->selection)
            {
                drawTab(offset, 20 + (idx % 5) * 36, 3, (idx > 4), idx + 2, scd->selection == idx);
            }
            else
            {
                drawTab((idx > 4) ? offset : 0, 20 + (idx % 5) * 36, 3, (idx > 4), idx + 2, scd->selection == idx);
            }
        }
        fillDisplayArea(0, 32, offset, TFT_HEIGHT - 28, c555);
        for (int i = 0; i < 3; i++)
        {
            drawLineFast(0, TFT_HEIGHT - (29 + i), offset, TFT_HEIGHT - (29 + i), c255);
        }
    }
    else
    {
        if (!out)
        {
            x -= NUM_PIXELS;
            offset -= NUM_PIXELS;
        }
        while (steps > STEP)
        {
            steps -= STEP;
            x      = (out) ? x - 1 : x + 1;
            offset = (out) ? offset - 1 : offset + 1;
        }
        drawWsgSimpleScaled(&scd->activeSona.image, x, TFT_HEIGHT - 192, 3, 3);
        for (int idx = 0; idx < NUM_CREATOR_OPTIONS - 1; idx++)
        {
            if (idx == scd->selection)
            {
                drawTab(offset, 20 + (idx % 5) * 36, 3, (idx > 4), idx + 2, scd->selection == idx);
            }
            else
            {
                drawTab((idx < 5) ? offset : 0, 20 + (idx % 5) * 36, 3, (idx > 4), idx + 2, scd->selection == idx);
            }
        }
        fillDisplayArea(TFT_WIDTH + offset, 32, TFT_WIDTH, TFT_HEIGHT - 28, c555);
        for (int i = 0; i < 3; i++)
        {
            drawLineFast(TFT_WIDTH + offset, TFT_HEIGHT - (29 + i), TFT_WIDTH, TFT_HEIGHT - (29 + i), c255);
        }
    }

    // Exit
    if (scd->animTimer >= SLIDE_TIME_AMOUNT)
    {
        scd->animTimer = 0;
        if (scd->out)
        {
            scd->cState = PANEL_OPEN;
        }
        else
        {
            scd->cState = BODY_PART;
        }
    }
    return false;
}

static void drawTab(int xOffset, int y, int scale, bool flip, int labelIdx, bool selected)
{
    int x = 0;
    if (!selected)
    {
        x -= 5 * scale;
    }
    wsg_t* tab = &scd->tabSprs[0];
    if (flip)
    {
        tab = &scd->tabSprs[1];
        x   = TFT_WIDTH - scd->tabSprs[1].w * scale;
        if (!selected)
        {
            x += 5 * scale;
        }
    }
    drawWsgSimpleScaled(tab, xOffset + x, y, scale, scale);
    drawWsgSimpleScaled(&scd->tabSprs[labelIdx], xOffset + x, y, scale, scale);
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
        drawTab(0, 20 + (idx % 5) * 36, 3, (idx > 4), idx + 2, scd->selection == idx);
    }
}