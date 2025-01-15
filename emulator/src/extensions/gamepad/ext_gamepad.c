#include "ext_gamepad.h"

#include "emu_utils.h"
#include "hdw-btn_emu.h"
#include "hdw-imu_emu.h"
#include "macros.h"
#include "trigonometry.h"

#include <esp_log.h>

#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

#if defined(EMU_WINDOWS)
    #include <Windows.h>
    #include <joystickapi.h>
#elif defined(EMU_LINUX)
    #include <linux/joystick.h>
    #include <fcntl.h>
    #include <unistd.h>
#elif defined(EMU_MACOS)
#endif

typedef struct
{
    const char* name;
    struct
    {
        int xAxis;
        int yAxis;
        int deadzone;
    } touchpad;

    struct
    {
        int xAxis;
        int yAxis;
        int zAxis;
    } accel;

    struct
    {
        int xAxis;
        int yAxis;
    } dpad;

    buttonBit_t buttonMap[32];
} emuJoystickConf_t;

const emuJoystickConf_t joyPresetSwadge = {
    .name = "Swadge",
    .touchpad = {
        .xAxis = 0,
        .yAxis = 1,
        .deadzone = 0,
    },

    .accel = {
        .xAxis = 5,
        .yAxis = 3,
        .zAxis = 4,
    },

    .dpad = {
        .xAxis = 6,
        .yAxis = 7,
    },

    .buttonMap = {
        PB_A, // 0
        PB_B, // 1
        0, 0, 0, 0, 0, 0, 0, 0,
        PB_SELECT, // 10
        PB_START, // 11
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    },
};

const emuJoystickConf_t joyPresetSwitch = {
    .name = "Switch",
    .touchpad = {
        .xAxis = 2,
        .yAxis = 3,
        .deadzone = 1024,
    },

    .accel = {
        .xAxis = -1,
        .yAxis = -1,
        .zAxis = -1,
    },

    .dpad = {
        .xAxis = 4,
        .yAxis = 5,
    },

    .buttonMap = {
        PB_B, // 0 (B)
        PB_A, // 1 (A)
        PB_B, // 2 (X)
        PB_A, // 3 (Y)
        0, 0, 0, 0, 0,
        PB_SELECT, // 9 (-)
        PB_START, // 10 (+)
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    },
};

#ifdef EMU_WINDOWS
typedef struct
{
    UINT deviceNum;
    // Represents the last state transmitted to the consumer
    JOYINFOEX curState;
    // Represents the actual state most recently returned by the OS
    JOYINFOEX newState;
    bool pendingState;
    // Whether the second POV hat axis is pending
    bool pendingPov;
} emuWinJoyData_t;
#endif

typedef enum
{
    BUTTON,
    AXIS,
} emuJoystickEventType_t;

typedef struct
{
    emuJoystickEventType_t type;
    union
    {
        uint16_t axis;
        uint16_t button;
    };
    int16_t value;

} emuJoystickEvent_t;

typedef struct
{
    int numAxes;
    int numButtons;
    void* data;
    int16_t* axisData;
    uint8_t* buttonData;
    bool connected;
} emuJoystick_t;

bool gamepadInitCb(emuArgs_t* args);
void gamepadDeinitCb(void);
void gamepadPreFrameCb(uint64_t frame);

bool gamepadReadEvent(emuJoystick_t* joystick, emuJoystickEvent_t* event);
bool gamepadConnect(emuJoystick_t* joystick, const char* name);
void gamepadDisconnect(emuJoystick_t* joystick);
int16_t getAxisData(int axis);

emuExtension_t gamepadEmuExtension = {
    .name         = "gamepad",
    .fnInitCb     = gamepadInitCb,
    .fnDeinitCb   = gamepadDeinitCb,
    .fnPreFrameCb = gamepadPreFrameCb,
};

static emuJoystick_t joystickExt        = {0};
static emuJoystickConf_t joystickConfig = {0};

