//==============================================================================
// Includes
//==============================================================================

#include "swsnCreator.h"
#include "mainMenu.h"
#include "swadgesona.h"
#include "settingsManager.h"

//==============================================================================
// Define
//==============================================================================

#define MAX_SWSN_SLOTS 14

// Animations
#define SLIDE_TIME_AMOUNT (1000 * 500)
#define NUM_PIXELS        75
#define STEP              (SLIDE_TIME_AMOUNT / NUM_PIXELS)

// Tab Panel Pixel data
#define NUM_TABS     5
#define GRID_ROW     3
#define GRID_COL     4
#define GRID_SIZE    (GRID_ROW * GRID_COL)
#define SWATCH_H     26
#define SWATCH_W     35
#define PADDING      7
#define Y_PADDING    32
#define BOTT_PADDING 28
#define CORNER_RAD   12
#define ARROW_SHIFT  13
#define ARROW_HEIGHT 14
#define SONA_SCALE   3
#define TAB_SPACE    -4

//==============================================================================
// Consts
//==============================================================================

const char sonaModeName[]                 = "Swadgesona Creator";
static const char sonaMenuName[]          = "Sona Creator";
static const char sonaSlotUninitialized[] = "Uninitialized";
static const char cursorNVS[]             = "cursor";

// Main
static const char* const menuOptions[] = {
    "Create!",
    "View",
    "cursor: ",
    "Exit",
};
static const char* const cursorOptions[] = {
    "Default", "Ball",         "Beaker", "Bow",   "Candy Cane", "Carrot", "White Paw", "Black Paw",  "Computer cursor",
    "Donut",   "Hotdog",       "Ender",  "Gem",   "Ghost",      "Hammer", "Heart",     "Leaf",       " Magic Wand",
    "Glove",   "Corner arrow", "Pencil", "Pizza", "Rainbow",    "Ruler",  "Sword",     "Watermelon", "Weiner",
};
static const int32_t cursorSprs[] = {
    SWSN_POINTER_ARROW_WSG,
    SWSN_POINTER_BALL_WSG,
    SWSN_POINTER_BEAKER_WSG,
    SWSN_POINTER_BOW_WSG,
    SWSN_POINTER_CANDY_CANE_WSG,
    SWSN_POINTER_CARROT_WSG,
    SWSN_POINTER_CAT_PAW_WSG,
    SWSN_POINTER_CAT_PAW_B_WSG,
    SWSN_POINTER_COMPUTER_POINTER_WSG,
    SWSN_POINTER_DONUT_WSG,
    SWSN_POINTER_DRESSED_WEINER_WSG,
    SWSN_POINTER_ENDER_WSG,
    SWSN_POINTER_GEM_WSG,
    SWSN_POINTER_GHOST_WSG,
    SWSN_POINTER_HAMMER_WSG,
    SWSN_POINTER_HEART_WSG,
    SWSN_POINTER_LEAF_WSG,
    SWSN_POINTER_MAGIC_WAND_WSG,
    SWSN_POINTER_NO_GLOVE_NO_LOVE_WSG,
    SWSN_POINTER_NO_STEM_WSG,
    SWSN_POINTER_PENCIL_WSG,
    SWSN_POINTER_PIZZA_WSG,
    SWSN_POINTER_RAINBOW_WSG,
    SWSN_POINTER_RULER_WSG,
    SWSN_POINTER_SWORD_WSG,
    SWSN_POINTER_WATERMELON_WSG,
    SWSN_POINTER_WEINER_WSG,
};

// Tab data
static const cnfsFileIdx_t tabImages[] = {
    OPEN_TAB_LEFT_WSG, OPEN_TAB_RIGHT_WSG, SWSN_SKIN_WSG,        SWSN_EYEBROW_WSG, SWSN_EYE_WSG,
    SWSN_MOUTH_WSG,    SWSN_EAR_WSG,       SWSN_FACIAL_HAIR_WSG, SWSN_HAIR_WSG,    SWSN_HATS_WSG,
    SWSN_GLASSES_WSG,  SWSN_SHIRT_WSG,     SWSN_ARROW_R_WSG,     SWSN_ARROW_L_WSG,
};

