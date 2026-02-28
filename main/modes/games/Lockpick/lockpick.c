//==============================================================================
// Includes
//==============================================================================

#include "lockpick.h"

//==============================================================================
// Defines
//==============================================================================

//==============================================================================
// Consts
//==============================================================================

// Strings
const char lpModeName[] = "Digipicker";

// Images
static const cnfsFileIdx_t frameImgs[]   = {FRAME_LARGE_WSG, FRAME_MED_WSG, FRAME_SMALL_WSG};
static const cnfsFileIdx_t sectionImgs[] = {
    SECTION_CENTER_L_WSG, SECTION_CENTER_M_WSG, SECTION_CENTER_S_WSG, SECTION_LEFT_L_WSG,  SECTION_LEFT_M_WSG,
    SECTION_LEFT_S_WSG,   SECTION_RIGHT_L_WSG,  SECTION_RIGHT_M_WSG,  SECTION_RIGHT_S_WSG,
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

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    // Images
    wsg_t* frames;
    wsg_t* sections;
    wsg_t* ticks;

    // Overhead
    lpStateEnum_t state;
} lpData_t;

//==============================================================================
// Functions declarations
//==============================================================================

static void lpEnterMode(void);
static void lpExitMode(void);
static void lpMainLoop(int64_t elapsedUs);

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
}