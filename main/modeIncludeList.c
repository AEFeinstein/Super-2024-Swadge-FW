//==============================================================================
// Includes
//==============================================================================

#include "modeIncludeList.h"

//==============================================================================
// Variables
//==============================================================================

/// @brief Add swadgeMode_t pointers to this struct to include them in emulator and main menu
swadgeMode_t* allSwadgeModes[] = {
    &accelTestMode,   &bigbugMode,  &bongoTest,     &cGroveMode,  &colorchordMode, &danceMode,
    &factoryTestMode, &gamepadMode, &introMode,     &jukeboxMode, &keebTestMode,   &mainMenuMode,
    &modeCredits,     &pangoMode,   &sequencerMode, &sokoMode,    &swadgeHeroMode, &synthMode,
    &t48Mode,         &timerMode,   &touchTestMode, &tttMode,     &tunernomeMode,
};

//==============================================================================
// Functions
//==============================================================================

int swadgeModeCount()
{
    return ARRAY_SIZE(allSwadgeModes);
}

void modeListSetMenu(menu_t* menu)
{
    // Games sub menu
    menu = startSubMenu(menu, "Games");
    addSingleItemToMenu(menu, bigbugMode.modeName);
    addSingleItemToMenu(menu, swadgeHeroMode.modeName);
    addSingleItemToMenu(menu, pangoMode.modeName);
    addSingleItemToMenu(menu, tttMode.modeName);
    addSingleItemToMenu(menu, cGroveMode.modeName);
    addSingleItemToMenu(menu, t48Mode.modeName);
    addSingleItemToMenu(menu, sokoMode.modeName);
    menu = endSubMenu(menu);

    // Music sub menu
    menu = startSubMenu(menu, "Music");
    addSingleItemToMenu(menu, sequencerMode.modeName);
    addSingleItemToMenu(menu, bongoTest.modeName);
    addSingleItemToMenu(menu, colorchordMode.modeName);
    addSingleItemToMenu(menu, tunernomeMode.modeName);
    addSingleItemToMenu(menu, jukeboxMode.modeName);
    addSingleItemToMenu(menu, synthMode.modeName);
    menu = endSubMenu(menu);

    // Utilities sub menu
    menu = startSubMenu(menu, "Utilities");
    addSingleItemToMenu(menu, gamepadMode.modeName);
    addSingleItemToMenu(menu, danceMode.modeName);
    addSingleItemToMenu(menu, timerMode.modeName);
    addSingleItemToMenu(menu, introMode.modeName);
    menu = endSubMenu(menu);

    addSingleItemToMenu(menu, modeCredits.modeName);
}