// Panel data
static const paletteColor_t skinSwatch[] = {
    c544, c545, c543, c432, c321, c210, c255, c444, c243, c545, c334, c422,
};
static const paletteColor_t clothesSwatch[] = {
    c000, c004, c210, c455, c435, c203, c222, c240, c505, c503, c233,
    c554, c102, c521, c544, c545, c305, c401, c543, c033, c555, c541,
};
static const cnfsFileIdx_t bodymarksWsgs[] = {
    BM_BEARD_WSG,      BM_BLUSH_WSG,         BM_BOTTOM_MOLE_WSG,  BM_BOTTOM_WSG,       BM_CHIN_WSG,
    BM_CHIN_PATCH_WSG, BM_CHIN_STRAP_WSG,    BM_COP_WSG,          BM_COWBOY_WSG,       BM_EYE_MOLE_WSG,
    BM_FRECKLES_WSG,   BM_FULL_SCRAGGLY_WSG, BM_HALF_STACHE_WSG,  BM_HEART_STACHE_WSG, BM_LESS_WISE_WSG,
    BM_MAGICIAN_WSG,   BM_MARILYN_WSG,       BM_OLD_WSG,          BM_PILLOW_WSG,       BM_SAND_P_WSG,
    BM_SCRAGGLY_WSG,   BM_SMALL_CURL_WSG,    BM_SMALL_STACHE_WSG, BM_SOUL_PATCH_WSG,   BM_STACHE_AND_STRAP_WSG,
    BM_STRONGMAN_WSG,  BM_THIN_CHIN_WSG,     BM_THIS_WSG,         BM_TIRED_WSG,        BM_WISEMAN_WSG,
};
static const cnfsFileIdx_t earWsgs[] = {
    EA_BIG_HOOP_WSG, EA_BUNNY_WSG,    EA_CAT_WSG,        EA_DOG_WSG,  EA_DOWN_COW_WSG,
    EA_DWARF_WSG,    EA_EARRINGS_WSG, EA_ELF_WSG,        EA_LEFT_WSG, EA_MEDIUM_HOOP_WSG,
    EA_OPEN_COW_WSG, EA_RIGHT_WSG,    EA_SMALL_HOOP_WSG,
};
static const cnfsFileIdx_t eyebrowsWsgs[] = {
    EB_ARCHED_WSG,
    EB_BUSHY_WSG,
    EB_CONCERN_WSG,
    EB_CUT_LEFT_WSG,
    EB_CUT_LEFT_PIERCING_WSG,
    EB_CUT_RIGHT_WSG,
    EB_CUT_RIGHT_PIERCING_WSG,
    EB_DOT_WSG,
    EB_DOWNTURNED_WSG,
    EB_HMM_WSG,
    EB_MISCHIEVOUS_WSG,
    EB_ODD_WSG,
    EB_PUFFY_WSG,
    EB_SLIGHT_CONCERN_WSG,
    EB_THICC_WSG,
    EB_THIN_WSG,
    EB_TINY_WSG,
};
static const cnfsFileIdx_t eyeWsgs[] = {
    E_ANGRY_WSG, E_ANGY_WSG,        E_BABY_WSG,          E_BIG_WSG,           E_BIG_LINER_WSG, E_BOOPED_WSG,
    E_CAT_WSG,   E_CLOSED_WSG,      E_CLOSED_LASHES_WSG, E_CLOSED_LINER_WSG,  E_CRAZY_WSG,     E_CROSSES_WSG,
    E_CUTE_WSG,  E_DOOFY_WSG,       E_EXASPERATED_WSG,   E_HEARTS_WSG,        E_LINER_WSG,     E_MAKEUP_WSG,
    E_SEXY_WSG,  E_SEXY_LASHES_WSG, E_SLEEPING_WSG,      E_SMALL_WLASHES_WSG, E_SQUINTING_WSG, E_SQUINTING_LASHES_WSG,
    E_STARE_WSG, E_STARING_WSG,     E_SWIRLS_WSG,        E_THIN_WSG,          E_WIDE_WSG,
};
static const cnfsFileIdx_t hairWsgs[] = {
    H_BALLET_BUN_WSG,
    H_BOWL_CUT_WSG,
    H_CHIBIUSA_WSG,
    H_CURLY_WSG,
    H_CUTE_WSG,
    H_CUTE_BANGS_WSG,
    H_DOLLY_WSG,
    H_DOWN_DREADS_WSG,
    H_DOWN_DREADS_R_WSG,
    H_FRANKEY_STEIN_WSG,
    H_FRO_WSG,
    H_HINATA_WSG,
    H_JINX_WSG,
    H_LONG_WSG,
    H_LONG_PIGS_WSG,
    H_MAIN_CHARACTER_WSG,
    H_MAIN_CHARACTER_R_WSG,
    H_MAIN_VILLAIN_WSG,
    H_MAIN_VILLAIN_R_WSG,
    H_MALE_PATTERN_WSG,
    H_MCR_WSG,
    H_MINAKO_WSG,
    H_MOHAWK_WSG,
    H_POMPADOUR_WSG,
    H_RAVEN_WSG,
    H_SHORT_WSG,
    H_SHORT_PIGS_WSG,
    H_SIDE_PUFFS_WSG,
    H_SIDE_PUFFS_R_WSG,
    H_SKULL_WSG,
    H_SKULL_R_WSG,
    H_SMALL_BUNS_WSG,
    H_SPOCK_WSG,
    H_STAR_PUFF_NB_WSG,
    H_STAR_PUFFS_WSG,
    H_STAR_PUFFS_R_WSG,
    H_TATTOO_WSG,
    H_THING_WSG,
    H_VBANG_WSG,
    H_USAGI_WSG,
    H_WAVY_HAWK_WSG,
    H_WAVY_HAWK_R_WSG,
    H_WAVY_SHORT_WSG,
    H_WAVY_LONG_WSG,
    H_WEDNESDAY_WSG,
    H_WEDNESDAY_R_WSG,
    H_WET_CURLY_WSG,
    H_WET_SHORT_WSG,
};
static const cnfsFileIdx_t hatWsgs[] = {
    HA_BATTRICE_WSG,   HA_BEANIE_WSG,          HA_BIGMA_WSG,    HA_CHEF_WSG,     HA_COOL_HAT_WSG,
    HA_COWBOY_WSG,     HA_GARBOTNIK_WSG,       HA_GRAD_CAP_WSG, HA_HEART_WSG,    HA_KINETIC_DONUT_WSG,
    HA_MET_HELMET_WSG, HA_PUFFBALL_BEANIE_WSG, HA_PULSE_WSG,    HA_SAWTOOTH_WSG, HA_TRON_WSG,
};
static const cnfsFileIdx_t mouthWsgs[] = {
    M_AH_WSG,
    M_ANGEL_BITE_WSG,
    M_BAR_PIERCING_WSG,
    M_BITE_WSG,
    M_BITE_PIERCING_WSG,
    M_CENSOR_WSG,
    M_CONCERN_WSG,
    M_CONTENT_WSG,
    M_DROOL_WSG,
    M_HALF_SMILE_WSG,
    M_KET_WSG,
    M_KISSES_WSG,
    M_LIP_WSG,
    M_LITTLE_DROOL_WSG,
    M_MISCHIEF_WSG,
    M_MLEM_WSG,
    M_NO_CUPID_BOW_WSG,
    M_OH_WSG,
    M_OH_PIERCING_WSG,
    M_OPEN_SMILE_WSG,
    M_POUTY_WSG,
    M_SAD_WSG,
    M_SAD_PIERCING_WSG,
    M_SATISFIED_WSG,
    M_SMILE_WSG,
    M_STOIC_WSG,
    M_TONGUE_WSG,
    M_TONGUE_PIERCING_WSG,
    M_UHM_WSG,
    M_VAMPIRE_WSG,
    M_YELLING_WSG,
};
static const cnfsFileIdx_t glassesWsgs[] = {
    G_3D_WSG,         G_ANIME_WSG,
    G_BANDAGE_WSG,    G_BIG_WSG,
    G_BIG_ANGLE_WSG,  G_BIG_ANGLE_SUN_WSG,
    G_BIG_SQUARE_WSG, G_BIG_SQUARE_SUN_WSG,
    G_BLACK_SUN_WSG,  G_EGGMAN_WSG,
    G_GOEORDI_WSG,    G_LINDA_WSG,
    G_LINDA_SUN_WSG,  G_LOW_WSG,
    G_LOW_SUN_WSG,    G_PATCH_WSG,
    G_RAY_BAN_WSG,    G_RAY_BAN_SUN_WSG,
    G_READING_WSG,    G_SCOUTER_WSG,
    G_SMALL_WSG,      G_SQUARE_WSG,
    G_SQUARE_SUN_WSG, G_SQUIRTLE_SQUAD_WSG,
    G_THIN_ANGLE_WSG, G_THIN_ANGLE_SUN_WSG,
    G_UPTURNED_WSG,   G_UPTURNED_SUN_WSG,
    G_WIDE_NOSE_WSG,  G_WIDE_NOSE_SUN_WSG,
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
    creatorStates_t cState;  // Sub-state
    wsg_t* tabSprs;          // Image location
    swadgesona_t activeSona; // Drawn to the screen
    int selection;           // Primary selection

    // Slide
    bool out;
    int64_t animTimer;

    // Panel
    int page;
    int subSelection;
    wsg_t* selectionImages;
    int cursorType;
    wsg_t cursorImage;
} swsnCreatorData_t;

