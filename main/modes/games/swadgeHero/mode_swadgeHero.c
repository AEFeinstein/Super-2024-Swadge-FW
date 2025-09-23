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

const char shName[] = "Swadge Hero";

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
    // Allocate mode memory
    shv = heap_caps_calloc(1, sizeof(shVars_t), MALLOC_CAP_8BIT);

    // Load fonts
    loadFont(IBM_VGA_8_FONT, &shv->ibm, true);
    loadFont(RIGHTEOUS_150_FONT, &shv->righteous, true);
    loadFont(RODIN_EB_FONT, &shv->rodin, true);

    struct
    {
        cnfsFileIdx_t f1;
        cnfsFileIdx_t f2;
        cnfsFileIdx_t outline;
        cnfsFileIdx_t pressed;
    } wsgFiles[] = {
        {
            .f1      = SH_L_1_WSG,
            .f2      = SH_L_2_WSG,
            .outline = SH_LO_WSG,
            .pressed = SH_LP_WSG,
        },
        {
            .f1      = SH_D_1_WSG,
            .f2      = SH_D_2_WSG,
            .outline = SH_DO_WSG,
            .pressed = SH_DP_WSG,
        },
        {
            .f1      = SH_U_1_WSG,
            .f2      = SH_U_2_WSG,
            .outline = SH_UO_WSG,
            .pressed = SH_UP_WSG,
        },
        {
            .f1      = SH_R_1_WSG,
            .f2      = SH_R_2_WSG,
            .outline = SH_RO_WSG,
            .pressed = SH_RP_WSG,
        },
        {
            .f1      = SH_B_1_WSG,
            .f2      = SH_B_2_WSG,
            .outline = SH_BO_WSG,
            .pressed = SH_BP_WSG,
        },
        {
            .f1      = SH_A_1_WSG,
            .f2      = SH_A_2_WSG,
            .outline = SH_AO_WSG,
            .pressed = SH_AP_WSG,
        },
    };

    for (int i = 0; i < ARRAY_SIZE(wsgFiles); i++)
    {
        loadWsg(wsgFiles[i].f1, &shv->icons[i][0], true);
        loadWsg(wsgFiles[i].f2, &shv->icons[i][1], true);
        loadWsg(wsgFiles[i].outline, &shv->outlines[i], true);
        loadWsg(wsgFiles[i].pressed, &shv->pressed[i], true);
    }

    loadWsg(STAR_WSG, &shv->star, true);

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
    heap_caps_free(shv);
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
            shTeardownGame(shv);
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
