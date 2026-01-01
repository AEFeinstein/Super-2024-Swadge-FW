//==============================================================================
// Functions
//==============================================================================

#include <stdlib.h>
#include <esp_log.h>

#include "swadge2024.h"

#include "picross_menu.h"
#include "mode_picross.h"
#include "picross_select.h"
#include "picross_tutorial.h"
#include "picross_consts.h"

#include "mainMenu.h"

//==============================================================================
// Enums & Structs
//==============================================================================

typedef enum
{
    PICROSS_MENU,
    PICROSS_LEVELSELECT,
    PICROSS_GAME,
    PICROSS_TUTORIAL
} picrossScreen_t;

typedef struct
{
    font_t mmFont;
    menu_t* menu;
    menuMegaRenderer_t* renderer;
    picrossLevelDef_t levels[PICROSS_LEVEL_COUNT];
    picrossScreen_t screen;
    int32_t savedIndex;
    int32_t options; // bit 0: hints
    midiFile_t bgm;
} picrossMenu_t;

//==============================================================================
// Function Prototypes
//==============================================================================

void picrossEnterMode(void);
void picrossExitMode(void);
void picrossMainLoop(int64_t elapsedUs);
void picrossButtonCb(buttonEvt_t* evt);
void picrossTouchCb(bool touched);
void loadLevels(void);
bool picrossMainMenuCb(const char* label, bool selected, uint32_t value);
void picrossMenuOptionsCb(const char* opt);

//==============================================================================
// Variables
//==============================================================================

// Key values for persistent save data.
const char picrossCurrentPuzzleIndexKey[] = "pic_cur_ind";
const char picrossSavedOptionsKey[]       = "pic_opts";
const char picrossCompletedLevelData[]    = "pic_victs"; // todo: rename to Key suffix
const char picrossProgressData[]          = "pic_prog";  // todo: rename to key suffix
const char picrossMarksData[]             = "pic_marks";
const char picrossHoverLevelIndexKey[]    = "pic_hov_ind";
const char picrossTopVisibleRowKey[]      = "pic_scroll_ind";

// Main menu strings
static char str_picrossTitle[]      = "Pi-cross 2"; // \x7f is interpreted as the pi char
static const char str_continue[]    = "Continue";
static const char str_levelSelect[] = "Puzzle Select";
static const char str_howtoplay[]   = "How To Play";
static const char str_exit[]        = "Exit";

// Options Menu
static const char str_options[] = "Options";

static const char str_On[]       = "On";
static const char str_Off[]      = "Off";
static const char* strs_on_off[] = {str_Off, str_On};

static const char str_X[]         = "X";
static const char str_Solid[]     = "Solid";
static const char* strs_x_solid[] = {str_X, str_Solid};

static const char str_Hints[]        = "Mistake Alert: ";
static const char str_Guides[]       = "Guides: ";
static const char str_Mark[]         = "Empty Marks: ";
static const char str_AnimateBG[]    = "BG: ";
static const char* str_Backgrounds[] = {"hexagons", "dots", "none"};

static const int32_t backgroundVals[] = {PICROSS_BG_HEXAGONS, PICROSS_BG_DOTS, PICROSS_BG_NONE};

static const paletteColor_t bgColors[] = {c122, c132, c133, c144};

static const int32_t trueFalseVals[] = {
    false,
    true,
};

static const char str_eraseProgress[] = "Reset Current";