//==============================================================================
// Function declarations
//==============================================================================

static void swsnEnterMode(void);
static void swsnExitMode(void);
static void swsnLoop(int64_t elapsedUs);

// Menu
static bool swsnMenuCb(const char* label, bool selected, uint32_t settingVal);

// Creating
static bool slideTab(int selected, bool out, uint64_t elapsedUs);
static void runCreator(buttonEvt_t evt);

// Drawing
static void drawCreator(void);
static bool panelOpen(buttonEvt_t* evt);

// Subdraw
static void drawTab(int xOffset, int y, int scale, bool flip, int labelIdx, bool selected);
static void drawColors(const paletteColor_t* colors, int arrSize, bool left);
static void drawArrows(int arrSize, bool left);
static void drawItems(int arrSize, bool left, bool half);
static void drawTabContents(void);

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
    scd = (swsnCreatorData_t*)heap_caps_calloc(1, sizeof(swsnCreatorData_t), MALLOC_CAP_8BIT);

    // Load
    scd->tabSprs = heap_caps_calloc(ARRAY_SIZE(tabImages), sizeof(wsg_t), MALLOC_CAP_8BIT);
    for (int idx = 0; idx < ARRAY_SIZE(tabImages); idx++)
    {
        loadWsg(tabImages[idx], &scd->tabSprs[idx], true);
    }

    // Menu
    scd->menu     = initMenu(sonaMenuName, swsnMenuCb);
    scd->renderer = initMenuMegaRenderer(NULL, NULL, NULL);
    scd->menu     = startSubMenu(scd->menu, menuOptions[0]);
    for (int16_t idx = 0; idx < MAX_SWSN_SLOTS; idx++)
    {
        // TODO: Load swsn name from NVS when slot is used
        addSingleItemToMenu(scd->menu, sonaSlotUninitialized);
    }
    scd->menu = endSubMenu(scd->menu);
    addSingleItemToMenu(scd->menu, menuOptions[1]);

    // Cursor
    int32_t prevSaved;
    readNvs32(cursorNVS, &prevSaved);
    const settingParam_t cursorImages = {
        .def = prevSaved,
        .key = NULL,
        .min = cursorSprs[0],
        .max = cursorSprs[ARRAY_SIZE(cursorSprs) - 1],
    };
    addSettingsOptionsItemToMenu(scd->menu, menuOptions[2], cursorOptions, cursorSprs, ARRAY_SIZE(cursorSprs),
                                 &cursorImages, prevSaved);

    // Exit
    addSingleItemToMenu(scd->menu, menuOptions[3]);
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
    heap_caps_free(scd);
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
                    // Input
                    runCreator(evt);
                    break;
                }
                case SLIDING:
                {
                    while (checkButtonQueueWrapper(&evt))
                    {
                        if (evt.down)
                        {
                            scd->cState = (scd->out) ? PANEL_OPEN : BODY_PART;
                        }
                    }
                    if (slideTab(scd->selection, scd->out, elapsedUs))
                    {
                        if (evt.down)
                        {
                            scd->cState = (scd->out) ? PANEL_OPEN : BODY_PART;
                        }
                    }
                    break;
                }
                case PANEL_OPEN:
                {
                    panelOpen(&evt);
                    break;
                }
                default:
                {
                    break;
                }
            }
            break;
        }
        default:
        {
            break;
        }
    }
}

