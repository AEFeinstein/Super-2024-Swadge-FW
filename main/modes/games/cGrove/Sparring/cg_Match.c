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

// Microsecond based
#define SECOND         1000000 // One second of us
#define STAMINA_SCALAR 60000   // How fast the stamina regenerates

// Min values
#define MIN_STAMINA 40  // Minimum value for stamina
#define MIN_DAMAGE  10  // Minimum Damage an attack can do
#define MIN_HP      250 // Minimum HP for Chowa to have

// Stamina costs
#define MIN_STAMINA_COST          25 // Minumum Stamina loss
#define HEADBUTT_STAMINA_COST     20 // Cost to headbutt
#define PUNCH_STAMINA_COST        20 // Cost to Punch
#define FAST_PUNCH_STAMINA_COST   15 // Cost to Fast Punch
#define KICK_STAMINA_COST         15 // Cost to Kick
#define JUMPING_KICK_STAMINA_COST 25 // Cost to Jumping Kick
#define DODGE_STAMINA_COST        30 // Cost to Dodge

//==============================================================================
// Static Functions
//==============================================================================

static void cg_sparMatchPlayerInput(cGrove_t* cg);
static void cg_sparMatchChowaState(cGrove_t* cg, int64_t elapsedUs);
static void cg_sparMatchResolve(cGrove_t* cg);
int8_t static cg_sparMatchStaminaCost(cgRPSState_t rps);
void static cg_sparMatchResolveState(cGrove_t* cg);

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
void cg_initSparMatch(cGrove_t* cg, char* matchName, cgChowa_t* player1Chowa, cgChowa_t* player2Chowa, int8_t round,
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
        // FIXME: Setting stats for test purposes
        cg->spar.match.chowaData[i].chowa->stats[CG_SPEED]    = 128;
        cg->spar.match.chowaData[i].chowa->stats[CG_STAMINA]  = 128;
        cg->spar.match.chowaData[i].chowa->stats[CG_STRENGTH] = 128;
        cg->spar.match.chowaData[i].chowa->stats[CG_AGILITY]  = 128;
        cg->spar.match.chowaData[i].chowa->stats[CG_HEALTH]   = 128;
        // Charisma not needed for spar

        cg->spar.match.chowaData[i].currState = CG_SPAR_UNREADY;
        cg->spar.match.chowaData[i].currMove  = CG_SPAR_UNSET;
        cg->spar.match.chowaData[i].maxStamina
            = MIN_STAMINA + (cg->spar.match.chowaData[i].chowa->stats[CG_STAMINA] >> 1);
        cg->spar.match.chowaData[i].stamina   = cg->spar.match.chowaData[i].maxStamina;
        cg->spar.match.chowaData[i].readiness = 0;
        cg->spar.match.chowaData[i].readiness = 0;
        cg->spar.match.chowaData[i].maxHP
            = MIN_HP + cg->spar.match.chowaData[i].chowa->stats[CG_HEALTH] * 3; // 250 - 1000
        cg->spar.match.chowaData[i].HP          = cg->spar.match.chowaData[i].maxHP;
        cg->spar.match.chowaData[i].updateTimer = 0;
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
    if (cg->spar.match.usTimer >= SECOND)
    {
        cg->spar.match.usTimer -= SECOND;
        cg->spar.match.timer += 1;
    }
    // If enough seconds have passed, end match
    if (cg->spar.match.timer >= cg->spar.match.maxTime)
    {
        // TODO: end the match
    }

    // Loop over both Chowa
    cg_sparMatchPlayerInput(cg);
    if (cg->spar.match.online)
    {
        // TODO: Get input from other swadge
    }
    else
    {
        // TODO: Enemy AI
    }
    cg_sparMatchChowaState(cg, elapsedUs);

    // Resolve is both are ready or if one has been ready for a full second
    if (cg->spar.match.resolve
        || (cg->spar.match.chowaData[0].currState == CG_SPAR_READY
            && cg->spar.match.chowaData[1].currState == CG_SPAR_READY))
    {
        cg_sparMatchResolve(cg);
    }

    // Animate Chowa
    if (cg->spar.match.animating)
    {
        // TODO: Animation timer
        // Once animations are done, set state to unready or exhausted as appropriate
        cg->spar.match.animating = false;
        cg_sparMatchResolveState(cg);
    }
}

/**
 * @brief Gets the player's input during the match
 *
 * @param cg Game data
 */
