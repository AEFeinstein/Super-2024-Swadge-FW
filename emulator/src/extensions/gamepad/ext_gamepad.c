#include "ext_gamepad.h"

#include "emu_utils.h"
#include "hdw-btn_emu.h"
#include "hdw-imu_emu.h"
#include "macros.h"

#include <esp_log.h>

#include <fcntl.h>
#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>
#include <stdint.h>

#if defined(EMU_WINDOWS)
#include <Windows.h>
#include <joystickapi.h>
#elif defined(EMU_LINUX)
#include <linux/joystick.h>
#elif defined(EMU_MACOS)
#else
#error "Unrecognized platform"
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
    PB_A,
    PB_B,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    PB_SELECT,
    PB_START,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
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
    union {
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
void gamepadPreFrameCb(uint64_t frame);

bool gamepadReadEvent(emuJoystick_t* joystick, emuJoystickEvent_t* event);
bool gamepadConnect(emuJoystick_t* joystick);

emuExtension_t gamepadEmuExtension = {
    .name = "gamepad",
    .fnInitCb = gamepadInitCb,
    .fnPreFrameCb = gamepadPreFrameCb,
};

static emuJoystick_t joystick = {0};

bool gamepadConnect(emuJoystick_t* joystick)
{
#if defined(EMU_WINDOWS)
    UINT numDevices = joyGetNumDevs();
    JOYCAPS caps;
    bool set = false;
    for (int i = 0; i < numDevices; i++)
    {
        MMRESULT result = joyGetDevCaps(i, &caps, sizeof(caps));

        if (JOYERR_NOERROR == result)
        {
            void* data = calloc(1, sizeof(emuWinJoyData_t));

            if (NULL != data)
            {
                emuWinJoyData_t* winData = (emuWinJoyData_t*)data;

                printf("Joystick #%d: %s\n", i, caps.szPname);
                printf("  Buttons: %d/%d\n", caps.wNumAxes, caps.wMaxAxes);
                printf("  Axes: %d\n", caps.wNumButtons);

                winData->deviceNum = i;

                joystick->numButtons = caps.wNumButtons;
                joystick->numAxes = caps.wNumAxes;

                joystick->data = data;

                return true;
            }
        }
    }

    return false;
#elif defined(EMU_LINUX)
    const char* device = "/dev/input/js0";
    int jsFd;

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
        joystick->numAxes = axes;
        joystick->axisData = calloc(axes, sizeof(int16_t));
    }

    return true;
#else
    return false;
#endif
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

            default: break;
        }
    }
}

