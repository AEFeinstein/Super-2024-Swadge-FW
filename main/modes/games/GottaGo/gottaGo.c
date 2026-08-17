//==============================================================================
// Includes
//==============================================================================

#include "gottaGo.h"
#include "mainMenu.h"

//==============================================================================
// Defines
//==============================================================================

// Main
#define FRAME_RATE_US 16667
#define SECOND        1000000

// Splash
#define SPLASH_US          500000
#define SPLASH_TEXT_Y      210
#define SPLASH_TEXT_BUFFER 2
#define SPLASH_TEXT_X      44
#define SPLASH_TEXT_Y_2    44

// Menu
#define OPTION_SPACING 35
#define OPTION_BUFFER  20
#define OPTION_X       32
#define MENU_OPTIONS   4

// Rules
#define RULES_X_BORDER 32
#define RULES_TITLE_Y  18
#define RULES_DESC_Y   8

// High scores
#define HS_PAGES     2
#define TIMER_BUFFER 1000000

// Game
#define READY_TIMEOUT  3000000  // 3 Secs
#define MAX_TIMER_LEN  10000000 // 10 Seconds
#define TIMER_CHANGE   27       // What fraction of the timer is removed each round
#define SOLUTION_TIMER 2000000

// Bathroom
#define FLOOR_HEIGHT 40

// Toilets
#define MAX_TOILETS         7
#define DIVIDER_X_OFFSET    32
#define DIVIDER_TOP_OFFSET  -24
#define DIVIDER_BOT_OFFSET  12
#define URINAL_SPACING      41
#define URINAL_7_SPACING    37
#define SMALL_URINAL_OFFSET 15
#define URINAL_HEIGHT       132
#define URINAL_MAX_WIDTH    259
#define URINAL_DEFAULT_H    35

// NPCs
#define LEGS_OFFSET        8
#define NPC_HEIGHT_OFFSET  (URINAL_HEIGHT - 100)
#define NPC_X_OFFSET       3
#define NPC_PANTS_X_OFFSET 6
#define NPC_PANTS_X_LARGE  -3
#define NPC_PANTS_Y_HIKE   -12

// UI
#define UI_STALL_X       12
#define UI_STALL_Y       44
#define UI_X_DIM         12
#define UI_SPACING       4
#define UI_BAR_PADDING_X 12
#define UI_BAR_PADDING_Y 28
#define UI_BAR_HEIGHT    10
#define UI_TIMER_X       64
#define UI_TIMER_Y       5

//==============================================================================
// Consts
//==============================================================================

// Text
const char ggModeName[]               = "Gotta Go!";
static const char* const ggNVSSpace[] = {
    "ggSaves", "maxGames", "avg", "score", "adjScore",
};
static const char* const strings[] = {
    "Press 'A' to start!",
    "Play!",
    "Rules",
    "High Scores",
    "Quit",
    "Nice Choice!",
    "Press any button to advance to next round",
    "You peed yourself",
    "Press any button to go back to the menu",
    "ATTENTION",
    "This game contains 'suggestive themes' and may not be suitable for all audiences. Press A to continue and "
    "anything else to back out.",
    "Suggestive themes is a stupid way to put 'this game shows butts.' Saturday morning cartoons shows butts. "
    "Everyone has one, this isn't going to cause someone to have some sort of awakening or turn your children into "
    "perverts, it's just some very low pixel count butts. Geting you underwear in a twist over this says more about "
    "you that you would probably like.",
    "Ready?",
    "3",
    "2",
    "1!",
    "Paused",
    "Press A to continue, B to quit",
    "High Scores",
    "You used a Stall.",
    "You used a broken toilet",
    "All the stalls are filled",
};
static const char* const rulesText[] = {
    "Rules",
    "Here's the rules book for Gotta Go! The rules should be instinctual for a lot of people, but for those that don't "
    "use urinals on a regular basis, this will help.",
    "Controls",
    "Use the left/right arrows to pick a urinal, A to select it, or B to use the stall. There is no pausing!",
    "How to play",
    "Once you're ready, you must quickly figure out the most ideal urinal out of the available options. Be quick, your "
    "bladder is about to burst!",
    "Urinals",
    "You always want to use the cleanest urinal. Obviously. Try not to stand in puddles, and if you use one that's out "
    "of order and increase the mess, expect to be judged.",
    "People",
    "People are weird. Try to stay as far away from them as possible. If you have to pick, always pick the most normal "
    "one. That means not the guy with his pants around his ankles or the guy that smells like onions.",
    "Stalls",
    "You can use the stall sometimes, but you only have a limit amount of times you can do that. No hogging the "
    "stalls.",
    "Scoring",
    "Score is tracked in three ways: Total score, accuracy, and adjusted score. The total score is an accumulation of "
    "all the score up to this point. Accuracy score is how close to optimal picks you are. Th adjusted score takes "
    "your total score and adjusts it by your accuracy to provide a final number that's easy to compare.",
};

// Images
static const cnfsFileIdx_t splashImages[] = {
    GG_PIPE_G_WSG, GG_PIPE_O_WSG, GG_PIPE_T_WSG, GG_PIPE_A_WSG, GG_PIPE_EXC_MARK_WSG, GG_VALVE_O_WSG,
};
static const cnfsFileIdx_t backgroundImages[] = {
    GG_BATHROOM_DOOR_WSG,
    GG_BATHROOM_TILE_WSG,
};
static const cnfsFileIdx_t toiletImages[] = {
    GG_FLUSH_HANDLE_WSG,
    GG_FLUSH_AUTO_WSG,
    GG_TOILET_TOP_WSG,
    GG_TOILET_STRIP_WSG,
    GG_TOILET_BOTTOM_WSG,
    GG_TOILET_BOTTOM_BROKEN_WSG,
    GG_TOILET_BOTTOM_BROKEN_2_WSG,
    GG_DOWNPIPE_WSG,
    GG_DOWNPIPE_BROKEN_WSG,
    GG_DOWNPIPE_BROKEN_2_WSG,
    GG_GRAFFITI_1_WSG,
    GG_GRAFFITI_2_WSG,
    GG_GRAFFITI_3_WSG,
    GG_CRACK_1_WSG,
    GG_CRACK_2_WSG,
    GG_WATER_LEAK_WSG,
    GG_PLUGGED_DRAIN_WSG,
    GG_OUT_OF_ORDER_WSG,
    GG_DIVIDER_WSG,
    GG_DIVIDER_TOP_WSG,
    GG_DIVIDER_BOTTOM_WSG,
    GG_PEE_S_WSG,
    GG_PEE_M_WSG,
    GG_PEE_L_WSG,
};
static const cnfsFileIdx_t uiImages[] = {
    GG_STALL_ICON_WSG,
    GG_ARROW_WSG,
    GG_X_WSG,
    GG_CHECK_WSG,
};
static const cnfsFileIdx_t npcImages[] = {
    GG_UPPER_BOD_WSG,   GG_LEGS_WSG,   GG_FEET_WSG,  GG_STINK_WSG,     GG_SHIRT_WSG,     GG_PANTS_WSG,
    GG_PANTS_SHORT_WSG, GG_SHORTS_WSG, GG_SKIRT_WSG, GG_ON_GROUND_WSG, GG_UNDERWEAR_WSG,
};

// Color Arrays
static const paletteColor_t skinColors[] = {
    c555, c333, c444, c023, c402, c233, c343,
};
static const paletteColor_t shirtColors[] = {
    c500, c050, c005, c440, c204, c404, c044,
};
static const paletteColor_t shirtAccentColors[] = {
    c400, c040, c004, c330, c103, c303, c033,
};
static const paletteColor_t pantsColors[] = {
    c024, c330, c222, c224, c503, c240, c031,
};
static const paletteColor_t pantsAccentColors[] = {
    c012, c220, c111, c113, c402, c130, c020,
};
static const paletteColor_t shoeColors[] = {
    c210,
    c000,
    c111,
    c300,
};

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    GG_WARNING,
    GG_SPLASH,
    GG_MENU,
    GG_RULES,
    GG_READY,
    GG_GAME,
    GG_CHOICE,
    GG_WIN,
    GG_LOSE,
    GG_HIGHSCORE,
} ggState_t;

typedef enum
{
    GG_PLAY_GAME,
    GG_SHOW_RULES,
    GG_SHOW_HS,
    GG_QUIT,
} ggMenuItems_t;

