//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include <esp_log.h>
#include <esp_timer.h>

#include "mode_colorchord.h"
#include "mode_colorchord_types.h"

// For colorchord
#include "embeddedOut.h"

//==============================================================================
// Defines
//==============================================================================

#define TEXT_Y      10
#define TEXT_MARGIN 20

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    CC_OPT_GAIN,
    CC_OPT_LED_BRIGHT,
    CC_OPT_LED_MODE,
    COLORCHORD_NUM_OPTS
} ccOpt_t;

//==============================================================================
// Functions Prototypes
//==============================================================================

void colorchordEnterMode(void);
void colorchordExitMode(void);
void colorchordMainLoop(int64_t elapsedUs);
void colorchordAudioCb(uint16_t* samples, uint32_t sampleCnt);
void colorchordButtonCb(buttonEvt_t* evt);

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    font_t ibm_vga8;
    dft32_data dd;
    embeddedNf_data end;
    embeddedOut_data eod;
    uint8_t samplesProcessed;
    uint16_t maxValue;
    ccOpt_t optSel;
    uint16_t* sampleHist;
    uint16_t sampleHistHead;
    uint16_t sampleHistCount;
} colorchord_t;

//==============================================================================
// Variables
//==============================================================================

static const char colorchordName[] = "Colorchord";

colorchord_t* colorchord;

swadgeMode_t colorchordMode = {
    .modeName                 = colorchordName,
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = false,
    .usesAccelerometer        = false,
    .usesThermometer          = false,
    .fnEnterMode              = colorchordEnterMode,
    .fnExitMode               = colorchordExitMode,
    .fnMainLoop               = colorchordMainLoop,
    .fnAudioCallback          = colorchordAudioCb,
    .fnBackgroundDrawCallback = NULL,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL,
};

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Enter colorchord mode, allocate memory, initialize code
 */
void colorchordEnterMode(void)
{
    // Allocate memory for this mode
    colorchord = (colorchord_t*)calloc(1, sizeof(colorchord_t));

    colorchord->sampleHistCount = 512;
    colorchord->sampleHist      = (uint16_t*)calloc(colorchord->sampleHistCount, sizeof(uint16_t));
    colorchord->sampleHistHead  = 0;

    // Load a font
    loadFont("ibm_vga8.font", &colorchord->ibm_vga8, false);

    // Init CC
    InitColorChord(&colorchord->end, &colorchord->dd);
    colorchord->maxValue = 1;
}

/**
 * @brief Exit colorchord mode, free memory
 */
void colorchordExitMode(void)
{
    if (colorchord->sampleHist)
    {
        free(colorchord->sampleHist);
    }
    freeFont(&colorchord->ibm_vga8);
    free(colorchord);
}

/**
 * @brief This is the main loop, and it draws to the TFT
 *
 * @param elapsedUs unused
 */
