//==============================================================================
// Includes
//==============================================================================

#include "karplus_strong.h"
#include "fp_math.h"

//==============================================================================
// Defines
//==============================================================================

#define NUM_STRINGS 6

//==============================================================================
// Enums
//==============================================================================

// In q24_8
typedef enum
{
    E2  = 21096,
    F2  = 22351,
    Fs2 = 23680,
    G2  = 25088,
    Gs2 = 26579,
    A2  = 28160,
    As2 = 29834,
    B2  = 31609,
    C3  = 33488,
    Cs3 = 35479,
    D3  = 37589,
    Ds3 = 39824,
    E3  = 42192,
    F3  = 44701,
    Fs3 = 47359,
    G3  = 50175,
    Gs3 = 53159,
    A3  = 56320,
    As3 = 59669,
    B3  = 63217,
    C4  = 66976,
    Cs4 = 70959,
    D4  = 75178,
    Ds4 = 79649,
    E4  = 84385,
    F4  = 89402,
    Fs4 = 94718,
    G4  = 100351,
    Gs4 = 106318,
    A4  = 112640,
    As4 = 119338,
    B4  = 126434,
    C5  = 133952,
    Cs5 = 141917,
    D5  = 150356,
    Ds5 = 159297,
    E5  = 168769,
    F5  = 178805,
    Fs5 = 189437,
    G5  = 200702,
    Gs5 = 212636,
    A5  = 225280,
    As5 = 238676,
    B5  = 252868,
    C6  = 267905,
    Cs6 = 283835,
    D6  = 300713,
    Ds6 = 318594,
    E6  = 337539,
} q24_8_freq;

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    // Delay line for synthesis
    int32_t* delayLine;
    uint32_t delayLineLen;
    uint32_t delayLineIdx;

    // For touchpad input
    int16_t yCrossing;

    // For TFT drawing
    paletteColor_t color;
} ks_string;

typedef struct
{
    q24_8 notes[NUM_STRINGS];
    const char* name;
    led_t color;
} guitarChord_t;

typedef struct
{
    ks_string strings[NUM_STRINGS];
    vec_t tpJs;
    int32_t stringColorTimer;
    const char* chordStr;

    uint16_t btnState;

    bool tpTouched;

    led_t leds[CONFIG_NUM_LEDS];
    int32_t ledTimer;
} karplusStrong_t;

//==============================================================================
// Const data
//==============================================================================

static const char karplusStrongName[] = "Karplus Strong";

const trophyData_t ksTrophies[] = {

};

const trophySettings_t ksTrophySettings = {
    .drawFromBottom   = true,
    .staticDurationUs = DRAW_STATIC_US * 2,
    .slideDurationUs  = DRAW_SLIDE_US,
    .namespaceKey     = karplusStrongName,
};

const trophyDataList_t ksTrophyData = {
    .settings = &ksTrophySettings,
    .list     = ksTrophies,
    .length   = ARRAY_SIZE(ksTrophies),
};