typedef enum
{
    GG_PEE_NONE,
    GG_PEE_SMALL,
    GG_PEE_MED,
    GG_PEE_LARGE,
} ggPeePool_t;

typedef enum
{
    GG_DIV_FULL,
    GG_DIV_TOP,
    GG_DIV_BOTTOM,
    GG_DIV_NONE,
} ggDivider_t;

typedef enum
{
    GG_PANTS,
    GG_SHORTS,
    GG_SKIRT,
    GG_DOWN,
    GG_UNDERWEAR,
    GG_NAKED,
} ggPants_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    int skinColor;
    int shirtColor;
    int pantsColor;
    int shoeColor;
    int randOffset;
    uint8_t active : 1;
    uint8_t small  : 1;
    uint8_t shirt  : 1;
    uint8_t pants  : 3;
    uint8_t shoes  : 1;
    uint8_t stink  : 1;
} ggNPC_t;

typedef struct
{
    int height; // Height of actual unit (default: 35)
    uint8_t autoFlush    : 1;
    uint8_t graffiti     : 3;
    uint8_t cracks       : 2;
    uint8_t waterLeak    : 1;
    uint8_t brokenDrain  : 1;
    uint8_t outOfOrder   : 1;
    uint8_t brokenBowl   : 1;
    uint8_t pluggedDrain : 1;
    uint8_t puddle       : 2;
    uint8_t divider      : 2;
    uint8_t small        : 1;
    ggNPC_t npc;
} ggToilet_t;

// TODO: Reorder
typedef struct
{
    // Main
    ggState_t state;
    int64_t timer;
    font_t normalFont;
    font_t normalFontOutline;
    font_t smallFont;
    int selection;

    // Splash
    bool splashToggle;
    wsg_t* splashImgs;

    // Game
    int score;
    int avgScore;
    int adjScore;
    int64_t loseTimerMax;
    int stallUses;
    int stallInc;
    int numGames;
    bool pause;
    bool stallUsed;
    bool lose;
    bool timeOut;

    // Bathroom
    wsg_t* backgroundImages;
    wsg_t* toiletImages;
    wsg_t* npcImages;
    int numActive;
    ggToilet_t toilets[MAX_TOILETS]; // Based on physical width
    wsgPalette_t npcPalette;

    // UI
    wsg_t* uiImages;
} ggData_t;

//==============================================================================
// Functions declarations
//==============================================================================

// Main
static void ggEnterMode(void);
static void ggExitMode(void);
static void ggMainLoop(int64_t elapsedUs);

// Game
static void clearToilets(void);
static void initNPC(ggToilet_t* t, bool active);
static void gameInit(void);
static void initRandomNPC(ggToilet_t* t);
static void end(bool lose, bool wasStall);
static void calcToiletScore(int* values);
static void saveToNVS(void);

// Drawing
// Warning
static void drawWarning(int64_t elapsedUs);
// Splash
static void initSplash(void);
static void drawSplash(int64_t elapsedUs);
static void drawTitle(void);
// Menu
static void drawMenu(void);
// Rules
static void drawRules(void);
// High Score
static void drawHighScore(void);
// Ready
static void drawReady(int64_t elapsedUs);
// Game
static void drawGame(bool ui);
static void drawPause(void);
static void drawSolution(void);
// Win
static void drawWin(void);
// Lose
static void drawLose(void);
// Common draw
static void drawBackground(void);
static void drawToiletArray(void);
static void drawToilet(ggToilet_t* t, int x, int y);
static void drawNPC(ggToilet_t* t, int x, int y);
static void drawUI(void);
static void drawScores(int x, int y);

//==============================================================================
// Variables
//==============================================================================

swadgeMode_t gottaGoMode = {
    .modeName          = ggModeName,
    .wifiMode          = NO_WIFI,
    .overrideUsb       = false,
    .usesAccelerometer = false,
    .usesThermometer   = false,
    .fnEnterMode       = ggEnterMode,
    .fnExitMode        = ggExitMode,
    .fnMainLoop        = ggMainLoop,
    .overrideSelectBtn = true,
};

ggData_t* ggd;

//==============================================================================
// Functions
//==============================================================================

// Main
static void ggEnterMode(void)
{
    setFrameRateUs(FRAME_RATE_US);
    // Loading resources
    ggd = (ggData_t*)heap_caps_calloc(1, sizeof(ggData_t), MALLOC_CAP_8BIT);
    // Font
    loadFont(PULSE_AUX_FONT, &ggd->normalFont, true); // FIXME: Need a better font
    makeOutlineFont(&ggd->normalFont, &ggd->normalFontOutline, true);
    loadFont(OXANIUM_13MED_FONT, &ggd->smallFont, true);
    // Images
    ggd->splashImgs = heap_caps_calloc(ARRAY_SIZE(splashImages), sizeof(wsg_t), MALLOC_CAP_8BIT);
    for (int idx = 0; idx < ARRAY_SIZE(splashImages); idx++)
    {
        loadWsg(splashImages[idx], &ggd->splashImgs[idx], true);
    }
    ggd->backgroundImages = heap_caps_calloc(ARRAY_SIZE(backgroundImages), sizeof(wsg_t), MALLOC_CAP_8BIT);
    for (int idx = 0; idx < ARRAY_SIZE(backgroundImages); idx++)
    {
        loadWsg(backgroundImages[idx], &ggd->backgroundImages[idx], true);
    }
    ggd->toiletImages = heap_caps_calloc(ARRAY_SIZE(toiletImages), sizeof(wsg_t), MALLOC_CAP_8BIT);
    for (int idx = 0; idx < ARRAY_SIZE(toiletImages); idx++)
    {
        loadWsg(toiletImages[idx], &ggd->toiletImages[idx], true);
    }
    ggd->uiImages = heap_caps_calloc(ARRAY_SIZE(uiImages), sizeof(wsg_t), MALLOC_CAP_8BIT);
    for (int idx = 0; idx < ARRAY_SIZE(uiImages); idx++)
    {
        loadWsg(uiImages[idx], &ggd->uiImages[idx], true);
    }
    ggd->npcImages = heap_caps_calloc(ARRAY_SIZE(npcImages), sizeof(wsg_t), MALLOC_CAP_8BIT);
    for (int idx = 0; idx < ARRAY_SIZE(npcImages); idx++)
    {
        loadWsg(npcImages[idx], &ggd->npcImages[idx], true);
    }

    // Initialize
    wsgPaletteReset(&ggd->npcPalette);
    ggd->state = GG_WARNING;
}

static void ggExitMode(void)
{
    for (int idx = 0; idx < ARRAY_SIZE(npcImages); idx++)
    {
        freeWsg(&ggd->npcImages[idx]);
    }
    free(ggd->npcImages);
    for (int idx = 0; idx < ARRAY_SIZE(uiImages); idx++)
    {
        freeWsg(&ggd->uiImages[idx]);
    }
    free(ggd->uiImages);
    for (int idx = 0; idx < ARRAY_SIZE(toiletImages); idx++)
    {
        freeWsg(&ggd->toiletImages[idx]);
    }
    free(ggd->toiletImages);
    for (int idx = 0; idx < ARRAY_SIZE(backgroundImages); idx++)
    {
        freeWsg(&ggd->backgroundImages[idx]);
    }
    free(ggd->backgroundImages);
    for (int idx = 0; idx < ARRAY_SIZE(splashImages); idx++)
    {
        freeWsg(&ggd->splashImgs[idx]);
    }
    free(ggd->splashImgs);
    freeFont(&ggd->smallFont);
    freeFont(&ggd->normalFontOutline);
    freeFont(&ggd->normalFont);
    free(ggd);
}