struct
{
    const char* name;
    cnfsFileIdx_t pzl;
    cnfsFileIdx_t slv;
    const char* marqueeFact;
} picrossPuzzles[PICROSS_LEVEL_COUNT] = {
    {
        .name        = "arrow",
        .pzl         = _55_ARROW_PZL_WSG,
        .slv         = _55_ARROW_SLV_WSG,
        .marqueeFact = "You solved it!",
    },
    {
        .name        = "check",
        .pzl         = _55_CHECK_PZL_WSG,
        .slv         = _55_CHECK_SLV_WSG,
        .marqueeFact = "A new feature in Picross 2 is transparent blocks in some solutions.",
    },
    {
        .name        = "crown",
        .pzl         = _55_CROWN_PZL_WSG,
        .slv         = _55_CROWN_SLV_WSG,
        .marqueeFact = "Perhaps this belongs to that donut character?",
    },
    {
        .name        = "flower",
        .pzl         = _55_FLOWER_PZL_WSG,
        .slv         = _55_FLOWER_SLV_WSG,
        .marqueeFact = "All but one of these puzzles are created by @objectdiscret.itch.io",
    },
    {
        .name        = "ghost",
        .pzl         = _55_GHOST_PZL_WSG,
        .slv         = _55_GHOST_SLV_WSG,
        .marqueeFact = "We shall name her Maggy.",
    },
    {
        .name        = "heart",
        .pzl         = _55_HEART_PZL_WSG,
        .slv         = _55_HEART_SLV_WSG,
        .marqueeFact = "Love you for making it this far!",
    },
    {
        .name        = "kitty",
        .pzl         = _55_KITTY_PZL_WSG,
        .slv         = _55_KITTY_SLV_WSG,
        .marqueeFact = "Every year at magfest, staff's pet photos will appear on the jumbotron.",
    },
    {
        .name        = "pacman",
        .pzl         = _55_PACMAN_PZL_WSG,
        .slv         = _55_PACMAN_SLV_WSG,
        .marqueeFact = "wakka wakka wakka",
    },
    {
        .name        = "rainbow",
        .pzl         = _55_RAINBOW_PZL_WSG,
        .slv         = _55_RAINBOW_SLV_WSG,
        .marqueeFact = "MAGPIE nest, new in 2025, hosts several events to contextualize marginalized groups at "
                       "MAGFest. Not just for LGBTQIA+ communities, but also people of color, parents, disabled folks, "
                       "women, neurodivergence, and more.",
    },
    {
        .name        = "triangle",
        .pzl         = _55_TRIANGLE_PZL_WSG,
        .slv         = _55_TRIANGLE_SLV_WSG,
        .marqueeFact = "Congratulations! You found the triangle!",
    },
    {
        .name        = "mag",
        .pzl         = _6F_MAG_1_PZL_WSG,
        .slv         = _6F_MAG_1_SLV_WSG,
        .marqueeFact = "Woooooooo! It's Music And Gaming!!!",
    },
    {
        .name        = "fest",
        .pzl         = _6F_MAG_2_PZL_WSG,
        .slv         = _6F_MAG_2_SLV_WSG,
        .marqueeFact = "24,500 people attended the festival in 2025.",
    },
    {
        .name        = "2026",
        .pzl         = _6F_MAG_3_PZL_WSG,
        .slv         = _6F_MAG_3_SLV_WSG,
        .marqueeFact = "2026 features Alpha Pulse who must take down evil Bigma to save the future of music!",
    },
    {
        .name        = "mag logo",
        .pzl         = _6F_MAGLOGO_PZL_WSG,
        .slv         = _6F_MAGLOGO_SLV_WSG,
        .marqueeFact = "The MAGFest logo features a joystick, a square wave, and a speaker.",
    },
    {
        .name        = "music",
        .pzl         = _6F_MUSIC_PZL_WSG,
        .slv         = _6F_MUSIC_SLV_WSG,
        .marqueeFact = "The 2025 swadge upgraded the buzzers to a speaker and midi player. In 2026, dylwhich added "
                       "support for the megaman soundfont!",
    },
    {
        .name        = "flyin donut",
        .pzl         = _8F_FLYINDONUT_PZL_WSG,
        .slv         = _8F_FLYINDONUT_SLV_WSG,
        .marqueeFact = "Flight Sims from the 2022 chainsaw swadge and 2024 gunship swadge : Featured integration to "
                       "VRchat by cnlohr inside a 3d Gaylord modeled by gplord.",
    },
    {
        .name = "m-type",
        .pzl  = _8F_MTYPE_PZL_WSG,
        .slv  = _8F_MTYPE_SLV_WSG,
        .marqueeFact
        = "Produced by jt-moriarty for the 2022 chainsaw swadge : Blast your way through an endless army of evil space "
          "aliens with your experimental M-Type fighter!",
    },
    {
        .name        = "personal demon",
        .pzl         = _8F_PERSONALDEMON_PZL_WSG,
        .slv         = _8F_PERSONALDEMON_SLV_WSG,
        .marqueeFact = "Produced by gelakinetic for the 2022 chainsaw swadge : We all have our own Personal "
                       "Demons, and now you can take care of yours!",
    },
    {
        .name = "shredder",
        .pzl  = _8F_SHREDDER_PZL_WSG,
        .slv  = _8F_SHREDDER_SLV_WSG,
        .marqueeFact
        = "Produced by gelakinetic for the 2022 chainsaw swadge : Hate'th Notes have taken over the arenas, and it's "
          "up to you "
          "to "
          "Shred them to pieces! Wield your righteous guitar and get them before they get you with their scythes!",
    },
    {
        .name = "stomp",
        .pzl  = _8F_STOMP_PZL_WSG,
        .slv  = _8F_STOMP_SLV_WSG,
        .marqueeFact
        = "Produced by Ruzihm for the 2022 chainsaw swadge : The flying skulls are coming to get you and the only way "
          "to stop them is to Stomp them into the ground!",
    },
    {
        .name        = "apple",
        .pzl         = AA_APPLE_PZL_WSG,
        .slv         = AA_APPLE_SLV_WSG,
        .marqueeFact = "Quick snack break from all the swadge history.",
    },
    {
        .name        = "baba",
        .pzl         = AA_BABA_PZL_WSG,
        .slv         = AA_BABA_SLV_WSG,
        .marqueeFact = "BABA is NONOGRAM. NONOGRAM is WIN.",
    },
    {
        .name        = "dragon ball",
        .pzl         = AA_DRAGONBALL_PZL_WSG,
        .slv         = AA_DRAGONBALL_SLV_WSG,
        .marqueeFact = "If your head is empty, you can pack it with dreams!",
    },
    {
        .name        = "heart piece",
        .pzl         = AA_HEARTPIECE_PZL_WSG,
        .slv         = AA_HEARTPIECE_SLV_WSG,
        .marqueeFact = "badabedem badabedem badabedem badabedem badabedem badabedem badabedem badabedem ...... DUN "
                       "DADADADA DUUUUUHNNNN!",
    },
    {
        .name        = "metroid",
        .pzl         = AA_METROID_PZL_WSG,
        .slv         = AA_METROID_SLV_WSG,
        .marqueeFact = "jellyfish is helping yes. Y can't metroid crawl?",
    },
    {
        .name        = "moon stick",
        .pzl         = AA_MOONSTICK_PZL_WSG,
        .slv         = AA_MOONSTICK_SLV_WSG,
        .marqueeFact = "Moon Healing Escalation!",
    },
    {
        .name        = "peach",
        .pzl         = AA_PEACH_PZL_WSG,
        .slv         = AA_PEACH_SLV_WSG,
        .marqueeFact = "More snack break pls, to fuel puzzle solving.",
    },
    {
        .name        = "pocket ball",
        .pzl         = AA_POKEBALL_PZL_WSG,
        .slv         = AA_POKEBALL_SLV_WSG,
        .marqueeFact = "Redacted",
    },
    {
        .name        = "sword",
        .pzl         = AA_SWORD_PZL_WSG,
        .slv         = AA_SWORD_SLV_WSG,
        .marqueeFact = "Fitting for the 2025 MAGStock theme.",
    },
    {
        .name        = "toadstool",
        .pzl         = AA_TOADSTOOL_PZL_WSG,
        .slv         = AA_TOADSTOOL_SLV_WSG,
        .marqueeFact = "MORE. SNACKS.",
    },
    {
        .name = "donut jump",
        .pzl  = AF_DONUTJUMP_PZL_WSG,
        .slv  = AF_DONUTJUMP_SLV_WSG,
        .marqueeFact
        = "Produced by MrTroy for the 2020 barrel swadge : King Donut jumps to all the blocks without being "
          "hit by the Evil Eclair or Devil Donut.",
    },
    {
        .name        = "picross",
        .pzl         = AF_PICROSS_PZL_WSG,
        .slv         = AF_PICROSS_SLV_WSG,
        .marqueeFact = "Produced by blooper for the 2023 wavebird swadge : the prequel to this game!",
    },
    {
        .name        = "super swadge land",
        .pzl         = AF_SUPERSWADGELAND_PZL_WSG,
        .slv         = AF_SUPERSWADGELAND_SLV_WSG,
        .marqueeFact = "Produced by JVeg199X for the 2023 wavebird swadge : Rack up a high score across 16 "
                       "distinct areas, each full of secrets and danger!",
    },
    {
        .name = "swadge bros",
        .pzl  = AF_SWADGEBROS_PZL_WSG,
        .slv  = AF_SWADGEBROS_SLV_WSG,
        .marqueeFact
        = "Produced by gelakinetic for the 2023 wavebird swadge with art by gplord & kaitie : Prove your might in "
          "wireless PvP or versus a CPU!",
    },
    {
        .name        = "tiltrads",
        .pzl         = AF_TILTRADS_PZL_WSG,
        .slv         = AF_TILTRADS_SLV_WSG,
        .marqueeFact = "Produced by jt-moriarty for the 2023 wavebird swadge : I'm using tilt controls! This game also "
                       "made a colorized return on the 2023 wavebird swadge.",
    },
    {
        .name = "galactic brickdown",
        .pzl  = CF_GALACTICBRICKDOWN_PZL_WSG,
        .slv  = CF_GALACTICBRICKDOWN_SLV_WSG,
        .marqueeFact
        = "Produced by JVeg199X for the 2024 gunship swadge : This is a unique take on a \"breakout\" style game.",
    },
    {
        .name        = "lumber jacks",
        .pzl         = CF_LUMBERJACKS_PZL_WSG,
        .slv         = CF_LUMBERJACKS_SLV_WSG,
        .marqueeFact = "Produced by MrTroy for the 2024 gunship swadge : Tight arcade action with panic mode featuring "
                       "rising water, and attack mode featuring wave after wave of Bad Seeds.",
    },
    {
        .name        = "magtroid pocket",
        .pzl         = CF_MAGTROIDPOCKET_PZL_WSG,
        .slv         = CF_MAGTROIDPOCKET_SLV_WSG,
        .marqueeFact = "Flagship title of the 2024 gunship swadge made by gelakinetic, gplord, newmajoe, and others : "
                       "Features a fully "
                       "colored doom-style rendering engine, an epic soundtrack, and a complex single player campaign.",
    },
    {
        .name = "pushy kawaii go",
        .pzl  = CF_PUSHYKAWAIIGO_PZL_WSG,
        .slv  = CF_PUSHYKAWAIIGO_SLV_WSG,
        .marqueeFact
        = "Produced by Bryce Browner for the 2024 gunship swadge : A swadge port of socks magoc's game. Number go up.",
    },
    {
        .name        = "big bug",
        .pzl         = EF_BIGBUG_PZL_WSG,
        .slv         = EF_BIGBUG_SLV_WSG,
        .marqueeFact = "Produced by James Albracht for the 2025 hotdog swadge with art from gplord, kaitie, Nathan, "
                       "and music from newmajoe, and livingston rampey : Dr. Ovo Garbotnik fights hordes of giant "
                       "bugs in a garbage landfill loosely inspired by Dig Dug and Helldivers 2.",
    },
    {
        .name = "pango",
        .pzl  = EF_PANGO_PZL_WSG,
        .slv  = EF_PANGO_SLV_WSG,
        .marqueeFact
        = "Produced by JVeg199X for the 2025 hotdog swadge : Another high score chaser in which Pango must "
          "destroy the Drill Bots by sliding ice blocks to hit them.",
    },
    {
        .name        = "2048",
        .pzl         = EF_2048_PZL_WSG,
        .slv         = EF_2048_SLV_WSG,
        .marqueeFact = "Produced by Jonny Wycliffe for the 2025 hotdog swadge : Did you know? 2048 is so named because "
                       "that's the highest value the dev was able to get to before he started to cry !",
    },
    {
        .name        = "chowa grove",
        .pzl         = EF_CHOWAGROVE_PZL_WSG,
        .slv         = EF_CHOWAGROVE_SLV_WSG,
        .marqueeFact = "Produced by Jonny Wycliffe for the 2025 hotdog swadge : Did you know? Chowa Grove was "
                       "going to have multiple Chowa types but the dev got too lazy",
    },
    {
        .name = "hunter's puzzles",
        .pzl  = EF_HUNTERSPUZZLES_PZL_WSG,
        .slv  = EF_HUNTERSPUZZLES_SLV_WSG,
        .marqueeFact
        = "Produced by blooper for the 2025 hotdog swadge : A sokoban-style puzzle game where you are an eye.",
    },
    {
        .name = "swadge hero",
        .pzl  = EF_SWADGEHERO_PZL_WSG,
        .slv  = EF_SWADGEHERO_SLV_WSG,
        .marqueeFact
        = "Swadge Hero from the 2025 hotdog swadge made by gelakinetic and many volunteer composers : A rhythm "
          "game showing off the new audio capabilities.",
    },
    {
        .name        = "ultimate ttt",
        .pzl         = EF_ULTIMATETTT_PZL_WSG,
        .slv         = EF_ULTIMATETTT_SLV_WSG,
        .marqueeFact = "Produced by gelakinetic for the 2025 hotdog swadge : Play 9 small PvP games at "
                       "the same dang time!",
    },
    {
        .name        = "leaf",
        .pzl         = FF_ACLEAF_PZL_WSG,
        .slv         = FF_ACLEAF_SLV_WSG,
        .marqueeFact = "The ultimate flat-pack furniture. IKEA could never.",
    },
    {
        .name        = "double bass",
        .pzl         = FF_DOUBLEBASS_PZL_WSG,
        .slv         = FF_DOUBLEBASS_SLV_WSG,
        .marqueeFact = "The Magfest Community Orchestra features over 200 volunteer musicians.",
    },
    {
        .name = "drum",
        .pzl  = FF_DRUM_PZL_WSG,
        .slv  = FF_DRUM_SLV_WSG,
        .marqueeFact
        = "Musicians of all instruments and skill levels join together to improve their chops at Jam Clinic.",
    },
    {
        .name        = "meteor",
        .pzl         = FF_METEOR_PZL_WSG,
        .slv         = FF_METEOR_SLV_WSG,
        .marqueeFact = "We're gonna need a bigger Holy.",
    },
    {
        .name        = "ps logo",
        .pzl         = FF_PSLOGO_PZL_WSG,
        .slv         = FF_PSLOGO_SLV_WSG,
        .marqueeFact = "You are now hearing the PS1 start-up jingle in your head.",
    },
    {
        .name        = "sax",
        .pzl         = FF_SAX_PZL_WSG,
        .slv         = FF_SAX_SLV_WSG,
        .marqueeFact = "Newmajoe's instrument of choice.",
    },
    {
        .name        = "sfc logo",
        .pzl         = FF_SFCLOGO_PZL_WSG,
        .slv         = FF_SFCLOGO_SLV_WSG,
        .marqueeFact = "YOU ARE A SUPER PLAYER!!",
    },
    {
        .name        = "tank",
        .pzl         = FF_TANK_PZL_WSG,
        .slv         = FF_TANK_SLV_WSG,
        .marqueeFact = "UTE! UTE! UTE! UTE! UTE!",
    },
    {
        .name        = "trumpet",
        .pzl         = FF_TRUMPET_PZL_WSG,
        .slv         = FF_TRUMPET_SLV_WSG,
        .marqueeFact = "Come see the orchestra perform on Saturday morning in the atrium!",
    },
    {
        .name        = "wing",
        .pzl         = FF_WING_PZL_WSG,
        .slv         = FF_WING_SLV_WSG,
        .marqueeFact = "What does a Moogle need in the morning? A Kup-o Coffee!",
    },
};

