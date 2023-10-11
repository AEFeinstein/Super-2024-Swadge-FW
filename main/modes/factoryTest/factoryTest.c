//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>

#include <esp_log.h>
#include <esp_timer.h>

#include "embeddedOut.h"
#include "hdw-bzr.h"
#include "hdw-led.h"
#include "settingsManager.h"
#include "touchTest.h"

#include "factoryTest.h"
#include "mainMenu.h"

//==============================================================================
// Defines
//==============================================================================

#define BTN_RADIUS 10
#define BTN_DIA    (BTN_RADIUS * 2)
#define AB_SEP     6
#define DPAD_SEP   15

#define ACCEL_BAR_HEIGHT 8
#define ACCEL_BAR_SEP    1
#define MAX_ACCEL_BAR_W  60

#define TOUCHBAR_WIDTH  60
#define TOUCHBAR_HEIGHT 60
#define TOUCHBAR_Y_OFF  32

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    BTN_NOT_PRESSED,
    BTN_PRESSED,
    BTN_RELEASED
} testButtonState_t;

typedef enum
{
    R_RISING,
    R_FALLING,
    G_RISING,
    G_FALLING,
    B_RISING,
    B_FALLING
} testLedState_t;

//==============================================================================
// Functions Prototypes
//==============================================================================

void testEnterMode(void);
void testExitMode(void);
void testMainLoop(int64_t elapsedUs);
void testAudioCb(uint16_t* samples, uint32_t sampleCnt);
void testProcessButtons(buttonEvt_t* evt);
uint8_t touchJoystickToTouchIdx(touchJoystick_t touchJoystick);
touchJoystick_t touchIdxToTouchJoystick(uint8_t touchIdx);
void testReadAndValidateTouch(void);
void testReadAndValidateAccelerometer(void);

void plotButtonState(int16_t x, int16_t y, testButtonState_t state);
static void touchFillCircleSegments(int16_t x, int16_t y, int16_t r, int16_t segs, bool center);

//==============================================================================
// Variables
//==============================================================================

typedef struct
{
    font_t ibm_vga8;
    wsg_t kd_idle0;
    wsg_t kd_idle1;
    uint64_t tSpriteElapsedUs;
    uint8_t spriteFrame;
    // Microphone test
    dft32_data dd;
    embeddedNf_data end;
    embeddedOut_data eod;
    uint8_t samplesProcessed;
    uint16_t maxValue;
    // Buzzers
    song_t song;
    // Button
    testButtonState_t buttonStates[8];
    // Touch, as an 8-way joystick with center deadzone
    testButtonState_t touchStates[9];
    uint8_t lastTouchStateIdx;
    touchJoystick_t touchJoystick;
    bool touched;
    // Accelerometer
    int16_t x;
    int16_t y;
    int16_t z;
    // LED
    led_t cLed;
    testLedState_t ledState;
    uint64_t tLedElapsedUs;
    // Test results
    bool buttonsPassed;
    bool touchPassed;
    bool accelPassed;
    bool bzrMicPassed;
} factoryTest_t;

factoryTest_t* test;

swadgeMode_t factoryTestMode = {
    .modeName                 = "Factory Test",
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = false,
    .usesAccelerometer        = true,
    .usesThermometer          = true,
    .overrideSelectBtn        = true,
    .fnEnterMode              = testEnterMode,
    .fnExitMode               = testExitMode,
    .fnMainLoop               = testMainLoop,
    .fnAudioCallback          = testAudioCb,
    .fnBackgroundDrawCallback = NULL,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL,
};

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Enter test mode, allocate memory, initialize code
 */
void testEnterMode(void)
{
    // Allocate memory for this mode
    test = (factoryTest_t*)calloc(1, sizeof(factoryTest_t));

    // Load a font
    loadFont("ibm_vga8.font", &test->ibm_vga8, false);

    // Load a sprite
    loadWsg("kid0.wsg", &test->kd_idle0, false);
    loadWsg("kid1.wsg", &test->kd_idle1, false);

    // Init last touchState index to indicate no previous state
    test->lastTouchStateIdx = UINT8_MAX;

    // Init CC
    InitColorChord(&test->end, &test->dd);
    test->maxValue = 1;

    // Temporarily set the buzzer to full volume
    bzrSetBgmVolume(MAX_VOLUME);
    bzrSetSfxVolume(MAX_VOLUME);

    // Set the mic to listen
    setMicGainSetting(MAX_MIC_GAIN);

    // Play a song
    loadSong("stereo_test.sng", &test->song, false);
    bzrPlayBgm(&test->song, BZR_STEREO);
}

