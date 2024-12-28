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
#include "cg_Match.h"
#include "cg_SparDraw.h"

//==============================================================================
// Const variables
//==============================================================================

static const char sparMenuName[] = "Chowa Sparring!";

static const char* sparMenuNames[] = {"Match", "Tutorial", "Main Menu"};

static const char* attackIcons[]
    = {"Dodge-1.wsg", "Fist-1.wsg", "Fist-2.wsg", "Headbutt-1.wsg", "Kick-1.wsg", "Kick-2.wsg"};

static const int16_t sparMatchTimes[] = {30, 60, 90, 120, 999};

//==============================================================================
// Variables
//==============================================================================

cGrove_t* cg;

//==============================================================================
// Function Declarations
//==============================================================================

static void sparMenuCb(const char* label, bool selected, uint32_t settingVal);

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
    cg->spar.attackIcons = calloc(ARRAY_SIZE(attackIcons), sizeof(wsg_t));
    for (int idx = 0; idx < ARRAY_SIZE(attackIcons); idx++)
    {
        loadWsg(attackIcons[idx], &cg->spar.attackIcons[idx], true);
    }

    // Audio
    loadMidiFile("Chowa_Race.mid", &cg->spar.sparBGM, true);

    // Init menu
    cg->spar.sparMenu = initMenu(sparMenuName, sparMenuCb);
    addSingleItemToMenu(cg->spar.sparMenu, sparMenuNames[0]); // Start match
    addSingleItemToMenu(cg->spar.sparMenu, sparMenuNames[1]); // View tutorial
    addSingleItemToMenu(cg->spar.sparMenu, sparMenuNames[2]); // Go back to main menu

    // Renderer
    cg->spar.renderer = initMenuManiaRenderer(&cg->titleFont, &cg->titleFontOutline, &cg->menuFont);
    static const paletteColor_t shadowColors[] = {c001, c002, c002, c003, c013, c014, c013, c003, c002, c001};
    led_t ledColor                             = {.r = 0, .g = 200, .b = 200};
    recolorMenuManiaRenderer(cg->spar.renderer, c111, c430, c445, c045, c542, c430, c111, c445, shadowColors,
                             ARRAY_SIZE(shadowColors), ledColor);

    // Play BGM
    midiGmOn(cg->mPlayer);
    globalMidiPlayerPlaySong(&cg->spar.sparBGM, MIDI_BGM);

    // Load the splash screen
    // TODO: Load tutorial the first time mode is loaded
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
    deinitMenuManiaRenderer(cg->spar.renderer);

    // Audio
    unloadMidiFile(&cg->spar.sparBGM);

    // Free assets
    for (int idx = 0; idx < ARRAY_SIZE(attackIcons); idx++)
    {
        freeWsg(&cg->spar.attackIcons[idx]);
    }
    free(cg->spar.attackIcons);
    freeWsg(&cg->spar.dojoBG);
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
            drawMenuMania(cg->spar.sparMenu, cg->spar.renderer, elapsedUs);
            break;
        }
        case CG_MATCH_PREP:
        {
            // Handle input
            while (checkButtonQueueWrapper(&evt))
            {
                if (evt.down && cg->spar.numActiveChowa > 0)
                {
                    if (evt.button & PB_DOWN)
                    {
                        cg->spar.optionSelect++;
                        if (cg->spar.optionSelect >= 3)
                        {
                            cg->spar.optionSelect = 0;
                        }
                    }
                    else if (evt.button & PB_UP)
                    {
                        cg->spar.optionSelect--;
                        if (cg->spar.optionSelect < 0)
                        {
                            cg->spar.optionSelect = 2;
                        }
                    }
                    else if (evt.button & PB_RIGHT)
                    {
                        switch (cg->spar.optionSelect)
                        {
                            case 0:
                            {
                                cg->spar.chowaSelect++;
                                if (cg->spar.chowaSelect >= cg->spar.numActiveChowa)
                                {
                                    cg->spar.chowaSelect = 0;
                                }
                                break;
                            }
                            case 1:
                            {
                                cg->spar.aiSelect++;
                                if (cg->spar.aiSelect >= CG_NUM_DIFF)
                                {
                                    cg->spar.aiSelect = 0;
                                }
                                break;
                            }
                            case 2:
                            {
                                cg->spar.timerSelect++;
                                if (cg->spar.timerSelect >= ARRAY_SIZE(sparMatchTimes))
                                {
                                    cg->spar.timerSelect = 0;
                                }
                                break;
                            }
                            default:
                            {
                                break;
                            }
                        }
                    }
                    else if (evt.button & PB_LEFT)
                    {
                        switch (cg->spar.optionSelect)
                        {
                            case 0:
                            {
                                cg->spar.chowaSelect--;
                                if (cg->spar.chowaSelect < 0)
                                {
                                    cg->spar.chowaSelect = cg->spar.numActiveChowa - 1;
                                }
                                break;
                            }
                            case 1:
                            {
                                cg->spar.aiSelect--;
                                if (cg->spar.aiSelect < 0)
                                {
                                    cg->spar.aiSelect = CG_NUM_DIFF - 1;
                                }
                                break;
                            }
                            case 2:
                            {
                                cg->spar.timerSelect--;
                                if (cg->spar.timerSelect < 0)
                                {
                                    cg->spar.timerSelect = ARRAY_SIZE(sparMatchTimes) - 1;
                                }
                                break;
                            }
                            default:
                            {
                                break;
                            }
                        }
                    }
                    else if (evt.button & PB_A)
                    {
                        // Starts match
                        strcpy(cg->spar.match.data.matchTitle, "Random Match");
                        // TODO: Make Chowa Opponent
                        /* cg->spar.match.data.chowa[1]->stats[CG_SPEED]    = 128;
                        cg->spar.match.data.chowa[1]->stats[CG_STAMINA]  = 128;
                        cg->spar.match.data.chowa[1]->stats[CG_STRENGTH] = 0;
                        cg->spar.match.data.chowa[1]->stats[CG_AGILITY]  = 128;
                        cg->spar.match.data.chowa[1]->stats[CG_HEALTH]   = 128;
                        cg->spar.match.data.chowa[1]->playerAffinity     = 255;
                        cg->spar.match.data.chowa[1]->type               = CG_KING_DONUT;
                        cg->spar.match.data.chowa[1]->age                = i * 100; */
                        cg->spar.match.data.chowa[0] = &cg->chowa[cg->spar.activeChowaIdxs[cg->spar.chowaSelect]];
                        cg->spar.match.data.chowa[1] = &cg->spar.opponent;
                        cg_initSparMatch(cg, 0, sparMatchTimes[cg->spar.timerSelect], cg->spar.aiSelect,
                                         &cg->chowa[cg->spar.activeChowaIdxs[cg->spar.chowaSelect]]);
                        cg->spar.state = CG_SPAR_MATCH;
                    }
                    else if (evt.button & PB_B)
                    {
                        cg->spar.state = CG_SPAR_MENU;
                    }
                }
                else if (evt.down)
                {
                    cg->spar.state = CG_SPAR_MENU;
                }
            }
            // Draw prep screen
            cg_drawSparMatchPrep(cg);
            break;
        }
        case CG_SPAR_MATCH:
        {
            cg_runSparMatch(cg, elapsedUs);
            cg_drawSparMatch(cg, elapsedUs);
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
}