const trophyData_t trophyPicrossModeTrophies[] = {
    {
        .title       = "I missed my flight home",
        .description = "Solved all picross puzzles",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_EXTREME,
        .maxVal      = 1, // For trigger type, set to one
    },
    {
        .title       = "Understood the puzzle mechanics",
        .description = "Solved all 5x5 Puzzles",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_EASY,
        .maxVal      = 1, // For trigger type, set to one
    },
    {
        .title       = "A slim margin",
        .description = "Solved all 15x6 Puzzles",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_EASY,
        .maxVal      = 1, // For trigger type, set to one
    },
    {
        .title       = "1.875 : 1",
        .description = "Solved all 15x8 Puzzles",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_MEDIUM,
        .maxVal      = 1, // For trigger type, set to one
    },
    {
        .title       = "1 hectoblocks",
        .description = "Solved all 10x10 Puzzles",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_MEDIUM,
        .maxVal      = 1, // For trigger type, set to one
    },
    {
        .title       = "This is getting ridiculous",
        .description = "Solved all 15x10 Puzzles",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_MEDIUM,
        .maxVal      = 1, // For trigger type, set to one
    },

    {
        .title       = "Puzzles for ants",
        .description = "Solved all 15x12 Puzzles",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_HARD,
        .maxVal      = 1, // For trigger type, set to one
    },
    {
        .title       = "Slightly assymetrical",
        .description = "Solved all 15x14 Puzzles",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_HARD,
        .maxVal      = 1, // For trigger type, set to one
    },
    {
        .title       = "Impromptu eye exam",
        .description = "Solved all 15x15 Puzzles",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_EXTREME,
        .maxVal      = 1, // For trigger type, set to one
    },
};