bool gamepadReadEvent(emuJoystick_t* joystick, emuJoystickEvent_t* event)
{
    if (joystick && joystick->data)
    {
#if defined(EMU_WINDOWS)
        emuWinJoyData_t* winData = (emuWinJoyData_t*)joystick->data;

        do {
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
                winEvent.dwSize = sizeof(JOYINFOEX);
                winEvent.dwFlags = 0;

                MMRESULT result = joyGetPosEx(winData->deviceNum, &winEvent);

                if (result == JOYERR_NOERROR)
                {
                    printf("xPos: %d\nyPos: %d\nzPos: %d\n", winEvent.dwXpos, winEvent.dwYpos, winEvent.dwZpos);
                    printf("rPos: %d\nuPos: %d\nvPos: %d\n", winEvent.dwRpos, winEvent.dwUpos, winEvent.dwVpos);
                    printf("buttons: %d\n", winEvent.dwButtons);
                    printf("dwPOV: %d\n", winEvent.dwPOV);

                    if (memcmp(&winEvent, &winData->curState, sizeof(JOYINFOEX)))
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
                JOYINFOEX* cur = &winData->curState;
                JOYINFOEX* new = &winData->newState;
                if (cur->dwXpos != new->dwXpos)
                {
                    event->type = AXIS;
                    event->axis = 0;
                    event->value = new->dwXpos;
                    cur->dwXpos = new->dwXpos;
                    return true;
                }
                else if (cur->dwYpos != new->dwYpos)
                {
                    event->type = AXIS;
                    event->axis = 1;
                    event->value = new->dwYpos;
                    cur->dwYpos = new->dwYpos;
                    return true;
                }
                else if (cur->dwZpos != new->dwZpos)
                {
                    event->type = AXIS;
                    event->axis = 2;
                    event->value = new->dwZpos;
                    cur->dwZpos = new->dwZpos;
                    return true;
                }
                else if (cur->dwRpos != new->dwRpos)
                {
                    event->type = AXIS;
                    event->axis = 3;
                    event->value = new->dwRpos;
                    cur->dwRpos = new->dwRpos;
                    return true;
                }
                else if (cur->dwUpos != new->dwUpos)
                {
                    event->type = AXIS;
                    event->axis = 4;
                    event->value = new->dwUpos;
                    cur->dwUpos = new->dwUpos;
                    return true;
                }
                else if (cur->dwVpos != new->dwVpos)
                {
                    event->type = AXIS;
                    event->axis = 5;
                    event->value = new->dwVpos;
                    cur->dwVpos = new->dwVpos;
                }
                else if (cur->dwButtons != new->dwButtons)
                {
                    uint32_t change = cur->dwButtons ^ new->dwButtons;
                    if (change != 0)
                    {
                        uint32_t buttonIdx = __builtin_ctz(change);

                        event->type = BUTTON;
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

                        return true;
                    }
                }
                else if (cur->dwPOV != new->dwPOV)
                {
                    event->type = AXIS;
                    event->axis = 6;
                    event->value = new->dwPOV;
                    cur->dwPOV = new->dwPOV;
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

        errno = 0;
        int count = read((int)joystick->data, &linuxEvent, sizeof(struct js_event));
        if (count >= (int)sizeof(struct js_event))
        {
            switch (linuxEvent.type)
            {
                case JS_EVENT_BUTTON:
                {
                    event->type = BUTTON;
                    event->button = linuxEvent.number;
                    event->value = linuxEvent.value;
                    break;
                }

                case JS_EVENT_AXIS:
                {
                    event->type = AXIS;
                    event->axis = linuxEvent.number;
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
            
            close((int)joystick->data);
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
    if (gamepadConnect(&joystick))
    {
        ESP_LOGI("GamepadExt", "Connected to joystick with %d axes and %d buttons\n", joystick.numAxes, joystick.numButtons);
        return true;
    }

    return false;
}

void gamepadPreFrameCb(uint64_t frame)
{
    emuJoystickEvent_t event;
    while (gamepadReadEvent(&joystick, &event))
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
                        double x = (double)joystick.axisData[TOUCH_AXIS_X];
                        double y = (double)joystick.axisData[TOUCH_AXIS_Y];

                        if (x == 0 && y == 0)
                        {
                            emulatorSetTouchJoystick(0, 0, 0);
                        }
                        else
                        {
                            const double maxRadius = 1024;
                            
                            double angle = atan2(-y, x);
                            double angleDegrees = (angle * 180) / M_PI;
                            int radius = CLAMP(sqrt(x * x + y * y) * maxRadius / 32768, 0, maxRadius);

                            int angleDegreesRounded = (int)(angleDegrees + 360) % 360;
                            emulatorSetTouchJoystick(CLAMP(angleDegreesRounded, 0, 359), radius, 1024);
                        }
                        break;
                    }

                    case ACCEL_AXIS_X:
                    case ACCEL_AXIS_Y:
                    case ACCEL_AXIS_Z:
                    {
                        int16_t accelX = joystick.axisData[ACCEL_AXIS_X] / 128;
                        int16_t accelY = joystick.axisData[ACCEL_AXIS_Y] / 128;
                        int16_t accelZ = joystick.axisData[ACCEL_AXIS_Z] / 128;

                        emulatorSetAccelerometer(accelX, accelY, accelZ);
                        break;
                    }

                    case DPAD_AXIS_X:
                    case DPAD_AXIS_Y:
                    {
                        uint8_t val = 0;
                        buttonBit_t curState = 0;
                        if (joystick.axisData[DPAD_AXIS_X] < 0)
                        {
                            curState |= PB_LEFT;
                        }
                        else if (joystick.axisData[DPAD_AXIS_X] > 0)
                        {
                            curState |= PB_RIGHT;
                        }

                        if (joystick.axisData[DPAD_AXIS_Y] < 0)
                        {
                            curState |= PB_UP;
                        }
                        else if (joystick.axisData[DPAD_AXIS_Y] > 0)
                        {
                            curState |= PB_DOWN;
                        }

                        buttonBit_t lastState = emulatorGetButtonState();
                        buttonBit_t changes = (lastState ^ curState) & ((1 << 8) - 1);

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