static void ggMainLoop(int64_t elapsedUs)
{
    buttonEvt_t evt;
    switch (ggd->state)
    {
        case GG_WARNING:
        {
            while (checkButtonQueueWrapper(&evt))
            {
                if (evt.down && (evt.button & PB_A))
                {
                    initSplash();
                    ggd->state = GG_SPLASH;
                }
                else if (evt.down)
                {
                    switchToSwadgeMode(&mainMenuMode);
                }
            }
            drawWarning(elapsedUs);
            break;
        }
        case GG_SPLASH:
        {
            while (checkButtonQueueWrapper(&evt))
            {
                if (evt.button & PB_A && evt.down)
                {
                    ggd->state = GG_MENU;
                }
            }
            drawSplash(elapsedUs);
            break;
        }
        case GG_MENU:
        {
            while (checkButtonQueueWrapper(&evt))
            {
                if (evt.down)
                {
                    if (evt.button & PB_DOWN)
                    {
                        ggd->selection++;
                        ggd->selection %= MENU_OPTIONS;
                    }
                    if (evt.button & PB_UP)
                    {
                        ggd->selection--;
                        if (ggd->selection < 0)
                        {
                            ggd->selection = MENU_OPTIONS - 1;
                        }
                    }
                    if (evt.button & PB_B)
                    {
                        ggd->state = GG_SPLASH;
                    }
                    if (evt.button & PB_A)
                    {
                        // Move to appropriate mode
                        switch (ggd->selection)
                        {
                            case GG_PLAY_GAME:
                            {
                                ggd->state        = GG_READY;
                                ggd->timer        = 0;
                                ggd->score        = 0;
                                ggd->avgScore     = 0;
                                ggd->adjScore     = 0;
                                ggd->stallInc     = 3;
                                ggd->stallUses    = 3;
                                ggd->numGames     = 0;
                                ggd->timeOut      = false;
                                ggd->loseTimerMax = MAX_TIMER_LEN;
                                break;
                            }
                            case GG_SHOW_RULES:
                            {
                                ggd->state     = GG_RULES;
                                ggd->selection = 0;
                                break;
                            }
                            case GG_SHOW_HS:
                            {
                                ggd->state     = GG_HIGHSCORE;
                                ggd->selection = 0;
                                break;
                            }
                            case GG_QUIT:
                            {
                                switchToSwadgeMode(&mainMenuMode);
                            }
                            default:
                            {
                                break;
                            }
                        }
                    }
                }
            }
            drawMenu();
            break;
        }
        case GG_RULES:
        {
            int size = ARRAY_SIZE(rulesText) / 2;
            while (checkButtonQueueWrapper(&evt))
            {
                if (!evt.down)
                {
                    if (evt.button & PB_RIGHT)
                    {
                        ggd->selection++;
                        ggd->selection %= size;
                    }
                    if (evt.button & PB_LEFT)
                    {
                        ggd->selection--;
                        if (ggd->selection < 0)
                        {
                            ggd->selection = size - 1;
                        }
                    }
                    if (evt.button & PB_B)
                    {
                        ggd->selection = GG_SHOW_RULES;
                        ggd->state     = GG_MENU;
                    }
                }
            }
            drawRules();
            break;
        }
        case GG_HIGHSCORE:
        {
            while (checkButtonQueueWrapper(&evt))
            {
                if (evt.button & PB_DOWN)
                {
                    ggd->selection++;
                    ggd->selection %= HS_PAGES;
                }
                if (evt.button & PB_UP)
                {
                    ggd->selection--;
                    if (ggd->selection < 0)
                    {
                        ggd->selection = HS_PAGES - 1;
                    }
                }
                if (evt.button & PB_B)
                {
                    ggd->selection = GG_SHOW_HS;
                    ggd->state     = GG_MENU;
                }
            }
            drawHighScore();
            break;
        }
        case GG_READY:
        {
            while (checkButtonQueueWrapper(&evt))
            {
                // Allows for backing out of the mode.
                if (evt.down)
                {
                    ggd->timer += SECOND;
                }
            }
            ggd->timer += elapsedUs;
            if (ggd->timer >= READY_TIMEOUT)
            {
                ggd->state = GG_GAME;
                gameInit();
            }
            drawReady(elapsedUs);
            break;
        }
        case GG_GAME:
        {
            while (checkButtonQueueWrapper(&evt))
            {
                if (evt.down)
                {
                    if (evt.button & PB_A)
                    {
                        if (ggd->pause)
                        {
                            ggd->pause = false;
                        }
                        else
                        {
                            ggd->lose      = ggd->toilets[ggd->selection].brokenBowl == 1
                                             || ggd->toilets[ggd->selection].brokenDrain == 1
                                             || ggd->toilets[ggd->selection].outOfOrder == 1
                                             || ggd->toilets[ggd->selection].pluggedDrain == 1;
                            ggd->stallUsed = false;
                            ggd->state     = GG_CHOICE;
                            ggd->timer     = 0;
                        }
                    }
                    else if (evt.button & PB_B)
                    {
                        if (ggd->pause)
                        {
                            ggd->pause     = false;
                            ggd->lose      = true;
                            ggd->stallUsed = false;
                            ggd->state     = GG_CHOICE;
                            ggd->timer     = 0;
                        }
                        else if (ggd->stallUses > 0)
                        {
                            ggd->stallUses--;
                            ggd->lose      = false;
                            ggd->stallUsed = true;
                            ggd->state     = GG_CHOICE;
                            ggd->timer     = 0;
                        }
                        else
                        {
                            ggd->lose      = true;
                            ggd->stallUsed = true;
                            ggd->state     = GG_CHOICE;
                            ggd->timer     = 0;
                        }
                    }
                    else if (evt.button & PB_LEFT)
                    {
                        ggd->selection--;
                        if (ggd->selection < 0)
                        {
                            ggd->selection = ggd->numActive - 1;
                        }
                        while (ggd->toilets[ggd->selection].npc.active != 0)
                        {
                            ggd->selection--;
                            if (ggd->selection < 0)
                            {
                                ggd->selection = ggd->numActive - 1;
                            }
                        }
                    }
                    else if (evt.button & PB_RIGHT)
                    {
                        ggd->selection++;
                        if (ggd->selection >= ggd->numActive)
                        {
                            ggd->selection = 0;
                        }
                        while (ggd->toilets[ggd->selection].npc.active != 0)
                        {
                            ggd->selection++;
                            if (ggd->selection >= ggd->numActive)
                            {
                                ggd->selection = 0;
                            }
                        }
                    }
                    else if (evt.button & PB_START || evt.button & PB_SELECT)
                    {
                        ggd->pause = true;
                    }
                }
            }
            // FIXME: Sometimes cannot select rightmost urinal with this method. Needs investigating.
            /* linearTouch_t touch[2] = {0};
            getTouchLinear(touch, ARRAY_SIZE(touch));
            if (touch[0].touched)
            {
                int pos = touch[0].position;
                // bool validPos[7] = {false};
                int numValid = 0;
                for (int idx = 0; idx < ggd->numActive; idx++)
                {
                    if (!ggd->toilets[idx].npc.active)
                    {
                        // validPos[idx] = true;
                        numValid++;
                    }
                }
                int section    = 1024 / numValid;
                int option     = pos / section;
                ggd->selection = 0;
                while (option > 0)
                {
                    ggd->selection++;
                    while (ggd->toilets[ggd->selection].npc.active != 0)
                    {
                        ggd->selection++;
                    }
                    option--;
                }
                while (ggd->toilets[ggd->selection].npc.active != 0)
                {
                    ggd->selection++;
                }
            } */
            // Timer
            if (ggd->pause)
            {
                drawPause();
            }
            else
            {
                ggd->timer += elapsedUs;
                if (ggd->timer >= ggd->loseTimerMax)
                {
                    ggd->lose      = true;
                    ggd->stallUsed = false;
                    ggd->state     = GG_CHOICE;
                    ggd->timer     = 0;
                    ggd->timeOut   = true;
                }
                drawGame(true);
            }

            break;
        }
        case GG_CHOICE:
        {
            while (checkButtonQueueWrapper(&evt))
            {
                // Speed up timer
                ggd->timer += SECOND;
            }
            ggd->timer += elapsedUs;
            if (ggd->timer >= SOLUTION_TIMER)
            {
                end(ggd->lose, ggd->stallUsed);
            }
            drawGame(false);
            drawSolution();
            break;
        }
        case GG_WIN:
        {
            while (checkButtonQueueWrapper(&evt))
            {
                if (evt.down)
                {
                    ggd->state = GG_READY;
                    ggd->timer = 0;
                }
            }
            drawWin();
            break;
        }
        case GG_LOSE:
        {
            while (checkButtonQueueWrapper(&evt))
            {
                if (evt.down)
                {
                    ggd->selection = GG_PLAY_GAME;
                    ggd->state     = GG_MENU;
                }
            }
            drawLose();
            break;
        }
        default:
        {
            while (checkButtonQueueWrapper(&evt))
            {
                // Allows for backing out of the mode.
            }
            break;
        }
    }
}