const trophySettings_t trophyPicrossModeTrophySettings = {
    .drawFromBottom   = false,
    .staticDurationUs = DRAW_STATIC_US * 2,
    .slideDurationUs  = DRAW_SLIDE_US,
    .namespaceKey     = str_picrossTitle,
};

const trophyDataList_t trophyPicrossModeData = {
    .settings = &trophyPicrossModeTrophySettings,
    .list     = trophyPicrossModeTrophies,
    .length   = ARRAY_SIZE(trophyPicrossModeTrophies),
};

swadgeMode_t modePicross = {
    .modeName                 = str_picrossTitle,
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = false,
    .usesAccelerometer        = false,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = picrossEnterMode,
    .fnExitMode               = picrossExitMode,
    .fnMainLoop               = picrossMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = NULL,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL,
    .fnDacCb                  = NULL,
    .fnAddToSwadgePassPacket  = NULL,
    .trophyData               = &trophyPicrossModeData,
};

picrossMenu_t* pm;

//==============================================================================
// Functions
//==============================================================================
/**
 * @brief Enter the picross mode by displaying the top level menu
 *
 * @param disp The display to draw to
 *
 */
void picrossEnterMode(void)
{
    // Allocate and zero memory
    pm = heap_caps_calloc(1, sizeof(picrossMenu_t), MALLOC_CAP_8BIT);

    // Load sounds
    loadMidiFile(LULLABY_IN_NUMBERS_MID, &pm->bgm, true);

    // Init sound player
    midiPlayer_t* player = globalMidiPlayerGet(MIDI_BGM);
    player->loop         = true;
    midiGmOn(player);
    soundPlayBgm(&pm->bgm, MIDI_BGM);

    loadFont(MM_FONT, &(pm->mmFont), false);

    pm->menu                 = initMenu(str_picrossTitle, picrossMainMenuCb);
    pm->renderer             = initMenuMegaRenderer(NULL, NULL, NULL);
    pm->renderer->bgColorIdx = 0;

    pm->screen = PICROSS_MENU;

    addSingleItemToMenu(pm->menu, str_levelSelect);
    addSingleItemToMenu(pm->menu, str_howtoplay);

    pm->menu = startSubMenu(pm->menu, str_options);

    settingParam_t sp_tf = {
        .min = trueFalseVals[0],
        .max = trueFalseVals[ARRAY_SIZE(trueFalseVals) - 1],
        .def = trueFalseVals[0],
    };
    settingParam_t sp_bg = {
        .min = backgroundVals[0],
        .max = backgroundVals[ARRAY_SIZE(backgroundVals) - 1],
        .def = backgroundVals[0],
    };
    addSettingsOptionsItemToMenu(pm->menu, str_Guides, strs_on_off, trueFalseVals, ARRAY_SIZE(strs_on_off), &sp_tf,
                                 picrossGetSaveFlag(PO_SHOW_GUIDES));
    addSettingsOptionsItemToMenu(
        pm->menu, str_AnimateBG, str_Backgrounds, backgroundVals, ARRAY_SIZE(str_Backgrounds), &sp_bg,
        -1 + picrossGetLoadedSaveFlag(PO_BG_HEXAGONS) + (picrossGetLoadedSaveFlag(PO_BG_DOTS) * 2)
            + (picrossGetLoadedSaveFlag(PO_BG_NONE) * 3));
    addSettingsOptionsItemToMenu(pm->menu, str_Mark, strs_on_off, trueFalseVals, ARRAY_SIZE(strs_x_solid), &sp_tf,
                                 picrossGetLoadedSaveFlag(PO_MARK_X));
    addSettingsOptionsItemToMenu(pm->menu, str_Hints, strs_on_off, trueFalseVals, ARRAY_SIZE(strs_on_off), &sp_tf,
                                 picrossGetLoadedSaveFlag(PO_SHOW_HINTS));
    pm->menu = endSubMenu(pm->menu);

    addSingleItemToMenu(pm->menu, str_exit);

    loadLevels();
}

