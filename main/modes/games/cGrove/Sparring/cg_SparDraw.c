/**
 * @file cg_sparDraw.c
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief Draws the Chowa Garden Spar
 * @version 0.1
 * @date 2024-09-19
 *
 * @copyright Copyright (c) 2024
 *
 */

//==============================================================================
// Includes
//==============================================================================

#include "cg_SparDraw.h"

//==============================================================================
// Defines
//==============================================================================

#define STAT_BAR_WIDTH 4
#define STAT_BAR_BASE  200
#define PADDING        6

//==============================================================================
// Const Strings
//==============================================================================

static const char sparSplashScreen[]       = "Chowa Sparring";
static const char sparSplashScreenPrompt[] = "Press any button to continue!";

//==============================================================================
// Function Declarations
//==============================================================================

static void cg_drawSparField(cGrove_t* cg);
static void cg_drawSparBGObject(cGrove_t* cg, int64_t elapsedUs);
static void cg_drawSparChowa(cGrove_t* cg, int64_t elapsedUs);
static void cg_drawSparChowaUI(cGrove_t* cg);
static void cg_drawSparProgBars(cGrove_t* cg, int16_t maxVal, int16_t currVal, int16_t x, int16_t y,
                                paletteColor_t color, int8_t bitShift);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Draws the match
 *
 * @param cg Game data
 * @param elapsedUs TIme since last frame
 */
void cg_drawMatch(cGrove_t* cg, int64_t elapsedUs)
{
    // Draw dojo
    cg_drawSparField(cg);

    // Draw Objects
    cg_drawSparBGObject(cg, elapsedUs);

    // Draw Chowa
}

/**
 * @brief Draws the splash screen
 *
 * @param cg Game data
 * @param elapsedUs Time since last frame
 */
void cg_drawSparSplash(cGrove_t* cg, int64_t elapsedUs)
{
    // Draw dojo
    cg_drawSparField(cg);
    cg_drawSparBGObject(cg, elapsedUs);

    // TODO: Draw chowa sparring

    // Draw title text
    // Get the text offset
    int16_t xOff = (TFT_WIDTH - textWidth(&cg->spar.sparTitleFont, sparSplashScreen)) / 2;
    int16_t yOff = TFT_HEIGHT / 2;

    // drawText(&t48->titleFont, color, mode, (TFT_WIDTH - textWidth(&t48->titleFont, mode)) / 2, TFT_HEIGHT / 2 - 12);
    drawText(&cg->spar.sparTitleFont, c555, sparSplashScreen, xOff, yOff);
    drawText(&cg->spar.sparTitleFontOutline, c000, sparSplashScreen, xOff, yOff);

    xOff = 32;
    yOff = TFT_HEIGHT - 32;
    // Draw "Press A to continue" prompt
    cg->spar.timer += elapsedUs;
    if (cg->spar.timer >= 500000)
    {
        cg->spar.timer  = 0;
        cg->spar.toggle = !cg->spar.toggle;
    }
    if (cg->spar.toggle)
    {
        drawTextWordWrap(&cg->spar.sparRegFont, c000, sparSplashScreenPrompt, &xOff, &yOff, TFT_WIDTH - 32, TFT_HEIGHT);
    }
}

/**
 * @brief Draws the battle record
 *
 * @param cg Game data
 */