bool gamepadConnect(emuJoystick_t* joystick, const char* name)
{
#if defined(EMU_WINDOWS)
    int winDevNum = -1;
    if (name != NULL)
    {
        char* endptr  = NULL;
        errno         = 0;
        int parsedVal = strtol(name, &endptr, 10);

        if (errno != 0 || endptr == name || parsedVal > 15)
        {
            printf("ERR: Invalid joystick device number '%s'\n", name);
            return false;
        }

        winDevNum = parsedVal;
    }

    UINT numDevices = joyGetNumDevs();
    JOYCAPS caps;
    for (int i = 0; i < numDevices; i++)
    {
        if (winDevNum != -1 && winDevNum != i)
        {
            // If we're trying to open a specific device, skip the other ones
            continue;
        }

        MMRESULT result = joyGetDevCaps(i, &caps, sizeof(caps));

        if (JOYERR_NOERROR == result)
        {
            void* data = calloc(1, sizeof(emuWinJoyData_t));

            if (NULL != data)
            {
                emuWinJoyData_t* winData = (emuWinJoyData_t*)data;

                winData->deviceNum = i;

                joystick->numButtons = caps.wNumButtons;
                joystick->numAxes    = caps.wNumAxes;

                if (caps.wCaps & JOYCAPS_HASPOV)
                {
                    joystick->numAxes += 2;
                }

                if (joystick->numButtons > 0)
                {
                    joystick->buttonData = calloc(joystick->numButtons, sizeof(uint8_t));
                }
                else
                {
                    joystick->buttonData = NULL;
                }

                if (joystick->numAxes > 0)
                {
                    joystick->axisData = calloc(joystick->numAxes, sizeof(int16_t));
                }
                else
                {
                    joystick->axisData = NULL;
                }

                joystick->data      = data;
                joystick->connected = true;

                return true;
            }
        }
        else if (winDevNum == i)
        {
            // Targeting a specific device and it didn't open, give up
            printf("ERR: Failed to open joystick device %d\n", winDevNum);
            return false;
        }
    }

    return false;
#elif defined(EMU_LINUX)
    const char* device = (name != NULL) ? name : "/dev/input/js0";
    int jsFd;

    jsFd = open(device, O_RDONLY | O_NONBLOCK);
    if (jsFd == -1)
    {
        ESP_LOGE("Gamepad", "Failed to open joystick device %s", device);
        return false;
    }

    joystick->data = (void*)(jsFd);
    uint8_t buttons;
    if (ioctl(jsFd, JSIOCGBUTTONS, &buttons) == -1)
    {
        joystick->numButtons = 0;
        joystick->buttonData = NULL;
    }
    else
    {
        joystick->numButtons = buttons;
        joystick->buttonData = calloc(buttons, sizeof(uint8_t));
    }

    uint8_t axes;
    if (ioctl(jsFd, JSIOCGAXES, &axes) == -1)
    {
        joystick->numAxes  = 0;
        joystick->axisData = NULL;
    }
    else
    {
        joystick->numAxes  = axes;
        joystick->axisData = calloc(axes, sizeof(int16_t));
    }

    if (joystick->numAxes == 0 && joystick->numButtons == 0)
    {
        return false;
    }

    joystick->connected = true;

    return true;
#else
    return false;
#endif
}

void gamepadDisconnect(emuJoystick_t* joystick)
{
#if defined(EMU_WINDOWS)
    if (joystick->data)
    {
        free(joystick->data);
        joystick->data = NULL;
    }
#elif defined(EMU_LINUX)
    close((int)joystick->data);
    joystick->data = NULL;
#endif

    if (joystick->axisData)
    {
        free(joystick->axisData);
        joystick->axisData = NULL;
        joystick->numAxes  = 0;
    }

    if (joystick->buttonData)
    {
        free(joystick->buttonData);
        joystick->buttonData = NULL;
        joystick->numButtons = 0;
    }

    joystick->connected = false;
}

static void applyEvent(emuJoystick_t* joystick, const emuJoystickEvent_t* event)
{
    if (event)
    {
        switch (event->type)
        {
            case AXIS:
            {
                if (event->axis < joystick->numAxes)
                {
                    joystick->axisData[event->axis] = event->value;
                }
                break;
            }

            case BUTTON:
            {
                joystick->buttonData[event->button] = (event->value) ? 1 : 0;
                break;
            }

            default:
                break;
        }
    }
}

