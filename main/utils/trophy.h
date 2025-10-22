/**
 * @file trophy.h
 * @author Jeremy.Stintzcum@gmail.com
 * @brief Trophies for swadge modes
 * @date 2025-05-22
 */

/*! \file trophy.h
 *
 * \section trophy_overview Overview
 *
 * Welcome to the swadge's trophy system! This fun addition to your game gives players an extrinsic reason to keep
 * playing your game again and again.
 *
 * The trophies work similarly to a mix of the big three: Valve's Achievements, Xbox's Gamer Score, and Playstation's
 * Trophies. There are several different unlock conditions that can be specifed, there's minimal overhead on the swadge
 * (when set up properly) and trophies give the player points that they can use as bragging rights.
 *
 * All the dev has to do is make a few data structures and load them into the swadgeMode_t struct. This will
 * automatically initialize the system.
 *
 * Trophies are drawn from either the top or the bottom of the screen, and the duration they're on screen can be
 * adjusted. All NVS activity is handled for the Dev.
 *
 * \section trophy_system The trophy system
 *
 * Like Cthulu creeping into your mind, the trophy system sits on top of the swadge mode. It's always there, but sits
 * disabled unless explicitly set active by a developer. To activate it, create the following structs in the .c file for
 * the mode:
 * \code {.c}
// Modify the following with your trophies
const trophyData_t trophyTestModeTrophies[] = {
    {
        .title       = "Trigger Trophy",
        .description = "You pressed A!",
        .image       = KID_0_WSG,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_EASY,
        .maxVal      = 1, // For trigger type, set to one
    },
    {
        .title       = "Additive Trophy - Testing a very long trophy na",
        .description = "Pressed B ten times!",
        .image       = KID_1_WSG,
        .type        = TROPHY_TYPE_ADDITIVE,
        .difficulty  = TROPHY_DIFF_HARD,
        .maxVal      = 10,
    },
    {
        .title       = "Progress Trophy",
        .description = "Hold down the up button for eight seconds",
        .image       = NO_IMAGE_SET, // Hardcoded "Ignore" value
        .type        = TROPHY_TYPE_PROGRESS,
        .difficulty  = TROPHY_DIFF_EXTREME,
        .maxVal      = 8,
        .hidden      = true, // If set to true, the trophy isn't drawn in the trophy case
        .noImage     = true, // Set this to true to remove image from the trophy alltogether
    },
    {
        .title       = "Checklist",
        .description = "This is gonna need a bunch of verification, but like has a very long description",
        .image       = NO_IMAGE_SET, // If set like this, it will draw a default trophy based on difficulty
        .type        = TROPHY_TYPE_CHECKLIST,
        .difficulty  = TROPHY_DIFF_HARD,
        .maxVal      = 0x0007, // Three tasks, 0x01, 0x02, and 0x04
    },
};

// Individual mode settings
const trophySettings_t trophyTestModeTrophySettings = {
    .drawFromBottom   = false,
    .staticDurationUs = DRAW_STATIC_US * 2,
    .slideDurationUs  = DRAW_SLIDE_US,
    .namespaceKey     = trophyModeName,
};

// This is passed to the swadgeMode_t
const trophyDataList_t trophyTestData = {
    .settings = &trophyTestModeTrophySettings,
    .list     = trophyTestModeTrophies,
    .length   = ARRAY_SIZE(trophyTestModeTrophies),
};

// swadgeMode_t
swadgeMode_t trophyTestMode = {
    .modeName      = modeName,
    .wifiMode      = NO_WIFI,
    //...
    .fnAdvancedUSB = NULL,
    .trophyData    = &trophyTestData,
}; // This line activates the trophy for this mode
 * \endcode
 *
 * This will allow the trophy system to access your trophies and make sure the right text and images are displayed when
 * a trophy is triggered.
 *
 * \section trophy_settings Settings
 *
 * There are four settings that can be set for the mode overall:
 * - ::trophySettings_t.drawFromBottom : If set to true, draws the banner at the bottom
 * - ::trophySettings_t.staticDurationUs : How long the banner stays on screen for. The default value, ::DRAW_STATIC_US,
 * is about half a second
 * - ::trophySettings_t.slideDurationUs : How long it takes for the banner to slide in and out. Set to ::DRAW_SLIDE_US
 * for quarter second slide, or 0 to have the banner pop in.
 * - ::trophySettings_t.namespaceKey : A text string used to save data to the NVS. This is set automatically if left
 * blank, and should only be manually set when there's a conflict between modes
 *
 * These can be loaded into the settings struct as shown above.
 *
 * \section trophy_individual Individual trophy settings
 *
 * The largest portion of the work required to set up the trophies it to create all the trophies from scratch. Here's
 * the types of data available to set for each individual trophy:
 * - ::trophyData_t.title : The name of the trophy. Make it unique! This is used as the identifier and having two of
 * the same names will
 * confuse the program. There's no need to worry about sharing the name with other modes though, that's handled
 * internally.
 * - ::trophyData_t.description : Displays text to help the player figure out how to get the trophy. Text-box size is
 * limited, and too
 * long descriptions will get truncated. Descriptions can be off-topic, but may confuse players.
 * - ::trophyData_t.image : Provide index of a image up to 36 x 36 to be used. Use "NO_IMAGE_SET" to get a default image
 * or no image at all. See .noImage below.
 * - ::trophyData_t.type : See type descriptions below
 * - ::trophyData_t.difficulty : Instead of setting specific values, set the expected difficulty and the program will
 * automatically assign score based on the number of trophies in the mode and their relative difficulty.
 * - ::trophyData_t.maxVal : Depending on the type, this number is the value that the player is trying to get to. See
 * the type discussion below for more info.
 * - ::trophyData_t.hidden : If set to true, does not appear in lists of trophies until won.
 * - ::trophyData_t.noImage : If set to true, does not draw an image for this trophy.
 * - ::trophyData_t.identifier : This is a convenience identifier not used by the Trophy code. It can be set to a
 * pointer value or an integer cast to `intptr_t` and used by the Swadge Mode to find a specific trophy in a list of
 * trophyData_t[].
 *
 * \warning NVS keys have a max length of 15 characters. Trophies with the same first 15 character will have conflicts
 *
 * Types of trophies:
 * - ::TROPHY_TYPE_TRIGGER : Only required to trigger once. These are best suited to trophies that are secrets,
 * milestones, etc.
 * - ::TROPHY_TYPE_ADDITIVE : Each time update is called, adds to the tracker until the max value is reached. Useful
 * for lifetime
 * stats.
 * - ::TROPHY_TYPE_PROGRESS : Each time the trophy is updated, it takes the highest value and uses that. This is best
 * for trophies that
 * are expected to be completed in a single run, such as farthest distance climbed in a race or highest amount of coins
 * collected per life.
 * - ::TROPHY_TYPE_CHECKLIST : When a trophy is updated, it sets bits based on task ID. This allows unique tasks to
 * be separated off,
 * such as visiting all games or collecting all types of gems, not just a quantity of gems. Checklists can also be
 * "unchecked" in case the player has to balance two things, like having two torches lit at the same time. Checklists
 * have
 * a hard limit of 32 flags.
 *
 * A special "All trophies gotten" trophy is automatically generated when the first standard trophy is generated.
 *
 * \section trophy_update Updating the Trophy
 *
 * Use either `trophyUpdate()` or `trophyUpdateMilestone()` to update the status of the trophy. If the trophy is a
 * 'Trigger' type, it will automatically set the trophy to having been won regardless of the value entered. If the
 * trophy is either 'Additive' or 'Progress' modes, it will either add or replace the previous value if appropriate.
 * Lastly, if it's a checklist, you can use either the standard update command or `trophySetChecklistTask()` to shortcut
 * needing to set bit flags yourself.
 *
 * If the developer wants to update quietly, there is a argument to disable drawing. This allows for times when the
 * banner would cause issues (the middle of a boss fight, for example) to have the drawing disabled without stopping the
 * whole trophy system.
 *
 * Using the milestone system, the program will automatically display trophy progress at the set percentage thresholds.
 * This is great for trophies that have frequent updates and wish to display only sometimes. An example would be if
 * counting eggs found up to 100, you may only want to update every 10 eggs rather than each egg to avoid spamming
 * banners at the player.
 *
 * Here's some important notes on sending updates:
 * - No overflow protection: The internal data is always saved as an int32_t, not a uint32_t. Negative values are
 *   currently not supported.
 * - If the first fifteen characters of NVS Namespace or two or more trophy names inside the same mode there will be
 *   data clashes
 * - Does not save values once trophy is won. For example, if 10 key presses unlocks a trophy, even if a hundred presses
 *   are registered, the number 10 will be saved to NVS. However, some cases the value saved may be higher than the max
 *   Value expected.
 * - When testing, renaming and reordering trophies (especially items inside a checklist) may cause hard to discover
 *   errors. You may have to clear NVS on ESP32 / delete the nvs.json file for the emulator to fix issues.
 * - Only update as often as you need: While the trophy system has a few safeguards, if you're asking it to update every
 *   centimeter moved forward in a marathon, you're going to be doing a lot of writes, which could harm the swadge
 * - Every `trophyUpdate()` call has the potential to save to NVS. NVS has a limited amount of writes over it's lifetime
 *   which we're not likely to hit, but maybe don't hammer the NVS by incrementing by one every frame. Only update when
 *   reasonable. The code will cut out a lot of frivolous requests such as:
 *   - Trying to update trophy after it's been won
 *   - Trying to save the same value into NVS
 *   - Trying to save a lower value into NVS (Unless it's a Checklist. You can un-check Checklists)
 * - In the above cases, it's safe to keep the update code running without issue, but incrementing by one every frame
 * may cause slowdowns as there is a memory allocation that goes along with saving data.
 *
 * Once set, the value can be pulled back out by running `trophyGetSavedValue()`.
 *
 * The last won trophy is saved to the NVS for later use.
 *
 * \section trophy_helpers Helper functions
 *
 * Several helper functions exist for the developer to get critical information
 * - `checkBitFlag()` & `setBitFlag()`: Two complimentary functions to handle that pesky bit shifting
 * - `trophyGetPoints()`: Get either the total number of points for this mode of for the entire swadge
 * - `trophyGetLatest()`: Returns the last saved trophy
 * - `trophySetSystemData()`: Sets the trophy System to the same as provided mode. Used primarily by Trophy Case mode.
 *
 * \section trophy_draw Drawing a trophy
 *
 * The Swadge will automatically display trophies as they're unlocked. You do not need to provide draw calls, it is
 * handled automatically.
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
 * data for each update.
 *
 * \section trophy_draw_list Drawing all the trophies
 *
 * A trophy 'gallery' type function has been provided. Initialize with `trophyDrawListInit()`, set the colors with
 * `trophyDrawListColors()`, and display with `trophyDrawList()`. It displays a long list that can be scrolled by
 * providing new y values. Remember to deinit the list with `trophyDrawListDeinit()` to cover any memory leaks.
 *
 * In addition, there is an auto-populating Trophy Case mode where this is already done for you, so you can go to see
 * the trophy data. Anything marked hidden will not be shown in there. Trophies can be sorted by unlocked, locked, or
all.
 *
 * \section trophy_clear Clearing a trophy
 *
 * Reset a trophy to 0 (but don't remove it from the list of trophies) with `trophyClear()`.
 *
 * Trophies should only be cleared under abnormal circumstances, such as the player having been caught cheating.
 * Consider that the Steamworks API (Valve) has a "Remove trophy" function, yet most people have never lost an trophy.
 *
 * Clearing a trophy will delete the points acquired if it was previously won.
 */

