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
// Defines
//==============================================================================

#define CG_SECOND         1000000 // One second of us
#define CG_MIN_STAMINA    40      // Minimum value for stamina
#define CG_STAMINA_SCALAR 60000   // How fast the stamina regenerates

//==============================================================================
// Static Functions
//==============================================================================

static void cg_sparPlayerInput(cGrove_t* cg);
static void cg_sparChowaState(cGrove_t* cg, int64_t elapsedUs);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initializes the match based on given parameters
 *
 * @param cg Game data
 * @param matchName Name of the match to display
 * @param player1Chowa Pointer to the swadge player's chowa
 * @param player2Chowa Point to the opposing chowa
 * @param round What round of the spar this is
 * @param maxTime When the timeout occurs
 */
void cg_initMatch(cGrove_t* cg, char* matchName, cgChowa_t* player1Chowa, cgChowa_t* player2Chowa, int8_t round,
                  int16_t maxTime)
{
    // Initialize
    // Load tournament/match data
    strcpy(cg->spar.match.matchName, matchName);
    cg->spar.match.round   = round;
    cg->spar.match.maxTime = maxTime;

    // Timer is set to 0
    cg->spar.match.timer = 0;

    // Chowa
    cg->spar.match.chowaData[0].chowa = player1Chowa;
    cg->spar.match.chowaData[1].chowa = player2Chowa;
    for (int32_t i = 0; i < 2; i++)
    {
        cg->spar.match.chowaData[i].currState = CG_UNREADY;
        cg->spar.match.chowaData[i].currMove  = CG_SPAR_PUNCH;
        cg->spar.match.chowaData[i].maxStamina
            = CG_MIN_STAMINA + (cg->spar.match.chowaData[i].chowa->stats[CG_STAMINA] >> 1);
        cg->spar.match.chowaData[i].stamina     = 0; // FIXME: cg->spar.match.chowaData[i].maxStamina;
        cg->spar.match.chowaData[i].readiness   = 0;
        cg->spar.match.chowaData[i].readiness   = 0;
        cg->spar.match.chowaData[i].updateTimer = 0;

        cg->spar.match.chowaData[i].chowa->stats[CG_SPEED] = 128;
        cg->spar.match.chowaData[i].chowa->stats[CG_STAMINA] = 128;
    }
}

/**
 * @brief Executes the logic for the spar
 *
 * @param cg Game data
 * @param elapsedUs time since last frame
 */
void cg_runSparMatch(cGrove_t* cg, int64_t elapsedUs)
{
    // Round timer
    // Timer doesn't accumulate if paused and single player
    if (!cg->spar.match.paused && !cg->spar.match.online)
    {
        cg->spar.match.usTimer += elapsedUs;
    }
    // If enough us have built up, increments seconds timer
    if (cg->spar.match.usTimer >= CG_SECOND)
    {
        cg->spar.match.usTimer -= CG_SECOND;
        cg->spar.match.timer += 1;
    }
    // If enough seconds have passed, end match
    if (cg->spar.match.timer >= cg->spar.match.maxTime)
    {
        // TODO: end the match
    }

    // Loop over both Chowa
    cg_sparPlayerInput(cg);
    if (cg->spar.match.online)
    {
        // TODO: Get input from other swadge
    }
    else
    {
        // TODO: Enemy AI
    }
    cg_sparChowaState(cg, elapsedUs);

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

    // Exhausted
    // Chowa sits down
    // Stamina starts increasing slowly
    // A bar appears showing how fast it's going to get up
    // Player can influence both bars via button presses
    // Once either bar fills, CHowa stands up enters the getting ready phase

    // RANDOMNESS
    // Need to add some randomness to checks
}

/**
 * @brief Gets the player's input during the match
 *
 * @param cg Game data
 */
static void cg_sparPlayerInput(cGrove_t* cg)
{
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        // Pause or unpause regardless of state
        if (evt.down && evt.button & PB_START)
        {
            cg->spar.match.paused = true;
        }

        // Only accept correct inputs per state
        switch (cg->spar.match.chowaData[0].currState)
        {
            case CG_UNREADY:
            {
                if (!cg->spar.match.paused && evt.down && !(evt.button & PB_SELECT || evt.button & PB_START))
                {
                    // Increase readiness
                    cg->spar.match.chowaData[0].readiness += 1;

                    // Change action based on
                    switch (evt.button)
                    {
                        case PB_A:
                        {
                            if (cg->spar.match.chowaData[0].currMove != CG_SPAR_HEADBUTT)
                            {
                                cg->spar.match.chowaData[0].readiness -= 25;
                            }
                            cg->spar.match.chowaData[0].currMove = CG_SPAR_HEADBUTT;
                            break;
                        }
                        case PB_B:
                        {
                            if (cg->spar.match.chowaData[0].currMove != CG_SPAR_DODGE)
                            {
                                cg->spar.match.chowaData[0].readiness -= 25;
                            }
                            cg->spar.match.chowaData[0].currMove = CG_SPAR_DODGE;
                            break;
                        }
                        case PB_UP:
                        {
                            if (cg->spar.match.chowaData[0].currMove != CG_SPAR_PUNCH)
                            {
                                cg->spar.match.chowaData[0].readiness -= 25;
                            }
                            cg->spar.match.chowaData[0].currMove = CG_SPAR_PUNCH;
                            break;
                        }
                        case PB_DOWN:
                        {
                            if (cg->spar.match.chowaData[0].currMove != CG_SPAR_FAST_PUNCH)
                            {
                                cg->spar.match.chowaData[0].readiness -= 25;
                            }
                            cg->spar.match.chowaData[0].currMove = CG_SPAR_FAST_PUNCH;
                            break;
                        }
                        case PB_LEFT:
                        {
                            if (cg->spar.match.chowaData[0].currMove != CG_SPAR_KICK)
                            {
                                cg->spar.match.chowaData[0].readiness -= 25;
                            }
                            cg->spar.match.chowaData[0].currMove = CG_SPAR_KICK;
                            break;
                        }
                        case PB_RIGHT:
                        {
                            if (cg->spar.match.chowaData[0].currMove != CG_SPAR_JUMP_KICK)
                            {
                                cg->spar.match.chowaData[0].readiness -= 25;
                            }
                            cg->spar.match.chowaData[0].currMove = CG_SPAR_JUMP_KICK;
                            break;
                        }
                        default:
                        {
                            break;
                        }
                    }
                }
                break;
            }
            case CG_EXHAUSTED:
            {
                // Steadily regain stamina and readiness to stand up
                if (!cg->spar.match.paused && evt.down && !(evt.button & PB_SELECT || evt.button & PB_START))
                {
                    // Change action based on
                    switch (evt.button)
                    {
                        case PB_A:
                        {
                            cg->spar.match.chowaData[0].readiness += 1;
                            break;
                        }
                        case PB_B:
                        {
                            cg->spar.match.chowaData[0].stamina += 1;
                            break;
                        }
                        default:
                        {
                            break;
                        }
                    }
                }
                break;
            }
            default:
            {
                // If won, lost, or resolving
                break;
            }
        }
    }
}

