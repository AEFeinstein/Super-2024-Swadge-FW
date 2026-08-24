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
    "There's more that goes into choosing a urinal than you might expect or thing about when you use the boy's room, "
    "but all the details are included in these rules.",
    "Timer/Need to Pee",
    "There's a timer at the top of the screen with a numeric readout above it. This is how long you can hold your "
    "bladder for. Let it build up too much and you'll pee yourself.",
    "Number of Urinals",
    "There's always at least three urinals, and up to seven. THe number is random, but starts on the small end until "
    "you have time to learn the ropes. The urinals on the sides are more desirable since they provide the most space "
    "for a theoretical next person to come into bathroom.",
    "People",
    "People are weird. Some are very weird. Some drop their pants around their ankle, some come in with their "
    "underwear, and some butt naked. Some smell really bad. Stay further away from the weird people.",
    "Major issues",
    "Some issues are dealbreakers. If oyu use that urinal, you're going to get pee everywhere. If the drain is plugged "
    "and there's a puddle in the bowl, the bowl or drain are broken, or if there's an out of order sign you shouldn't "
    "be using it.",
    "Minor issues",
    "Vandals may scrawl graffiti, there may be small cracks, and the flushing mechanism may be constantly flowing "
    "water, but none of these are dealbreakers. Still, it's less than ideal.",
    "Dividers",
    "Dividers provide a bit of privacy. from your immediate neighbors. Well, unless they're missing. It's important "
    "there's on on either side, though the wall on the left side is acceptable. Without a divider, using the one next "
    "to the door is risky.",
    "Puddles",
    "Some people just can't aim. Sometimes the pipe leaks. At least you're wearing shoes... right? Try not to stand in "
    "a puddle, or even near one if you don't have to. They're gross.",
    "Low Urinals",
    "The American Disability Act means that there's usually at least one shorter urinal for people who need it, and "
    "surprise surprise, you don't need. it. Use this one last.",
    "Stalls",
    "All of the urinals occupied, broken, or the situation is too confusing? Use a stall! Press B to use a stall, but "
    "beware, there's only so many times you can use it before the stalls are full. Try to use a stall when you don't "
    "have any available and you'll pee your pants.",
    "Autoflusher",
    "Some urinals include an autoflusher, and with how disgusting these things get, you're probably going to want to "
    "use that one over the others.",
    "Helper mode",
    "In settings, there's a 'helper' mode. This mode shows what the best and worst urinals are after each round. All "
    "of the best and worst urinals are marked with a check or an x respectively.",
    "Casual mode",
    "Don't really need to go? Play without the timer looming over your head and spend your time picking the best "
    "option.\n\n Scores are not tracked in this game mode.",
    "Scoreing",
    "There are four metrics tracked: Game count, accuracy, total score, and adjusted score. ",
    "Game count",
    "Game count is straightforward. The highest level a player has completed. It doesn't count the final round which "
    "is inevitably a loss, and the score is not counted for that round.",
    "Accuracy",
    "Accuracy is how well the player's choices match the best option in any given situation. 100%% means the player "
    "always picks the best option. Turn on helper mode if you find yourself confused with what is the bast option.",
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