//==============================================================================
// Includes
//==============================================================================

#include "swsnCreator.h"
#include "mainMenu.h"
#include "swadgesona.h"
#include "settingsManager.h"
#include "textEntry.h"

//==============================================================================
// Define
//==============================================================================

#define MAX_SWSN_SLOTS 14
#define MAX_STR_LEN    50

// Creator Settings
#define NUM_TABS_PER_SIDE 5
#define TAB_SPACE         -4
#define SONA_SCALE        3
#define TAB_SCALE         3

// Panels
#define PANEL_WIDTH    150
#define BOTTOM_PADDING 28
#define TOP_PADDING    32
// Grid
#define GRID_ROW  3
#define GRID_COL  4
#define GRID_SIZE (GRID_ROW * GRID_COL)
// Color swatches
#define SWATCH_H   26
#define SWATCH_W   35
#define CORNER_RAD 12
#define PADDING    8
// Arrows
#define ARROW_X     13
#define ARROW_Y     14
#define ARROW_SCALE 2
// Cursor
#define CURSOR_POS_X 20
#define CURSOR_POS_Y 12
#define CURSOR_SCALE 2

// Animations
#define SLIDE_TIME_AMOUNT (1000 * 500)
#define SONA_SLIDE        (PANEL_WIDTH >> 1)
#define STEP              (SLIDE_TIME_AMOUNT / (PANEL_WIDTH >> 1))

// Nickname
#define MAX_NAME_LEN 30
#define NICK_Y_OFF   32

// Save/Quit
#define TEXT_Y_OFF 6
#define EXIT_X_OFF 4

// Warning
#define W_BORDER       16
#define W_HEIGHT       24
#define WB_HEIGHT      48
#define OPTIONS_BORDER 24
#define OPTIONS_H      48

// Saved message
#define SAVE_TIMER 1500000

// Shake exclusion
#define SHAKE_TIMER 500000

//==============================================================================
// Consts
//==============================================================================

const char sonaModeName[]                 = "Swadgesona Creator";
const char sonaTrophyNVS[]                = "SonaTrophyNVS";
static const char sonaMenuName[]          = "Sona Creator";
static const char sonaSlotUninitialized[] = "Uninitialized";
static const char cursorNVS[]             = "cursor";
static const char prompt[]                = "Name this Swadgesona";

// Main
static const char* const menuOptions[] = {
    "Create!", "View", "cursor: ", "Exit", "Edit SwadgePass Sona",
};
static const char* const cursorOptions[] = {
    "Default", "Ball",         "Beaker", "Bow",   "Candy Cane", "Carrot", "White Paw", "Black Paw",  "Computer cursor",
    "Donut",   "Hotdog",       "Ender",  "Gem",   "Ghost",      "Hammer", "Heart",     "Leaf",       " Magic Wand",
    "Glove",   "Corner arrow", "Pencil", "Pizza", "Rainbow",    "Ruler",  "Sword",     "Watermelon", "Weiner",
};
static const char* const creatorText[] = {"Save | Quit", "Save ", " Quit", "SAVED!"};
static const char* const warningText[] = {
    "WARNING!",
    "You have unsaved changes. To save your changes, select 'Enter Name'. To exit without saving, select 'exit'.",
    "Enter name",
    "Exit",
};
static const char* const NVSStrings[] = {
    "sonaCreator",
    "slot-",
    "-nick",
};
static const char* const atriumText[] = {
    "Would you like to go to the atrium mode to edit your SwadgePass profile?",
    "Yes",
    "No",
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
    c544, c545, c543, c432, c321, c210, c255, c444, c243, c545, c334, c422, c555, c312,
};
static const paletteColor_t clothesSwatch[] = {
    c000, c004, c210, c455, c435, c203, c222, c240, c505, c503, c233,
    c554, c102, c521, c544, c545, c305, c401, c543, c033, c555, c541,
};
static const paletteColor_t hairSwatch[] = {
    c333, c542, c531, c520, c300, c210, c000, c555, c535, c413, c435, c203, c144, c013, c353, c131,
};
static const paletteColor_t eyeSwatch[] = {
    c000, c005, c210, c222, c010, c403, c224, c300, c541,
};
static const paletteColor_t hatSwatch[] = {
    c135, c255, c123, c230, c333, c053, c514, c512, c500, c345, c543, c534, c313, c540,
};
static const paletteColor_t glassesSwatch[] = {
    c000,
    c211,
    c410,
};

static const cnfsFileIdx_t bodymarksWsgs[] = {
    SWSN_NO_GO_WSG,     BM_AVATAR_WSG,       BM_BEARD_WSG,      BM_BLUSH_WSG,           BM_BOTTOM_MOLE_WSG,
    BM_BOTTOM_WSG,      BM_CHIN_WSG,         BM_CHIN_PATCH_WSG, BM_CHIN_STRAP_WSG,      BM_CHOKER_WSG,
    BM_COP_WSG,         BM_COWBOY_WSG,       BM_EYE_MOLE_WSG,   BM_FRECKLES_WSG,        BM_FULL_SCRAGGLY_WSG,
    BM_HALF_STACHE_WSG, BM_HEART_STACHE_WSG, BM_LESS_WISE_WSG,  BM_MAGICIAN_WSG,        BM_MARILYN_WSG,
    BM_NECK_BLOOD_WSG,  BM_OLD_WSG,          BM_PILLOW_WSG,     BM_SAND_P_WSG,          BM_SCRAGGLY_WSG,
    BM_SMALL_CURL_WSG,  BM_SMALL_STACHE_WSG, BM_SOUL_PATCH_WSG, BM_SPIKED_NECKLACE_WSG, BM_STACHE_AND_STRAP_WSG,
    BM_STRONGMAN_WSG,   BM_THIN_CHIN_WSG,    BM_THIS_WSG,       BM_TIRED_WSG,           BM_VITILIGO_WSG,
    BM_WISEMAN_WSG,
};
static const cnfsFileIdx_t earWsgs[] = {
    SWSN_NO_GO_WSG,     EA_BIG_HOOP_WSG, EA_BUNNY_WSG,    EA_CAT_WSG,        EA_DOG_WSG,
    EA_DOWN_COW_WSG,    EA_DWARF_WSG,    EA_EARRINGS_WSG, EA_ELF_WSG,        EA_LEFT_WSG,
    EA_MEDIUM_HOOP_WSG, EA_OPEN_COW_WSG, EA_RIGHT_WSG,    EA_SMALL_HOOP_WSG,
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
    E_ANGRY_WSG,       E_ANGY_WSG,          E_BABY_WSG,         E_BIG_WSG,
    E_BIG_LINER_WSG,   E_BLOOD_WSG,         E_BOOPED_WSG,       E_CAT_WSG,
    E_CLOSED_WSG,      E_CLOSED_LASHES_WSG, E_CLOSED_LINER_WSG, E_CRAZY_WSG,
    E_CRYING_WSG,      E_CROSSES_WSG,       E_CUTE_WSG,         E_DOOFY_WSG,
    E_EXASPERATED_WSG, E_HEARTS_WSG,        E_LINER_WSG,        E_MAKEUP_WSG,
    E_MY_EYES_WSG,     E_RANDOMIZER_WSG,    E_SEXY_WSG,         E_SEXY_LASHES_WSG,
    E_SLEEPING_WSG,    E_SMALL_WLASHES_WSG, E_SQUINTING_WSG,    E_SQUINTING_LASHES_WSG,
    E_STARE_WSG,       E_STARING_WSG,       E_SWIRLS_WSG,       E_THIN_WSG,
    E_WIDE_WSG,
};
static const cnfsFileIdx_t hairWsgs[] = {
    SWSN_NO_GO_WSG,      H_BALLET_BUN_WSG,     H_BOWL_CUT_WSG,
    H_CHIBIUSA_WSG,      H_CURLY_WSG,          H_CUTE_WSG,
    H_CUTE_BANGS_WSG,    H_DOLLY_WSG,          H_DOWN_DREADS_WSG,
    H_DOWN_DREADS_R_WSG, H_FRANKEY_STEIN_WSG,  H_FRO_WSG,
    H_HINATA_WSG,        H_JINX_WSG,           H_LONG_WSG,
    H_LONG_PIGS_WSG,     H_MAIN_CHARACTER_WSG, H_MAIN_CHARACTER_R_WSG,
    H_MAIN_VILLAIN_WSG,  H_MAIN_VILLAIN_R_WSG, H_MALE_PATTERN_WSG,
    H_MCR_WSG,           H_MINAKO_WSG,         H_MOHAWK_WSG,
    H_POMPADOUR_WSG,     H_PONYTAIL_WSG,       H_PONYTAIL_NO_BANGS_WSG,
    H_RAVEN_WSG,         H_SHORT_WSG,          H_SHORT_PIGS_WSG,
    H_SIDE_PUFFS_WSG,    H_SIDE_PUFFS_R_WSG,   H_SKULL_WSG,
    H_SKULL_R_WSG,       H_SMALL_BUNS_WSG,     H_SPOCK_WSG,
    H_STAR_PUFF_NB_WSG,  H_STAR_PUFFS_WSG,     H_STAR_PUFFS_R_WSG,
    H_TATTOO_WSG,        H_THING_WSG,          H_VBANG_WSG,
    H_USAGI_WSG,         H_WAVY_HAWK_WSG,      H_WAVY_HAWK_R_WSG,
    H_WAVY_SHORT_WSG,    H_WAVY_LONG_WSG,      H_WEDNESDAY_WSG,
    H_WEDNESDAY_R_WSG,   H_WET_CURLY_WSG,      H_WET_SHORT_WSG,
};
static const cnfsFileIdx_t hatWsgs[] = {
    SWSN_NO_GO_WSG,         HA_ANGEL_WSG,        HA_BATTRICE_WSG, HA_BEANIE_WSG,         HA_BIGMA_WSG,
    HA_BLITZO_WSG,          HA_CHAOS_GOBLIN_WSG, HA_CHEF_WSG,     HA_COOL_HAT_WSG,       HA_COWBOY_WSG,
    HA_DEVIL_WSG,           HA_GARBOTNIK_WSG,    HA_GRAD_CAP_WSG, HA_HEART_WSG,          HA_HOMESTUCK_WSG,
    HA_KINETIC_DONUT_WSG,   HA_MET_HELMET_WSG,   HA_MILLIE_WSG,   HA_MINI_HOMESTUCK_WSG, HA_MOXXIE_WSG,
    HA_PUFFBALL_BEANIE_WSG, HA_PULSE_WSG,        HA_SANS_WSG,     HA_SAWTOOTH_WSG,       HA_TALL_HOMESTUCK_WSG,
    HA_TINY_HOMESTUCK_WSG,  HA_TENNA_WSG,        HA_TRON_WSG,     HA_TV_HEAD_WSG,        HA_VEROSIKA_WSG,
    HA_WIDE_HOMESTUCK_WSG,
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
    SWSN_NO_GO_WSG,
    G_3D_WSG,
    G_ANIME_WSG,
    G_BANDAGE_WSG,
    G_BIG_WSG,
    G_BIG_ANGLE_WSG,
    G_BIG_ANGLE_SUN_WSG,
    G_BIG_SQUARE_WSG,
    G_BIG_SQUARE_SUN_WSG,
    G_BLACK_SUN_WSG,
    G_CUTE_PATCH_WSG,
    G_EGGMAN_WSG,
    G_GOEORDI_WSG,
    G_LINDA_WSG,
    G_LINDA_SUN_WSG,
    G_LOW_WSG,
    G_LOW_SUN_WSG,
    G_PATCH_WSG,
    G_RAY_BAN_WSG,
    G_RAY_BAN_SUN_WSG,
    G_READING_WSG,
    G_SCOUTER_WSG,
    G_SMALL_WSG,
    G_SQUARE_WSG,
    G_SQUARE_SUN_WSG,
    G_SQUIRTLE_SQUAD_WSG,
    G_THIN_ANGLE_WSG,
    G_THIN_ANGLE_SUN_WSG,
    G_UPTURNED_WSG,
    G_UPTURNED_SUN_WSG,
    G_WIDE_NOSE_WSG,
    G_WIDE_NOSE_SUN_WSG,
};