#pragma once

//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>
#include <stdbool.h>
#include "hdw-nvs.h"
#include "wsgPalette.h"
#include "fs_font.h"

//==============================================================================
// Defines
//==============================================================================

#define TROPHY_MAX_TITLE_LEN 48  ///< The longest possible name for a trophy
#define TROPHY_MAX_DESC_LEN  128 ///< The longest possible description for a trophy

#define DRAW_SLIDE_US  262144 ///< Recommended slide time, a power of 2 about a quarter second
#define DRAW_STATIC_US 524288 ///< Recommended static time, a power of 2 about a half second

#define NO_IMAGE_SET CNFS_NUM_FILES ///< Special value for ::trophyData_t.image to indicate no image

//==============================================================================
// Enum
//==============================================================================

/// @brief Types of trophies the devs can instantiate
typedef enum
{
    TROPHY_TYPE_TRIGGER,   ///< Only needs to be triggered once (Did a handstand, found a hidden message)
    TROPHY_TYPE_ADDITIVE,  ///< Each update adds to the previous (Lifetime distance, number of cakes eaten)
    TROPHY_TYPE_PROGRESS,  ///< Tracks how far a user got per update (How long player survived, how far player ran)
    TROPHY_TYPE_CHECKLIST, ///< Each bit is a flag for a specific action. Repeating actions does not increment.
                           ///< (Used for collecting different objects, like chaos emeralds)
} trophyType_t;

