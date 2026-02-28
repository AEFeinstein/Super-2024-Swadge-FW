//==============================================================================
// Includes
//==============================================================================

#include "lockpick.h"

#include "wsgCanvas.h"
#include "wsgPalette.h"

//==============================================================================
// Defines
//==============================================================================

// Gameplay
#define MAX_RINGS      3
#define MAX_KEYS       10
#define TOTAL_SECTIONS 12

// Pixel
#define BUFFER_EDGE 16

//==============================================================================
// Consts
//==============================================================================

// Strings
const char lpModeName[] = "Digipicker";

// Images
static const cnfsFileIdx_t frameImgs[]   = {FRAME_LARGE_WSG, FRAME_MED_WSG, FRAME_SMALL_WSG};
static const cnfsFileIdx_t sectionImgs[] = {
    SECTION_LEFT_L_WSG,  SECTION_CENTER_L_WSG, SECTION_RIGHT_L_WSG,  SECTION_LEFT_M_WSG,  SECTION_CENTER_M_WSG,
    SECTION_RIGHT_M_WSG, SECTION_LEFT_S_WSG,   SECTION_CENTER_S_WSG, SECTION_RIGHT_S_WSG,
};
static const cnfsFileIdx_t tickImgs[] = {
    KEY_RING_WSG, TICK_1_WSG, TICK_2_WSG, TICK_3_WSG,  TICK_4_WSG,  TICK_5_WSG,  TICK_6_WSG,
    TICK_7_WSG,   TICK_8_WSG, TICK_9_WSG, TICK_10_WSG, TICK_11_WSG, TICK_12_WSG,
};

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    MENU,
    GAME,
    PAUSE,
} lpStateEnum_t;

typedef enum
{
    OUTER,
    MID,
    INNER,
} layer_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    int8_t orientation; // 1-12
    int16_t sections;   // Which positions are filled
    wsg_t image;
} pick_t;

typedef struct
{
    int16_t sections;      // Pre-Filled sections
    int16_t keyedSections; // Key filled sections
    wsg_t image;
} ring_t;

typedef struct
{
    ring_t rings[MAX_RINGS]; // Max num of rings
    pick_t picks[MAX_KEYS];  // Max num of picks
} puzzle_t;

typedef struct
{
    // Images
    wsg_t* frames;
    wsg_t* sections;
    wsg_t* ticks;

    // Overhead
    lpStateEnum_t state;
    bool regen;
    wsgPalette_t pal;
    wsgPalette_t pal2;

    // Puzzle
    puzzle_t active;
} lpData_t;

//==============================================================================
// Functions declarations
//==============================================================================

static void lpEnterMode(void);
static void lpExitMode(void);
static void lpMainLoop(int64_t elapsedUs);

// Logic
static void initPuzzle(void);

// Drawing
static void drawPuzzle(void);
static void constructRingImgs(ring_t* p, layer_t layer);

//==============================================================================
// Variables
//==============================================================================

swadgeMode_t lpMode = {
    .modeName    = lpModeName,
    .fnEnterMode = lpEnterMode,
    .fnExitMode  = lpExitMode,
    .fnMainLoop  = lpMainLoop,
};

lpData_t* lp;

//==============================================================================
// Functions
//==============================================================================
static void lpEnterMode(void)
{
    // Allocate
    lp = (lpData_t*)heap_caps_calloc(1, sizeof(lpData_t), MALLOC_CAP_8BIT);
    // Images
    lp->frames = heap_caps_calloc(ARRAY_SIZE(frameImgs), sizeof(wsg_t), MALLOC_CAP_8BIT);
    for (int idx = 0; idx < ARRAY_SIZE(frameImgs); idx++)
    {
        loadWsg(frameImgs[idx], &lp->frames[idx], true);
    }
    lp->sections = heap_caps_calloc(ARRAY_SIZE(sectionImgs), sizeof(wsg_t), MALLOC_CAP_8BIT);
    for (int idx = 0; idx < ARRAY_SIZE(sectionImgs); idx++)
    {
        loadWsg(sectionImgs[idx], &lp->sections[idx], true);
    }
    lp->ticks = heap_caps_calloc(ARRAY_SIZE(tickImgs), sizeof(wsg_t), MALLOC_CAP_8BIT);
    for (int idx = 0; idx < ARRAY_SIZE(tickImgs); idx++)
    {
        loadWsg(tickImgs[idx], &lp->ticks[idx], true);
    }

    // Palette
    wsgPaletteReset(&lp->pal);
    wsgPaletteReset(&lp->pal2);
    wsgPaletteSet(&lp->pal2, c555, c500);

    // Init
    initPuzzle();
}