static void clearToilets()
{
    for (int idx = 0; idx < MAX_TOILETS; idx++)
    {
        ggd->toilets[idx].autoFlush    = 0;
        ggd->toilets[idx].graffiti     = 0;
        ggd->toilets[idx].cracks       = 0;
        ggd->toilets[idx].waterLeak    = 0;
        ggd->toilets[idx].brokenDrain  = 0;
        ggd->toilets[idx].outOfOrder   = 0;
        ggd->toilets[idx].brokenBowl   = 0;
        ggd->toilets[idx].pluggedDrain = 0;
        ggd->toilets[idx].divider      = 0;
        ggd->toilets[idx].small        = 0;
        ggd->toilets[idx].height       = URINAL_DEFAULT_H;
        ggd->toilets[idx].puddle       = GG_PEE_NONE;
        initNPC(&ggd->toilets[idx], 0);
    }
}

static void initNPC(ggToilet_t* t, bool active)
{
    if (!active)
    {
        t->npc.active = 0;
    }
    else
    {
        t->npc.active = 1;
    }
    t->npc.small      = 0;
    t->npc.shirt      = 1;
    t->npc.pants      = GG_PANTS;
    t->npc.shoes      = 1;
    t->npc.stink      = 0;
    t->npc.skinColor  = 0;
    t->npc.shirtColor = 0;
    t->npc.pantsColor = 0;
    t->npc.shoeColor  = 0;
    t->npc.randOffset = 0;
}

static void gameInit()
{
    // Reset vars
    ggd->timer = 0;
    // Timer
    ggd->loseTimerMax -= ggd->loseTimerMax / TIMER_CHANGE;
    // Max number of toilets
    ggd->numActive = 3 + (esp_random() % (MIN(ggd->numGames / 4, 4) + 1));
    clearToilets();
    int points = ggd->numGames;
    // Attempt to add NPC
    int maxNPCs = ggd->numActive - 2;
    for (int idx = 0; idx < maxNPCs; idx++)
    {
        if (points < 3)
        {
            continue;
        }
        else
        {
            int chance = esp_random() % 3; // 67/33 chance to appear
            if (chance != 0)
            {
                points -= 3;
                chance = esp_random() % ggd->numActive;
                initRandomNPC(&ggd->toilets[chance]);
            }
            // Chance to change attributes
            if (points > 0 && esp_random() % 2 == 0)
            {
                points--;
                switch (esp_random() % 6)
                {
                    case 0:
                    {
                        ggd->toilets[chance].npc.shirt = 0;
                        break;
                    }
                    case 1:
                    {
                        ggd->toilets[chance].npc.pants = GG_NAKED;
                        break;
                    }
                    case 2:
                    {
                        ggd->toilets[chance].npc.pants = GG_DOWN;
                        break;
                    }
                    case 3:
                    {
                        ggd->toilets[chance].npc.pants = GG_UNDERWEAR;
                        break;
                    }
                    case 4:
                    {
                        ggd->toilets[chance].npc.shoes = 0;
                        break;
                    }
                    case 5:
                    {
                        ggd->toilets[chance].npc.stink = 1;
                        break;
                    }
                }
            }
        }
    }
    // Attempt to modify partitions
    for (int idx = 0; idx < (ggd->numActive / 2); idx++)
    {
        if (points > 1 && esp_random() % 3 == 0)
        {
            int urinal                   = esp_random() % ggd->numActive;
            ggd->toilets[urinal].divider = 1 + (esp_random() % 3);
            if (ggd->toilets[urinal].divider == GG_DIV_NONE)
            {
                points--;
            }
            points -= 2;
        }
    }
    // Attempt to make puddles
    for (int idx = 0; idx < (ggd->numActive / 2); idx++)
    {
        if (points > 1 && esp_random() % 3 == 0)
        {
            ggd->toilets[esp_random() % ggd->numActive].puddle = 1 + (esp_random() % 3);
            points -= 2;
        }
    }
    // Toilet size
    if (points > 2)
    {
        points--;
        ggd->toilets[esp_random() % ggd->numActive].small = 1;
    }
    if (esp_random() % 100 == 0)
    {
        ggd->toilets[esp_random() % ggd->numActive].height += 15;
    }
    // Adjust toilets
    while (points > 0)
    {
        if (points >= 2 && esp_random() % 2 == 0)
        {
            points -= 2;
            int urinal = esp_random() % ggd->numActive;
            switch (esp_random() % 4)
            {
                case 0:
                {
                    ggd->toilets[urinal].brokenBowl = 1;
                    break;
                }
                case 1:
                {
                    ggd->toilets[urinal].brokenDrain = 1;
                    break;
                }
                case 2:
                {
                    ggd->toilets[urinal].pluggedDrain = 1;
                    break;
                }
                case 3:
                {
                    ggd->toilets[urinal].outOfOrder = 1;
                    break;
                }
            }
        }
        else if (esp_random() % 2 == 0)
        {
            int urinal = esp_random() % ggd->numActive;
            int result = esp_random() % 11;
            switch (result)
            {
                default:
                {
                    ggd->toilets[urinal].graffiti = result;
                    break;
                }
                case 7:
                {
                    ggd->toilets[urinal].waterLeak = 1;
                    break;
                }
                case 8:
                {
                    ggd->toilets[urinal].cracks = 1;
                    break;
                }
                case 9:
                {
                    ggd->toilets[urinal].cracks = 2;
                    break;
                }
                case 10:
                {
                    ggd->toilets[urinal].cracks = 3;
                    break;
                }
            }
        }
        else
        {
            points -= 2;
        }
    }

    // Set selection to a valid option
    ggd->selection = ggd->numActive / 2;
    while (ggd->toilets[ggd->selection].npc.active == 1)
    {
        ggd->selection++;
    }
}

static void initRandomNPC(ggToilet_t* t)
{
    t->npc.active     = 1;
    t->npc.skinColor  = esp_random() % ARRAY_SIZE(skinColors);
    t->npc.shirtColor = esp_random() % ARRAY_SIZE(shirtColors);
    t->npc.pantsColor = esp_random() % ARRAY_SIZE(pantsColors);
    t->npc.shoeColor  = esp_random() % ARRAY_SIZE(shoeColors);
    t->npc.randOffset = 8 - (esp_random() % 15);
    switch (esp_random() % 20)
    {
        case 0:
        {
            t->npc.pants = GG_SKIRT;
            break;
        }
        case 1:
        case 2:
        case 3:
        case 4:
        {
            t->npc.pants = GG_SHORTS;
        }
        default:
        {
            break;
        }
    }
}

static void end(bool lose, bool wasStall)
{
    if (lose)
    {
        ggd->state = GG_LOSE;
        return;
    }
    // Inc number of games
    ggd->numGames++;
    // Determine if the player gets another use of the stall
    if (ggd->numGames > ggd->stallInc)
    {
        ggd->stallUses++;
        ggd->stallInc *= 2;
        ggd->stallInc += ggd->stallInc / 2;
    }
    // Compute score for each urinal
    int badness[7] = {0};
    calcToiletScore(badness);
    // Find best option
    int best = badness[0];
    for (int idx = 1; idx < ggd->numActive; idx++)
    {
        if (ggd->toilets[idx].npc.active == 1)
        {
            continue;
        }
        if (best > badness[idx])
        {
            best = badness[idx];
        }
    }
    // Calc percentage
    int perc = 0;
    if (wasStall)
    {
        perc = 500;
    }
    else
    {
        perc = (1000 * (1000 - badness[ggd->selection])) / (1000 - best);
    }
    ggd->avgScore = (ggd->avgScore * (ggd->numGames - 1) + perc) / ggd->numGames;
    // Calc total score
    int timerGrace = MAX(ggd->timer, TIMER_BUFFER);
    ggd->score += perc * ((ggd->loseTimerMax + TIMER_BUFFER) - timerGrace) / ggd->loseTimerMax;
    // Calc adjusted score
    ggd->adjScore = (ggd->avgScore * ggd->score) / 1000;

    // Save to NVS
    saveToNVS();

    // Move on
    ggd->state = GG_WIN;
}