bool gamepadReadEvent(emuJoystick_t* joystick, emuJoystickEvent_t* event)
{
    if (joystick && joystick->connected)
    {
#if defined(EMU_WINDOWS)
        emuWinJoyData_t* winData = (emuWinJoyData_t*)joystick->data;

        do
        {
            if (!winData->pendingState)
            {
                // If we're here, we have successfully transmitted all state changes via events
                // Now, try to get the new joystick state and see if it's different

                // 1. Get the current joystick state data
                // 2. Check which values are different from the last state
                // 3. Emit the event with the new data
                // 4. Update the last state to match the new data returned
                // 5. If nothing is different, set pendingState = false;
                JOYINFOEX winEvent = {0};
                // what the hell kind of API requires this
                winEvent.dwSize  = sizeof(JOYINFOEX);
                winEvent.dwFlags = JOY_RETURNALL | JOY_RETURNPOV;

                MMRESULT result = joyGetPosEx(winData->deviceNum, &winEvent);

                if (result == JOYERR_NOERROR)
                {
                    if (winEvent.dwFlags && memcmp(&winEvent, &winData->curState, sizeof(JOYINFOEX)))
                    {
                        // The new data is different from the current data so copy it over
                        memcpy(&winData->newState, &winEvent, sizeof(JOYINFOEX));
                        winData->pendingState = true;
                    }
                    else
                    {
                        // The new data is the same as what we already have, nothing more to do
                        winData->pendingState = false;
                    }
                }
                else
                {
                    gamepadDisconnect(joystick);
                }
            }

            if (winData->pendingState)
            {
    #define CALC_AXIS(n, val) (val - 32767)
                JOYINFOEX* cur = &winData->curState;
                JOYINFOEX* new = &winData->newState;
                if ((new->dwFlags& JOY_RETURNX) && cur->dwXpos != new->dwXpos)
                {
                    event->type  = AXIS;
                    event->axis  = 0;
                    event->value = CALC_AXIS(0, new->dwXpos);
                    cur->dwXpos  = new->dwXpos;
                    applyEvent(joystick, event);
                    return true;
                }
                else if ((new->dwFlags& JOY_RETURNY) && cur->dwYpos != new->dwYpos)
                {
                    event->type  = AXIS;
                    event->axis  = 1;
                    event->value = CALC_AXIS(1, new->dwYpos);
                    cur->dwYpos  = new->dwYpos;
                    applyEvent(joystick, event);
                    return true;
                }
                else if ((new->dwFlags& JOY_RETURNZ) && cur->dwZpos != new->dwZpos)
                {
                    event->type  = AXIS;
                    event->axis  = 2;
                    event->value = CALC_AXIS(2, new->dwZpos);
                    cur->dwZpos  = new->dwZpos;
                    applyEvent(joystick, event);
                    return true;
                }
                else if ((new->dwFlags& JOY_RETURNR) && cur->dwRpos != new->dwRpos)
                {
                    event->type  = AXIS;
                    event->axis  = 3;
                    event->value = CALC_AXIS(3, new->dwRpos);
                    cur->dwRpos  = new->dwRpos;
                    applyEvent(joystick, event);
                    return true;
                }
                else if ((new->dwFlags& JOY_RETURNU) && cur->dwUpos != new->dwUpos)
                {
                    event->type  = AXIS;
                    event->axis  = 4;
                    event->value = CALC_AXIS(4, new->dwUpos);
                    cur->dwUpos  = new->dwUpos;
                    applyEvent(joystick, event);
                    return true;
                }
                else if ((new->dwFlags& JOY_RETURNV) && cur->dwVpos != new->dwVpos)
                {
                    event->type  = AXIS;
                    event->axis  = 5;
                    event->value = CALC_AXIS(5, new->dwVpos);
                    cur->dwVpos  = new->dwVpos;
                    applyEvent(joystick, event);
                    return true;
                }
                else if ((new->dwFlags& JOY_RETURNBUTTONS) && cur->dwButtons != new->dwButtons)
                {
                    DWORD change = cur->dwButtons ^ new->dwButtons;
                    if (change != 0)
                    {
                        uint32_t buttonIdx = __builtin_ctz(change);

                        event->type   = BUTTON;
                        event->button = buttonIdx;

                        if (new->dwButtons & (1 << buttonIdx))
                        {
                            // Button pressed
                            event->value = 1;
                            cur->dwButtons |= (1 << buttonIdx);
                        }
                        else
                        {
                            // Button released
                            event->value = 0;
                            cur->dwButtons &= ~(1 << buttonIdx);
                        }

                        applyEvent(joystick, event);
                        return true;
                    }
                }
                else if ((new->dwFlags& JOY_RETURNPOV) && cur->dwPOV != new->dwPOV)
                {
                    // This is kinda weird!
                    // This is because we're splitting a single POV hat event into two discrete axes
                    // So, we just send the X axis first, and then set a flag (and don't update the 'cur' value)
                    // And next time if the flag is set, we return the Y axis instead, and then update the 'cur' value
                    event->type = AXIS;
                    if (!winData->pendingPov)
                    {
                        // Send this as the first axis (6)
                        event->axis = 6;

                        if (new->dwPOV == 65535)
                        {
                            event->value = 0;
                        }
                        else
                        {
                            event->value = CLAMP(-32 * getCos1024(((new->dwPOV / 100) + 90) % 360), -32768, 32767);
                        }

                        winData->pendingPov = true;
                    }
                    else
                    {
                        // Send this as the second axis (7)
                        event->axis = 7;

                        if (new->dwPOV == 65535)
                        {
                            event->value = 0;
                        }
                        else
                        {
                            event->value = CLAMP(-32 * getSin1024(((new->dwPOV / 100) + 90) % 360), -32768, 32767);
                        }

                        winData->pendingPov = false;
                        cur->dwPOV          = new->dwPOV;
                    }

                    applyEvent(joystick, event);
                    return true;
                }
                else
                {
                    // nothing is different!
                    // check one more time
                    winData->pendingState = false;
                    continue;
                }
            }
        } while (0);
#elif defined(EMU_LINUX)
        struct js_event linuxEvent;

        errno     = 0;
        int count = read((int)joystick->data, &linuxEvent, sizeof(struct js_event));
        if (count >= (int)sizeof(struct js_event))
        {
            switch (linuxEvent.type)
            {
                case JS_EVENT_BUTTON:
                {
                    event->type   = BUTTON;
                    event->button = linuxEvent.number;
                    event->value  = linuxEvent.value;
                    break;
                }

                case JS_EVENT_AXIS:
                {
                    event->type  = AXIS;
                    event->axis  = linuxEvent.number;
                    event->value = linuxEvent.value;
                    break;
                }

                default:
                {
                }
            }

            applyEvent(joystick, event);
            return true;
        }
        else if (count <= 0)
        {
            if (errno == EAGAIN)
            {
                return false;
            }

            ESP_LOGE("GamepadExt", "Failed to read, disconnecting gamepad");
            gamepadDisconnect(joystick);

            return false;
        }
        else
        {
            ESP_LOGE("GamepadExt", "Short read of %d", count);
            return false;
        }
#endif
    }

    return false;
}