// Text Entry
static const textEntrySettings_t tes = {
    .textPrompt      = prompt,
    .maxLen          = MAX_NAME_LEN,
    .startKMod       = TE_PROPER_NOUN,
    .useMultiLine    = false,
    .useNewCapsStyle = true,
    .useOKEnterStyle = false,
    .blink           = true,
    .textColor       = c555,
    .emphasisColor   = c544,
    .bgColor         = cTransparent,
    .shadowboxColor  = c202, // c202
};

// Trophies
const trophyData_t swsnTrophies[] = {
    {
        .title       = "Mirror Mirror",
        .description = "Create your first swadgesona!",
        .image       = SWSN_MIRROR_WSG,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_EASY,
        .maxVal      = 1,
    },
    {
        .title       = "New face who dis?",
        .description = "Edit your SwadgePass Swadgesona for the first time",
        .image       = SWSN_NEW_FACE_WSG,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_EASY,
        .maxVal      = 1,
    },
    {
        .title       = "Randomizer",
        .description = "Randomize your swadgesona 10 times",
        .image       = E_RANDOMIZER_WSG,
        .type        = TROPHY_TYPE_ADDITIVE,
        .difficulty  = TROPHY_DIFF_MEDIUM,
        .maxVal      = 10,
    },
    {
        .title       = "Chaos Goblin",
        .description = "Randomize and save without making changes",
        .image       = SWSN_CHAOS_GOBLIN_WSG,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_HARD,
        .maxVal      = 1,
        .hidden      = true,
    },
    {
        .title       = "Michelangelo",
        .description = "Fill 5 swadgesona slots with swadgesonas",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_EASY,
        .maxVal      = 1,
    },
    {
        .title       = "Donatello",
        .description = "Fill 10 swadgesona slots with swadgesonas",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_MEDIUM,
        .maxVal      = 1,
    },
    {
        .title       = "Marcel Duchamp",
        .description = "Fill 14 swadgesona slots with swadgesonas",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_HARD,
        .maxVal      = 1,
    },
    {
        .title       = "A Face Made for Radio",
        .description = "Kidding, kidding, It looks great!",
        .image       = E_MY_EYES_WSG,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_EXTREME,
        .maxVal      = 1,
        .hidden      = true,
    },
};

const trophySettings_t swsnTrophySettings = {
    .drawFromBottom   = false,
    .staticDurationUs = DRAW_STATIC_US,
    .slideDurationUs  = DRAW_SLIDE_US,
    .namespaceKey     = sonaTrophyNVS,
};

const trophyDataList_t swsnTrophyDataList = {
    .settings = &swsnTrophySettings,
    .list     = swsnTrophies,
    .length   = ARRAY_SIZE(swsnTrophies),
};

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    MENU,
    CREATING,
    NAMING,
    SP_NAMING,
    PROMPT_SAVE,
    SAVED,
    GOTO_ATRIUM,
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
    // Not tabs
    HAIR_COLOR,
    EYE_COLOR,
    HAT_COLOR,
    GLASSES_COLOR,
    // Exit
    SAVE,
    EXIT
} creatorSelections_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    // Main
    font_t fnt;
    swsnCreatorState_t state;

    // Audio
    midiFile_t bgm;
    midiPlayer_t* bgmPlayer;
    midiFile_t sfxClick;
    midiFile_t sfxMove;
    midiPlayer_t* sfxPlayer;

    // Menu
    menu_t* menu;
    menuMegaRenderer_t* renderer;
    char savedNames[MAX_SWSN_SLOTS][MAX_STR_LEN + 12];

    // Creator
    creatorStates_t cState;  // Sub-state
    wsg_t* tabSprs;          // Image location
    swadgesona_t activeSona; // Drawn to the screen
    int arr[14];
    swadgesona_t liveSona; // Used for editing
    int selection;         // Primary selection

    // Save system
    bool hasChanged; // If the sona has changed
    bool shouldQuit;
    uint8_t slot;
    int64_t savedTimer;

    // Slide
    bool out;
    int64_t animTimer;

    // Panel
    int page;
    wsg_t* selectionImages;
    int32_t cursorType;
    wsg_t cursorImage;

    // Nickname
    char nickname[MAX_NAME_LEN];

    // Shake detection
    vec3d_t lastOrientation;
    list_t shakeHistory;
    bool isShook;
    bool untouchedRandom;
    int shakeRandom;
    int32_t shakeTimer;

    // Trophy Locked items
    bool trophiesUnlocked[ARRAY_SIZE(swsnTrophies)];
    wsg_t noGo;
} swsnCreatorData_t;

//==============================================================================
// Function declarations
//==============================================================================

static void swsnEnterMode(void);
static void swsnExitMode(void);
static void swsnLoop(int64_t elapsedUs);

// Menu
static bool swsnMenuCb(const char* label, bool selected, uint32_t settingVal);
static void swsnResetMenu(void);

// Creator
static void runCreator(buttonEvt_t evt);
static void drawCreator(void);

// Text Entry
static void initTextEntry(void);
static bool promptSaveBeforeQuit(void);
static void drawPrompt(void);
static void drawAtriumPrompt(void);

// Tabs
static void drawTab(int xOffset, int y, bool flip, int labelIdx, bool selected);
static bool slideTab(int selected, bool out, uint64_t elapsedUs);
static void initSlideTabClosed(int size);