static const guitarChord_t gChords[] = {
    // Open
    {
        .name  = "Open",
        .notes = {E2, A2, D3, G3, B3, E4},
        .color = {.r = 0xFF, .g = 0xFF, .b = 0xFF},
    },
    // PB_UP
    {
        .name  = "D Major",
        .notes = {0, 0, D3, A3, D4, Fs4},
        .color = {.r = 0xFF, .g = 0x00, .b = 0x00},
    },
    // PB_DOWN
    {
        .name  = "E Minor",
        .notes = {E2, B2, E3, G3, B3, E4},
        .color = {.r = 0xFF, .g = 0x00, .b = 0xFF},
    },
    // PB_DOWN | PB_UP
    {
        .name  = "None",
        .notes = {0, 0, 0, 0, 0, 0},
        .color = {.r = 0xFF, .g = 0xFF, .b = 0xFF},
    },
    // PB_LEFT
    {
        .name  = "G Major",
        .notes = {G2, B2, D3, G3, B3, G4},
        .color = {.r = 0x00, .g = 0xFF, .b = 0x00},
    },
    // PB_LEFT | PB_UP
    {
        .name  = "None",
        .notes = {0, 0, 0, 0, 0, 0},
        .color = {.r = 0xFF, .g = 0xFF, .b = 0xFF},
    },
    // PB_LEFT | PB_DOWN
    {
        .name  = "None",
        .notes = {0, 0, 0, 0, 0, 0},
        .color = {.r = 0xFF, .g = 0xFF, .b = 0xFF},
    },
    // PB_LEFT | PB_DOWN | PB_UP
    {
        .name  = "None",
        .notes = {0, 0, 0, 0, 0, 0},
        .color = {.r = 0xFF, .g = 0xFF, .b = 0xFF},
    },
    // PB_RIGHT
    {
        .name  = "C Major",
        .notes = {0, C3, E3, G3, C4, E4},
        .color = {.r = 0x00, .g = 0x00, .b = 0xFF},
    },
    // PB_RIGHT | PB_UP
    {
        .name  = "None",
        .notes = {0, 0, 0, 0, 0, 0},
        .color = {.r = 0xFF, .g = 0xFF, .b = 0xFF},
    },
    // PB_RIGHT | PB_DOWN
    {
        .name  = "None",
        .notes = {0, 0, 0, 0, 0, 0},
        .color = {.r = 0xFF, .g = 0xFF, .b = 0xFF},
    },
    // PB_RIGHT | PB_DOWN | PB_UP
    {
        .name  = "None",
        .notes = {0, 0, 0, 0, 0, 0},
        .color = {.r = 0xFF, .g = 0xFF, .b = 0xFF},
    },
    // PB_RIGHT | PB_LEFT
    {
        .name  = "None",
        .notes = {0, 0, 0, 0, 0, 0},
        .color = {.r = 0xFF, .g = 0xFF, .b = 0xFF},
    },
    // PB_RIGHT | PB_LEFT | PB_UP
    {
        .name  = "None",
        .notes = {0, 0, 0, 0, 0, 0},
        .color = {.r = 0xFF, .g = 0xFF, .b = 0xFF},
    },
    // PB_RIGHT | PB_LEFT | PB_DOWN
    {
        .name  = "None",
        .notes = {0, 0, 0, 0, 0, 0},
        .color = {.r = 0xFF, .g = 0xFF, .b = 0xFF},
    },
    // PB_RIGHT | PB_LEFT | PB_DOWN | PB_UP
    {
        .name  = "None",
        .notes = {0, 0, 0, 0, 0, 0},
        .color = {.r = 0xFF, .g = 0xFF, .b = 0xFF},
    },
};

//==============================================================================
// Function Prototypes
//==============================================================================

static void ksEnterMode(void);
static void ksExitMode(void);
static void ksMainLoop(int64_t elapsedUs);
static void ksBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);
static void ksDacCallback(uint8_t* samples, int16_t len);

static void initKsString(ks_string* string, q24_8 frequency);
static void deinitKsString(ks_string* string);
static void genKsStringSamples(ks_string* string, uint8_t* samples, uint32_t numSamples);

//==============================================================================
// Variables
//==============================================================================

karplusStrong_t* ks = NULL;

swadgeMode_t karplusStrongMode = {
    .modeName                 = karplusStrongName,
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = false,
    .usesAccelerometer        = false,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = ksEnterMode,
    .fnExitMode               = ksExitMode,
    .fnMainLoop               = ksMainLoop,
    .fnBackgroundDrawCallback = ksBackgroundDrawCallback,
    .fnDacCb                  = ksDacCallback,
    .trophyData               = &ksTrophyData,
};

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Enter KS mode and initialize variables
 */
static void ksEnterMode(void)
{
    // 60 FPS
    setFrameRateUs(1000000 / 60);

    // Initialize mode
    ks = heap_caps_calloc(1, sizeof(karplusStrong_t), MALLOC_CAP_8BIT);

    // Set pluck crossings (touchpad is 0->1024)
    int16_t gap = 1024 / (1 + ARRAY_SIZE(ks->strings));
    int16_t yc  = 1024 - gap;
    for (int sIdx = 0; sIdx < ARRAY_SIZE(ks->strings); sIdx++)
    {
        ks->strings[sIdx].yCrossing = yc;
        yc -= gap;
    }

    // Blank LEDs
    setLeds(ks->leds, CONFIG_NUM_LEDS);
}

