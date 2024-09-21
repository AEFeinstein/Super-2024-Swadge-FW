/**
 * @file cg_spar.c
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief Provides the sparring implementation for Chowa Grove
 * @version 0.1
 * @date 2024-09-19
 *
 * @copyright Copyright (c) 2024
 *
 */

//==============================================================================
// Includes
//==============================================================================

#include "cg_Spar.h"
#include "cg_SparDraw.h"

//==============================================================================
// Function Declarations
//==============================================================================

static void sparMenuCb(const char* label, bool selected, uint32_t settingVal);

//==============================================================================
// Const variables
//==============================================================================

static const char sparMenuName[] = "Chowa Sparring!";

static const char* sparMenuNames[] = {"Schedule Match", "View records", "Tutorial", "Settings", "Main Menu"};

static const char* sparDojoSprites[] = {
    "Dojo_Gong.wsg",
    "Dojo_PunchingBag.wsg",
};

//==============================================================================
// Variables
//==============================================================================

cGrove_t* cg;

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initialize the spar
 *
 * @param cg Game data
 */
void cg_initSpar(cGrove_t* grove)
{
    cg = grove;
    // WSGs
    loadWsg("DojoBG.wsg", &cg->spar.dojoBG, true);
    cg->spar.dojoBGItems = calloc(ARRAY_SIZE(sparDojoSprites), sizeof(wsg_t));
    for (int32_t idx = 0; idx < ARRAY_SIZE(sparDojoSprites); idx++)
    {
        loadWsg(sparDojoSprites[idx], &cg->spar.dojoBGItems[idx], true);
    }

    // Fonts
    loadFont("righteous_150.font", &cg->spar.sparTitleFont, true);
    loadFont("eightbit_atari_grube2.font", &cg->spar.sparRegFont, true);
    makeOutlineFont(&cg->spar.sparTitleFont, &cg->spar.sparTitleFontOutline, true);

    // Init menu
    cg->spar.sparMenu = initMenu(sparMenuName, sparMenuCb);
    addSingleItemToMenu(cg->spar.sparMenu, sparMenuNames[0]); // Start a match
    addSingleItemToMenu(cg->spar.sparMenu, sparMenuNames[1]); // View combat records
    addSingleItemToMenu(cg->spar.sparMenu, sparMenuNames[2]); // View tutorial
    addSingleItemToMenu(cg->spar.sparMenu, sparMenuNames[3]); // Settings
    addSingleItemToMenu(cg->spar.sparMenu, sparMenuNames[4]); // Go back to main menu

    // TODO: New renderer
    cg->spar.renderer = initMenuManiaRenderer(NULL, NULL, NULL);

    // Load the splash screen
    cg->spar.state = CG_SPAR_SPLASH;
}

/**
 * @brief Deinit Spar
 *
 * @param cg Game data
 */
void cg_deInitSpar()
{
    // Free the menu
    deinitMenu(cg->spar.sparMenu);
    // TODO: New renderer
    deinitMenuManiaRenderer(cg->spar.renderer);

    // Fonts
    freeFont(&cg->spar.sparTitleFont);
    freeFont(&cg->spar.sparTitleFontOutline);
    freeFont(&cg->spar.sparRegFont);

    // Free assets
    freeWsg(&cg->spar.dojoBG);
    for (uint8_t i = 0; i < ARRAY_SIZE(sparDojoSprites); i++)
    {
        freeWsg(&cg->spar.dojoBGItems[i]);
    }
    free(cg->spar.dojoBGItems);
}

/**
 * @brief Runs the spar game mode
 *
 * @param cg Game data
 * @param elapsedUs Time between this frame and the last
 */
void cg_runSpar(int64_t elapsedUs)
{
    // State
    buttonEvt_t evt = {0};
    switch (cg->spar.state)
    {
        case CG_SPAR_SPLASH:
        {
            while (checkButtonQueueWrapper(&evt))
            {
                if (evt.down)
                {
                    cg->spar.state = CG_SPAR_MENU;
                }
            }
            // Draw
            cg_drawSparSplash(cg, elapsedUs);
            break;
        }
        case CG_SPAR_MENU:
        {
            // Get input
            while (checkButtonQueueWrapper(&evt))
            {
                cg->spar.sparMenu = menuButton(cg->spar.sparMenu, evt);
            }
            // TODO: new renderer
            drawMenuMania(cg->spar.sparMenu, cg->spar.renderer, elapsedUs);
            break;
        }
        case CG_SPAR_SCHEDULE:
        {
            break;
        }
        case CG_SPAR_MATCH:
        {
            break;
        }
        case CG_SPAR_BATTLE_RECORD:
        {
            while (checkButtonQueueWrapper(&evt))
            {
                if (evt.down)
                {
                    // FIXME: Scroll through records
                    switch (evt.button)
                    {
                        case PB_B:
                        {
                            cg->spar.state = CG_SPAR_MENU;
                            break;
                        }
                        default:
                        {
                            break;
                        }
                    }
                }
            }
            // Draw
            cg_drawSparRecord(cg);
            break;
        }
        case CG_SPAR_TUTORIAL:
        {
            break;
        }
        default:
        {
            // Should never hit
            break;
        }
    }

    // Manage state
    // - Schedule match (Pick from opponents, Select your Chowa)

    // Handle input in match
    // - Chowa is preparing:
    //   - L/R/U/D/A/B: Pick a move (Punch, Fast punch, Kick, Fast kick, Headbutt, Dodge)
    // - Chowa is regaining stamina:
    //   - A: Increases rate of Sta regen
    //   - B: Encourage them to stand up
    // - Start: pause
    // Tutorial is available when paused
}

static void sparMenuCb(const char* label, bool selected, uint32_t settingVal)
{
    if (selected)
    {
        if (label == sparMenuNames[0])
        {
            // Go to match setup
            cg->spar.state = CG_SPAR_SCHEDULE;
        }
        else if (label == sparMenuNames[1])
        {
            // View records
            cg->spar.state = CG_SPAR_BATTLE_RECORD;
        }
        else if (label == sparMenuNames[2])
        {
            // Tutorial
            cg->spar.state = CG_SPAR_TUTORIAL;
        }
        else if (label == sparMenuNames[3])
        {
            // Settings
        }
        else if (label == sparMenuNames[4])
        {
            // Go to main menu
            // TODO: see if I can load and unload without crashing
            cg->state      = CG_MAIN_MENU;
            cg->spar.state = CG_SPAR_SPLASH;
        }
        else
        {
            // Something went wrong
        }
    }
    // FIXME: Remove once menu works
    printf("%s %s, setting=%d\n", label, selected ? "selected" : "scrolled to", settingVal);
}