void picrossExitMode(void)
{
    unloadMidiFile(&pm->bgm);
    switch (pm->screen)
    {
        case PICROSS_MENU:
        {
            // Nothing extra
            break;
        }
        case PICROSS_LEVELSELECT:
        {
            picrossExitLevelSelect();
            break;
        }
        case PICROSS_GAME:
        {
            picrossExitGame();
            break;
        }
        case PICROSS_TUTORIAL:
        {
            picrossExitTutorial();
            break;
        }
    }

    // Free WSG's
    for (int i = 0; i < PICROSS_LEVEL_COUNT; i++)
    {
        freeWsg(&pm->levels[i].levelWSG);
        freeWsg(&pm->levels[i].completedWSG);
    }
    picrossExitLevelSelect(); // this doesnt actually get called as we go in and out of levelselect (because it breaks
                              // everything), so lets call it now
    deinitMenu(pm->menu);
    deinitMenuMegaRenderer(pm->renderer);
    // p2pDeinit(&jm->p2p, true);
    freeFont(&(pm->mmFont));
    heap_caps_free(pm);
}

/**
 * @brief Loads in all of the level files. Each level is made up of a black/white data file, and a completed color
 * version. The black.white version can be white or transparent for empty spaces, and any other pixel value for filled
 * spaces. Clues and level size is determined by the images, but only 10x10 is currently supported. It will completely
 * break if it's not 10x10 right now. I would like to support any size that is <=15, including non-square puzzles. First
 * is getting them to render and not write out of bounds, second is scaling and placing the puzzle on screen correctly.
 *
 */