/**
 * @brief Exit KS mode and deinitialize variables
 */
static void ksExitMode(void)
{
    // Deinit all strings
    for (int sIdx = 0; sIdx < ARRAY_SIZE(ks->strings); sIdx++)
    {
        deinitKsString(&ks->strings[sIdx]);
    }

    // Free memory
    heap_caps_free(ks);
}

/**
 * @brief Run the KS main loop. This handles button input and screen drawing.
 * Synthesis happens in ksDacCallback()
 *
 * @param elapsedUs The time since this was last called
 */
static void ksMainLoop(int64_t elapsedUs)
{
    // Check button events
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        // Save state and name
        ks->btnState = evt.state & (PB_UP | PB_DOWN | PB_LEFT | PB_RIGHT);
        ks->chordStr = gChords[ks->btnState].name;
    }

    // Check touchpad events
    int32_t angle, radius, intensity;
    if (getTouchJoystick(&angle, &radius, &intensity))
    {
        // Save last touchpad location
        vec_t lastJs = ks->tpJs;
        // Get current touchpad location
        getTouchCartesian(angle, radius, &ks->tpJs.x, &ks->tpJs.y);
        // If the touchpad was previously touched, we can compare points
        if (ks->tpTouched)
        {
            // For each string
            for (int sIdx = 0; sIdx < ARRAY_SIZE(ks->strings); sIdx++)
            {
                // If there is a note here
                if (gChords[ks->btnState].notes[sIdx])
                {
                    // Check if a vertical crossing occurred
                    int16_t yc = ks->strings[sIdx].yCrossing;
                    if ((ks->tpJs.y > yc && lastJs.y <= yc) || (ks->tpJs.y < yc && lastJs.y >= yc))
                    {
                        // Pluck the string
                        deinitKsString(&ks->strings[sIdx]);
                        initKsString(&ks->strings[sIdx], gChords[ks->btnState].notes[sIdx]);
                        ks->strings[sIdx].color = c555;

                        ks->leds[sIdx] = gChords[ks->btnState].color;
                    }
                }
            }
        }
        // Mark the touchpad as touched for next time
        ks->tpTouched = true;
    }
    else
    {
        // Mark the touchpad as not touched
        ks->tpTouched = false;
    }

    // Draw FPS
    DRAW_FPS_COUNTER((*getSysFont()));

    // Draw chord name
    if (ks->chordStr)
    {
        font_t* f = getSysFont();
        drawText(f, c555, ks->chordStr, (TFT_WIDTH - textWidth(f, ks->chordStr)) / 2, (TFT_HEIGHT - f->height) / 2);
    }

    // Draw Strings
    paletteColor_t* fb = getPxTftFramebuffer();
    int16_t gap        = TFT_HEIGHT / (1 + ARRAY_SIZE(ks->strings));
    int16_t yIdx       = gap;
    for (int sIdx = 0; sIdx < ARRAY_SIZE(ks->strings); sIdx++)
    {
        memset(&fb[TFT_WIDTH * yIdx], ks->strings[sIdx].color, TFT_WIDTH);
        yIdx += gap;
    }

    // Run a timer to decay string color
    RUN_TIMER_EVERY(ks->stringColorTimer, 1000000 / 6, elapsedUs, {
        for (int sIdx = 0; sIdx < ARRAY_SIZE(ks->strings); sIdx++)
        {
            if (ks->strings[sIdx].color)
            {
                // Math to go c555 -> c444, etc.
                ks->strings[sIdx].color -= (1 + 6 + 36);
            }
        }
    });

    // Run a timer to decay LEDs
    RUN_TIMER_EVERY(ks->ledTimer, 1000000 / 256, elapsedUs, {
        for (int lIdx = 0; lIdx < CONFIG_NUM_LEDS; lIdx++)
        {
            if (ks->leds[lIdx].r)
            {
                ks->leds[lIdx].r--;
            }
            if (ks->leds[lIdx].g)
            {
                ks->leds[lIdx].g--;
            }
            if (ks->leds[lIdx].b)
            {
                ks->leds[lIdx].b--;
            }
        }
    });

    setLeds(ks->leds, CONFIG_NUM_LEDS);
}