//==============================================================================
// Static Functions
//==============================================================================

static void sparMenuCb(const char* label, bool selected, uint32_t settingVal)
{
    if (selected)
    {
        if (label == sparMenuNames[0])
        {
            // Clear old data out
            for (int idx = 0; idx < 3; idx++)
            {
                cg->spar.match.data.timer[idx]  = 0;
                cg->spar.match.data.result[idx] = CG_DRAW;
            }

            // Generate List of active Chowa
            cg->spar.numActiveChowa = 0; // Reset
            for (int idx = 0; idx < CG_MAX_CHOWA; idx++)
            {
                if (cg->chowa[idx].active)
                {
                    cg->spar.activeChowaIdxs[cg->spar.numActiveChowa] = idx;
                    strcpy(cg->spar.activeChowaNames[cg->spar.numActiveChowa], cg->chowa[idx].name);
                    cg->spar.numActiveChowa++;
                }
            }

            // Allow player to choose their Chowa and difficulty
            cg->spar.state = CG_MATCH_PREP;
        }
        else if (label == sparMenuNames[1])
        {
            // Tutorial
            cg->spar.state = CG_SPAR_TUTORIAL;
        }
        else if (label == sparMenuNames[2])
        {
            // Go to main menu
            cg->unload = true;
            globalMidiPlayerStop(true);
            globalMidiPlayerPlaySong(&cg->menuBGM, MIDI_BGM);
            cg->spar.state = CG_SPAR_SPLASH;
        }
    }
}