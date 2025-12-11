/*! \file modeIncludeList.h
 *
 * \section modeList_usage Usage
 *
 * This file simplifies adding your mode to the emulator, main menu, and other systems that require knowledge of
 * multiple modes.
 *
 * \section modeList_changes Changes
 *
 * To add a new mode, simply add `#include "your_mode_name.h"` to the bottom of the includes list in modeIncludesList.h
 * before going to modeIncludeList.c and performing the following two actions:
 * - Add extern swadgemode_t from your_mode_name.h to allSwadgeModes. Don't forget the `&`.
 * - Add a new `addSingleItemToMenu(menu, yourMode.modeName);` line in the appropriate section of modeListSetMenu()
 *
 * Currently, there's three categories: Games, Music, and Utilities. Add a new mode to the correct section of the
 * modeListSetMenu() to avoid causing confusion.
 *
 * Devs can also get the count of mode via the modeListGetCount() function, which just returns the count of items in the
 * allSwadgeModes array.
 */

//==============================================================================
// Includes
//==============================================================================

/*
 Quickly regenerate with:
   grep -lirE '^extern swadgeMode_t (.*);' main | grep -oE '([^/]+\.h)' \
    | grep -v quickSettings | awk '{printf "#include \"%s\"\n",$1 }' | sort
*/

#include "accelTest.h"
#include "artillery.h"
#include "bouncy.h"
#include "canvas.h"
#include "colorchord.h"
#include "cosCrunch.h"
#include "dance.h"
#include "danceNetwork.h"
#include "factoryTest.h"
#include "faceFinder.h"
#include "gamepad.h"
#include "introMode.h"
#include "jukebox.h"
#include "keebTest.h"
#include "mainMenu.h"
#include "megaPulseEx.h"
#include "mode_ch32v003test.h"
#include "mode_credits.h"
#include "mode_diceroller.h"
#include "mode_synth.h"
#include "nameTest.h"
#include "picross_menu.h"
#include "roboRunner.h"
#include "sequencerMode.h"
#include "sonaTest.h"
#include "swadgedoku.h"
#include "swsnCreator.h"
#include "swadgeIt.h"
#include "swadgePassTest.h"
#include "swadgetamatone.h"
#include "touchTest.h"
#include "trophyCase.h"
#include "trophyTest.h"
#include "tunernome.h"
#include "cipher.h"

//==============================================================================
// Function Prototypes
//==============================================================================

/**
 * @brief Gets the current number of swadgeModes present int he file
 *
 * @return int num of modes
 */
int modeListGetCount(void);

/**
 * @brief Initializes a menu with all the current swadge modes
 *
 * @param menu Menu to set
 */
void modeListSetMenu(menu_t* menu);

/**
 * @brief Add secret items to a menu
 *
 * @param menu The menu to add secret items to
 */
void modeListAddSecretMenuModes(menu_t* menu);

/**
 * @brief Get the unique index of a mode
 *
 * @param mode The mode to get an index for
 * @return The index of the mode
 */
int32_t getModeIdx(const swadgeMode_t* mode);

//==============================================================================
// Externs
//==============================================================================

extern swadgeMode_t* const allSwadgeModes[];
