//==============================================================================
// Includes
//==============================================================================

#include "gottaGo.h"

//==============================================================================
// Defines
//==============================================================================

// Main
#define FRAME_RATE_US 16667

// Splash
#define SPLASH_US          500000
#define SPLASH_TEXT_Y      210
#define SPLASH_TEXT_BUFFER 2
#define SPLASH_TEXT_X      44
#define SPLASH_TEXT_Y_2    44

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
#define URINAL_HEIGHT       120
#define URINAL_MAX_WIDTH    259

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
const char ggModeName[]            = "Gotta Go!";
static const char* const strings[] = {
    "Press 'A' to start!",
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
};

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    GG_SPLASH,
    GG_MENU,
    GG_RULES,
    GG_GAME,
    GG_HIGHSCORE,
} ggState_t;

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

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    int height; // Height of actual unit (default: 35)
    uint autoFlush    : 1;
    uint graffiti     : 3;
    uint cracks       : 2;
    uint waterLeak    : 1;
    uint brokenDrain  : 1;
    uint outOfOrder   : 1;
    uint brokenBowl   : 1;
    uint pluggedDrain : 1;
    uint puddle       : 2;
    uint divider      : 2;
    uint small        : 1;
} ggToilet_t;

typedef struct
{
    // Main
    ggState_t state;
    font_t normalFont;
    font_t normalFontOutline;

    // Splash
    int64_t attractTimer;
    bool splashToggle;
    wsg_t* splashImgs;

    // Bathroom
    wsg_t* backgroundImages;
    wsg_t* toiletImages;
    int numActive;
    ggToilet_t toilets[MAX_TOILETS]; // Based on physical width

    // UI
    wsg_t* uiImages;
    int64_t loseTimer;
    int64_t loseTimerMax;
    int stallUses;
} ggData_t;

//==============================================================================
// Functions declarations
//==============================================================================

// Main
static void ggEnterMode(void);
static void ggExitMode(void);
static void ggMainLoop(int64_t elapsedUs);

// Splash
static void initSplash(void);
static void drawSplash(int64_t elapsedUs);
void drawTitle(void);

// Common draw
void drawBackground(void);
void drawToiletArray(void);
void drawToilet(ggToilet_t* t, int x, int y);
void drawUI(void);

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
};

ggData_t* ggd;

//==============================================================================
// Functions
//==============================================================================

// Main
static void ggEnterMode(void)
{
    // Initialization
    setFrameRateUs(FRAME_RATE_US);

    // Loading resources
    ggd = (ggData_t*)heap_caps_calloc(1, sizeof(ggData_t), MALLOC_CAP_8BIT);
    // Font
    loadFont(RADIOSTARS_FONT, &ggd->normalFont, true); // FIXME: Need a better font
    makeOutlineFont(&ggd->normalFont, &ggd->normalFontOutline, true);
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

    // Initialize Splash
    initSplash();

    // FIXME: Test values
    ggd->loseTimerMax = 1000000;
    ggd->loseTimer    = 333000;
}

static void ggExitMode(void)
{
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
    freeFont(&ggd->normalFontOutline);
    freeFont(&ggd->normalFont);
    free(ggd);
}

static void ggMainLoop(int64_t elapsedUs)
{
    buttonEvt_t evt;
    switch (ggd->state)
    {
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
            drawUI();
            break;
        }
        default:
        {
            while (checkButtonQueueWrapper(&evt))
            {
                // Allows for backing out of the mode.
            }
        }
    }
}

// Splash
static void initSplash()
{
    ggd->numActive               = 7;
    ggd->toilets[4].waterLeak    = 1;
    ggd->toilets[1].pluggedDrain = 1;
    ggd->toilets[0].outOfOrder   = 1;
    ggd->toilets[2].small        = 1;
    for (int idx = 0; idx < MAX_TOILETS; idx++)
    {
        ggd->toilets[idx].autoFlush   = idx % 2;
        ggd->toilets[idx].brokenBowl  = (idx >= 5) ? 1 : 0;
        ggd->toilets[idx].brokenDrain = (idx < 2) ? 1 : 0;
        ggd->toilets[idx].graffiti    = idx;
        ggd->toilets[idx].cracks      = idx % 4;
        ggd->toilets[idx].puddle      = idx % 4;
        ggd->toilets[idx].divider     = idx % 4;
        ggd->toilets[idx].height      = 35;
    }
}