/**
 * @brief Updates the states of the Chowa
 *
 * @param cg Game data
 * @param elapsedUs Time since last frame
 */
static void cg_sparChowaState(cGrove_t* cg, int64_t elapsedUs)
{
    for (int32_t idx = 0; idx < 2; idx++)
    {
        switch (cg->spar.match.chowaData[idx].currState)
        {
            case CG_UNREADY:
            {
                // Steadily becomes more ready
                cg->spar.match.chowaData[idx].updateTimer += elapsedUs;
                int32_t readinessTick
                    = CG_SECOND - (CG_STAMINA_SCALAR * cg->spar.match.chowaData[idx].chowa->stats[CG_SPEED]);
                if (cg->spar.match.chowaData[idx].updateTimer >= readinessTick)
                {
                    cg->spar.match.chowaData[idx].updateTimer = 0;
                    cg->spar.match.chowaData[idx].readiness += 1;
                }
                // Check if ready
                if (cg->spar.match.chowaData[idx].readiness > 255)
                {
                    cg->spar.match.chowaData[idx].readiness   = 255;
                    cg->spar.match.chowaData[idx].updateTimer = 0;
                    cg->spar.match.chowaData[idx].currState   = CG_READY;
                }
                else if (cg->spar.match.chowaData[idx].readiness < 0)
                {
                    cg->spar.match.chowaData[idx].readiness = 0;
                }
                break;
            }
            case CG_READY:
            {
                // Runs timer until automatically resolve
                cg->spar.match.chowaData[idx].updateTimer += elapsedUs;
                int32_t readinessTick = CG_SECOND;
                if (cg->spar.match.chowaData[idx].updateTimer >= readinessTick)
                {
                    cg->spar.match.chowaData[idx].updateTimer = 0;
                    cg->spar.match.chowaData[idx].stamina -= 24;
                    // TODO: Resolve 
                    if (cg->spar.match.chowaData[idx].stamina <= 0)
                    {
                        cg->spar.match.chowaData[idx].currState = CG_EXHAUSTED;
                        cg->spar.match.chowaData[idx].updateTimer = 0;
                        cg->spar.match.chowaData[idx].readiness   = 200;
                    } else {
                        cg->spar.match.chowaData[idx].currState = CG_UNREADY;
                        cg->spar.match.chowaData[idx].updateTimer = 0;
                        cg->spar.match.chowaData[idx].readiness   = 0;
                    }
                }
                break;
            }
            case CG_EXHAUSTED:
            {
                // Steadily regain stamina and readiness to stand up
                // Steadily becomes more ready
                cg->spar.match.chowaData[idx].updateTimer += elapsedUs;
                int32_t readinessTick
                    = CG_SECOND  - (CG_STAMINA_SCALAR * cg->spar.match.chowaData[idx].chowa->stats[CG_SPEED]);
                if (cg->spar.match.chowaData[idx].updateTimer >= readinessTick)
                {
                    cg->spar.match.chowaData[idx].updateTimer = 0;
                    cg->spar.match.chowaData[idx].readiness += 1;
                    cg->spar.match.chowaData[idx].stamina += 1;
                }
                // Check if ready
                if (cg->spar.match.chowaData[idx].readiness > 255
                    || cg->spar.match.chowaData[idx].stamina >= cg->spar.match.chowaData[idx].maxStamina)
                {
                    if (cg->spar.match.chowaData[idx].stamina > cg->spar.match.chowaData[idx].maxStamina)
                    {
                        cg->spar.match.chowaData[idx].stamina = cg->spar.match.chowaData[idx].maxStamina;
                    }
                    cg->spar.match.chowaData[idx].readiness   = 0;
                    cg->spar.match.chowaData[idx].updateTimer = 0;
                    cg->spar.match.chowaData[idx].currState   = CG_UNREADY;
                }
                else if (cg->spar.match.chowaData[idx].readiness < 0)
                {
                    cg->spar.match.chowaData[idx].readiness = 0;
                }
                break;
            }
            default:
            {
                // If won, lost, or resolving
                break;
            }
        }
    }
}