// Menu
static bool swsnMenuCb(const char* label, bool selected, uint32_t settingVal)
{
    if (selected)
    {
        if (label == sonaSlotUninitialized)
        {
            // Load the creator
            generateRandomSwadgesona(&scd->activeSona);
            scd->state = CREATING;
            readNvs32(cursorNVS, &scd->cursorType);
            loadWsg(scd->cursorType, &scd->cursorImage, true);
        }
        else if (label == menuOptions[1])
        {
            // Enter the viewer
        }
        else if (label == menuOptions[3])
        {
            // Exit the mode
            switchToSwadgeMode(&mainMenuMode);
        }
    }
    else
    {
        if (label == menuOptions[2])
        {
            // Handle saving cursor
            writeNvs32(cursorNVS, settingVal);
        }
    }
    return false;
}

// Creating
static void runCreator(buttonEvt_t evt)
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
                // FIXME: Automatically set vars to sona
                scd->page         = 0;
                scd->subSelection = 0;
                switch (scd->selection)
                {
                    case HAIR:
                    {
                        scd->selectionImages = heap_caps_calloc(ARRAY_SIZE(hairWsgs), sizeof(wsg_t), MALLOC_CAP_8BIT);
                        for (int i = 0; i < ARRAY_SIZE(hairWsgs); i++)
                        {
                            loadWsg(hairWsgs[i], &scd->selectionImages[i], true);
                        }
                        break;
                    }
                    case EYES:
                    {
                        scd->selectionImages = heap_caps_calloc(ARRAY_SIZE(eyeWsgs), sizeof(wsg_t), MALLOC_CAP_8BIT);
                        for (int i = 0; i < ARRAY_SIZE(eyeWsgs); i++)
                        {
                            loadWsg(eyeWsgs[i], &scd->selectionImages[i], true);
                        }
                        break;
                    }
                    case HAT:
                    {
                        scd->selectionImages = heap_caps_calloc(ARRAY_SIZE(hatWsgs), sizeof(wsg_t), MALLOC_CAP_8BIT);
                        for (int i = 0; i < ARRAY_SIZE(hatWsgs); i++)
                        {
                            loadWsg(hatWsgs[i], &scd->selectionImages[i], true);
                        }
                        break;
                    }
                    case MOUTH:
                    {
                        scd->selectionImages = heap_caps_calloc(ARRAY_SIZE(mouthWsgs), sizeof(wsg_t), MALLOC_CAP_8BIT);
                        for (int i = 0; i < ARRAY_SIZE(mouthWsgs); i++)
                        {
                            loadWsg(mouthWsgs[i], &scd->selectionImages[i], true);
                        }
                        break;
                    }
                    case GLASSES:
                    {
                        scd->selectionImages
                            = heap_caps_calloc(ARRAY_SIZE(glassesWsgs), sizeof(wsg_t), MALLOC_CAP_8BIT);
                        for (int i = 0; i < ARRAY_SIZE(glassesWsgs); i++)
                        {
                            loadWsg(glassesWsgs[i], &scd->selectionImages[i], true);
                        }
                        break;
                    }
                    case BODY_MODS:
                    {
                        scd->selectionImages
                            = heap_caps_calloc(ARRAY_SIZE(bodymarksWsgs), sizeof(wsg_t), MALLOC_CAP_8BIT);
                        for (int i = 0; i < ARRAY_SIZE(bodymarksWsgs); i++)
                        {
                            loadWsg(bodymarksWsgs[i], &scd->selectionImages[i], true);
                        }
                        break;
                    }
                    case EARS:
                    {
                        scd->selectionImages = heap_caps_calloc(ARRAY_SIZE(earWsgs), sizeof(wsg_t), MALLOC_CAP_8BIT);
                        for (int i = 0; i < ARRAY_SIZE(earWsgs); i++)
                        {
                            loadWsg(earWsgs[i], &scd->selectionImages[i], true);
                        }
                        break;
                    }
                    case EYEBROWS:
                    {
                        scd->selectionImages
                            = heap_caps_calloc(ARRAY_SIZE(eyebrowsWsgs), sizeof(wsg_t), MALLOC_CAP_8BIT);
                        for (int i = 0; i < ARRAY_SIZE(eyebrowsWsgs); i++)
                        {
                            loadWsg(eyebrowsWsgs[i], &scd->selectionImages[i], true);
                        }
                        break;
                    }
                    default:
                    {
                        break;
                    }
                }
            }
            else if (evt.button & PB_B)
            {
                freeWsg(&scd->cursorImage);
                scd->state = MENU;
            }
        }
    }
    drawCreator();
}

