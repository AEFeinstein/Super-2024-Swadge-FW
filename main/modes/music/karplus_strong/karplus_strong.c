//==============================================================================
// Includes
//==============================================================================

#include "karplus_strong.h"
#include "fp_math.h"

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
    ks_string string;
    bool tpTouched;
    vec_t tpJs;
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

//==============================================================================
// Function Prototypes
//==============================================================================

static void ksEnterMode(void);
static void ksExitMode(void);
static void ksMainLoop(int64_t elapsedUs);
static void ksBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);
static void ksDacCallback(uint8_t* samples, int16_t len);

static void initKsString(ks_string* string, q24_8 frequency);
static void reinitKsString(ks_string* string);
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
    initKsString(&ks->string, TO_FX(440));
}

/**
 * @brief Exit KS mode and deinitialize variables
 */
static void ksExitMode(void)
{
    deinitKsString(&ks->string);
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
        if (evt.down)
        {
            reinitKsString(&ks->string);
        }
    }

    // Check touchpad events
    int32_t angle, radius, intensity;
    if (getTouchJoystick(&angle, &radius, &intensity))
    {
        vec_t lastJs = ks->tpJs;
        getTouchCartesian(angle, radius, &ks->tpJs.x, &ks->tpJs.y);
        if (ks->tpTouched)
        {
            if ((ks->tpJs.y > 512 && lastJs.y <= 512) || (ks->tpJs.y < 512 && lastJs.y >= 512))
            {
                reinitKsString(&ks->string);
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
    // Make sure the string is initialized first
    if (ks->string.delayLine)
    {
        genKsStringSamples(&ks->string, samples, len);
    }
    else
    {
        // If not initialized, fill with 'zeros'
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
    // Calculate the delay line length, then allocate it and fill it with random numbers
    // With the division the q24_8 math comes out to a normal integer
    string->delayLineLen = (TO_FX(DAC_SAMPLE_RATE_HZ) + (frequency / 2)) / frequency;
    string->delayLine    = heap_caps_calloc(string->delayLineLen, sizeof(int32_t), MALLOC_CAP_8BIT);
    esp_fill_random(string->delayLine, string->delayLineLen * sizeof(int32_t));

    // Reset the index
    string->delayLineIdx = 0;
}

/**
 * @brief Reinitialize a Karplus Strong string by randomizing the delay line
 *
 * @param string The string to reinitialize
 */
static void reinitKsString(ks_string* string)
{
    esp_fill_random(string->delayLine, string->delayLineLen * sizeof(int32_t));
    string->delayLineIdx = 0;
}

/**
 * @brief Deinitialize a Karplus Strong string
 *
 * @param string The string to deinitialize
 */
static void deinitKsString(ks_string* string)
{
    heap_caps_free(string->delayLine);
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
