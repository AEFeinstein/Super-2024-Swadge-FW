//==============================================================================
// Includes
//==============================================================================

#include "swadgeIt.h"
#include "mainMenu.h"
#include "heatshrink_helper.h"

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    SI_MENU,
    SI_REACTION,
    SI_MEMORY,
    SI_HIGH_SCORES,
} swadgeItScreen_t;

typedef enum
{
    EVT_BOP_IT,
    EVT_FLICK_IT,
    EVT_PULL_IT,
    EVT_SPIN_IT,
    EVT_TWIST_IT,
    MAX_NUM_EVTs,
} swadgeItEvt_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    uint8_t* samples;
    uint32_t len;
} rawSample_t;

typedef struct
{
    menu_t* menu;
    menuManiaRenderer_t* menuRenderer;
    swadgeItScreen_t screen;

    rawSample_t sfx[MAX_NUM_EVTs];

    uint32_t sampleIdx;

    uint32_t timeToNextEvent;
    uint32_t nextEvtTimer;
    uint32_t currentEvt;
} swadgeIt_t;

//==============================================================================
// Function Declarations
//==============================================================================

static void swadgeItEnterMode(void);
static void swadgeItExitMode(void);
static void swadgeItMainLoop(int64_t elapsedUs);
static void swadgeItBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);
static void swadgeItDacCallback(uint8_t* samples, int16_t len);

static void swadgeItMenuCb(const char* label, bool selected, uint32_t value);

//==============================================================================
// Const data
//==============================================================================

static const char swadgeItStrName[]       = "Swadge It!";
static const char swadgeItStrReaction[]   = "Reaction";
static const char swadgeItStrMemory[]     = "Memory";
static const char swadgeItStrHighScores[] = "High Scores";
static const char swadgeItStrExit[]       = "Exit";

/** Must match order of swadgeItEvt_t */
const char* swadgeItSfxFiles[MAX_NUM_EVTs] = {
    "bopit.raw", "flickit.raw", "pullit.raw", "spinit.raw", "twistit.raw",
};

const swadgeMode_t swadgeItMode = {
    .modeName                 = swadgeItStrName,
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = false,
    .usesAccelerometer        = true,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = swadgeItEnterMode,
    .fnExitMode               = swadgeItExitMode,
    .fnMainLoop               = swadgeItMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = swadgeItBackgroundDrawCallback,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL,
    .fnDacCb                  = swadgeItDacCallback,
};

//==============================================================================
// Variables
//==============================================================================

static swadgeIt_t* si;

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief This function is called when this mode is started. It should initialize variables and start the mode.
 */
static void swadgeItEnterMode(void)
{
    // Allocate mode memory
    si = heap_caps_calloc(1, sizeof(swadgeIt_t), MALLOC_CAP_8BIT);

    // Allocate menu
    si->menu = initMenu(swadgeItStrName, swadgeItMenuCb);
    addSingleItemToMenu(si->menu, swadgeItStrReaction);
    addSingleItemToMenu(si->menu, swadgeItStrMemory);
    addSingleItemToMenu(si->menu, swadgeItStrHighScores);
    addSingleItemToMenu(si->menu, swadgeItStrExit);
    si->menuRenderer = initMenuManiaRenderer(NULL, NULL, NULL);

    for (uint8_t i = 0; i < ARRAY_SIZE(si->sfx); i++)
    {
        si->sfx[i].samples = readHeatshrinkFile(swadgeItSfxFiles[i], &si->sfx[i].len, true);
    }

    si->currentEvt = MAX_NUM_EVTs;
}

/**
 * @brief This function is called when the mode is exited. It should free any allocated memory.
 */
static void swadgeItExitMode(void)
{
    // Free menu
    deinitMenuManiaRenderer(si->menuRenderer);
    deinitMenu(si->menu);

    for (uint8_t i = 0; i < ARRAY_SIZE(si->sfx); i++)
    {
        heap_caps_free(si->sfx[i].samples);
    }

    // Free mode memory
    heap_caps_free(si);
}

/**
 * @brief This function is called from the main loop. It's pretty quick, but the timing may be inconsistent.
 *
 * @param elapsedUs The time elapsed since the last time this function was called. Use this value to determine when
 * it's time to do things
 */
