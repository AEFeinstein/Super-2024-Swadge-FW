//==============================================================================
// Imports
//==============================================================================

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

#include "ext_fuzzer.h"
#include "emu_args.h"
#include "hdw-btn_emu.h"
#include "hdw-imu_emu.h"
#include "esp_timer_emu.h"

//==============================================================================
// Function Prototypes
//==============================================================================

static bool fuzzerInitCb(emuArgs_t* emuArgs);
static void fuzzerPreFrameCb(uint64_t frame);

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    bool buttons;
    bool touch;
    bool motion;
    bool time;

    buttonBit_t buttonMask;
} fuzzer_t;

//==============================================================================
// Variables
//==============================================================================

emuExtension_t fuzzerEmuExtension = {
    .name            = "fuzzer",
    .fnInitCb        = fuzzerInitCb,
    .fnPreFrameCb    = fuzzerPreFrameCb,
    .fnPostFrameCb   = NULL,
    .fnKeyCb         = NULL,
    .fnMouseMoveCb   = NULL,
    .fnMouseButtonCb = NULL,
    .fnRenderCb      = NULL,
};

static fuzzer_t fuzzer = {0};

//==============================================================================
// Functions
//==============================================================================

static bool fuzzerInitCb(emuArgs_t* emuArgs)
{
    // Save the options in our own struct for convenience
    fuzzer.buttons = emuArgs->fuzzButtons;
    fuzzer.touch   = emuArgs->fuzzTouch;
    fuzzer.motion  = emuArgs->fuzzMotion;
    fuzzer.time    = emuArgs->fuzzTime;

    fuzzer.buttonMask = (PB_A | PB_B | PB_UP | PB_DOWN | PB_LEFT | PB_RIGHT | PB_START);

    if (emuArgs->fuzz)
    {
        printf("\nFuzzing:\n - [%c] Buttons\n - [%c] Touch\n - [%c] Motion\n - [%c] Time\n", fuzzer.buttons ? 'X' : ' ',
               fuzzer.touch ? 'X' : ' ', fuzzer.motion ? 'X' : ' ', fuzzer.time ? 'X' : ' ');

        if (fuzzer.time)
        {
            emuSetUseRealTime(false);
        }
    }

    return emuArgs->fuzz;
}

static void fuzzerPreFrameCb(uint64_t frame)
{
    if (fuzzer.buttons)
    {
        buttonBit_t buttonState = emulatorGetButtonState();

        // Pick a random button
        uint8_t i              = rand() % 8;
        buttonBit_t fuzzButton = (1 << i);

        if (fuzzButton & fuzzer.buttonMask)
        {
            // Inject the button event to flip that button's state
            emulatorInjectButton(fuzzButton, (buttonState & fuzzButton) == 0);
        }
    }

    if (fuzzer.touch)
    {
        if (0 == (rand() % 2))
        {
            // Set a random angle, radius (up to 1024, not 1023), and intensity value 50% of the time
            emulatorSetTouchJoystick(rand() % 360, rand() % 1025, rand() % (1 << 18));
        }
        else
        {
            // Set no touch 50% of the time
            emulatorSetTouchJoystick(0, 0, 0);
        }
    }

    if (fuzzer.motion)
    {
        // Get the accelerometer range, once
        static int16_t accelMin = 0, accelMax = 0;
        if (accelMin == 0 && accelMax == 0)
        {
            emulatorGetAccelerometerRange(&accelMin, &accelMax);
        }

        // Set the accelerometer to 3 random readings
        emulatorSetAccelerometer((rand() % (1 + accelMax - accelMin)) + accelMin,
                                 (rand() % (1 + accelMax - accelMin)) + accelMin,
                                 (rand() % (1 + accelMax - accelMin)) + accelMin);
    }

    if (fuzzer.time)
    {
        static uint64_t fakeTime = 0;

        // 100FPS is  10000us per frame
        //  60FPS is  16666us per frame
        //  25FPS is  40000us per frame
        //   1FPS = 1000000us per frame
        fakeTime += (rand() % 128) * 1000 << (rand() % (4));
        emuSetEspTimerTime(fakeTime);
    }
}

void emuSetFuzzButtonsEnabled(bool enabled)
{
    fuzzer.buttons = enabled;
}

bool emuGetFuzzButtonsEnabled(void)
{
    return fuzzer.buttons;
}

void emuSetFuzzTouchEnabled(bool enabled)
{
    fuzzer.touch = enabled;
}

bool emuGetFuzzTouchEnabled(void)
{
    return fuzzer.touch;
}

void emuSetFuzzMotionEnabled(bool enabled)
{
    fuzzer.motion = enabled;
}

bool emuGetFuzzMotionEnabled(void)
{
    return fuzzer.motion;
}

void emuSetFuzzTimeEnabled(bool enabled)
{
    fuzzer.time = enabled;
}

bool emuGetFuzzTimeEnabled(void)
{
    return fuzzer.time;
}

void emuSetFuzzButtonsMask(buttonBit_t mask)
{
    fuzzer.buttonMask = mask;
}

buttonBit_t emuGetFuzzButtonsMask(void)
{
    return fuzzer.buttonMask;
}