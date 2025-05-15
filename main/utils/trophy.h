/**
 * @file trophy.h
 * @author Jeremy.Stintzcum@gmail.com
 * @brief Trophies for swadge modes
 * @date 2025-01-13
 *
 * @copyright Copyright (c) 2025
 *
 */

/*! \file trophy.h
 *
 * \section trophy_overview Overview
 *
 * Welcome to the swadge's trophy system! THis fun addition to your game gives players an extrinsic reason to keep
 * playing your game again and again.asm
 *
 * The trophies work similarly to a mix of the big three: Valve's Achievements, Xbox's gamer score, and Playstation's
 * Trophies. THere are several different unlock conditions that can be specifed, there's minimal overhead on teh swadge
 * (when set up properly) and trophies give the player points that they can use as bragging rights.
 *
 * As a dev, you will have to put together a "modeName_TL.h" file that will contain a few data stuctures that will
 * define how the trophy system interacts with your mode and also all of the static data for your trophies. After all,
 * it's not a minor amount of data!
 *
 * Trophies are drawn from either the top or the bottom of the screen, and the duration thy're on screen can be
adjusted. All NVS activity is handled for the Dev.
 *
 * \section trophy_system The trophy system
 *
 * Like Cthulu creeping into your mind, the trophy system sits on top of the swadge mode. It's always there, but sits
disabled unless explicitly set active by a developer. To activate it, create the following structs in the .c file for
the mode:
 * \code {.c}
const trophyData_t exampleTrophies[] = {
    {
        .title       = "Trigger Trophy",
        .description = "You pressed A!",
        .imageString = "kid0.wsg",
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_EASY,
        .maxVal      = 1, // For trigger type, set to one
    },
}

trophySettings_t exampleTrophySettings = {
    .drawFromBottom   = false,
    .staticDurationUs = DRAW_STATIC_US,
    .slideDurationUs  = DRAW_SLIDE_US,
};
 trophyDataList_t trophyTestData = {.settings = &trophyTestModeTrophySettings,
                                    .list = trophyTestModeTrophies,
                                    .length = ARRAY_SIZE(trophyTestModeTrophies)};

swadgeMode_t trophyTestMode = {.modeName      = modeName,
                               .wifiMode      = NO_WIFI,
                                //...
                               .fnAdvancedUSB = NULL,
                               .trophyData    = &trophyTestData};
 * \endcode
 *
 * This will allow the trophy system to access your trophies and make sure the right text and images are displayed when
a trophy is triggered. Also, add the setting data as `extern` to ensure that the data can be accessed by the trophy case
and swadgepass
 *
 * \section trophy_settings Settings
 *
 * There are four settings that can be set for the mode overall:
 * - drawFromBottom: If set to true, draws the banner at the bottom
 * - staticDurationUs: How long the banner stays on screen for. The default value (DRAW_STATIC_US) is about half a
second
 * - slideDurationUS: How long it takes for the banner to slide in and out. Set to (DRAW_SLIDE_US) for quarter second
slide, or 0 to have the banner pop in.
 * - namespaceKey: A text string used to save data to the NVS. This is set automatically if left blank, and should only
be manually set when there's a conflict between modes
 *
 * These can be loaded into the settings struct as shown above.
 *
 * \section trophy_individual Individual trophy settings
 *
 * The largest portion of the work required to set up the trophies it to create all the trophies from scratch. Here's
the types of data available to set for each individual trophy:
 * - Title: The name of the trophy. Make it unique! This is used as the identifier and having two of the same names will
 * confuse the program. There's no need to worry about sharing the name with other modes though, that's handled
 * internally.
 * - Description: Displays text to help the player figure out how to get the trophy. Text-box size is limited, and too
 * long descriptions will get truncated. Descriptions can be off-topic, but may confuse players.
 * - Image string: Provide a 36x36 sprite that will be displayed. If string is empty (""), the system will not draw and
will give more space for the text
 * - Type: See type descriptions below
 * - Difficulty: Instead of setting specific values, set the expected difficulty and the program will automatically
assign score based on the number of trophies in the mode and their relative difficulty.
 * - Max Value: Depending on the type, this number is the value that the player is trying to get to. See the type
discussion below for more info.
 * - hidden: If set to true, does not appear in lists of trophies until won.
 *
 * \warning NVS keys have a max length of 15 characters. Trophies with the same first 15 character will have conflicts
 *
 * Types of trophies:
 * - Trigger: Only required to trigger once. These are best suited to trophies that are secrets, milestones, etc.
 * - Additive: Each time update is called, adds to the tracker until the max value is reached. Useful for lifetime
 * stats.
 * - Progress: Each time the trophy is updated, it takes the highest value and uses that. This is best for trophies that
 * are expected to be completed in a single run, such as farthest distance climbed in a race or highest amount of coins
 * collected per life.
 * - Checklist: When a trophy is updated, it sets bits based on task ID. This allows unique tasks to be separated off,
 * such as visiting all games or collecting all types of gems, not just a quantity of gems. CHecklists can also be
"unchecked" in case the player has to balance two things, like having two torches lit at the same time. Checklists have
a hard limit of 32 flags.
 *
 * A special "All trophies gotten" trophy is automatically generated when the first standard trophy is generated.
 *
 * \section trophy_update Updating the Trophy
 *
 * Use either `trophyUpdate()` or `trophyUpdateMilestone()` to update the status of the trophy. If the trophy is a
 * 'Trigger' type, it will automatically set the trophy to having been won regardless of the value entered. If the
 * trophy is either 'Additive' or 'Progress' modes, it will either add or replace the previous value if appropriate.
Lastly, if it's a checklist, you can use either the standard update command or `trophySetChecklistTask()` to shortcut
needing to set bit flags yourself.
 *
 * If the developer wants to update quietly, there is a argument to disable drawing. This allows smaller updates if
required.
 * Otherwise, set to true to show the banner automatically.
 *
 * Using the milestone system, the program will automatically display trophy progress at the set percentage thresholds.
 * This is great for trophies that have frequent updates and wish to display milestones instead of for every little
update.
 *
 * Here's some important notes on sending updates:
 * - No overflow protection: You
 * - If the first fifteen characters of NVS Namespace or two or more trophy names inside the same mode there will be
 *   data clashes
 * - Does not save values once trophy is won. For example, if 10 key presses unlocks a trophy, even if a hundred presses
 *   are registered, the number 10 will be saved to NVS.
 * - When testing, renaming and reordering trophies (especially items inside a checklist) may cause hard to discover
 *   errors. You may have to clear NVS on ESP32 / delete the nvs.json file to fix issues.
 * - Only update as often as you need: While the trophy system has a few safeguards, if you're asking it to update every
cm moved forward in a marathon, you're going to be saving a lot of writes, which could harm the swadge
 * - Every trophyUpdate() call has the potential to save to NVS. NVS has a limited amount of writes over it's lifetime
which we're not likely to hit, but maybe don't hammer the NVS by incrementing by one every frame. Only update when
reasonable. The code will cut out a lot of frivolous requests such as:
 *   - Trying to update trophy after it's been won
 *   - Trying to save the same value into NVS
 *   - Trying to save a lower value into NVS (Unless it's a Checklist. You can un-check Checklists)
 * - In the above cases, it's safe to keep the update code running without issue, but incrementing by one every frame
may cause slowdowns.
 *
 * Once set, the value can be pulled back out by running `trophyGetSavedValue()`.
 *
 * \section trophy_helpers Helper functions
 *
 * Several helper functions exist for the developer to get critical information
 * - check/setBitFlags(): Two complimentary functions to handle that pesky bit shifting
 * - trophyGetPoints(): Get either the total number of points for this mode of for the entire swadge
 * - trophyGetNumTrophies(): Gets the total number of trophies for a mode
 * - trophyGetTrophyList(): Returns a list of all trophy data
 * - trophyGetLatestTrophy(): Grabs the data stored for the latest saved trophy.
 *
 * \section trophy_draw Drawing a trophy
 *
 * The Swadge will automatically display trophies as they're unlocked. You do not need to provide draw calls, it is
handled automatically.
 *
 * The banner notifications look like this:
 * \image html InProgressTrophy.png
 * \image html CompletedTrophy.png
 *
 * The banner will scroll in from the top by default, but can be set to the bottom in the settings. See system
 * initialization. The banner will be fully visible for the set duration, not including scroll in and scroll out
 * duration.
 *
 * Banners will automatically queue up. A significant number of calls may slow down the swadge as it has to allocate
data for each update.
 *
 * \section trophy_draw_list Drawing all the trophies
 *
 * A trophy 'gallery' type function has been provided. Initialize with `trophyDrawListInit()`, set the colors with
`trophyDrawListColors()`, and display with `trophyDrawListDraw()`. It displays a long list that can be scrolled by
providing new y values. Remember to deinit the list with `trophyDrawListDeinit()` to cover any memory leaks.
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

#define TROPHY_MAX_TITLE_LEN 48
#define TROPHY_MAX_DESC_LEN  128
#define MAX_NVS_KEY_LEN      16

#define DRAW_SLIDE_US  262144 // Recommended slide time, a power of 2 about a quarter second
#define DRAW_STATIC_US 524288 // Recommended static time, a power of 2 about a half second

#define NO_IMAGE_SET CNFS_NUM_FILES
#define TROPHY_PLAT  -1

//==============================================================================
// Enum
//==============================================================================

/// @brief Types of trophies the devs can instantiate
typedef enum
{
    TROPHY_TYPE_TRIGGER,   //< Only needs to be triggered once (Did a handstand, found a hidden message)
    TROPHY_TYPE_ADDITIVE,  //< Each update adds to teh previous (Lifetime distance, number of cakes eaten)
    TROPHY_TYPE_PROGRESS,  //< Tracks how far a user got per update (How long player survived, how far player ran)
    TROPHY_TYPE_CHECKLIST, //< Each bit is a flag for a specific action. Repeating actions does not increment.
                           //< (Used for collecting different objects, like chaos emeralds)
} trophyType_t;

/// @brief Dev inferred difficulty of achieving, used to distribute points.
typedef enum
{
    TROPHY_DIFF_EASY,
    TROPHY_DIFF_MEDIUM,
    TROPHY_DIFF_HARD,
    TROPHY_DIFF_EXTREME,
} trophyDifficulty_t;

/// @brief What display mode to use to draw trophies
typedef enum
{
    TROPHY_DISPLAY_ALL,
    TROPHY_DISPLAY_UNLOCKED,
    TROPHY_DISPLAY_LOCKED,
    TROPHY_DISPLAY_INCL_HIDDEN,
} trophyListDisplayMode_t;

//==============================================================================
// Structs
//==============================================================================

/// @brief Individual Trophy data objects
typedef struct
{
    char title[TROPHY_MAX_TITLE_LEN];      //< Name of the Trophy, used as ID
    char description[TROPHY_MAX_DESC_LEN]; //< Short description of task required
    cnfsFileIdx_t image;                   //< Index of the image
    trophyType_t type;                     //< Type of trophy. See "trophy.h" for descriptions
    trophyDifficulty_t difficulty;         //< How many points the trophy is worth
    int32_t maxVal;                        //< The value that
    bool hidden;                           //< If trophy is hidden by default
} trophyData_t;

/// @brief Settings for the trophy system
typedef struct
{
    bool drawFromBottom;                //< If banner should be drawn from the bottom of the screen
    int32_t staticDurationUs;           //< How long the banner will be drawn fully extended
    int32_t slideDurationUs;            //< How long the banner will take to slide in and out
    char namespaceKey[MAX_NVS_KEY_LEN]; //< key used for trophy namespace
} trophySettings_t;

/// @brief The data object dev hands to the trophy showcase that contains all the const data.
typedef struct
{
    int32_t length;                 //< Length of the trophy arrays
    const trophyData_t* const list; //< Array of trophies
    trophySettings_t* settings;     //< Setting data
} trophyDataList_t;

//==============================================================================
// Functions
//==============================================================================

// Trophy system

/**
 * @brief Initializes the Trophy system settings. The system is a global setting and every mode needs to set this when
 * entered to avoid copying from other modes.
 *
 * @param settings The settings data
 * @param modeName Name of the mode
 */
