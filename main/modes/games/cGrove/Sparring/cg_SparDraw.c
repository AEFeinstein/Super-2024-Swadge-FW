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
#include "cg_Chowa.h"

//==============================================================================
// Defines
//==============================================================================

#define STAT_BAR_WIDTH 4
#define STAT_BAR_BASE  200
#define PADDING        6

//==============================================================================
// Const Strings
//==============================================================================

static const char* sparSplashScreen[] = {"Chowa Sparring", "Press any button to continue!"};
static const char* matchText[]        = {"--Pause--", "FINISHED", "DRAW", "YOU LOST!", "YOU WIN!"};
static const char* prompts[]
    = {"Set up a match", "Press A to start match", "Press B to go back", "Go to the grove to get a Chowa first!"};
static const char* difficultyStrs[] = {"Beginner", "Very Easy", "Easy", "Medium", "Hard", "Very Hard", "Expert"};

static const int16_t sparMatchTimes[] = {30, 60, 90, 120, 999};

//==============================================================================
// Function Declarations
//==============================================================================

static void cg_drawSparField(cGrove_t* cg);
static void cg_drawSparChowa(cGrove_t* cg, int64_t elapsedUs);
static void cg_drawSparChowaUI(cGrove_t* cg);
static void cg_drawSparProgBars(cGrove_t* cg, int16_t maxVal, int16_t currVal, int16_t x, int16_t y,
                                paletteColor_t color, int8_t bitShift);

//==============================================================================
// Functions
//==============================================================================

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

    // Draw Chowa
    cgChowa_t* example = NULL;
    for (int idx = 0; idx < CG_MAX_CHOWA; idx++)
    {
        if (cg->chowa[idx].active)
        {
            example = &cg->chowa[idx];
            break;
        }
    }
    if (example != NULL)
    {
        cg->spar.timer += elapsedUs;
        if (cg->spar.timer >= 300000)
        {
            cg->spar.timer     = 0;
            cg->spar.animFrame = (cg->spar.animFrame + 1) % 2;
        }
        wsg_t* spr = cg_getChowaWSG(cg, example, CG_ANIM_JUMP, cg->spar.animFrame);
        drawWsgSimpleScaled(spr, (TFT_WIDTH - (spr->w * 2)) >> 1, 153, 2, 2);
    }

    // Draw title text
    // Get the text offset
    int16_t xOff = (TFT_WIDTH - textWidth(&cg->titleFont, sparSplashScreen[0])) >> 1;
    int16_t yOff = 110;
    drawText(&cg->titleFont, c555, sparSplashScreen[0], xOff, yOff);
    drawText(&cg->titleFontOutline, c000, sparSplashScreen[0], xOff, yOff);
    xOff = 32;
    yOff = 10;
    drawTextWordWrap(&cg->largeMenuFont, c555, sparSplashScreen[1], &xOff, &yOff, TFT_WIDTH - 16, TFT_HEIGHT);
}

