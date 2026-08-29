#pragma once

//==============================================================================
// Includes
//==============================================================================

#include "ggCommonTypes.h"

//==============================================================================
// Defines
//==============================================================================

#define LONG_WAIT (GG_SECOND * 15)

//==============================================================================
// Const
//==============================================================================

static const char* const rulesText[] = {
    "Rules",
    "Here is a compendium fo the rules of picking a urinal. Some of these will feel pretty intuitive.",
    "Controls",
    "A: Select Urinal\nB: Use a stall\nLeft/right: Pick a urinal\nLeft swipe pad can also be used\nStart/select: Pause "
    "game",
    "Mechanics",
    "There's more that goes into choosing a urinal than you might expect or think about when you use the little boy's "
    "room, but all the details are included in these rules.",
    "Timer/Need to Pee",
    "There's a timer at the top of the screen with a numeric readout above it. This is how long you can hold your "
    "bladder for. Let it build up too much and you'll pee yourself.",
    "Number of Urinals",
    "There's always at least three urinals, and up to seven. The number is random. The urinals on the ends are more "
    "desirable.",
    "People",
    "There's a wide variety of people.",
    "Weird people",
    "You want to stay away from the weirder people.",
    "Major issues",
    "All of the following options result in a failure. You'll get pee everywhere or at least damage the sign.",
    "Minor issues",
    "All of the following options are not deal-breakers. Still, you'd rather a clean toilet.",
    "Dividers",
    "Dividers provide a bit of privacy unless they're missing. The one next to the door can go missing too.",
    "Puddles",
    "Puddles suck to stand in, and even next to. Avoid them if possible.",
    "Low Urinals",
    "The low toilets are an accessability toilet. Use it only when the other options are really bad.",
    "Autoflusher",
    "Some urinals include an autoflusher, and with how disgusting these things get, you're probably going to want to "
    "use that one over the others.",
    "Stalls",
    "Use a stall if you have no good options or the situation is too confusing. Press 'B' to use a stall. You fail if "
    "you don't have any uses left, however.",
    "Helper mode",
    "If you want to be shown the best and worst options after each level, activate helper mode. Best has green check, "
    "worst has a red X.",
    "Casual mode",
    "Don't really need to go? Play without the timer looming over your head and spend your time picking the best "
    "option.\n\n Scores are not tallied, and you can keep playing until you're done.",
    "Scoring",
    "There are four metrics tracked: Game count, accuracy, total score, and adjusted score. ",
    "Game count",
    "Game count is straightforward. The highest level a player has completed. It doesn't count the final round which "
    "is inevitably a loss, and the score is not counted for that round.",
    "Accuracy",
    "Accuracy is how well the player's choices match the best option in any given situation. 100% means the player "
    "always picks the best option. Turn on helper mode if you find yourself confused with what is the best option.",
    "Total Score",
    "Total raw score, calculated as per-level accuracy subtract time taken. The longer you take, the less of your "
    "score you get to keep.",
    "Adjusted score",
    "Total score multiplied by the overall accuracy for a single overall number to compare the performance of a player "
    "vs others.\n\nAll four scores are sent through swadgepass, compare with your friends!",
};

//==============================================================================
// Function definitions
//==============================================================================

// init
/**
 * @brief Initializes the warning data
 *
 * @param ggd Game data
 */
void ggInitWarning(ggData_t* ggd);

/**
 * @brief Initializes the splash screen
 *
 * @param ggd Game data
 */
void ggInitSplash(ggData_t* ggd);

// Single screens
/**
 * @brief Draws the initial warning screen
 *
 * @param ggd Game data
 * @param elapsedUs Time since last frame
 */
void ggDrawWarning(ggData_t* ggd, int64_t elapsedUs);

/**
 * @brief Draws the splash screen
 *
 * @param ggd Game data
 * @param elapsedUs Time since last frame
 */
void ggDrawSplash(ggData_t* ggd, int64_t elapsedUs);

/**
 * @brief Draws the pick screen
 *
 * @param ggd Game data
 */
void ggDrawPick(ggData_t* ggd);

/**
 * @brief Draws the ready screen
 *
 * @param ggd Game data
 * @param elapsedUs Time since last frame
 */
void ggDrawReady(ggData_t* ggd, int64_t elapsedUs);

/**
 * @brief Draws the pause screen
 *
 * @param ggd Game data
 */
void ggDrawPause(ggData_t* ggd);

/**
 * @brief Draws the game's end result
 *
 * @param ggd Game data
 */
void ggDrawResult(ggData_t* ggd);

// Levels

/**
 * @brief Draws a level
 *
 * @param ggd Game data
 * @param solution If the solution should be drawn
 */
void ggDrawLevel(ggData_t* ggd, bool solution);

// Menus

/**
 * @brief Draws the menu
 *
 * @param ggd Game data
 */
void ggDrawMenu(ggData_t* ggd);

/**
 * @brief Draws the rules tabs
 *
 * @param ggd Game data
 */
void ggDrawRules(ggData_t* ggd);

/**
 * @brief Draws the high score tables
 *
 * @param ggd Game data
 */
void ggDrawHighScore(ggData_t* ggd);

/**
 * @brief Draws the options menu
 *
 * @param ggd Game data
 */
void ggDrawOptions(ggData_t* ggd);

/**
 * @brief Returns the length of the hydrate text array
 *
 * @return int number of positions in the array
 */
int ggGetDrinkTextLen(void);