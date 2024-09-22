//==============================================================================
// Includes
//==============================================================================

#include "ext_modes.h"
#include "emu_main.h"
#include "esp_timer.h"
#include "esp_sleep_emu.h"
#include "macros.h"

#include <stdlib.h>

// Mode Includes
/*
 Quickly regenerate with:
   grep -lirE '^extern swadgeMode_t (.*);' main | grep -oE '([^/]+\.h)' \
    | grep -v quickSettings | awk '{printf "#include \"%s\"\n",$1 }' | sort
*/
#include "accelTest.h"
#include "colorchord.h"
#include "dance.h"
#include "factoryTest.h"
#include "gamepad.h"
#include "introMode.h"
#include "jukebox.h"
#include "keebTest.h"
#include "mainMenu.h"
#include "mode_2048.h"
#include "mode_bigbug.h"
#include "mode_credits.h"
#include "mode_synth.h"
#include "modeTimer.h"
#include "pango.h"
#include "soko.h"
#include "touchTest.h"
#include "tunernome.h"
#include "ultimateTTT.h"

//==============================================================================
// Defines
//==============================================================================

#define ONE_SECOND 1000000 // us

//==============================================================================
// Function Prototypes
//==============================================================================

bool modesInitCb(emuArgs_t* args);
void modesPreFrameCb(uint64_t frame);
static swadgeMode_t* getRandomSwadgeMode(void);

//==============================================================================
// Variables
//==============================================================================

/*
 Quickly regenerate with:
   grep -hirE '^extern swadgeMode_t (.*);' main/modes/ | awk '{print $3}' \
     | sed -E 's/(.*);/\&\1,/g' | grep -v quickSettings | sort
*/
// clang-format off
static swadgeMode_t* allSwadgeModes[] = {
    &accelTestMode,
    &bigbugMode,
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
    &sokoMode,
    &synthMode,
    &t48Mode,
    &timerMode,
    &touchTestMode,
    &tttMode,
    &tunernomeMode,
};
// clang-format on

emuExtension_t modesEmuExtension = {
    .name            = "modes",
    .fnInitCb        = modesInitCb,
    .fnPreFrameCb    = modesPreFrameCb,
    .fnPostFrameCb   = NULL,
    .fnKeyCb         = NULL,
    .fnMouseMoveCb   = NULL,
    .fnMouseButtonCb = NULL,
    .fnRenderCb      = NULL,
};

static swadgeMode_t* startMode = NULL;

//==============================================================================
// Functions
//==============================================================================

bool modesInitCb(emuArgs_t* args)
{
    if (args->lock)
    {
        emulatorSetSwadgeModeLocked(true);
    }

    if (args->startMode)
    {
        startMode = emulatorFindSwadgeMode(args->startMode);

        if (!startMode)
        {
            printf("ERR: No swadge mode matching '%s' found.\n", args->startMode);
            emulatorQuit();
            return false;
        }
    }

    return NULL != startMode || args->modeSwitchTime != 0;
}

void modesPreFrameCb(uint64_t frame)
{
    if (frame == 1 && startMode != NULL)
    {
        emulatorForceSwitchToSwadgeMode(startMode);
    }
    else if (emulatorArgs.modeSwitchTime != 0)
    {
        // Periodic mode switching is enabled!
        // Keep track of when we need to switch modes
        static int64_t lastTime = 0;
        static int64_t timer    = 0;

        if (lastTime == 0)
        {
            lastTime = esp_timer_get_time();
            timer    = ((int64_t)emulatorArgs.modeSwitchTime) * ONE_SECOND;
        }
        else
        {
            int64_t now     = esp_timer_get_time();
            int64_t elapsed = now - lastTime;
            lastTime        = now;
            if (elapsed >= timer)
            {
                timer = ((int64_t)emulatorArgs.modeSwitchTime * ONE_SECOND) - (elapsed - timer);
                emulatorForceSwitchToSwadgeMode(getRandomSwadgeMode());
            }
            else
            {
                timer -= elapsed;
            }
        }
    }
}

swadgeMode_t** emulatorGetSwadgeModes(int* count)
{
    *count = ARRAY_SIZE(allSwadgeModes);
    return allSwadgeModes;
}

swadgeMode_t* emulatorFindSwadgeMode(const char* name)
{
    for (uint8_t i = 0; i < ARRAY_SIZE(allSwadgeModes); i++)
    {
        if (!strncmp(allSwadgeModes[i]->modeName, name, strlen(name)))
        {
            return allSwadgeModes[i];
        }
    }

    return NULL;
}

swadgeMode_t* getRandomSwadgeMode(void)
{
    return allSwadgeModes[rand() % ARRAY_SIZE(allSwadgeModes)];
}

bool emulatorSetSwadgeModeByName(const char* name)
{
    swadgeMode_t* mode = emulatorFindSwadgeMode(name);

    if (NULL != mode)
    {
        emulatorForceSwitchToSwadgeMode(mode);
        return true;
    }
    else
    {
        return false;
    }
}