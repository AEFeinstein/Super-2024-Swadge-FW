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
    int32_t* delayLine;
    uint32_t delayLineLen;
    uint32_t delayLineIdx;
} ks_string;

typedef struct
{
    q24_8 notes[NUM_STRINGS];
    const char* name;
} guitarChord_t;

typedef struct
{
    bool isPlaying;
    ks_string strings[NUM_STRINGS];
    bool tpTouched;
    vec_t tpJs;
    uint16_t btnState;
    const char* chordStr;
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
    },
    // PB_UP
    {
        .name  = "D Major",
        .notes = {0, 0, D3, A3, D4, Fs4},
    },
    // PB_DOWN
    {
        .name  = "E Minor",
        .notes = {E2, B2, E3, G3, B3, E4},
    },
    // PB_DOWN | PB_UP
    {
        .name  = "None",
        .notes = {0, 0, 0, 0, 0, 0},
    },
    // PB_LEFT
    {
        .name  = "G Major",
        .notes = {G2, B2, D3, G3, B3, G4},
    },
    // PB_LEFT | PB_UP
    {
        .name  = "None",
        .notes = {0, 0, 0, 0, 0, 0},
    },
    // PB_LEFT | PB_DOWN
    {
        .name  = "None",
        .notes = {0, 0, 0, 0, 0, 0},
    },
    // PB_LEFT | PB_DOWN | PB_UP
    {
        .name  = "None",
        .notes = {0, 0, 0, 0, 0, 0},
    },
    // PB_RIGHT
    {
        .name  = "C Major",
        .notes = {0, C3, E3, G3, C4, E4},
    },
    // PB_RIGHT | PB_UP
    {
        .name  = "None",
        .notes = {0, 0, 0, 0, 0, 0},
    },
    // PB_RIGHT | PB_DOWN
    {
        .name  = "None",
        .notes = {0, 0, 0, 0, 0, 0},
    },
    // PB_RIGHT | PB_DOWN | PB_UP
    {
        .name  = "None",
        .notes = {0, 0, 0, 0, 0, 0},
    },
    // PB_RIGHT | PB_LEFT
    {
        .name  = "None",
        .notes = {0, 0, 0, 0, 0, 0},
    },
    // PB_RIGHT | PB_LEFT | PB_UP
    {
        .name  = "None",
        .notes = {0, 0, 0, 0, 0, 0},
    },
    // PB_RIGHT | PB_LEFT | PB_DOWN
    {
        .name  = "None",
        .notes = {0, 0, 0, 0, 0, 0},
    },
    // PB_RIGHT | PB_LEFT | PB_DOWN | PB_UP
    {
        .name  = "None",
        .notes = {0, 0, 0, 0, 0, 0},
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
    // Uncapped framerate
    setFrameRateUs(0);

    // Initialize mode
    ks = heap_caps_calloc(1, sizeof(karplusStrong_t), MALLOC_CAP_8BIT);
}

/**
 * @brief Exit KS mode and deinitialize variables
 */
static void ksExitMode(void)
{
    for (int i = 0; i < ARRAY_SIZE(ks->strings); i++)
    {
        deinitKsString(&ks->strings[i]);
    }
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
        ks->btnState = evt.state & (PB_UP | PB_DOWN | PB_LEFT | PB_RIGHT);
        ks->chordStr = gChords[ks->btnState].name;
    }

    // Check touchpad events
    int32_t angle, radius, intensity;
    if (getTouchJoystick(&angle, &radius, &intensity))
    {
        vec_t lastJs = ks->tpJs;
        getTouchCartesian(angle, radius, &ks->tpJs.x, &ks->tpJs.y);
        if (ks->tpTouched)
        {
            // Vertical zero-crossing occurred
            if ((ks->tpJs.y > 512 && lastJs.y <= 512) || (ks->tpJs.y < 512 && lastJs.y >= 512))
            {
                ks->isPlaying = false;
                for (int sIdx = 0; sIdx < ARRAY_SIZE(ks->strings); sIdx++)
                {
                    deinitKsString(&ks->strings[sIdx]);
                    if (gChords[ks->btnState].notes[sIdx])
                    {
                        initKsString(&ks->strings[sIdx], gChords[ks->btnState].notes[sIdx]);
                        ks->isPlaying = true;
                    }
                }
            }
        }
        ks->tpTouched = true;
    }
    else
    {
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
    paletteColor_t* tftFb = getPxTftFramebuffer();
    memset(&tftFb[y * TFT_WIDTH + x], c111, w * h);
}

/**
 * @brief Callback from the DAC to generate Karplus Strong samples
 *
 * @param samples The samples to write to
 * @param len The length of the samples to write
 */
static void ksDacCallback(uint8_t* samples, int16_t len)
{
    if (ks->isPlaying)
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
    else
    {
        memset(samples, 128, len);
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
        heap_caps_free(string->delayLine);
        memset(string, 0, sizeof(ks_string));
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

        // Decay the current delay line value
        string->delayLine[string->delayLineIdx] = (averaged * 32637) / 32768; // 0.99600219726

        // Iterate delay line index
        string->delayLineIdx = nextIdx;

        // Get the 8 bit output sample
        int32_t decayed = (current * 31130) / 32768; // 0.95001220703
        int32_t clamped = CLAMP(decayed, -32768, 32767);
        samples[sIdx]   = (clamped + 32768) / 256;
    }
}
