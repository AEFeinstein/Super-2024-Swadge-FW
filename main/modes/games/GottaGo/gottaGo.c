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
#define SPLASH_TEXT_Y      200
#define SPLASH_TEXT_BUFFER 2
#define SPLASH_TEXT_X      32
#define SPLASH_TEXT_Y_2    32

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
    PIPE_G_WSG, PIPE_O_WSG, PIPE_T_WSG, PIPE_A_WSG, PIPE_EXC_MARK_WSG, VALVE_O_WSG,
};
static const cnfsFileIdx_t bathroomImages[] = {
    BATHROOM_DOOR_WSG,
    BATHROOM_TILE_WSG,
};

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    SPLASH,
    MENU,
    RULES,
    GAME,
    HIGHSCORE,
} ggState_t;

//==============================================================================
// Structs
//==============================================================================

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
    wsg_t* bathroomImages;
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
    ggd->bathroomImages = heap_caps_calloc(ARRAY_SIZE(bathroomImages), sizeof(wsg_t), MALLOC_CAP_8BIT);
    for (int idx = 0; idx < ARRAY_SIZE(bathroomImages); idx++)
    {
        loadWsg(bathroomImages[idx], &ggd->bathroomImages[idx], true);
    }
}

static void ggExitMode(void)
{
    for (int idx = 0; idx < ARRAY_SIZE(bathroomImages); idx++)
    {
        freeWsg(&ggd->bathroomImages[idx]);
    }
    free(ggd->bathroomImages);
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
        case SPLASH:
        {
            while (checkButtonQueueWrapper(&evt))
            {
                if (evt.button & PB_A && evt.down)
                {
                    ggd->state = MENU;
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
            drawWsgSimple(&ggd->bathroomImages[1], x * ggd->bathroomImages[1].w, y * ggd->bathroomImages[1].h);
        }
    }
    // Roof
    fillDisplayArea(0, 0, TFT_WIDTH, ggd->bathroomImages[1].h, c222);
    drawLineFast(0, ggd->bathroomImages[1].h, TFT_WIDTH, ggd->bathroomImages[1].h, c000);

    // Floor
    fillDisplayArea(0, TFT_HEIGHT - FLOOR_HEIGHT, TFT_WIDTH, TFT_HEIGHT, c445);
    drawLineFast(0, TFT_HEIGHT - FLOOR_HEIGHT, TFT_WIDTH, TFT_HEIGHT - FLOOR_HEIGHT, c000);

    // Door
    drawWsgSimple(&ggd->bathroomImages[0], TFT_WIDTH - ggd->bathroomImages[0].w,
                  TFT_HEIGHT - (ggd->bathroomImages[0].h + FLOOR_HEIGHT));
}