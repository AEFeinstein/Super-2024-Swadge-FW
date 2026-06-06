//==============================================================================
// Includes
//==============================================================================

#include <stdio.h>

#include <esp_timer.h>
#include <esp_heap_caps.h>
#include <esp_log.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "hdw-touch.h"

//==============================================================================
// Variables
//==============================================================================

/// The number of configured touchPads
static int _numTouchPads = 0;
/// A pointer to an array of configured touchPads
static const touch_pad_t* _touchPads = NULL;
/// Touch pad sensitivity
static float _touchPadSensitivity = 0;
/// Touch pad denoise enable
static bool _denoiseEnable = false;

/// Used in getBaseTouchVals() to get zeroed touch pad values
static int32_t* baseOffsets = NULL;

//==============================================================================
// Function Declarations
//==============================================================================

static int getTouchRawValues(uint32_t* rawValues, int maxPads);
static int getBaseTouchVals(int32_t* data, int count);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initialize touchPad sensors
 *
 * @param touchPads A list of touchPads to initialize
 * @param numTouchPads The number of touchPads to initialize
 * @param touchPadSensitivity The sensitivity to set for these touchPads
 * @param denoiseEnable true to denoise the input, false to use it raw
 */
void initTouchPads(const touch_pad_t* touchPads, uint8_t numTouchPads, float touchPadSensitivity, bool denoiseEnable)
{
    ESP_LOGD("TOUCH", "Initializing touch pads");

    /* Save the list of touchPads */
    if (NULL != touchPads)
    {
        _numTouchPads        = numTouchPads;
        _touchPads           = touchPads;
        _touchPadSensitivity = touchPadSensitivity;
        _denoiseEnable       = denoiseEnable;
    }

    /* Initialize touch pad peripheral. */
    ESP_ERROR_CHECK(touch_pad_init());

    /* Initialize each touch pad */
    for (uint8_t i = 0; i < _numTouchPads; i++)
    {
        ESP_ERROR_CHECK(touch_pad_config(_touchPads[i]));
    }

    /* Initialize denoise if requested */
    if (_denoiseEnable)
    {
        /* Denoise setting at TouchPads 0. */
        touch_pad_denoise_t denoise = {
            /* The bits to be cancelled are determined according to the noise
             * level.
             */
            .grade = TOUCH_PAD_DENOISE_BIT4,
            /* By adjusting the parameters, the reading of T0 should be
             * approximated to the reading of the measured channel.
             */
            .cap_level = TOUCH_PAD_DENOISE_CAP_L4,
        };
        ESP_ERROR_CHECK(touch_pad_denoise_set_config(&denoise));
        ESP_ERROR_CHECK(touch_pad_denoise_enable());
        ESP_LOGD("TOUCH", "Denoise function init");
    }

    /* Filter setting */
    touch_filter_config_t filter_info = {
        .mode         = TOUCH_PAD_FILTER_IIR_16, // Test jitter and filter 1/4.
        .debounce_cnt = 1,                       // 1 time count.
        .noise_thr    = 0,                       // 50%
        .jitter_step  = 4,                       // use for jitter mode.
        .smh_lvl      = TOUCH_PAD_SMOOTH_IIR_2,
    };
    ESP_ERROR_CHECK(touch_pad_filter_set_config(&filter_info));
    ESP_ERROR_CHECK(touch_pad_filter_enable());
    ESP_LOGD("TOUCH", "touch pad filter init");
#if defined(SOC_TOUCH_PAD_THRESHOLD_MAX)
    ESP_ERROR_CHECK(touch_pad_timeout_set(true, SOC_TOUCH_PAD_THRESHOLD_MAX));
#elif defined(TOUCH_PAD_THRESHOLD_MAX)
    ESP_ERROR_CHECK(touch_pad_timeout_set(true, TOUCH_PAD_THRESHOLD_MAX));
#else
    #error "Touch pad threshold max not defined"
#endif

    /* Enable interrupts, but not TOUCH_PAD_INTR_MASK_SCAN_DONE */
    ESP_ERROR_CHECK(
        touch_pad_intr_enable(TOUCH_PAD_INTR_MASK_ACTIVE | TOUCH_PAD_INTR_MASK_INACTIVE | TOUCH_PAD_INTR_MASK_TIMEOUT));

    /* Enable touch pad clock. Work mode is "timer trigger" */
    ESP_ERROR_CHECK(touch_pad_set_fsm_mode(TOUCH_FSM_MODE_TIMER));
    ESP_ERROR_CHECK(touch_pad_fsm_start());

    /* Wait touch pad init done */
    vTaskDelay(50 / portTICK_PERIOD_MS);

    /* Set thresholds */
    uint32_t touch_value;
    for (int i = 0; i < _numTouchPads; i++)
    {
        /* read benchmark value */
        ESP_ERROR_CHECK(touch_pad_read_benchmark(_touchPads[i], &touch_value));
        /* set interrupt threshold */
        ESP_ERROR_CHECK(touch_pad_set_thresh(_touchPads[i], touch_value * _touchPadSensitivity));
        ESP_LOGD("TOUCH", "touch pad [%d] base %lu, thresh %lu", _touchPads[i], touch_value,
                 (uint32_t)(touch_value * _touchPadSensitivity));
    }

    getTouchJoystick(0, 0, 0);
}

