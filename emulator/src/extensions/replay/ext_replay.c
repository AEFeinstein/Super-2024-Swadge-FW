#include "ext_replay.h"
#include "emu_ext.h"
#include "esp_timer.h"
#include "hdw-btn.h"
#include "hdw-btn_emu.h"
#include "hdw-imu.h"
#include "hdw-imu_emu.h"
#include "macros.h"
#include "emu_main.h"
#include "ext_modes.h"
#include "ext_tools.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>
#include <unistd.h>

#include "hdw-tft_emu.h"

//==============================================================================
// Defines
//==============================================================================

#define HEADER "Time,Type,Value\n"

#ifdef DEBUG
    #define REPLAY_DEBUG(str, ...) printf(str "\n", __VA_ARGS__);
#else
    #define REPLAY_DEBUG(str, ...)
#endif

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    RECORD,
    REPLAY,
} replayMode_t;

typedef enum
{
    BUTTON_PRESS,
    BUTTON_RELEASE,
    TOUCH_PHI,
    TOUCH_R,
    TOUCH_INTENSITY,
    ACCEL_X,
    ACCEL_Y,
    ACCEL_Z,
    FUZZ,
    QUIT,
    SCREENSHOT,
    SET_MODE,
} replayLogType_t;

#define LAST_TYPE SET_MODE

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    int64_t time;

    replayLogType_t type;

    union
    {
        buttonBit_t buttonVal;
        int32_t touchVal;
        int16_t accelVal;
        char* filename;
        char* modeName;
    };
} replayEntry_t;

typedef struct
{
    FILE* file;
    bool readCompleted;

    replayMode_t mode;
    bool headerHandled;

    buttonBit_t lastButtons;

    int32_t lastTouchPhi;
    int32_t lastTouchR;
    int32_t lastTouchIntensity;

    int16_t lastAccelX;
    int16_t lastAccelY;
    int16_t lastAccelZ;

    replayEntry_t nextEntry;
} replay_t;

//==============================================================================
// Function Prototypes
//==============================================================================

static bool replayInit(emuArgs_t* emuArgs);
static void replayRecordFrame(uint64_t frame);
static void replayPlaybackFrame(uint64_t frame);
static void replayPreFrame(uint64_t frame);

static bool readEntry(replayEntry_t* out);
static void writeEntry(const replayEntry_t* entry);

//==============================================================================
// Variables
//==============================================================================

static const char* replayLogTypeStrs[] = {
    "BtnDown", "BtnUp",  "TouchPhi", "TouchR", "TouchI",     "AccelX",
    "AccelY",  "AccelZ", "Fuzz",     "Quit",   "Screenshot", "SetMode",
};

static const char* replayButtonNames[] = {
    "Up", "Down", "Left", "Right", "A", "B", "Start", "Select",
};

emuExtension_t replayEmuExtension = {
    .name            = "replay",
    .fnInitCb        = replayInit,
    .fnPreFrameCb    = replayPreFrame,
    .fnPostFrameCb   = NULL,
    .fnKeyCb         = NULL,
    .fnMouseMoveCb   = NULL,
    .fnMouseButtonCb = NULL,
    .fnRenderCb      = NULL,
};

replay_t replay = {0};

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initialize the replay extension
 *
 * @param emuArgs
 * @return true If the extension is enabled and will etiher record or play back.
 * @return false
 */
static bool replayInit(emuArgs_t* emuArgs)
{
    if (emuArgs->record)
    {
        // Construct a timestamp-based filename
        char buf[64];
        const char* filename = emuArgs->recordFile;

        if (!filename || !*filename)
        {
            filename = getTimestampFilename(buf, sizeof(buf) - 1, "rec-", "csv");
        }

        // If specified, use custom filename, otherwise use timestamp one
        printf("\nReplay: Recording inputs to file %s\n", filename);
        replay.file = fopen(filename, "w");
        replay.mode = RECORD;
        return replay.file != NULL;
    }
    else if (emuArgs->playback)
    {
        printf("\nReplay: Replaying inputs from file %s\n", emuArgs->replayFile);
        replay.file = fopen(emuArgs->replayFile, "r");
        replay.mode = REPLAY;

        // Return true if the file was opened OK and has a valid header and first entry
        return NULL != replay.file && readEntry(&replay.nextEntry);
    }

    return false;
}

