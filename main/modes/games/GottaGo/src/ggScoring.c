#include "ggScoring.h"

//==============================================================================
// Defines
//==============================================================================

// Score values
// NPC
#define SCORE_BASE_NPC         200
#define SCORE_WEIRD_ADJUSTMENT ((SCORE_BASE_NPC * 3) / 2)
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
    "ggSaves", "mGames", "mGamesHS", "acc", "accHS", "score", "scoreHS", "adjScore", "adjScoreHS",
};

//==============================================================================
// Enums
//==============================================================================

// Game states
typedef enum
{
    GG_PEE_NONE,
    GG_PEE_SMALL,
    GG_PEE_MED,
    GG_PEE_LARGE,
} ggPeePool_t;

typedef enum
{
    GG_DIV_FULL,
    GG_DIV_TOP,
    GG_DIV_BOTTOM,
    GG_DIV_NONE,
} ggDivider_t;

typedef enum
{
    GG_PANTS,
    GG_SHORTS,
    GG_SKIRT,
    GG_DOWN,
    GG_UNDERWEAR,
    GG_NAKED,
} ggPants_t;

// NVS Keys
typedef enum
{
    GG_NAMESPACE,
    GG_MAX_GAMES,
    GG_MAX_GAMES_HS,
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

static int calcNPCScore(ggNPC_t* n, int distance);

//==============================================================================
// Functions
//==============================================================================

void ggCalcUrinalScore(ggData_t* ggd, int* value)
{
    int pick[MAX_URINALS]   = {0}; // Score this urinal has due to issues
    int nextTo[MAX_URINALS] = {0}; // nextTo: Score this urinal receives from neighbors

    // NPCs
    for (int idx = 0; idx < ggd->numActive; idx++)
    {
        // Leftmost
        int pos = 1;
        for (int i = idx - 1; i >= 0; i--)
        {
            if (ggd->urinals[i].npc.active)
            {
                nextTo[idx] += calcNPCScore(&ggd->urinals[i].npc, pos);
                break;
            }
            else
            {
                pos++;
            }
        }
        // Rightmost
        pos = 1;
        for (int i = idx + 1; i < ggd->numActive; i++)
        {
            if (ggd->urinals[i].npc.active)
            {
                nextTo[idx] += calcNPCScore(&ggd->urinals[i].npc, pos);
                break;
            }
            else
            {
                pos++;
            }
        }
    }
    // Puddles
    for (int idx = 0; idx < ggd->numActive; idx++)
    {
        ggUrinal_t* urinal = &ggd->urinals[idx];
        // Left
        if (idx < ggd->numActive - 1)
        {
            switch (urinal->puddle)
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
            switch (urinal->puddle)
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
        switch (urinal->puddle)
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
    for (int idx = 0; idx < ggd->numActive; idx++)
    {
        switch (ggd->urinals[idx].divider)
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
            switch (ggd->urinals[idx - 1].divider)
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
    for (int idx = 0; idx < ggd->numActive; idx++)
    {
        // Urinal
        ggUrinal_t* urinal = &ggd->urinals[idx];
        // Only if you pick the urinal
        pick[idx] += (urinal->autoFlush) ? SCORE_AUTOFLUSH : 0;
        pick[idx] += (urinal->graffiti > 0) ? SCORE_MINOR_ISSUE : 0;
        pick[idx] += (urinal->cracks > 0) ? SCORE_MINOR_ISSUE : 0;
        pick[idx] += (urinal->waterLeak) ? SCORE_MINOR_ISSUE : 0;
        pick[idx] += (urinal->brokenBowl) ? SCORE_MAJOR_ISSUE : 0;
        pick[idx] += (urinal->brokenDrain) ? SCORE_MAJOR_ISSUE : 0;
        pick[idx] += (urinal->outOfOrder) ? SCORE_MAJOR_ISSUE : 0;
        pick[idx] += (urinal->pluggedDrain) ? SCORE_MAJOR_ISSUE : 0;
        pick[idx] += (urinal->small) ? SCORE_SMALL_URINAL : 0;
        
        // If you're next to the urinal
        // Urinal to the right
        if (idx < ggd->numActive - 1 && urinal->pluggedDrain)
        {
            nextTo[idx + 1] += SCORE_MAJOR_ISSUE / 2;
        }
        // Urinal to the left
        if (idx > 0 && urinal->pluggedDrain)
        {
            nextTo[idx - 1] += SCORE_MAJOR_ISSUE / 2;
        }
    }
    // Save values out
    for (int idx = 0; idx < ggd->numActive; idx++)
    {
        value[idx] = pick[idx] + nextTo[idx];
    }
}

void ggSaveFinalToNVS(ggData_t* ggd)
{
    int outVal;
    if (!readNamespaceNvs32(ggNVSSpace[GG_NAMESPACE], ggNVSSpace[GG_MAX_GAMES], &outVal))
    {
        writeNamespaceNvs32(ggNVSSpace[GG_NAMESPACE], ggNVSSpace[GG_MAX_GAMES], ggd->numGames);
    }
    else
    {
        if (ggd->numGames > outVal)
        {
            writeNamespaceNvs32(ggNVSSpace[GG_NAMESPACE], ggNVSSpace[GG_MAX_GAMES], ggd->numGames);
        }
    }
    if (!readNamespaceNvs32(ggNVSSpace[GG_NAMESPACE], ggNVSSpace[GG_MAX_ACC], &outVal))
    {
        writeNamespaceNvs32(ggNVSSpace[GG_NAMESPACE], ggNVSSpace[GG_MAX_ACC], ggd->avgScore);
    }
    else
    {
        if (ggd->avgScore > outVal)
        {
            writeNamespaceNvs32(ggNVSSpace[GG_NAMESPACE], ggNVSSpace[GG_MAX_ACC], ggd->avgScore);
        }
    }
    if (!readNamespaceNvs32(ggNVSSpace[GG_NAMESPACE], ggNVSSpace[GG_MAX_SCORE], &outVal))
    {
        writeNamespaceNvs32(ggNVSSpace[GG_NAMESPACE], ggNVSSpace[GG_MAX_SCORE], ggd->score);
    }
    else
    {
        if (ggd->score > outVal)
        {
            writeNamespaceNvs32(ggNVSSpace[GG_NAMESPACE], ggNVSSpace[GG_MAX_SCORE], ggd->score);
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

static int calcNPCScore(ggNPC_t* n, int distance)
{
    int score = SCORE_BASE_NPC;
    // No Shirt, stinks, has pants around ankles, no pants, or underwear
    if (!n->shirt || n->pants == GG_NAKED || n->pants == GG_UNDERWEAR || n->pants == GG_DOWN || n->stink)
    {
        score += SCORE_WEIRD_ADJUSTMENT;
    }
    return score / distance;
}
