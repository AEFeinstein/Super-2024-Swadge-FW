#include "emu_console_cmds.h"
#include "macros.h"

#include <errno.h>

#include "ext_modes.h"
#include "ext_tools.h"
#include "emu_utils.h"
#include "ext_replay.h"
#include "ext_fuzzer.h"
#include "ext_gamepad.h"
#include "ext_screensaver.h"
#include "hdw-nvs_emu.h"
#include "emu_cnfs.h"

// Console command handlers
static int screenshotCommandCb(const char** args, int argCount, char* out);
static int screenRecordCommandCb(const char** args, int argCount, char* out);
static int setModeCommandCb(const char** args, int argCount, char* out);
static int recordCommandCb(const char** args, int argCount, char* out);
static int replayCommandCb(const char** args, int argCount, char* out);
static int fuzzCommandCb(const char** args, int argCount, char* out);
static int touchCommandCb(const char** args, int argCount, char* out);
static int ledsCommandCb(const char** args, int argCount, char* out);
static int injectCommandCb(const char** args, int argCount, char* out);
static int joystickCommandCb(const char** args, int argCount, char* out);
static int attractCommandCb(const char** args, int argCount, char* out);
static int helpCommandCb(const char** args, int argCount, char* out);

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
    {"fuzz buttons mask", "fuzz buttons mask [<button> [<button>  ...]]",
     "sets or prints which buttons will be fuzzed\n    Valid options: Up, Down, Left, Right, A, B, Start, or Select"},
    {"fuzz touch", "fuzz touch [on|off]", "toggles fuzzing of touchpad inputs"},
    {"fuzz motion", "fuzz motion [on|off]", "toggles fuzzing of accelerometer motion inputs"},
    {"fuzz time", "fuzz time [on|off]", "toggles fuzzing of frame times"},
    {"touchpad", "touchpad [on|off]", "toggles the virtual touchpad"},
    {"joystick", "joystick [on|off]", "toggles the joystick"},
    {"joystick device", "joystick device <devname>", "connects to a specific joystick device"},
    {"joystick map button", "joystick map button <btn-num> <btn-name> [<btn-num> <btn-name> ...]",
     "maps one or more joystick button numbers to button names"},
    {"joystick map touchpad", "joystick map touchpad <x-axis> <y-axis>", "maps two joystick axes to the touchpad"},
    {"joystick map motion", "joystick map motion <x-axis> <y-axis> <z-axis>",
     "maps three joystick axes to the accelerometer axes"},
    {"joystick map dpad", "joystick map dpad <x-axis> <y-axis>", "maps two joystick axes to the D-pad buttons"},
    {"joystick deadzone touchpad", "joystick deadzone touchpad <0-32767>",
     "sets the deadzone for the touchpad joystick axes"},
    {"inject", "inject <nvs|asset> <...>", "injects data into NVS or assets"},
    {"inject nvs", "inject nvs [namespace] <key> <int|str|file> <value>",
     "injects data into an NVS key. Value can be either an integer, a string, or a file path"},
    {"inject asset", "inject asset <name> <filename>", "injects a file's entire contents as an asset"},
    {"help", "help [command]", "prints help text for all commands, or for commands matching [command]"},
};

static const consoleCommand_t consoleCommands[] = {
    {.name = "screenshot", .cb = screenshotCommandCb}, {.name = "mode", .cb = setModeCommandCb},
    {.name = "gif", .cb = screenRecordCommandCb},      {.name = "replay", .cb = replayCommandCb},
    {.name = "record", .cb = recordCommandCb},         {.name = "fuzz", .cb = fuzzCommandCb},
    {.name = "touchpad", .cb = touchCommandCb},        {.name = "leds", .cb = ledsCommandCb},
    {.name = "inject", .cb = injectCommandCb},         {.name = "help", .cb = helpCommandCb},
    {.name = "joystick", .cb = joystickCommandCb},     {.name = "attract", .cb = attractCommandCb},
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
        startPlayback(args[0]);
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
                            // parseButtonName() returns 0 for invalid values which is fine here
                            mask |= parseButtonName(args[argNum]);
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
                            written += sprintf(out + written, "- %s\n", getButtonName(btn));
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

static int ledsCommandCb(const char** args, int argCount, char* out)
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
        enable = emulatorArgs.hideLeds;
    }

    if (enable)
    {
        emulatorArgs.hideLeds = false;
        enableExtension("leds");
    }
    else
    {
        emulatorArgs.hideLeds = true;
        disableExtension("leds");
    }
    return sprintf(out, "LEDs %s\n", enable ? "enabled" : "disabled");
}