// Open Panels
static int panelOpen(void);
static void panelInput(buttonEvt_t evt, int size);
static int drawPanelContents(void);
static void drawColors(const paletteColor_t* colors, int arrSize, bool left);
static void drawArrows(int arrSize, bool left);
static void drawItems(int arrSize, bool left, bool half);

// Swadgesona copy
static void copySonaToList(swadgesona_t* swsn);
static void copyListToSona(swadgesona_t* swsn);

// SP Packet
static void swsnPacket(swadgePassPacket_t* packet);

//==============================================================================
// Variables
//==============================================================================

swadgeMode_t swsnCreatorMode = {
    .modeName                = sonaModeName,
    .wifiMode                = NO_WIFI,
    .usesAccelerometer       = true,
    .fnEnterMode             = swsnEnterMode,
    .fnExitMode              = swsnExitMode,
    .fnMainLoop              = swsnLoop,
    .fnAddToSwadgePassPacket = swsnPacket,
    .trophyData              = &swsnTrophyDataList,
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
    loadFont(RODIN_EB_FONT, &scd->fnt, true);
    loadWsg(SWSN_NO_GO_WSG, &scd->noGo, true);

    // Load audio
    loadMidiFile(SWSN_CREATOR_BGM1_MID, &scd->bgm, true);
    loadMidiFile(SWSN_CHOOSE_SFX_MID, &scd->sfxClick, true);
    loadMidiFile(SWSN_MOVE_SFX_MID, &scd->sfxMove, true);
    scd->bgmPlayer       = globalMidiPlayerGet(MIDI_BGM);
    scd->sfxPlayer       = globalMidiPlayerGet(MIDI_SFX);
    scd->bgmPlayer->loop = true;
    midiGmOn(scd->bgmPlayer);
    midiGmOn(scd->sfxPlayer);
    globalMidiPlayerSetVolume(MIDI_BGM, 12);
    globalMidiPlayerPlaySong(&scd->bgm, MIDI_BGM);

    // Load previous trophy win states into RAM to avoid excessive NVS calls
    // Yes, I know reads aren't limited or destructive, they're just kinda slow. Shush.
    scd->trophiesUnlocked[1] = trophyGetSavedValue(&swsnTrophies[1]); // Booleans are just '0' or '1' in C
    scd->trophiesUnlocked[2] = (trophyGetSavedValue(&swsnTrophies[2]) == 10);
    scd->trophiesUnlocked[3] = trophyGetSavedValue(&swsnTrophies[3]);
    scd->trophiesUnlocked[7] = trophyGetSavedValue(&swsnTrophies[7]);

    // If the SP swadgesona isn't saved yet, automatically load into creating the SP Sona
    size_t len = sizeof(swadgesonaCore_t);
    if (!readNvsBlob(spSonaNVSKey, &scd->activeSona.core, &len))
    {
        generateRandomSwadgesona(&scd->activeSona);
        scd->activeSona.name      = *getSystemUsername();
        scd->activeSona.name.user = true;
        strcpy(scd->nickname, scd->activeSona.name.nameBuffer);
        scd->slot       = 255; // Indicates it's the SP Sona
        scd->state      = CREATING;
        scd->shakeTimer = SHAKE_TIMER;
    }

    // Load the cursor, using the default if it doesn't exist yet
    if (!readNvs32(cursorNVS, &scd->cursorType))
    {
        writeNvs32(cursorNVS, cursorSprs[0]);
        scd->cursorType = cursorSprs[0];
    }
    loadWsg(scd->cursorType, &scd->cursorImage, true);

    // Load trophy data
    scd->shakeRandom = trophyGetSavedValue(&swsnTrophies[2]);

    swsnResetMenu();
}

static void swsnExitMode(void)
{
    globalMidiPlayerStop(true);
    unloadMidiFile(&scd->bgm);
    unloadMidiFile(&scd->sfxClick);
    unloadMidiFile(&scd->sfxMove);
    deinitMenuMegaRenderer(scd->renderer);
    deinitMenu(scd->menu);
    freeWsg(&scd->noGo);
    freeFont(&scd->fnt);
    for (int idx = 0; idx < ARRAY_SIZE(tabImages); idx++)
    {
        freeWsg(&scd->tabSprs[idx]);
    }
    heap_caps_free(scd->tabSprs);
    heap_caps_free(scd);
}

