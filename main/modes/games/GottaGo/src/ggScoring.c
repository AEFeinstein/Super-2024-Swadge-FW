#include "ggScoring.h"

//==============================================================================
// Defines
//==============================================================================

// Timer
#define TIMER_BUFFER (GG_SECOND) ///< Grace period before timer starts to penalize player

// Score values
#define MAX_SCORE      1000 ///< Default score
#define MAX_PERCENTAGE 1000 ///< Used to define percentage scale
// NPC
#define SCORE_BASE_NPC         200
#define SCORE_WEIRD_ADJUSTMENT ((SCORE_BASE_NPC * 3) / 2) ///< 150% score penalty for weirdness
// Puddle
#define SCORE_PUDDLE_SMALL  100
#define SCORE_PUDDLE_MEDIUM 200
#define SCORE_PUDDLE_LARGE  300
// Divider
#define SCORE_BROKEN_DIVIDER  50
#define SCORE_MISSING_DIVIDER 100
// Urinal
#define SCORE_AUTOFLUSH    -10
#define SCORE_MINOR_ISSUE  50
#define SCORE_MAJOR_ISSUE  300
#define SCORE_SMALL_URINAL 200

//==============================================================================
// Consts
//==============================================================================

static const char* const ggNVSSpace[] = {
    "ggSaves", "mLevels", "mLevelsHS", "acc", "accHS", "score", "scoreHS", "adjScore", "adjScoreHS",
};

//==============================================================================
// Enums
//==============================================================================

// NVS Keys
typedef enum
{
    GG_NAMESPACE,
    GG_MAX_LEVELS,
    GG_MAX_LEVELS_HS,
    GG_MAX_ACC,
    GG_MAX_ACC_HS,
    GG_MAX_SCORE,
    GG_MAX_SCORE_HS,
    GG_ADJ_SCORE,
    GG_ADJ_SCORE_HS,
} ggNVSEnum_t;

//==============================================================================
// Function Declarations
//==============================================================================

/**
 * @brief Calculate the NPC's score
 *
 * @param npc NPC to evaluate
 * @param distance How far from a particular urinal
 * @return int Final score this NPC contributes
 */
static int calcNPCScore(ggNPC_t* npc, int distance);

/**
 * @brief Get the best and worst options set
 *
 * @param urinals List of urinals to evaluate
 * @param numActive Number of active urinals
 * @param urinalScores Array of scores, one for each urinal in order from left to right
 * @param best Best possible score
 * @param worst Worst possible score
 */
static void getBestWorstOption(ggUrinal_t* urinals, int numActive, int* urinalScores, int* best, int* worst);

//==============================================================================
// Functions
//==============================================================================

void ggCalcFinalScores(ggData_t* ggd, int* urinalScores, int* best, int* worst)
{
    ggCalcUrinalScores(ggd->urinals, ggd->numActive, urinalScores, best, worst);

    // Accuracy
    int accuracy = 0;
    if (ggd->stallUsed)
    { // Using a stall gives you a perfect score
        accuracy = MAX_SCORE;
    }
    else
    {
        accuracy = (MAX_SCORE * urinalScores[ggd->selection]) / *best;
    }
    // Average accuracy
    ggd->accScore = (ggd->accScore * (ggd->numLevels - 1) + accuracy) / ggd->numLevels;
    // Calc total score
    ggd->totalScore
        += accuracy * ((ggd->timeLimit + TIMER_BUFFER) - MAX(ggd->timeRemaining, TIMER_BUFFER)) / ggd->timeLimit;
    // Calc adjusted score
    ggd->adjScore = (ggd->accScore * ggd->totalScore) / MAX_PERCENTAGE;
    // Check if trophies have been triggered
    ggSaveFinalToNVS(ggd);
}