static void cg_sparMatchPlayerInput(cGrove_t* cg)
{
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        // Pause or unpause regardless of state
        if (evt.down && evt.button & PB_START)
        {
            cg->spar.match.paused = true;
        }

        // FIXME: Make it so the numbers increase faster if the relationship w/ player is better

        // Only accept correct inputs per state
        switch (cg->spar.match.chowaData[0].currState)
        {
            case CG_SPAR_UNREADY:
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
                            if (cg->spar.match.chowaData[0].currMove != CG_SPAR_HEADBUTT
                                && cg->spar.match.chowaData[0].currMove != CG_SPAR_UNSET)
                            {
                                cg->spar.match.chowaData[0].readiness -= 25;
                            }
                            cg->spar.match.chowaData[0].currMove = CG_SPAR_HEADBUTT;
                            break;
                        }
                        case PB_B:
                        {
                            if (cg->spar.match.chowaData[0].currMove != CG_SPAR_DODGE
                                && cg->spar.match.chowaData[0].currMove != CG_SPAR_UNSET)
                            {
                                cg->spar.match.chowaData[0].readiness -= 25;
                            }
                            cg->spar.match.chowaData[0].currMove = CG_SPAR_DODGE;
                            break;
                        }
                        case PB_UP:
                        {
                            if (cg->spar.match.chowaData[0].currMove != CG_SPAR_PUNCH
                                && cg->spar.match.chowaData[0].currMove != CG_SPAR_UNSET)
                            {
                                cg->spar.match.chowaData[0].readiness -= 25;
                            }
                            cg->spar.match.chowaData[0].currMove = CG_SPAR_PUNCH;
                            break;
                        }
                        case PB_DOWN:
                        {
                            if (cg->spar.match.chowaData[0].currMove != CG_SPAR_FAST_PUNCH
                                && cg->spar.match.chowaData[0].currMove != CG_SPAR_UNSET)
                            {
                                cg->spar.match.chowaData[0].readiness -= 25;
                            }
                            cg->spar.match.chowaData[0].currMove = CG_SPAR_FAST_PUNCH;
                            break;
                        }
                        case PB_LEFT:
                        {
                            if (cg->spar.match.chowaData[0].currMove != CG_SPAR_KICK
                                && cg->spar.match.chowaData[0].currMove != CG_SPAR_UNSET)
                            {
                                cg->spar.match.chowaData[0].readiness -= 25;
                            }
                            cg->spar.match.chowaData[0].currMove = CG_SPAR_KICK;
                            break;
                        }
                        case PB_RIGHT:
                        {
                            if (cg->spar.match.chowaData[0].currMove != CG_SPAR_JUMP_KICK
                                && cg->spar.match.chowaData[0].currMove != CG_SPAR_UNSET)
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
            case CG_SPAR_EXHAUSTED:
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
static void cg_sparMatchChowaState(cGrove_t* cg, int64_t elapsedUs)
{
    for (int32_t idx = 0; idx < 2; idx++)
    {
        switch (cg->spar.match.chowaData[idx].currState)
        {
            case CG_SPAR_UNREADY:
            {
                // Steadily becomes more ready
                cg->spar.match.chowaData[idx].updateTimer += elapsedUs;
                int32_t readinessTick
                    = SECOND - (STAMINA_SCALAR * cg->spar.match.chowaData[idx].chowa->stats[CG_SPEED]);
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
                    cg->spar.match.chowaData[idx].currState   = CG_SPAR_READY;
                }
                else if (cg->spar.match.chowaData[idx].readiness < 0)
                {
                    cg->spar.match.chowaData[idx].readiness = 0;
                }
                break;
            }
            case CG_SPAR_READY:
            {
                // Runs timer until automatically resolve
                cg->spar.match.chowaData[idx].updateTimer += elapsedUs;
                int32_t readinessTick = SECOND;
                if (cg->spar.match.chowaData[idx].updateTimer >= readinessTick)
                {
                    // Resolve
                    cg->spar.match.resolve                    = true;
                    cg->spar.match.chowaData[idx].updateTimer = 0;
                }
                break;
            }
            case CG_SPAR_EXHAUSTED:
            {
                // Steadily regain stamina and readiness to stand up
                // Steadily becomes more ready
                cg->spar.match.chowaData[idx].updateTimer += elapsedUs;
                int32_t readinessTick
                    = SECOND - (STAMINA_SCALAR * cg->spar.match.chowaData[idx].chowa->stats[CG_SPEED]);
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
                    cg->spar.match.chowaData[idx].currState   = CG_SPAR_UNREADY;
                    cg->spar.match.chowaData[idx].currMove    = CG_SPAR_UNSET;
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

//
static void cg_sparMatchResolve(cGrove_t* cg)
{
    // Check RPS
    // Winner's attack stat is compared with Los67er's agility with some randomness
    // If executed a move, burn stamina in accordance with move

    // Unset resolved
    cg->spar.match.resolve   = false;
    cg->spar.match.animating = true;

    // Check if both Chowa are ready
    for (int32_t idx = 0; idx < 2; idx++)
    {
        if (cg->spar.match.chowaData[idx].currState != CG_SPAR_READY)
        {
            // This Chowa is not ready
            // Reduce health based on other Chowa's Str
            cg->spar.match.chowaData[idx].HP
                -= MIN_DAMAGE + (cg->spar.match.chowaData[(idx + 1) % 2].chowa->stats[CG_STRENGTH] << 2);

            // Reduce Stamina
            cg->spar.match.chowaData[idx].stamina -= MIN_STAMINA_COST;
            cg->spar.match.chowaData[(idx + 1) % 2].stamina
                -= MIN_STAMINA_COST + cg_sparMatchStaminaCost(cg->spar.match.chowaData[(idx + 1) % 2].currMove);

            // Set state for animations
            cg->spar.match.chowaData[idx].currState           = CG_SPAR_HIT;
            cg->spar.match.chowaData[(idx + 1) % 2].currState = CG_SPAR_ATTACK;
            cg->spar.match.wasCrit                            = true;
            return;
        }
    }

    // Check if either one is dodging
    for (int32_t idx = 0; idx < 2; idx++)
    {
        if (cg->spar.match.chowaData[idx].currMove == CG_SPAR_DODGE)
        {
            // Adjust stamina
            cg->spar.match.chowaData[idx].stamina -= (MIN_STAMINA_COST + DODGE_STAMINA_COST);
            cg->spar.match.chowaData[(idx + 1) % 2].stamina
                -= MIN_STAMINA_COST + cg_sparMatchStaminaCost(cg->spar.match.chowaData[(idx + 1) % 2].currMove);

            // Set state for animations
            cg->spar.match.chowaData[idx].currState = CG_SPAR_DODGE_ST;
            if (cg->spar.match.chowaData[(idx + 1) % 2].currMove == CG_SPAR_DODGE)
            {
                cg->spar.match.chowaData[(idx + 1) % 2].currState = CG_SPAR_DODGE_ST;
            }
            else
            {
                cg->spar.match.chowaData[(idx + 1) % 2].currState = CG_SPAR_ATTACK;
            }
            return; // Returns out of loop to avoid double counting
        }
    }

    // Run RPS

    // Change state
    // FIXME: Dummy to trigger updates
    cg->spar.match.chowaData[0].currState = CG_SPAR_DODGE_ST;
    cg->spar.match.chowaData[1].currState = CG_SPAR_DODGE_ST;
}

/**
 * @brief Converts the selected move into a stamina cost
 *
 * @param rps Move selected
 * @return int8_t cost of the move to execute
 */
int8_t static cg_sparMatchStaminaCost(cgRPSState_t rps)
{
    switch (rps)
    {
        case CG_SPAR_PUNCH:
        {
            return PUNCH_STAMINA_COST;
        }
        case CG_SPAR_FAST_PUNCH:
        {
            return FAST_PUNCH_STAMINA_COST;
        }
        case CG_SPAR_KICK:
        {
            return KICK_STAMINA_COST;
        }
        case CG_SPAR_JUMP_KICK:
        {
            return JUMPING_KICK_STAMINA_COST;
        }
        case CG_SPAR_DODGE:
        {
            return DODGE_STAMINA_COST;
        }
        case CG_SPAR_HEADBUTT:
        {
            return HEADBUTT_STAMINA_COST;
        }
        default:
        {
            return 0;
        }
    }
}

/**
 * @brief Changes the CHowa to the appropriate state after resolving
 *
 * @param cg Game Data
 */
void static cg_sparMatchResolveState(cGrove_t* cg)
{
    for (int32_t idx = 0; idx < 2; idx++)
    {
        if (cg->spar.match.chowaData[idx].stamina <= 0)
        {
            cg->spar.match.chowaData[idx].currState = CG_SPAR_EXHAUSTED;
            cg->spar.match.chowaData[idx].readiness = 0;
        }
        else
        {
            cg->spar.match.chowaData[idx].currState = CG_SPAR_UNREADY;
            cg->spar.match.chowaData[idx].currMove  = CG_SPAR_UNSET;
            cg->spar.match.chowaData[idx].readiness = 0;
        }
    }
}
