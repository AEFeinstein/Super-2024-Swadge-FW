/**
 * @file trophy.h
 * @author Jeremy.Stintzcum@gmail.com
 * @brief Trophies for swadge modes
 * @version 0.1
 * @date 2025-01-13
 *
 * @copyright Copyright (c) 2025
 *
 */

/*! \file trophy.h
 *
 * \section trophy_overview Overview
 *
 * The trophy system should be almost 100% handled by five easy to use functions. This system is designed to be as
 * compact as possible without compromising usability while also not hammering the NVS with every update, or requiring
 * more overhead. Each trophy has options as shown in the initializations section.
 *
 * Each trophy has a "point" value associated with it which is totalled up to a "Gamer Score" type value to compare with
 * friends.
 *
 * \section trophy_system The trophy system
 *
 * It is recommended to call `trophySystemInit()` in the mode entry function in case other modes have been accessed
 * which changed the settings.
 *
 * Options:
 * - Banner Top/Bottom: Choose if the banner is viewed from the top or bottom.
 * - Display Duration: How long the banners display by. Units is tenths of a second.
 * - Animate Banners: If the banners should animate by default
 * - Slide Duration: How long the banner should take to slide in. Units is tenths of a second.
 *
 * Call `trophySystemGetPoints()` to retrieve the number of points the player has per mode. Use NULL to get points for
 * whole system.
 *
 * \section trophy_init Initialization of each individual trophy
 *
 * Call `trophyInit()` near the start of the program. Only initializes once, so it can be put anywhere in the code
 * without worry, so long as it's called before the player tries to operate on it. Recommendation is in the mode entry
 * function so it is fully initialized before the player can run any inputs to avoid errors.
 *
 * Options:
 * - Title: The name of the trophy. Make it unique! This is used as the identifier and having two of the same names will
 * confuse the program. There's no need to worry about sharing the name with other modes though, that's handled
 * internally.
 * - Description: Displays text to help the player figure out how to get the trophy. Text-box size is limited, and too
 * long descriptions will get truncated. Descriptions can be off-topic, but may confuse players.
 * - Image: Provide a 36x36 sprite that will be displayed. If "NULL" is provided, will not draw and will give more space
 * for text
 * - Type: See type descriptions below
 * - Points: Number of arbitrary points player gets for winning trophy. Like gamer score on XBOX.
 * - Max Value: Depending on the type, this number is the value that the player is trying to get to
 *
 * Types of trophies:
 * - Trigger: Only required to trigger once. These are best suited to trophies that are secrets, milestones, etc.
 * - Additive: Each time update is called, adds to the tracker until the max value is reached. Useful for lifetime
 * stats.
 * - Progress: Each time the trophy is updated, it takes the highest value and uses that. This is best for trophies that
 * are expected to be completed in a single run, such as farthest distance climbed in a race or highest amount of coins
 * collected per life.
 * - Task List: When a trophy is updated, it sets bits based on task ID. This allows unique tasks to be separated off,
 * such as visiting all games or collecting all types of gems, not just a quantity of gems.
 *
 * A special "All trophies gotten" trophy is automatically generated when the first standard trophy is generated.
 *
 * It is encouraged for developers to equate all trophies values to add up to 1000. The default trophy has a value of
 * 100, leaving 900 to be divide by all other trophies. Any points over 1000 earned by a mode will result in
 *
 * \section trophy_update Updating the Trophy
 *
 * Use either `trophyUpdate()` or `trophyUpdateMilestone()` to update the status of the trophy. If the trophy is a
 * 'Trigger' type, it will automatically set the trophy to having been won regardless of the value entered. If the
 * trophy is one of the other two modes, it will either add or replace the previous value if appropriate.
 *
 * If the developer wants to update quietly, set the drawUpdate to false. This allows smaller updates if required.
 * Otherwise, set to true to show the banner automatically.
 *
 * Using the milestone system, the program will automatically display trophy progress at 25%, 50%, 75% and 100%
 * completion rate. This is great for trophies that have frequent updates and wish to display partial milestones.
 *
 * If a custom milestone is desired, use `trophyGetData()` to get a struct of the trophy data, and then compare the max
 * value inside the struct to the current value the player has, or just keep an external reference tot eh max value. Set
 * the conditions for each milestone desired, then set the draw argument to true if a milestone has been crossed.
 * Example code below:
 *
 * \code {.c}
 * //TODO: Example of custom milestone boolean structure
 * \endcode
 *
 * Updates should be as large and as infrequent as possible. Only update when the developer has to to avoid the overhead
 * of reading from the NVS to check if it's been updated, as that's slow. The system checks if the NVS needs writing so
 * the user don't have to worry about the NVS being destroyed by too many updates. The worst case scenario is rapidly
 * adding 1 to a very large number constantly, as this will hit the NVS every time, so try to avoid this.
 *
 * \section trophy_helpers Helper functions
 *
 * Several helper functions exist for the developer to get critical information
 * - trophyGetNumTrophies(): Gets the total number of trophies for a mode, or for all modes if NULL
 * - trophyGetTrophyList(): Returns a list of trophy titles, which can then be used to pull individual data or just
 * display on the screen. Setting NULL as the mode name will grab all trophy titles in unspecified order.
 * - trophyGetData(): Grabs the data stored for the specified trophy, such as sprite name, description, and current
 * value.
 *
 * \section trophy_draw Drawing a trophy
 *
 * Individual trophies can be viewed at any time by calling `trophyDraw()` and providing it an index. Set animations
 * to false to draw the box immediately. This function should be last in the draw stack to ensure it draws on top of all
 * other items in the mode.
 *
 * The standard single trophy draw function draws a banner across the top of the screen using the provided data. It will
 * display:
 * - An image if provided
 * - The title text
 * - The description
 * - If a value is required, draw current value and progress
 *
 * //TODO: Example image
 *
 * The banner will scroll in from the top by default, but can be set to the bottom with `trophySystemSet()`. See system
 * initialization. The banner will be fully visible for the set duration, not including scroll in and scroll out
 * duration.
 *
 * There can be up to five banners in the queue, and will display one after another.
 *
 * \section trophy_draw_list Drawing all the trophies
 *
 * The trophies can be drawn in one of two ways: By calling `trophyDrawList()` or by getting the list of trophies and
 * making a separate display function.
 *
 * The list draw function can be re-initialized to change the colors to theme the menu without recreating it from
 * scratch.
 *
 * //TODO: Example image
 *
 * The colors can be changed to theme the display with `trophyDrawListInit()`.
 *
 * \section trophy_clear Clearing a trophy
 *
 * Reset a trophy to 0 (but don't remove it from the list of trophies) with `trophyClear()`.
 *
 * Trophies should only be cleared under abnormal circumstances, such as the player having been caught cheating.
 * Consider that the Steamworks API (Valve) has a "Remove trophy" function, yet most people have never lost an trophy.
 *
 * Clearing a trophy will delete the points acquired if it was previously won.
 *
 * \code {.C}
 * // TODO: Examples of all code
 * \endcode
 *
 */
