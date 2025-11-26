//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include <esp_log.h>
#include <esp_timer.h>

#include "colorchord.h"
#include "colorchordTypes.h"

// For colorchord
#include "embeddedOut.h"

//==============================================================================
// Defines
//==============================================================================

#define TEXT_Y      10
#define TEXT_MARGIN 20

#define EYE_FPS    20
#define EYE_FPS_US (1000000 / EYE_FPS)

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
    dft32_data dd;
    embeddedNf_data end;
    embeddedOut_data eod;
    uint8_t samplesProcessed;
    uint16_t maxValue;
    ccOpt_t optSel;
    uint16_t* sampleHist;
    uint16_t sampleHistHead;
    uint16_t sampleHistCount;

    // Variables for LED eyes
    int32_t eyeTimer;
    uint16_t binAvgHist[EYE_FPS];
    int32_t bahIdx;
    uint8_t barHeights[EYE_LED_W];
<<<<<<< HEAD
=======
    uint8_t bmpSlot;
>>>>>>> origin/main
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
    .overrideSelectBtn        = false,
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
    colorchord = (colorchord_t*)heap_caps_calloc(1, sizeof(colorchord_t), MALLOC_CAP_8BIT);

    colorchord->sampleHistCount = 512;
    colorchord->sampleHist
        = (uint16_t*)heap_caps_calloc(colorchord->sampleHistCount, sizeof(uint16_t), MALLOC_CAP_8BIT);
    colorchord->sampleHistHead = 0;

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
        heap_caps_free(colorchord->sampleHist);
    }
    heap_caps_free(colorchord);
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

    // This is the center line to draw the graph around
    uint8_t centerLine = TFT_HEIGHT / 2;

    // Draw sine wave
    uint16_t* sampleHist     = colorchord->sampleHist;
    uint16_t sampleHistCount = colorchord->sampleHistCount;
    int16_t sampleHistMark   = colorchord->sampleHistHead - 1;

    int16_t lastX = 0xFF;
    int16_t lastY = 0;
    for (int16_t x = 0; x < TFT_WIDTH; x++)
    {
        if (sampleHistMark < 0)
        {
            sampleHistMark = sampleHistCount - 1;
        }
        int16_t sample = sampleHist[sampleHistMark];
        sampleHistMark--;
        uint16_t y = ((sample * TFT_HEIGHT) >> 16) + centerLine;
        if (0xFF != lastX)
        {
            drawLineFast(lastX, lastY, x, y, c333);
        }
        lastX = x;
        lastY = y;
    }

    // Find the max value
    for (uint16_t i = 0; i < FIX_BINS; i++)
    {
        if (colorchord->end.fuzzed_bins[i] > colorchord->maxValue)
        {
            colorchord->maxValue = colorchord->end.fuzzed_bins[i];
        }
    }

    // Draw the spectrum as a bar graph. Figure out bar and margin size
    int16_t binWidth  = (TFT_WIDTH / FIX_BINS);
    int16_t binMargin = (TFT_WIDTH - (binWidth * FIX_BINS)) / 2;

    // Plot the bars
    for (uint16_t i = 0; i < FIX_BINS; i++)
    {
        uint8_t height
            = ((TFT_HEIGHT - getSysFont()->height - 2) * colorchord->end.fuzzed_bins[i]) / colorchord->maxValue;

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

    // Draw the HUD text
    char text[16] = {0};

    // Draw gain indicator
    uint8_t gain = getMicGainSetting();
    snprintf(text, sizeof(text), "Gain: %" PRIu8, gain);
    drawText(getSysFont(), c555, text, TEXT_MARGIN, TEXT_Y);

    // Underline it if selected
    if (CC_OPT_GAIN == colorchord->optSel)
    {
        int16_t lineY = 10 + getSysFont()->height + 2;
        drawLine(TEXT_MARGIN, lineY, TEXT_MARGIN + textWidth(getSysFont(), text) - 1, lineY, c555, 0);
    }

    // Draw LED brightness indicator
    snprintf(text, sizeof(text), "LED: %" PRIu8, getLedBrightnessSetting());
    int16_t tWidth = textWidth(getSysFont(), text);
    drawText(getSysFont(), c555, text, (TFT_WIDTH - tWidth) / 2, TEXT_Y);

    // Underline it if selected
    if (CC_OPT_LED_BRIGHT == colorchord->optSel)
    {
        int16_t lineY = TEXT_Y + getSysFont()->height + 2;
        drawLine((TFT_WIDTH - tWidth) / 2, lineY, (TFT_WIDTH - tWidth) / 2 + tWidth - 1, lineY, c555, 0);
    }

    // Draw colorchord mode
    switch (getColorchordModeSetting())
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
    int16_t textX = TFT_WIDTH - TEXT_MARGIN - textWidth(getSysFont(), text);
    drawText(getSysFont(), c555, text, textX, TEXT_Y);

    // Underline it if selected
    if (CC_OPT_LED_MODE == colorchord->optSel)
    {
        int16_t lineY = TEXT_Y + getSysFont()->height + 2;
        drawLine(textX, lineY, textX + textWidth(getSysFont(), text) - 1, lineY, c555, 0);
    }

    // Draw reminder text
    const char exitText[] = "Hold Menu to Exit";
    int16_t exitWidth     = textWidth(getSysFont(), exitText);
    drawText(getSysFont(), c555, exitText, (TFT_WIDTH - exitWidth) / 2, TFT_HEIGHT - getSysFont()->height - TEXT_Y);

    RUN_TIMER_EVERY(colorchord->eyeTimer, EYE_FPS_US, elapsedUs, {
        // There are 24 bins and 12 columns, so average every two bins into a column
        uint16_t binAvgs[EYE_LED_W] = {0};
        uint16_t maxBinAvg          = 0;
        for (int32_t bIdx = 0; bIdx < ARRAY_SIZE(binAvgs); bIdx++)
        {
            binAvgs[bIdx] = (colorchord->end.folded_bins[2 * bIdx] + colorchord->end.folded_bins[(2 * bIdx) + 1]) / 2;

            // Keep track of the largest average
            if (binAvgs[bIdx] > maxBinAvg)
            {
                maxBinAvg = binAvgs[bIdx];
            }
        }

        // Add the largest average to the history
        colorchord->binAvgHist[colorchord->bahIdx] = maxBinAvg;
        colorchord->bahIdx                         = (colorchord->bahIdx + 1) % ARRAY_SIZE(colorchord->binAvgHist);

        // Find the largest average in the moving history
        for (int32_t bIdx = 0; bIdx < ARRAY_SIZE(colorchord->binAvgHist); bIdx++)
        {
            if (colorchord->binAvgHist[bIdx] > maxBinAvg)
            {
                maxBinAvg = colorchord->binAvgHist[bIdx];
            }
        }

        // Bring up the average to a floor, derived from mic gain.
        // This prevents visualizing noise on the LEDs
        maxBinAvg = MAX(87 * gain, maxBinAvg);

<<<<<<< HEAD
=======
        // Don't bother drawing if there is no change
        bool change = false;

>>>>>>> origin/main
        // Normalize the values to LED_H
        if (maxBinAvg)
        {
            for (int32_t bIdx = 0; bIdx < ARRAY_SIZE(binAvgs); bIdx++)
            {
                binAvgs[bIdx] = ((1 + EYE_LED_H) * binAvgs[bIdx]) / maxBinAvg;

                if (binAvgs[bIdx] > colorchord->barHeights[bIdx])
                {
                    colorchord->barHeights[bIdx] = binAvgs[bIdx];
<<<<<<< HEAD
=======
                    change                       = true;
>>>>>>> origin/main
                }
                else if (colorchord->barHeights[bIdx])
                {
                    colorchord->barHeights[bIdx]--;
<<<<<<< HEAD
=======
                    change = true;
>>>>>>> origin/main
                }
            }
        }

<<<<<<< HEAD
        // Draw bars to the eye LEDs
        uint8_t bitmap[EYE_LED_H][EYE_LED_W] = {0};
        for (int x = 0; x < EYE_LED_W; x++)
        {
            for (int y = 0; y < EYE_LED_H; y++)
            {
                if (y > (EYE_LED_H - colorchord->barHeights[x]))
                {
                    bitmap[EYE_LED_H - 1 - y][x] = EYE_LED_BRIGHT;
                }
            }
        }
        ch32v003WriteBitmap(0, bitmap);
        ch32v003SelectBitmap(0);
=======
        if (change)
        {
            // Draw bars to the eye LEDs
            uint8_t bitmap[EYE_LED_H][EYE_LED_W] = {0};
            for (int x = 0; x < EYE_LED_W; x++)
            {
                for (int y = 0; y < EYE_LED_H; y++)
                {
                    if (y > (EYE_LED_H - colorchord->barHeights[x]))
                    {
                        bitmap[EYE_LED_H - 1 - y][x] = EYE_LED_BRIGHT;
                    }
                }
            }
            // Write and select the bitmap to an unused slot
            ch32v003WriteBitmap(colorchord->bmpSlot, bitmap);
            ch32v003SelectBitmap(colorchord->bmpSlot);
            // Set up the next slot for the next frame
            colorchord->bmpSlot = (colorchord->bmpSlot + 1) % CH32V003_MAX_IMAGE_SLOTS;
        }
>>>>>>> origin/main
    });
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
                        incMicGainSetting();
                        // Reset max val
                        colorchord->maxValue = 1;
                        break;
                    }
                    case CC_OPT_LED_MODE:
                    {
                        // LED Output
                        colorchordMode_t newMode = (getColorchordModeSetting() + 1) % NUM_CC_MODES;
                        setColorchordModeSetting(newMode);
                        break;
                    }
                    case CC_OPT_LED_BRIGHT:
                    {
                        incLedBrightnessSetting();
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
                        decMicGainSetting();
                        // Reset max val
                        colorchord->maxValue = 1;
                        break;
                    }
                    case CC_OPT_LED_MODE:
                    {
                        // LED Output
                        colorchordMode_t newMode = getColorchordModeSetting();
                        if (newMode == 0)
                        {
                            newMode = NUM_CC_MODES - 1;
                        }
                        else
                        {
                            newMode--;
                        }
                        setColorchordModeSetting(newMode);
                        break;
                    }
                    case CC_OPT_LED_BRIGHT:
                    {
                        decLedBrightnessSetting();
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
            switch (getColorchordModeSetting())
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
