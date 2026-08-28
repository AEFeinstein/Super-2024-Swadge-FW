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
#define SCORE_PUDDLE_SMALL  150
#define SCORE_PUDDLE_MEDIUM 250
#define SCORE_PUDDLE_LARGE  350
// Divider
#define SCORE_BROKEN_DIVIDER  50
#define SCORE_MISSING_DIVIDER 100
// Urinal
#define SCORE_AUTOFLUSH    -10
#define SCORE_MINOR_ISSUE  50
#define SCORE_MAJOR_ISSUE  300
#define SCORE_SMALL_URINAL 200
#define SCORE_END_URINAL   -20

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

/**
 * @brief Set up all data for HS tables at the start of the mode
 * 
 * @param hs score table
 * @param table Which table to reference
 */
static void initHSTable(ggHSTable_t* hs, ggNVSEnum_t table);

/**
 * @brief Initializes a table with the best five of each score type
 * 
 * @param mode Swadgemode to provide modename
 * @param tables A set of tables to fill up
 */
static void initHSTableFomSwadgepass(swadgeMode_t* mode, ggHSTable_t* tables);

/**
 * @brief Updates the table via bubble sort (?)
 * 
 * @param hs score table
 * @param newScore Newest score to add
 * @param packedName Packed representation of the username
 * @return true If there was an update to the table
 * @return false If there was not an update to the table
 */
static bool updateHSTable(ggHSTable_t* hs, int newScore, int32_t packedName);

/**
 * @brief Updates score of user
 * 
 * @param hs score table
 * @param newScore Newest score to add
 * @param table Which table to reference
 */
static void updateHighScore(ggHSTable_t* hs, int newScore, ggNVSEnum_t table);

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
    ggd->levelScore = accuracy * ((ggd->timeLimit + TIMER_BUFFER) - MAX(ggd->timer, TIMER_BUFFER)) / ggd->timeLimit;
    ggd->totalScore += ggd->levelScore;
    // Calc adjusted score
    ggd->adjScore = (ggd->accScore * ggd->totalScore) / MAX_PERCENTAGE;
}

void ggCalcUrinalScores(ggUrinal_t* urinals, int numActive, int* urinalScores, int* best, int* worst)
{
    int pick[MAX_URINALS]   = {0}; // Score this urinal has due to issues
    int nextTo[MAX_URINALS] = {0}; // nextTo: Score this urinal receives from neighbors
    pick[0]                 = SCORE_END_URINAL;
    pick[numActive - 1]     = SCORE_END_URINAL;
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

// High score

void ggInitHighScores(swadgeMode_t* mode, ggData_t* ggd)
{
    // From NVS
    initHSTable(&ggd->tables[0], GG_MAX_LEVELS_HS);
    initHSTable(&ggd->tables[1], GG_MAX_ACC_HS);
    initHSTable(&ggd->tables[2], GG_MAX_SCORE_HS);
    initHSTable(&ggd->tables[3], GG_ADJ_SCORE_HS);

    // From Swadgepass
    ggHSTable_t tables[4] = {0};
    initHSTableFomSwadgepass(mode, tables);

    // Merge new score table into table loaded from NVS
    for (int idx = 0; idx < 4; idx++)
    {
        for (int i = 0; i < MAX_SCORES; i++)
        {
            updateHSTable(&ggd->tables[idx], tables[idx].scores[i].score, tables[idx].scores[i].packedName);
        }
    }
}

void ggNewUserScore(ggData_t* ggd)
{
    // Max levels
    updateHighScore(&ggd->tables[0], ggd->numLevels, GG_MAX_LEVELS_HS);

    // Accuracy
    updateHighScore(&ggd->tables[1], ggd->accScore, GG_MAX_ACC_HS);

    // Total Score
    updateHighScore(&ggd->tables[2], ggd->totalScore, GG_MAX_SCORE_HS);

    // Adjusted score
    updateHighScore(&ggd->tables[3], ggd->adjScore, GG_ADJ_SCORE_HS);
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
    *best     = 0;    // Worst case value
    *worst    = 1000; // Worst case value
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

static void initHSTable(ggHSTable_t* hs, ggNVSEnum_t table)
{
    size_t nvsLength = sizeof(hs->scores);
    if (!readNamespaceNvsBlob(ggNVSSpace[GG_NAMESPACE], ggNVSSpace[table], hs->scores, &nvsLength))
    {
        for (int idx = 0; idx < MAX_SCORES; idx++)
        {
            hs->scores[idx].score      = 0;
            hs->scores[idx].packedName = 0xFFFFFFFF;
        }
        writeNamespaceNvsBlob(ggNVSSpace[GG_NAMESPACE], ggNVSSpace[table], hs->scores, nvsLength);
    }
}

static void initHSTableFomSwadgepass(swadgeMode_t* mode, ggHSTable_t* tables)
{
    list_t spList = {0};
    getSwadgePasses(&spList, mode, false);
    node_t* currNode = spList.first;
    switchToSpeaker();
    dacStop();
    while (currNode)
    {
        swadgePassData_t* data     = (swadgePassData_t*)currNode->val;
        swadgePassPacket_t* packet = &data->data.packet;
        updateHSTable(&tables[0], packet->gottaGo.maxLevels, packet->swadgesona.core.packedName);
        updateHSTable(&tables[1], packet->gottaGo.accuracy, packet->swadgesona.core.packedName);
        updateHSTable(&tables[2], packet->gottaGo.totalScore, packet->swadgesona.core.packedName);
        updateHSTable(&tables[3], packet->gottaGo.adjScore, packet->swadgesona.core.packedName);
        setPacketUsedByMode(data, mode, true);
    }
    dacStart();
}

static bool updateHSTable(ggHSTable_t* hs, int newScore, int32_t packedName)
{
    int currScore = MAX_SCORES - 1;
    bool new      = false;
    while (hs->scores[currScore].score < newScore && currScore >= 0)
    {
        // Swap
        int tempScore                    = hs->scores[currScore].score;
        int32_t tempName                 = hs->scores[currScore].packedName;
        hs->scores[currScore].score      = newScore;
        hs->scores[currScore].packedName = packedName;
        if (currScore != MAX_SCORES - 1)
        {
            // If item hasn't fallen off list, save it
            hs->scores[currScore + 1].score      = tempScore;
            hs->scores[currScore + 1].packedName = tempName;
        }
        // Check the next rung
        currScore--;
        new = true;
    }
    return new;
}

static void updateHighScore(ggHSTable_t* hs, int newScore, ggNVSEnum_t table)
{
    nameData_t nd      = *getSystemUsername();
    int32_t packedName = GET_PACKED_USERNAME(nd);
    if (updateHSTable(hs, newScore, packedName))
    {
        size_t nvsLength = sizeof(hs) * MAX_SCORES;
        writeNamespaceNvsBlob(ggNVSSpace[GG_NAMESPACE], ggNVSSpace[table], hs, nvsLength);
    }
}