void cg_drawSparRecord(cGrove_t* cg)
{
    // Background
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);

    // Reference the record
    cgRecord_t* record = &cg->spar.sparRecord[cg->spar.recordSelect];

    // Draw Match title + Arrows
    drawText(&cg->spar.sparRegFont, c555, record->matchTitle,
             (TFT_WIDTH - textWidth(&cg->spar.sparRegFont, record->matchTitle)) / 2, 8);
    drawWsg(&cg->arrow, (TFT_WIDTH - textWidth(&cg->spar.sparRegFont, record->matchTitle)) / 2 - (4 + cg->arrow.w), 4,
            false, false, 270);
    drawWsg(&cg->arrow, (TFT_WIDTH + textWidth(&cg->spar.sparRegFont, record->matchTitle)) / 2 + 4, 4, false, false,
            90);

    // Player names + vs label
    int16_t vsStart = (TFT_WIDTH - textWidth(&cg->spar.sparRegFont, "VS")) / 2;
    drawText(&cg->spar.sparRegFont, c333, "VS", vsStart, 24);
    drawText(&cg->spar.sparRegFont, c533, record->playerNames[0],
             vsStart - (textWidth(&cg->spar.sparRegFont, record->playerNames[0]) + 16), 24);
    drawText(&cg->spar.sparRegFont, c335, record->playerNames[1], vsStart + textWidth(&cg->spar.sparRegFont, "VS") + 16,
             24);

    // Round number and arrows
    char buffer[32];
    snprintf(buffer, sizeof(buffer) - 1, "Round %d", cg->spar.roundSelect);
    drawWsgSimple(&cg->arrow, (TFT_WIDTH - cg->arrow.w) / 2, 40);
    drawText(&cg->spar.sparRegFont, c444, buffer, (TFT_WIDTH - textWidth(&cg->spar.sparRegFont, buffer)) / 2, 60);
    drawWsg(&cg->arrow, (TFT_WIDTH - cg->arrow.w) / 2, 76, false, true, 0);

    // Draw Chowa
    // TODO: Draw Chowa

    // Draw time
    snprintf(buffer, sizeof(buffer) - 1, "Time: %d", record->timer[cg->spar.roundSelect]);
    drawText(&cg->spar.sparRegFont, c444, buffer, (TFT_WIDTH - textWidth(&cg->spar.sparRegFont, buffer)) / 2, 188);

    // Draw Chowa names
    int8_t offset = cg->spar.roundSelect * 2;
    drawText(&cg->spar.sparRegFont, c444, record->chowaNames[offset], 8, 204);
    drawText(&cg->spar.sparRegFont, c444, record->chowaNames[offset + 1],
             TFT_WIDTH - (8 + textWidth(&cg->spar.sparRegFont, record->chowaNames[offset + 1])), 204);

    // Draw result of the game
    switch (record->result[cg->spar.roundSelect])
    {
        case CG_P1_WIN:
        {
            snprintf(buffer, sizeof(buffer) - 1, "Winner: %s", record->playerNames[0]);
            drawText(&cg->spar.sparRegFont, c533, buffer, (TFT_WIDTH - textWidth(&cg->spar.sparRegFont, buffer)) / 2,
                     220);
            break;
        }
        case CG_P2_WIN:
        {
            snprintf(buffer, sizeof(buffer) - 1, "Winner: %s", record->playerNames[1]);
            drawText(&cg->spar.sparRegFont, c335, buffer, (TFT_WIDTH - textWidth(&cg->spar.sparRegFont, buffer)) / 2,
                     220);
            break;
        }
        case CG_DRAW:
        {
            drawText(&cg->spar.sparRegFont, c333, "Draw", (TFT_WIDTH - textWidth(&cg->spar.sparRegFont, "Draw")) / 2,
                     220);
            break;
        }
    }
}

/**
 * @brief Draws the match initialization screen
 *
 * @param cg Game data
 */
void cg_drawSparMatchSetup(cGrove_t* cg)
{
    // Provide options for Tournament, Single match, or online

    // Secondary selection. chooses a specific tournament, match, or online.

    // Allow a player to look at the tutorial
}

/**
 * @brief Draws the match based on current state
 *
 * @param cg Game Data
 * @param elapsedUs TIme since last frame
 */
void cg_drawSparMatch(cGrove_t* cg, int64_t elapsedUs)
{
    // BG
    cg_drawSparField(cg);

    // Draw Chowa
    cg_drawSparChowa(cg, elapsedUs);

    // Draw BG objects
    cg_drawSparBGObject(cg, elapsedUs);

    // Draw UI
    // Buffer
    char buffer[32];

    // Draw match title
    snprintf(buffer, sizeof(buffer) - 1, "%s, round %d", cg->spar.match.matchName, cg->spar.match.round);
    drawText(&cg->spar.sparRegFont, c000, buffer, (TFT_WIDTH - textWidth(&cg->spar.sparRegFont, buffer)) / 2, 8);

    // Time
    paletteColor_t color = c000;
    if (cg->spar.match.maxTime <= (cg->spar.match.timer + 10))
    {
        color = c500;
    }
    snprintf(buffer, sizeof(buffer) - 1, "Time: %d", cg->spar.match.maxTime - cg->spar.match.timer);
    drawText(&cg->spar.sparRegFont, color, buffer, (TFT_WIDTH - textWidth(&cg->spar.sparRegFont, buffer)) / 2, 24);

    // Draw Chowa UI
    cg_drawSparChowaUI(cg);

    // If paused, draw pause text
    if (cg->spar.match.paused)
    {
        drawText(&cg->spar.sparTitleFont, c005, "--PAUSE--",
                 (TFT_WIDTH - textWidth(&cg->spar.sparTitleFont, "--PAUSE--")) / 2, TFT_HEIGHT / 2 - 16);
    }

    // Draw match end
    if (cg->spar.match.done)
    {
        drawText(&cg->spar.sparTitleFont, c005, "FINISHED",
                 (TFT_WIDTH - textWidth(&cg->spar.sparTitleFont, "FINISHED")) / 2, TFT_HEIGHT / 2 - 16);
    }
}