/// @brief Dev inferred difficulty of achieving, used to distribute points.
typedef enum
{
    TROPHY_DIFF_FINAL,   ///< Only used by the trophy System. Will break normal trophies.
    TROPHY_DIFF_EASY,    ///< The easiest trophies to unlock
    TROPHY_DIFF_MEDIUM,  ///< A bit more difficult to unlock than ::TROPHY_DIFF_EASY
    TROPHY_DIFF_HARD,    ///< A trophy that takes serious skill to unlock
    TROPHY_DIFF_EXTREME, ///< A trophy that takes skill, dedication, and insanity to unlock
} trophyDifficulty_t;

/// @brief What display mode to use to draw trophies
typedef enum
{
    TROPHY_DISPLAY_ALL,         ///< Display locked and unlocked trophies, but not hidden ones
    TROPHY_DISPLAY_UNLOCKED,    ///< Display only unlocked trophies
    TROPHY_DISPLAY_LOCKED,      ///< Display only locked trophies
    TROPHY_DISPLAY_INCL_HIDDEN, ///< Display locked, unlocked, and hidden trophies
} trophyListDisplayMode_t;

//==============================================================================
// Structs
//==============================================================================

/// @brief Individual Trophy data objects
typedef struct
{
    char title[TROPHY_MAX_TITLE_LEN];      ///< Name of the Trophy, used as ID
    char description[TROPHY_MAX_DESC_LEN]; ///< Short description of task required
    cnfsFileIdx_t image;                   ///< Index of the image
    trophyType_t type;                     ///< Type of trophy. See "trophy.h" for descriptions
    trophyDifficulty_t difficulty;         ///< How many points the trophy is worth
    int32_t maxVal;                        ///< The value that
    bool hidden;                           ///< If trophy is hidden by default
    bool noImage;                          ///< If trophy will use an image
    const void* identifier; ///< An optional identifier to find trophy data. May be set to a pointer or an integer cast
                            ///< to `intptr_t`
} trophyData_t;