void ggCalcUrinalScores(ggUrinal_t* urinals, int numActive, int* urinalScores, int* best, int* worst)
{
    int pick[MAX_URINALS]   = {0}; // Score this urinal has due to issues
    int nextTo[MAX_URINALS] = {0}; // nextTo: Score this urinal receives from neighbors

    // NPCs
    for (int idx = 0; idx < numActive; idx++)
    {
        // Leftmost
        int pos = 1;
        for (int i = idx - 1; i >= 0; i--)
        {
            if (urinals[i].npc.active)
            {
                nextTo[idx] += calcNPCScore(&urinals[i].npc, pos);
                break;
            }
            else
            {
                pos++;
            }
        }
        // Rightmost
        pos = 1;
        for (int i = idx + 1; i < numActive; i++)
        {
            if (urinals[i].npc.active)
            {
                nextTo[idx] += calcNPCScore(&urinals[i].npc, pos);
                break;
            }
            else
            {
                pos++;
            }
        }
    }
    // Puddles
    for (int idx = 0; idx < numActive; idx++)
    {
        // Left
        if (idx < numActive - 1)
        {
            switch (urinals[idx].puddle)
            {
                case GG_PEE_SMALL:
                {
                    nextTo[idx + 1] += SCORE_PUDDLE_SMALL / 2;
                    break;
                }
                case GG_PEE_MED:
                {
                    nextTo[idx + 1] += SCORE_PUDDLE_MEDIUM / 2;
                    break;
                }
                case GG_PEE_LARGE:
                {
                    nextTo[idx + 1] += SCORE_PUDDLE_LARGE / 2;
                    break;
                }
                case GG_PEE_NONE:
                default:
                {
                    break;
                }
            }
        }
        // Right
        if (idx > 0)
        {
            switch (urinals[idx].puddle)
            {
                case GG_PEE_SMALL:
                {
                    nextTo[idx - 1] += SCORE_PUDDLE_SMALL / 2;
                    break;
                }
                case GG_PEE_MED:
                {
                    nextTo[idx - 1] += SCORE_PUDDLE_MEDIUM / 2;
                    break;
                }
                case GG_PEE_LARGE:
                {
                    nextTo[idx - 1] += SCORE_PUDDLE_LARGE / 2;
                    break;
                }
                case GG_PEE_NONE:
                default:
                {
                    break;
                }
            }
        }
        // Center
        switch (urinals[idx].puddle)
        {
            case GG_PEE_SMALL:
            {
                pick[idx] += SCORE_PUDDLE_SMALL;
                break;
            }
            case GG_PEE_MED:
            {
                pick[idx] += SCORE_PUDDLE_MEDIUM;
                break;
            }
            case GG_PEE_LARGE:
            {
                pick[idx] += SCORE_PUDDLE_LARGE;
                break;
            }
            case GG_PEE_NONE:
            default:
            {
                break;
            }
        }
    }
    // Dividers
    for (int idx = 0; idx < numActive; idx++)
    {
        switch (urinals[idx].divider)
        {
            case GG_DIV_FULL:
            {
                break;
            }
            case GG_DIV_TOP:
            case GG_DIV_BOTTOM:
            {
                pick[idx] += SCORE_BROKEN_DIVIDER;
                break;
            }
            case GG_DIV_NONE:
            {
                pick[idx] += SCORE_MISSING_DIVIDER;
                break;
            }
        }
        if (idx != 0)
        {
            switch (urinals[idx - 1].divider)
            {
                case GG_DIV_FULL:
                {
                    break;
                }
                case GG_DIV_TOP:
                case GG_DIV_BOTTOM:
                {
                    pick[idx] += SCORE_BROKEN_DIVIDER;
                    break;
                }
                case GG_DIV_NONE:
                {
                    pick[idx] += SCORE_MISSING_DIVIDER;
                    break;
                }
            }
        }
    }
    // Urinals
    for (int idx = 0; idx < numActive; idx++)
    {
        // Only if you pick the urinal
        pick[idx] += (urinals[idx].autoFlush) ? SCORE_AUTOFLUSH : 0;
        pick[idx] += (urinals[idx].graffiti > 0) ? SCORE_MINOR_ISSUE : 0;
        pick[idx] += (urinals[idx].cracks > 0) ? SCORE_MINOR_ISSUE : 0;
        pick[idx] += (urinals[idx].waterLeak) ? SCORE_MINOR_ISSUE : 0;
        pick[idx] += (urinals[idx].brokenBowl) ? SCORE_MAJOR_ISSUE : 0;
        pick[idx] += (urinals[idx].brokenDrain) ? SCORE_MAJOR_ISSUE : 0;
        pick[idx] += (urinals[idx].outOfOrder) ? SCORE_MAJOR_ISSUE : 0;
        pick[idx] += (urinals[idx].pluggedDrain) ? SCORE_MAJOR_ISSUE : 0;
        pick[idx] += (urinals[idx].small) ? SCORE_SMALL_URINAL : 0;

        // If you're next to the urinal
        // Urinal to the right
        if (idx < numActive - 1 && urinals[idx].pluggedDrain)
        {
            nextTo[idx + 1] += SCORE_MAJOR_ISSUE / 2;
        }
        // Urinal to the left
        if (idx > 0 && urinals[idx].pluggedDrain)
        {
            nextTo[idx - 1] += SCORE_MAJOR_ISSUE / 2;
        }
    }
    // Save values out
    for (int idx = 0; idx < numActive; idx++)
    {
        urinalScores[idx] = MAX_SCORE - (pick[idx] + nextTo[idx]);
    }
    getBestWorstOption(urinals, numActive, urinalScores, best, worst);
}

