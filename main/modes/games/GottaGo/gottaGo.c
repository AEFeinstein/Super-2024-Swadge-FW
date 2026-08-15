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
    GG_FLUSH_HANDLE_WSG,  GG_FLUSH_AUTO_WSG,      GG_TOILET_TOP_WSG,
    GG_TOILET_STRIP_WSG,  GG_TOILET_BOTTOM_WSG,   GG_TOILET_BOTTOM_BROKEN_WSG,
    GG_DOWNPIPE_WSG,      GG_DOWNPIPE_BROKEN_WSG, GG_DOWNPIPE_BROKEN_2_WSG,
    GG_DIVIDER_WSG,       GG_DIVIDER_TOP_WSG,     GG_DIVIDER_BOTTOM_WSG,
    GG_CRACK_1_WSG,       GG_CRACK_2_WSG,         GG_GRAFFITI_1_WSG,
    GG_GRAFFITI_2_WSG,    GG_GRAFFITI_3_WSG,      GG_OUT_OF_ORDER_WSG,
    GG_PLUGGED_DRAIN_WSG, GG_WATER_LEAK_WSG,      GG_PEE_S_WSG,
    GG_PEE_M_WSG,         GG_PEE_L_WSG,
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
    int heightOffFloor;       // Height above floor
    int height;               // Height of actual unit (default: 35)
    bool autoFlush;           // Autoflush or manual
    bool mildBreak;           // Graffiti, Cracks, Water flowing
    bool hardBreak;           // Drainpipe, Out-Of-Order, Broken Bowl, Not draining
    ggPeePool_t pp;           // Puddle on the ground (Small, Med, Large)
    ggDivider_t rightDivider; // Full, top, bottom, none
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
} ggData_t;

//==============================================================================
// Functions declarations
//==============================================================================

// Main
static void ggEnterMode(void);
static void ggExitMode(void);
static void ggMainLoop(int64_t elapsedUs);

// Splash
static void drawSplash(int64_t elapsedUs);
void drawTitle(void);

// Common draw
void drawBackground(void);
void drawToilet(ggToilet_t* t, int x, int y);

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
}

static void ggExitMode(void)
{
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
static void drawSplash(int64_t elapsedUs)
{
    clearPxTft();
    // Draw background
    drawBackground();

    // Draw set pattern of urinals

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

void drawToilet(ggToilet_t* t, int x, int y)
{
}