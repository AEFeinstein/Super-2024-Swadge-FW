//==============================================================================
// Includes
//==============================================================================

#include "modeIncludeList.h"

//==============================================================================
// Variables
//==============================================================================

/*
 Quickly regenerate with:
   grep -hirE '^extern swadgeMode_t (.*);' main/modes/ | awk '{print $3}' \
     | sed -E 's/(.*);/\&\1,/g' | grep -v quickSettings | sort
*/

/// @brief Add swadgeMode_t pointers to this struct to include them in emulator and main menu
swadgeMode_t* const allSwadgeModes[] = {
    &accelTestMode, &colorchordMode, &danceMode,   &factoryTestMode, &gamepadMode, &introMode,     &jukeboxMode,
    &keebTestMode,  &mainMenuMode,   &modeCredits, &sequencerMode,   &synthMode,   &touchTestMode, &tunernomeMode,
};

//==============================================================================
// Functions
//==============================================================================

int modeListGetCount()
{
    return ARRAY_SIZE(allSwadgeModes);
}

void modeListSetMenu(menu_t* menu)
{
    // Games sub menu
    menu = startSubMenu(menu, "Games");
    menu = endSubMenu(menu);

    // Music sub menu
    menu = startSubMenu(menu, "Music");
    addSingleItemToMenu(menu, sequencerMode.modeName);
    addSingleItemToMenu(menu, colorchordMode.modeName);
    addSingleItemToMenu(menu, tunernomeMode.modeName);
    addSingleItemToMenu(menu, jukeboxMode.modeName);
    addSingleItemToMenu(menu, synthMode.modeName);
    menu = endSubMenu(menu);

    // Utilities sub menu
    menu = startSubMenu(menu, "Utilities");
    addSingleItemToMenu(menu, gamepadMode.modeName);
    addSingleItemToMenu(menu, danceMode.modeName);
    addSingleItemToMenu(menu, introMode.modeName);
    menu = endSubMenu(menu);

    addSingleItemToMenu(menu, modeCredits.modeName);
}
