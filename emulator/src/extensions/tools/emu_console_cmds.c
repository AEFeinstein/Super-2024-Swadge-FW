#include "emu_console_cmds.h"
#include "macros.h"

#include "ext_modes.h"
#include "ext_tools.h"
#include "emu_utils.h"
#include "ext_replay.h"
#include "ext_fuzzer.h"

// Console command handlers
static int screenshotCommandCb(const char** args, int argCount, char* out);
static int screenRecordCommandCb(const char** args, int argCount, char* out);
static int setModeCommandCb(const char** args, int argCount, char* out);
static int recordCommandCb(const char** args, int argCount, char* out);
static int replayCommandCb(const char** args, int argCount, char* out);
static int fuzzCommandCb(const char** args, int argCount, char* out);
static int touchCommandCb(const char** args, int argCount, char* out);
static int helpCommandCb(const char** args, int argCount, char* out);

static const char* buttonNames[] = {
    "Up", "Down", "Left", "Right", "A", "B", "Start", "Select",
};

// command, usage, description
// just a simple way to document sub-commands, etc.
static const char* commandDocs[][3] = {
    {"screenshot", "screenshot [filename]",
     "saves a screenshot to [filename], or an auto-generated file name if not specified"},
    {"gif", "gif [filename]",
     "starts or stops recording the screen to a GIF named [filename], or an auto-generated file name if not specified"},
    {"mode", "mode [name]", "immediately changes the mode to [name], or lists all mode names if not specified"},
    {"replay", "replay [filename]", "open and replay recorded inputs from replay file [filename]"},
    {"record", "record [name]",
     "begin recording inputs into replay file [filename], or an auto-generated file name if not specified"},
    {"fuzz", "fuzz [on|off]", "toggles the fuzzer"},
    {"fuzz buttons", "fuzz buttons [on|off]", "toggles fuzzing of button inputs"},
    {"fuzz buttons mask", "fuzz buttons mask [<button>[ <button>  ...]]",
     "sets or prints which buttons will be fuzzed\n    Valid options: Up, Down, Left, Right, A, B, Start, or Select"},
    {"fuzz touch", "fuzz touch [on|off]", "toggles fuzzing of touchpad inputs"},
    {"fuzz motion", "fuzz motion [on|off]", "toggles fuzzing of accelerometer motion inputs"},
    {"fuzz time", "fuzz time [on|off]", "toggles fuzzing of frame times"},
    {"touch", "touch [on|off]", "toggles the virtual touchpad"},
    {"help", "help [command]", "prints help text for all commands, or for commands matching [command]"},
};

