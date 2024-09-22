/**
 * @file cg_Match.c
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief Provide the individual match implementation for Chowa Grove spars
 * @version 0.1
 * @date 2024-09-22
 *
 * @copyright Copyright (c) 2024
 *
 */

//==============================================================================
// Includes
//==============================================================================

#include "cg_Match.h"

//==============================================================================
// Functions
//==============================================================================

void cg_runSparMatch(cGrove_t* cg, int64_t elapsedUs)
{
    // Initialize
    // - Chowa readiness is set to zero
    // - Stamina is set to max
    // - Timer is set to 0
    // - Load tournament/match data

    // Loop
    // Check Chowa's stamina
    // - If higher than 0, move to getting ready
    // - Else, move to Exhausted branch

    // Getting ready
    // Ready status climbs based on Chowa's stats
    // Player can press buttons to indicate which move they want to execute
    // Repeated presses increases the rate of readiness slightly
    // Once ready bar is full, move to ready state

    // Ready
    // Move is locked in
    // Check of other Chowa is also ready
    // - If not, Wait a short time before attacking to see if the other chowa gets ready in time
    // Once timer is up, resolve

    // Resolve
    // If executed a move, burn stamina in accordance with move
    // If a dodge is commanded on either side, no damage happens
    // If one of the Chowa is still not ready, they automatically take damage
    // Check RPS
    // Winner's attack stat is compared with Loser's agility with some randomness
    //

    // Exhausted
    // Chowa sits down
    // Stamina starts increasing slowly
    // A bar appears showing how fast it's going to get up
    // Player can influence both bars via button presses
    // Once either bar fills, CHowa stands up enters the getting ready phase

    // RANDOMNESS
    // Need to add some randomness to checks
}

// Chowa states
// Unready
// Ready
// Resolving (attacking, dodging)
// Exhausted