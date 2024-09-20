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
// Function Declarations
//==============================================================================

static void cg_drawSparField(cGrove_t* cg);
static void cg_drawSparBGObject(cGrove_t* cg);
static void cg_drawSparChowa(cGrove_t* cg);
static void cg_drawSparUI(cGrove_t* cg);

//==============================================================================
// Functions
//==============================================================================

void cg_drawSpar(cGrove_t* cg)
{
    // Draw dojo
    cg_drawSparField(cg);

    // Draw Objects
    cg_drawSparBGObject(cg);

    // Draw Chowa
}

static void cg_drawSparField(cGrove_t* cg)
{
    // Draw dojo
}

static void cg_drawSparBGObject(cGrove_t* cg)
{
    // Draw objects
    // - Have them bounce when a critical is made
}

static void cg_drawSparChowa(cGrove_t* cg)
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