static const consoleCommand_t consoleCommands[] = {
    {.name = "screenshot", .cb = screenshotCommandCb}, {.name = "mode", .cb = setModeCommandCb},
    {.name = "gif", .cb = screenRecordCommandCb},      {.name = "replay", .cb = replayCommandCb},
    {.name = "record", .cb = recordCommandCb},         {.name = "fuzz", .cb = fuzzCommandCb},
    {.name = "touch", .cb = touchCommandCb},           {.name = "help", .cb = helpCommandCb},
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

static int screenRecordCommandCb(const char** args, int argCount, char* out)
{
    const char* name = NULL;

    if (isScreenRecording())
    {
        stopScreenRecording();
        return sprintf(out, "GIF Recording finished\n");
    }
    else
    {
        if (argCount > 0)
        {
            name = args[0];
        }

        startScreenRecording(name);
        return sprintf(out, "GIF Recording started\n");
    }
}

static int replayCommandCb(const char** args, int argCount, char* out)
{
    if (argCount > 0)
    {
        startPlayback(args[1]);
        return sprintf(out, "Playback started\n");
    }
    else
    {
        return sprintf(out, "Filename is required!\n");
    }
}

static int recordCommandCb(const char** args, int argCount, char* out)
{
    if (argCount > 0)
    {
        startRecording(args[0]);
        return sprintf(out, "Input recording started\n");
    }
    else
    {
        if (isRecordingInput())
        {
            stopRecording();
            return sprintf(out, "Input recording stopped\n");
        }
        else
        {
            startRecording(NULL);
            return sprintf(out, "Input recording started\n");
        }
    }
}

static int fuzzCommandCb(const char** args, int argCount, char* out)
{
    if (argCount > 0)
    {
        // on
        // off
        // buttons (toggle)
        // touch (toggle)
        // motion (toggle)

        if (!strncmp("on", args[0], strlen(args[0])))
        {
            // fuzz on
            if (!emulatorArgs.fuzz)
            {
                emulatorArgs.fuzz        = true;
                emulatorArgs.fuzzButtons = emuGetFuzzButtonsEnabled();
                emulatorArgs.fuzzMotion  = emuGetFuzzMotionEnabled();
                emulatorArgs.fuzzTouch   = emuGetFuzzTouchEnabled();
                emulatorArgs.fuzzTime    = emuGetFuzzTimeEnabled();
                enableExtension("fuzzer");

                return sprintf(out, "Fuzzer enabled\n");
            }
        }
        else if (!strncmp("off", args[0], strlen(args[0])))
        {
            // fuzz off
            if (emulatorArgs.fuzz)
            {
                emulatorArgs.fuzz = false;
                disableExtension("fuzzer");

                return sprintf(out, "Fuzzer disabled\n");
            }
        }
        else if (!strncmp("buttons", args[0], strlen(args[0])))
        {
            if (argCount > 1)
            {
                // fuzz buttons ...
                if (!strncmp("on", args[1], strlen(args[1])))
                {
                    // fuzz buttons on
                    emuSetFuzzButtonsEnabled(true);

                    return sprintf(out, "Fuzzing buttons enabled\n");
                }
                else if (!strncmp("off", args[1], strlen(args[1])))
                {
                    // fuzz buttons off
                    emuSetFuzzButtonsEnabled(false);

                    return sprintf(out, "Fuzzing buttons disabled\n");
                }
                else if (!strncmp("mask", args[1], strlen(args[1])))
                {
                    // Set mask (fuzz buttons mask up down left right a b start select)
                    if (argCount > 2)
                    {
                        // fuzz buttons mask ...
                        buttonBit_t mask = 0;
                        for (int argNum = 2; argNum < argCount; argNum++)
                        {
                            for (int i = 0; i < 8; i++)
                            {
                                buttonBit_t btn        = (buttonBit_t)(1 << i);
                                const char* buttonName = buttonNames[i];

                                if (!strcasecmp(buttonName, args[argNum]))
                                {
                                    mask |= btn;
                                    break;
                                }
                            }
                        }
                        emuSetFuzzButtonsMask(mask);
                    }

                    // fuzz buttons mask[ ...]

                    // Print mask, regardless of whether it was just changed
                    // this is part of the code path for both "fuzz buttons mask" and e.g. "fuzz buttons mask a b up"
                    int written = 0;
                    written += sprintf(out + written, "Fuzzing buttons:\n");

                    buttonBit_t mask = emuGetFuzzButtonsMask();

                    for (int i = 0; i < 8; i++)
                    {
                        buttonBit_t btn = (buttonBit_t)(1 << i);

                        if (mask & btn)
                        {
                            written += sprintf(out + written, "- %s\n", buttonNames[i]);
                        }
                    }

                    return written;
                }
            }
            else
            {
                emuSetFuzzButtonsEnabled(!emuGetFuzzButtonsEnabled());

                return sprintf(out, "Fuzzing buttons %s\n", emuGetFuzzButtonsEnabled() ? "enabled" : "disabled");
            }
        }
        else if (!strncmp("touch", args[0], strlen(args[0])))
        {
            if (argCount > 1)
            {
                if (!strncmp("on", args[1], strlen(args[1])))
                {
                    emuSetFuzzTouchEnabled(true);
                    return sprintf(out, "Fuzzing touch enabled\n");
                }
                else if (!strncmp("off", args[1], strlen(args[1])))
                {
                    emuSetFuzzTouchEnabled(false);
                    return sprintf(out, "Fuzzing touch disabled\n");
                }
            }
            else
            {
                emuSetFuzzTouchEnabled(!emuGetFuzzTouchEnabled());

                return sprintf(out, "Fuzzing touch %s\n", emuGetFuzzTouchEnabled() ? "enabled" : "disabled");
            }
        }
        else if (!strncmp("motion", args[0], strlen(args[0])))
        {
            if (argCount > 1)
            {
                if (!strncmp("on", args[1], strlen(args[1])))
                {
                    emuSetFuzzMotionEnabled(true);
                    return sprintf(out, "Fuzzing motion enabled\n");
                }
                else if (!strncmp("off", args[1], strlen(args[1])))
                {
                    emuSetFuzzMotionEnabled(false);
                    return sprintf(out, "Fuzzing motion disabled\n");
                }
            }
            else
            {
                emuSetFuzzMotionEnabled(!emuGetFuzzMotionEnabled());

                return sprintf(out, "Fuzzing motion %s\n", emuGetFuzzMotionEnabled() ? "enabled" : "disabled");
            }
        }
        else if (!strncmp("time", args[0], strlen(args[0])))
        {
            if (argCount > 1)
            {
                if (!strncmp("on", args[1], strlen(args[1])))
                {
                    emuSetFuzzTimeEnabled(true);
                    return sprintf(out, "Fuzzing time enabled\n");
                }
                else if (!strncmp("off", args[1], strlen(args[1])))
                {
                    emuSetFuzzTimeEnabled(false);
                    return sprintf(out, "Fuzzing time disabled\n");
                }
            }
        }
    }
    else
    {
        // Toggle fuzzing on/off
        if (emulatorArgs.fuzz)
        {
            emulatorArgs.fuzz = false;
            disableExtension("fuzzer");

            return sprintf(out, "Fuzzer disabled\n");
        }
        else
        {
            emulatorArgs.fuzz        = true;
            emulatorArgs.fuzzButtons = emuGetFuzzButtonsEnabled();
            emulatorArgs.fuzzMotion  = emuGetFuzzMotionEnabled();
            emulatorArgs.fuzzTouch   = emuGetFuzzTouchEnabled();
            emulatorArgs.fuzzTime    = emuGetFuzzTimeEnabled();
            enableExtension("fuzzer");

            return sprintf(out, "Fuzzer enabled\n");
        }
    }

    return 0;
}

static int touchCommandCb(const char** args, int argCount, char* out)
{
    bool enable = false;
    if (argCount > 0)
    {
        if (!strncmp("on", args[0], strlen(args[0])))
        {
            enable = true;
        }
        else if (!strncmp("off", args[0], strlen(args[0])))
        {
            enable = false;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        enable = !emulatorArgs.emulateTouch;
    }

    if (enable)
    {
        if (!emulatorArgs.emulateTouch)
        {
            // It needs to be reset
            disableExtension("touch");
        }
        emulatorArgs.emulateTouch = true;
        enableExtension("touch");
    }
    else
    {
        emulatorArgs.emulateTouch = false;
        disableExtension("touch");
    }
    return sprintf(out, "Touchpad %s\n", enable ? "enabled" : "disabled");
}

static int helpCommandCb(const char** args, int argCount, char* out)
{
    char* cur = out;

    if (argCount > 0)
    {
        char cmdMatchBuf[512];
        char* bufOut = cmdMatchBuf;
        for (int i = 0; i < argCount; i++)
        {
            if (i > 0)
            {
                bufOut += snprintf(bufOut, sizeof(cmdMatchBuf) - (bufOut - cmdMatchBuf), " %s", args[i]);
            }
            else
            {
                bufOut += snprintf(bufOut, sizeof(cmdMatchBuf) - (bufOut - cmdMatchBuf), "%s", args[i]);
            }
        }

        printf("Looking for %s...\n", cmdMatchBuf);

        int matches = 0;
        for (int i = 0; i < (sizeof(commandDocs) / sizeof(commandDocs[0])); i++)
        {
            char** doc = commandDocs[i];

            if (!strncasecmp(cmdMatchBuf, doc[0], strlen(cmdMatchBuf)))
            {
                matches++;
                cur += snprintf(cur, 1024 - (cur - out), "- %s    ---- %s\n", doc[1], doc[2]);
            }
        }

        if (!matches)
        {
            cur += snprintf(cur, 1024 - (cur - out), "No matching commands found!\n");
        }
    }
    else
    {
        cur += snprintf(cur, 1024, "Available Commands:\n");
        for (const consoleCommand_t* action = consoleCommands; action < (consoleCommands + ARRAY_SIZE(consoleCommands));
             action++)
        {
            cur += snprintf(cur, 1024 - (cur - out), "- %s\n", action->name);
        }
        cur += snprintf(cur, 1024 - (cur - out), "Type 'help <command>' for more information\n");
    }

    return (cur - out);
}