void trophySystemInit(trophyDataList_t* settings, const char* modeName);

// Utilize trophies

/**
 * @brief Updates specifed trophy if required
 *
 * @param t Trophy to update
 * @param newVal New value to try to set. Behavior is set by trophy type
 * @param drawUpdate If this update should be drawn to the screen
 */
void trophyUpdate(trophyData_t t, int newVal, bool drawUpdate);

/**
 * @brief Updates just like trophyUpdate(), but only draws when crossing percentage boundary
 *
 * @param t Trophy to update
 * @param newVal Value to attempt to set
 * @param threshold Value (0-100, representing a percent) to draw at
 */
void trophyUpdateMilestone(trophyData_t t, int newVal, int threshold);

/**
 * @brief Returns the value saved to the NVS or 0 if the key isn't found.
 *
 * @param t Trophy to grab value for
 * @return int32_t Stored value for the requested trophy
 */
int32_t trophyGetSavedValue(trophyData_t t);

/**
 * @brief Sets or unsets a checklist item.
 *
 * @param t Trophy to set
 * @param flag Task that was just completed
 * @param unset If we're unsetting the bit
 * @param drawUpdate If this update should be drawn
 */
void trophySetChecklistTask(trophyData_t t, int32_t flag, bool unset, bool drawUpdate);

