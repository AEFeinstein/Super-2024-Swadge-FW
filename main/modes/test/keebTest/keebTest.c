/**
 * @file keebTest.c
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief A mode designed to test keyboard variations in rapid succession 
 * @version 1.0
 * @date 2024-07-06
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include "keebTest.h"
#include "textEntry.h"

//==============================================================================
// Function Prototypes
//==============================================================================
static void keebEnterMode(void);
static void keebExitMode(void);
static void keebMainLoop(int64_t elapsedUs);

// Variables
//==============================================================================

const char keebTestName[] = "Keyboard Test";

swadgeMode_t keebTestMode = {
    .modeName                 = keebTestName,
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = false,
    .usesAccelerometer        = false,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = keebEnterMode,
    .fnExitMode               = keebExitMode,
    .fnMainLoop               = keebMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = NULL,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL,
};

keebTest_t* kbTest;

// Function definitions
//==============================================================================

static void keebEnterMode(void)
{
    kbTest = (keebTest_t*)calloc(1, sizeof(keebTest_t));
    loadWsg("kid0.wsg", &kbTest->bg, false);
    loadFont("ibm_vga8.font", &kbTest->fnt1, false);
    loadFont("radiostars.font", &kbTest->fnt2, false);
    kbTest->displayText = true;
}

static void keebExitMode(void)
{
    freeFont(&kbTest->fnt1);
    freeFont(&kbTest->fnt2);
    freeWsg(&kbTest->bg);
    free(kbTest);
}

static void keebMainLoop(int64_t elapsedUs)
{
    if (kbTest->displayText)
    {
        int16_t width = textWidth(&kbTest->fnt1, kbTest->typedText);
        fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c111);
        int16_t xOff = (TFT_WIDTH - width) / 2;
        int16_t yOff = (TFT_HEIGHT - kbTest->fnt1.height) / 2;
        drawTextWordWrap(&kbTest->fnt1, c555, kbTest->typedText, &xOff, &yOff, 180, 60);
        drawText(&kbTest->fnt1, c555, kbTest->typedText, xOff, yOff);
        buttonEvt_t evt = {0};
        while (checkButtonQueueWrapper(&evt))
        {
            if (evt.down)
            {
                kbTest->displayText = false;
                textEntryInit(&kbTest->fnt1, MAX_TEXT_LEN + 1, kbTest->typedText);
                textEntrySetTextColor(c555, false);
                textEntrySetEmphasisColor(c500);
                textEntrySetShadowboxColor(true, c111);
                textEntrySetMultiline(true);
            }
        }
    }
    else
    {
        bool done       = false;
        buttonEvt_t evt = {0};
        while (checkButtonQueueWrapper(&evt))
        {
            done = !textEntryInput(evt.down, evt.button);
        }
        if (done)
        {
            kbTest->displayText = true;
        }
        textEntryDraw(elapsedUs);
    }
}