static void calcToiletScore(int* values)
{
    // Conditional
    for (int idx = 0; idx < ggd->numActive; idx++)
    {
        // Toilet
        values[idx] += (ggd->toilets[idx].autoFlush == 1) ? -10 : 0;
        values[idx] += (ggd->toilets[idx].graffiti > 0) ? 10 : 0;
        values[idx] += (ggd->toilets[idx].cracks > 0) ? 15 : 0;
        values[idx] += (ggd->toilets[idx].waterLeak == 1) ? 25 : 0;
        values[idx] += (ggd->toilets[idx].brokenBowl == 1) ? 35 : 0;
        values[idx] += (ggd->toilets[idx].brokenDrain == 1) ? 35 : 0;
        values[idx] += (ggd->toilets[idx].pluggedDrain == 1) ? 45 : 0;
        values[idx] += (ggd->toilets[idx].outOfOrder == 1) ? 30 : 0;
        values[idx] += (ggd->toilets[idx].small == 1) ? 50 : 0;
        // Divider
        switch (ggd->toilets[idx].divider)
        {
            case GG_DIV_FULL:
            {
                break;
            }
            case GG_DIV_TOP:
            case GG_DIV_BOTTOM:
            {
                values[idx] += 25;
                break;
            }
            case GG_DIV_NONE:
            {
                values[idx] += 50;
                break;
            }
        }
        if (idx != 0)
        {
            switch (ggd->toilets[idx - 1].divider)
            {
                case GG_DIV_FULL:
                {
                    break;
                }
                case GG_DIV_TOP:
                case GG_DIV_BOTTOM:
                {
                    values[idx] += 25;
                    break;
                }
                case GG_DIV_NONE:
                {
                    values[idx] += 50;
                    break;
                }
            }
        }
        // Puddle
        switch (ggd->toilets[idx].puddle)
        {
            case GG_PEE_NONE:
            {
                break;
            }
            case GG_PEE_SMALL:
            {
                values[idx] += 25;
                break;
            }
            case GG_PEE_MED:
            {
                values[idx] += 50;
                break;
            }
            case GG_PEE_LARGE:
            {
                values[idx] += 75;
                break;
            }
        }
        // NPC
        if (ggd->toilets[idx].npc.active == 1)
        {
            values[idx] += 150;
            values[idx] += (ggd->toilets[idx].npc.shirt == 0) ? 20 : 0;
            values[idx] += (ggd->toilets[idx].npc.shoes == 0) ? 30 : 0;
            values[idx] += (ggd->toilets[idx].npc.stink == 0) ? 40 : 0;

            // Specific pants
            switch (ggd->toilets[idx].npc.pants)
            {
                case GG_PANTS:
                case GG_SHORTS:
                case GG_SKIRT:
                {
                    break;
                }
                case GG_DOWN:
                {
                    values[idx] += 35;
                    break;
                }
                case GG_UNDERWEAR:
                {
                    values[idx] += 25;
                    break;
                }
                case GG_NAKED:
                {
                    values[idx] += 50;
                    break;
                }
            }
        }
    }
    /* ESP_LOGI("GG", "Scores (conditional):\n1: %d\n2: %d\n3: %d\n4: %d\n5: %d\n6: %d\n7: %d\n", values[0], values[1],
             values[2], values[3], values[4], values[5], values[6]); */
    // Positional
    int temp[7] = {0};
    for (int i = 0; i < ggd->numActive; i++)
    {
        if (i != 0)
        {
            temp[i] += values[i - 1];
        }
        if (i != ggd->numActive - 1)
        {
            temp[i] += values[i + 1];
        }
    }
    /* ESP_LOGI("GG", "Scores (Positional):\n1: %d\n2: %d\n3: %d\n4: %d\n5: %d\n6: %d\n7: %d\n", temp[0], temp[1],
       temp[2], temp[3], temp[4], temp[5], temp[6]); */
    for (int i = 0; i < ggd->numActive; i++)
    {
        values[i] += temp[i];
    }
    /* ESP_LOGI("GG", "Scores total:\n1: %d\n2: %d\n3: %d\n4: %d\n5: %d\n6: %d\n7: %d\n", values[0], values[1],
       values[2], values[3], values[4], values[5], values[6]); */
}

static void saveToNVS()
{
    int outVal;
    if (!readNamespaceNvs32(ggNVSSpace[0], ggNVSSpace[1], &outVal))
    {
        writeNamespaceNvs32(ggNVSSpace[0], ggNVSSpace[1], ggd->numGames);
    }
    else
    {
        if (ggd->numGames > outVal)
        {
            writeNamespaceNvs32(ggNVSSpace[0], ggNVSSpace[1], ggd->numGames);
        }
    }
    if (!readNamespaceNvs32(ggNVSSpace[0], ggNVSSpace[2], &outVal))
    {
        writeNamespaceNvs32(ggNVSSpace[0], ggNVSSpace[2], ggd->avgScore);
    }
    else
    {
        if (ggd->avgScore > outVal)
        {
            writeNamespaceNvs32(ggNVSSpace[0], ggNVSSpace[1], ggd->avgScore);
        }
    }
    if (!readNamespaceNvs32(ggNVSSpace[0], ggNVSSpace[3], &outVal))
    {
        writeNamespaceNvs32(ggNVSSpace[0], ggNVSSpace[3], ggd->score);
    }
    else
    {
        if (ggd->score > outVal)
        {
            writeNamespaceNvs32(ggNVSSpace[0], ggNVSSpace[3], ggd->score);
        }
    }
    if (!readNamespaceNvs32(ggNVSSpace[0], ggNVSSpace[4], &outVal))
    {
        writeNamespaceNvs32(ggNVSSpace[0], ggNVSSpace[4], ggd->adjScore);
    }
    else
    {
        if (ggd->adjScore > outVal)
        {
            writeNamespaceNvs32(ggNVSSpace[0], ggNVSSpace[4], ggd->adjScore);
        }
    }
}

// Drawing functions
// Warning
static void drawWarning(int64_t elapsedUs)
{
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);
    drawText(&ggd->normalFont, c500, strings[9], 32, 32);
    ggd->timer += elapsedUs;
    int16_t xOff = 32;
    int16_t yOff = 64;
    drawTextWordWrap(&ggd->smallFont, c555, strings[(ggd->timer >= 15000000) ? 11 : 10], &xOff, &yOff, TFT_WIDTH - 32,
                     TFT_HEIGHT);
}

// Splash
static void initSplash()
{
    clearToilets();
    ggd->numActive                 = 6;
    ggd->toilets[0].small          = 1;
    ggd->toilets[2].divider        = GG_DIV_BOTTOM;
    ggd->toilets[4].puddle         = GG_PEE_MED;
    ggd->toilets[4].outOfOrder     = 1;
    ggd->toilets[1].npc.active     = 1;
    ggd->toilets[1].npc.skinColor  = 4;
    ggd->toilets[1].npc.shirtColor = 2;
    ggd->toilets[1].npc.randOffset = 4;
    ggd->toilets[1].npc.pantsColor = 2;
    ggd->toilets[5].npc.active     = 1;
    ggd->toilets[5].npc.skinColor  = 6;
    ggd->toilets[5].npc.shirtColor = 5;
    ggd->toilets[5].npc.pantsColor = 1;
    ggd->toilets[5].npc.pants      = GG_SHORTS;
    ggd->toilets[1].npc.randOffset = -3;
}

static void drawSplash(int64_t elapsedUs)
{
    clearPxTft();
    // Draw background
    drawBackground();
    // Draw set pattern of urinals
    drawToiletArray();
    // Title
    drawTitle();
    // "Press A to start"
    ggd->timer += elapsedUs;
    if (ggd->timer >= SPLASH_US)
    {
        ggd->splashToggle = !ggd->splashToggle;
        ggd->timer        = 0;
    }
    if (!ggd->splashToggle)
    {
        int x = (TFT_WIDTH - textWidth(&ggd->normalFont, strings[0])) / 2;
        drawText(&ggd->normalFont, c555, strings[0], x, SPLASH_TEXT_Y);
        drawText(&ggd->normalFontOutline, c000, strings[0], x, SPLASH_TEXT_Y);
    }
}