/**
 * @brief Erases completion data from swadge. Only use in extreme circumstances.
 *
 * @param t Trophy to set to 0
 */
void trophyClear(trophyData_t t);

// Helpers

/**
 * @brief Checks an individual bit flag out of a int32
 *
 * @param flags int32 containing the flag to check
 * @param idx Index of the bit
 * @return true If bit is set
 * @return false If bit is not set
 */
bool checkBitFlag(int32_t flags, int8_t idx);

/**
 * @brief Set a bit flag
 *
 * @param flags pointer to variable to store flags
 * @param idx Index of the bit to set
 * @param setTrue Set to false to unset the
 */
void setBitFlag(int32_t* flags, int8_t idx, bool setTrue);

/**
 * @brief Get the point totals for the gamer score
 *
 * @param total If loading the full score or for the current mode
 * @param modeName Mode name to load. Set to NULL to get currently loaded mode
 * @return int Value of the score
 */
int trophyGetPoints(bool total, const char* modeName);

/**
 * @brief Gets the number of trophies associated with a mode, or current mode if NULL
 *
 * @return int total number of trophies in given mode.
 */
int trophyGetNumTrophies(void);

/**
 * @brief Returns a pointer to the data struct containing all the trophy data
 *
 * @return const trophyData_t*
 */