/**
 * @brief Draws the background
 *
 * @param cg Game data
 */
static void cg_drawSparField(cGrove_t* cg)
{
    // Draw dojo
    drawWsgSimple(&cg->spar.dojoBG, 0, 0);
}

/**
 * @brief Draws the background objects
 *
 * @param cg Game data
 * @param elapsedUs Time since last frame
 */
static void cg_drawSparBGObject(cGrove_t* cg, int64_t elapsedUs)
{
    // If wasCrit == true, setg false and add impulses to items
    // FIXME: Do bounce thing
    for (int32_t i = 0; i < 2; i++)
    {
        drawWsgSimple(&cg->spar.dojoBGItems[i], 32 * (i + 1), 32 * (i + 1));
    }
}

static void cg_drawSparChowa(cGrove_t* cg, int64_t elapsedUs)
{
    for (int32_t idx = 0; idx < 2; idx++)
    {
        switch (cg->spar.match.chowaData[idx].currState)
        {
            case CG_SPAR_UNREADY:
            case CG_SPAR_NOTHING:
            {
                // Bounce back and forth as ready
                break;
            }
            case CG_SPAR_READY:
            {
                // Pause on a specific frame
                break;
            }
            case CG_SPAR_EXHAUSTED:
            {
                // Sitting on the floor, panting
                break;
            }
            case CG_SPAR_HIT:
            {
                // Chowa flashes as they get hit
                break;
            }
            case CG_SPAR_ATTACK:
            {
                // Draw the Chowa attacking
                break;
            }
            case CG_SPAR_DODGE_ST:
            {
                // Draw chowa dodge
                break;
            }
            case CG_SPAR_WIN:
            {
                // Draw Chowa cheering
                break;
            }
            case CG_SPAR_LOSE:
            {
                // Draw Chowa crying
                break;
            }
        }
    }
    // Draw Chowa
    // - 2x size
    // - Nametags / "You!"
    // FIXME: Only set done when actually done
    if (cg->spar.match.chowaData[0].doneAnimating && cg->spar.match.chowaData[1].doneAnimating)
    {
        cg->spar.match.animDone = true;
    }
    //
}

/**
 * @brief Draws the Chowa centric UI
 *
 * @param cg Game Data
 */
