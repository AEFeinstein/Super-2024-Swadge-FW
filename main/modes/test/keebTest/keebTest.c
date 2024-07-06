// Includes
//==============================================================================

#include "textEntry.h"

// Function Prototypes
//==============================================================================
void keebEnterMode(void);
void keebExitMode(void);
void keebMainLoop(int64_t elapsedUs);

// Variables
//==============================================================================
#define MAX_TEXT_LEN 128

typedef struct
{
    wsg_t bg;
    font_t fnt;
    bool displayText;
    char typedText[MAX_TEXT_LEN];
} keebTest_t;

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

void keebEnterMode(void)
{
    kbTest = (keebTest_t*)calloc(1, sizeof(keebTest_t));
    loadWsg("menu_bg.wsg", &kbTest->bg, false);
    loadFont("ibm_vga8.font", &kbTest->fnt, false);
    // loadFont("radiostars.font", &kbTest->fnt, false);
    kbTest->displayText = true;
    strcpy(kbTest->typedText, "Press any key");
}

void keebExitMode(void)
{
    freeFont(&kbTest->fnt);
    freeWsg(&kbTest->bg);
    free(kbTest);
}

void keebMainLoop(int64_t elapsedUs)
{
    if (kbTest->displayText)
    {
        int16_t width = textWidth(&kbTest->fnt, kbTest->typedText);
        fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c111);
        int16_t xOff = (TFT_WIDTH - width) / 2;
        int16_t yOff = (TFT_HEIGHT - kbTest->fnt.height) / 2;
        drawTextWordWrap(&kbTest->fnt, c555, kbTest->typedText, &xOff, &yOff, 180, 60);
        drawText(&kbTest->fnt, c555, kbTest->typedText, xOff, yOff);
        buttonEvt_t evt = {0};
        while (checkButtonQueueWrapper(&evt))
        {
            if (evt.down)
            {
                kbTest->displayText = false;
                textEntryInit(&kbTest->fnt, MAX_TEXT_LEN + 1, kbTest->typedText);
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