static void replayRecordFrame(uint64_t frame)
{
    replayEntry_t logEntry = {0};

    if (!replay.headerHandled)
    {
        replay.headerHandled = true;
        fwrite(HEADER, 1, strlen(HEADER), replay.file);
    }

    logEntry.time = esp_timer_get_time();

    int32_t touchPhi, touchR, touchIntensity;
    if (!getTouchJoystick(&touchPhi, &touchR, &touchIntensity))
    {
        touchPhi       = 0;
        touchR         = 0;
        touchIntensity = 0;
    }

    int16_t accelX, accelY, accelZ;
    accelGetOrientVec(&accelX, &accelY, &accelZ);

    buttonBit_t curButtons = emulatorGetButtonState();

    for (replayLogType_t type = BUTTON_PRESS; type <= LAST_TYPE; type += 1)
    {
        logEntry.type = type;

        switch (type)
        {
            case BUTTON_PRESS:
            {
                for (uint8_t i = 0; i < 8; i++)
                {
                    buttonBit_t btn = (1 << i);
                    if ((curButtons & btn) != (replay.lastButtons & btn))
                    {
                        bool press         = (curButtons & btn) == btn;
                        logEntry.type      = press ? BUTTON_PRESS : BUTTON_RELEASE;
                        logEntry.buttonVal = btn;
                        writeEntry(&logEntry);

                        if (press)
                        {
                            replay.lastButtons |= btn;
                        }
                        else
                        {
                            replay.lastButtons &= ~btn;
                        }
                    }
                }
                break;
            }

            case BUTTON_RELEASE:
                // Handled already by BUTTON_PRESS for fastness
                break;

            case TOUCH_PHI:
            {
                if (touchPhi != replay.lastTouchPhi)
                {
                    logEntry.accelVal = touchPhi;
                    writeEntry(&logEntry);
                }
                break;
            }

            case TOUCH_R:
            {
                if (touchR != replay.lastTouchR)
                {
                    logEntry.touchVal = touchR;
                    writeEntry(&logEntry);
                }
                break;
            }

            case TOUCH_INTENSITY:
            {
                if (touchIntensity != replay.lastTouchIntensity)
                {
                    logEntry.touchVal = touchIntensity;
                    writeEntry(&logEntry);
                }
                break;
            }

            case ACCEL_X:
            {
                if (accelX != replay.lastAccelX)
                {
                    logEntry.accelVal = accelX;
                    writeEntry(&logEntry);
                }
                break;
            }

            case ACCEL_Y:
            {
                if (accelY != replay.lastAccelY)
                {
                    logEntry.accelVal = accelY;
                    writeEntry(&logEntry);
                }
                break;
            }

            case ACCEL_Z:
            {
                if (accelZ != replay.lastAccelZ)
                {
                    logEntry.accelVal = accelZ;
                    writeEntry(&logEntry);
                }
                break;
            }

            // These would be manually inserted, so no need to handle writing
            case FUZZ:
            case QUIT:
            case SCREENSHOT:
            case SET_MODE:
                break;
        }
    }

    replay.lastTouchR         = touchR;
    replay.lastTouchPhi       = touchPhi;
    replay.lastTouchIntensity = touchIntensity;
    replay.lastAccelX         = accelX;
    replay.lastAccelY         = accelY;
    replay.lastAccelZ         = accelZ;

    // Flush all the entries to the file so that we can close the file the proper way
    // which is obviously to let the OS deal with it when the process exits
    fflush(replay.file);
}

/**
 * @brief Play back any recorded actions queued for the given frame
 *
 * @param frame
 */
