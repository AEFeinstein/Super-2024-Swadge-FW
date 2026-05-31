/*! \file highScores.h
 *
 * A SwadgePass-aware high score persistence utility.
 *
 * \section overview Overview
 *
 * This util is intended to make saving a high score table as painless as possible. Features:
 * - Save a configurable number of high scores to NVS, up to ::MAX_HIGH_SCORE_COUNT
 * - Include scores from SwadgePass in high score table, max one per SwadgePass user, along with Swadgesona image and
 * username data
 * - Keep at least one score from this Swadge's user, even if ~~they suck~~ SwadgePass scores would overtake theirs
 *
 * \section implementing Implementing
 *
 * The steps to add high scores to your mode are:
 * -# Instantiate a ::highScores_t struct and set ::highScores_t.highScoreCount
 * -# Call initHighScores()
 * -# Implement get and set callback functions in your mode for your ::swadgePassPacket_t high score field
 * -# Retrieve SwadgePasses and pass them to saveHighScoresFromSwadgePass(), ideally once on mode start
 * -# In your mode's ::swadgeMode.fnAddToSwadgePassPacket, call addHighScoreToSwadgePassPacket()
 * -# When the player has finished a game, submit their score(s) via updateHighScores()
 * -# The high score array in ::highScores_t.highScores will always be up-to-date, so it can be used to display high
 * scores without any other considerations. See nameList.h for how to get display names from
 * ::swadgesonaCore_t.packedName, or use ::initHighScoreSonas() to initialize names and Swadgesona images all at once.
 */

#pragma once

#include "linked_list.h"
#include "swadgePass.h"

#include <stdbool.h>
#include <stdint.h>

#define MAX_HIGH_SCORE_COUNT 20

typedef struct
{
    /// The point value of this score. This will be `0` for uninitialized scores.
    int32_t score;
    /// The SwadgePass key for this score, to ensure we only save one score from a given SP user. This will be empty if
    /// this score is from this Swadge's user and not from Swadgepass.
    char spKey[NVS_KEY_NAME_MAX_SIZE];
    /// The Swadgesona of the player who achieved this score. If the score is from this Swadge's user, this data will be
    /// all `00`; built-in methods to load the current SP Swadgesona and username should be used instead.
    swadgesonaCore_t swadgesona;
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
 * @brief Extract high score data received from SwadgePass and save to the high score table. This should be called from
 * your mode's `fnEnterMode` function after calling initHighScores().
 *
 * @param hs The ::highScores_t struct that contains the high scores
 * @param nvsNamespace The NVS namespace to write data to
 * @param swadgePasses A ::list_t of SwadgePasses from getSwadgePasses()
 * @param mode The ::swadgeMode that is consuming these SwadgePasses. They will be marked as used by this mode.
 * @param fnGetSwadgePassHighScore Pointer to a function that returns your mode's high score from a SwadgePass packet
 */
void saveHighScoresFromSwadgePass(highScores_t* hs, const char* nvsNamespace, list_t swadgePasses,
                                  const struct swadgeMode* mode,
                                  int32_t (*fnGetSwadgePassHighScore)(const swadgePassPacket_t* packet));

/**
 * @brief Write high score data to SwadgePass packet for sending to other swadges. This should be called from your
 * mode's `fnAddToSwadgePassPacket` function.
 *
 * @param nvsNamespace The NVS namespace to read data from
 * @param packet The SwadgePass packet to modify
 * @param fnSetSwadgePassHighScore Pointer to a function that sets your mode's high score field in a SwadgePass packet
 */
void addHighScoreToSwadgePassPacket(const char* nvsNamespace, swadgePassPacket_t* packet,
                                    void (*fnSetSwadgePassHighScore)(swadgePassPacket_t* packet, int32_t highScore));

/**
 * @brief Load the usernames and Swadgesona images for the high score table into memory. When you are finished with the
 * Swadgesona images, call ::freeHighScoreSonas() to free the memory. It is safe to call this function multiple times
 * before calling ::freeHighScoreSonas().
 *
 * @param hs The ::highScores_t struct that contains the high scores
 * @param sonas Array of swadgesona structs. ::nameData_t.nameBuffer in ::swadgesona_t.name and ::swadgesona_t.image
 * will be populated. This array must be the same length as ::highScores_t.highScoreCount.
 */
void initHighScoreSonas(highScores_t* hs, swadgesona_t sonas[]);

/**
 * @brief Free memory used for Swadgesona images.
 *
 * @param hs The ::highScores_t struct that contains the high scores
 * @param sonas Array of swadgesona structs. This array must be the same length as ::highScores_t.highScoreCount.
 */
void freeHighScoreSonas(highScores_t* hs, swadgesona_t sonas[]);