const trophyData_t* trophyGetTrophyList(void);

/**
 * @brief Get the Latest Trophy Idx object
 *
 * @return int32_t Returned idx
 */
int32_t getLatestTrophyIdx(char* buffer);

/**
 * @brief Get the Trophy Data From Idx object
 *
 * @param idx
 * @return trophyData_t
 */
trophyData_t getTrophyDataFromIdx(int idx);

/**
 * @brief Set the Trophy System to a specific mode's values
 *
 * @param dl Data object from the mode
 * @param modeName Name of the mode
 */
void trophySetSystemData(trophyDataList_t* dl, const char* modeName);

// Drawing functions

/**
 * @brief Draws the banner if one is queued
 *
 * @param fnt Font to be used
 * @param elapsedUs TIme since last frame
 */
void trophyDraw(font_t* fnt, int64_t elapsedUs);

/**
 * @brief Initialize the trophy Draw list
 *
 * @param mode What display mode to draw
 */
void trophyDrawListInit(trophyListDisplayMode_t mode);

/**
 * @brief Set the colors of the panel to custom. Call after initialization of the list, or it'll be overwritten
 *
 * @param background Color of the background behind the panels
 * @param panel The main panel color
 * @param shadowBoxes The check box and image shadowbox colors
 * @param dimText Description text and numbers
 * @param titleText Title of the trophy
 * @param checkmark Color of the checkmark
 */
void trophyDrawListColors(paletteColor_t background, paletteColor_t panel, paletteColor_t shadowBoxes,
                          paletteColor_t dimText, paletteColor_t titleText, paletteColor_t checkmark);

/**
 * @brief Tears down the WSGs and height list
 *
 */
void trophyDrawListDeinit(void);

/**
 * @brief Draws the list.
 *
 * @param fnt Font to use
 * @param yOffset Current Y offset. Higher numbers effectively scroll down
 */
void trophyDrawList(font_t* fnt, int yOffset);
