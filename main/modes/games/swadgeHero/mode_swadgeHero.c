//==============================================================================
// Includes
//==============================================================================

#include "mode_swadgeHero.h"
#include "swadgeHero_game.h"
#include "swadgeHero_gameEnd.h"
#include "swadgeHero_menu.h"

//==============================================================================
// Function Declarations
//==============================================================================

static void shEnterMode(void);
static void shExitMode(void);
static void shMainLoop(int64_t elapsedUs);
static void shBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);

//==============================================================================
// Const Variables
//==============================================================================

static const char shName[] = "Swadge Hero";

//==============================================================================
// Variables
//==============================================================================

swadgeMode_t swadgeHeroMode = {
    .modeName                 = shName,
    .wifiMode                 = ESP_NOW,
    .overrideUsb              = false,
    .usesAccelerometer        = true,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = shEnterMode,
    .fnExitMode               = shExitMode,
    .fnMainLoop               = shMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = shBackgroundDrawCallback,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL,
};

shVars_t* shv;

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief TODO doc
 *
 * @return
 */
shVars_t* getShVars(void)
{
    return shv;
}

/**
 * This function is called when this mode is started. It should initialize
 * variables and start the mode.
 */
static void shEnterMode(void)
{
    // 60FPS please
    setFrameRateUs(16667);

    // Allocate mode memory
    shv = calloc(1, sizeof(shVars_t));

    // Load a font
    loadFont("ibm_vga8.font", &shv->ibm, false);
    loadFont("righteous_150.font", &shv->righteous, false);
    loadFont("rodin_eb.font", &shv->rodin, false);

    // TODO import new blinking icons
    const char* icons[] = {"sh_left.wsg", "sh_down.wsg", "sh_up.wsg", "sh_right.wsg", "sh_b.wsg", "sh_a.wsg"};
    for (int32_t i = 0; i < ARRAY_SIZE(shv->icons); i++)
    {
        loadWsg(icons[i], &shv->icons[i], true);
    }
    loadWsg("star.wsg", &shv->star, true);

    // Show initial menu
    shChangeScreen(shv, SH_MENU);
}

/**
 * This function is called when the mode is exited. It should free any allocated memory.
 */
static void shExitMode(void)
{
    // Free the screen
    shChangeScreen(shv, SH_NONE);

    // Free the fonts
    freeFont(&shv->ibm);
    freeFont(&shv->rodin);
    freeFont(&shv->righteous);

    for (int32_t i = 0; i < ARRAY_SIZE(shv->icons); i++)
    {
        freeWsg(&shv->icons[i]);
    }
    freeWsg(&shv->star);

    // Free mode memory
    free(shv);
}

/**
 * This function is called from the main loop. It's pretty quick, but the
 * timing may be inconsistent.
 *
 * @param elapsedUs The time elapsed since the last time this function was called. Use this value to determine when
 * it's time to do things
 */
static void shMainLoop(int64_t elapsedUs)
{
    // Process button events
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        switch (shv->screen)
        {
            case SH_GAME:
            {
                shGameInput(shv, &evt);
                break;
            }
            case SH_MENU:
            {
                shMenuInput(shv, &evt);
                break;
            }
            case SH_GAME_END:
            {
                shGameEndInput(shv, &evt);
                break;
            }
            case SH_NONE:
            default:
            {
                break;
            }
        }
    }

    // Run logic and outputs
    switch (shv->screen)
    {
        case SH_GAME:
        {
            if (shRunTimers(shv, elapsedUs))
            {
                shDrawGame(shv);
            }
            break;
        }
        case SH_MENU:
        {
            shMenuDraw(shv, elapsedUs);
            break;
        }
        case SH_GAME_END:
        {
            shGameEndDraw(shv, elapsedUs);
            break;
        }
        case SH_NONE:
        default:
        {
            break;
        }
    }
}

/**
 * This function is called when the display driver wishes to update a
 * section of the display.
 *
 * @param x the x coordinate that should be updated
 * @param y the x coordinate that should be updated
 * @param w the width of the rectangle to be updated
 * @param h the height of the rectangle to be updated
 * @param up update number
 * @param numUp update number denominator
 */
static void shBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
    // fillDisplayArea(x, y, x + w, y + h, c555);
}

/**
 * @brief TODO doc
 *
 * @param sh
 * @param newScreen
 */
void shChangeScreen(shVars_t* sh, shScreen_t newScreen)
{
    // Cleanup
    switch (sh->screen)
    {
        case SH_MENU:
        {
            shTeardownMenu(shv);
            break;
        }
        case SH_GAME:
        {
            // Free MIDI data
            globalMidiPlayerStop(true);
            unloadMidiFile(&shv->midiSong);

            // Free chart data
            free(shv->chartNotes);

            // Free UI data
            void* val;
            while ((val = pop(&shv->gameNotes)))
            {
                free(val);
            }
            while ((val = pop(&shv->fretLines)))
            {
                free(val);
            }
            while ((val = pop(&shv->starList)))
            {
                free(val);
            }
            break;
        }
        case SH_GAME_END:
        {
            // Free fail samples
            clear(&shv->failSamples);
            break;
        }
        case SH_NONE:
        default:
        {
            // Nothing tear down
            break;
        }
    }

    sh->screen = newScreen;

    // Setup
    switch (sh->screen)
    {
        case SH_GAME:
        {
            // Load the chart data
            shLoadSong(sh, sh->menuSong, sh->difficulty);
            break;
        }
        case SH_MENU:
        {
            shSetupMenu(shv);
            break;
        }
        case SH_GAME_END:
        {
            // Nothing to set up
            break;
        }
        case SH_NONE:
        default:
        {
            // Clear this on exit, just in case
            clear(&shv->failSamples);
            break;
        }
    }
}

/**
 * @brief TODO doc
 *
 * @param songName
 * @param difficulty
 * @param key Must be at least 9 bytes
 */
void shGetNvsKey(const char* songName, shDifficulty_t difficulty, char* key)
{
    int32_t toCopy = MIN(strlen(songName), 7);
    memcpy(key, songName, toCopy);

    char dChar;
    switch (difficulty)
    {
        default:
        case SH_EASY:
        {
            dChar = 'e';
            break;
        }
        case SH_MEDIUM:
        {
            dChar = 'm';
            break;
        }
        case SH_HARD:
        {
            dChar = 'h';
            break;
        }
    }
    key[toCopy]     = dChar;
    key[toCopy + 1] = 0;
}
