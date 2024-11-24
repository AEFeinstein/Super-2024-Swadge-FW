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
// Function Declarations
//==============================================================================

static void sparMenuCb(const char* label, bool selected, uint32_t settingVal);
static void sparLoadBattleRecords(void);

//==============================================================================
// Const variables
//==============================================================================

static const char sparMenuName[] = "Chowa Sparring!";

static const char* sparMenuNames[] = {"Schedule Match", "View records", "Tutorial", "Settings", "Main Menu"};

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

    // Audio
    loadMidiFile("Chowa_Battle.mid", &cg->spar.sparBGM, true);

    // Init menu
    cg->spar.sparMenu = initMenu(sparMenuName, sparMenuCb);
    addSingleItemToMenu(cg->spar.sparMenu, sparMenuNames[0]); // Start a match
    addSingleItemToMenu(cg->spar.sparMenu, sparMenuNames[1]); // View combat records
    addSingleItemToMenu(cg->spar.sparMenu, sparMenuNames[2]); // View tutorial
    addSingleItemToMenu(cg->spar.sparMenu, sparMenuNames[3]); // Settings
    addSingleItemToMenu(cg->spar.sparMenu, sparMenuNames[4]); // Go back to main menu

    cg->spar.renderer = initMenuManiaRenderer(&cg->titleFont, &cg->titleFontOutline, &cg->menuFont);
    static const paletteColor_t shadowColors[] = {c001, c002, c002, c003, c013, c014, c013, c003, c002, c001};
    led_t ledColor                             = {.r = 0, .g = 200, .b = 200};
    recolorMenuManiaRenderer(cg->spar.renderer, c111, c430, c445, c045, c542, c430, c111, c445, shadowColors,
                             ARRAY_SIZE(shadowColors), ledColor);

    // Initialize battle record
    sparLoadBattleRecords();

    // Play BGM
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

    // Free assets
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
        case CG_SPAR_SCHEDULE:
        {
            cg_drawSparMatchSetup(cg);
            // FIXME: don't immediately drop through
            if (true)
            {
                cg->spar.state = CG_SPAR_MATCH;
                cg_initSparMatch(cg, "TestMatch", &cg->chowa[0], &cg->chowa[1], 0, 1200, CG_HARD);
            }
            break;
        }
        case CG_MATCH_PREP:
        {
            // TODO: Match preview
            // Show the matchup, then handle countdown
            break;
        }
        case CG_SPAR_MATCH:
        {
            cg_runSparMatch(cg, elapsedUs);
            cg_drawSparMatch(cg, elapsedUs);
            break;
        }
        case CG_SPAR_MATCH_RESULTS:
        {
            // TODO: Show match results, save to Swadge
            // Show the final results
            break;
        }
        case CG_SPAR_BATTLE_RECORD:
        {
            while (checkButtonQueueWrapper(&evt))
            {
                if (evt.down)
                {
                    switch (evt.button)
                    {
                        case PB_RIGHT:
                        {
                            cg->spar.recordSelect++;
                            if (cg->spar.recordSelect >= CG_SPAR_MAX_RECORDS)
                            {
                                cg->spar.recordSelect = 0;
                            }
                            break;
                        }
                        case PB_LEFT:
                        {
                            cg->spar.recordSelect--;
                            if (cg->spar.recordSelect < 0)
                            {
                                cg->spar.recordSelect = CG_SPAR_MAX_RECORDS - 1;
                            }
                            break;
                        }
                        case PB_UP:
                        {
                            cg->spar.roundSelect--;
                            if (cg->spar.roundSelect < 0)
                            {
                                cg->spar.roundSelect = 2;
                            }
                            break;
                        }
                        case PB_DOWN:
                        {
                            cg->spar.roundSelect++;
                            if (cg->spar.roundSelect >= 3)
                            {
                                cg->spar.roundSelect = 0;
                            }
                            break;
                        }
                        case PB_A:
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
            cg->unload = true;
            globalMidiPlayerStop(true);
            globalMidiPlayerPlaySong(&cg->menuBGM, MIDI_BGM);
            cg->spar.state = CG_SPAR_SPLASH;
        }
        else
        {
            // Something went wrong
        }
    }
}

static void sparLoadBattleRecords()
{
    // FIXME: Load from disk
    for (int32_t idx = 0; idx < CG_SPAR_MAX_RECORDS; idx++)
    {
        char buff[32];
        snprintf(buff, sizeof(buff) - 1, "Match %" PRId32, idx);
        strcpy(cg->spar.sparRecord[idx].matchTitle, buff);
        for (int32_t i = 0; i < 2; i++)
        {
            snprintf(buff, sizeof(buff) - 1, "Player %" PRId32, i);
            strcpy(cg->spar.sparRecord[idx].playerNames[i], buff);
        }
        for (int32_t i = 0; i < 6; i++)
        {
            snprintf(buff, sizeof(buff) - 1, "TestChowa%" PRId32, i);
            strcpy(cg->spar.sparRecord[idx].chowaNames[i], buff);
            cg->spar.sparRecord[idx].colorType[i] = i;
        }
        for (int32_t i = 0; i < 3; i++)
        {
            cg->spar.sparRecord[idx].result[i] = i;
            cg->spar.sparRecord[idx].timer[i]  = (i + 1) * 50;
        }
    }
}

/**
 * @brief Initializes the high scores based either from NVS or predetermined scores to beat
 *
 */
/* static void t48InitHighScores()
{
    // Init High scores
    for (int8_t i = 0; i < T48_HS_COUNT; i++)
    {
        if (!readNvs32(highScoreKey[i], &t48->highScore[i]))
        {
            switch (i)
            {
                case 0:
                    t48->highScore[i] = 96880;
                    break;
                case 1:
                    t48->highScore[i] = 69224;
                    break;
                case 2:
                    t48->highScore[i] = 24244;
                    break;
                case 3:
                    t48->highScore[i] = 11020;
                    break;
                case 4:
                    t48->highScore[i] = 5176;
                    break;
            }
            writeNvs32(highScoreKey[i], t48->highScore[i]);
        }
        size_t len = 4;
        if (!readNvsBlob(highScoreInitialsKey[i], &t48->hsInitials[i], &len))
        {
            static char buff[5];
            switch (i)
            {
                case 0:
                    strcpy(buff, "JW");
                    break;
                case 1:
                    strcpy(buff, "Pan");
                    break;
                case 2:
                    strcpy(buff, "Pix");
                    break;
                case 3:
                    strcpy(buff, "Poe");
                    break;
                case 4:
                    strcpy(buff, "DrG");
                    break;
            }
            strcpy(t48->hsInitials[i], buff);
            writeNvsBlob(highScoreInitialsKey[i], &t48->hsInitials[i], len);
        }
    }
} */