int16_t getAxisData(int axis)
{
    if (axis >= 0 && axis < joystickExt.numAxes)
    {
        return joystickExt.axisData[axis];
    }

    return 0;
}

bool emuGamepadConnected(void)
{
    return joystickExt.connected;
}

/**
 * @brief Applies a preset joystick configuration
 *
 * Valid preset options are:
 * - "Swadge"
 * - "Switch"
 *
 * @param presetName The name of the joystick preset to use
 * @return true
 * @return false
 */
bool emuSetGamepadPreset(const char* presetName)
{
    if (!strcasecmp(presetName, joyPresetSwadge.name))
    {
        memcpy(&joystickConfig, &joyPresetSwadge, sizeof(emuJoystickConf_t));
    }
    else if (!strcasecmp(presetName, joyPresetSwitch.name))
    {
        memcpy(&joystickConfig, &joyPresetSwitch, sizeof(emuJoystickConf_t));
        return true;
    }

    return false;
}

/**
 * @brief Sets the deadzone of the touchpad axes, with 0 being no deadzone and 32767 being all deadzone
 */
void emuSetTouchpadDeadzone(int deadzone)
{
    if (deadzone >= 0 && deadzone < 32768)
    {
        joystickConfig.touchpad.deadzone = deadzone;
    }
}

/**
 * @brief Maps a joystick button to an emulator button
 *
 * Using a value of 0 for button will un-map the joystick button
 *
 * @param buttonIdx The joystick button number to map
 * @param button The emulator button value to set
 */
void emuSetGamepadButtonMapping(uint8_t buttonIdx, buttonBit_t button)
{
    if (buttonIdx < 32)
    {
        joystickConfig.buttonMap[buttonIdx] = button;
    }
}

/**
 * @brief Maps two joystick axes onto the emulator touchpad
 *
 * A negative axis value means no axis will be mapped onto that value
 *
 * @param xAxis The axis to map to the horizontal touchpad value
 * @param yAxis The axis to map to the vertical touchpad value
 */
void emuSetTouchpadAxisMapping(int xAxis, int yAxis)
{
    printf("Mapping touchpad to axes %d and %d\n", xAxis, yAxis);
    joystickConfig.touchpad.xAxis = xAxis;
    joystickConfig.touchpad.yAxis = yAxis;
}

