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
#include "bongoTest.h"
#include "colorchord.h"
#include "dance.h"
#include "factoryTest.h"
#include "gamepad.h"
#include "introMode.h"
#include "jukebox.h"
#include "keebTest.h"
#include "mainMenu.h"
#include "mode_credits.h"
#include "mode_synth.h"
#include "modeTimer.h"
#include "sequencerMode.h"
#include "touchTest.h"
#include "tunernome.h"

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
 * @brief Iniitalizes a menu with all the current swadge modes
 *
 * @param menu Menu to set
 */
void modeListSetMenu(menu_t* menu);

//==============================================================================
// Externs
//==============================================================================

extern swadgeMode_t* const allSwadgeModes[];