void loadLevels()
{
    // LEVEL SETUP AREA
    // DACVAK JUST SEARCH FOR "DACVAK" TO FIND THIS

    // Todo: we can cut our memory use down by about 2/3 if we use a naming convention and the titles to pull the wsg
    // names. snprint into loadWsg, i think. Make sure it all works with, say, apostophes and spaces.

    for (int32_t i = 0; i < MIN(PICROSS_LEVEL_COUNT, ARRAY_SIZE(picrossPuzzles)); i++)
    {
        pm->levels[i].title = picrossPuzzles[i].name;
        loadWsg(picrossPuzzles[i].pzl, &pm->levels[i].levelWSG, false);
        loadWsg(picrossPuzzles[i].slv, &pm->levels[i].completedWSG, false);
        pm->levels[i].marqueeFact = picrossPuzzles[i].marqueeFact;
    }

    // dont forget to update PICROSS_LEVEL_COUNT (in #define in picross_consts.h) when adding levels.

    // set indices. Used to correctly set save data. levels are loaded without context of the levels array, so they
    // carry the index info with them so we can save victories.
    for (int i = 0; i < PICROSS_LEVEL_COUNT; i++) // 8 should = number of levels and that should = levelCount.
    {
        pm->levels[i].index = i;
    }
}

/**
 * Call the appropriate main loop function for the screen being displayed
 *
 * @param elapsedUd Time.deltaTime
 */
