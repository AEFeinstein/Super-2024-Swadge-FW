//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include <string.h>

#include "swadge2024.h"
#include "mode_ch32v003test.h"
#include "mainMenu.h"

//==============================================================================
// Functions Prototypes
//==============================================================================

void ch32v003testEnterMode(void);
void ch32v003testExitMode(void);
void ch32v003testMainLoop(int64_t elapsedUs);

//==============================================================================
// Variables
//==============================================================================

typedef struct
{
    font_t* font;
    int64_t tElapsedUs;
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

    // Load a font
    font_t* ch32v003testFont = (font_t*)heap_caps_calloc(1, sizeof(font_t), MALLOC_CAP_8BIT);
    loadFont(SONIC_FONT, ch32v003testFont, false);

    // Load some fonts
    ch32v003test->font = ch32v003testFont;

    ch32v003test->tElapsedUs = 0;

    // Turn off LEDs
    led_t leds[CONFIG_NUM_LEDS] = {0};
    setLeds(leds, CONFIG_NUM_LEDS);
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
		        }
			}
		}
    }

    ch32v003test->tElapsedUs += elapsedUs;

    // Clear first
    clearPxTft();

	size_t sz;
    const uint8_t* buf = cnfsGetFile( LEDARRAY_CFUN_BIN, &sz );

	char buffer[128];
	sprintf( buffer, "%ld %p", sz, buf );
    drawText(ch32v003test->font, 215, buffer, 0, 50);
}