static void drawTitle()
{
    int stdSpacing = SPLASH_TEXT_BUFFER + ggd->splashImgs[0].w;
    drawWsgSimple(&ggd->splashImgs[0], SPLASH_TEXT_X + stdSpacing * 0, SPLASH_TEXT_Y_2);
    drawWsgSimple(&ggd->splashImgs[5], SPLASH_TEXT_X + stdSpacing * 1, SPLASH_TEXT_Y_2 - 1);
    drawWsgSimple(&ggd->splashImgs[2], SPLASH_TEXT_X + stdSpacing * 2, SPLASH_TEXT_Y_2);
    drawWsgSimple(&ggd->splashImgs[2], SPLASH_TEXT_X + stdSpacing * 3, SPLASH_TEXT_Y_2);
    drawWsgSimple(&ggd->splashImgs[3], SPLASH_TEXT_X + stdSpacing * 4 - 4, SPLASH_TEXT_Y_2);
    drawWsgSimple(&ggd->splashImgs[0], SPLASH_TEXT_X + stdSpacing * 6 - 20, SPLASH_TEXT_Y_2);
    drawWsgSimple(&ggd->splashImgs[1], SPLASH_TEXT_X + stdSpacing * 7 - 20, SPLASH_TEXT_Y_2);
    drawWsgSimple(&ggd->splashImgs[4], SPLASH_TEXT_X + stdSpacing * 8 - 20, SPLASH_TEXT_Y_2);
}

// Menu
static void drawMenu()
{
    clearPxTft();
    // Draw background
    drawBackground();
    // Draw options
    for (int idx = 1; idx < 5; idx++)
    {
        drawText(&ggd->normalFont, c555, strings[idx], OPTION_X, OPTION_BUFFER + idx * OPTION_SPACING);
        drawText(&ggd->normalFontOutline, c000, strings[idx], OPTION_X, OPTION_BUFFER + idx * OPTION_SPACING);
    }
    // Draw selection
    int xOff = OPTION_X + textWidth(&ggd->normalFont, strings[ggd->selection + 1]) + 10;
    int yOff = OPTION_BUFFER + (ggd->selection + 1) * OPTION_SPACING - 6;
    drawWsgSimple(&ggd->uiImages[1], xOff, yOff);
}

// Rules
static void drawRules()
{
    clearPxTft();
    // Draw background
    drawBackground();
    // Draw information
    int16_t xOff = 32;
    int16_t yOff = 32;
    drawText(&ggd->normalFont, c555, rulesText[ggd->selection * 2], RULES_X_BORDER, RULES_TITLE_Y);
    drawText(&ggd->normalFontOutline, c000, rulesText[ggd->selection * 2], RULES_X_BORDER, RULES_TITLE_Y);
    yOff += RULES_DESC_Y;
    drawTextWordWrap(&ggd->smallFont, c111, rulesText[ggd->selection * 2 + 1], &xOff, &yOff, TFT_WIDTH - RULES_X_BORDER,
                     TFT_HEIGHT);
    char buffer[32];
    snprintf(buffer, sizeof(buffer) - 1, "Page %d/%d", ggd->selection + 1, (int)ARRAY_SIZE(rulesText) / 2);
    drawText(&ggd->smallFont, c000, buffer, RULES_X_BORDER, TFT_HEIGHT - 32);
}

// High Score
static void drawHighScore()
{
    clearPxTft();
    // Draw background
    drawBackground();
    // TItle
    drawText(&ggd->normalFont, c555, strings[18], RULES_X_BORDER, RULES_TITLE_Y);
    drawText(&ggd->normalFontOutline, c000, strings[18], RULES_X_BORDER, RULES_TITLE_Y);
    // Draw high Scores based on selection page
    char buffer[36];
    int32_t outVal;
    if (!readNamespaceNvs32(ggNVSSpace[0], ggNVSSpace[1], &outVal))
    {
        outVal = 0;
    }
    snprintf(buffer, sizeof(buffer) - 1, "Highest number of games won: %d", outVal);
    drawText(&ggd->smallFont, c000, buffer, RULES_X_BORDER, 60);
    if (!readNamespaceNvs32(ggNVSSpace[0], ggNVSSpace[2], &outVal))
    {
        outVal = 0;
    }
    snprintf(buffer, sizeof(buffer) - 1, "Highest adjusted score: %d", outVal);
    drawText(&ggd->smallFont, c000, buffer, RULES_X_BORDER, 80);
    if (!readNamespaceNvs32(ggNVSSpace[0], ggNVSSpace[3], &outVal))
    {
        outVal = 0;
    }
    snprintf(buffer, sizeof(buffer) - 1, "Highest total score: %d", outVal);
    drawText(&ggd->smallFont, c000, buffer, RULES_X_BORDER, 100);
    if (!readNamespaceNvs32(ggNVSSpace[0], ggNVSSpace[4], &outVal))
    {
        outVal = 0;
    }
    snprintf(buffer, sizeof(buffer) - 1, "Highest average: %d", outVal);
    drawText(&ggd->smallFont, c000, buffer, RULES_X_BORDER, 120);
}

// Ready
static void drawReady(int64_t elapsedUs)
{
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);
    drawText(&ggd->normalFont, c555, strings[12], (TFT_WIDTH - textWidth(&ggd->normalFont, strings[12])) / 2, 100);
    drawText(&ggd->normalFont, c555, strings[13], 60, 160);
    if (ggd->timer >= SECOND)
    {
        drawText(&ggd->normalFont, c555, strings[14], 140, 160);
    }
    if (ggd->timer >= SECOND * 2)
    {
        drawText(&ggd->normalFont, c555, strings[15], 220, 160);
    }
}

// Game
static void drawGame(bool ui)
{
    drawBackground();
    drawToiletArray();
    if (ui)
    {
        drawUI();
    }
}

static void drawPause()
{
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);
    drawText(&ggd->normalFont, c555, strings[16], 32, 32);
    int16_t xOff = 32;
    int16_t yOff = 64;
    drawTextWordWrap(&ggd->smallFont, c555, strings[17], &xOff, &yOff, TFT_WIDTH - 32, TFT_HEIGHT);
}

static void drawSolution()
{
    int values[7] = {0};
    calcToiletScore(values);
    int best          = 1000;
    int bestUIPos[7]  = {0};
    int worst         = 0;
    int worstUIPos[7] = {0};
    for (int idx = 0; idx < ggd->numActive; idx++)
    {
        if (ggd->toilets[idx].npc.active == 1)
        {
            continue;
        }
        else
        {
            if (best > values[idx])
            {
                if (ggd->toilets[idx].brokenBowl == 1 || ggd->toilets[idx].brokenDrain == 1
                    || ggd->toilets[idx].outOfOrder == 1 || ggd->toilets[idx].pluggedDrain == 1)
                {
                    worstUIPos[idx] = 1;
                }
                else
                {
                    best = values[idx];
                    for (int i = 0; i < idx; i++)
                    {
                        bestUIPos[i] = 0;
                    }
                    bestUIPos[idx] = 1;
                }
            }
            else if (best == values[idx])
            {
                bestUIPos[idx] = 1;
            }
            if (worst < values[idx])
            {
                worst = values[idx];
                for (int i = 0; i < idx; i++)
                {
                    worstUIPos[i] = 0;
                }
                worstUIPos[idx] = 1;
            }
            else if (worst == values[idx])
            {
                worstUIPos[idx] = 1;
            }
        }
    }
    int xStart
        = (URINAL_MAX_WIDTH - (((ggd->numActive == MAX_TOILETS) ? URINAL_7_SPACING : URINAL_SPACING) * ggd->numActive))
          / 2;
    for (int idx = 0; idx < ggd->numActive; idx++)
    {
        if (bestUIPos[idx] == 1)
        {
            drawWsgSimpleHalf(&ggd->uiImages[3],
                              idx * ((ggd->numActive == MAX_TOILETS) ? URINAL_7_SPACING : URINAL_SPACING) + xStart - 1,
                              140);
        }
        if (worstUIPos[idx] == 1 && worstUIPos[idx] != bestUIPos[idx])
        {
            drawWsgSimpleHalf(&ggd->uiImages[2],
                              idx * ((ggd->numActive == MAX_TOILETS) ? URINAL_7_SPACING : URINAL_SPACING) + xStart - 1,
                              140);
        }
    }
    drawWsg(&ggd->uiImages[1],
            xStart + ggd->selection * ((ggd->numActive == MAX_TOILETS) ? URINAL_7_SPACING : URINAL_SPACING), 60, false,
            false, 270);
    if (ggd->stallUsed)
    {
        drawText(&ggd->normalFont, c000, strings[19], (TFT_WIDTH - textWidth(&ggd->normalFont, strings[19])) / 2, 60);
    }
    if (ggd->timeOut)
    {
        drawText(&ggd->normalFont, c000, strings[7], (TFT_WIDTH - textWidth(&ggd->normalFont, strings[7])) / 2, 60);
    }
}