#pragma once

//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>
#include <stdbool.h>
#include "wsgPalette.h"
#include "fs_font.h"

//==============================================================================
// Defines
//==============================================================================

// String lengths
#define TROPHY_MAX_TITLE_LEN 24
#define TROPHY_MAX_DESC_LEN  96
#define TROPHY_MAX_WSG_LEN   24
#define TROPHY_MAX_BANNERS   5

//==============================================================================
// Enum
//==============================================================================

typedef enum
{
    TROPHY_TYPE_TRIGGER,  //< Only needs to be triggered once (Did a handstand, found a hidden message)
    TROPHY_TYPE_ADDITIVE, //< Each update adds to teh previous (Lifetime distance, number of cakes eaten)
    TROPHY_TYPE_PROGRESS, //< Tracks how far a user got per update (How long player survived, how far player ran)
} trophyTypes_t;

//==============================================================================
// Structs
//==============================================================================

// Individual Trophy data objects (Possibly unnecessary)
typedef struct
{
    char title[TROPHY_MAX_TITLE_LEN];      //< Name of the Trophy, used as ID
    char description[TROPHY_MAX_DESC_LEN]; //< Short description of task required
    char imageString[TROPHY_MAX_WSG_LEN];  //< String leading to the .wsg file.
    trophyTypes_t type;                    //< Type of trophy. See "trophy.h" for descriptions
    // FIXME: Needs to be a difficult rating, for auto point assignment
    int8_t points;    //< How many points the trophy is worth
    int currentValue; //< Current status of the trophy
    int maxValue;     //< The value that
} trophy_t;