/**
 * @brief Deinitialize the touch pads
 */
void deinitTouchPads(void)
{
    powerDownTouchPads();
}

/**
 * @brief Power up the touchpad
 */
void powerUpTouchPads(void)
{
    initTouchPads(_touchPads, _numTouchPads, _touchPadSensitivity, _denoiseEnable);
}

/**
 * @brief Power down the touchpad
 */
void powerDownTouchPads(void)
{
    // Disable touch pads
    ESP_ERROR_CHECK(touch_pad_fsm_stop());
    ESP_ERROR_CHECK(touch_pad_reset());
    ESP_ERROR_CHECK(touch_pad_deinit());

    if (baseOffsets)
    {
        heap_caps_free(baseOffsets);
        baseOffsets = NULL;
    }
}

/**
 * @brief Get totally raw touch pad values from buffer.
 * NOTE: You must have touch callbacks enabled to use this.
 *
 * @param rawValues is a pointer to an array of int32_t's to receive the raw touch data.
 * @param maxPads is the number of ints in your array.
 * @return is the number of values that were successfully read.
 */
static int getTouchRawValues(uint32_t* rawValues, int maxPads)
{
    if (maxPads > _numTouchPads)
    {
        maxPads = _numTouchPads;
    }
    for (int i = 0; i < maxPads; i++)
    {
        // If any errors, abort.
        if (touch_pad_read_raw_data(_touchPads[i], &rawValues[i]))
        {
            return 0;
        }
    }
    return maxPads;
}

/**
 * @brief Get "zeroed" touch pad values.
 * NOTE: You must have touch callbacks enabled to use this.
 *
 * @param data is a pointer to an array of int32_t's to receive the zeroed touch data.
 * @param count is the number of ints in your array.
 * @return is the number of values that were successfully read.
 */
int getBaseTouchVals(int32_t* data, int count)
{
    uint32_t curVals[_numTouchPads];
    if (getTouchRawValues(curVals, _numTouchPads) == 0)
    {
        return 0;
    }
    for (int i = 0; i < _numTouchPads; i++)
    {
        if (curVals[i] == 0)
        {
            return 0;
        }
    }

    if (count > _numTouchPads)
    {
        count = _numTouchPads;
    }

    // curVals is valid.
    if (NULL == baseOffsets)
    {
        baseOffsets = heap_caps_malloc(sizeof(baseOffsets[0]) * _numTouchPads, MALLOC_CAP_8BIT);
        for (int i = 0; i < _numTouchPads; i++)
        {
            baseOffsets[i] = curVals[i] << 8;
        }
    }

    for (int i = 0; i < _numTouchPads; i++)
    {
        int32_t base     = baseOffsets[i];
        int32_t val      = curVals[i];
        int32_t baseNorm = (base >> 8);

        // Asymmetric filter on base.
        if (baseNorm < val)
        {
            base++; // VERY slowly slack offset up.
        }
        else
        {
            base -= 8192; // VERY quickly slack up.
        }

        baseOffsets[i] = base;
        if (i < count)
        {
            data[i] = val - baseNorm;
        }
    }

    return count;
}