void picrossMainLoop(int64_t elapsedUs)
{
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        picrossButtonCb(&evt);
    }

    // Check for touchpad touches
    int32_t phi       = 0;
    int32_t r         = 0;
    int32_t intensity = 0;
    picrossTouchCb(getTouchJoystick(&phi, &r, &intensity));

    switch (pm->screen)
    {
        case PICROSS_MENU:
        {
            drawMenuMega(pm->menu, pm->renderer, elapsedUs);
            break;
        }
        case PICROSS_LEVELSELECT:
        {
            picrossLevelSelectLoop(elapsedUs);
            break;
        }
        case PICROSS_GAME:
        {
            picrossGameLoop(elapsedUs);
            break;
        }
        case PICROSS_TUTORIAL:
        {
            picrossTutorialLoop(elapsedUs);
            break;
        }
    }
}

/**
 * @brief Call the appropriate button functions for the screen being displayed
 *
 * @param evt
 */
void picrossButtonCb(buttonEvt_t* evt)
{
    switch (pm->screen)
    {
        case PICROSS_MENU:
        {
            pm->menu = menuButton(pm->menu, *evt);
            break;
        }
        case PICROSS_LEVELSELECT:
        {
            picrossLevelSelectButtonCb(evt);
            break;
        }
        case PICROSS_GAME:
        {
            picrossGameButtonCb(evt);
            break;
        }
        case PICROSS_TUTORIAL:
        {
            picrossTutorialButtonCb(evt);
            break;
        }
    }
}

void picrossTouchCb(bool touched)
{
    if (pm->screen == PICROSS_GAME)
    {
        picrossGameTouchCb(touched);
    }
}

/////////////////////////

/**
 * @brief Frees level select menu and returns to the picross menu, except skipping past the level select menu.
 *
 */
void returnToPicrossMenu(void)
{
    pm->screen = PICROSS_MENU;
    // Reinit MenuMegaRenderer to go back to using default menu colors.
    deinitMenuMegaRenderer(pm->renderer);
    pm->renderer = initMenuMegaRenderer(NULL, NULL, NULL);
}

/**
 * @brief Frees level select menu and returns to the picross menu. Should not be called by not-the-level-select-menu.
 *
 */
void returnToPicrossMenuFromLevelSelect(void)
{
    picrossExitLevelSelect(); // free data
    returnToPicrossMenu();
}

/**
 * @brief Exits the tutorial mode.
 *
 */
void exitTutorial(void)
{
    pm->screen = PICROSS_MENU;
}

void continueGame(bool solved, int8_t currentIdx)
{
    // get the current level index
    int32_t currentIndex = 0; // just load 0 if its 0.
    readNvs32(picrossCurrentPuzzleIndexKey, &currentIndex);

    // Set the level background to more muted colors.
    pm->renderer->bgColors    = bgColors;
    pm->renderer->numBgColors = ARRAY_SIZE(bgColors);

    // load in the level we selected.
    // uh. read the currentLevelIndex and get the value from
    if (solved)
    {
        currentIndex = currentIdx;
    }
    picrossStartGame(&pm->mmFont, &pm->levels[currentIndex], true, pm->renderer, solved);
    pm->screen = PICROSS_GAME;
}