/**
 * @brief Maps three joystick axes onto the emulator accelerometer data
 *
 * @param xAxis The axis to map onto the accelerometer X axis
 * @param yAxis The axis to map onto the accelerometer Y axis
 * @param zAxis The axis to map onto the accelerometer Z axis
 */
void emuSetAccelAxisMapping(int xAxis, int yAxis, int zAxis)
{
    printf("Mapping motion to axes %d, %d, and %d\n", xAxis, yAxis, zAxis);
    joystickConfig.accel.xAxis = xAxis;
    joystickConfig.accel.yAxis = yAxis;
    joystickConfig.accel.zAxis = zAxis;
}

/**
 * @brief Maps two joystick axes onto the emulator D-pad
 *
 * @param xAxis The axis to map onto the horizontal D-pad value
 * @param yAxis The axis to map onto the vertical D-pad value
 */
void emuSetDpadAxisMapping(int xAxis, int yAxis)
{
    printf("Mapping D-pad to axes %d and %d\n", xAxis, yAxis);
    joystickConfig.dpad.xAxis = xAxis;
    joystickConfig.dpad.yAxis = yAxis;
}

/**
 * Returns the current deadzone used for the touchpad axes.
 *
 * @return A number from 0 to 32767, with 0 being no deadzone.
 */
int emuGetTouchpadDeadzone(void)
{
    return joystickConfig.touchpad.deadzone;
}

/**
 * @brief Writes the current joystick button mapping to the provided array
 *
 * The corresponding `buttonBit_t` mapping, or 0 for unmapped buttons, will be written
 * to the provided array at the index corresponding to the mapped joystick button.
 *
 * @param[out] buttons A pointer to an array of buttonBit_t with a length of at least 32
 */
void emuGetGamepadButtonMapping(buttonBit_t buttons[32])
{
    if (buttons)
    {
        for (int i = 0; i < 32; i++)
        {
            buttons[i] = joystickConfig.buttonMap[i];
        }
    }
}

/**
 * @brief Writes the current joystick axes mapped to the emulator touchpad
 *
 * Values that are not mapped to any axis will be set to -1
 *
 * @param[out] xAxis A pointer to be set with the joystick axis mapped to the horizontal touchpad
 * @param[out] yAxis A pointer to be set with the joystick axis mapped to the vertical touchpad
 */
void emuGetTouchpadAxisMapping(int* xAxis, int* yAxis)
{
    if (xAxis)
    {
        *xAxis = joystickConfig.touchpad.xAxis >= 0 ? joystickConfig.touchpad.xAxis : -1;
    }

    if (yAxis)
    {
        *yAxis = joystickConfig.touchpad.yAxis >= 0 ? joystickConfig.touchpad.yAxis : -1;
    }
}

/**
 * @brief Writes the current joystick axes mapped to the emulator accelerometer
 *
 * Values that are not mapped to any axis will be set to -1
 *
 * @param[out] xAxis A pointer to be set with the joystick axis mapped to the accelerometer X axis
 * @param[out] yAxis A pointer to be set with the joystick axis mapped to the accelerometer Y axis
 * @param[out] zAxis A pointer to be set with the joystick axis mapped to the accelerometer Z axis
 */
void emuGetAccelAxisMapping(int* xAxis, int* yAxis, int* zAxis)
{
    if (xAxis)
    {
        *xAxis = joystickConfig.accel.xAxis >= 0 ? joystickConfig.accel.xAxis : -1;
    }

    if (yAxis)
    {
        *yAxis = joystickConfig.accel.yAxis >= 0 ? joystickConfig.accel.yAxis : -1;
    }

    if (zAxis)
    {
        *zAxis = joystickConfig.accel.zAxis >= 0 ? joystickConfig.accel.zAxis : -1;
    }
}

/**
 * @brief Writes the current joystick axes mapped to the emulator D-pad
 *
 * @param[out] xAxis A pointer to be set with the joystick axis mapped to the horizontal D-pad value
 * @param[out] yAxis A pointer to be set with the joystick axis mapped to the vertical D-pad value
 */
void emuGetDpadAxisMapping(int* xAxis, int* yAxis)
{
    if (xAxis)
    {
        *xAxis = joystickConfig.dpad.xAxis >= 0 ? joystickConfig.dpad.xAxis : -1;
    }

    if (yAxis)
    {
        *yAxis = joystickConfig.dpad.yAxis >= 0 ? joystickConfig.dpad.yAxis : -1;
    }
}