/**
 * @brief Get high-level touch input, an analog input.
 * NOTE: You must have touch callbacks enabled to use this.
 *
 * @param[out] phi the angle of the touch. Where 0 is right, 320 is up, 640 is left and 960 is down.
 * @param[out] r is how far from center you are.  511 is on the outside edge, 0 is on the inside.
 * @param[out] intensity is how hard the user is pressing.
 * @return true if touched (joystick), false if not touched (no centroid)
 */
int getTouchJoystick(int32_t* phi, int32_t* r, int32_t* intensity)
{
#define TOUCH_CENTER 2
    const uint8_t ringZones[] = {3, 0, 1, 4, 5};
#define NUM_TZ_RING 5
    int32_t baseVals[6];
    int32_t ringIntensity = 0;
    int bc                = getBaseTouchVals(baseVals, 6);
    if (bc != 6)
        return 0;

    int centerIntensity = baseVals[TOUCH_CENTER];

    // First, compute phi.

    // Find most pressed pad
    int peak    = -1;
    int peakBin = -1;
    for (int i = 0; i < NUM_TZ_RING; i++)
    {
        int32_t bv = baseVals[ringZones[i]];
        if (bv > peak)
        {
            peak    = bv;
            peakBin = i;
        }
    }

    if (peakBin < 0)
    {
        return 0;
    }

    // Arbitrary, but we use 1200 as the minimum peak value.
    if (peak < 4200 && centerIntensity < 4200)
    {
        return 0;
    }

    // We know our peak bin, now we need to know the average and differential of the adjacent bins.
    int leftOfPeak  = (peakBin > 0) ? baseVals[ringZones[peakBin - 1]] : baseVals[ringZones[NUM_TZ_RING - 1]];
    int rightOfPeak = (peakBin < NUM_TZ_RING - 1) ? baseVals[ringZones[peakBin + 1]] : baseVals[ringZones[0]];

    int oPeak  = peak;
    int center = peakBin << 8;

    if (rightOfPeak >= leftOfPeak)
    {
        // We bend upward (or are neutral)
        rightOfPeak -= leftOfPeak;
        peak -= leftOfPeak;
        center += (rightOfPeak << 8) / (rightOfPeak + peak);

        ringIntensity = oPeak + rightOfPeak;
    }
    else
    {
        // We bend downward
        leftOfPeak -= rightOfPeak;
        peak -= rightOfPeak;
        center -= (leftOfPeak << 8) / (leftOfPeak + peak);

        ringIntensity = oPeak + leftOfPeak;
    }

    center -= 80;
    if (center < -1280)
        center += 1280;
    int ringPh = (center < 0) ? (center + 1280) : center;

    // 0->1280 --> 0->360
    ringPh = (ringPh * 9) >> 5;
    if (phi)
    {
        *phi = ringPh;
    }

    // Find ratio of ring to inner.
    int totalIntensity = centerIntensity + ringIntensity;
    int radius         = (ringIntensity << 10) / totalIntensity;

    if (r)
    {
        *r = radius;
    }

    if (intensity)
    {
        *intensity = totalIntensity;
    }

#if defined(CONFIG_HARDWARE_HOTDOG_PROTO)
    // The prototype had a rotated touchPad, so un-rotate it
    *phi = (*phi + 225) % 360;
#endif

    return 1;
}