// menu button & options menu callbacks. Set the screen and call the appropriate start functions
bool picrossMainMenuCb(const char* label, bool selected, uint32_t value)
{
    if (selected)
    {
        if (label == str_continue)
        {
            continueGame(false, -1);

            // Turn off LEDs
            led_t leds[CONFIG_NUM_LEDS] = {0};
            setLeds(leds, CONFIG_NUM_LEDS);
        }
        else if (label == str_howtoplay)
        {
            pm->screen = PICROSS_TUTORIAL;
            picrossStartTutorial(&pm->mmFont);

            // Turn off LEDs
            led_t leds[CONFIG_NUM_LEDS] = {0};
            setLeds(leds, CONFIG_NUM_LEDS);
        }
        else if (label == str_levelSelect)
        {
            pm->screen = PICROSS_LEVELSELECT;
            picrossStartLevelSelect(&pm->mmFont, pm->levels);

            // Turn off LEDs
            led_t leds[CONFIG_NUM_LEDS] = {0};
            setLeds(leds, CONFIG_NUM_LEDS);
        }
        else if (label == str_exit)
        {
            // Exit to main menu
            switchToSwadgeMode(&mainMenuMode);
        }
        else if (label == str_eraseProgress)
        {
            // todo: still need to do a confirmation on these, probably just by changing the text.
            // Commented out
            //  //Next time we load a puzzle it will re-save and zero-out the data, we just have to tell the mennu not
            //  to show the 'continue' option.
            writeNvs32(picrossCurrentPuzzleIndexKey, -1);

            // see comment above as to why this isn't needed.
            //  size_t size = sizeof(picrossProgressData_t);
            //  picrossProgressData_t* progData = heap_caps_calloc(1,size, MALLOC_CAP_8BIT);//zero out = reset.
            //  writeNvsBlob(picrossProgressData,progData,size);
            //  heap_caps_free(progData);

            // the code to erase ALL (victory) progress. Still want to put this... somewhere
            //  size_t size = sizeof(picrossVictoryData_t);
            //  picrossVictoryData_t* victData = heap_caps_calloc(1,size, MALLOC_CAP_8BIT);//zero out = reset.
            //  writeNvsBlob(picrossCompletedLevelData,victData,size);
            //  heap_caps_free(victData);
        }
    }
    else
    {
        // Not selected
        if (str_Guides == label)
        {
            picrossSetSaveFlag(PO_SHOW_GUIDES, value);
        }
        else if (str_AnimateBG == label)
        {
            switch (value)
            {
                case PICROSS_BG_HEXAGONS:
                {
                    picrossSetSaveFlag(PO_BG_HEXAGONS, 1);
                    picrossSetSaveFlag(PO_BG_DOTS, 0);
                    picrossSetSaveFlag(PO_BG_NONE, 0);
                    break;
                }
                case PICROSS_BG_DOTS:
                {
                    picrossSetSaveFlag(PO_BG_HEXAGONS, 0);
                    picrossSetSaveFlag(PO_BG_DOTS, 1);
                    picrossSetSaveFlag(PO_BG_NONE, 0);
                    break;
                }
                case PICROSS_BG_NONE:
                {
                    picrossSetSaveFlag(PO_BG_HEXAGONS, 0);
                    picrossSetSaveFlag(PO_BG_DOTS, 0);
                    picrossSetSaveFlag(PO_BG_NONE, 1);
                    break;
                }
            }
        }
        else if (str_Hints == label)
        {
            picrossSetSaveFlag(PO_SHOW_HINTS, value);
        }
        else if (str_Mark == label)
        {
            picrossSetSaveFlag(PO_MARK_X, value);
        }
    }
    return false;
}

/**
 * @brief Selects a picross level then starts the game. Called by level select, and sends us into mode_picross.c Doesn't
 * free memory.
 *
 * @param selectedLevel Pointer to the a level struct.
 */
void selectPicrossLevel(picrossLevelDef_t* selectedLevel)
{
    // picrossExitLevelSelect();//we do this BEFORE we enter startGame.
    pm->screen = PICROSS_GAME;
    picrossStartGame(&pm->mmFont, selectedLevel, false, pm->renderer, false);
}

// void returnToLevelSelect()//todo: rename
// {
//     //todo: abstract this to function
//     pm->screen = PICROSS_LEVELSELECT;
//     //todo: fix below warning
//     picrossStartLevelSelect(&pm->mmFont,pm->levels);
// }

/**
 * @brief Save data is stored in a single integer, using the register as 32 flags.
 * Hints on/off is pos 0.
 * Guide on/off is pos 1
 * BG Animation is pos 2
 *
 * @param pos Pos (<32) which flag to get.
 * @return true
 * @return false
 */
bool picrossGetSaveFlag(picrossOption_t pos)
{
    if (false == readNvs32(picrossSavedOptionsKey, &pm->options))
    {
        // set default options
        // x's on, bg on, guide on, hintwarning off. On the fence on guides on or off.
        int32_t defaults = (1 << PO_BG_HEXAGONS) | (1 << PO_SHOW_GUIDES) | (1 << PO_MARK_X);
        writeNvs32(picrossSavedOptionsKey, defaults);
        pm->options = defaults;
    }

    return picrossGetLoadedSaveFlag(pos);
}

/**
 * @brief once you have called picrossGetSaveFlag for any position, you don't need to call it again until an option
 * changes. As we often read a bunch of values in a row, this function can be used after the first one to save on
 * uneccesary read calls.
 *
 * @param pos
 * @return true
 * @return false
 */
bool picrossGetLoadedSaveFlag(picrossOption_t pos)
{
    int val = (pm->options & (1 << pos)) >> pos;
    return val == 1;
}

// also sets pm->options
/**
 * @brief Sets a flag. Sets pm->options, which is actually used by the game without necessarily having to (repeatedly)
 * call picrossGetSaveFlag. Of course, it also also saves it to perma.
 *
 * @param pos index to get. 0<pos<32.
 * @param on save vale.
 */
void picrossSetSaveFlag(picrossOption_t pos, bool on)
{
    if (on)
    {
        pm->options = pm->options | (1 << (pos));
    }
    else
    {
        pm->options = pm->options & ~(1 << (pos));
    }
    writeNvs32(picrossSavedOptionsKey, pm->options);
}