static void swsnLoop(int64_t elapsedUs)
{
    bool shaketh    = checkForShake(&scd->lastOrientation, &scd->shakeHistory, &scd->isShook);
    buttonEvt_t evt = {0};
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
        case CREATING:
        {
            switch (scd->cState)
            {
                case BODY_PART:
                {
                    // Input
                    while (checkButtonQueueWrapper(&evt))
                    {
                        runCreator(evt);
                    }
                    if (scd->shakeTimer >= 0)
                    {
                        scd->shakeTimer -= elapsedUs;
                    }
                    if (shaketh)
                    {
                        if (!scd->isShook && !(scd->shakeTimer >= 0))
                        {
                            scd->shakeTimer = SHAKE_TIMER;
                            scd->shakeRandom++;
                            // Stopped shaking, time to randomize
                            generateRandomSwadgesona(&scd->activeSona);
                            // Add a count to the randomized total
                            trophyUpdate(&swsnTrophies[2], scd->shakeRandom, true);
                            // Set bool for unedited randomized sona
                            scd->untouchedRandom = true;
                            if (scd->shakeRandom >= 10)
                            {
                                scd->trophiesUnlocked[2] = true;
                            }
                        }
                    }
                    drawCreator();
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
                    int size = panelOpen();
                    // Handle input after drawing because input can free resources
                    while (checkButtonQueueWrapper(&evt))
                    {
                        panelInput(evt, size);
                    }
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
            bool done = false;
            while (checkButtonQueueWrapper(&evt))
            {
                if (evt.button & PB_B && evt.down)
                {
                    scd->state = CREATING;
                }
                done = textEntryInput(evt);
                if (done && strcmp(scd->nickname, "") == 0)
                {
                    done = false;
                    textEntryInit(&tes, scd->nickname, getSysFont());
                }
            }
            if (done)
            {
                globalMidiPlayerPlaySong(&scd->sfxClick, MIDI_SFX);
                char buffer[16];
                snprintf(buffer, sizeof(buffer) - 1, "%s%" PRId16, NVSStrings[1], scd->slot);
                writeNamespaceNvsBlob(NVSStrings[0], buffer, &scd->activeSona.core, sizeof(swadgesonaCore_t));
                snprintf(buffer, sizeof(buffer) - 1, "%s%d%s", NVSStrings[1], scd->slot, NVSStrings[2]);
                writeNamespaceNvsBlob(NVSStrings[0], buffer, &scd->nickname, MAX_NAME_LEN);
                textEntryDeinit();
                scd->state      = SAVED;
                scd->hasChanged = false;
                trophyUpdate(&swsnTrophies[0], 1, true);

                // Calculate number of slots used
                int accum = 0;
                for (int idx = 0; idx < MAX_SWSN_SLOTS; idx++)
                {
                    snprintf(buffer, sizeof(buffer) - 1, "%s%" PRId16, NVSStrings[1], idx);
                    size_t len = sizeof(swadgesonaCore_t);
                    if (readNamespaceNvsBlob(NVSStrings[0], buffer, &scd->activeSona.core, &len))
                    {
                        accum++;
                    }
                }
                if (accum >= MAX_SWSN_SLOTS)
                {
                    trophyUpdate(&swsnTrophies[6], 1, true);
                }
                else if (accum >= 10)
                {
                    trophyUpdate(&swsnTrophies[5], 1, true);
                }
                else if (accum >= 5)
                {
                    trophyUpdate(&swsnTrophies[4], 1, true);
                }
                if (scd->untouchedRandom)
                {
                    trophyUpdate(&swsnTrophies[3], 1, true);
                    scd->trophiesUnlocked[3] = true;
                }
                if (esp_random() % 100 == 0)
                {
                    trophyUpdate(&swsnTrophies[7], 1, true);
                    scd->trophiesUnlocked[7] = true;
                }
                break;
            }
            drawCreator();
            textEntryDraw(elapsedUs);
            break;
        }
        case SP_NAMING:
        {
            bool done = false;
            while (checkButtonQueueWrapper(&evt))
            {
                done = handleUsernamePickerInput(&evt, &scd->activeSona.name);
            }
            if (done)
            {
                globalMidiPlayerPlaySong(&scd->sfxClick, MIDI_SFX);
                strcpy(scd->nickname, scd->activeSona.name.nameBuffer);
                setSystemUsername(&scd->activeSona.name);
                writeNvsBlob(spSonaNVSKey, &scd->activeSona.core, sizeof(swadgesonaCore_t));
                scd->state      = GOTO_ATRIUM;
                scd->hasChanged = false;
                trophyUpdate(&swsnTrophies[1], 1, true);
                scd->trophiesUnlocked[1] = true;
                break;
            }
            drawUsernamePicker(&scd->activeSona.name);
            break;
        }
        case PROMPT_SAVE:
        {
            while (checkButtonQueueWrapper(&evt))
            {
                if (evt.down)
                {
                    if (evt.button & PB_LEFT)
                    {
                        scd->shouldQuit = false;
                    }
                    else if (evt.button & PB_RIGHT)
                    {
                        scd->shouldQuit = true;
                    }
                    else if (evt.button & PB_A)
                    {
                        if (scd->shouldQuit)
                        {
                            freeWsg(&scd->cursorImage);
                            scd->state     = MENU;
                            scd->selection = 0;
                        }
                        else
                        {
                            initTextEntry();
                            scd->shouldQuit = true; // Automatically close when done
                        }
                    }
                    else if (evt.button & PB_B)
                    {
                        scd->state = CREATING;
                    }
                }
            }

            // Draw
            drawPrompt();
            break;
        }
        case SAVED:
        {
            scd->savedTimer += elapsedUs;
            while (checkButtonQueueWrapper(&evt))
            {
                if (evt.down)
                {
                    scd->state      = (scd->shouldQuit) ? MENU : CREATING;
                    scd->shouldQuit = false;
                    scd->savedTimer = 0;
                }
            }
            if (scd->savedTimer > SAVE_TIMER)
            {
                scd->state      = (scd->shouldQuit) ? MENU : CREATING;
                scd->shouldQuit = false;
                scd->savedTimer = 0;
            }
            clearPxTft();
            drawText(&scd->fnt, c555, creatorText[3], (TFT_WIDTH - textWidth(&scd->fnt, creatorText[3])) >> 1,
                     (TFT_HEIGHT - scd->fnt.height) >> 1);
            break;
        }
        case GOTO_ATRIUM:
        {
            while (checkButtonQueueWrapper(&evt))
            {
                if (evt.down)
                {
                    if (evt.button & PB_LEFT)
                    {
                        scd->shouldQuit = false;
                    }
                    else if (evt.button & PB_RIGHT)
                    {
                        scd->shouldQuit = true;
                    }
                    else if (evt.button & PB_A)
                    {
                        if (!scd->shouldQuit)
                        {
                            freeWsg(&scd->cursorImage);
                            // switchToSwadgeMode(&); TODO Atrium
                        }
                        else
                        {
                            scd->state = CREATING;
                        }
                    }
                    else if (evt.button & PB_B)
                    {
                        scd->state = CREATING;
                    }
                }
            }
            drawAtriumPrompt();
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
        if (label == menuOptions[1])
        {
            // Enter the viewer
            // TODO: Link to viewer mode.
        }
        else if (label == menuOptions[3])
        {
            // Exit the mode
            switchToSwadgeMode(&mainMenuMode);
        }
        else if (label == menuOptions[4])
        {
            // Set swadgepass swadgesona
            // Pull data from NVS
            size_t len = sizeof(swadgesonaCore_t);
            if (!readNvsBlob(spSonaNVSKey, &scd->activeSona.core, &len))
            {
                generateRandomSwadgesona(&scd->activeSona);
            }
            else
            {
                generateSwadgesonaImage(&scd->activeSona, true);
            }

            // Move to creator mode
            scd->activeSona.name = *getSystemUsername();
            strcpy(scd->nickname, scd->activeSona.name.nameBuffer);

            scd->slot  = 255; // Indicates it's the SP Sona
            scd->state = CREATING;
            readNvs32(cursorNVS, &scd->cursorType);
            loadWsg(scd->cursorType, &scd->cursorImage, true);
        }
        else if (label != menuOptions[2] && label != menuOptions[0])
        {
            for (int idx = 0; idx < MAX_SWSN_SLOTS; idx++)
            {
                char buffer[32];
                snprintf(buffer, sizeof(buffer) - 1, "Slot %d: %s", idx + 1, sonaSlotUninitialized);
                if (strncmp(buffer, label, 17) == 0)
                {
                    generateRandomSwadgesona(&scd->activeSona);
                    scd->slot  = idx;
                    scd->state = CREATING;
                    strcpy(scd->nickname, "");
                    readNvs32(cursorNVS, &scd->cursorType);
                    loadWsg(scd->cursorType, &scd->cursorImage, true);
                }
                snprintf(buffer, sizeof(buffer) - 1, "Slot %d: ", idx + 1);
                if (strncmp(buffer, label, 8) == 0)
                {
                    // Pull data from NVS
                    size_t len = sizeof(swadgesonaCore_t);
                    char nvsBuffer[16];
                    snprintf(nvsBuffer, sizeof(nvsBuffer) - 1, "%s%" PRId16, NVSStrings[1], idx);
                    readNamespaceNvsBlob(NVSStrings[0], nvsBuffer, &scd->activeSona.core, &len);
                    snprintf(nvsBuffer, sizeof(nvsBuffer) - 1, "%s%d%s", NVSStrings[1], idx, NVSStrings[2]);
                    readNamespaceNvsBlob(NVSStrings[0], nvsBuffer, NULL, &len);
                    readNamespaceNvsBlob(NVSStrings[0], nvsBuffer, scd->nickname, &len);
                    generateSwadgesonaImage(&scd->activeSona, true);

                    // Move to creator mode
                    scd->slot  = idx;
                    scd->state = CREATING;
                    readNvs32(cursorNVS, &scd->cursorType);
                    loadWsg(scd->cursorType, &scd->cursorImage, true);
                }
            }
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

static void swsnResetMenu()
{
    // Menu
    scd->menu     = initMenu(sonaMenuName, swsnMenuCb);
    scd->renderer = initMenuMegaRenderer(NULL, NULL, NULL);
    scd->menu     = startSubMenu(scd->menu, menuOptions[0]);
    for (int8_t idx = 0; idx < MAX_SWSN_SLOTS; idx++)
    {
        char nvsBuffer[32];
        size_t len;
        snprintf(nvsBuffer, sizeof(nvsBuffer) - 1, "%s%d%s", NVSStrings[1], idx, NVSStrings[2]);
        readNamespaceNvsBlob(NVSStrings[0], nvsBuffer, NULL, &len);
        char buff[MAX_STR_LEN];
        if (!readNamespaceNvsBlob(NVSStrings[0], nvsBuffer, buff, &len))
        {
            snprintf(scd->savedNames[idx], sizeof(scd->savedNames[idx]) - 1, "Slot %d: %s", idx + 1,
                     sonaSlotUninitialized);
            addSingleItemToMenu(scd->menu, scd->savedNames[idx]);
        }
        else
        {
            snprintf(scd->savedNames[idx], sizeof(scd->savedNames[idx]) - 1, "Slot %d: %s", idx + 1, buff);
            addSingleItemToMenu(scd->menu, scd->savedNames[idx]);
        }
    }
    scd->menu = endSubMenu(scd->menu);
    addSingleItemToMenu(scd->menu, menuOptions[4]);
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

// Creator
static void runCreator(buttonEvt_t evt)
{
    if (evt.down)
    {
        globalMidiPlayerPlaySong(&scd->sfxMove, MIDI_SFX);
        if (scd->selection >= (NUM_TABS_PER_SIDE * 2))
        {
            if (evt.button & PB_DOWN)
            {
                if (scd->selection == EXIT)
                {
                    scd->selection = NUM_TABS_PER_SIDE;
                }
                else if (scd->selection == SAVE)
                {
                    scd->selection = 0;
                }
            }
            else if (evt.button & PB_RIGHT)
            {
                scd->selection = EXIT;
            }
            else if (evt.button & PB_LEFT)
            {
                scd->selection = SAVE;
            }
        }
        else
        {
            if (evt.button & PB_UP)
            {
                scd->selection--;
                if (scd->selection < 0)
                {
                    scd->selection = SAVE;
                }
                else if (scd->selection == NUM_TABS_PER_SIDE - 1)
                {
                    scd->selection = EXIT;
                }
            }
            else if (evt.button & PB_DOWN)
            {
                if ((scd->selection + 1) % NUM_TABS_PER_SIDE != 0)
                {
                    scd->selection++;
                }
            }
            else if (evt.button & PB_LEFT && scd->selection >= NUM_TABS_PER_SIDE
                     && scd->selection < (NUM_TABS_PER_SIDE * 2))
            {
                scd->selection -= NUM_TABS_PER_SIDE;
            }
            else if (evt.button & PB_RIGHT && scd->selection < NUM_TABS_PER_SIDE)
            {
                scd->selection += NUM_TABS_PER_SIDE;
            }
        }
        if (evt.button & PB_A)
        {
            if (scd->selection == EXIT)
            {
                // Handle exit
                if (!promptSaveBeforeQuit())
                {
                    freeWsg(&scd->cursorImage);
                    scd->state     = MENU;
                    scd->selection = 0;
                    swsnResetMenu();
                    return;
                }
                return;
            }
            else if (scd->selection == SAVE)
            {
                initTextEntry();
                return;
            }
            copySwadgesona(&scd->liveSona, &scd->activeSona);
            copySonaToList(&scd->activeSona);
            scd->out                 = true;
            scd->cState              = SLIDING;
            scd->arr[scd->selection] = 0;
            switch (scd->selection)
            {
                case HAIR:
                {
                    scd->selectionImages = heap_caps_calloc(ARRAY_SIZE(hairWsgs), sizeof(wsg_t), MALLOC_CAP_8BIT);
                    for (int i = 0; i < ARRAY_SIZE(hairWsgs); i++)
                    {
                        loadWsg(hairWsgs[i], &scd->selectionImages[i], true);
                    }
                    scd->arr[scd->selection] = scd->activeSona.core.hairStyle;
                    break;
                }
                case EYES:
                {
                    scd->selectionImages = heap_caps_calloc(ARRAY_SIZE(eyeWsgs), sizeof(wsg_t), MALLOC_CAP_8BIT);
                    for (int i = 0; i < ARRAY_SIZE(eyeWsgs); i++)
                    {
                        loadWsg(eyeWsgs[i], &scd->selectionImages[i], true);
                    }
                    scd->arr[scd->selection] = scd->activeSona.core.eyeShape;
                    break;
                }
                case HAT:
                {
                    scd->selectionImages = heap_caps_calloc(ARRAY_SIZE(hatWsgs), sizeof(wsg_t), MALLOC_CAP_8BIT);
                    for (int i = 0; i < ARRAY_SIZE(hatWsgs); i++)
                    {
                        loadWsg(hatWsgs[i], &scd->selectionImages[i], true);
                    }
                    scd->arr[scd->selection] = scd->activeSona.core.hat;
                    break;
                }
                case MOUTH:
                {
                    scd->selectionImages = heap_caps_calloc(ARRAY_SIZE(mouthWsgs), sizeof(wsg_t), MALLOC_CAP_8BIT);
                    for (int i = 0; i < ARRAY_SIZE(mouthWsgs); i++)
                    {
                        loadWsg(mouthWsgs[i], &scd->selectionImages[i], true);
                    }
                    scd->arr[scd->selection] = scd->activeSona.core.mouthShape;
                    break;
                }
                case GLASSES:
                {
                    scd->selectionImages = heap_caps_calloc(ARRAY_SIZE(glassesWsgs), sizeof(wsg_t), MALLOC_CAP_8BIT);
                    for (int i = 0; i < ARRAY_SIZE(glassesWsgs); i++)
                    {
                        loadWsg(glassesWsgs[i], &scd->selectionImages[i], true);
                    }
                    scd->arr[scd->selection] = scd->activeSona.core.glasses;
                    break;
                }
                case BODY_MODS:
                {
                    scd->selectionImages = heap_caps_calloc(ARRAY_SIZE(bodymarksWsgs), sizeof(wsg_t), MALLOC_CAP_8BIT);
                    for (int i = 0; i < ARRAY_SIZE(bodymarksWsgs); i++)
                    {
                        loadWsg(bodymarksWsgs[i], &scd->selectionImages[i], true);
                    }
                    scd->arr[scd->selection] = scd->activeSona.core.bodyMarks;
                    break;
                }
                case EARS:
                {
                    scd->selectionImages = heap_caps_calloc(ARRAY_SIZE(earWsgs), sizeof(wsg_t), MALLOC_CAP_8BIT);
                    for (int i = 0; i < ARRAY_SIZE(earWsgs); i++)
                    {
                        loadWsg(earWsgs[i], &scd->selectionImages[i], true);
                    }
                    scd->arr[scd->selection] = scd->activeSona.core.earShape;
                    break;
                }
                case EYEBROWS:
                {
                    scd->selectionImages = heap_caps_calloc(ARRAY_SIZE(eyebrowsWsgs), sizeof(wsg_t), MALLOC_CAP_8BIT);
                    for (int i = 0; i < ARRAY_SIZE(eyebrowsWsgs); i++)
                    {
                        loadWsg(eyebrowsWsgs[i], &scd->selectionImages[i], true);
                    }
                    scd->arr[scd->selection] = scd->activeSona.core.eyebrows;
                    break;
                }
                case SKIN:
                {
                    scd->arr[scd->selection] = scd->activeSona.core.skin;
                    break;
                }
                case CLOTHES:
                {
                    scd->arr[scd->selection] = scd->activeSona.core.clothes;
                    break;
                }
                default:
                {
                    break;
                }
            }
            scd->page = scd->arr[scd->selection] / GRID_SIZE;
        }
        else if (evt.button & PB_B)
        {
            if (!promptSaveBeforeQuit())
            {
                freeWsg(&scd->cursorImage);
                scd->state     = MENU;
                scd->selection = 0;
                swsnResetMenu();
            }
            return;
        }
    }
}

static void drawCreator(void)
{
    // Background
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c445);

    // Draw the swadgesona face
    drawWsgSimpleScaled(&scd->activeSona.image, (TFT_WIDTH - (scd->activeSona.image.w * SONA_SCALE)) >> 1,
                        TFT_HEIGHT - (scd->activeSona.image.h * SONA_SCALE), SONA_SCALE, SONA_SCALE);

    // Draw the tabs
    for (int idx = 0; idx < (NUM_TABS_PER_SIDE * 2); idx++)
    {
        drawTab(0, 20 + (idx % NUM_TABS_PER_SIDE) * (scd->tabSprs[0].h + TAB_SPACE) * TAB_SCALE,
                (idx >= NUM_TABS_PER_SIDE), idx + 2, scd->selection == idx);
    }

    // Draw nickname
    if (scd->state != NAMING)
    {
        drawText(getSysFont(), c000, scd->nickname, (TFT_WIDTH - textWidth(getSysFont(), scd->nickname)) >> 1,
                 NICK_Y_OFF);
    }

    // Draw save / quit options
    drawText(&scd->fnt, c000, creatorText[0], (TFT_WIDTH - textWidth(&scd->fnt, creatorText[0])) >> 1, TEXT_Y_OFF);
    if (scd->selection == SAVE)
    {
        drawText(&scd->fnt, c142, creatorText[1], (TFT_WIDTH >> 1) - textWidth(&scd->fnt, creatorText[1]), TEXT_Y_OFF);
    }
    else if (scd->selection == EXIT)
    {
        drawText(&scd->fnt, c500, creatorText[2], (TFT_WIDTH >> 1) + EXIT_X_OFF, TEXT_Y_OFF);
    }
}

// Text entry / prompt save
static void initTextEntry(void)
{
    if (scd->slot == 255)
    {
        scd->state = SP_NAMING;
    }
    else
    {
        scd->state = NAMING;
        textEntryInit(&tes, scd->nickname, getSysFont());
    }
}

static bool promptSaveBeforeQuit()
{
    if (scd->hasChanged)
    {
        scd->shouldQuit = false;
        scd->state      = PROMPT_SAVE;
        return true;
    }
    return false;
}

static void drawPrompt()
{
    clearPxTft();
    drawText(&scd->fnt, c511, warningText[0], (TFT_WIDTH - textWidth(&scd->fnt, warningText[0])) >> 1, W_HEIGHT);
    int16_t x = W_BORDER;
    int16_t y = WB_HEIGHT;
    drawTextWordWrapCentered(&scd->fnt, c555, warningText[1], &x, &y, TFT_WIDTH - W_BORDER, TFT_HEIGHT - OPTIONS_H);
    if (scd->shouldQuit)
    {
        drawText(&scd->fnt, c333, warningText[2], OPTIONS_BORDER, TFT_HEIGHT - OPTIONS_H);
        drawText(&scd->fnt, c550, warningText[3], (TFT_WIDTH - (textWidth(&scd->fnt, warningText[3]) + OPTIONS_BORDER)),
                 TFT_HEIGHT - OPTIONS_H);
    }
    else
    {
        drawText(&scd->fnt, c550, warningText[2], OPTIONS_BORDER, TFT_HEIGHT - OPTIONS_H);
        drawText(&scd->fnt, c333, warningText[3], (TFT_WIDTH - (textWidth(&scd->fnt, warningText[3]) + OPTIONS_BORDER)),
                 TFT_HEIGHT - OPTIONS_H);
    }
}

static void drawAtriumPrompt()
{
    clearPxTft();
    int16_t x = W_BORDER;
    int16_t y = WB_HEIGHT;
    drawTextWordWrapCentered(&scd->fnt, c555, atriumText[0], &x, &y, TFT_WIDTH - W_BORDER, TFT_HEIGHT - OPTIONS_H);
    if (scd->shouldQuit)
    {
        drawText(&scd->fnt, c333, atriumText[1], OPTIONS_BORDER, TFT_HEIGHT - OPTIONS_H);
        drawText(&scd->fnt, c550, atriumText[2], (TFT_WIDTH - (textWidth(&scd->fnt, atriumText[2]) + OPTIONS_BORDER)),
                 TFT_HEIGHT - OPTIONS_H);
    }
    else
    {
        drawText(&scd->fnt, c550, atriumText[1], OPTIONS_BORDER, TFT_HEIGHT - OPTIONS_H);
        drawText(&scd->fnt, c333, atriumText[2], (TFT_WIDTH - (textWidth(&scd->fnt, atriumText[2]) + OPTIONS_BORDER)),
                 TFT_HEIGHT - OPTIONS_H);
    }
}

// Tabs
static void drawTab(int xOffset, int y, bool flip, int labelIdx, bool selected)
{
    int x = 0;
    if (!selected)
    {
        x -= NUM_TABS_PER_SIDE * TAB_SCALE;
    }
    wsg_t* tab = &scd->tabSprs[0];
    if (flip)
    {
        tab = &scd->tabSprs[1];
        x   = TFT_WIDTH - scd->tabSprs[1].w * TAB_SCALE;
        if (!selected)
        {
            x += NUM_TABS_PER_SIDE * TAB_SCALE;
        }
    }
    drawWsgSimpleScaled(tab, xOffset + x, y, TAB_SCALE, TAB_SCALE);
    drawWsgSimpleScaled(&scd->tabSprs[labelIdx], xOffset + x, y, TAB_SCALE, TAB_SCALE);
}

static bool slideTab(int selected, bool out, uint64_t elapsedUs)
{
    // Update variables
    scd->animTimer += elapsedUs;
    // Change direction based selection
    bool left = scd->selection < NUM_TABS_PER_SIDE;

    // Draw BG
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c445);

    // Draw swadgesona
    int64_t steps = scd->animTimer;
    int x         = (TFT_WIDTH - (scd->activeSona.image.w * SONA_SCALE)) >> 1;
    int offset    = 0;
    if (left || scd->selection == EYE_COLOR)
    {
        if (!out)
        {
            x += SONA_SLIDE;
            offset += PANEL_WIDTH;
        }
        while (steps > STEP)
        {
            steps -= STEP;
            x      = (out) ? x + 1 : x - 1;
            offset = (out) ? offset + 2 : offset - 2;
        }
        drawWsgSimpleScaled(&scd->activeSona.image, x, TFT_HEIGHT - (scd->activeSona.image.h * SONA_SCALE), SONA_SCALE,
                            SONA_SCALE);
        for (int idx = 0; idx < (NUM_TABS_PER_SIDE * 2); idx++)
        {
            if (idx == scd->selection)
            {
                drawTab(offset, 20 + (idx % NUM_TABS_PER_SIDE) * (scd->tabSprs[0].h + TAB_SPACE) * TAB_SCALE,
                        (idx >= NUM_TABS_PER_SIDE), idx + 2, scd->selection == idx);
            }
            else
            {
                drawTab((idx >= NUM_TABS_PER_SIDE) ? offset : 0,
                        20 + (idx % NUM_TABS_PER_SIDE) * (scd->tabSprs[0].h + TAB_SPACE) * TAB_SCALE,
                        (idx >= NUM_TABS_PER_SIDE), idx + 2, scd->selection == idx);
            }
        }
        fillDisplayArea(0, TOP_PADDING, offset, TFT_HEIGHT - BOTTOM_PADDING, c555);
        for (int i = 0; i < 3; i++)
        {
            drawLineFast(0, TFT_HEIGHT - (BOTTOM_PADDING + 1 + i), offset, TFT_HEIGHT - (BOTTOM_PADDING + 1 + i), c255);
        }
    }
    else
    {
        if (!out)
        {
            x -= SONA_SLIDE;
            offset -= PANEL_WIDTH;
        }
        while (steps > STEP)
        {
            steps -= STEP;
            x      = (out) ? x - 1 : x + 1;
            offset = (out) ? offset - 2 : offset + 2;
        }
        drawWsgSimpleScaled(&scd->activeSona.image, x, TFT_HEIGHT - (scd->activeSona.image.h * SONA_SCALE), SONA_SCALE,
                            SONA_SCALE);
        for (int idx = 0; idx < (NUM_TABS_PER_SIDE * 2); idx++)
        {
            if (idx == scd->selection)
            {
                drawTab(offset, 20 + (idx % NUM_TABS_PER_SIDE) * (scd->tabSprs[0].h + TAB_SPACE) * TAB_SCALE,
                        (idx >= NUM_TABS_PER_SIDE), idx + 2, scd->selection == idx);
            }
            else
            {
                drawTab((idx < NUM_TABS_PER_SIDE) ? offset : 0,
                        20 + (idx % NUM_TABS_PER_SIDE) * (scd->tabSprs[0].h + TAB_SPACE) * TAB_SCALE,
                        (idx >= NUM_TABS_PER_SIDE), idx + 2, scd->selection == idx);
            }
        }
        fillDisplayArea(TFT_WIDTH + offset, TOP_PADDING, TFT_WIDTH, TFT_HEIGHT - BOTTOM_PADDING, c555);
        for (int i = 0; i < 3; i++)
        {
            drawLineFast(TFT_WIDTH + offset, TFT_HEIGHT - (BOTTOM_PADDING + 1 + i), TFT_WIDTH,
                         TFT_HEIGHT - (BOTTOM_PADDING + 1 + i), c255);
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

static void initSlideTabClosed(int size)
{
    copyListToSona(&scd->activeSona);
    scd->hasChanged = true;
    scd->cState     = SLIDING;
    scd->out        = false;
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

// Open Panels
static int panelOpen()
{
    // Draw
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c445);

    // Draw open tab
    bool leftSide = scd->selection < NUM_TABS_PER_SIDE;
    int x         = (TFT_WIDTH - (scd->activeSona.image.w * SONA_SCALE)) >> 1;
    if (leftSide || scd->selection == EYE_COLOR)
    {
        drawWsgSimpleScaled(&scd->liveSona.image, x + (PANEL_WIDTH >> 1),
                            TFT_HEIGHT - (scd->activeSona.image.h * SONA_SCALE), SONA_SCALE, SONA_SCALE);
        for (int idx = 0; idx < (NUM_TABS_PER_SIDE * 2); idx++)
        {
            if (idx == scd->selection)
            {
                drawTab(PANEL_WIDTH, 20 + (idx % NUM_TABS_PER_SIDE) * (scd->tabSprs[0].h + TAB_SPACE) * SONA_SCALE,
                        (idx >= NUM_TABS_PER_SIDE), idx + 2, scd->selection == idx);
            }
            else if (scd->selection == EYE_COLOR)
            {
                drawTab(PANEL_WIDTH, 20 + (EYES % NUM_TABS_PER_SIDE) * (scd->tabSprs[0].h + TAB_SPACE) * SONA_SCALE,
                        (idx >= NUM_TABS_PER_SIDE), EYES + 2, true);
            }
            else
            {
                drawTab((idx >= NUM_TABS_PER_SIDE) ? PANEL_WIDTH : 0,
                        20 + (idx % NUM_TABS_PER_SIDE) * (scd->tabSprs[0].h + TAB_SPACE) * SONA_SCALE,
                        (idx >= NUM_TABS_PER_SIDE), idx + 2, scd->selection == idx);
            }
        }
        fillDisplayArea(0, TOP_PADDING, PANEL_WIDTH, TFT_HEIGHT - BOTTOM_PADDING, c555);
        for (int i = 0; i < 3; i++)
        {
            drawLineFast(0, TFT_HEIGHT - (BOTTOM_PADDING + 1 + i), PANEL_WIDTH - 1,
                         TFT_HEIGHT - (BOTTOM_PADDING + 1 + i), c255);
        }
    }
    else
    {
        drawWsgSimpleScaled(&scd->liveSona.image, x - (PANEL_WIDTH >> 1),
                            TFT_HEIGHT - (scd->activeSona.image.h * SONA_SCALE), SONA_SCALE, SONA_SCALE);
        for (int idx = 0; idx < (NUM_TABS_PER_SIDE * 2); idx++)
        {
            if (idx == scd->selection)
            {
                drawTab(-PANEL_WIDTH, 20 + (idx % NUM_TABS_PER_SIDE) * (scd->tabSprs[0].h + TAB_SPACE) * TAB_SCALE,
                        (idx >= NUM_TABS_PER_SIDE), idx + 2, scd->selection == idx);
            }
            else if (scd->selection == HAIR_COLOR)
            {
                drawTab(-PANEL_WIDTH, 20 + (HAIR % NUM_TABS_PER_SIDE) * (scd->tabSprs[0].h + TAB_SPACE) * TAB_SCALE,
                        (idx >= NUM_TABS_PER_SIDE), HAIR + 2, true);
            }
            else if (scd->selection == HAT_COLOR)
            {
                drawTab(-PANEL_WIDTH, 20 + (HAT % NUM_TABS_PER_SIDE) * (scd->tabSprs[0].h + TAB_SPACE) * TAB_SCALE,
                        (idx >= NUM_TABS_PER_SIDE), HAT + 2, true);
            }
            else if (scd->selection == GLASSES_COLOR)
            {
                drawTab(-PANEL_WIDTH, 20 + (GLASSES % NUM_TABS_PER_SIDE) * (scd->tabSprs[0].h + TAB_SPACE) * TAB_SCALE,
                        (idx >= NUM_TABS_PER_SIDE), GLASSES + 2, true);
            }
            else
            {
                drawTab((idx < 5) ? -PANEL_WIDTH : 0,
                        20 + (idx % NUM_TABS_PER_SIDE) * (scd->tabSprs[0].h + TAB_SPACE) * TAB_SCALE,
                        (idx >= NUM_TABS_PER_SIDE), idx + 2, scd->selection == idx);
            }
        }
        fillDisplayArea(TFT_WIDTH - PANEL_WIDTH, TOP_PADDING, TFT_WIDTH, TFT_HEIGHT - BOTTOM_PADDING, c555);
        for (int i = 0; i < 3; i++)
        {
            drawLineFast(TFT_WIDTH - PANEL_WIDTH, TFT_HEIGHT - (BOTTOM_PADDING + 1 + i), TFT_WIDTH,
                         TFT_HEIGHT - (BOTTOM_PADDING + 1 + i), c255);
        }
    }

    // When to update sona?
    return drawPanelContents();
}

static void panelInput(buttonEvt_t evt, int size)
{
    if (evt.down)
    {
        globalMidiPlayerPlaySong(&scd->sfxMove, MIDI_SFX);
        if (evt.button & PB_RIGHT)
        {
            if (scd->arr[scd->selection] % GRID_ROW == GRID_ROW - 1)
            {
                if (scd->page < (size / GRID_SIZE) && !(scd->page + 1 == (size / GRID_SIZE) && (size % GRID_SIZE) == 0))
                {
                    scd->page++;
                    scd->arr[scd->selection] += GRID_SIZE - (GRID_ROW - 1);
                }
                if (scd->arr[scd->selection] > size - 1)
                {
                    scd->arr[scd->selection] = size - 1;
                }
            }
            else if (scd->arr[scd->selection] < size - 1)
            {
                scd->arr[scd->selection]++;
            }
        }
        else if (evt.button & PB_LEFT)
        {
            if (scd->arr[scd->selection] % GRID_ROW == 0)
            {
                if (scd->page > 0)
                {
                    scd->page--;
                    scd->arr[scd->selection] -= GRID_SIZE - (GRID_ROW - 1);
                }
            }
            else if (scd->arr[scd->selection] > 0)
            {
                scd->arr[scd->selection]--;
            }
        }
        else if (evt.button & PB_UP)
        {
            if (scd->arr[scd->selection] >= GRID_ROW + scd->page * GRID_SIZE)
            {
                scd->arr[scd->selection] -= GRID_ROW;
            }
        }
        else if (evt.button & PB_DOWN)
        {
            if (scd->arr[scd->selection] < (scd->page + 1) * GRID_SIZE - GRID_ROW)
            {
                scd->arr[scd->selection] += GRID_ROW;
            }
            if (scd->arr[scd->selection] > size - 1)
            {
                scd->arr[scd->selection] = size - 1;
            }
        }
        else if (evt.button & PB_A)
        {
            switch (scd->selection)
            {
                case HAIR:
                {
                    scd->selection = HAIR_COLOR;
                    scd->page      = scd->arr[scd->selection] / GRID_SIZE;
                    break;
                }
                case EYES:
                {
                    if ((scd->arr[EYES] == EE_RANDOMIZER && !scd->trophiesUnlocked[2])
                        || (scd->arr[EYES] == EE_MY_EYES && !scd->trophiesUnlocked[7]))
                    {
                        break; // Don't save this one
                    }
                    scd->selection = EYE_COLOR;
                    scd->page      = scd->arr[scd->selection] / GRID_SIZE;
                    break;
                }
                case HAT:
                {
                    if (scd->arr[HAT] == HAE_CHAOS_GOBLIN && !scd->trophiesUnlocked[3])
                    {
                        break; // Don't save this one
                    }
                    if (scd->arr[scd->selection] == HAE_BEANIE || scd->arr[scd->selection] == HAE_COOL_HAT
                        || scd->arr[scd->selection] == HAE_PUFFBALL || scd->arr[scd->selection] == HAE_HEART)
                    {
                        scd->selection = HAT_COLOR;
                        scd->page      = scd->arr[scd->selection] / GRID_SIZE;
                        break;
                    }
                    initSlideTabClosed(size);
                    break;
                }
                case GLASSES:
                {
                    if (scd->arr[scd->selection] == G_BIG || scd->arr[scd->selection] == G_BIGANGLE
                        || scd->arr[scd->selection] == G_BIGSQUARE || scd->arr[scd->selection] == G_LINDA
                        || scd->arr[scd->selection] == G_LOW || scd->arr[scd->selection] == G_RAYBAN
                        || scd->arr[scd->selection] == G_READING || scd->arr[scd->selection] == G_SMALL
                        || scd->arr[scd->selection] == G_SQUARE || scd->arr[scd->selection] == G_THINANGLE
                        || scd->arr[scd->selection] == G_UPTURNED || scd->arr[scd->selection] == G_WIDENOSE)
                    {
                        scd->selection = GLASSES_COLOR;
                        scd->page      = scd->arr[scd->selection] / GRID_SIZE;
                        break;
                    }
                    initSlideTabClosed(size);
                    break;
                }
                default:
                {
                    if (scd->selection == SKIN && scd->arr[SKIN] == 13 && !scd->trophiesUnlocked[1])
                    {
                        break; // Don't select this one
                    }
                    switch (scd->selection)
                    {
                        case HAIR_COLOR:
                        {
                            scd->selection = HAIR;
                            break;
                        }
                        case EYE_COLOR:
                        {
                            scd->selection = EYES;
                            break;
                        }
                        case HAT_COLOR:
                        {
                            scd->selection = HAT;
                            break;
                        }
                        case GLASSES_COLOR:
                        {
                            scd->selection = GLASSES;
                            break;
                        }
                        default:
                        {
                            break;
                        }
                    }
                    // If no color needs to be selected
                    initSlideTabClosed(size);
                    break;
                }
            }
            scd->untouchedRandom = false;
        }
        else if (evt.button & PB_B)
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
        // Copy selection into live
        copyListToSona(&scd->liveSona);
    }
}

static int drawPanelContents(void)
{
    int size = -1;
    switch (scd->selection)
    {
        case SKIN:
        {
            size = ARRAY_SIZE(skinSwatch);
            drawColors(skinSwatch, size, true);
            break;
        }
        case CLOTHES:
        {
            size = ARRAY_SIZE(clothesSwatch);
            drawColors(clothesSwatch, size, false);
            break;
        }
        case HAIR:
        {
            size = ARRAY_SIZE(hairWsgs);
            drawItems(size, false, true);
            break;
        }
        case HAIR_COLOR:
        {
            size = ARRAY_SIZE(hairSwatch);
            drawColors(hairSwatch, size, false);
            break;
        }
        case EYES:
        {
            size = ARRAY_SIZE(eyeWsgs);
            drawItems(size, true, false);
            break;
        }
        case EYE_COLOR:
        {
            size = ARRAY_SIZE(eyeSwatch);
            drawColors(eyeSwatch, size, true);
            break;
        }
        case HAT:
        {
            size = ARRAY_SIZE(hatWsgs);
            drawItems(size, false, true);
            break;
        }
        case HAT_COLOR:
        {
            size = ARRAY_SIZE(hatSwatch);
            drawColors(hatSwatch, size, false);
            break;
        }
        case MOUTH:
        {
            size = ARRAY_SIZE(mouthWsgs);
            drawItems(size, true, false);
            break;
        }
        case GLASSES:
        {
            size = ARRAY_SIZE(glassesWsgs);
            drawItems(size, false, true);
            break;
        }
        case GLASSES_COLOR:
        {
            size = ARRAY_SIZE(glassesSwatch);
            drawColors(glassesSwatch, size, false);
            break;
        }
        case BODY_MODS:
        {
            size = ARRAY_SIZE(bodymarksWsgs);
            drawItems(size, false, false);
            break;
        }
        case EARS:
        {
            size = ARRAY_SIZE(earWsgs);
            drawItems(size, true, true);
            break;
        }
        case EYEBROWS:
        {
            size = ARRAY_SIZE(eyebrowsWsgs);
            drawItems(size, true, false);
            break;
        }
        default:
        {
            break;
        }
    }
    return size;
}

static void drawColors(const paletteColor_t* colors, int arrSize, bool left)
{
    int end    = (arrSize - (GRID_SIZE * scd->page) < GRID_SIZE) ? arrSize - (GRID_SIZE * scd->page) : GRID_SIZE;
    int xStart = PADDING;
    if (!left)
    {
        xStart += (TFT_WIDTH - PANEL_WIDTH);
    }
    for (int idx = 0; idx < end; idx++)
    {
        drawRoundedRect(xStart + ((idx % GRID_ROW) * (PADDING * 2 + SWATCH_W)),
                        TOP_PADDING + PADDING + ((idx / GRID_ROW) * (PADDING * 2 + SWATCH_H)),
                        xStart + ((idx % GRID_ROW) * (PADDING * 2 + SWATCH_W)) + SWATCH_W,
                        TOP_PADDING + PADDING + ((idx / GRID_ROW) * (PADDING * 2 + SWATCH_H)) + SWATCH_H, CORNER_RAD,
                        colors[idx + (scd->page * GRID_SIZE)], c000);
        if (scd->selection == SKIN && idx + (scd->page * GRID_SIZE) == SKIN_MAUVE && !scd->trophiesUnlocked[1])
        {
            drawWsgSimple(&scd->noGo, xStart + 1 + ((idx % GRID_ROW) * (PADDING * 2 + SWATCH_W)),
                          TOP_PADDING + PADDING - 2 + ((idx / GRID_ROW) * (PADDING * 2 + SWATCH_H)));
        }
        if (scd->arr[scd->selection] == idx + (scd->page * GRID_SIZE))
        {
            drawWsgSimpleScaled(&scd->cursorImage,
                                xStart + ((idx % GRID_ROW) * (PADDING * 2 + SWATCH_W)) + CURSOR_POS_X,
                                TOP_PADDING + PADDING + ((idx / GRID_ROW) * (PADDING * 2 + SWATCH_H)) + CURSOR_POS_Y,
                                CURSOR_SCALE, CURSOR_SCALE);
        }
    }
    // Arrows
    drawArrows(arrSize, left);
}

static void drawItems(int arrSize, bool left, bool half)
{
    // Arrows
    drawArrows(arrSize, left);

    // Images
    int end = (arrSize - (GRID_SIZE * scd->page) < GRID_SIZE) ? arrSize - (GRID_SIZE * scd->page) : GRID_SIZE;
    void (*drawFunc)(const wsg_t* wsg, int16_t xOff, int16_t yOff) = drawWsgSimpleHalf;
    int yOffset                                                    = (half) ? TOP_PADDING + PADDING : TOP_PADDING;
    int xOffset       = (left) ? PADDING : PADDING + (TFT_WIDTH - PANEL_WIDTH);
    int cursorXOffset = xOffset;
    if (!half)
    {
        drawFunc = drawWsgSimple;
        xOffset -= (scd->activeSona.image.w >> 2);
        yOffset -= (scd->activeSona.image.h >> 2);
    }
    for (int idx = 0; idx < end; idx++)
    {
        // If None selected option
        if (scd->page * GRID_SIZE + idx == 0
            && (scd->selection == EARS || scd->selection == BODY_MODS || scd->selection == HAT
                || scd->selection == GLASSES || scd->selection == HAIR))
        {
            drawWsgSimple(&scd->selectionImages[idx + (scd->page * GRID_SIZE)],
                          cursorXOffset + ((idx % GRID_ROW) * (PADDING * 2 + SWATCH_W)),
                          TOP_PADDING + PADDING + ((idx / GRID_ROW) * (PADDING * 2 + SWATCH_H)));
        }
        else
        {
            drawFunc(&scd->selectionImages[idx + (scd->page * GRID_SIZE)],
                     xOffset + ((idx % GRID_ROW) * (PADDING * 2 + SWATCH_W)),
                     yOffset + ((idx / GRID_ROW) * (PADDING * 2 + SWATCH_H)));
        }
        if ((scd->selection == HAT && idx + (scd->page * GRID_SIZE) == HAE_CHAOS_GOBLIN && !scd->trophiesUnlocked[3])
            || (scd->selection == EYES && idx + (scd->page * GRID_SIZE) == EE_MY_EYES && !scd->trophiesUnlocked[7])
            || (scd->selection == EYES && idx + (scd->page * GRID_SIZE) == EE_RANDOMIZER && !scd->trophiesUnlocked[2]))
        {
            drawWsgSimple(&scd->noGo, cursorXOffset + ((idx % GRID_ROW) * (PADDING * 2 + SWATCH_W)),
                          TOP_PADDING + PADDING + ((idx / GRID_ROW) * (PADDING * 2 + SWATCH_H)));
        }
        // Cursor
        if (scd->arr[scd->selection] == idx + (scd->page * GRID_SIZE))
        {
            drawWsgSimpleScaled(&scd->cursorImage,
                                cursorXOffset + ((idx % GRID_ROW) * (PADDING * 2 + SWATCH_W)) + CURSOR_POS_X,
                                TOP_PADDING + PADDING + ((idx / GRID_ROW) * (PADDING * 2 + SWATCH_H)) + CURSOR_POS_Y,
                                CURSOR_SCALE, CURSOR_SCALE);
        }
    }
}

static void drawArrows(int arrSize, bool left)
{
    if (arrSize <= GRID_SIZE)
    {
        return;
    }
    int xOff = (left) ? (PANEL_WIDTH >> 1) + ARROW_X : (TFT_WIDTH - (PANEL_WIDTH >> 1)) + ARROW_X;
    // Draw arrow to the right if not the last page
    if (scd->page * GRID_SIZE < arrSize - GRID_SIZE)
    {
        drawWsgSimpleScaled(&scd->tabSprs[12], xOff, TFT_HEIGHT - (BOTTOM_PADDING + scd->tabSprs[12].h + ARROW_Y),
                            ARROW_SCALE, ARROW_SCALE);
    }
    // Draw arrow to the left if not the first page
    if (scd->page != 0)
    {
        drawWsgSimpleScaled(&scd->tabSprs[13], xOff - ((ARROW_X * 2) + (scd->tabSprs[13].w * ARROW_SCALE)),
                            TFT_HEIGHT - (BOTTOM_PADDING + scd->tabSprs[13].h + ARROW_Y), ARROW_SCALE, ARROW_SCALE);
    }
}

// Swadgesona copy
static void copySonaToList(swadgesona_t* swsn)
{
    scd->arr[SKIN]          = swsn->core.skin;
    scd->arr[HAIR_COLOR]    = swsn->core.hairColor;
    scd->arr[EYE_COLOR]     = swsn->core.eyeColor;
    scd->arr[CLOTHES]       = swsn->core.clothes;
    scd->arr[HAT_COLOR]     = swsn->core.hatColor;
    scd->arr[GLASSES_COLOR] = swsn->core.glassesColor;
    scd->arr[BODY_MODS]     = swsn->core.bodyMarks;
    scd->arr[EARS]          = swsn->core.earShape;
    scd->arr[EYEBROWS]      = swsn->core.eyebrows;
    scd->arr[EYES]          = swsn->core.eyeShape;
    scd->arr[HAIR]          = swsn->core.hairStyle;
    scd->arr[HAT]           = swsn->core.hat;
    scd->arr[MOUTH]         = swsn->core.mouthShape;
    scd->arr[GLASSES]       = swsn->core.glasses;
}

static void copyListToSona(swadgesona_t* swsn)
{
    swsn->core.skin         = scd->arr[SKIN];
    swsn->core.hairColor    = scd->arr[HAIR_COLOR];
    swsn->core.eyeColor     = scd->arr[EYE_COLOR];
    swsn->core.clothes      = scd->arr[CLOTHES];
    swsn->core.hatColor     = scd->arr[HAT_COLOR];
    swsn->core.glassesColor = scd->arr[GLASSES_COLOR];
    swsn->core.bodyMarks    = scd->arr[BODY_MODS];
    swsn->core.eyebrows     = scd->arr[EYEBROWS];
    swsn->core.earShape     = scd->arr[EARS];
    swsn->core.eyeShape     = scd->arr[EYES];
    swsn->core.hairStyle    = scd->arr[HAIR];
    swsn->core.hat          = scd->arr[HAT];
    swsn->core.mouthShape   = scd->arr[MOUTH];
    swsn->core.glasses      = scd->arr[GLASSES];
    generateSwadgesonaImage(swsn, true);
}

// SP Packet
static void swsnPacket(swadgePassPacket_t* packet)
{
    size_t len = sizeof(swadgesonaCore_t);
    readNvsBlob(spSonaNVSKey, &packet->swadgesona.core, &len);

    nameData_t nd                      = *getSystemUsername();
    packet->swadgesona.core.packedName = GET_PACKED_USERNAME(nd);
}