/**
 * @brief Exit test mode, free memory
 */
void testExitMode(void)
{
    freeFont(&test->ibm_vga8);
    freeWsg(&test->kd_idle0);
    freeWsg(&test->kd_idle1);
    freeSong(&test->song);
    free(test);
}

/**
 * @brief This is the main loop, and it draws to the TFT
 *
 * @param elapsedUs unused
 */
void testMainLoop(int64_t elapsedUs __attribute__((unused)))
{
    // Process button events
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        testProcessButtons(&evt);
    }

    testReadAndValidateTouch();
    testReadAndValidateAccelerometer();

    // Check for a test pass
    if (test->buttonsPassed && test->touchPassed && test->accelPassed && test->bzrMicPassed)
    {
        // Set NVM to indicate the test passed
        setTestModePassedSetting(true);

        if (getTestModePassedSetting())
        {
            switchToSwadgeMode(&mainMenuMode);
        }
    }

    // Clear everything
    clearPxTft();

    // Draw the spectrum as a bar graph. Figure out bar and margin size
    int16_t binWidth  = (TFT_WIDTH / FIX_BINS);
    int16_t binMargin = (TFT_WIDTH - (binWidth * FIX_BINS)) / 2;

    // Find the max value
    for (uint16_t i = 0; i < FIX_BINS; i++)
    {
        if (test->end.fuzzed_bins[i] > test->maxValue)
        {
            test->maxValue = test->end.fuzzed_bins[i];
        }
    }

    // Plot the bars
    int32_t energy = 0;
    for (uint16_t i = 0; i < FIX_BINS; i++)
    {
        energy += test->end.fuzzed_bins[i];
        uint8_t height       = ((TFT_HEIGHT / 2) * test->end.fuzzed_bins[i]) / test->maxValue;
        paletteColor_t color = test->bzrMicPassed ? c050 : c500; // paletteHsvToHex((i * 256) / FIX_BINS, 255, 255);
        int16_t x0           = binMargin + (i * binWidth);
        int16_t x1           = binMargin + ((i + 1) * binWidth);
        // Big enough, fill an area
        fillDisplayArea(x0, TFT_HEIGHT - height, x1, TFT_HEIGHT, color);
    }

    // Check for a pass
    if (energy > 100000)
    {
        test->bzrMicPassed = true;
    }

    // Draw button states
    int16_t centerLine = TFT_HEIGHT / 4;
    int16_t btnX       = BTN_DIA;

    // PB_LEFT
    plotButtonState(btnX, centerLine, test->buttonStates[2]);
    btnX += DPAD_SEP;
    // PB_UP
    plotButtonState(btnX, centerLine - DPAD_SEP, test->buttonStates[0]);
    // PB_DOWN
    plotButtonState(btnX, centerLine + DPAD_SEP, test->buttonStates[1]);
    btnX += DPAD_SEP;
    // PB_RIGHT
    plotButtonState(btnX, centerLine, test->buttonStates[3]);
    btnX += BTN_DIA + 4;

    // PB_SELECT
    plotButtonState(btnX, centerLine, test->buttonStates[7]);
    btnX += BTN_DIA + 2;
    // PB_START
    plotButtonState(btnX, centerLine, test->buttonStates[6]);
    btnX += BTN_DIA + 4;

    // PB_B
    plotButtonState(btnX, centerLine + AB_SEP, test->buttonStates[5]);
    btnX += BTN_DIA + 2;
    // PB_A
    plotButtonState(btnX, centerLine - AB_SEP, test->buttonStates[4]);

    // Set up drawing accel bars
    int16_t barY = TOUCHBAR_Y_OFF;

    paletteColor_t accelColor = c500;
    if (test->accelPassed)
    {
        accelColor = c050;
    }

    // Plot X accel
    int16_t barWidth = ((test->x + 256) * MAX_ACCEL_BAR_W) / 512;
    if (barWidth >= 0)
    {
        fillDisplayArea(TFT_WIDTH - barWidth, barY, TFT_WIDTH, barY + ACCEL_BAR_HEIGHT, accelColor);
    }
    barY += (ACCEL_BAR_HEIGHT + ACCEL_BAR_SEP);

    // Plot Y accel
    barWidth = ((test->y + 256) * MAX_ACCEL_BAR_W) / 512;
    if (barWidth >= 0)
    {
        fillDisplayArea(TFT_WIDTH - barWidth, barY, TFT_WIDTH, barY + ACCEL_BAR_HEIGHT, accelColor);
    }
    barY += (ACCEL_BAR_HEIGHT + ACCEL_BAR_SEP);

    // Plot Z accel
    barWidth = ((test->z + 256) * MAX_ACCEL_BAR_W) / 512;
    if (barWidth >= 0)
    {
        fillDisplayArea(TFT_WIDTH - barWidth, barY, TFT_WIDTH, barY + ACCEL_BAR_HEIGHT, accelColor);
    }
    barY += (ACCEL_BAR_HEIGHT + ACCEL_BAR_SEP);

    // Draw the 8-direction touchpad with center circle
    int16_t tBarX        = TFT_WIDTH - TOUCHBAR_WIDTH;
    int16_t tBarY        = barY + 4 + TOUCHBAR_HEIGHT / 2 + test->ibm_vga8.height + 15;
    touchDrawCircle(&test->ibm_vga8, "Touch Pad", tBarX, tBarY, 35, 8, true,
                    test->touched, test->touchJoystick);
    touchFillCircleSegments(tBarX, tBarY, 35, 8, true);

    // Plot some text depending on test status
    char dbgStr[32] = {0};
    if (false == test->buttonsPassed)
    {
        sprintf(dbgStr, "Test Buttons");
    }
    else if (false == test->touchPassed)
    {
        sprintf(dbgStr, "Test Touch Pad");
    }
    else if (false == test->accelPassed)
    {
        sprintf(dbgStr, "Test Accelerometer");
    }
    else if (false == test->bzrMicPassed)
    {
        sprintf(dbgStr, "Test Buzzer & Mic");
    }
    else if (getTestModePassedSetting())
    {
        sprintf(dbgStr, "All Tests Passed");
    }

    int16_t tWidth = textWidth(&test->ibm_vga8, dbgStr);
    drawText(&test->ibm_vga8, c555, dbgStr, (TFT_WIDTH - tWidth) / 2, 0);

    sprintf(dbgStr, "Verify RGB LEDs & Tunes");
    tWidth = textWidth(&test->ibm_vga8, dbgStr);
    drawText(&test->ibm_vga8, c555, dbgStr, 70, test->ibm_vga8.height + 8);

    // Animate a sprite
    test->tSpriteElapsedUs += elapsedUs;
    while (test->tSpriteElapsedUs >= 500000)
    {
        test->tSpriteElapsedUs -= 500000;
        test->spriteFrame = (test->spriteFrame + 1) % 2;
    }

    // Draw the sprite
    if (0 == test->spriteFrame)
    {
        drawWsg(&test->kd_idle0, 32, 4, false, false, 0);
    }
    else
    {
        drawWsg(&test->kd_idle1, 32, 4, false, false, 0);
    }

    // Pulse LEDs, each color for 1s
    test->tLedElapsedUs += elapsedUs;
    while (test->tLedElapsedUs >= (1000000 / 512))
    {
        test->tLedElapsedUs -= (1000000 / 512);
        switch (test->ledState)
        {
            case R_RISING:
            {
                test->cLed.r++;
                if (255 == test->cLed.r)
                {
                    test->ledState = R_FALLING;
                }
                break;
            }
            case R_FALLING:
            {
                test->cLed.r--;
                if (0 == test->cLed.r)
                {
                    test->ledState = G_RISING;
                }
                break;
            }
            case G_RISING:
            {
                test->cLed.g++;
                if (255 == test->cLed.g)
                {
                    test->ledState = G_FALLING;
                }
                break;
            }
            case G_FALLING:
            {
                test->cLed.g--;
                if (0 == test->cLed.g)
                {
                    test->ledState = B_RISING;
                }
                break;
            }
            case B_RISING:
            {
                test->cLed.b++;
                if (255 == test->cLed.b)
                {
                    test->ledState = B_FALLING;
                }
                break;
            }
            case B_FALLING:
            {
                test->cLed.b--;
                if (0 == test->cLed.b)
                {
                    test->ledState = R_RISING;
                }
                break;
            }
        }
    }

    // Display LEDs
    led_t leds[CONFIG_NUM_LEDS] = {0};
    for (uint8_t i = 0; i < CONFIG_NUM_LEDS; i++)
    {
        leds[i].r = test->cLed.r;
        leds[i].g = test->cLed.g;
        leds[i].b = test->cLed.b;
    }
    setLeds(leds, CONFIG_NUM_LEDS);
}

