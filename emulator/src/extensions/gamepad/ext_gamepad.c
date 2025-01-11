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

#if defined(EMU_WINDOWS)
    #include <Windows.h>
    #include <joystickapi.h>
#elif defined(EMU_LINUX)
    #include <linux/joystick.h>
    #include <fcntl.h>
    #include <unistd.h>
#elif defined(EMU_MACOS)
#endif

// Defines the mapping of the default Swadge Gamepad mode
#define TOUCH_AXIS_X 0
#define TOUCH_AXIS_Y 1

#define ACCEL_AXIS_X 5
#define ACCEL_AXIS_Y 3
#define ACCEL_AXIS_Z 4

#define DPAD_AXIS_X 6
#define DPAD_AXIS_Y 7

const buttonBit_t joystickButtonMap[] = {
    PB_A, PB_B, 0, 0, 0, 0, 0, 0, 0, 0, PB_SELECT, PB_START, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
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
} emuJoystick_t;

bool gamepadInitCb(emuArgs_t* args);
void gamepadDeinitCb(void);
void gamepadPreFrameCb(uint64_t frame);

bool gamepadReadEvent(emuJoystick_t* joystick, emuJoystickEvent_t* event);
bool gamepadConnect(emuJoystick_t* joystick);
void gamepadDisconnect(emuJoystick_t* joystick);

emuExtension_t gamepadEmuExtension = {
    .name         = "gamepad",
    .fnInitCb     = gamepadInitCb,
    .fnDeinitCb   = gamepadDeinitCb,
    .fnPreFrameCb = gamepadPreFrameCb,
};

static emuJoystick_t joystickExt = {0};

bool gamepadConnect(emuJoystick_t* joystick)
{
#if defined(EMU_WINDOWS)
    UINT numDevices = joyGetNumDevs();
    JOYCAPS caps;
    for (int i = 0; i < numDevices; i++)
    {
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

                joystick->data = data;

                return true;
            }
        }
    }

    return false;
#elif defined(EMU_LINUX)
    const char* device = "/dev/input/js0";
    intptr_t jsFd;

    jsFd = open(device, O_RDONLY | O_NONBLOCK);
    if (jsFd == -1)
    {
        ESP_LOGE("Gamepad", "Failed to open joystick device /dev/input/js0");
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
        joystick->numAxes = 0;
    }
    else
    {
        joystick->numAxes  = axes;
        joystick->axisData = calloc(axes, sizeof(int16_t));
    }

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
    close((intptr_t)joystick->data);
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
    if (joystick && joystick->data)
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
        int count = read((intptr_t)joystick->data, &linuxEvent, sizeof(struct js_event));
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
            if (joystick->axisData)
            {
                free(joystick->axisData);
                joystick->axisData = NULL;
            }
            if (joystick->buttonData)
            {
                free(joystick->buttonData);
                joystick->buttonData = NULL;
            }

            close((intptr_t)joystick->data);
            joystick->data = NULL;
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

bool gamepadInitCb(emuArgs_t* args)
{
    if (gamepadConnect(&joystickExt))
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
                if (event.button < 32 && joystickButtonMap[event.button])
                {
                    emulatorInjectButton(joystickButtonMap[event.button], event.value != 0);
                }
                break;
            }

            case AXIS:
            {
                switch (event.axis)
                {
                    case TOUCH_AXIS_X:
                    case TOUCH_AXIS_Y:
                    {
                        // Joystick data comes in as a value from (-2**15) to (2**15-1)
                        // We need to take that down to (-128) to (127) (divide by 8)
                        // Then need to take the atan2 of it
                        double x = (double)joystickExt.axisData[TOUCH_AXIS_X];
                        double y = (double)joystickExt.axisData[TOUCH_AXIS_Y];

                        if (x == 0 && y == 0)
                        {
                            emulatorSetTouchJoystick(0, 0, 0);
                        }
                        else
                        {
                            const double maxRadius = 1024;

                            double angle        = atan2(-y, x);
                            double angleDegrees = (angle * 180) / M_PI;
                            int radius          = CLAMP(sqrt(x * x + y * y) * maxRadius / 32768, 0, maxRadius);

                            int angleDegreesRounded = (int)(angleDegrees + 360) % 360;
                            emulatorSetTouchJoystick(CLAMP(angleDegreesRounded, 0, 359), radius, 1024);
                        }
                        break;
                    }

                    case ACCEL_AXIS_X:
                    case ACCEL_AXIS_Y:
                    case ACCEL_AXIS_Z:
                    {
                        int16_t accelX = joystickExt.axisData[ACCEL_AXIS_X] / 128;
                        int16_t accelY = joystickExt.axisData[ACCEL_AXIS_Y] / 128;
                        int16_t accelZ = joystickExt.axisData[ACCEL_AXIS_Z] / 128;

                        emulatorSetAccelerometer(accelX, accelY, accelZ);
                        break;
                    }

                    case DPAD_AXIS_X:
                    case DPAD_AXIS_Y:
                    {
                        buttonBit_t curState = 0;
                        if (joystickExt.axisData[DPAD_AXIS_X] < 0)
                        {
                            curState |= PB_LEFT;
                        }
                        else if (joystickExt.axisData[DPAD_AXIS_X] > 0)
                        {
                            curState |= PB_RIGHT;
                        }

                        if (joystickExt.axisData[DPAD_AXIS_Y] < 0)
                        {
                            curState |= PB_UP;
                        }
                        else if (joystickExt.axisData[DPAD_AXIS_Y] > 0)
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
                        break;
                    }

                    default:
                    {
                        break;
                    }
                }
                break;
            }
        }
    }
}