static void replayPlaybackFrame(uint64_t frame)
{
    // Unless we've finished reading the file completely
    if (!replay.readCompleted)
    {
        int64_t time           = esp_timer_get_time();
        int32_t touchPhi       = replay.lastTouchPhi;
        int32_t touchR         = replay.lastTouchR;
        int32_t touchIntensity = replay.lastTouchIntensity;

        int16_t accelX = replay.lastAccelX;
        int16_t accelY = replay.lastAccelY;
        int16_t accelZ = replay.lastAccelZ;

        while (time >= replay.nextEntry.time)
        {
            switch (replay.nextEntry.type)
            {
                case BUTTON_PRESS:
                {
                    replay.lastButtons |= replay.nextEntry.buttonVal;
                    REPLAY_DEBUG("Injecting button %x down\n", replay.nextEntry.buttonVal);
                    emulatorInjectButton(replay.nextEntry.buttonVal, true);
                    break;
                }

                case BUTTON_RELEASE:
                {
                    replay.lastButtons &= (~replay.nextEntry.buttonVal);
                    REPLAY_DEBUG("Injecting button %x up\n", replay.nextEntry.buttonVal);
                    emulatorInjectButton(replay.nextEntry.buttonVal, false);
                    break;
                }

                case TOUCH_PHI:
                {
                    touchPhi = replay.nextEntry.touchVal;
                    break;
                }

                case TOUCH_R:
                {
                    touchR = replay.nextEntry.touchVal;
                    break;
                }

                case TOUCH_INTENSITY:
                {
                    touchIntensity = replay.nextEntry.touchVal;
                    break;
                }

                case ACCEL_X:
                {
                    accelX = replay.nextEntry.accelVal;
                    break;
                }

                case ACCEL_Y:
                {
                    accelY = replay.nextEntry.accelVal;
                    break;
                }

                case ACCEL_Z:
                {
                    accelZ = replay.nextEntry.accelVal;
                    break;
                }

                case FUZZ:
                {
                    emulatorArgs.fuzz        = true;
                    emulatorArgs.fuzzButtons = true;
                    emulatorArgs.fuzzTouch   = true;
                    emulatorArgs.fuzzMotion  = true;

                    printf("Replay: Enabling Fuzzer");
                    enableExtension("fuzzer");
                    break;
                }

                case QUIT:
                {
                    printf("Replay: Stopping Emulator\n");
                    emulatorQuit();
                    break;
                }

                case SCREENSHOT:
                {
                    if (!takeScreenshot(replay.nextEntry.filename))
                    {
                        printf("ERR: Replay: Couldn't save screenshot!\n");
                    }

                    if (NULL != replay.nextEntry.filename)
                    {
                        // This string is dynamically alloated, so delete it
                        free(replay.nextEntry.filename);
                        replay.nextEntry.filename = NULL;
                    }
                    break;
                }

                case SET_MODE:
                {
                    if (NULL != replay.nextEntry.modeName)
                    {
                        if (emulatorSetSwadgeModeByName(replay.nextEntry.modeName))
                        {
                            printf("Replay: Set mode to '%s'\n", replay.nextEntry.modeName);
                        }
                        else
                        {
                            printf("ERR: Replay: Can't find mode '%s'!", replay.nextEntry.modeName);
                        }

                        // This string is dynamically alloated, so delete it
                        free(replay.nextEntry.modeName);
                        replay.nextEntry.modeName = NULL;
                    }
                    break;
                }
            }

            // Get the next entry
            if (!readEntry(&replay.nextEntry))
            {
                printf("Replay: Reached end of recording\n");
                replay.readCompleted = true;
                break;
            }
        }

        if (touchPhi != replay.lastTouchPhi || touchR != replay.lastTouchR
            || touchIntensity != replay.lastTouchIntensity)
        {
            REPLAY_DEBUG("Updating touch to Phi=%d, R=%d, Intensity=%d\n", touchPhi, touchR, touchIntensity);
            emulatorSetTouchJoystick(touchPhi, touchR, touchIntensity);
            replay.lastTouchPhi       = touchPhi;
            replay.lastTouchR         = touchR;
            replay.lastTouchIntensity = touchIntensity;
        }

        if (accelX != replay.lastAccelX || accelY != replay.lastAccelY || accelZ != replay.lastAccelZ)
        {
            REPLAY_DEBUG("Updating accel to X=%d, Y=%d, Z=%d\n", accelX, accelY, accelZ);
            emulatorSetAccelerometer(accelX, accelY, accelZ);
            replay.lastAccelX = accelX;
            replay.lastAccelY = accelY;
            replay.lastAccelZ = accelZ;
        }
    }
}

static void replayPreFrame(uint64_t frame)
{
    switch (replay.mode)
    {
        case RECORD:
        {
            replayRecordFrame(frame);
            break;
        }

        case REPLAY:
        {
            replayPlaybackFrame(frame);
            break;
        }
    }
}