/**
 * @brief Helper function to draw button statuses to the TFT
 *
 * @param x
 * @param y
 * @param state
 */
void plotButtonState(int16_t x, int16_t y, testButtonState_t state)
{
    switch (state)
    {
        case BTN_NOT_PRESSED:
        {
            drawCircle(x, y, BTN_RADIUS, c500);
            break;
        }
        case BTN_PRESSED:
        {
            drawCircleFilled(x, y, BTN_RADIUS, c005);
            break;
        }
        case BTN_RELEASED:
        {
            drawCircleFilled(x, y, BTN_RADIUS, c050);
            break;
        }
    }
}

/**
 * @brief Button callback, used to change settings
 *
 * @param evt The button event
 */
void testProcessButtons(buttonEvt_t* evt)
{
    // Find the button index based on bit
    uint8_t btnIdx = 0;
    switch (evt->button)
    {
        case PB_UP:
        {
            btnIdx = 0;
            break;
        }
        case PB_DOWN:
        {
            btnIdx = 1;
            break;
        }
        case PB_LEFT:
        {
            btnIdx = 2;
            break;
        }
        case PB_RIGHT:
        {
            btnIdx = 3;
            break;
        }
        case PB_A:
        {
            btnIdx = 4;
            break;
        }
        case PB_B:
        {
            btnIdx = 5;
            break;
        }
        case PB_START:
        {
            btnIdx = 6;
            break;
        }
        case PB_SELECT:
        {
            btnIdx = 7;
            break;
        }
        default:
        {
            return;
        }
    }

    // Transition states
    if (evt->down)
    {
        if (BTN_NOT_PRESSED == test->buttonStates[btnIdx])
        {
            test->buttonStates[btnIdx] = BTN_PRESSED;
        }
    }
    else
    {
        if (BTN_PRESSED == test->buttonStates[btnIdx])
        {
            test->buttonStates[btnIdx] = BTN_RELEASED;
        }
    }

    // Check if all buttons have passed
    test->buttonsPassed = true;
    for (uint8_t i = 0; i < 8; i++)
    {
        if (BTN_RELEASED != test->buttonStates[i])
        {
            test->buttonsPassed = false;
        }
    }
}