// Win
static void drawWin()
{
    clearPxTft();
    // Draw background
    drawBackground();
    // Draw text
    drawText(&ggd->normalFont, c555, strings[5], 32, 44);
    drawText(&ggd->normalFontOutline, c000, strings[5], 32, 44);
    // Show score
    drawScores(32, 80);
    // Prompt button press
    int16_t xOff = 32;
    int16_t yOff = 160;
    drawTextWordWrap(&ggd->smallFont, c000, strings[6], &xOff, &yOff, TFT_WIDTH - 32, TFT_HEIGHT);
}

// Lose
static void drawLose()
{
    clearPxTft();
    // Draw background
    drawBackground();
    // Draw text
    int textIdx = 20; // Broken stall
    if (ggd->timeOut)
    {
        textIdx = 7;
    }
    else if (ggd->stallUsed)
    {
        textIdx = 21;
    }
    drawText(&ggd->normalFont, c555, strings[textIdx], (TFT_WIDTH - textWidth(&ggd->normalFont, strings[textIdx])) / 2,
             44);
    drawText(&ggd->normalFontOutline, c000, strings[textIdx],
             (TFT_WIDTH - textWidth(&ggd->normalFont, strings[textIdx])) / 2, 44);
    // Show score
    drawScores(32, 80);
    // Prompt button press
    int16_t xOff = 32;
    int16_t yOff = 160;
    drawTextWordWrap(&ggd->smallFont, c000, strings[8], &xOff, &yOff, TFT_WIDTH - 32, TFT_HEIGHT);
}

// Common draw
static void drawBackground()
{
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);
    // Draw tiles
    for (int x = 0; x < TFT_WIDTH; x++)
    {
        for (int y = 1; y < TFT_HEIGHT; y++)
        {
            drawWsgSimple(&ggd->backgroundImages[1], x * ggd->backgroundImages[1].w, y * ggd->backgroundImages[1].h);
        }
    }
    // Roof
    fillDisplayArea(0, 0, TFT_WIDTH, ggd->backgroundImages[1].h, c222);
    drawLineFast(0, ggd->backgroundImages[1].h, TFT_WIDTH, ggd->backgroundImages[1].h, c000);

    // Floor
    fillDisplayArea(0, TFT_HEIGHT - FLOOR_HEIGHT, TFT_WIDTH, TFT_HEIGHT, c445);
    drawLineFast(0, TFT_HEIGHT - FLOOR_HEIGHT, TFT_WIDTH, TFT_HEIGHT - FLOOR_HEIGHT, c000);

    // Door
    drawWsgSimple(&ggd->backgroundImages[0], TFT_WIDTH - ggd->backgroundImages[0].w,
                  TFT_HEIGHT - (ggd->backgroundImages[0].h + FLOOR_HEIGHT));
}

static void drawToiletArray()
{
    int xStart
        = (URINAL_MAX_WIDTH - (((ggd->numActive == MAX_TOILETS) ? URINAL_7_SPACING : URINAL_SPACING) * ggd->numActive))
          / 2;
    for (int idx = 0; idx < ggd->numActive; idx++)
    {
        // Urinal
        drawToilet(&ggd->toilets[idx],
                   idx * ((ggd->numActive == MAX_TOILETS) ? URINAL_7_SPACING : URINAL_SPACING) + xStart,
                   (ggd->toilets[idx].small == 1) ? URINAL_HEIGHT + SMALL_URINAL_OFFSET : URINAL_HEIGHT);
        // NPC
        drawNPC(&ggd->toilets[idx],
                idx * ((ggd->numActive == MAX_TOILETS) ? URINAL_7_SPACING : URINAL_SPACING) + xStart,
                (ggd->toilets[idx].small == 1) ? URINAL_HEIGHT + SMALL_URINAL_OFFSET : URINAL_HEIGHT);
    }
}

static void drawToilet(ggToilet_t* t, int x, int y)
{
    // Puddle - Needs to happen before toilet is generated since debris can be on top
    switch (t->puddle)
    {
        case GG_PEE_NONE:
        default:
        {
            break;
        }
        case GG_PEE_SMALL:
        {
            drawWsgSimple(&ggd->toiletImages[21], x + 6, TFT_HEIGHT - FLOOR_HEIGHT + 4);
            break;
        }
        case GG_PEE_MED:
        {
            drawWsgSimple(&ggd->toiletImages[22], x + 3, TFT_HEIGHT - FLOOR_HEIGHT + 1);
            break;
        }
        case GG_PEE_LARGE:
        {
            drawWsgSimple(&ggd->toiletImages[23], x - 2, TFT_HEIGHT - FLOOR_HEIGHT + 1);
            break;
        }
    }
    // Flush
    int flag = (t->autoFlush == 1) ? 1 : 0;
    drawWsgSimple(&ggd->toiletImages[flag], x + (ggd->toiletImages[2].w - ggd->toiletImages[flag].w) / 2,
                  y - ggd->toiletImages[flag].h);
    // Top
    drawWsgSimple(&ggd->toiletImages[2], x, y);
    // Middle
    int currHeight;
    for (currHeight = ggd->toiletImages[2].h; currHeight < t->height; currHeight++)
    {
        drawWsgSimple(&ggd->toiletImages[3], x, y + currHeight);
    }
    // Bottom
    if (t->brokenBowl == 1)
    {
        drawWsgSimple(&ggd->toiletImages[5], x, y + currHeight);
        drawWsgSimple(&ggd->toiletImages[6], x + 16, TFT_HEIGHT - FLOOR_HEIGHT + 4);
    }
    else
    {
        drawWsgSimple(&ggd->toiletImages[4], x, y + currHeight);
    }
    // Drain
    if (t->brokenDrain == 1)
    {
        drawWsgSimple(&ggd->toiletImages[8], x + (ggd->toiletImages[2].w - ggd->toiletImages[8].w) / 2,
                      y + currHeight + ggd->toiletImages[4].h);
        drawWsgSimple(&ggd->toiletImages[9], x + 5, TFT_HEIGHT - FLOOR_HEIGHT + 4);
    }
    else
    {
        drawWsgSimple(&ggd->toiletImages[7], x + (ggd->toiletImages[2].w - ggd->toiletImages[7].w) / 2,
                      y + currHeight + ggd->toiletImages[4].h);
    }
    // Graffiti
    if ((t->graffiti & 0x1) == 0x1)
    {
        drawWsgSimple(&ggd->toiletImages[10], x + 16, y + 25);
    }
    if ((t->graffiti & 0x2) == 0x2)
    {
        drawWsgSimple(&ggd->toiletImages[11], x + 5, y + 15);
    }
    if ((t->graffiti & 0x4) == 0x4)
    {
        drawWsgSimple(&ggd->toiletImages[12], x + 7, y + 5);
    }
    // Cracks
    if ((t->cracks & 0x1) == 0x1)
    {
        drawWsgSimple(&ggd->toiletImages[13], x + 3, y + 25);
    }
    if ((t->cracks & 0x2) == 0x2)
    {
        drawWsgSimple(&ggd->toiletImages[14], x + ggd->toiletImages[2].w - (ggd->toiletImages[14].w + 4), y + 8);
    }
    // Water Leak
    if (t->waterLeak == 1)
    {
        drawWsgSimple(&ggd->toiletImages[15], x + 12, y + 4);
    }
    // Plugged Drain
    if (t->pluggedDrain == 1)
    {
        drawWsgSimple(&ggd->toiletImages[16], x + 5, y + currHeight - 1);
    }
    // OOO
    if (t->outOfOrder == 1)
    {
        drawWsgSimple(&ggd->toiletImages[17], x + 5, y + 9);
    }
    // Divider
    switch (t->divider)
    {
        case GG_DIV_FULL:
        default:
        {
            int yOff = y + DIVIDER_TOP_OFFSET - ((t->small == 1) ? SMALL_URINAL_OFFSET : 0);
            drawWsgSimple(&ggd->toiletImages[18], x + DIVIDER_X_OFFSET, yOff);
            break;
        }
        case GG_DIV_TOP:
        {
            int yOff = y + DIVIDER_TOP_OFFSET - ((t->small == 1) ? SMALL_URINAL_OFFSET : 0);
            drawWsgSimple(&ggd->toiletImages[19], x + DIVIDER_X_OFFSET, yOff);
            break;
        }
        case GG_DIV_BOTTOM:
        {
            int yOff = y + DIVIDER_BOT_OFFSET - ((t->small == 1) ? SMALL_URINAL_OFFSET : 0);
            drawWsgSimple(&ggd->toiletImages[20], x + DIVIDER_X_OFFSET, yOff);
            break;
        }
        case GG_DIV_NONE:
        {
            break;
        }
    }
}