static bool readEntry(replayEntry_t* entry)
{
    char buffer[64];
    if (!replay.headerHandled)
    {
        if (1 != fscanf(replay.file, "%63[^\n]\n", buffer) || strncmp(buffer, HEADER, strlen(buffer)))
        {
            // Couldn't read, anything.
            printf("ERR: Invalid playback file, could not parse header\n");
            return false;
        }

        replay.headerHandled = true;
    }

    int result;
    // Read timestamp index
    result = fscanf(replay.file, "%" PRId64 ",", &replay.nextEntry.time);

    // Check if the index key was readable
    if (result != 1)
    {
        if (EOF == result)
        {
            // EOF returned; return false without printing an error
            return false;
        }
        else
        {
            printf("ERR: Can't read Time from recording: %d\n", result);
        }
        return false;
    }

    if (1 != fscanf(replay.file, "%63[^,],", buffer))
    {
        printf("ERR: Can't read action type\n");
        return false;
    }

    for (replayLogType_t type = BUTTON_PRESS; type <= LAST_TYPE; type += 1)
    {
        const char* str = replayLogTypeStrs[type];
        if (!strncmp(str, buffer, sizeof(buffer) - 1))
        {
            replay.nextEntry.type = type;
            break;
        }

        if (type == LAST_TYPE)
        {
            printf("ERR: No action type matched '%s'\n", buffer);
            return false;
            // not found
        }
    }

    switch (replay.nextEntry.type)
    {
        case BUTTON_PRESS:
        case BUTTON_RELEASE:
        {
            if (1 != fscanf(replay.file, "%63s\n", buffer))
            {
                printf("ERR: Can't read button name\n");
                return false;
            }

            for (uint8_t i = 0; i < 8; i++)
            {
                buttonBit_t button = (1 << i);
                if (!strncmp(replayButtonNames[i], buffer, sizeof(buffer) - 1))
                {
                    replay.nextEntry.buttonVal = button;
                    break;
                }

                if (i == 7)
                {
                    // Should have broken by now, throw error
                    printf("ERR: Can't find button matching '%s'\n", buffer);
                    return false;
                }
            }

            break;
        }

        case TOUCH_PHI:
        case TOUCH_R:
        case TOUCH_INTENSITY:
        {
            if (1 != fscanf(replay.file, "%" PRId32 "\n", &replay.nextEntry.touchVal))
            {
                return false;
            }
            break;
        }

        case ACCEL_X:
        case ACCEL_Y:
        case ACCEL_Z:
        {
            if (1 != fscanf(replay.file, "%hd\n", &replay.nextEntry.accelVal))
            {
                return false;
            }
            break;
        }

        case FUZZ:
        {
            // Just advance to the next line
            fscanf(replay.file, "%*[^\n]\n");

            break;
        }

        case QUIT:
        {
            // Just advance to the next line
            while (fgetc(replay.file) != '\n')
                ;
            break;
        }

        case SCREENSHOT:
        {
            // Read the filename from the screenshot
            if (1 != fscanf(replay.file, "%63[^\n]\n", buffer))
            {
                // Skip to the end
                while (fgetc(replay.file) != '\n')
                    ;
                entry->filename = NULL;
            }
            else
            {
                char* tmpStr = malloc(strlen(buffer) + 1);
                strncpy(tmpStr, buffer, strlen(buffer) + 1);
                entry->filename = tmpStr;
            }

            break;
        }

        case SET_MODE:
        {
            // Read the mode name from the file
            if (1 != fscanf(replay.file, "%63[^\n]\n", buffer))
            {
                return false;
            }

            char* tmpStr = malloc(strlen(buffer) + 1);
            strncpy(tmpStr, buffer, strlen(buffer) + 1);
            entry->modeName = tmpStr;

            break;
        }

        default:
        {
            return false;
        }
    }

    return true;
}

static void writeEntry(const replayEntry_t* entry)
{
    char buffer[256];
    char* ptr = buffer;
#define BUFSIZE (buffer + sizeof(buffer) - 1 - ptr)

    // Write time key
    ptr += snprintf(ptr, BUFSIZE, "%" PRId64 ",", entry->time);

    // Write entry type
    ptr += snprintf(ptr, BUFSIZE, "%s,", replayLogTypeStrs[entry->type]);

    switch (entry->type)
    {
        case BUTTON_PRESS:
        case BUTTON_RELEASE:
        {
            // Find button index
            int i = 0;
            while ((1 << i) != entry->buttonVal && i < 7)
            {
                i++;
            }

            // TODO: Check for invalid button index?
            snprintf(ptr, BUFSIZE, "%s\n", replayButtonNames[i]);
            break;
        }

        case TOUCH_PHI:
        case TOUCH_R:
        case TOUCH_INTENSITY:
        {
            snprintf(ptr, BUFSIZE, "%" PRId32 "\n", entry->touchVal);
            break;
        }

        case ACCEL_X:
        case ACCEL_Y:
        case ACCEL_Z:
        {
            snprintf(ptr, BUFSIZE, "%" PRId16 "\n", entry->accelVal);
            break;
        }

        case FUZZ:
        case QUIT:
        {
            break;
        }

        case SCREENSHOT:
        {
            snprintf(ptr, BUFSIZE, "%s\n", entry->filename ? entry->filename : "");
            break;
        }

        case SET_MODE:
        {
            break;
        }
    }

    fwrite(buffer, 1, strlen(buffer), replay.file);
}

/**
 * @brief Notifies the replay extension that a screenshot was taken
 *
 * @param name The screenshot filename, or NULL if none was used
 */
void recordScreenshotTaken(const char* name)
{
    // Check that we're recording, otherwise we don't do anything
    if (replay.mode == RECORD && replay.file)
    {
        // Create a copy of the filename since entry.filename is not const
        char tmp[strlen(name) + 1];
        strcpy(tmp, name);

        replayEntry_t entry = {
            .time     = esp_timer_get_time(),
            .type     = SCREENSHOT,
            .filename = tmp,
        };
        writeEntry(&entry);
    }
}