uint8_t touchJoystickToTouchIdx(touchJoystick_t touchJoystick)
{
    switch (touchJoystick)
    {
        case TB_CENTER:
            return 0;
        case TB_RIGHT:
            return 1;
        case TB_UP:
            return 2;
        case TB_LEFT:
            return 3;
        case TB_DOWN:
            return 4;
        case TB_UP_RIGHT:
            return 5;
        case TB_UP_LEFT:
            return 6;
        case TB_DOWN_LEFT:
            return 7;
        case TB_DOWN_RIGHT:
            return 8;
        default:
            return UINT8_MAX;
    }
}

touchJoystick_t touchIdxToTouchJoystick(uint8_t touchIdx)
{
    switch (touchIdx)
    {
        case 0:
            return TB_CENTER;
        case 1:
            return TB_RIGHT;
        case 2:
            return TB_UP;
        case 3:
            return TB_LEFT;
        case 4:
            return TB_DOWN;
        case 5:
            return TB_UP_RIGHT;
        case 6:
            return TB_UP_LEFT;
        case 7:
            return TB_DOWN_LEFT;
        case 8:
            return TB_DOWN_RIGHT;
        default:
            return UINT8_MAX;
    }
}

/**
 * @brief Read, save, and validate touch
 *
 */
void testReadAndValidateTouch(void)
{
    int32_t phi, r, intensity;
    test->touched = getTouchJoystick(&phi, &r, &intensity);

    // Transition states
    if (test->touched)
    {
        test->touchJoystick = getTouchJoystickZones(phi, r, true, true);
        uint8_t pad = touchJoystickToTouchIdx(test->touchJoystick);
        if(pad == UINT8_MAX)
        {
            return;
        }

        if (BTN_NOT_PRESSED == test->touchStates[pad])
        {
            test->touchStates[pad] = BTN_PRESSED;
        }

        if (test->lastTouchStateIdx != UINT8_MAX && pad != test->lastTouchStateIdx)
        {
            if (BTN_PRESSED == test->touchStates[test->lastTouchStateIdx])
            {
                test->touchStates[test->lastTouchStateIdx] = BTN_RELEASED;
            }
        }

        test->lastTouchStateIdx = pad;
    }
    else if (test->lastTouchStateIdx != UINT8_MAX)
    {
        if (BTN_PRESSED == test->touchStates[test->lastTouchStateIdx])
        {
            test->touchStates[test->lastTouchStateIdx] = BTN_RELEASED;
        }
    }

    // Check if all buttons have passed
    test->touchPassed = true;
    for (uint8_t i = 0; i < 9; i++)
    {
        if (BTN_RELEASED != test->touchStates[i])
        {
            test->touchPassed = false;
        }
    }
}