static int injectCommandCb(const char** args, int argCount, char* out)
{
    if (argCount < 1)
    {
        return 0;
    }

    if (!strncmp("nvs", args[0], strlen(args[0])))
    {
        // inject nvs [namespace] <key> <int|str|file> <value>
        // inject nvs   <key> <int|str|file> <value>
        // -1      0       1        2         3           4

        if (argCount < 3)
        {
            return 0;
        }

        const char* typeArg   = args[3];
        const char* keyArg    = args[2];
        const char** valueArg = NULL;

        const char* namespace = "storage";

        if (!strcmp(typeArg, "int") || !strcmp(typeArg, "str") || !strcmp(typeArg, "file"))
        {
            namespace = args[1];
            if (argCount > 4)
            {
                valueArg = &args[4];
            }
        }
        else
        {
            typeArg = args[2];
            keyArg  = args[1];

            if (argCount > 3)
            {
                valueArg = &args[3];
            }
        }

        if (!strcmp(typeArg, "int"))
        {
            if (!valueArg)
            {
                return snprintf(out, 1024, "Value is required\n");
            }

            char* endPtr   = NULL;
            errno          = 0;
            int32_t intVal = strtol(*valueArg, &endPtr, 10);
            if (errno != 0)
            {
                return snprintf(out, 1024, "Invalid int value: %" PRId32 "\n", intVal);
            }

            // Set int32 value
            emuInjectNvs32(namespace, keyArg, intVal);

            return snprintf(out, 1024, "Set NVS %s:%s to %" PRId32 "\n", namespace, keyArg, intVal);
        }
        else if (!strcmp(typeArg, "str"))
        {
            char valueBuf[512];
            char* bufOut = valueBuf;

            if (!valueArg)
            {
                valueBuf[0] = '\0';
            }
            else
            {
                for (const char** argPtr = valueArg; argPtr < (args + argCount); argPtr++)
                {
                    if (argPtr > valueArg)
                    {
                        bufOut += snprintf(bufOut, sizeof(valueBuf) - (bufOut - valueBuf), " %s", *argPtr);
                    }
                    else
                    {
                        bufOut += snprintf(bufOut, sizeof(valueBuf) - (bufOut - valueBuf), "%s", *argPtr);
                    }
                }
            }

            emuInjectNvsBlob(namespace, keyArg, (bufOut - valueBuf), valueBuf);

            return snprintf(out, 1024, "Set NVS %s:%s to %s\n", namespace, keyArg, valueBuf);
        }
        else if (!strcmp(typeArg, "file"))
        {
            if (!valueArg)
            {
                return snprintf(out, 1024, "Filename is required\n");
            }

            char filenameBuf[1024];
            expandPath(filenameBuf, sizeof(filenameBuf), *valueArg);

            FILE* file = NULL;
            file       = fopen(filenameBuf, "rb");
            if (NULL != file)
            {
                fseek(file, 0, SEEK_END);
                size_t len = ftell(file);
                fseek(file, 0, SEEK_SET);

                uint8_t* fileData = malloc(len);

                if (NULL != fileData)
                {
                    size_t read = 0;
                    while (read < len)
                    {
                        errno = 0;
                        read += fread(fileData, 1, read - len, file);

                        if (0 != errno)
                        {
                            fclose(file);
                            free(fileData);
                            return snprintf(out, 1024, "Failed to read data from file %s\n", filenameBuf);
                        }
                    }
                    fclose(file);

                    emuInjectNvsBlob(namespace, keyArg, len, fileData);

                    free(fileData);

                    return snprintf(out, 1024, "Set NVS %s:%s to content of file '%s'\n", namespace, keyArg,
                                    filenameBuf);
                }
                fclose(file);
            }

            return snprintf(out, 1024, "Could not open file %s\n", filenameBuf);
        }
        else
        {
            return snprintf(out, 1024, "Invalid type '%s'\n", typeArg);
        }
    }
    else if (!strncmp("asset", args[0], strlen(args[0])))
    {
        // inject asset <name> <filename>
        if (argCount > 2)
        {
            char filenameBuf[1024];
            expandPath(filenameBuf, sizeof(filenameBuf), args[2]);

            if (emuCnfsInjectFile(args[1], filenameBuf))
            {
                return snprintf(out, 1024, "Set asset %s to content of file '%s'\n", args[1], filenameBuf);
            }
            else
            {
                return snprintf(out, 1024, "Could not load data from file '%s'\n", filenameBuf);
            }
        }
        else
        {
            return snprintf(out, 1024, "asset name and file name are required\n");
        }
    }
    else
    {
        return 0;
    }
}