/// @brief Settings for the trophy system
typedef struct
{
    bool drawFromBottom;      ///< If banner should be drawn from the bottom of the screen
    int32_t staticDurationUs; ///< How long the banner will be drawn fully extended
    int32_t slideDurationUs;  ///< How long the banner will take to slide in and out
    const char* namespaceKey; ///< key used for trophy namespace
} trophySettings_t;

/// @brief The data object dev hands to the trophy showcase that contains all the const data.
typedef struct
{
    int32_t length;                   ///< Length of the trophy arrays
    const trophyData_t* const list;   ///< Array of trophies
    const trophySettings_t* settings; ///< Setting data
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
void trophySystemInit(const trophyDataList_t* settings, const char* modeName);

// Utilize trophies

/**
 * @brief Updates specifed trophy if required
 *
 * @param t Trophy to update
 * @param newVal New value to try to set. Behavior is set by trophy type
 * @param drawUpdate If this update should be drawn to the screen
 * @return true if a notification is being drawn, false otherwise
 */
bool trophyUpdate(const trophyData_t* t, int newVal, bool drawUpdate);

/**
 * @brief Updates just like `trophyUpdate()`, but only draws when crossing percentage boundary
 *
 * @param t Trophy to update
 * @param newVal Value to attempt to set
 * @param threshold Value (0-100, representing a percent) to draw at
 * @return true if a notification is being drawn, false otherwise
 */
bool trophyUpdateMilestone(const trophyData_t* t, int newVal, int threshold);

/**
 * @brief Returns the value saved to the NVS or 0 if the key isn't found.
 *
 * @param t Trophy to grab value for
 * @return int32_t Stored value for the requested trophy
 */
int32_t trophyGetSavedValue(const trophyData_t* t);

/**
 * @brief Sets or unsets a checklist item.
 *
 * @param t Trophy to set
 * @param flag Task that was just completed
 * @param unset If we're unsetting the bit
 * @param drawUpdate If this update should be drawn
 * @return true if a notification is being drawn, false otherwise
 */
bool trophySetChecklistTask(const trophyData_t* t, int32_t flag, bool unset, bool drawUpdate);

/**
 * @brief Erases completion data from swadge. Only use in extreme circumstances.
 *
 * @param t Trophy to set to 0
 */
void trophyClear(const trophyData_t* t);

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
 * @brief Get the Latest Trophy Idx object
 *
 * @return trophyData_t data for the latest win
 */
const trophyData_t* trophyGetLatest(void);

/**
 * @brief Set the Trophy System to a specific mode's values
 *
 * @param dl Data object from the mode
 * @param modeName Name of the mode
 */
void trophySetSystemData(const trophyDataList_t* dl, const char* modeName);

// Drawing functions

/**
 * @brief Draws the banner if one is queued
 *
 * @param fnt Font to be used
 * @param elapsedUs Time since last frame
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

/**
 * @brief Check if a trophy is currently being drawn
 *
 * @return true if a trophy is being drawn, false otherwise
 */
bool isTrophyDrawing(void);