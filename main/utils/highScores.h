/*! \file highScores.h
 *
 * A SwadgePass-aware high score persistence utility.
 *
 * \section overview Overview
 *
 * This util is intended to make saving a high score table as painless as possible. Features:
 * - Save a configurable number of high scores to NVS, up to ::MAX_HIGH_SCORE_COUNT
 * - Include scores from SwadgePass in high score table, max one per SwadgePass user
 * - Keep at least one score from this Swadge's user, even if ~~they suck~~ SwadgePass scores would overtake theirs
 *
 * \section implementing Implementing
 *
 * The steps to add high scores to your mode are:
 * -# Instantiate a ::highScores_t struct and set ::highScores_t.highScoreCount
 * -# Call initHighScores()
 * -# Retrieve SwadgePasses and pass them to SAVE_HIGH_SCORES_FROM_SWADGE_PASS(), ideally once on mode start
 * -# In your mode's `fnAddToSwadgePassPacket`, call WRITE_HIGH_SCORE_TO_SWADGE_PASS_PACKET()
 * -# When the player has finished a game, submit their score(s) via updateHighScores()
 * -# The high score array in ::highScores_t.highScores will always be up-to-date, so it can be used to display high
 * scores without any other considerations. See nameList.h for how to get display names from
 * ::score_t.swadgePassUsername.
 */

#pragma once
#include <stdbool.h>
#include <stdint.h>

#define MAX_HIGH_SCORE_COUNT    20
#define NVS_KEY_USER_HIGH_SCORE "user_high_score"
#define NVS_KEY_HIGH_SCORES     "high_scores"

typedef struct
{
    /// The point value of this score. This will be `0` for uninitialized scores.
    int32_t score;
    /// The SwadgePass user who achieved this score, or `0` for this Swadge's user
    int32_t swadgePassUsername;
} score_t;

typedef struct
{
    /// Number of high scores to save. Defaults to ::MAX_HIGH_SCORE_COUNT if not set.
    uint8_t highScoreCount;
    /// The highest score achieved by this Swadge's user, for sending via SwadgePass and
    /// ensuring we don't kick ourselves off of the leaderboard.
    int32_t userHighScore;
    /// High score list, sorted from high to low
    score_t highScores[MAX_HIGH_SCORE_COUNT];
} highScores_t;

/**
 * @brief Load high score data from NVS.
 *
 * @param hs The struct that contains the high scores. ::highScores_t.highScoreCount should be set; other fields will be
 * overwritten
 * @param nvsNamespace The NVS namespace to read saved high score data from
 */
void initHighScores(highScores_t* hs, const char* nvsNamespace);

/**
 * @brief Update the high score table and user high score with new scores from the player or SwadgePass and persist to
 * NVS. This function handles sorting and limiting the number of scores, so no preprocessing is necessary.
 *
 * @param hs The struct that contains the high scores
 * @param nvsNamespace The NVS namespace to write data to
 * @param newScores Array of scores to add to the table
 * @param numNewScores Count of scores in the array
 * @return `true` if the high score table changed, `false` if it didn't
 */
bool updateHighScores(highScores_t* hs, const char* nvsNamespace, score_t newScores[], uint8_t numNewScores);

/**
 * @brief Convenience macro to save high score data received from SwadgePass. This should be called from your mode's
 * `fnEnterMode` function after calling initHighScores().
 *
 * @param hs The ::highScores_t struct that contains the high scores
 * @param nvsNamespace The NVS namespace to write data to
 * @param swadgePasses A ::list_t of SwadgePasses from getSwadgePasses()
 * @param highScoreVar The field in the SwadgePass packet that holds the high score
 */
#define SAVE_HIGH_SCORES_FROM_SWADGE_PASS(hs, nvsNamespace, swadgePasses, highScoreVar) \
    do                                                                                  \
    {                                                                                   \
        if (swadgePasses.length > 0)                                                    \
        {                                                                               \
            score_t spScores[swadgePasses.length];                                      \
            int i        = 0;                                                           \
            node_t* node = swadgePasses.first;                                          \
            while (node)                                                                \
            {                                                                           \
                swadgePassData_t* spd          = node->val;                             \
                spScores[i].score              = spd->data.packet.highScoreVar;         \
                spScores[i].swadgePassUsername = spd->data.packet.username;             \
                i++;                                                                    \
                node = node->next;                                                      \
            }                                                                           \
            updateHighScores(hs, nvsNamespace, spScores, ARRAY_SIZE(spScores));         \
        }                                                                               \
    } while (0)

/**
 * @brief Convenience macro to write high score data to SwadgePass packet for sending to other swadges. This should be
 * called from your mode's `fnAddToSwadgePassPacket` function.
 *
 * @param nvsNamespace The NVS namespace to read data from
 * @param highScoreVar A field in your mode's struct defined in ::swadgePassPacket_t
 */
#define WRITE_HIGH_SCORE_TO_SWADGE_PASS_PACKET(nvsNamespace, highScoreVar)     \
    do                                                                         \
    {                                                                          \
        int32_t highScore = 0;                                                 \
        readNamespaceNvs32(nvsNamespace, NVS_KEY_USER_HIGH_SCORE, &highScore); \
        highScoreVar = highScore;                                              \
    } while (0)