void cg_drawSparMatchPrep(cGrove_t* cg)
{
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);

    if (cg->spar.numActiveChowa < 1)
    {
        int16_t xOff = 16;
        int16_t yOff = (TFT_HEIGHT - 64) >> 1;
        drawTextWordWrap(&cg->largeMenuFont, c555, prompts[3], &xOff, &yOff, TFT_WIDTH - 16, TFT_HEIGHT);
        return;
    }

    // Draw prompts
    drawText(&cg->largeMenuFont, c555, prompts[0], (TFT_WIDTH - textWidth(&cg->largeMenuFont, prompts[0])) / 2, 16);
    drawText(&cg->menuFont, c555, prompts[1], (TFT_WIDTH - textWidth(&cg->menuFont, prompts[1])) / 2, TFT_HEIGHT - 32);
    drawText(&cg->menuFont, c555, prompts[2], (TFT_WIDTH - textWidth(&cg->menuFont, prompts[2])) / 2, TFT_HEIGHT - 16);

    char buffer[40];

    // Draw Selected Chowa name
    snprintf(buffer, sizeof(buffer) - 1, "Selected Chowa: %s", cg->spar.activeChowaNames[cg->spar.chowaSelect]);
    drawText(&cg->menuFont, c550, buffer, 48, 64);
    // Draw difficulty
    snprintf(buffer, sizeof(buffer) - 1, "Difficulty: %s", difficultyStrs[cg->spar.aiSelect]);
    drawText(&cg->menuFont, c550, buffer, 48, 96);
    // Draw match timer
    snprintf(buffer, sizeof(buffer) - 1, "Match Time: %" PRId16, sparMatchTimes[cg->spar.timerSelect]);
    drawText(&cg->menuFont, c550, buffer, 48, 128);

    // Draw arrows around current selection
    drawWsg(&cg->arrow, 16, 64 + (cg->spar.optionSelect * 32), false, false, 270);
    drawWsg(&cg->arrow, TFT_WIDTH - (16 + cg->arrow.h), 64 + (cg->spar.optionSelect * 32), false, false, 90);
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

    // Draw UI
    // Buffer
    char buffer[32];

    // Draw match title
    snprintf(buffer, sizeof(buffer) - 1, "%s, round %d", cg->spar.match.data.matchTitle, cg->spar.match.round);
    drawText(&cg->largeMenuFont, c000, buffer, (TFT_WIDTH - textWidth(&cg->largeMenuFont, buffer)) >> 1, 8);

    // Time
    paletteColor_t color = c000;
    if (cg->spar.match.maxTime <= (cg->spar.match.timer + 10))
    {
        color = c500;
    }
    snprintf(buffer, sizeof(buffer) - 1, "Time: %d", cg->spar.match.maxTime - cg->spar.match.timer);
    drawText(&cg->largeMenuFont, color, buffer, (TFT_WIDTH - textWidth(&cg->largeMenuFont, buffer)) >> 1, 24);

    // Draw Chowa UI
    cg_drawSparChowaUI(cg);

    // If paused, draw pause text
    if (cg->spar.match.paused)
    {
        drawText(&cg->titleFont, c555, matchText[0], ((TFT_WIDTH - textWidth(&cg->titleFont, matchText[0])) >> 1),
                 (TFT_HEIGHT >> 1) - 16);
        drawText(&cg->titleFontOutline, c000, matchText[0], (TFT_WIDTH - textWidth(&cg->titleFont, matchText[0])) >> 1,
                 (TFT_HEIGHT >> 1) - 16);
    }

    // Draw match end
    if (cg->spar.match.done)
    {
        drawText(&cg->titleFont, c555, matchText[1], (TFT_WIDTH - textWidth(&cg->titleFontOutline, matchText[1])) >> 1,
                 (TFT_HEIGHT >> 1) - 16);
        drawText(&cg->titleFontOutline, c000, matchText[1],
                 (TFT_WIDTH - textWidth(&cg->titleFontOutline, matchText[1])) >> 1, (TFT_HEIGHT >> 1) - 16);

        switch (cg->spar.match.data.result[cg->spar.match.round])
        {
            case CG_P1_WIN:
            {
                drawText(&cg->titleFont, c555, matchText[4],
                         (TFT_WIDTH - textWidth(&cg->titleFontOutline, matchText[4])) >> 1, (TFT_HEIGHT >> 1) + 20);
                drawText(&cg->titleFontOutline, c000, matchText[4],
                         (TFT_WIDTH - textWidth(&cg->titleFontOutline, matchText[4])) >> 1, (TFT_HEIGHT >> 1) + 20);
                break;
            }
            case CG_P2_WIN:
            {
                drawText(&cg->titleFont, c555, matchText[3],
                         (TFT_WIDTH - textWidth(&cg->titleFontOutline, matchText[3])) >> 1, (TFT_HEIGHT >> 1) + 20);
                drawText(&cg->titleFontOutline, c000, matchText[3],
                         (TFT_WIDTH - textWidth(&cg->titleFontOutline, matchText[3])) >> 1, (TFT_HEIGHT >> 1) + 20);
                break;
            }
            case CG_DRAW:
            {
                drawText(&cg->titleFont, c555, matchText[2],
                         (TFT_WIDTH - textWidth(&cg->titleFontOutline, matchText[2])) >> 1, (TFT_HEIGHT >> 1) + 20);
                drawText(&cg->titleFontOutline, c000, matchText[2],
                         (TFT_WIDTH - textWidth(&cg->titleFontOutline, matchText[2])) >> 1, (TFT_HEIGHT >> 1) + 20);
                break;
            }
        }
    }
}