static void cg_drawSparChowaUI(cGrove_t* cg)
{
    // Player 1
    // Draw health bar
    cg_drawSparProgBars(cg, cg->spar.match.chowaData[CG_P1].maxHP, cg->spar.match.chowaData[CG_P1].HP, PADDING,
                        STAT_BAR_BASE, c500, 3);
    // Draw stamina bar
    cg_drawSparProgBars(cg, cg->spar.match.chowaData[CG_P1].maxStamina, cg->spar.match.chowaData[CG_P1].stamina,
                        1 * (PADDING + STAT_BAR_WIDTH) + PADDING, STAT_BAR_BASE, c550, 1);
    // Draw Readiness bar
    cg_drawSparProgBars(cg, CG_MAX_READY_VALUE, cg->spar.match.chowaData[CG_P1].readiness,
                        2 * (PADDING + STAT_BAR_WIDTH) + PADDING, STAT_BAR_BASE, c050, -1);

    switch (cg->spar.match.chowaData[CG_P1].currState)
    {
        case CG_SPAR_UNREADY:
        case CG_SPAR_READY:
        {
            // TODO: Draw attack icon
            switch (cg->spar.match.chowaData[CG_P1].currMove)
            {
                case CG_SPAR_PUNCH:
                {
                    drawText(&cg->spar.sparRegFont, c005, "P1: Punch", 100, 108);
                    drawWsgSimple(&cg->arrow, 64, 64);
                    break;
                }
                case CG_SPAR_FAST_PUNCH:
                {
                    drawText(&cg->spar.sparRegFont, c005, "P1: Fast Punch", 100, 108);
                    drawWsg(&cg->arrow, 64, 64, false, true, 0);
                    break;
                }
                case CG_SPAR_KICK:
                {
                    drawText(&cg->spar.sparRegFont, c005, "P1: Kick", 100, 108);
                    drawWsg(&cg->arrow, 64, 64, false, false, 270);
                    break;
                }
                case CG_SPAR_JUMP_KICK:
                {
                    drawText(&cg->spar.sparRegFont, c005, "P1: Jump Kick", 100, 108);
                    drawWsg(&cg->arrow, 64, 64, false, false, 90);
                    break;
                }
                case CG_SPAR_HEADBUTT:
                {
                    drawText(&cg->spar.sparRegFont, c005, "P1: Headbutt", 100, 108);
                    drawWsg(&cg->arrow, 64, 64, false, false, 90);
                    drawWsg(&cg->arrow, 64, 64, false, false, 270);
                    break;
                }
                case CG_SPAR_DODGE:
                {
                    drawText(&cg->spar.sparRegFont, c005, "P1: Dodge", 100, 108);
                    drawWsg(&cg->arrow, 64, 64, false, true, 0);
                    drawWsg(&cg->arrow, 64, 64, false, false, 0);
                    break;
                }
                default:
                {
                    drawText(&cg->spar.sparRegFont, c005, "P1: Unset", 100, 108);
                    break;
                }
            }
            break;
        }
        default:
        {
            break;
        }
    }

    // Player 2
    // Draw health bar
    cg_drawSparProgBars(cg, cg->spar.match.chowaData[CG_P2].maxHP, cg->spar.match.chowaData[CG_P2].HP,
                        TFT_WIDTH - PADDING, STAT_BAR_BASE, c500, 3);
    // Draw stamina bar
    cg_drawSparProgBars(cg, cg->spar.match.chowaData[CG_P2].maxStamina, cg->spar.match.chowaData[CG_P2].stamina,
                        TFT_WIDTH - (1 * (PADDING + STAT_BAR_WIDTH) + PADDING), STAT_BAR_BASE, c550, 1);
    // Draw Readiness bar
    cg_drawSparProgBars(cg, CG_MAX_READY_VALUE, cg->spar.match.chowaData[CG_P2].readiness,
                        TFT_WIDTH - (2 * (PADDING + STAT_BAR_WIDTH) + PADDING), STAT_BAR_BASE, c050, -1);

    // FIXME: For debug only
    switch (cg->spar.match.chowaData[CG_P2].currState)
    {
        case CG_SPAR_UNREADY:
        case CG_SPAR_READY:
        {
            // TODO: Draw attack icon
            switch (cg->spar.match.chowaData[CG_P2].currMove)
            {
                case CG_SPAR_PUNCH:
                {
                    drawText(&cg->spar.sparRegFont, c500, "AI: Punch", 100, 128);
                    break;
                }
                case CG_SPAR_FAST_PUNCH:
                {
                    drawText(&cg->spar.sparRegFont, c500, "AI: Fast Punch", 100, 128);
                    break;
                }
                case CG_SPAR_KICK:
                {
                    drawText(&cg->spar.sparRegFont, c500, "AI: Kick", 100, 128);
                    break;
                }
                case CG_SPAR_JUMP_KICK:
                {
                    drawText(&cg->spar.sparRegFont, c500, "AI: Jump Kick", 100, 128);
                    break;
                }
                case CG_SPAR_HEADBUTT:
                {
                    drawText(&cg->spar.sparRegFont, c500, "AI: Headbutt", 100, 128);
                    break;
                }
                case CG_SPAR_DODGE:
                {
                    drawText(&cg->spar.sparRegFont, c500, "AI: Dodge", 100, 128);
                    break;
                }
                default:
                {
                    drawText(&cg->spar.sparRegFont, c500, "AI: Unset", 100, 128);
                    break;
                }
            }
            break;
        }
        default:
        {
            break;
        }
    }
}

/**
 * @brief Draws a progress bar facing vertically
 *
 * @param cg Game Data
 * @param maxVal Height of the stat bar, bit shifted in half (To fit on screen)
 * @param currVal Current value of the stat to track
 * @param x X pos of the colored bar
 * @param y Y pos base of the bar
 * @param color Color of the bar
 * @param bitShift How many bits to shift to ring bar into scale. Negative to expand bar.
 */
static void cg_drawSparProgBars(cGrove_t* cg, int16_t maxVal, int16_t currVal, int16_t x, int16_t y,
                                paletteColor_t color, int8_t bitShift)
{
    if (bitShift < 0)
    {
        // Draw background
        fillDisplayArea(x - 1, y - (maxVal << abs(bitShift)) - 1, x + STAT_BAR_WIDTH + 1, y + 1, c000);

        // Draw bar in proper color
        fillDisplayArea(x, y - (currVal << abs(bitShift)), x + STAT_BAR_WIDTH, y, color);
        return;
    }
    // Draw background
    fillDisplayArea(x - 1, y - (maxVal >> bitShift) - 1, x + STAT_BAR_WIDTH + 1, y + 1, c000);

    // Draw bar in proper color
    fillDisplayArea(x, y - (currVal >> bitShift), x + STAT_BAR_WIDTH, y, color);
}