static void swadgeItMainLoop(int64_t elapsedUs)
{
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        switch (si->screen)
        {
            default:
            case SI_MENU:
            {
                si->menu = menuButton(si->menu, evt);
                break;
            }
            case SI_REACTION:
            {
                // TODO gameplay logic
                if (evt.down)
                {
                    si->screen = SI_MENU;
                }
                break;
            }
            case SI_MEMORY:
            {
                // TODO gameplay logic
                if (evt.down)
                {
                    si->screen = SI_MENU;
                }
                break;
            }
            case SI_HIGH_SCORES:
            {
                if (evt.down)
                {
                    si->screen = SI_MENU;
                }
                break;
            }
        }
    }

    switch (si->screen)
    {
        default:
        case SI_MENU:
        {
            drawMenuMania(si->menu, si->menuRenderer, elapsedUs);
            break;
        }
        case SI_REACTION:
        {
            // TODO gameplay logic
            RUN_TIMER_EVERY(si->nextEvtTimer, si->timeToNextEvent, elapsedUs, {
                // Pick a new event and reset the sample count
                si->currentEvt = esp_random() % MAX_NUM_EVTs;
                si->sampleIdx  = 0;

                // Decrement the time between actions
                if (si->timeToNextEvent > 500000)
                {
                    si->timeToNextEvent -= 100000;
                }
            });
            break;
        }
        case SI_MEMORY:
        {
            // TODO gameplay logic
            break;
        }
        case SI_HIGH_SCORES:
        {
            // TODO high score rendering
            break;
        }
    }
}

/**
 * @brief This function is called when the display driver wishes to update a section of the display.
 *
 * @param x the x coordinate that should be updated
 * @param y the x coordinate that should be updated
 * @param w the width of the rectangle to be updated
 * @param h the height of the rectangle to be updated
 * @param up update number
 * @param upNum update number denominator
 */
static void swadgeItBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
    switch (si->screen)
    {
        default:
        case SI_MENU:
        {
            break;
        }
        case SI_REACTION:
        {
            // TODO gameplay rendering
            fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c123);
            break;
        }
        case SI_MEMORY:
        {
            // TODO gameplay rendering
            fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c321);
            break;
        }
        case SI_HIGH_SCORES:
        {
            // TODO high score rendering
            fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c132);
            break;
        }
    }
}

/**
 * @brief This function is called to fill sample buffers for the DAC. If this is NULL, then
 * globalMidiPlayerFillBuffer() will be used instead to fill sample buffers
 *
 * @param samples The sample buffer to fill
 * @param len The number of samples to fill
 */
static void swadgeItDacCallback(uint8_t* samples, int16_t len)
{
    if (si->currentEvt < MAX_NUM_EVTs)
    {
        rawSample_t* rs = &si->sfx[si->currentEvt];
        if (rs->samples)
        {
            // Make sure we don't read out of bounds
            int16_t cpLen = len;
            if (si->sampleIdx + len > rs->len)
            {
                cpLen = (rs->len - si->sampleIdx);
            }

            // Copy samples out
            memcpy(samples, &rs->samples[si->sampleIdx], cpLen);
            si->sampleIdx += cpLen;

            // Increment samples and decrement len
            samples += cpLen;
            len -= cpLen;
        }
    }

    // If there's anything else to write
    if (len)
    {
        // Write blanks
        memset(samples, 127, len);
    }
}

/**
 * @brief A callback which is called when a menu changes or items are selected
 * @param label A pointer to the label which was selected or scrolled to
 * @param selected true if the item was selected with the A button, false if it was scrolled to
 * @param value If a settings item was selected or scrolled, this is the new value for the setting
 */
static void swadgeItMenuCb(const char* label, bool selected, uint32_t value)
{
    if (selected)
    {
        if (swadgeItStrReaction == label)
        {
            si->screen = SI_REACTION;

            si->timeToNextEvent = 2000000;
            si->nextEvtTimer    = 0;
            si->currentEvt      = MAX_NUM_EVTs;
        }
        else if (swadgeItStrMemory == label)
        {
            si->screen = SI_MEMORY;
        }
        else if (swadgeItStrHighScores == label)
        {
            si->screen = SI_HIGH_SCORES;
        }
        else if (swadgeItStrExit == label)
        {
            // Exit to the main menu
            switchToSwadgeMode(&mainMenuMode);
        }
    }
}