static char joyDevName[128];
static int joystickCommandCb(const char** args, int argCount, char* out)
{
    if (argCount > 0)
    {
        if (!strncmp("on", args[0], strlen(args[0])))
        {
            disableExtension("gamepad");
            enableExtension("gamepad");

            if (emuGamepadConnected())
            {
                return snprintf(out, 1024, "Connected to joystick!\n");
            }
            else
            {
                return snprintf(out, 1024, "Failed to connect to joystick\n");
            }
        }
        else if (!strncmp("off", args[0], strlen(args[0])))
        {
            disableExtension("gamepad");

            return snprintf(out, 1024, "Joystick disconnected\n");
        }
        else if (!strncmp("device", args[0], strlen(args[0])))
        {
            const char* deviceName  = NULL;
            const char* originalArg = emulatorArgs.joystick;
            if (argCount > 1)
            {
                deviceName = args[1];
            }

            // Enable and connect to named device
            disableExtension("gamepad");

            if (deviceName == NULL)
            {
                strncpy(joyDevName, deviceName, sizeof(joyDevName));
                emulatorArgs.joystick = joyDevName;
            }
            else
            {
                emulatorArgs.joystick = joyDevName;
            }

            enableExtension("gamepad");

            if (emuGamepadConnected())
            {
                return snprintf(out, 1024, "Connected to joystick %s!\n", emulatorArgs.joystick);
            }
            else
            {
                emulatorArgs.joystick = originalArg;
                return snprintf(out, 1024, "Failed to connect to joystick %s\n", joyDevName);
            }
        }
        else if (!strncmp("map", args[0], strlen(args[0])))
        {
            if (argCount > 1)
            {
                if (!strncmp("button", args[1], strlen(args[1])))
                {
                    // joystick map button <button-idx> <button-name>
                    if (0 != (argCount % 2))
                    {
                        return snprintf(out, 1024,
                                        "ERR: Arguments must be in pairs of button number and button name\n");
                    }

                    // Get the button numbers and names in pairs
                    for (int argNum = 2; argNum < argCount; argNum += 2)
                    {
                        const char* buttonNumStr = args[argNum];
                        const char* buttonName   = args[argNum + 1];
                        char* end                = NULL;
                        int buttonNum            = strtol(buttonNumStr, &end, 10);
                        if ((buttonNum == 0 && end == buttonNumStr) || buttonNum < 0 || buttonNum > 31)
                        {
                            return snprintf(out, 1024, "ERR: Invalid button number '%s'\n", buttonNumStr);
                        }

                        buttonBit_t button = parseButtonName(buttonName);
                        emuSetGamepadButtonMapping(buttonNum, button);
                    }
                }
                else if (!strncmp("touchpad", args[1], strlen(args[1])))
                {
                    // joystick map touchpad <x-axis> <y-axis>
                    if (argCount < 4)
                    {
                        return snprintf(out, 1024, "ERR: x-axis and y-axis are required");
                    }

                    char* end = NULL;
                    int xAxis = strtol(args[2], &end, 10);
                    if (xAxis == 0 && end == args[2])
                    {
                        // negative axis to map nothing
                        xAxis = -1;
                    }

                    int yAxis = strtol(args[3], &end, 10);
                    if (yAxis == 0 && end == args[3])
                    {
                        // negative axis to map nothing
                        yAxis = -1;
                    }

                    emuSetTouchpadAxisMapping(xAxis, yAxis);
                }
                else if (!strncmp("motion", args[1], strlen(args[1])))
                {
                    // joystick map motion <x-axis> <y-axis> <z-axis>
                    if (argCount < 5)
                    {
                        return snprintf(out, 1024, "ERR: x-axis, y-axis, and z-axis are required");
                    }

                    char* end = NULL;
                    int xAxis = strtol(args[2], &end, 10);
                    if (xAxis == 0 && end == args[2])
                    {
                        // negative axis to map nothing
                        xAxis = -1;
                    }

                    int yAxis = strtol(args[3], &end, 10);
                    if (yAxis == 0 && end == args[3])
                    {
                        // negative axis to map nothing
                        yAxis = -1;
                    }

                    int zAxis = strtol(args[4], &end, 10);
                    if (zAxis == 0 && end == args[4])
                    {
                        // negative axis to map nothing
                        zAxis = -1;
                    }

                    emuSetAccelAxisMapping(xAxis, yAxis, zAxis);
                }
                else if (!strncmp("dpad", args[1], strlen(args[1])))
                {
                    // joystick map dpad <x-axis> <y-axis>
                    if (argCount < 4)
                    {
                        return snprintf(out, 1024, "ERR: x-axis and y-axis are required");
                    }

                    char* end = NULL;
                    int xAxis = strtol(args[2], &end, 10);
                    if (xAxis == 0 && end == args[2])
                    {
                        // negative axis to map nothing
                        xAxis = -1;
                    }

                    int yAxis = strtol(args[3], &end, 10);
                    if (yAxis == 0 && end == args[3])
                    {
                        // negative axis to map nothing
                        yAxis = -1;
                    }

                    emuSetDpadAxisMapping(xAxis, yAxis);
                }
            }

            // Print mapping
            char* cur = out;

            cur += snprintf(cur, 1024 - (cur - out), "Joystick Mapping");

            return cur - out;
        }
        else if (!strncmp("deadzone", args[0], strlen(args[0])))
        {
            if (argCount > 1)
            {
                if (!strncmp("touchpad", args[1], strlen(args[1])))
                {
                    int deadzone = emuGetTouchpadDeadzone();
                    if (argCount > 2)
                    {
                        char* end = NULL;
                        deadzone  = strtol(args[2], &end, 10);
                        if (deadzone == 0 && end == args[2])
                        {
                            deadzone = 0;
                        }

                        emuSetTouchpadDeadzone(deadzone);
                    }

                    return snprintf(out, 1024, "Touchpad deadzone: %d\n", deadzone);
                }
                else
                {
                    return snprintf(out, 1024, "Unrecognized command 'joystick %s %s'\n", args[0], args[1]);
                }
            }
            else
            {
                return snprintf(out, 1024, "Touchpad deadzone: %d\n", emuGetTouchpadDeadzone());
            }
        }
        else if (!strncmp("preset", args[0], strlen(args[0])))
        {
            if (argCount > 1)
            {
                if (emuSetGamepadPreset(args[1]))
                {
                    return snprintf(out, 1024, "Joystick preset %s loaded\n", args[1]);
                }
                else
                {
                    return snprintf(out, 1024, "ERR: No joystick preset '%s' found\n", args[1]);
                }
            }
            else
            {
                return snprintf(out, 1024, "Preset name is required\n");
            }
        }
        else
        {
            return snprintf(out, 1024, "Unrecognized command 'joystick %s'\n", args[0]);
        }
    }
    else
    {
        // Toggle, don't touch the joystick name
        if (emuGamepadConnected())
        {
            disableExtension("gamepad");

            return snprintf(out, 1024, "Joystick disconnected\n");
        }
        else
        {
            disableExtension("gamepad");
            enableExtension("gamepad");

            if (emuGamepadConnected())
            {
                return snprintf(out, 1024, "Connected to joystick!\n");
            }
            else
            {
                return snprintf(out, 1024, "Failed to connect to joystick\n");
            }
        }
    }
}

static int attractCommandCb(const char** args, int argCount, char* out)
{
    if (argCount > 0 && !strncmp(args[0], "end", strlen(args[0])))
    {
        emuScreensaverNext();
    }
    return 0;
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
            const char** doc = commandDocs[i];

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