//==============================================================================
// Static Functions
//==============================================================================

/**
 * @brief Draws the background
 *
 * @param cg Game data
 */
static void cg_drawSparField(cGrove_t* cg)
{
    // Draw the sky
    drawWsgSimple(&cg->title[1], 0, 0);
    // Draw dojo
    drawWsgSimple(&cg->spar.dojoBG, 0, 0);
}

/**
 * @brief Draws the Chowa depending on the sparring state
 *
 * @param cg Game Data
 * @param elapsedUs Time since last frame
 */
static void cg_drawSparChowa(cGrove_t* cg, int64_t elapsedUs)
{
    cgSparChowaData_t* cg1 = &cg->spar.match.chowa[CG_P1];
    cgSparChowaData_t* cg2 = &cg->spar.match.chowa[CG_P2];
    cg1->animTimer += elapsedUs;
    cg2->animTimer += elapsedUs;

    // Player 1
    int16_t xOff = 80;
    int16_t yOff = 175;
    if (cg1->chowa->age >= CG_ADULT_AGE)
    {
        xOff = 50;
        yOff = 153;
    }
    wsg_t* spr;
    switch (cg1->currState)
    {
        case CG_SPAR_UNREADY:
        case CG_SPAR_NOTHING:
        default:
        {
            // Standing there, menacingly
            spr = cg_getChowaWSG(cg, cg1->chowa, CG_ANIM_WALK_RIGHT, 0);
            drawWsgSimpleScaled(spr, xOff, yOff, 2, 2);
            break;
        }
        case CG_SPAR_READY:
        {
            // Walk forward toward center
            if (cg1->animTimer > 1000000 >> 2)
            {
                cg1->animFrame++;
                if (cg1->animFrame >= 4)
                {
                    cg1->animFrame = 0;
                }
                cg1->hOffset += 5;
                cg1->animTimer = 0;
            }
            spr = cg_getChowaWSG(cg, cg1->chowa, CG_ANIM_WALK_RIGHT, cg1->animFrame);
            drawWsgSimpleScaled(spr, xOff + cg1->hOffset, yOff, 2, 2);
            break;
        }
        case CG_SPAR_EXHAUSTED:
        {
            // Sitting on the floor, panting
            if (cg1->animTimer >= 500000)
            {
                cg1->animFrame = (cg1->animFrame + 1) % 2;
                cg1->animTimer = 0;
            }
            spr = cg_getChowaWSG(cg, cg1->chowa, CG_ANIM_SAD, cg1->animFrame);
            drawWsgSimpleScaled(spr, xOff, yOff, 2, 2);
            break;
        }
        case CG_SPAR_HIT:
        {
            // Chowa falls over when they get hit
            if (cg1->animTimer < 330000)
            {
                spr = cg_getChowaWSG(cg, cg1->chowa, CG_ANIM_HIT_LEFT, 0);
            }
            else if (cg1->animTimer < 660000)
            {
                spr = cg_getChowaWSG(cg, cg1->chowa, CG_ANIM_HIT_LEFT, 1);
            }
            else if (cg1->animTimer >= 660000)
            {
                spr = cg_getChowaWSG(cg, cg1->chowa, CG_ANIM_HIT_LEFT, 2);
            }
            if (cg1->animTimer >= 1000000)
            {
                cg1->doneAnimating = true;
            }
            drawWsgSimpleScaled(spr, xOff, yOff, 2, 2);
            break;
        }
        case CG_SPAR_ATTACK:
        {
            // TODO: Draw the Chowa attacking
            switch (cg1->currMove)
            {
                case CG_SPAR_PUNCH:
                case CG_SPAR_FAST_PUNCH:
                {
                    spr = cg_getChowaWSG(cg, cg1->chowa, CG_ANIM_PUNCH_RIGHT, 0);
                    break;
                }
                case CG_SPAR_KICK:
                case CG_SPAR_JUMP_KICK:
                case CG_SPAR_DODGE:
                {
                    spr = cg_getChowaWSG(cg, cg1->chowa, CG_ANIM_KICK_RIGHT, 0);
                    break;
                }
                case CG_SPAR_HEADBUTT:
                {
                    spr = cg_getChowaWSG(cg, cg1->chowa, CG_ANIM_HEADBUTT_RIGHT, 0);
                    break;
                }
                default:
                {
                    spr = cg_getChowaWSG(cg, cg1->chowa, CG_ANIM_WALK_RIGHT, 0);
                    break;
                }
            }
            break;
        }
        case CG_SPAR_DODGE_ST:
        {
            // TODO: Draw Chowa dodge
            break;
        }
        case CG_SPAR_WIN:
        {
            // Draw Chowa cheering
            if (cg1->animTimer >= 500000)
            {
                cg1->animFrame = (cg1->animFrame + 1) % 2;
                cg1->animTimer = 0;
            }
            spr = cg_getChowaWSG(cg, cg1->chowa, CG_ANIM_JUMP, cg1->animFrame);
            drawWsgSimpleScaled(spr, xOff, yOff, 2, 2);
            break;
        }
        case CG_SPAR_LOSE:
        {
            // Draw Chowa crying
            if (cg1->animTimer >= 500000)
            {
                cg1->animFrame = (cg1->animFrame + 1) % 3;
                cg1->animTimer = 0;
            }
            spr = cg_getChowaWSG(cg, cg1->chowa, CG_ANIM_CRY, cg1->animFrame);
            drawWsgSimpleScaled(spr, xOff, yOff, 2, 2);
            break;
        }
    }

    // Player 2
    wsg_t* widthSpr;
    widthSpr = cg_getChowaWSG(cg, cg2->chowa, CG_ANIM_WALK_LEFT, 0);
    xOff     = TFT_WIDTH - (80 + 2 * widthSpr->h);
    yOff     = 175;
    if (cg2->chowa->age >= CG_ADULT_AGE)
    {
        xOff = TFT_WIDTH - (50 + 2 * widthSpr->h);
        yOff = 153;
    }
    switch (cg2->currState)
    {
        case CG_SPAR_UNREADY:
        case CG_SPAR_NOTHING:
        default:
        {
            // Standing there, menacingly
            spr = cg_getChowaWSG(cg, cg2->chowa, CG_ANIM_WALK_LEFT, 0);
            drawWsgSimpleScaled(spr, xOff, yOff, 2, 2);
            break;
        }
        case CG_SPAR_READY:
        {
            // Pause on a specific frame
            // Walk forward toward center
            if (cg2->animTimer > 1000000 >> 2)
            {
                cg2->animFrame++;
                if (cg2->animFrame >= 4)
                {
                    cg2->animFrame = 0;
                }
                cg2->hOffset -= 5;
                cg2->animTimer = 0;
            }
            spr = cg_getChowaWSG(cg, cg2->chowa, CG_ANIM_WALK_LEFT, cg2->animFrame);
            drawWsgSimpleScaled(spr, xOff + cg2->hOffset, yOff, 2, 2);
            break;
        }
        case CG_SPAR_EXHAUSTED:
        {
            // Sitting on the floor, panting
            if (cg2->animTimer >= 500000)
            {
                cg2->animFrame = (cg2->animFrame + 1) % 2;
                cg2->animTimer = 0;
            }
            spr = cg_getChowaWSG(cg, cg2->chowa, CG_ANIM_SAD, cg2->animFrame);
            drawWsgSimpleScaled(spr, xOff, yOff, 2, 2);

            break;
        }
        case CG_SPAR_HIT:
        {
            // Chowa flashes as they get hit
            if (cg2->animTimer < 330000)
            {
                spr = cg_getChowaWSG(cg, cg2->chowa, CG_ANIM_HIT_RIGHT, 0);
            }
            else if (cg2->animTimer < 660000)
            {
                spr = cg_getChowaWSG(cg, cg2->chowa, CG_ANIM_HIT_RIGHT, 1);
            }
            else if (cg2->animTimer >= 660000)
            {
                spr = cg_getChowaWSG(cg, cg2->chowa, CG_ANIM_HIT_RIGHT, 2);
            }
            if (cg2->animTimer >= 1000000)
            {
                cg2->doneAnimating = true;
            }
            drawWsgSimpleScaled(spr, xOff, yOff, 2, 2);
            break;
        }
        case CG_SPAR_ATTACK:
        {
            // Draw the Chowa attacking
            switch (cg2->currMove)
            {
                case CG_SPAR_PUNCH:
                case CG_SPAR_FAST_PUNCH:
                {
                    spr = cg_getChowaWSG(cg, cg2->chowa, CG_ANIM_PUNCH_LEFT, 0);
                    break;
                }
                case CG_SPAR_KICK:
                case CG_SPAR_JUMP_KICK:
                case CG_SPAR_DODGE:
                {
                    spr = cg_getChowaWSG(cg, cg2->chowa, CG_ANIM_KICK_LEFT, 0);
                    break;
                }
                case CG_SPAR_HEADBUTT:
                {
                    spr = cg_getChowaWSG(cg, cg2->chowa, CG_ANIM_HEADBUTT_LEFT, 0);
                    break;
                }
                default:
                {
                    spr = cg_getChowaWSG(cg, cg2->chowa, CG_ANIM_WALK_LEFT, 0);
                    break;
                }
            }
            drawWsgSimpleScaled(spr, xOff, yOff, 2, 2);
            break;
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
            if (cg2->animTimer >= 500000)
            {
                cg2->animFrame = (cg2->animFrame + 1) % 2;
                cg2->animTimer = 0;
            }
            spr = cg_getChowaWSG(cg, cg2->chowa, CG_ANIM_JUMP, cg2->animFrame);
            drawWsgSimpleScaled(spr, xOff, yOff, 2, 2);
            break;
        }
        case CG_SPAR_LOSE:
        {
            // Draw Chowa crying
            if (cg2->animTimer >= 500000)
            {
                cg2->animFrame = (cg2->animFrame + 1) % 3;
                cg2->animTimer = 0;
            }
            spr = cg_getChowaWSG(cg, cg2->chowa, CG_ANIM_CRY, cg2->animFrame);
            drawWsgSimpleScaled(spr, xOff, yOff, 2, 2);
            break;
        }
    }
    // Draw Chowa
    // - 2x size
    // - Nametags / "You!"
    if (cg1->doneAnimating && cg2->doneAnimating)
    {
        cg1->doneAnimating      = false;
        cg2->doneAnimating      = false;
        cg1->hOffset            = 0;
        cg2->hOffset            = 0;
        cg->spar.match.animDone = true;
    }
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
    cg_drawSparProgBars(cg, cg->spar.match.chowa[CG_P1].maxHP, cg->spar.match.chowa[CG_P1].HP, PADDING, STAT_BAR_BASE,
                        c500, 3);
    // Draw stamina bar
    cg_drawSparProgBars(cg, cg->spar.match.chowa[CG_P1].maxStamina, cg->spar.match.chowa[CG_P1].stamina,
                        1 * (PADDING + STAT_BAR_WIDTH) + PADDING, STAT_BAR_BASE, c550, 1);
    // Draw Readiness bar
    cg_drawSparProgBars(cg, CG_MAX_READY_VALUE, cg->spar.match.chowa[CG_P1].readiness,
                        2 * (PADDING + STAT_BAR_WIDTH) + PADDING, STAT_BAR_BASE, c050, -1);

    switch (cg->spar.match.chowa[CG_P1].currState)
    {
        case CG_SPAR_UNREADY:
        case CG_SPAR_READY:
        {
            switch (cg->spar.match.chowa[CG_P1].currMove)
            {
                case CG_SPAR_PUNCH:
                {
                    // drawText(&cg->menuFont, c005, "P1: Punch", 100, 108);
                    drawWsgSimpleScaled(&cg->spar.attackIcons[1], 64, 64, 2, 2);
                    break;
                }
                case CG_SPAR_FAST_PUNCH:
                {
                    // drawText(&cg->menuFont, c005, "P1: Fast Punch", 100, 108);
                    drawWsgSimpleScaled(&cg->spar.attackIcons[2], 64, 64, 2, 2);
                    break;
                }
                case CG_SPAR_KICK:
                {
                    // drawText(&cg->menuFont, c005, "P1: Kick", 100, 108);
                    drawWsgSimpleScaled(&cg->spar.attackIcons[4], 64, 64, 2, 2);
                    break;
                }
                case CG_SPAR_JUMP_KICK:
                {
                    // drawText(&cg->menuFont, c005, "P1: Jump Kick", 100, 108);
                    drawWsgSimpleScaled(&cg->spar.attackIcons[5], 64, 64, 2, 2);
                    break;
                }
                case CG_SPAR_HEADBUTT:
                {
                    // drawText(&cg->menuFont, c005, "P1: Headbutt", 100, 108);
                    drawWsgSimpleScaled(&cg->spar.attackIcons[3], 64, 64, 2, 2);
                    break;
                }
                case CG_SPAR_DODGE:
                {
                    // drawText(&cg->menuFont, c005, "P1: Dodge", 100, 108);
                    drawWsgSimpleScaled(&cg->spar.attackIcons[0], 64, 64, 2, 2);
                    break;
                }
                default:
                {
                    // drawText(&cg->menuFont, c005, "P1: Unset", 100, 108);
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
    cg_drawSparProgBars(cg, cg->spar.match.chowa[CG_P2].maxHP, cg->spar.match.chowa[CG_P2].HP, TFT_WIDTH - PADDING,
                        STAT_BAR_BASE, c500, 3);
    // Draw stamina bar
    cg_drawSparProgBars(cg, cg->spar.match.chowa[CG_P2].maxStamina, cg->spar.match.chowa[CG_P2].stamina,
                        TFT_WIDTH - (1 * (PADDING + STAT_BAR_WIDTH) + PADDING), STAT_BAR_BASE, c550, 1);
    // Draw Readiness bar
    cg_drawSparProgBars(cg, CG_MAX_READY_VALUE, cg->spar.match.chowa[CG_P2].readiness,
                        TFT_WIDTH - (2 * (PADDING + STAT_BAR_WIDTH) + PADDING), STAT_BAR_BASE, c050, -1);

    /* // For debug only
    switch (cg->spar.match.chowa[CG_P2].currState)
    {
        case CG_SPAR_UNREADY:
        case CG_SPAR_READY:
        {
            switch (cg->spar.match.chowa[CG_P2].currMove)
            {
                case CG_SPAR_PUNCH:
                {
                    drawText(&cg->menuFont, c500, "AI: Punch", 100, 128);
                    break;
                }
                case CG_SPAR_FAST_PUNCH:
                {
                    drawText(&cg->menuFont, c500, "AI: Fast Punch", 100, 128);
                    break;
                }
                case CG_SPAR_KICK:
                {
                    drawText(&cg->menuFont, c500, "AI: Kick", 100, 128);
                    break;
                }
                case CG_SPAR_JUMP_KICK:
                {
                    drawText(&cg->menuFont, c500, "AI: Jump Kick", 100, 128);
                    break;
                }
                case CG_SPAR_HEADBUTT:
                {
                    drawText(&cg->menuFont, c500, "AI: Headbutt", 100, 128);
                    break;
                }
                case CG_SPAR_DODGE:
                {
                    drawText(&cg->menuFont, c500, "AI: Dodge", 100, 128);
                    break;
                }
                default:
                {
                    drawText(&cg->menuFont, c500, "AI: Unset", 100, 128);
                    break;
                }
            }
            break;
        }
        default:
        {
            break;
        }
    } */
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