/**
 * @brief Fill a background color
 *
 * @param x the x coordinate that should be updated
 * @param y the x coordinate that should be updated
 * @param w the width of the rectangle to be updated
 * @param h the height of the rectangle to be updated
 * @param up update number
 * @param upNum update number denominator
 */
static void ksBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
    // Simple fill
    paletteColor_t* tftFb = getPxTftFramebuffer();
    memset(&tftFb[y * TFT_WIDTH + x], c011, w * h);
}

/**
 * @brief Callback from the DAC to generate Karplus Strong samples
 *
 * @param samples The samples to write to
 * @param len The length of the samples to write
 */
static void ksDacCallback(uint8_t* samples, int16_t len)
{
    // Buffer to sum all string output
    uint16_t sumBuf[len];
    memset(sumBuf, 0, sizeof(sumBuf));

    // For each string
    for (int sIdx = 0; sIdx < ARRAY_SIZE(ks->strings); sIdx++)
    {
        if (ks->strings[sIdx].delayLine)
        {
            // Generate samples
            uint8_t tmpBuf[len];
            genKsStringSamples(&ks->strings[sIdx], tmpBuf, len);

            // Add to the sum buf
            for (int i = 0; i < len; i++)
            {
                sumBuf[i] += tmpBuf[i];
            }
        }
    }

    // Write to output buffer
    for (int i = 0; i < len; i++)
    {
        samples[i] = sumBuf[i] / NUM_STRINGS;
    }
}

/**
 * @brief Initialize a Karplus Strong string to play a given frequency
 *
 * @param string The string to initialize
 * @param frequency The frequency of the string
 */
static void initKsString(ks_string* string, q24_8 frequency)
{
    if (!string->delayLine)
    {
        // Calculate the delay line length, then allocate it and fill it with random numbers
        // With the division the q24_8 math comes out to a normal integer
        string->delayLineLen = (TO_FX(DAC_SAMPLE_RATE_HZ) + (frequency / 2)) / frequency;
        string->delayLine    = heap_caps_calloc(string->delayLineLen, sizeof(int32_t), MALLOC_CAP_8BIT);
        esp_fill_random(string->delayLine, string->delayLineLen * sizeof(int32_t));

        for (int i = 0; i < string->delayLineLen; i++)
        {
            string->delayLine[i] /= 32768;
        }

        // Reset the index
        string->delayLineIdx = 0;
    }
}

/**
 * @brief Deinitialize a Karplus Strong string
 *
 * @param string The string to deinitialize
 */
static void deinitKsString(ks_string* string)
{
    if (string->delayLine)
    {
        // Free the delay line
        heap_caps_free(string->delayLine);

        // Clear everything else, but preserve yCrossing
        int16_t yc = string->yCrossing;
        memset(string, 0, sizeof(ks_string));
        string->yCrossing = yc;
    }
}

/**
 * @brief Generate samples from a Karplus Strong string
 *
 * @param string The string to generate samples from
 * @param samples Where to write the samples
 * @param numSamples The number of samples to write
 */
static void genKsStringSamples(ks_string* string, uint8_t* samples, uint32_t numSamples)
{
    for (uint32_t sIdx = 0; sIdx < numSamples; sIdx++)
    {
        // Find next index in the circular delay line (faster than mod!)
        uint32_t nextIdx = string->delayLineIdx + 1;
        if (nextIdx >= string->delayLineLen)
        {
            nextIdx = 0;
        }

        // Get average of current and next delay line values
        int32_t current  = string->delayLine[string->delayLineIdx];
        int32_t averaged = (current + string->delayLine[nextIdx]) / 2;

        // Update the value in the delay line (decayed average of adjacent samples)
        string->delayLine[string->delayLineIdx] = (averaged * 32637) / 32768; // 0.99600219726

        // Iterate delay line index
        string->delayLineIdx = nextIdx;

        // Get the 8 bit output sample
        int32_t decayed = (current * 31130) / 32768; // 0.95001220703
        int32_t clamped = CLAMP(decayed, -32768, 32767);
        samples[sIdx]   = (clamped + 32768) / 256;
    }
}