// NVS
void ggSaveFinalToNVS(ggData_t* ggd)
{
    int outVal;
    if (!readNamespaceNvs32(ggNVSSpace[GG_NAMESPACE], ggNVSSpace[GG_MAX_LEVELS], &outVal))
    {
        writeNamespaceNvs32(ggNVSSpace[GG_NAMESPACE], ggNVSSpace[GG_MAX_LEVELS], ggd->numLevels);
    }
    else
    {
        if (ggd->numLevels > outVal)
        {
            writeNamespaceNvs32(ggNVSSpace[GG_NAMESPACE], ggNVSSpace[GG_MAX_LEVELS], ggd->numLevels);
        }
    }
    if (!readNamespaceNvs32(ggNVSSpace[GG_NAMESPACE], ggNVSSpace[GG_MAX_ACC], &outVal))
    {
        writeNamespaceNvs32(ggNVSSpace[GG_NAMESPACE], ggNVSSpace[GG_MAX_ACC], ggd->accScore);
    }
    else
    {
        if (ggd->accScore > outVal)
        {
            writeNamespaceNvs32(ggNVSSpace[GG_NAMESPACE], ggNVSSpace[GG_MAX_ACC], ggd->accScore);
        }
    }
    if (!readNamespaceNvs32(ggNVSSpace[GG_NAMESPACE], ggNVSSpace[GG_MAX_SCORE], &outVal))
    {
        writeNamespaceNvs32(ggNVSSpace[GG_NAMESPACE], ggNVSSpace[GG_MAX_SCORE], ggd->totalScore);
    }
    else
    {
        if (ggd->totalScore > outVal)
        {
            writeNamespaceNvs32(ggNVSSpace[GG_NAMESPACE], ggNVSSpace[GG_MAX_SCORE], ggd->totalScore);
        }
    }
    if (!readNamespaceNvs32(ggNVSSpace[GG_NAMESPACE], ggNVSSpace[GG_ADJ_SCORE], &outVal))
    {
        writeNamespaceNvs32(ggNVSSpace[GG_NAMESPACE], ggNVSSpace[GG_ADJ_SCORE], ggd->adjScore);
    }
    else
    {
        if (ggd->adjScore > outVal)
        {
            writeNamespaceNvs32(ggNVSSpace[GG_NAMESPACE], ggNVSSpace[GG_ADJ_SCORE], ggd->adjScore);
        }
    }
}

// Static functions

static int calcNPCScore(ggNPC_t* npc, int distance)
{
    int score = SCORE_BASE_NPC;
    // No Shirt, stinks, has pants around ankles, no pants, or underwear
    if (!npc->shirt || npc->pants == GG_NO_PANTS || npc->pants == GG_UNDERWEAR || npc->pants == GG_DOWN || npc->stink)
    {
        score += SCORE_WEIRD_ADJUSTMENT;
    }
    return score / distance;
}

static void getBestWorstOption(ggUrinal_t* urinals, int numActive, int* urinalScores, int* best, int* worst)
{
    // Init
    int start = 0;
    *best      = 0;    // Worst case value
    *worst     = 1000; // Worst case value
    while (urinals[start].npc.active)
    { // Best toilet can't be occupied
        start++;
    }
    *best = urinalScores[start];
    // Loop
    for (int idx = start; idx < numActive; idx++)
    {
        if (urinals[idx].npc.active)
        {
            continue;
        }
        if (*best < urinalScores[idx]
            && !(urinals[idx].brokenBowl || urinals[idx].brokenDrain || urinals[idx].outOfOrder
                 || urinals[idx].pluggedDrain))
        {
            *best = urinalScores[idx];
        }
        if (*worst > urinalScores[idx])
        {
            *worst = urinalScores[idx];
        }
    }
}