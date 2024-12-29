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
void cg_initSparMatch(cGrove_t* cg, int8_t round, int16_t maxTime, cgAIDifficulty_t ai, cgChowa_t* c1)
{
    // Initialize
    // Load tournament/match data
    cg->spar.match.round        = round;
    cg->spar.match.maxTime      = maxTime;
    cg->spar.match.animDone     = false;
    cg->spar.match.done         = false;
    cg->spar.match.timer = 0;
    cg->spar.match.endGameTimer = 0;

    // AI
    cg->spar.match.ai.aiDifficulty = ai;

    // Chowa
    cg->spar.match.chowa[CG_P1].chowa = &cg->chowa[0];//c1; //cg->spar.match.data.chowa[2 * round];
    cg->spar.match.chowa[CG_P2].chowa = cg->spar.match.data.chowa[(2 * round) + 1];

    // Setting stats
    for (int32_t i = 0; i < 2; i++)
    {
        cg->spar.match.chowa[i].currState = CG_SPAR_UNREADY;
        cg->spar.match.chowa[i].currMove  = CG_SPAR_UNSET;
        cg->spar.match.chowa[i].maxStamina
            = MIN_STAMINA + (cg->spar.match.chowa[i].chowa->stats[CG_STAMINA] >> 1);
        cg->spar.match.chowa[i].stamina   = cg->spar.match.chowa[i].maxStamina;
        cg->spar.match.chowa[i].readiness = 0;
        cg->spar.match.chowa[i].readiness = 0;
        cg->spar.match.chowa[i].maxHP
            = MIN_HP + cg->spar.match.chowa[i].chowa->stats[CG_HEALTH] * 3; // 250 - 1000
        cg->spar.match.chowa[i].HP          = cg->spar.match.chowa[i].maxHP;
        cg->spar.match.chowa[i].updateTimer = 0;
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
        cg->spar.match.endGameTimer += elapsedUs;
        if (cg->spar.match.endGameTimer >= 5000000)
        {
            // Move on
            cg->spar.state = CG_SPAR_MENU;
            // TODO: Assign rewards
        }
        return;
    }

    // Round timer
    // Timer doesn't accumulate if paused and single player
    if (cg->spar.match.paused)
    {
        elapsedUs = 0;
    }
    cg->spar.match.usTimer += elapsedUs;
    // If enough us have built up, increments seconds timer
    if (cg->spar.match.usTimer >= SECOND)
    {
        cg->spar.match.usTimer -= SECOND;
        cg->spar.match.timer += 1;
    }
    // If enough seconds have passed, end match
    if (cg->spar.match.timer >= cg->spar.match.maxTime)
    {
        cg->spar.match.done                              = true;
        cg->spar.match.data.result[cg->spar.match.round] = CG_DRAW;
        cg->spar.match.chowa[CG_P1].currState        = CG_SPAR_LOSE;
        cg->spar.match.chowa[CG_P2].currState        = CG_SPAR_LOSE;
        return;
    }

    // Loop over both Chowa
    cg_sparMatchPlayerInput(cg);
    cg_sparMatchAI(cg, elapsedUs);

    cg_sparMatchChowaState(cg, elapsedUs);

    // Resolve if both are ready or if one has been ready for a full second
    if (cg->spar.match.resolve
        || (cg->spar.match.chowa[0].currState == CG_SPAR_READY
            && cg->spar.match.chowa[1].currState == CG_SPAR_READY))
    {
        cg->spar.match.chowa[0].animTimer = 0;
        cg->spar.match.chowa[0].animFrame = 0;
        cg->spar.match.chowa[1].animTimer = 0;
        cg->spar.match.chowa[1].animFrame = 0;
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
    if (cg->spar.match.chowa[CG_P1].HP <= 0 || cg->spar.match.chowa[CG_P2].HP <= 0)
    {
        if (cg->spar.match.chowa[CG_P1].HP <= 0)
        {
            cg->spar.match.data.result[cg->spar.match.round] = CG_P2_WIN;
            cg->spar.match.chowa[CG_P1].currState        = CG_SPAR_LOSE;
            cg->spar.match.chowa[CG_P2].currState        = CG_SPAR_WIN;
        }
        else if (cg->spar.match.chowa[CG_P2].HP <= 0)
        {
            cg->spar.match.data.result[cg->spar.match.round] = CG_P1_WIN;
            cg->spar.match.chowa[CG_P1].currState        = CG_SPAR_WIN;
            cg->spar.match.chowa[CG_P2].currState        = CG_SPAR_LOSE;
        }

        // End the game
        cg->spar.match.endGameTimer = 0;
        cg->spar.match.done         = true;
    }
}

//==============================================================================
// Static Functions
//==============================================================================

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
        switch (cg->spar.match.chowa[CG_P1].currState)
        {
            case CG_SPAR_UNREADY:
            {
                if (!cg->spar.match.paused && evt.down && !(evt.button & PB_SELECT || evt.button & PB_START))
                {
                    // Increase readiness
                    cg->spar.match.chowa[CG_P1].readiness
                        += 1 + (cg->spar.match.chowa[CG_P1].chowa->playerAffinity >> 6);

                    // Change action based on
                    switch (evt.button)
                    {
                        case PB_A:
                        {
                            if (cg->spar.match.chowa[CG_P1].currMove != CG_SPAR_HEADBUTT
                                && cg->spar.match.chowa[CG_P1].currMove != CG_SPAR_UNSET)
                            {
                                cg->spar.match.chowa[CG_P1].readiness -= READINESS_PENALTY;
                            }
                            cg->spar.match.chowa[CG_P1].currMove = CG_SPAR_HEADBUTT;
                            break;
                        }
                        case PB_B:
                        {
                            if (cg->spar.match.chowa[CG_P1].currMove != CG_SPAR_DODGE
                                && cg->spar.match.chowa[CG_P1].currMove != CG_SPAR_UNSET)
                            {
                                cg->spar.match.chowa[CG_P1].readiness -= READINESS_PENALTY;
                            }
                            cg->spar.match.chowa[CG_P1].currMove = CG_SPAR_DODGE;
                            break;
                        }
                        case PB_UP:
                        {
                            if (cg->spar.match.chowa[CG_P1].currMove != CG_SPAR_PUNCH
                                && cg->spar.match.chowa[CG_P1].currMove != CG_SPAR_UNSET)
                            {
                                cg->spar.match.chowa[CG_P1].readiness -= READINESS_PENALTY;
                            }
                            cg->spar.match.chowa[CG_P1].currMove = CG_SPAR_PUNCH;
                            break;
                        }
                        case PB_DOWN:
                        {
                            if (cg->spar.match.chowa[CG_P1].currMove != CG_SPAR_FAST_PUNCH
                                && cg->spar.match.chowa[CG_P1].currMove != CG_SPAR_UNSET)
                            {
                                cg->spar.match.chowa[CG_P1].readiness -= READINESS_PENALTY;
                            }
                            cg->spar.match.chowa[CG_P1].currMove = CG_SPAR_FAST_PUNCH;
                            break;
                        }
                        case PB_LEFT:
                        {
                            if (cg->spar.match.chowa[CG_P1].currMove != CG_SPAR_KICK
                                && cg->spar.match.chowa[CG_P1].currMove != CG_SPAR_UNSET)
                            {
                                cg->spar.match.chowa[CG_P1].readiness -= READINESS_PENALTY;
                            }
                            cg->spar.match.chowa[CG_P1].currMove = CG_SPAR_KICK;
                            break;
                        }
                        case PB_RIGHT:
                        {
                            if (cg->spar.match.chowa[CG_P1].currMove != CG_SPAR_JUMP_KICK
                                && cg->spar.match.chowa[CG_P1].currMove != CG_SPAR_UNSET)
                            {
                                cg->spar.match.chowa[CG_P1].readiness -= READINESS_PENALTY;
                            }
                            cg->spar.match.chowa[CG_P1].currMove = CG_SPAR_JUMP_KICK;
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
                            cg->spar.match.chowa[CG_P1].readiness
                                += 1 + (cg->spar.match.chowa[CG_P1].chowa->playerAffinity >> 6);
                            break;
                        }
                        case PB_B:
                        {
                            cg->spar.match.chowa[CG_P1].stamina
                                += 1 + (cg->spar.match.chowa[CG_P1].chowa->playerAffinity >> 6);
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
                cg->spar.match.chowa[CG_P2].currMove = (esp_random() % 2 == 0) ? esp_random() % 5 : CG_SPAR_UNSET;
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
                    cg->spar.match.chowa[CG_P2].currMove = cg->spar.match.ai.prevMoves[0];
                }
                cg->spar.match.chowa[CG_P2].currMove = (cg->spar.match.ai.prevMoves[0]++) % 5;
            }
            break;
        }
        case CG_EASY:
        {
            // Randomly select three of the five moves, pick them randomly as well each time
            if (!cg->spar.match.ai.pickedMove || cg->spar.match.chowa[CG_P2].currMove == CG_SPAR_UNSET)
            {
                cg->spar.match.ai.pickedMove = true;
                if (!cg->spar.match.ai.init || cg->spar.match.ai.movesPicked < 3)
                {
                    cg->spar.match.ai.init                   = true;
                    cg->spar.match.chowa[CG_P2].currMove = esp_random() % 5;
                    cg->spar.match.ai.prevMoves[cg->spar.match.ai.movesPicked]
                        = cg->spar.match.chowa[CG_P2].currMove;
                    cg->spar.match.ai.movesPicked++;
                }
                else
                {
                    cg->spar.match.chowa[CG_P2].currMove = cg->spar.match.ai.prevMoves[esp_random() % 3];
                }
            }
            break;
        }
        case CG_MEDIUM:
        {
            // Pick move randomly unless player picked two in a row, Starts motivating lazily (1x a second)
            // Rarely dodge
            if (!cg->spar.match.ai.pickedMove || cg->spar.match.chowa[CG_P2].currMove == CG_SPAR_UNSET)
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
                    cg->spar.match.chowa[CG_P2].currMove = esp_random() % 5;
                }
                // Dodge
                if (esp_random() % 10 == 0)
                {
                    cg->spar.match.chowa[CG_P2].currMove = CG_SPAR_DODGE;
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
            if (!cg->spar.match.ai.pickedMove || cg->spar.match.chowa[CG_P2].currMove == CG_SPAR_UNSET)
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
                    cg->spar.match.chowa[CG_P2].currMove = CG_SPAR_DODGE;
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
            if (!cg->spar.match.ai.pickedMove || cg->spar.match.chowa[CG_P2].currMove == CG_SPAR_UNSET)
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
                    cg->spar.match.chowa[CG_P2].currMove = CG_SPAR_DODGE;
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

            if (cg->spar.match.chowa[CG_P2].currMove == CG_SPAR_UNSET)
            {
                cg->spar.match.chowa[CG_P2].currMove = esp_random() % 6;
            }

            if (cg->spar.match.ai.movesPicked >= TICKS_UNTIL_PEEK)
            {
                cg->spar.match.ai.movesPicked = 0;
                cg_sparMatchGetAdvantagedMove(cg, cg->spar.match.chowa[CG_P1].currMove);
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
            cg->spar.match.chowa[CG_P2].currMove = (esp_random() % 2 == 0) ? CG_SPAR_FAST_PUNCH : CG_SPAR_HEADBUTT;
            break;
        }
        case CG_SPAR_FAST_PUNCH:
        {
            cg->spar.match.chowa[CG_P2].currMove = (esp_random() % 2 == 0) ? CG_SPAR_KICK : CG_SPAR_JUMP_KICK;
            break;
        }
        case CG_SPAR_KICK:
        {
            cg->spar.match.chowa[CG_P2].currMove = (esp_random() % 2 == 0) ? CG_SPAR_PUNCH : CG_SPAR_HEADBUTT;
            break;
        }
        case CG_SPAR_HEADBUTT:
        {
            cg->spar.match.chowa[CG_P2].currMove = (esp_random() % 2 == 0) ? CG_SPAR_FAST_PUNCH : CG_SPAR_JUMP_KICK;
            break;
        }
        case CG_SPAR_JUMP_KICK:
        {
            cg->spar.match.chowa[CG_P2].currMove = (esp_random() % 2 == 0) ? CG_SPAR_PUNCH : CG_SPAR_KICK;
            break;
        }
        default:
        {
            cg->spar.match.chowa[CG_P2].currMove = esp_random() % 5;
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
        if (cg->spar.match.chowa[CG_P2].currState == CG_SPAR_UNREADY)
        {
            cg->spar.match.chowa[CG_P2].readiness += CHEER;
        }
        else if (cg->spar.match.chowa[CG_P2].currState == CG_SPAR_EXHAUSTED)
        {
            cg->spar.match.chowa[CG_P2].stamina += CHEER;
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
    cgRPSState_t chosenMove = CG_SPAR_UNSET;
    int8_t moveCount[7]     = {0};

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
        switch (cg->spar.match.chowa[idx].currState)
        {
            case CG_SPAR_UNREADY:
            {
                // Steadily becomes more ready
                if (!cg->spar.match.paused)
                {
                    cg->spar.match.chowa[idx].updateTimer += elapsedUs;
                }
                int32_t readinessTick
                    = SECOND - (STAMINA_SCALAR * cg->spar.match.chowa[idx].chowa->stats[CG_SPEED]);
                if (cg->spar.match.chowa[idx].updateTimer >= readinessTick)
                {
                    cg->spar.match.chowa[idx].updateTimer = 0;
                    cg->spar.match.chowa[idx].readiness += 1;
                }
                // Check if ready
                if (cg->spar.match.chowa[idx].readiness > CG_MAX_READY_VALUE)
                {
                    cg->spar.match.chowa[idx].readiness   = CG_MAX_READY_VALUE;
                    cg->spar.match.chowa[idx].updateTimer = 0;
                    cg->spar.match.chowa[idx].currState   = CG_SPAR_READY;
                }
                else if (cg->spar.match.chowa[idx].readiness < 0)
                {
                    cg->spar.match.chowa[idx].readiness = 0;
                }
                break;
            }
            case CG_SPAR_READY:
            {
                // Runs timer until automatically resolve
                if (!cg->spar.match.paused)
                {
                    cg->spar.match.chowa[idx].updateTimer += elapsedUs;
                }
                if (cg->spar.match.chowa[idx].updateTimer >= SECOND)
                {
                    // Resolve
                    cg->spar.match.resolve                    = true;
                    cg->spar.match.chowa[idx].updateTimer = 0;
                }
                break;
            }
            case CG_SPAR_EXHAUSTED:
            {
                // Steadily regain stamina and readiness to stand up
                // Steadily becomes more ready
                if (!cg->spar.match.paused)
                {
                    cg->spar.match.chowa[idx].updateTimer += elapsedUs;
                }
                int32_t readinessTick
                    = SECOND - (STAMINA_SCALAR * cg->spar.match.chowa[idx].chowa->stats[CG_SPEED]);
                if (cg->spar.match.chowa[idx].updateTimer >= readinessTick)
                {
                    cg->spar.match.chowa[idx].updateTimer = 0;
                    cg->spar.match.chowa[idx].readiness += 1;
                    cg->spar.match.chowa[idx].stamina += 1;
                }
                // Check if ready
                if (cg->spar.match.chowa[idx].readiness > CG_MAX_READY_VALUE
                    || cg->spar.match.chowa[idx].stamina >= cg->spar.match.chowa[idx].maxStamina)
                {
                    if (cg->spar.match.chowa[idx].stamina > cg->spar.match.chowa[idx].maxStamina)
                    {
                        cg->spar.match.chowa[idx].stamina = cg->spar.match.chowa[idx].maxStamina;
                    }
                    cg->spar.match.chowa[idx].readiness   = 0;
                    cg->spar.match.chowa[idx].updateTimer = 0;
                    cg->spar.match.chowa[idx].currState   = CG_SPAR_UNREADY;
                    cg->spar.match.chowa[idx].currMove    = CG_SPAR_UNSET;
                }
                else if (cg->spar.match.chowa[idx].readiness < 0)
                {
                    cg->spar.match.chowa[idx].readiness = 0;
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

    // Check if both Chowa are ready6
    for (int32_t idx = 0; idx < 2; idx++)
    {
        cg->spar.match.chowa[idx].animTimer = 0;
        if (cg->spar.match.chowa[idx].currState != CG_SPAR_READY
            && !(cg->spar.match.chowa[(idx + 1) % 2].currMove == CG_SPAR_DODGE))
        {
            // This Chowa is not ready
            // Reduce health based on other Chowa's Str
            cg->spar.match.chowa[idx].HP
                -= MIN_DAMAGE + (cg->spar.match.chowa[(idx + 1) % 2].chowa->stats[CG_STRENGTH] << 1);

            // Reduce Stamina
            cg->spar.match.chowa[idx].stamina -= MIN_STAMINA_COST;
            cg->spar.match.chowa[(idx + 1) % 2].stamina
                -= MIN_STAMINA_COST + cg_sparMatchStaminaCost(cg->spar.match.chowa[(idx + 1) % 2].currMove);

            // Set state for animations
            cg->spar.match.chowa[idx].currState           = CG_SPAR_HIT;
            cg->spar.match.chowa[(idx + 1) % 2].currState = CG_SPAR_ATTACK;
            return;
        }
    }

    // Check if either one is dodging
    for (int32_t idx = 0; idx < 2; idx++)
    {
        if (cg->spar.match.chowa[idx].currMove == CG_SPAR_DODGE)
        {
            // Adjust stamina
            cg->spar.match.chowa[idx].stamina -= (MIN_STAMINA_COST + DODGE_STAMINA_COST);
            if (!(cg->spar.match.chowa[(idx + 1) % 2].currMove == CG_SPAR_UNSET))
            {
                cg->spar.match.chowa[(idx + 1) % 2].stamina
                    -= MIN_STAMINA_COST + cg_sparMatchStaminaCost(cg->spar.match.chowa[(idx + 1) % 2].currMove);
            }
            // Set state for animations
            cg->spar.match.chowa[idx].currState = CG_SPAR_DODGE_ST;
            if (cg->spar.match.chowa[(idx + 1) % 2].currMove == CG_SPAR_DODGE)
            {
                cg->spar.match.chowa[(idx + 1) % 2].currState = CG_SPAR_DODGE_ST;
            }
            else
            {
                cg->spar.match.chowa[(idx + 1) % 2].currState = CG_SPAR_ATTACK;
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
    cg->spar.match.ai.pickedMove = false;
    if (cg->spar.match.ai.aiDifficulty == CG_MEDIUM)
    {
        cg->spar.match.ai.prevMoves[1] = cg->spar.match.ai.prevMoves[0];
        cg->spar.match.ai.prevMoves[0] = cg->spar.match.chowa[CG_P1].currMove;
    }
    else if (cg->spar.match.ai.aiDifficulty == CG_HARD)
    {
        for (int32_t idx = 9; idx > 1; idx--)
        {
            cg->spar.match.ai.prevMoves[idx] = cg->spar.match.ai.prevMoves[idx - 1];
        }
        cg->spar.match.ai.prevMoves[0] = cg->spar.match.chowa[CG_P1].currMove;
    }
    else if (cg->spar.match.ai.aiDifficulty == CG_VERY_HARD)
    {
        for (int32_t idx = 19; idx > 1; idx--)
        {
            cg->spar.match.ai.prevMoves[idx] = cg->spar.match.ai.prevMoves[idx - 1];
        }
        cg->spar.match.ai.prevMoves[0] = cg->spar.match.chowa[CG_P1].currMove;
    }
    else if (cg->spar.match.ai.aiDifficulty == CG_EXPERT)
    {
        cg->spar.match.ai.movesPicked = 0;
    }

    // Set the state
    for (int32_t idx = 0; idx < 2; idx++)
    {
        // Change Chowa State
        if (cg->spar.match.chowa[idx].stamina <= 0)
        {
            cg->spar.match.chowa[idx].stamina   = 0;
            cg->spar.match.chowa[idx].currState = CG_SPAR_EXHAUSTED;
            cg->spar.match.chowa[idx].readiness = 0;
        }
        else
        {
            cg->spar.match.chowa[idx].currState = CG_SPAR_UNREADY;
            cg->spar.match.chowa[idx].currMove  = CG_SPAR_UNSET;
            cg->spar.match.chowa[idx].readiness = 0;
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
    cgRPSState_t player1Move = cg->spar.match.chowa[CG_P1].currMove;
    cgRPSState_t player2Move = cg->spar.match.chowa[CG_P2].currMove;

    cgWinLoss_t winner = CG_DRAW;

    // DRAW
    if (player1Move == player2Move)
    {
        winner = CG_DRAW;
        if (player1Move == CG_SPAR_UNSET)
        {
            cg->spar.match.chowa[CG_P1].currState = CG_SPAR_NOTHING;
            cg->spar.match.chowa[CG_P2].currState = CG_SPAR_NOTHING;
        }
        else
        {
            for (int idx = 0; idx < 2; idx++)
            {
                // Do a little damage
                cg->spar.match.chowa[idx].HP -= 10;

                // Drain stamina
                cg->spar.match.chowa[idx].stamina -= 10;

                // Both do attack animation
                cg->spar.match.chowa[idx].currState = CG_SPAR_ATTACK;
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
        printf("Draw propagated too far");
        return;
    }

    // Damage
    if ((esp_random() % 10) == 0)
    {
        // Crit
        cg->spar.match.chowa[loseIdx].HP
            -= MIN_DAMAGE + (cg->spar.match.chowa[winIdx].chowa->stats[CG_STRENGTH] << 1);
    }
    else
    {
        cg->spar.match.chowa[loseIdx].HP
            -= MIN_DAMAGE + (cg->spar.match.chowa[winIdx].chowa->stats[CG_STRENGTH]);
    }

    // Stamina
    cg->spar.match.chowa[loseIdx].stamina
        -= MIN_STAMINA_COST + cg_sparMatchStaminaCost(cg->spar.match.chowa[loseIdx].currMove);
    cg->spar.match.chowa[winIdx].stamina
        -= MIN_STAMINA_COST + cg_sparMatchStaminaCost(cg->spar.match.chowa[winIdx].currMove);

    // Set state of both Chowa
    cg->spar.match.chowa[loseIdx].currState = CG_SPAR_HIT;
    cg->spar.match.chowa[winIdx].currState  = CG_SPAR_ATTACK;
}