bool gamepadInitCb(emuArgs_t* args)
{
    memcpy(&joystickConfig, &joyPresetSwadge, sizeof(emuJoystickConf_t));
    if (gamepadConnect(&joystickExt, args->joystick))
    {
        ESP_LOGI("GamepadExt", "Connected to joystick with %d axes and %d buttons\n", joystickExt.numAxes,
                 joystickExt.numButtons);
        return true;
    }

    return false;
}

void gamepadDeinitCb(void)
{
    gamepadDisconnect(&joystickExt);
}

void gamepadPreFrameCb(uint64_t frame)
{
    emuJoystickEvent_t event;
    while (gamepadReadEvent(&joystickExt, &event))
    {
        switch (event.type)
        {
            case BUTTON:
            {
                if (event.button < 32 && joystickConfig.buttonMap[event.button])
                {
                    emulatorInjectButton(joystickConfig.buttonMap[event.button], event.value != 0);
                }
                break;
            }

            case AXIS:
            {
                if (event.axis == joystickConfig.touchpad.xAxis || event.axis == joystickConfig.touchpad.yAxis)
                {
                    // Joystick data comes in as a value from (-2**15) to (2**15-1)
                    // We need to take that down to (-128) to (127) (divide by 8)
                    // Then need to take the atan2 of it
                    double x = (double)getAxisData(joystickConfig.touchpad.xAxis);
                    double y = (double)getAxisData(joystickConfig.touchpad.yAxis);

                    // printf("I|GamepadExt| %03.1f, %03.1f --> ", x, y);

                    if ((x >= -joystickConfig.touchpad.deadzone && x <= joystickConfig.touchpad.deadzone)
                        && (y >= -joystickConfig.touchpad.deadzone && y <= joystickConfig.touchpad.deadzone))
                    {
                        emulatorSetTouchJoystick(0, 0, 0);
                        // printf("0, 0\n");
                    }
                    else
                    {
                        const double maxRadius = 1024;

                        double angle        = atan2(-y, x);
                        double angleDegrees = (angle * 180) / M_PI;
                        int radius          = CLAMP(sqrt(x * x + y * y) * maxRadius / 32768, 0, maxRadius);

                        // printf("r=%d, phi=%03.2f\n", radius, angleDegrees);

                        int angleDegreesRounded = (int)(angleDegrees + 360) % 360;
                        emulatorSetTouchJoystick(CLAMP(angleDegreesRounded, 0, 359), radius, 1024);
                    }
                }
                else if (event.axis == joystickConfig.accel.xAxis || event.axis == joystickConfig.accel.yAxis
                         || event.axis == joystickConfig.accel.zAxis)
                {
                    int16_t accelX = getAxisData(joystickConfig.accel.xAxis) / 128;
                    int16_t accelY = getAxisData(joystickConfig.accel.yAxis) / 128;
                    int16_t accelZ = getAxisData(joystickConfig.accel.zAxis) / 128;

                    emulatorSetAccelerometer(accelX, accelY, accelZ);
                }
                else if (event.axis == joystickConfig.dpad.xAxis || event.axis == joystickConfig.dpad.yAxis)
                {
                    buttonBit_t curState = 0;
                    if (getAxisData(joystickConfig.dpad.xAxis) < -256)
                    {
                        curState |= PB_LEFT;
                    }
                    else if (getAxisData(joystickConfig.dpad.xAxis) > 256)
                    {
                        curState |= PB_RIGHT;
                    }

                    if (getAxisData(joystickConfig.dpad.yAxis) < -256)
                    {
                        curState |= PB_UP;
                    }
                    else if (getAxisData(joystickConfig.dpad.yAxis) > 256)
                    {
                        curState |= PB_DOWN;
                    }

                    buttonBit_t lastState = emulatorGetButtonState();
                    buttonBit_t changes   = (lastState ^ curState) & ((1 << 8) - 1);

                    if (changes & PB_UP)
                    {
                        emulatorInjectButton(PB_UP, (curState & PB_UP));
                    }
                    else if (changes & PB_DOWN)
                    {
                        emulatorInjectButton(PB_DOWN, (curState & PB_DOWN));
                    }

                    if (changes & PB_LEFT)
                    {
                        emulatorInjectButton(PB_LEFT, (curState & PB_LEFT));
                    }
                    else if (changes & PB_RIGHT)
                    {
                        emulatorInjectButton(PB_RIGHT, (curState & PB_RIGHT));
                    }
                }
            }
            break;
        }
    }
}