static void lpExitMode(void)
{
    for (int idx = 0; idx < ARRAY_SIZE(frameImgs); idx++)
    {
        freeWsg(&lp->ticks[idx]);
    }
    heap_caps_free(lp->frames);
    for (int idx = 0; idx < ARRAY_SIZE(sectionImgs); idx++)
    {
        freeWsg(&lp->sections[idx]);
    }
    heap_caps_free(lp->sections);
    for (int idx = 0; idx < ARRAY_SIZE(tickImgs); idx++)
    {
        freeWsg(&lp->ticks[idx]);
    }
    heap_caps_free(lp->ticks);
    heap_caps_free(lp);
}

static void lpMainLoop(int64_t elapsedUs)
{
    // Input
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
    }

    // Logic

    // Draw
    if (lp->regen)
    {
        lp->regen = false;
        drawPuzzle();
    }
}

// Logic

static void initPuzzle()
{
    // TEST
    lp->regen                         = true;
    lp->active.rings[0].sections      = 0x0555;
    lp->active.rings[0].keyedSections = 0x0aaa;
    lp->active.rings[1].sections      = 0x05a2;
    lp->active.rings[1].keyedSections = 0x0a0a;
    lp->active.rings[2].sections      = 0x0000;
}

// Drawing
static void drawPuzzle()
{
    // Draw background

    // Draw rings
    int comp = 0;
    for (int idx = 0; idx < MAX_RINGS; idx++)
    {
        ring_t* r = &lp->active.rings[idx];
        if ((r->sections ^ r->keyedSections) == 0x0fff)
        {
            comp++;
        }
        else if (r->sections != 0)
        {
            if (idx + comp >= MAX_RINGS)
            {
                ESP_LOGE("LP", "OOB: Sommat is wrong: %d", (idx + comp));
            }
            constructRingImgs(r, idx - comp);
            // Draw set key values
            for (int idx2 = 0; idx2 < TOTAL_SECTIONS; idx2++)
            {
                int rot = (idx2 / 3) * 90;
                if ((lp->active.rings[comp].keyedSections >> idx2) & 0x1)
                {
                    canvasDrawPal(&lp->active.rings[comp].image, sectionImgs[(idx2) % 3], 0, 0, false, false, rot,
                                  &lp->pal2);
                }
            }
            drawWsgSimple(&r->image, BUFFER_EDGE, (TFT_HEIGHT - lp->active.rings[idx].image.h) >> 1);

            // Relies on 
            drawLineFast(lp->active.rings[idx].image.w + 2 * BUFFER_EDGE, 0, lp->active.rings[idx].image.w + 2 * BUFFER_EDGE,
                 TFT_HEIGHT, c555);
        }
    }

    // Draw active key around rings

    // Draw keys

}

static void constructRingImgs(ring_t* r, layer_t layer)
{
    canvasBlankInit(&r->image, 181, 181, cTransparent, true);
    canvasDrawSimplePal(&r->image, frameImgs[layer], 0, 0, &lp->pal);
    for (int idx = 0; idx < TOTAL_SECTIONS; idx++)
    {
        int rot = (idx / 3) * 90;
        if ((r->sections >> idx) & 0x1)
        {
            canvasDrawPal(&r->image, sectionImgs[(idx) % 3 + layer * 3], 0, 0, false, false, rot, &lp->pal);
        }
    }
}