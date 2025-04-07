//==============================================================================
// Includes
//==============================================================================

#include "swadgeIt.h"
#include "mainMenu.h"

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

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    menu_t* menu;
    menuManiaRenderer_t* menuRenderer;
    swadgeItScreen_t screen;
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
}

/**
 * @brief This function is called when the mode is exited. It should free any allocated memory.
 */
static void swadgeItExitMode(void)
{
    // Free menu
    deinitMenuManiaRenderer(si->menuRenderer);
    deinitMenu(si->menu);

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
    // TODO make it speak!
    memset(samples, 127, sizeof(uint8_t) * len);
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
