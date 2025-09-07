//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include <string.h>

#include "swadge2024.h"
#include "mode_ch32v003test.h"
#include "mainMenu.h"
#include "hdw-ch32v003.h"

#define MAX_MODES 11

//==============================================================================
// Functions Prototypes
//==============================================================================

void ch32v003testEnterMode(void);
void ch32v003testExitMode(void);
void ch32v003testMainLoop(int64_t elapsedUs);
void ch32v003testUpdateFramebuffers();
int uprintf(const char* fmt, ...);

//==============================================================================
// Variables
//==============================================================================

typedef struct
{
    font_t* font;
    int64_t tElapsedUs;
    int mode;
    int didSetFramebuffers;
} ch32v003test_t;

ch32v003test_t* ch32v003test;

const char ch32v003testName[] = "Ch32v003test";

swadgeMode_t modeCh32v003test = {
    .modeName                 = ch32v003testName,
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = false,
    .usesAccelerometer        = false,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = ch32v003testEnterMode,
    .fnExitMode               = ch32v003testExitMode,
    .fnMainLoop               = ch32v003testMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = NULL,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL,
};

static int initial_success;

//==============================================================================
// Functions
//==============================================================================

/**
 * Enter the ch32v003test mode, allocate and initialize memory
 */
void ch32v003testEnterMode(void)
{
    // Allocate memory for this mode
    ch32v003test = (ch32v003test_t*)heap_caps_calloc(1, sizeof(ch32v003test_t), MALLOC_CAP_8BIT);

    // First thing we want to do is wake the ch32v003 up, then we can make it do funny things later.
    // It is normally safe to assume it was already initialized.
    initial_success = ch32v003RunBinaryAsset(MATRIX_GRADIENT_CFUN_BIN);

    // If you do load custom firmware onto the 003, you must wait for it to run before yanking the framebuffer around.
    // vTaskDelay(1);

    // Load a font
    font_t* ch32v003testFont = (font_t*)heap_caps_calloc(1, sizeof(font_t), MALLOC_CAP_8BIT);
    loadFont(SONIC_FONT, ch32v003testFont, false);

    // Load some fonts
    ch32v003test->font = ch32v003testFont;

    ch32v003test->tElapsedUs         = 0;
    ch32v003test->didSetFramebuffers = 0;
    ch32v003test->mode               = 0;

    // Turn off LEDs
    led_t leds[CONFIG_NUM_LEDS] = {0};
    setLeds(leds, CONFIG_NUM_LEDS);
}

/**
 * Update framebuffers on the ch32v003
 */
void ch32v003testUpdateFramebuffers()
{
    if (!ch32v003test->didSetFramebuffers)
    {
        // Load up framebuffer on ch32v003.
        uint8_t sendbuffer[6][12];
        int i;

        for (i = 0; i < 72; i++)
            sendbuffer[i / 12][i % 12] = (i & 1) ? 0xff : 0x00;
        ch32v003WriteBitmap(0, sendbuffer);

        for (i = 0; i < 72; i++)
            sendbuffer[i / 12][i % 12] = i;
        ch32v003WriteBitmap(1, sendbuffer);

        for (i = 0; i < 72; i++)
            sendbuffer[i / 12][i % 12] = (i % 12) * 23;
        ch32v003WriteBitmap(2, sendbuffer);

        ch32v003WriteBitmapAsset( 3, EYES_DEAD_GS);
        ch32v003WriteBitmapAsset( 4, EYES_DEFAULT_GS);
        ch32v003WriteBitmapAsset( 5, EYES_HAPPY_GS);
        ch32v003WriteBitmapAsset( 6, EYES_SAD_GS);
        ch32v003WriteBitmapAsset( 7, EYES_SWIRL_0_GS);
        ch32v003WriteBitmapAsset( 8, EYES_SWIRL_1_GS);
        ch32v003WriteBitmapAsset( 9, EYES_SWIRL_2_GS);
        ch32v003WriteBitmapAsset(10, EYES_SWIRL_3_GS);

        ch32v003test->didSetFramebuffers = true;
    }
}

/**
 * Exit the ch32v003test mode, free memory
 */
void ch32v003testExitMode(void)
{
    // Free the font
    freeFont(ch32v003test->font);
    heap_caps_free(ch32v003test->font);
    // Free memory for this mode
    heap_caps_free(ch32v003test);
}

/**
 * Main ch32v003test loop, draw some scrolling ch32v003test
 *
 * @param elapsedUs The time elapsed since the last call
 */
void ch32v003testMainLoop(int64_t elapsedUs)
{
    // Clear first
    clearPxTft();

    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        if (evt.down)
        {
            switch (evt.button)
            {
                case PB_A:
                case PB_B:
                {
                    // Exit
                    switchToSwadgeMode(&mainMenuMode);
                    break;
                }
                case PB_UP:
                case PB_LEFT:
                {
                    ch32v003testUpdateFramebuffers();

                    ch32v003test->mode--;
                    if (ch32v003test->mode < 0)
                        ch32v003test->mode = MAX_MODES - 1;

                    ch32v003SelectBitmap(ch32v003test->mode);
                    break;
                }
                case PB_DOWN:
                case PB_RIGHT:
                {
                    ch32v003testUpdateFramebuffers();

                    ch32v003test->mode++;
                    if (ch32v003test->mode >= MAX_MODES)
                        ch32v003test->mode = 0;

                    ch32v003SelectBitmap(ch32v003test->mode);
                    break;
                }
                case PB_START:
                case PB_SELECT:
                    break;
            }
        }
    }

    ch32v003test->tElapsedUs += elapsedUs;

    char buffer[64];

    uint32_t dmdata0 = 0, dmdata1 = 0;
    ch32v003GetReg(4, &dmdata0);
    ch32v003GetReg(5, &dmdata1);
    sprintf(buffer, "%08x", (unsigned)dmdata0);
    drawText(ch32v003test->font, 215, buffer, 2, 50);
    sprintf(buffer, "%08x", (unsigned)dmdata1);
    drawText(ch32v003test->font, 215, buffer, 2, 70);
    sprintf(buffer, "%s", initial_success ? "FAIL" : "OK");
    drawText(ch32v003test->font, 215, buffer, 2, 90);
    sprintf(buffer, "%d", ch32v003test->mode);
    drawText(ch32v003test->font, 215, buffer, 2, 110);

    ch32v003CheckTerminal();
}
