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
#include "esp_random.h"

//==============================================================================
// Defines
//==============================================================================

// Microsecond based
#define SECOND         1000000 // One second of us
#define STAMINA_SCALAR 3900    // How fast the stamina regenerates

// Min values
#define MIN_STAMINA 40  // Minimum value for stamina
#define MIN_DAMAGE  10  // Minimum Damage an attack can do
#define MIN_HP      250 // Minimum HP for Chowa to have

// Readiness penalty
#define READINESS_PENALTY 10 // How much changing the move costs

// AI
#define CHEER            2 // Amount to cheer
#define TICKS_UNTIL_PEEK 8 // How fast the AI cheats

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
static void cg_sparMatchAI(cGrove_t* cg, int64_t elapsedUs);
static void cg_sparMatchGetAdvantagedMove(cGrove_t* cg, cgRPSState_t move);
static void cg_sparMatchAITimer(cGrove_t* cg, int32_t tick);
static void cg_sparMatchAIEvalPrevMoves(cGrove_t* cg, int8_t numOfMove);
static void cg_sparMatchChowaState(cGrove_t* cg, int64_t elapsedUs);
static void cg_sparMatchResolve(cGrove_t* cg);
int8_t static cg_sparMatchStaminaCost(cgRPSState_t rps);
void static cg_sparMatchResolveState(cGrove_t* cg);
void static cg_sparMatchRPS(cGrove_t* cg);

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
                      int16_t maxTime, cgAIDifficulty_t ai)
{
    // Initialize
    // Load tournament/match data
    strcpy(cg->spar.match.matchName, matchName);
    cg->spar.match.round    = round;
    cg->spar.match.maxTime  = maxTime;
    cg->spar.match.animDone = false;
    cg->spar.match.done     = false;

    // Timer is set to 0
    cg->spar.match.timer = 0;

    // AI
    cg->spar.match.ai.aiDifficulty = ai;

    // Chowa
    cg->spar.match.chowaData[CG_P1].chowa = player1Chowa;
    cg->spar.match.chowaData[CG_P2].chowa = player2Chowa;
    for (int32_t i = 0; i < 2; i++)
    {
        // FIXME: Setting stats for test purposes
        cg->spar.match.chowaData[i].chowa->stats[CG_SPEED]    = 128;
        cg->spar.match.chowaData[i].chowa->stats[CG_STAMINA]  = 128;
        cg->spar.match.chowaData[i].chowa->stats[CG_STRENGTH] = 0;
        cg->spar.match.chowaData[i].chowa->stats[CG_AGILITY]  = 128;
        cg->spar.match.chowaData[i].chowa->stats[CG_HEALTH]   = 128;
        cg->spar.match.chowaData[i].chowa->playerAffinity     = 255;
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
    // Check if match has ended
    if (cg->spar.match.done == true)
    {
        // TODO: wait until animations finish, then save data and load next match or show a summary
        return;
    }

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
        cg->spar.match.done        = true;
        cg->spar.match.finalResult = CG_DRAW;
        return;
    }

    // Loop over both Chowa
    cg_sparMatchPlayerInput(cg);
    if (cg->spar.match.online)
    {
        // TODO: Get input from other swadge
    }
    else
    {
        cg_sparMatchAI(cg, elapsedUs);
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
    // Once animations are done, set state to unready or exhausted as appropriate
    if (cg->spar.match.animDone)
    {
        cg->spar.match.animDone = false;
        cg_sparMatchResolveState(cg);
    }

    // End game if either health is Zero
    if (cg->spar.match.chowaData[CG_P1].HP <= 0 || cg->spar.match.chowaData[CG_P2].HP <= 0)
    {
        if (cg->spar.match.chowaData[CG_P1].HP <= 0 && cg->spar.match.chowaData[CG_P2].HP <= 0)
        {
            cg->spar.match.finalResult = CG_DRAW;
        }
        else if (cg->spar.match.chowaData[CG_P1].HP <= 0)
        {
            cg->spar.match.finalResult = CG_P2_WIN;
        }
        else if (cg->spar.match.chowaData[CG_P2].HP <= 0)
        {
            cg->spar.match.finalResult = CG_P1_WIN;
        }

        // End the game
        cg->spar.match.done = true;
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
            cg->spar.match.paused = !cg->spar.match.paused;
        }

        // Only accept correct inputs per state
        switch (cg->spar.match.chowaData[0].currState)
        {
            case CG_SPAR_UNREADY:
            {
                if (!cg->spar.match.paused && evt.down && !(evt.button & PB_SELECT || evt.button & PB_START))
                {
                    // Increase readiness
                    cg->spar.match.chowaData[0].readiness
                        += 1 + (cg->spar.match.chowaData[CG_P1].chowa->playerAffinity >> 6);

                    // Change action based on
                    switch (evt.button)
                    {
                        case PB_A:
                        {
                            if (cg->spar.match.chowaData[CG_P1].currMove != CG_SPAR_HEADBUTT
                                && cg->spar.match.chowaData[CG_P1].currMove != CG_SPAR_UNSET)
                            {
                                cg->spar.match.chowaData[CG_P1].readiness -= READINESS_PENALTY;
                            }
                            cg->spar.match.chowaData[CG_P1].currMove = CG_SPAR_HEADBUTT;
                            break;
                        }
                        case PB_B:
                        {
                            if (cg->spar.match.chowaData[CG_P1].currMove != CG_SPAR_DODGE
                                && cg->spar.match.chowaData[CG_P1].currMove != CG_SPAR_UNSET)
                            {
                                cg->spar.match.chowaData[CG_P1].readiness -= READINESS_PENALTY;
                            }
                            cg->spar.match.chowaData[CG_P1].currMove = CG_SPAR_DODGE;
                            break;
                        }
                        case PB_UP:
                        {
                            if (cg->spar.match.chowaData[CG_P1].currMove != CG_SPAR_PUNCH
                                && cg->spar.match.chowaData[CG_P1].currMove != CG_SPAR_UNSET)
                            {
                                cg->spar.match.chowaData[CG_P1].readiness -= READINESS_PENALTY;
                            }
                            cg->spar.match.chowaData[CG_P1].currMove = CG_SPAR_PUNCH;
                            break;
                        }
                        case PB_DOWN:
                        {
                            if (cg->spar.match.chowaData[CG_P1].currMove != CG_SPAR_FAST_PUNCH
                                && cg->spar.match.chowaData[CG_P1].currMove != CG_SPAR_UNSET)
                            {
                                cg->spar.match.chowaData[CG_P1].readiness -= READINESS_PENALTY;
                            }
                            cg->spar.match.chowaData[CG_P1].currMove = CG_SPAR_FAST_PUNCH;
                            break;
                        }
                        case PB_LEFT:
                        {
                            if (cg->spar.match.chowaData[CG_P1].currMove != CG_SPAR_KICK
                                && cg->spar.match.chowaData[CG_P1].currMove != CG_SPAR_UNSET)
                            {
                                cg->spar.match.chowaData[CG_P1].readiness -= READINESS_PENALTY;
                            }
                            cg->spar.match.chowaData[CG_P1].currMove = CG_SPAR_KICK;
                            break;
                        }
                        case PB_RIGHT:
                        {
                            if (cg->spar.match.chowaData[CG_P1].currMove != CG_SPAR_JUMP_KICK
                                && cg->spar.match.chowaData[CG_P1].currMove != CG_SPAR_UNSET)
                            {
                                cg->spar.match.chowaData[CG_P1].readiness -= READINESS_PENALTY;
                            }
                            cg->spar.match.chowaData[CG_P1].currMove = CG_SPAR_JUMP_KICK;
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
                            cg->spar.match.chowaData[CG_P1].readiness
                                += 1 + (cg->spar.match.chowaData[CG_P1].chowa->playerAffinity >> 6);
                            break;
                        }
                        case PB_B:
                        {
                            cg->spar.match.chowaData[CG_P1].stamina
                                += 1 + (cg->spar.match.chowaData[CG_P1].chowa->playerAffinity >> 6);
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
 * @brief Handles the AI's behavior
 *
 * @param cg Game data
 * @param elapsedUs Time since last frame
 */
static void cg_sparMatchAI(cGrove_t* cg, int64_t elapsedUs)
{
    switch (cg->spar.match.ai.aiDifficulty)
    {
        case CG_BEGINNER:
        {
            // Sometimes pick a move. Otherwise, just sit there like a lump
            if (!cg->spar.match.ai.pickedMove)
            {
                cg->spar.match.ai.pickedMove             = true;
                cg->spar.match.chowaData[CG_P2].currMove = (esp_random() % 2 == 0) ? esp_random() % 5 : CG_SPAR_UNSET;
            }
            break;
        }
        case CG_VERY_EASY:
        {
            // Rotates between all five moves in order, starting on a random one
            if (!cg->spar.match.ai.pickedMove)
            {
                cg->spar.match.ai.pickedMove = true;
                if (!cg->spar.match.ai.init)
                {
                    cg->spar.match.ai.init                   = true;
                    cg->spar.match.ai.prevMoves[0]           = esp_random() % 5;
                    cg->spar.match.chowaData[CG_P2].currMove = cg->spar.match.ai.prevMoves[0];
                }
                cg->spar.match.chowaData[CG_P2].currMove = (cg->spar.match.ai.prevMoves[0]++) % 5;
            }
            break;
        }
        case CG_EASY:
        {
            // Randomly select three of the five moves, pick them randomly as well each time
            if (!cg->spar.match.ai.pickedMove || cg->spar.match.chowaData[CG_P2].currMove == CG_SPAR_UNSET)
            {
                cg->spar.match.ai.pickedMove = true;
                if (!cg->spar.match.ai.init || cg->spar.match.ai.movesPicked < 3)
                {
                    cg->spar.match.ai.init                   = true;
                    cg->spar.match.chowaData[CG_P2].currMove = esp_random() % 5;
                    cg->spar.match.ai.prevMoves[cg->spar.match.ai.movesPicked]
                        = cg->spar.match.chowaData[CG_P2].currMove;
                    cg->spar.match.ai.movesPicked++;
                }
                else
                {
                    cg->spar.match.chowaData[CG_P2].currMove = cg->spar.match.ai.prevMoves[esp_random() % 3];
                }
            }
            break;
        }
        case CG_MEDIUM:
        {
            // Pick move randomly unless player picked two in a row, Starts motivating lazily (1x a second)
            // Rarely dodge
            if (!cg->spar.match.ai.pickedMove || cg->spar.match.chowaData[CG_P2].currMove == CG_SPAR_UNSET)
            {
                cg->spar.match.ai.pickedMove = true;
                if (!cg->spar.match.ai.init)
                {
                    cg->spar.match.ai.init         = true;
                    cg->spar.match.ai.prevMoves[0] = esp_random() % 5;
                }
                if (cg->spar.match.ai.prevMoves[1] == cg->spar.match.ai.prevMoves[0])
                {
                    cg_sparMatchGetAdvantagedMove(cg, cg->spar.match.ai.prevMoves[0]);
                }
                else
                {
                    // Random
                    cg->spar.match.chowaData[CG_P2].currMove = esp_random() % 5;
                }
                // Dodge
                if (esp_random() % 10 == 0)
                {
                    cg->spar.match.chowaData[CG_P2].currMove = CG_SPAR_DODGE;
                }
            }
            // Timer
            cg->spar.match.ai.timer += elapsedUs;
            cg_sparMatchAITimer(cg, SECOND);
            break;
        }
        case CG_HARD:
        {
            // Pick a move that counters the most of the previous 10 player moves. Motivate 2x a sec
            // Dodge if low on health
            if (!cg->spar.match.ai.pickedMove || cg->spar.match.chowaData[CG_P2].currMove == CG_SPAR_UNSET)
            {
                cg->spar.match.ai.pickedMove = true;
                if (!cg->spar.match.ai.init)
                {
                    cg->spar.match.ai.init = true;
                    for (int32_t idx = 0; idx < 10; idx++)
                    {
                        cg->spar.match.ai.prevMoves[idx] = esp_random() % 5;
                    }
                }
                // Interpret data
                cg_sparMatchAIEvalPrevMoves(cg, 10);

                // Dodge
                if (esp_random() % 8 == 0)
                {
                    cg->spar.match.chowaData[CG_P2].currMove = CG_SPAR_DODGE;
                }
            }

            // Timer
            cg->spar.match.ai.timer += elapsedUs;
            cg_sparMatchAITimer(cg, SECOND >> 1);
            break;
        }
        case CG_VERY_HARD:
        {
            // Same as hard, but dodge chance is higher and ai motivates 3x a sec
            if (!cg->spar.match.ai.pickedMove || cg->spar.match.chowaData[CG_P2].currMove == CG_SPAR_UNSET)
            {
                cg->spar.match.ai.pickedMove = true;
                if (!cg->spar.match.ai.init)
                {
                    cg->spar.match.ai.init = true;
                    for (int32_t idx = 0; idx < 20; idx++)
                    {
                        cg->spar.match.ai.prevMoves[idx] = esp_random() % 5;
                    }
                }
                // Interpret data
                cg_sparMatchAIEvalPrevMoves(cg, 20);

                // Dodge
                if (esp_random() % 6 == 0)
                {
                    cg->spar.match.chowaData[CG_P2].currMove = CG_SPAR_DODGE;
                }
            }
            // Timer
            cg->spar.match.ai.timer += elapsedUs;
            cg_sparMatchAITimer(cg, SECOND / 3);
            break;
        }
        case CG_EXPERT:
        {
            // Cheats and reads players input. Checks every 4s what the player is doing and counter-picks.
            // Timer
            cg->spar.match.ai.timer += elapsedUs;
            if (cg->spar.match.ai.timer >= SECOND >> 2)
            {
                cg->spar.match.ai.movesPicked++; // Used as a counter
            }
            cg_sparMatchAITimer(cg, SECOND >> 2);

            if (cg->spar.match.chowaData[CG_P2].currMove == CG_SPAR_UNSET)
            {
                cg->spar.match.chowaData[CG_P2].currMove = esp_random() % 6;
            }

            if (cg->spar.match.ai.movesPicked >= TICKS_UNTIL_PEEK)
            {
                cg->spar.match.ai.movesPicked = 0;
                cg_sparMatchGetAdvantagedMove(cg, cg->spar.match.chowaData[CG_P1].currMove);
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
 * @brief Gets an advantaged move given an input move
 *
 * @param cg Game Data
 * @param move Move to attempt to beat
 */
static void cg_sparMatchGetAdvantagedMove(cGrove_t* cg, cgRPSState_t move)
{
    // Pick a counter
    switch (move)
    {
        case CG_SPAR_PUNCH:
        {
            cg->spar.match.chowaData[CG_P2].currMove = (esp_random() % 2 == 0) ? CG_SPAR_FAST_PUNCH : CG_SPAR_HEADBUTT;
            break;
        }
        case CG_SPAR_FAST_PUNCH:
        {
            cg->spar.match.chowaData[CG_P2].currMove = (esp_random() % 2 == 0) ? CG_SPAR_KICK : CG_SPAR_JUMP_KICK;
            break;
        }
        case CG_SPAR_KICK:
        {
            cg->spar.match.chowaData[CG_P2].currMove = (esp_random() % 2 == 0) ? CG_SPAR_PUNCH : CG_SPAR_HEADBUTT;
            break;
        }
        case CG_SPAR_HEADBUTT:
        {
            cg->spar.match.chowaData[CG_P2].currMove = (esp_random() % 2 == 0) ? CG_SPAR_FAST_PUNCH : CG_SPAR_JUMP_KICK;
            break;
        }
        case CG_SPAR_JUMP_KICK:
        {
            cg->spar.match.chowaData[CG_P2].currMove = (esp_random() % 2 == 0) ? CG_SPAR_PUNCH : CG_SPAR_KICK;
            break;
        }
        default:
        {
            cg->spar.match.chowaData[CG_P2].currMove = esp_random() % 5;
            break;
        }
    }
}

/**
 * @brief Handles updating the time and responding appropriately
 *
 * @param cg Game Data
 * @param tick The amount of time between checks
 */
static void cg_sparMatchAITimer(cGrove_t* cg, int32_t tick)
{
    if (cg->spar.match.ai.timer >= tick)
    {
        cg->spar.match.ai.timer -= tick;
        if (cg->spar.match.chowaData[CG_P2].currState == CG_SPAR_UNREADY)
        {
            cg->spar.match.chowaData[CG_P2].readiness += CHEER;
        }
        else if (cg->spar.match.chowaData[CG_P2].currState == CG_SPAR_EXHAUSTED)
        {
            cg->spar.match.chowaData[CG_P2].stamina += CHEER;
        }
    }
}

/**
 * @brief Evaluate past moves
 * 
 * @param cg Game Data
 * @param numOfMove Past moves to evaluate
 */
static void cg_sparMatchAIEvalPrevMoves(cGrove_t* cg, int8_t numOfMove)
{
    // Count how many of each are in the provided range
    cgRPSState_t chosenMove =  CG_SPAR_UNSET;
    int8_t moveCount[7] = {0};

    for (int32_t idx = 0; idx < numOfMove; idx++)
    {
        // Get counts
        moveCount[cg->spar.match.ai.prevMoves[idx]]++;
    }
    for (int32_t idx = 0; idx < 5; idx++)
    {
        // Find largest
        if (chosenMove < moveCount[idx])
        {
            chosenMove = idx;
        }
        else if (chosenMove == moveCount[idx])
        {
            chosenMove = (esp_random() % 2 == 0) ? chosenMove : idx;
        }
    }

    // Pick best option
    cg_sparMatchGetAdvantagedMove(cg, chosenMove);
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
                if (!cg->spar.match.paused && !cg->spar.match.online)
                {
                    cg->spar.match.chowaData[idx].updateTimer += elapsedUs;
                }
                int32_t readinessTick
                    = SECOND - (STAMINA_SCALAR * cg->spar.match.chowaData[idx].chowa->stats[CG_SPEED]);
                if (cg->spar.match.chowaData[idx].updateTimer >= readinessTick)
                {
                    cg->spar.match.chowaData[idx].updateTimer = 0;
                    cg->spar.match.chowaData[idx].readiness += 1;
                }
                // Check if ready
                if (cg->spar.match.chowaData[idx].readiness > CG_MAX_READY_VALUE)
                {
                    cg->spar.match.chowaData[idx].readiness   = CG_MAX_READY_VALUE;
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
                if (!cg->spar.match.paused && !cg->spar.match.online)
                {
                    cg->spar.match.chowaData[idx].updateTimer += elapsedUs;
                }
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
                if (!cg->spar.match.paused && !cg->spar.match.online)
                {
                    cg->spar.match.chowaData[idx].updateTimer += elapsedUs;
                }
                int32_t readinessTick
                    = SECOND - (STAMINA_SCALAR * cg->spar.match.chowaData[idx].chowa->stats[CG_SPEED]);
                if (cg->spar.match.chowaData[idx].updateTimer >= readinessTick)
                {
                    cg->spar.match.chowaData[idx].updateTimer = 0;
                    cg->spar.match.chowaData[idx].readiness += 1;
                    cg->spar.match.chowaData[idx].stamina += 1;
                }
                // Check if ready
                if (cg->spar.match.chowaData[idx].readiness > CG_MAX_READY_VALUE
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

/**
 * @brief Evaluates the result of the preparation stage
 *
 * @param cg Game Data
 */
static void cg_sparMatchResolve(cGrove_t* cg)
{
    // Unset resolved
    cg->spar.match.resolve = false;

    // FIXME: Only set animation to done when animations finish
    cg->spar.match.animDone = true;

    // Check if both Chowa are ready
    for (int32_t idx = 0; idx < 2; idx++)
    {
        if (cg->spar.match.chowaData[idx].currState != CG_SPAR_READY
            && !(cg->spar.match.chowaData[(idx + 1) % 2].currMove == CG_SPAR_DODGE))
        {
            // This Chowa is not ready
            // Reduce health based on other Chowa's Str
            cg->spar.match.chowaData[idx].HP
                -= MIN_DAMAGE + (cg->spar.match.chowaData[(idx + 1) % 2].chowa->stats[CG_STRENGTH] << 1);

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
            if (!(cg->spar.match.chowaData[(idx + 1) % 2].currMove == CG_SPAR_UNSET))
            {
                cg->spar.match.chowaData[(idx + 1) % 2].stamina
                    -= MIN_STAMINA_COST + cg_sparMatchStaminaCost(cg->spar.match.chowaData[(idx + 1) % 2].currMove);
            }
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
    cg_sparMatchRPS(cg);
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
    // Update AI
    if (!cg->spar.match.online)
    {
        cg->spar.match.ai.pickedMove = false;
        if (cg->spar.match.ai.aiDifficulty == CG_MEDIUM)
        {
            cg->spar.match.ai.prevMoves[1] = cg->spar.match.ai.prevMoves[0];
            cg->spar.match.ai.prevMoves[0] = cg->spar.match.chowaData[CG_P1].currMove;
        }
        else if (cg->spar.match.ai.aiDifficulty == CG_HARD)
        {
            for (int32_t idx = 9; idx > 1; idx--)
            {
                cg->spar.match.ai.prevMoves[idx] = cg->spar.match.ai.prevMoves[idx - 1];
            }
            cg->spar.match.ai.prevMoves[0] = cg->spar.match.chowaData[CG_P1].currMove;
        }
        else if (cg->spar.match.ai.aiDifficulty == CG_VERY_HARD)
        {
            for (int32_t idx = 19; idx > 1; idx--)
            {
                cg->spar.match.ai.prevMoves[idx] = cg->spar.match.ai.prevMoves[idx - 1];
            }
            cg->spar.match.ai.prevMoves[0] = cg->spar.match.chowaData[CG_P1].currMove;
        }
        else if (cg->spar.match.ai.aiDifficulty == CG_EXPERT)
        {
            cg->spar.match.ai.movesPicked = 0;
        }
    }

    // Set the state
    for (int32_t idx = 0; idx < 2; idx++)
    {
        // Change Chowa State
        if (cg->spar.match.chowaData[idx].stamina <= 0)
        {
            cg->spar.match.chowaData[idx].stamina   = 0;
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

/**
 * @brief Evaluate RPS
 *
 * @param cg Game Data
 */
void static cg_sparMatchRPS(cGrove_t* cg)
{
    // Check RPS
    cgRPSState_t player1Move = cg->spar.match.chowaData[CG_P1].currMove;
    cgRPSState_t player2Move = cg->spar.match.chowaData[CG_P2].currMove;

    cgWinLoss_t winner = CG_DRAW;

    // DRAW
    if (player1Move == player2Move)
    {
        winner = CG_DRAW;
        if (player1Move == CG_SPAR_UNSET)
        {
            cg->spar.match.chowaData[0].currState = CG_SPAR_NOTHING;
            cg->spar.match.chowaData[1].currState = CG_SPAR_NOTHING;
        }
        else
        {
            for (int idx = 0; idx < 2; idx++)
            {
                // Do a little damage
                cg->spar.match.chowaData[idx].HP -= 10;

                // Drain stamina
                cg->spar.match.chowaData[idx].stamina -= 10;

                // Both do attack animation
                cg->spar.match.chowaData[idx].currState = CG_SPAR_ATTACK;
            }
        }
        return;
    }

    // Player one doesn't try to play
    else if (player1Move == CG_SPAR_UNSET)
    {
        winner = CG_P2_WIN;
    }

    // PUNCH
    else if (player1Move == CG_SPAR_PUNCH
             && (player2Move == CG_SPAR_PUNCH || player2Move == CG_SPAR_KICK || player2Move == CG_SPAR_JUMP_KICK))
    {
        winner = CG_P1_WIN;
    }
    else if (player1Move == CG_SPAR_PUNCH && (player2Move == CG_SPAR_FAST_PUNCH || player2Move == CG_SPAR_HEADBUTT))
    {
        winner = CG_P2_WIN;
    }

    // FAST PUNCH
    else if (player1Move == CG_SPAR_FAST_PUNCH
             && (player2Move == CG_SPAR_PUNCH || player2Move == CG_SPAR_HEADBUTT || player2Move == CG_SPAR_UNSET))
    {
        winner = CG_P1_WIN;
    }
    else if (player1Move == CG_SPAR_FAST_PUNCH && (player2Move == CG_SPAR_KICK || player2Move == CG_SPAR_JUMP_KICK))
    {
        winner = CG_P2_WIN;
    }

    // KICK
    else if (player1Move == CG_SPAR_KICK
             && (player2Move == CG_SPAR_FAST_PUNCH || player2Move == CG_SPAR_JUMP_KICK || player2Move == CG_SPAR_UNSET))
    {
        winner = CG_P1_WIN;
    }
    else if (player1Move == CG_SPAR_KICK && (player2Move == CG_SPAR_PUNCH || player2Move == CG_SPAR_HEADBUTT))
    {
        winner = CG_P2_WIN;
    }

    // HEADBUTT
    else if (player1Move == CG_SPAR_HEADBUTT
             && (player2Move == CG_SPAR_PUNCH || player2Move == CG_SPAR_KICK || player2Move == CG_SPAR_UNSET))
    {
        winner = CG_P1_WIN;
    }
    else if (player1Move == CG_SPAR_HEADBUTT && (player2Move == CG_SPAR_FAST_PUNCH || player2Move == CG_SPAR_JUMP_KICK))
    {
        winner = CG_P2_WIN;
    }

    // JUMP KICK
    else if (player1Move == CG_SPAR_JUMP_KICK
             && (player2Move == CG_SPAR_FAST_PUNCH || player2Move == CG_SPAR_HEADBUTT || player2Move == CG_SPAR_UNSET))
    {
        winner = CG_P1_WIN;
    }
    else if (player1Move == CG_SPAR_JUMP_KICK && (player2Move == CG_SPAR_PUNCH || player2Move == CG_SPAR_KICK))
    {
        winner = CG_P2_WIN;
    }

    // Set index for winner and loser for data crunching
    int8_t winIdx, loseIdx;
    if (winner == CG_P1_WIN)
    {
        winIdx  = CG_P1;
        loseIdx = CG_P2;
    }
    else if (winner == CG_P2_WIN)
    {
        winIdx  = CG_P2;
        loseIdx = CG_P1;
    }
    else
    {
        // Something went wrong
        printf("Draw propagated to far");
        return;
    }

    // Damage
    if ((esp_random() % 10) == 0)
    {
        // Crit
        cg->spar.match.chowaData[loseIdx].HP
            -= MIN_DAMAGE + (cg->spar.match.chowaData[winIdx].chowa->stats[CG_STRENGTH] << 1);
        cg->spar.match.wasCrit = true;
    }
    else
    {
        cg->spar.match.chowaData[loseIdx].HP
            -= MIN_DAMAGE + (cg->spar.match.chowaData[winIdx].chowa->stats[CG_STRENGTH]);
    }

    // Stamina
    cg->spar.match.chowaData[loseIdx].stamina
        -= MIN_STAMINA_COST + cg_sparMatchStaminaCost(cg->spar.match.chowaData[loseIdx].currMove);
    cg->spar.match.chowaData[winIdx].stamina
        -= MIN_STAMINA_COST + cg_sparMatchStaminaCost(cg->spar.match.chowaData[winIdx].currMove);

    // Set state of both Chowa
    cg->spar.match.chowaData[loseIdx].currState = CG_SPAR_HIT;
    cg->spar.match.chowaData[winIdx].currState  = CG_SPAR_ATTACK;
}