/**
 * @brief Audio callback. Take the samples and pass them to test
 *
 * @param samples The samples to process
 * @param sampleCnt The number of samples to process
 */
void testAudioCb(uint16_t* samples, uint32_t sampleCnt)
{
    // For each sample
    for (uint32_t idx = 0; idx < sampleCnt; idx++)
    {
        // Push to test
        PushSample32(&test->dd, samples[idx]);

        // If 128 samples have been pushed
        test->samplesProcessed++;
        if (test->samplesProcessed >= 128)
        {
            // Update LEDs
            test->samplesProcessed = 0;
            HandleFrameInfo(&test->end, &test->dd);

            // Keep track of max value for the spectrogram
            int16_t maxVal = 0;
            for (uint16_t i = 0; i < FIX_BINS; i++)
            {
                if (test->end.fuzzed_bins[i] > maxVal)
                {
                    maxVal = test->end.fuzzed_bins[i];
                }
            }

            // If already passed, just return
            if (true == test->bzrMicPassed)
            {
                return;
            }
        }
    }
}

/**
 * @brief Read, save, and validate accelerometer data
 *
 */
void testReadAndValidateAccelerometer(void)
{
    // Read accel
    int16_t x, y, z;
    accelGetAccelVec(&x, &y, &z);

    // Save accel values
    test->x = x;
    test->y = y;
    test->z = z;

    // Make sure all values are nonzero
    if ((x != 0) && (y != 0) && (z != 0))
    {
        // Make sure some value is shook
        if ((x > 500) || (y > 500) || (z > 500))
        {
            // Pass!
            test->accelPassed = true;
        }
    }
}

static void touchFillCircleSegments(int16_t x, int16_t y, int16_t r, int16_t segs, bool center)
{
    uint8_t numTouchElem = (sizeof(test->touchStates) / sizeof(test->touchStates[0]));
    for (uint8_t touchIdx = 0; touchIdx < numTouchElem; touchIdx++)
    {
        paletteColor_t color;
        switch (test->touchStates[touchIdx])
        {
            case BTN_PRESSED:
            {
                color = c005;
                break;
            }
            case BTN_RELEASED:
            {
                color = c050;
                break;
            }
            case BTN_NOT_PRESSED:
            default:
            {
                color = cTransparent;
            }
        }

        int16_t angle = 0;
        int16_t fillR = r / 2;
        switch (touchIdxToTouchJoystick(touchIdx))
        {
            case TB_CENTER:
                angle = 0;
                fillR = 0;
                break;

            case TB_RIGHT:
                angle = 0;
                break;

            case TB_UP | TB_RIGHT:
                angle = 45;
                break;

            case TB_UP:
                angle = 90;
                break;

            case TB_UP | TB_LEFT:
                angle = 135;
                break;

            case TB_LEFT:
                angle = 180;
                break;

            case TB_DOWN | TB_LEFT:
                angle = 225;
                break;

            case TB_DOWN:
                angle = 270;
                break;

            case TB_DOWN | TB_RIGHT:
                angle = 315;
                break;
        }

        if (color != cTransparent)
        {
            // Fill in the segment
            floodFill(x + getCos1024(angle) * fillR / 1024, y - getSin1024(angle) * fillR / 1024, color, x - r - 1,
                    y - r - 1, x + r + 1, y + r + 1);
        }
    }
}