// Drawing
// Main creator
static void drawCreator(void)
{
    // Background
    clearPxTft();
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c445);

    // Draw the swadgesona face
    drawWsgSimpleScaled(&scd->activeSona.image, (TFT_WIDTH - (scd->activeSona.image.w * SONA_SCALE)) >> 1,
                        TFT_HEIGHT - (scd->activeSona.image.h * SONA_SCALE), SONA_SCALE, SONA_SCALE);

    // Draw the tabs
    for (int idx = 0; idx < NUM_CREATOR_OPTIONS - 1; idx++)
    {
        drawTab(0, 20 + (idx % NUM_TABS) * (scd->tabSprs[0].h + TAB_SPACE) * SONA_SCALE, SONA_SCALE, (idx >= NUM_TABS), idx + 2, scd->selection == idx);
    }
}

// Panel open
static bool panelOpen(buttonEvt_t* evt)
{
    // Draw
    clearPxTft();
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c445);

    // Draw open tab
    bool leftSide = scd->selection < NUM_TABS;
    int x         = (TFT_WIDTH - (scd->activeSona.image.w * SONA_SCALE)) >> 1;
    if (leftSide)
    {
        drawWsgSimpleScaled(&scd->activeSona.image, x + NUM_PIXELS, TFT_HEIGHT - (scd->activeSona.image.h * SONA_SCALE),
                            SONA_SCALE, SONA_SCALE);
        for (int idx = 0; idx < NUM_CREATOR_OPTIONS - 1; idx++)
        {
            if (idx == scd->selection)
            {
                drawTab(NUM_PIXELS * 2, 20 + (idx % NUM_TABS) * (scd->tabSprs[0].h - TAB_SPACE) * SONA_SCALE,
                        SONA_SCALE, (idx >= NUM_TABS), idx + 2, scd->selection == idx);
            }
            else
            {
                drawTab((idx >= NUM_TABS) ? NUM_PIXELS * 2 : 0,
                        20 + (idx % NUM_TABS) * (scd->tabSprs[0].h - TAB_SPACE) * SONA_SCALE, SONA_SCALE,
                        (idx >= NUM_TABS), idx + 2, scd->selection == idx);
            }
        }
        fillDisplayArea(0, Y_PADDING, NUM_PIXELS * 2, TFT_HEIGHT - BOTT_PADDING, c555);
        for (int i = 0; i < 3; i++)
        {
            drawLineFast(0, TFT_HEIGHT - (BOTT_PADDING + 1 + i), NUM_PIXELS * 2 - 1,
                         TFT_HEIGHT - (BOTT_PADDING + 1 + i), c255);
        }
    }
    else
    {
        drawWsgSimpleScaled(&scd->activeSona.image, x - NUM_PIXELS, TFT_HEIGHT - (scd->activeSona.image.h * SONA_SCALE),
                            SONA_SCALE, SONA_SCALE);
        for (int idx = 0; idx < NUM_CREATOR_OPTIONS - 1; idx++)
        {
            if (idx == scd->selection)
            {
                drawTab(-NUM_PIXELS * 2, 20 + (idx % 5) * (scd->tabSprs[0].h - TAB_SPACE) * SONA_SCALE, SONA_SCALE, (idx > 4), idx + 2, scd->selection == idx);
            }
            else
            {
                drawTab((idx < 5) ? -NUM_PIXELS * 2 : 0, 20 + (idx % 5) * (scd->tabSprs[0].h - TAB_SPACE) * SONA_SCALE, SONA_SCALE, (idx > 4), idx + 2,
                        scd->selection == idx);
            }
        }
        fillDisplayArea(TFT_WIDTH - NUM_PIXELS * 2, Y_PADDING, TFT_WIDTH, TFT_HEIGHT - BOTT_PADDING, c555);
        for (int i = 0; i < 3; i++)
        {
            drawLineFast(TFT_WIDTH - NUM_PIXELS * 2, TFT_HEIGHT - (BOTT_PADDING + 1 + i), TFT_WIDTH,
                         TFT_HEIGHT - (BOTT_PADDING + 1 + i), c255);
        }
    }

    drawTabContents();

    int size = -1;
    switch (scd->selection)
    {
        case HAIR:
        {
            size = ARRAY_SIZE(hairWsgs);
            break;
        }
        case EYES:
        {
            size = ARRAY_SIZE(eyeWsgs);
            break;
        }
        case HAT:
        {
            size = ARRAY_SIZE(hatWsgs);
            break;
        }
        case MOUTH:
        {
            size = ARRAY_SIZE(mouthWsgs);
            break;
        }
        case GLASSES:
        {
            size = ARRAY_SIZE(glassesWsgs);
            break;
        }
        case BODY_MODS:
        {
            size = ARRAY_SIZE(bodymarksWsgs);
            break;
        }
        case EARS:
        {
            size = ARRAY_SIZE(earWsgs);
            break;
        }
        case EYEBROWS:
        {
            size = ARRAY_SIZE(eyebrowsWsgs);
            break;
        }
        case CLOTHES:
        {
            size = ARRAY_SIZE(clothesSwatch);
            break;
        }
        case SKIN:
        {
            size = ARRAY_SIZE(skinSwatch);
            break;
        }
        default:
        {
            break;
        }
    }

    // Handle input
    while (checkButtonQueueWrapper(evt))
    {
        if (evt->down)
        {
            if (evt->button & PB_RIGHT)
            {
                if (scd->subSelection % GRID_ROW == GRID_ROW - 1)
                {
                    if (scd->page < (size / GRID_SIZE)
                        && !(scd->page + 1 == (size / GRID_SIZE) && (size % GRID_SIZE) == 0))
                    {
                        scd->page++;
                        scd->subSelection += GRID_SIZE - (GRID_ROW - 1);
                    }
                    if (scd->subSelection > size - 1)
                    {
                        scd->subSelection = size - 1;
                    }
                }
                else if (scd->subSelection < size - 1)
                {
                    scd->subSelection++;
                }
            }
            else if (evt->button & PB_LEFT)
            {
                if (scd->subSelection % GRID_ROW == 0)
                {
                    if (scd->page > 0)
                    {
                        scd->page--;
                        scd->subSelection -= GRID_SIZE - (GRID_ROW - 1);
                    }
                }
                else if (scd->subSelection > 0)
                {
                    scd->subSelection--;
                }
            }
            else if (evt->button & PB_UP)
            {
                if (scd->subSelection >= GRID_ROW + scd->page * GRID_SIZE)
                {
                    scd->subSelection -= GRID_ROW;
                }
            }
            else if (evt->button & PB_DOWN)
            {
                if (scd->subSelection < (scd->page + 1) * GRID_SIZE - GRID_ROW)
                {
                    scd->subSelection += GRID_ROW;
                }
                if (scd->subSelection > size - 1)
                {
                    scd->subSelection = size - 1;
                }
            }
            else if (evt->button & PB_A)
            {
            }
            else if (evt->button & PB_B)
            {
                scd->cState = SLIDING;
                scd->out    = false;
                if (scd->selectionImages != NULL)
                {
                    for (int i = 0; i < size; i++)
                    {
                        freeWsg(&scd->selectionImages[i]);
                    }
                    heap_caps_free(scd->selectionImages);
                    scd->selectionImages = NULL;
                }
            }
        }
    }

    // When to update sona?
    return false;
}