typedef struct
{
    bool drawFromBottom;      //< If banner should be drawn from the bottom of the screen
    int32_t drawMaxDuration;  //< How long the banner will be drawn fully extended
    bool animated;            //< If being animated to slide in and out
    int32_t slideMaxDuration; //< How long the banner will take to slide in and out
    bool silent;
} trophySettings_t;

//==============================================================================
// Functions
//==============================================================================

// Trophy system

/**
 * @brief Initializes the Trophy system settings. The system is a global setting and every mode needs to set this when
 * entered to avoid copying from other modes.
 *
 * @param bottom True if the banner should appear from the bottom, false if should appear from the top.
 * @param displayDuration Time in tenths of a second that the banner should display for. Default is 3 seconds (30)
 * @param animate If the banner should scroll into view
 * @param scrollSpeed Time in tenths of a second for banner to appear. Default is half a second (5)
 */
void trophySystemInit(trophySettings_t* settings);

/**
 * @brief Loads the current number of points
 *
 * @param modeName Name of the made for namespace. NULL gets total of all modes.
 * @return int Number of points total for the provided mode
 */
int trophySystemGetPoints(char* modeName);

// Utilize trophies

/**
 * @brief Initializes a new Trophy
 *
 * @param modeName Mode to attach trophy to
 * @param title Name of the Trophy
 * @param imgStr String leading to WSG
 * @param desc String containing description of trophy
 * @param type Type of trophy. See Initialization section above
 * @param points Number of poi9nts gained by winning trophy
 * @param maxVal Value required to trigger win in non-single trigger mode
 */
void trophyInit(char* modeName, char* title, char* imgStr, char* desc, trophyTypes_t type, int8_t points, int maxVal);

/**
 * @brief Updates specifed trophy if required
 *
 * @param modeName Name of the mode
 * @param title Title fo the trophy to update
 * @param value New value to try to set. Behavior is set by trophy type
 * @param drawUpdate If this update should be drawn to the screen
 */
void trophyUpdate(char* modeName, char* title, int value, bool drawUpdate);

/**
 * @brief Updates just like trophyUpdate(), but automatically mutes updates that aren't breaking
 *
 * @param modeName Name of the mode
 * @param title Title of trophy to update
 * @param value New value to assign
 */
void trophyUpdateMilestone(char* modeName, char* title, int value);

/**
 * @brief Erases completion data from swadge. Only use in extreme circumstances.
 *
 * @param modeName Name of the mode
 * @param title Title of trophy to clear.
 */
void trophyClear(char* modeName, char* title);

// Helpers

/**
 * @brief Gets the total number of trophies from a specified mode.
 *
 * @param modeName Name of the mode from which to grab trophy quantity. NULL returns quantity of all modes.
 * @return int total number of trophies in given mode.
 */
int trophyGetNumTrophies(char* modeName);

/**
 * @brief Grabs a list of titles from specified mode.
 *
 * @param modeName Name of the mode from which to grab trophies. NULL returns all values in unspecified order
 * @param tList outFile that contains a list of strings which are the titles of the trophies
 * @param tLen Max number of trophies to grab. If actual total is less than provided tLen, value is set to total
 * @param offset Where to start grabbing from. Useful in batch mode
 */
void trophyGetTrophyList(char* modeName, trophy_t* tList, int* tLen, int offset);

/**
 * @brief Grabs data for specified Trophy.
 *
 * @param modeName Name of the mode
 * @param title Title of the Trophy
 * @return trophy_t trophy data struct
 */
trophy_t trophyGetData(char* modeName, char* title);

// Drawing functions

/**
 * @brief Draws the banner if one is queued
 *
 * @param modeName Name of the mode being used
 * @param fnt Font to be used
 * @param elapsedUs TIme since last frame
 */
void trophyDraw(char* modeName, font_t* fnt, int64_t elapsedUs);

/**
 * @brief Changes the colors of the default list of trophies.
 *
 * // TODO: Define color variables to set. Should not take void as argument.
 */
void trophyDrawListInit(void);

/**
 * @brief Draws a list of trophies for provided mode.
 *
 * @param modeName Name of the Mode. Set to NULL to get all trophies in an undefined order.
 * @param idx Index to start displaying at. Program will ensure it's not out of bounds, but developer is required to
 * update index as they see fit.
 */
void trophyDrawList(char* modeName, int idx);

// TEST ONLY
void trophyDrawDataDirectly(trophy_t t, int y, font_t* fnt);
void loadImage(int idx, char* string);
void unloadImage(int idx);