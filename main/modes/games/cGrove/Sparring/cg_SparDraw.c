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
static void cg_drawSparUI(cGrove_t* cg);

//==============================================================================
// Functions
//==============================================================================

void cg_drawSpar(cGrove_t* cg, int64_t elapsedUs)
{
    // Draw dojo
    cg_drawSparField(cg);

    // Draw Objects
    cg_drawSparBGObject(cg, elapsedUs);

    // Draw Chowa
}

void cg_drawSparSplash(cGrove_t* cg, int64_t elapsedUs)
{
    // Draw dojo
    cg_drawSparField(cg);
    cg_drawSparBGObject(cg, elapsedUs);

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
    drawTextWordWrap(&cg->spar.sparRegFont, c000, sparSplashScreenPrompt, &xOff, &yOff, TFT_WIDTH - 32, TFT_HEIGHT);
}

void cg_drawSparRecord(cGrove_t* cg)
{
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);
    // Draw Match title

    // Draw player names 

    // Draw a result
    // - Chowa names
    // - Chow sprites
    // - Round
    // - VS label
    // - Round length
    // - Result
    
    // Draw record number and arrows
}

static void cg_drawSparField(cGrove_t* cg)
{
    // Draw dojo
    drawWsgSimple(&cg->spar.dojoBG, 0, 0);
}

static void cg_drawSparBGObject(cGrove_t* cg, int64_t elapsedUs)
{
    // FIXME: DO bounce thing
    for (int32_t i = 0; i < 2; i++)
    {
        drawWsgSimple(&cg->spar.dojoBGItems[i], 32 * (i + 1), 32 * (i + 1));
    }
}

static void cg_drawSparChowa(cGrove_t* cg, int64_t elapsedUs)
{
    // Draw Chowa
    // - 2x size
    // - Nametags / "You!"
}

static void cg_drawSparUI(cGrove_t* cg)
{
    // Draw UI
    // - Stamina bar
    // - Timer / Rounds
    // - Moves selection
    // - Dialogue boxes
    // - "Ready", "Set" type bubbles
}
