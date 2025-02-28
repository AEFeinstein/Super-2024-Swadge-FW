#include "modeIncludeList.h"

swadgeMode_t* allSwadgeModes[] = {
    &accelTestMode,
    &bigbugMode,
    &bongoTest,
    &cGroveMode,
    &colorchordMode,
    &danceMode,
    &factoryTestMode,
    &gamepadMode,
    &introMode,
    &jukeboxMode,
    &keebTestMode,
    &mainMenuMode,
    &modeCredits,
    &pangoMode,
    &sequencerMode,
    &sokoMode,
    &swadgeHeroMode,
    &synthMode,
    &t48Mode,
    &timerMode,
    &touchTestMode,
    &tttMode,
    &tunernomeMode,
};

int swadgeModeCount()
{
    return ARRAY_SIZE(allSwadgeModes);
}