//==============================================================================
// Includes
//==============================================================================

#include "ext_modes.h"
#include "emu_main.h"
#include "esp_timer.h"
#include "esp_sleep_emu.h"
#include "macros.h"

#include <stdlib.h>

#include "modeIncludeList.h"

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

static const swadgeMode_t* startMode = NULL;

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

swadgeMode_t* const* emulatorGetSwadgeModes(int* count)
{
    *count = modeListGetCount();
    return allSwadgeModes;
}

const swadgeMode_t* emulatorFindSwadgeMode(const char* name)
{
    for (uint8_t i = 0; i < modeListGetCount(); i++)
    {
        if (!strncasecmp(allSwadgeModes[i]->modeName, name, strlen(name)))
        {
            return allSwadgeModes[i];
        }
    }

    return NULL;
}

swadgeMode_t* getRandomSwadgeMode(void)
{
    return allSwadgeModes[rand() % modeListGetCount()];
}

bool emulatorSetSwadgeModeByName(const char* name)
{
    const swadgeMode_t* mode = emulatorFindSwadgeMode(name);

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