static void drawSplash(int64_t elapsedUs)
{
    clearPxTft();
    // Draw background
    drawBackground();

    // Draw set pattern of urinals
    // Set urinals
    // Draw
    drawToiletArray();

    // Draw some people

    // Draw text
    // Title
    drawTitle();
    // "Press A to start"
    ggd->attractTimer += elapsedUs;
    if (ggd->attractTimer >= SPLASH_US)
    {
        ggd->splashToggle = !ggd->splashToggle;
        ggd->attractTimer = 0;
    }
    if (!ggd->splashToggle)
    {
        int x = (TFT_WIDTH - textWidth(&ggd->normalFont, strings[0])) / 2;
        drawText(&ggd->normalFont, c555, strings[0], x, SPLASH_TEXT_Y);
        drawText(&ggd->normalFontOutline, c000, strings[0], x, SPLASH_TEXT_Y);
    }
}

void drawTitle()
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

// Common draw
void drawBackground()
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

void drawToiletArray()
{
    int xStart
        = (URINAL_MAX_WIDTH - (((ggd->numActive == MAX_TOILETS) ? URINAL_7_SPACING : URINAL_SPACING) * ggd->numActive))
          / 2;
    for (int idx = 0; idx < ggd->numActive; idx++)
    {
        drawToilet(&ggd->toilets[idx],
                   idx * ((ggd->numActive == MAX_TOILETS) ? URINAL_7_SPACING : URINAL_SPACING) + xStart,
                   (ggd->toilets[idx].small == 1) ? URINAL_HEIGHT + SMALL_URINAL_OFFSET : URINAL_HEIGHT);
    }
}

void drawToilet(ggToilet_t* t, int x, int y)
{
    // Puddle - Needs to happen before toilet is generated since debris can be on top
    switch (t->puddle)
    {
        case 0:
        default:
        {
            break;
        }
        case 1:
        {
            drawWsgSimple(&ggd->toiletImages[21], x + 6, TFT_HEIGHT - FLOOR_HEIGHT + 4);
            break;
        }
        case 2:
        {
            drawWsgSimple(&ggd->toiletImages[22], x + 3, TFT_HEIGHT - FLOOR_HEIGHT + 1);
            break;
        }
        case 3:
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
    // FIXME: Doesn't work for variable length urinals (Currently works visually for h=35 to h=40)
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
        case 0:
        default:
        {
            int yOff = y + DIVIDER_TOP_OFFSET - ((t->small == 1) ? SMALL_URINAL_OFFSET : 0);
            drawWsgSimple(&ggd->toiletImages[18], x + DIVIDER_X_OFFSET, yOff);
            break;
        }
        case 1:
        {
            int yOff = y + DIVIDER_TOP_OFFSET - ((t->small == 1) ? SMALL_URINAL_OFFSET : 0);
            drawWsgSimple(&ggd->toiletImages[19], x + DIVIDER_X_OFFSET, yOff);
            break;
        }
        case 2:
        {
            int yOff = y + DIVIDER_BOT_OFFSET - ((t->small == 1) ? SMALL_URINAL_OFFSET : 0);
            drawWsgSimple(&ggd->toiletImages[20], x + DIVIDER_X_OFFSET, yOff);
            break;
        }
        case 3:
        {
            break;
        }
    }
}

void drawUI()
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
                       + ((ggd->loseTimer * (TFT_WIDTH - (2 * (UI_BAR_PADDING_X + 1) + 1))) / ggd->loseTimerMax),
                   UI_BAR_PADDING_Y + UI_BAR_HEIGHT - 1, c440);

    // Timer
    long timeLeft = ggd->loseTimerMax - ggd->loseTimer;
    snprintf(buffer, sizeof(buffer) - 1, "Time left: %ld.%03ld", timeLeft / 1000000, (timeLeft % 1000000) / 1000);
    drawText(&ggd->normalFont, c550, buffer, UI_TIMER_X, UI_TIMER_Y);
}