static void drawNPC(ggToilet_t* t, int x, int y)
{
    // Bail if not active
    if (!t->npc.active)
    {
        return;
    }
    // Set palette
    wsgPaletteSet(&ggd->npcPalette, c555, skinColors[t->npc.skinColor]);
    wsgPaletteSet(&ggd->npcPalette, c500, shirtColors[t->npc.shirtColor]);
    wsgPaletteSet(&ggd->npcPalette, c300, shirtAccentColors[t->npc.shirtColor]);
    wsgPaletteSet(&ggd->npcPalette, c024, pantsColors[t->npc.pantsColor]);
    wsgPaletteSet(&ggd->npcPalette, c013, pantsAccentColors[t->npc.pantsColor]);
    wsgPaletteSet(&ggd->npcPalette, c005, c555);
    if (t->npc.shoes == 1)
    {
        wsgPaletteSet(&ggd->npcPalette, c210, shoeColors[t->npc.shoeColor]);
    }
    else
    {
        wsgPaletteSet(&ggd->npcPalette, c210, skinColors[t->npc.skinColor]);
    }
    // Vars
    int xOff = x - NPC_X_OFFSET;
    int yOff = y - NPC_HEIGHT_OFFSET - ((t->small == 1) ? SMALL_URINAL_OFFSET : 0)
               + ((t->npc.small == 1) ? ggd->npcImages[1].h : 0) + t->npc.randOffset;
    // Body
    drawWsgPaletteSimple(&ggd->npcImages[0], xOff, yOff, &ggd->npcPalette);
    drawWsgPaletteSimple(&ggd->npcImages[1], xOff + LEGS_OFFSET, yOff + ggd->npcImages[0].h, &ggd->npcPalette);
    if (t->npc.small != 1)
    {
        drawWsgPaletteSimple(&ggd->npcImages[1], xOff + LEGS_OFFSET, yOff + ggd->npcImages[0].h + ggd->npcImages[1].h,
                             &ggd->npcPalette);
        drawWsgPaletteSimple(&ggd->npcImages[2], xOff + LEGS_OFFSET,
                             yOff + ggd->npcImages[0].h + 2 * ggd->npcImages[1].h, &ggd->npcPalette);
    }
    else
    {
        drawWsgPaletteSimple(&ggd->npcImages[2], xOff + LEGS_OFFSET, yOff + ggd->npcImages[0].h + ggd->npcImages[1].h,
                             &ggd->npcPalette);
    }
    // Shirts
    if (t->npc.shirt == 1)
    {
        drawWsgPaletteSimple(&ggd->npcImages[4], xOff + 1, yOff + 24, &ggd->npcPalette);
    }

    // Pants
    switch (t->npc.pants)
    {
        case GG_PANTS:
        default:
        {
            if (t->npc.small)
            {
                drawWsgPaletteSimple(&ggd->npcImages[6], x + NPC_PANTS_X_OFFSET,
                                     yOff + ggd->npcImages[0].h + NPC_PANTS_Y_HIKE, &ggd->npcPalette);
            }
            else
            {
                drawWsgPaletteSimple(&ggd->npcImages[5], x + NPC_PANTS_X_OFFSET,
                                     yOff + ggd->npcImages[0].h + NPC_PANTS_Y_HIKE, &ggd->npcPalette);
            }
            break;
        }
        case GG_SHORTS:
        {
            drawWsgPaletteSimple(&ggd->npcImages[7], x + NPC_PANTS_X_OFFSET,
                                 yOff + ggd->npcImages[0].h + NPC_PANTS_Y_HIKE, &ggd->npcPalette);
            break;
        }
        case GG_SKIRT:
        {
            drawWsgPaletteSimple(&ggd->npcImages[8], x + NPC_PANTS_X_LARGE, yOff + ggd->npcImages[0].h - 13,
                                 &ggd->npcPalette);
            break;
        }
        case GG_DOWN:
        {
            drawWsgPaletteSimple(&ggd->npcImages[9], x + NPC_PANTS_X_LARGE, yOff + ggd->npcImages[0].h + 17,
                                 &ggd->npcPalette);
            break;
        }
        case GG_UNDERWEAR:
        {
            drawWsgPaletteSimple(&ggd->npcImages[10], x + NPC_PANTS_X_OFFSET, yOff + ggd->npcImages[0].h - 10,
                                 &ggd->npcPalette);
            break;
        }
        case GG_NAKED:
        {
            break; // Nothing. Pervert.
        }
    }

    // Stink
    if (t->npc.stink == 1)
    {
        drawWsgSimple(&ggd->npcImages[3], x + 8, y - 44);
    }
}

static void drawUI()
{
    // Stalls icon
    drawWsgSimple(&ggd->uiImages[0], UI_STALL_X, UI_STALL_Y);

    // "X"
    int xStart = UI_STALL_X + ggd->uiImages[0].w + UI_SPACING;
    int yStart = UI_STALL_Y + (ggd->uiImages[0].h - UI_X_DIM) / 2 - 4;
    drawLineFast(xStart, yStart, xStart + UI_X_DIM, yStart + UI_X_DIM, c000);
    drawLineFast(xStart + UI_X_DIM, yStart, xStart, yStart + UI_X_DIM, c000);

    // Number of stall uses remaining
    char buffer[32];
    snprintf(buffer, sizeof(buffer) - 1, "%i", ggd->stallUses);
    drawText(&ggd->normalFont, c000, buffer, xStart + UI_X_DIM + UI_SPACING, yStart);

    // Prog bar
    drawRectFilled(UI_BAR_PADDING_X, UI_BAR_PADDING_Y, TFT_WIDTH - UI_BAR_PADDING_X, UI_BAR_PADDING_Y + UI_BAR_HEIGHT,
                   c111);
    drawRectFilled(UI_BAR_PADDING_X + 1, UI_BAR_PADDING_Y + 1,
                   UI_BAR_PADDING_X + 1
                       + ((ggd->timer * (TFT_WIDTH - (2 * (UI_BAR_PADDING_X + 1) + 1))) / ggd->loseTimerMax),
                   UI_BAR_PADDING_Y + UI_BAR_HEIGHT - 1, c440);

    // Timer
    long timeLeft = ggd->loseTimerMax - ggd->timer;
    snprintf(buffer, sizeof(buffer) - 1, "Time left: %ld.%03ld", timeLeft / SECOND, (timeLeft % SECOND) / 1000);
    drawText(&ggd->normalFont, c550, buffer, UI_TIMER_X, UI_TIMER_Y);

    // Draw selection
    xStart
        = (URINAL_MAX_WIDTH - (((ggd->numActive == MAX_TOILETS) ? URINAL_7_SPACING : URINAL_SPACING) * ggd->numActive))
          / 2;
    drawWsg(&ggd->uiImages[1],
            xStart + ggd->selection * ((ggd->numActive == MAX_TOILETS) ? URINAL_7_SPACING : URINAL_SPACING), 60, false,
            false, 270);
}

static void drawScores(int x, int y)
{
    char buffer[32];
    snprintf(buffer, sizeof(buffer) - 1, "Total score: %d", ggd->score);
    drawText(&ggd->smallFont, c000, buffer, x, y);
    snprintf(buffer, sizeof(buffer) - 1, "Average score: %d.%d", ggd->avgScore / 10, ggd->avgScore % 10);
    drawText(&ggd->smallFont, c000, buffer, x, y + ggd->smallFont.height + 4);
    snprintf(buffer, sizeof(buffer) - 1, "Adjusted score: %d", ggd->adjScore);
    drawText(&ggd->smallFont, c000, buffer, x, y + 2 * (ggd->smallFont.height + 4));
}