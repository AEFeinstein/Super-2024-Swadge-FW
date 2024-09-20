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

//==============================================================================
// Enum
//==============================================================================

typedef enum
{
    SPLASH,
    MENU,
    MATCH,
    MATCH_SCHEDULE,
    TUTORIAL,
} cgSparState_t;

//==============================================================================
// Functions
//==============================================================================

void cg_initSpar(cGrove_t* cg)
{
    // Init menu

    // Load splash screen
}

void cg_deInitSpar(cGrove_t* cg)
{
    // deinit menu
}


void cg_runSpar(cGrove_t* cg, int32_t elapsedUs)
{
    // TODO: A list!
    // Handle input
    // - In match
    //   - Chowa is preparing:
    //     - L/R/U/D/A/B: Pick a move (Punch, Fast punch, Kick, Fast kick, Headbutt, Dodge)
    //   - Chowa is regaining stamina:
    //     - A: Increases rate of Sta regen
    //     - B: Encourage them to stand up
    //   - Start: pause
    // - Menus

    // Manage state
    // - Splash screen
    // - Menu
    //   - Settings (Online?)
    // - Schedule match (Pick from opponents, Select your Chowa)
    // - In match
    // - Tutorial
    // - Battle record
    // - Paused
    //   - Tutorial when paused

    // Draw
}