void colorchordMainLoop(int64_t elapsedUs __attribute__((unused)))
{
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        colorchordButtonCb(&evt);
    }

    // Clear everything
    clearPxTft();

    // Draw the spectrum as a bar graph. Figure out bar and margin size
    int16_t binWidth  = (TFT_WIDTH / FIX_BINS);
    int16_t binMargin = (TFT_WIDTH - (binWidth * FIX_BINS)) / 2;

    // This is the center line to draw the graph around
    uint8_t centerLine
        = (TEXT_Y + colorchord->ibm_vga8.height + 2) + (TFT_HEIGHT - (TEXT_Y + colorchord->ibm_vga8.height + 2)) / 2;

    // Find the max value
    for (uint16_t i = 0; i < FIX_BINS; i++)
    {
        if (colorchord->end.fuzzed_bins[i] > colorchord->maxValue)
        {
            colorchord->maxValue = colorchord->end.fuzzed_bins[i];
        }
    }

    // Plot the bars
    for (uint16_t i = 0; i < FIX_BINS; i++)
    {
        uint8_t height
            = ((TFT_HEIGHT - colorchord->ibm_vga8.height - 2) * colorchord->end.fuzzed_bins[i]) / colorchord->maxValue;

        paletteColor_t color = RGBtoPalette(
            ECCtoHEX(((i << SEMI_BITS_PER_BIN) + colorchord->eod.RootNoteOffset) % NOTE_RANGE, 255, 255));
        int16_t x0 = binMargin + (i * binWidth);
        int16_t x1 = binMargin + ((i + 1) * binWidth);
        if (height < 2)
        {
            // Too small to plot, draw a line
            drawLine(x0, centerLine, x1, centerLine, color, 0);
        }
        else
        {
            // Big enough, fill an area
            fillDisplayArea(x0, centerLine - (height / 2), x1, centerLine + (height / 2), color);
        }
    }

    // Draw sine wave
    {
        SETUP_FOR_TURBO();
        int x;
        uint16_t* sampleHist     = colorchord->sampleHist;
        uint16_t sampleHistCount = colorchord->sampleHistCount;
        int16_t sampleHistMark   = colorchord->sampleHistHead - 1;

        for (x = 0; x < TFT_WIDTH; x++)
        {
            if (sampleHistMark < 0)
            {
                sampleHistMark = sampleHistCount - 1;
            }
            int16_t sample = sampleHist[sampleHistMark];
            sampleHistMark--;
            uint16_t y = ((sample * TFT_HEIGHT) >> 16) + (TFT_HEIGHT / 2);
            if (y >= TFT_HEIGHT)
            {
                continue;
            }
            TURBO_SET_PIXEL(x, y, 215);
        }
    }

    // Draw the HUD text
    char text[16] = {0};

    // Draw gain indicator
    snprintf(text, sizeof(text), "Gain: %" PRId32, getMicGain());
    drawText(&colorchord->ibm_vga8, c555, text, TEXT_MARGIN, TEXT_Y);

    // Underline it if selected
    if (CC_OPT_GAIN == colorchord->optSel)
    {
        int16_t lineY = 10 + colorchord->ibm_vga8.height + 2;
        drawLine(TEXT_MARGIN, lineY, TEXT_MARGIN + textWidth(&colorchord->ibm_vga8, text) - 1, lineY, c555, 0);
    }

    // Draw LED brightness indicator
    snprintf(text, sizeof(text), "LED: %" PRId32, getLedBrightness());
    int16_t tWidth = textWidth(&colorchord->ibm_vga8, text);
    drawText(&colorchord->ibm_vga8, c555, text, (TFT_WIDTH - tWidth) / 2, TEXT_Y);

    // Underline it if selected
    if (CC_OPT_LED_BRIGHT == colorchord->optSel)
    {
        int16_t lineY = TEXT_Y + colorchord->ibm_vga8.height + 2;
        drawLine((TFT_WIDTH - tWidth) / 2, lineY, (TFT_WIDTH - tWidth) / 2 + tWidth - 1, lineY, c555, 0);
    }

    // Draw colorchord mode
    switch (getColorchordMode())
    {
        case LINEAR_LEDS:
        {
            snprintf(text, sizeof(text), "Rainbow");
            break;
        }
        default:
        case NUM_CC_MODES:
        case ALL_SAME_LEDS:
        {
            snprintf(text, sizeof(text), "Solid");
            break;
        }
    }
    int16_t textX = TFT_WIDTH - TEXT_MARGIN - textWidth(&colorchord->ibm_vga8, text);
    drawText(&colorchord->ibm_vga8, c555, text, textX, TEXT_Y);

    // Underline it if selected
    if (CC_OPT_LED_MODE == colorchord->optSel)
    {
        int16_t lineY = TEXT_Y + colorchord->ibm_vga8.height + 2;
        drawLine(textX, lineY, textX + textWidth(&colorchord->ibm_vga8, text) - 1, lineY, c555, 0);
    }

    // Draw reminder text
    const char exitText[] = "Start + Select to Exit";
    int16_t exitWidth     = textWidth(&colorchord->ibm_vga8, exitText);
    drawText(&colorchord->ibm_vga8, c555, exitText, (TFT_WIDTH - exitWidth) / 2,
             TFT_HEIGHT - colorchord->ibm_vga8.height - TEXT_Y);
}