// Sliding
static bool slideTab(int selected, bool out, uint64_t elapsedUs)
{
    // Update variables
    scd->animTimer += elapsedUs;
    // Change direction based selection
    bool left = scd->selection < NUM_TABS;

    // Draw BG
    clearPxTft();
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c445);

    // Draw swadgesona
    int64_t steps = scd->animTimer;
    int x         = (TFT_WIDTH - (scd->activeSona.image.w * SONA_SCALE)) >> 1;
    int offset    = 0;
    if (left)
    {
        if (!out)
        {
            x += NUM_PIXELS;
            offset += NUM_PIXELS * 2;
        }
        while (steps > STEP)
        {
            steps -= STEP;
            x      = (out) ? x + 1 : x - 1;
            offset = (out) ? offset + 2 : offset - 2;
        }
        drawWsgSimpleScaled(&scd->activeSona.image, x, TFT_HEIGHT - (scd->activeSona.image.h * SONA_SCALE), SONA_SCALE,
                            SONA_SCALE);
        for (int idx = 0; idx < NUM_CREATOR_OPTIONS - 1; idx++)
        {
            if (idx == scd->selection)
            {
                drawTab(offset, 20 + (idx % NUM_TABS) * (scd->tabSprs[0].h - TAB_SPACE) * SONA_SCALE, SONA_SCALE, (idx >= NUM_TABS), idx + 2,
                        scd->selection == idx);
            }
            else
            {
                drawTab((idx >= NUM_TABS) ? offset : 0, 20 + (idx % NUM_TABS) * (scd->tabSprs[0].h - TAB_SPACE) * SONA_SCALE, SONA_SCALE, (idx >= NUM_TABS),
                        idx + 2, scd->selection == idx);
            }
        }
        fillDisplayArea(0, Y_PADDING, offset, TFT_HEIGHT - BOTT_PADDING, c555);
        for (int i = 0; i < 3; i++)
        {
            drawLineFast(0, TFT_HEIGHT - (BOTT_PADDING + 1 + i), offset, TFT_HEIGHT - (BOTT_PADDING + 1 + i), c255);
        }
    }
    else
    {
        if (!out)
        {
            x -= NUM_PIXELS;
            offset -= NUM_PIXELS * 2;
        }
        while (steps > STEP)
        {
            steps -= STEP;
            x      = (out) ? x - 1 : x + 1;
            offset = (out) ? offset - 2 : offset + 2;
        }
        drawWsgSimpleScaled(&scd->activeSona.image, x, TFT_HEIGHT - (scd->activeSona.image.h * SONA_SCALE), SONA_SCALE,
                            SONA_SCALE);
        for (int idx = 0; idx < NUM_CREATOR_OPTIONS - 1; idx++)
        {
            if (idx == scd->selection)
            {
                drawTab(offset, 20 + (idx % NUM_TABS) * (scd->tabSprs[0].h - TAB_SPACE) * SONA_SCALE, SONA_SCALE, (idx >= NUM_TABS), idx + 2,
                        scd->selection == idx);
            }
            else
            {
                drawTab((idx < NUM_TABS) ? offset : 0, 20 + (idx % NUM_TABS) * (scd->tabSprs[0].h - TAB_SPACE) * SONA_SCALE, SONA_SCALE, (idx >= NUM_TABS),
                        idx + 2, scd->selection == idx);
            }
        }
        fillDisplayArea(TFT_WIDTH + offset, Y_PADDING, TFT_WIDTH, TFT_HEIGHT - BOTT_PADDING, c555);
        for (int i = 0; i < 3; i++)
        {
            drawLineFast(TFT_WIDTH + offset, TFT_HEIGHT - (BOTT_PADDING + 1 + i), TFT_WIDTH,
                         TFT_HEIGHT - (BOTT_PADDING + 1 + i), c255);
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

// Subdrawing

static void drawTab(int xOffset, int y, int scale, bool flip, int labelIdx, bool selected)
{
    int x = 0;
    if (!selected)
    {
        x -= NUM_TABS * scale;
    }
    wsg_t* tab = &scd->tabSprs[0];
    if (flip)
    {
        tab = &scd->tabSprs[1];
        x   = TFT_WIDTH - scd->tabSprs[1].w * scale;
        if (!selected)
        {
            x += NUM_TABS * scale;
        }
    }
    drawWsgSimpleScaled(tab, xOffset + x, y, scale, scale);
    drawWsgSimpleScaled(&scd->tabSprs[labelIdx], xOffset + x, y, scale, scale);
}

static void drawTabContents(void)
{
    switch (scd->selection)
    {
        case SKIN:
        {
            drawColors(skinSwatch, ARRAY_SIZE(skinSwatch), true);
            break;
        }
        case CLOTHES:
        {
            drawColors(clothesSwatch, ARRAY_SIZE(clothesSwatch), false);
            break;
        }
        case HAIR:
        {
            drawItems(ARRAY_SIZE(hairWsgs), false, true);
            break;
        }
        case EYES:
        {
            drawItems(ARRAY_SIZE(eyeWsgs), true, false);
            break;
        }
        case HAT:
        {
            drawItems(ARRAY_SIZE(hatWsgs), false, true);
            break;
        }
        case MOUTH:
        {
            drawItems(ARRAY_SIZE(mouthWsgs), true, false);
            break;
        }
        case GLASSES:
        {
            drawItems(ARRAY_SIZE(glassesWsgs), false, true);
            break;
        }
        case BODY_MODS:
        {
            drawItems(ARRAY_SIZE(bodymarksWsgs), false, false);
            break;
        }
        case EARS:
        {
            drawItems(ARRAY_SIZE(earWsgs), true, true);
            break;
        }
        case EYEBROWS:
        {
            drawItems(ARRAY_SIZE(eyebrowsWsgs), true, false);
            break;
        }
        default:
        {
            break;
        }
    }
}

static void drawColors(const paletteColor_t* colors, int arrSize, bool left)
{
    int end = (arrSize - (GRID_SIZE * scd->page) < GRID_SIZE) ? arrSize - (GRID_SIZE * scd->page) : GRID_SIZE;
    if (left)
    {
        for (int idx = 0; idx < end; idx++)
        {
            drawRoundedRect(PADDING + ((idx % GRID_ROW) * (PADDING * 2 + SWATCH_W)),
                            Y_PADDING + PADDING + ((idx / GRID_ROW) * (PADDING * 2 + SWATCH_H)),
                            PADDING + ((idx % GRID_ROW) * (PADDING * 2 + SWATCH_W)) + SWATCH_W,
                            Y_PADDING + PADDING + ((idx / GRID_ROW) * (PADDING * 2 + SWATCH_H)) + SWATCH_H, CORNER_RAD,
                            colors[idx + (scd->page * GRID_SIZE)], c000);
            if (scd->subSelection == idx + (scd->page * GRID_SIZE))
            {
                drawWsgSimpleScaled(&scd->cursorImage, PADDING + ((idx % GRID_ROW) * (PADDING * 2 + SWATCH_W)) + 20,
                                    Y_PADDING + PADDING + ((idx / GRID_ROW) * (PADDING * 2 + SWATCH_H)) + 12, 2, 2);
            }
        }
    }
    else
    {
        for (int idx = 0; idx < end; idx++)
        {
            drawRoundedRect((TFT_WIDTH - NUM_PIXELS * 2) + PADDING + ((idx % GRID_ROW) * (PADDING * 2 + SWATCH_W)),
                            Y_PADDING + PADDING + ((idx / GRID_ROW) * (PADDING * 2 + SWATCH_H)),
                            (TFT_WIDTH - NUM_PIXELS * 2) + PADDING + ((idx % GRID_ROW) * (PADDING * 2 + SWATCH_W))
                                + SWATCH_W,
                            Y_PADDING + PADDING + ((idx / GRID_ROW) * (PADDING * 2 + SWATCH_H)) + SWATCH_H, CORNER_RAD,
                            colors[idx + (scd->page * GRID_SIZE)], c000);
            if (scd->subSelection == idx + (scd->page * GRID_SIZE))
            {
                drawWsgSimpleScaled(&scd->cursorImage,
                                    (TFT_WIDTH - NUM_PIXELS * 2) + PADDING
                                        + ((idx % GRID_ROW) * (PADDING * 2 + SWATCH_W)) + 20,
                                    Y_PADDING + PADDING + ((idx / GRID_ROW) * (PADDING * 2 + SWATCH_H)) + 12, 2, 2);
            }
        }
    }

    // Arrows
    drawArrows(arrSize, left);
}

static void drawItems(int arrSize, bool left, bool half)
{
    int end = (arrSize - (GRID_SIZE * scd->page) < GRID_SIZE) ? arrSize - (GRID_SIZE * scd->page) : GRID_SIZE;
    if (left)
    {
        if (half)
        {
            for (int idx = 0; idx < end; idx++)
            {
                drawWsgSimpleHalf(&scd->selectionImages[idx + (scd->page * GRID_SIZE)],
                                  PADDING + ((idx % GRID_ROW) * (PADDING * 2 + SWATCH_W)),
                                  Y_PADDING + PADDING + ((idx / GRID_ROW) * (PADDING * 2 + SWATCH_H)));
                if (scd->subSelection == idx + (scd->page * GRID_SIZE))
                {
                    drawWsgSimpleScaled(&scd->cursorImage, PADDING + ((idx % GRID_ROW) * (PADDING * 2 + SWATCH_W)) + 20,
                                        Y_PADDING + PADDING + ((idx / GRID_ROW) * (PADDING * 2 + SWATCH_H)) + 12, 2, 2);
                }
            }
        }
        else
        {
            for (int idx = 0; idx < end; idx++)
            {
                drawWsgSimple(&scd->selectionImages[idx + (scd->page * GRID_SIZE)],
                              PADDING + ((idx % GRID_ROW) * (PADDING * 2 + SWATCH_W)) - 16,
                              Y_PADDING + PADDING + ((idx / GRID_ROW) * (PADDING * 2 + SWATCH_H)) - 16);
                if (scd->subSelection == idx + (scd->page * GRID_SIZE))
                {
                    drawWsgSimpleScaled(&scd->cursorImage, PADDING + ((idx % GRID_ROW) * (PADDING * 2 + SWATCH_W)) + 20,
                                        Y_PADDING + PADDING + ((idx / GRID_ROW) * (PADDING * 2 + SWATCH_H)) + 12, 2, 2);
                }
            }
        }
    }
    else
    {
        if (half)
        {
            for (int idx = 0; idx < end; idx++)
            {
                drawWsgSimpleHalf(&scd->selectionImages[idx + (scd->page * GRID_SIZE)],
                                  (TFT_WIDTH - NUM_PIXELS * 2) + PADDING
                                      + ((idx % GRID_ROW) * (PADDING * 2 + SWATCH_W)),
                                  Y_PADDING + PADDING + ((idx / GRID_ROW) * (PADDING * 2 + SWATCH_H)));
                if (scd->subSelection == idx + (scd->page * GRID_SIZE))
                {
                    drawWsgSimpleScaled(&scd->cursorImage,
                                        (TFT_WIDTH - NUM_PIXELS * 2) + PADDING
                                            + ((idx % GRID_ROW) * (PADDING * 2 + SWATCH_W)) + 20,
                                        Y_PADDING + PADDING + ((idx / GRID_ROW) * (PADDING * 2 + SWATCH_H)) + 12, 2, 2);
                }
            }
        }
        else
        {
            for (int idx = 0; idx < end; idx++)
            {
                drawWsgSimple(&scd->selectionImages[idx + (scd->page * GRID_SIZE)],
                              (TFT_WIDTH - NUM_PIXELS * 2) + PADDING + ((idx % GRID_ROW) * (PADDING * 2 + SWATCH_W))
                                  - 16,
                              Y_PADDING + PADDING + ((idx / GRID_ROW) * (PADDING * 2 + SWATCH_H)) - 16);
                if (scd->subSelection == idx + (scd->page * GRID_SIZE))
                {
                    drawWsgSimpleScaled(&scd->cursorImage,
                                        (TFT_WIDTH - NUM_PIXELS * 2) + PADDING
                                            + ((idx % GRID_ROW) * (PADDING * 2 + SWATCH_W)) + 20,
                                        Y_PADDING + PADDING + ((idx / GRID_ROW) * (PADDING * 2 + SWATCH_H)) + 12, 2, 2);
                }
            }
        }
    }
    // Arrows
    drawArrows(arrSize, left);
}

static void drawArrows(int arrSize, bool left)
{
    if (arrSize <= GRID_SIZE)
    {
        return;
    }
    if (left)
    {
        if (scd->page * GRID_SIZE < arrSize - GRID_SIZE)
        {
            drawWsgSimpleScaled(&scd->tabSprs[12], NUM_PIXELS + ARROW_SHIFT,
                                TFT_HEIGHT - (BOTT_PADDING + scd->tabSprs[12].h + ARROW_HEIGHT), 2, 2);
        }

        if (scd->page != 0)
        {
            drawWsgSimpleScaled(&scd->tabSprs[13], NUM_PIXELS - (ARROW_SHIFT * 2 + scd->tabSprs[13].w),
                                TFT_HEIGHT - (BOTT_PADDING + scd->tabSprs[13].h + ARROW_HEIGHT), 2, 2);
        }
    }
    else
    {
        if (scd->page * GRID_SIZE < arrSize - GRID_SIZE)
        {
            drawWsgSimpleScaled(&scd->tabSprs[12], TFT_WIDTH - (NUM_PIXELS - ARROW_SHIFT),
                                TFT_HEIGHT - (BOTT_PADDING + scd->tabSprs[12].h + ARROW_HEIGHT), 2, 2);
        }
        if (scd->page != 0)
        {
            drawWsgSimpleScaled(&scd->tabSprs[13], TFT_WIDTH - (NUM_PIXELS + (ARROW_SHIFT * 2 + scd->tabSprs[13].w)),
                                TFT_HEIGHT - (BOTT_PADDING + scd->tabSprs[13].h + ARROW_HEIGHT), 2, 2);
        }
    }
}
