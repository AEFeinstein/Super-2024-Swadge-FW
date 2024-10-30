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
 * @brief Get the entire Swadge Hero game state
 *
 * @return the entire Swadge Hero game state
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

    // Load fonts
    loadFont("ibm_vga8.font", &shv->ibm, true);
    loadFont("righteous_150.font", &shv->righteous, true);
    loadFont("rodin_eb.font", &shv->rodin, true);

    // Load icons
    const char icons[] = {'l', 'd', 'u', 'r', 'b', 'a'};
    for (int32_t i = 0; i < ARRAY_SIZE(shv->icons); i++)
    {
        char tmp[16];

        for (int32_t fIdx = 0; fIdx < NUM_NOTE_FRAMES; fIdx++)
        {
            sprintf(tmp, "sh_%c%d.wsg", icons[i], fIdx + 1);
            loadWsg(tmp, &shv->icons[i][fIdx], true);
        }

        sprintf(tmp, "sh_%co.wsg", icons[i]);
        loadWsg(tmp, &shv->outlines[i], true);

        sprintf(tmp, "sh_%cp.wsg", icons[i]);
        loadWsg(tmp, &shv->pressed[i], true);
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

    // Free all icons
    for (int32_t i = 0; i < ARRAY_SIZE(shv->icons); i++)
    {
        for (int32_t fIdx = 0; fIdx < NUM_NOTE_FRAMES; fIdx++)
        {
            freeWsg(&shv->icons[i][fIdx]);
        }
        freeWsg(&shv->outlines[i]);
        freeWsg(&shv->pressed[i]);
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
 * section of the display. Always blank the display
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
    // Blank the display area
    fillDisplayArea(x, y, x + w, y + h, c000);
}

/**
 * @brief Tear down the current Swadge Hero screen and set up the next one
 *
 * @param sh The Swadge Hero game state
 * @param newScreen The new screen to display
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

            // Free game UI data
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
 * @brief Get the NVS key for high score for a given song and difficulty
 *
 * @param songName The name of the song
 * @param difficulty The difficulty for this key
 * @param key Must be at least 8 bytes
 */
void shGetNvsKey(const char* songName, shDifficulty_t difficulty, char* key)
{
    const char dMap[] = {'e', 'm', 'h'};
    sprintf(key, "%.6s%c", songName, dMap[difficulty]);
}