/**
 * @brief Button callback, used to change settings
 *
 * @param evt The button event
 */
void colorchordButtonCb(buttonEvt_t* evt)
{
    if (evt->down)
    {
        switch (evt->button)
        {
            case PB_A:
            case PB_UP:
            case PB_START:
            {
                switch (colorchord->optSel)
                {
                    case COLORCHORD_NUM_OPTS:
                    case CC_OPT_GAIN:
                    {
                        // Gain
                        incMicGain();
                        // Reset max val
                        colorchord->maxValue = 1;
                        break;
                    }
                    case CC_OPT_LED_MODE:
                    {
                        // LED Output
                        colorchordMode_t newMode = (getColorchordMode() + 1) % NUM_CC_MODES;
                        setColorchordMode(newMode);
                        break;
                    }
                    case CC_OPT_LED_BRIGHT:
                    {
                        incLedBrightness();
                        break;
                    }
                }
                break;
            }
            case PB_DOWN:
            case PB_B:
            {
                switch (colorchord->optSel)
                {
                    case COLORCHORD_NUM_OPTS:
                    case CC_OPT_GAIN:
                    {
                        // Gain
                        decMicGain();
                        // Reset max val
                        colorchord->maxValue = 1;
                        break;
                    }
                    case CC_OPT_LED_MODE:
                    {
                        // LED Output
                        colorchordMode_t newMode = getColorchordMode();
                        if (newMode == 0)
                        {
                            newMode = NUM_CC_MODES - 1;
                        }
                        else
                        {
                            newMode--;
                        }
                        setColorchordMode(newMode);
                        break;
                    }
                    case CC_OPT_LED_BRIGHT:
                    {
                        decLedBrightness();
                        break;
                    }
                }
                break;
            }
            case PB_SELECT:
            case PB_RIGHT:
            {
                // Select option
                colorchord->optSel++;
                if (colorchord->optSel >= COLORCHORD_NUM_OPTS)
                {
                    colorchord->optSel = 0;
                }
                break;
            }
            case PB_LEFT:
            {
                // Select option
                if (colorchord->optSel == 0)
                {
                    colorchord->optSel = COLORCHORD_NUM_OPTS - 1;
                }
                else
                {
                    colorchord->optSel--;
                }
                break;
            }
            case TB_0:
            case TB_1:
            case TB_2:
            case TB_3:
            case TB_4:
            {
                // Ignore touch pads
                break;
            }
        }
    }
}

/**
 * @brief Audio callback. Take the samples and pass them to colorchord
 *
 * @param samples The samples to process
 * @param sampleCnt The number of samples to process
 */
void colorchordAudioCb(uint16_t* samples, uint32_t sampleCnt)
{
    uint16_t* sampleHist     = colorchord->sampleHist;
    uint16_t sampleHistHead  = colorchord->sampleHistHead;
    uint16_t sampleHistCount = colorchord->sampleHistCount;

    // For each sample
    for (uint32_t idx = 0; idx < sampleCnt; idx++)
    {
        uint16_t sample = samples[idx];

        // Push to colorchord
        PushSample32(&colorchord->dd, sample);

        sampleHist[sampleHistHead] = sample;
        sampleHistHead++;
        if (sampleHistHead == sampleHistCount)
        {
            sampleHistHead = 0;
        }

        // If 128 samples have been pushed
        colorchord->samplesProcessed++;
        if (colorchord->samplesProcessed >= 128)
        {
            // Update LEDs
            colorchord->samplesProcessed = 0;
            HandleFrameInfo(&colorchord->end, &colorchord->dd);
            switch (getColorchordMode())
            {
                default:
                case NUM_CC_MODES:
                case ALL_SAME_LEDS:
                {
                    UpdateAllSameLEDs(&colorchord->eod, &colorchord->end);
                    break;
                }
                case LINEAR_LEDS:
                {
                    UpdateLinearLEDs(&colorchord->eod, &colorchord->end);
                    break;
                }
            }
            setLeds((led_t*)colorchord->eod.ledOut, CONFIG_NUM_LEDS);
        }
    }

    colorchord->sampleHistHead = sampleHistHead;
}
