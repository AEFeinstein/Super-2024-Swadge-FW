#include "emu_console_cmds.h"
#include "macros.h"

#include "ext_modes.h"
#include "ext_tools.h"
#include "emu_utils.h"
#include "ext_replay.h"

// Console command handlers
static int screenshotCommandCb(const char** args, int argCount, char* out);
static int setModeCommandCb(const char** args, int argCount, char* out);
static int recordCommandCb(const char** args, int argCount, char* out);
static int replayCommandCb(const char** args, int argCount, char* out);
static int helpCommandCb(const char** args, int argCount, char* out);

static const consoleCommand_t consoleCommands[] = {
    { .name = "screenshot", .cb = screenshotCommandCb },
    { .name = "mode",       .cb = setModeCommandCb },
    { .name = "record",     .cb = recordCommandCb },
    { .name = "replay",     .cb = replayCommandCb },
    { .name = "help",       .cb = helpCommandCb },
};

const consoleCommand_t* getConsoleCommands(void)
{
    return consoleCommands;
}

int consoleCommandCount(void)
{
    return ARRAY_SIZE(consoleCommands);
}

static int screenshotCommandCb(const char** args, int argCount, char* out)
{
    if (argCount > 0)
    {
        takeScreenshot(args[0]);
    }
    else
    {
        takeScreenshot(NULL);
    }

    return 0;
}

static int setModeCommandCb(const char** args, int argCount, char* out)
{
    if (argCount > 0)
    {
        if (!emulatorSetSwadgeModeByName(args[0]))
        {
            return snprintf(out, 1024, "Unable to find mode '%s'", args[0]);
        }
        else
        {
            swadgeMode_t* swadgeMode = emulatorFindSwadgeMode(args[0]);
            if (swadgeMode)
            {
                return snprintf(out, 1024, "Opening mode '%s'", swadgeMode->modeName);
            }
        }

        return 0;
    }
    else
    {
        char* cur = out;

        int modeCount;
        swadgeMode_t** swadgeModes = emulatorGetSwadgeModes(&modeCount);
        for (int i = 0; i < modeCount; i++)
        {
            cur += snprintf(cur, 1024 - (cur - out), "%s\n", swadgeModes[i]->modeName);
        }

        return (cur - out);
    }
}

static int recordCommandCb(const char** args, int argCount, char* out)
{
    const char* name = NULL;

    if (isScreenRecording())
    {
        stopScreenRecording();
        return sprintf(out, "Recording finished");
    }
    else
    {
        if (argCount > 0)
        {
            name = args[0];
        }

        startRecording(name);
        return sprintf(out, "Recording started");
    }
}

static int replayCommandCb(const char** args, int argCount, char* out)
{
    return sprintf(out, "Playback started");
}

static int helpCommandCb(const char** args, int argCount, char* out)
{
    char* cur = out;

    cur += snprintf(cur, 1024, "Available Commands:\n");
    for (const consoleCommand_t* action = consoleCommands; action < (consoleCommands + ARRAY_SIZE(consoleCommands)); action++)
    {
        cur += snprintf(cur, 1024 - (cur - out), "- %s\n", action->name);
    }

    return (cur - out);
}
