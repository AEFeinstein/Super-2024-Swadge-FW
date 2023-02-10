/*! \file hdw-btn.c
 *
 * \section btn_design Design Philosophy
 *
 * This component handles both pushbuttons and touchpads. Pushbuttons are physical, tactile buttons while touchpads are
 * touch-sensitive areas on the PCB. Events from pushbuttons and touchpads are processed different ways, but queued in
 * the same queue.
 *
 * The Swadge Mode needs to call checkButtonQueue() to receive queued button events.
 * The event contains which button caused the event, whether it was pressed or released, and the current state of all
 * buttons. This way the Swadge Mode is not responsible for high frequency button polling, and can still receive all
 * button inputs.
 *
 * In addition to acting as binary buttons, the touchpads may act as a single, analog, touch sensitive strip. These two
 * ways of reporting are not mutually exclusive.
 *
 * \section pbtn_design Pushbutton Design Philosophy
 *
 * The pushbuttons are polled continuously at 1ms intervals in an interrupt, but these readings are not reported to the
 * Swadge modes. The interrupt saves the prior ::DEBOUNCE_HIST_LEN polled button states and the last reported button
 * state. When all ::DEBOUNCE_HIST_LEN button states are identical, the interrupt accepts the current state and checks
 * if it different than the last reported state. If there is a difference, the button event is queued in the interrupt
 * to be received by the Swadge Mode.
 *
 * The pushbutton GPIOs are all read at the same time using <a
 * href="https://docs.espressif.com/projects/esp-idf/en/v5.0/esp32s2/api-reference/peripherals/dedic_gpio.html">Dedicated
 * GPIO</a>.
 *
 * Originally the pushbuttons would trigger an interrupt, but we found that to have glitchier and less reliable results
 * than polling.
 *
 * Button events used to be delivered to the Swadge Mode via a callback.
 * This led to cases where multiple callbacks would occur between a single invocation of that mode's main function.
 * Because the Swadge Mode didn't have a separate queue for button events, this caused events to be dropped.
 * Instead of forcing each mode to queue button events, now each mode must dequeue them rather than having a callback
 * called.
 *
 * \section tpad_design Touchpad Design Philosophy
 *
 * Unlike pushbutton polling, touchpads use interrupts to detect events. When a touchpad event is detected, an interrupt
 * will fire and the new event will be queued in the same queue used for pushbuttons.
 *
 * In addition to acting as binary buttons, the touchpad can act as a single analog touch strip. getTouchCentroid() may
 * be called to get the current analog touch value. Changes in the analog position are not reported checkButtonQueue(),
 * so getTouchCentroid() must be called as frequently as desired to get values. Do not assume that changes in the analog
 * position are correlated with events reported in checkButtonQueue().
 *
 * Touchpad interrupts are set up and touchpad values are read with <a
 * href="https://docs.espressif.com/projects/esp-idf/en/v5.0/esp32s2/api-reference/peripherals/touch_pad.html">Touch
 * Sensor</a>.
 *
 * \section btn_usage Usage
 *
 * You don't need to call initButtons() or deinitButtons(). The system does at the appropriate times.
 *
 * You do need to call checkButtonQueue() and should do so in a while-loop to receive all events since the last check.
 * This should be done in the Swadge Mode's main function.
 *
 * You may call getTouchCentroid() to get the analog touch position. This is independent of checkButtonQueue().
 *
 * \section btn_example Example
 *
 * \code{.c}
 * #include "hdw-btn.c"
 *
 * // Check all queued button events
 * buttonEvt_t evt;
 * while(checkButtonQueue(&evt))
 * {
 *     // Print the current event
 *     printf("state: %04X, button: %d, down: %s\n",
 *         evt.state, evt.button, evt.down ? "down" : "up");
 * }
 *
 * // Check if the touch area is touched, and print values if it is
 * int32_t centerVal, intensityVal;
 * if (getTouchCentroid(&centerVal, &intensityVal))
 * {
 *     printf("touch center: %lu, intensity: %lu\n", centerVal, intensityVal);
 * }
 * else
 * {
 *     printf("no touch\n");
 * }
 * \endcode
 */

//==============================================================================
// Includes
//==============================================================================

#include <stdio.h>
#include <string.h>

#include <esp_log.h>
#include <driver/gptimer.h>
#include <driver/dedic_gpio.h>
#include <driver/touch_sensor.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#include "hdw-btn.h"

//==============================================================================
// Defines
//==============================================================================

/// The number of samples kept in history to debounce buttons
#define DEBOUNCE_HIST_LEN 5

//==============================================================================
// Variables
//==============================================================================

/// A queue to move touch and push button reads from the ISR to the main loop
static QueueHandle_t btn_evt_queue = NULL;
/// The current state of the touch and push buttons
static uint32_t buttonStates = 0;
/// The current state of the touch buttons, used in btn_timer_isr_cb() and touchsensor_interrupt_cb()
static volatile uint32_t touchIsrState = 0;
/// The current state of the push buttons, used in btn_timer_isr_cb() and touchsensor_interrupt_cb()
static volatile uint32_t pushIsrState = 0;

/// A bundle of GPIOs to read as button input
static dedic_gpio_bundle_handle_t bundle = NULL;

/// The number of configured touchpads
static int numTouchPads;
/// A pointer to an array of configured touchpads
static touch_pad_t* touchPads;
/// A map between configured touch_pad_t and buttonBit_t
static buttonBit_t touchPadMap[TOUCH_PAD_MAX];
// Used in getBaseTouchVals() to get zeroed touch sensor values
static int32_t* baseOffsets = NULL;

//==============================================================================
// Prototypes
//==============================================================================

static void initPushButtons(gpio_num_t* pushButtons, uint8_t numPushButtons);
static bool btn_timer_isr_cb(gptimer_handle_t timer, const gptimer_alarm_event_data_t* edata, void* user_ctx);

static void initTouchSensor(touch_pad_t* _touchPads, uint8_t _numTouchPads, float touchPadSensitivity,
                            bool denoiseEnable);

static void touchsensor_interrupt_cb(void* arg);
static int getBaseTouchVals(int32_t* data, int count);
static int getTouchRawValues(uint32_t* rawvalues, int maxPads);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initialize both pushbuttons and touch buttons
 *
 * @param pushButtons A list of GPIOs with pushbuttons to initialize. The list should be in the same order as
 * ::buttonBit_t, starting at ::PB_UP
 * @param numPushButtons The number of pushbuttons to initialize
 * @param touchPads A list of touch buttons to initialize. The list should be in the same order as ::buttonBit_t,
 * starting at ::TB_0
 * @param numTouchPads The number of touch buttons to initialize
 */
void initButtons(gpio_num_t* pushButtons, uint8_t numPushButtons, touch_pad_t* touchPads, uint8_t numTouchPads)
{
    // create a queue to handle polling GPIO from ISR
    btn_evt_queue = xQueueCreate(3 * (numPushButtons + numTouchPads), sizeof(uint32_t));

    initPushButtons(pushButtons, numPushButtons);
    initTouchSensor(touchPads, numTouchPads, 0.2f, true);
}

/**
 * @brief Free memory used by the buttons
 */
void deinitButtons(void)
{
    vQueueDelete(btn_evt_queue);
    free(touchPads);
    free(baseOffsets);
}

/**
 * @brief Service the queue of button events that caused interrupts
 * This only reutrns a single event, even if there are multiple in the queue
 * This function may be called multiple times in a row to completely empty the queue
 *
 * @param evt If an event occurred, return it through this argument
 * @return true if an event occurred, false if nothing happened
 */
bool checkButtonQueue(buttonEvt_t* evt)
{
    // Check if there's an event to dequeue from the ISR
    uint32_t gpio_evt;
    while (xQueueReceive(btn_evt_queue, &gpio_evt, 0))
    {
        // Save the old state, set the new state
        uint32_t oldButtonStates = buttonStates;
        buttonStates             = gpio_evt;
        // If there was a change
        if (oldButtonStates != buttonStates)
        {
            // Figure out what the change was
            evt->button = oldButtonStates ^ buttonStates;
            evt->down   = (buttonStates > oldButtonStates);
            evt->state  = buttonStates;

            // Debug print
            // ESP_LOGE("BTN", "Bit 0x%02x was %s, buttonStates is %02x",
            //     evt->button,
            //     (evt->down) ? "pressed " : "released",
            //     evt->state);

            // Something happened
            return true;
        }
    }
    // Nothing happened
    return false;
}

//==============================================================================
// Pushbutton Functions
//==============================================================================

/**
 * @brief Initialize the given GPIOs as inputs for buttons
 * The GPIOs are polled on a hardware timer
 *
 * @param pushButtons A list of GPIOs to initialize as buttons
 * @param numPushButtons The number of GPIOs to initialize as buttons
 */
static void initPushButtons(gpio_num_t* pushButtons, uint8_t numPushButtons)
{
    ESP_LOGD("BTN", "initializing buttons");

    // Make sure there aren't too many
    if (numPushButtons > 31)
    {
        ESP_LOGE("BTN", "Too many buttons initialized (%d), max 31", numPushButtons);
        return;
    }

    for (uint8_t i = 0; i < numPushButtons; i++)
    {
        // Configure the GPIO
        gpio_config_t io_conf = {
            .mode         = GPIO_MODE_INPUT,
            .pull_up_en   = true,
            .pull_down_en = false,
        };
        io_conf.pin_bit_mask = 1ULL << pushButtons[i];
        gpio_config(&io_conf);
    }

    // Create bundle, input only
    dedic_gpio_bundle_config_t bundle_config =
    {
        .gpio_array = pushButtons,
        .array_size = numPushButtons,
        .flags = {
            .in_en = 1,
            .in_invert = 1,
            .out_en = 0,
            .out_invert = 0,
        },
    };
    ESP_ERROR_CHECK(dedic_gpio_new_bundle(&bundle_config, &bundle));

    // Get initial state
    buttonStates = dedic_gpio_bundle_read_in(bundle);

    // Initialize the timer
    gptimer_handle_t gptimer      = NULL;
    gptimer_config_t timer_config = {
        .clk_src       = GPTIMER_CLK_SRC_DEFAULT,
        .direction     = GPTIMER_COUNT_UP,
        .resolution_hz = 1000 * 1000, // 1MHz
    };
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer));

    gptimer_alarm_config_t config = {
        .alarm_count                = 1000, // Check every 1000 ticks of a 1MHz clock, i.e. every 1ms
        .reload_count               = 0,
        .flags.auto_reload_on_alarm = true,
    };
    ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer, &config));

    // Configure the ISR
    gptimer_event_callbacks_t callbacks = {
        .on_alarm = btn_timer_isr_cb,
    };
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(gptimer, &callbacks, NULL));

    // Start the timer
    ESP_ERROR_CHECK(gptimer_enable(gptimer));
    ESP_ERROR_CHECK(gptimer_start(gptimer));
}

/**
 * @brief Interrupt called by the hardware timer to poll buttons
 *
 * @param[in] timer Timer handle created by `gptimer_new_timer()`
 * @param[in] edata Alarm event data, fed by driver
 * @param[in] user_ctx User data, passed from `gptimer_register_event_callbacks()`
 * @return Whether a high priority task has been waken up by this function
 */
static bool IRAM_ATTR btn_timer_isr_cb(gptimer_handle_t timer, const gptimer_alarm_event_data_t* edata, void* user_ctx)
{
    static uint32_t evtHist[DEBOUNCE_HIST_LEN] = {0};
    static uint32_t evtIdx                     = 0;

    BaseType_t high_task_awoken = pdFALSE;

    // Read GPIOs
    uint32_t evt = dedic_gpio_bundle_read_in(bundle);

    // Store the event in a ring
    evtHist[evtIdx] = evt;
    evtIdx          = (evtIdx + 1) % DEBOUNCE_HIST_LEN;

    // Look for any difference in the debounce history
    for (int32_t ei = 0; ei < (DEBOUNCE_HIST_LEN - 1); ei++)
    {
        // Exclusive OR
        if (evtHist[ei] ^ evtHist[ei + 1])
        {
            // There is a difference, so return.
            // this is still debouncing
            return false;
        }
    }
    // No difference in the history, accept this input

    // Only queue changes
    if (pushIsrState != evt)
    {
        // save the event
        pushIsrState = evt;
        // Add the current touch state
        evt |= touchIsrState;
        // Queue this state from the ISR
        xQueueSendFromISR(btn_evt_queue, &evt, &high_task_awoken);
    }
    // return whether we need to yield at the end of ISR
    return high_task_awoken == pdTRUE;
}

//==============================================================================
// Touchpad Functions
//==============================================================================

/**
 * @brief Initialize touchpad sensors
 *
 * @param _touchPads A list of touchpads to initialize
 * @param _numTouchPads The number of touchpads to initialize
 * @param touchPadSensitivity The sensitivity to set for these touchpads
 * @param denoiseEnable true to denoise the input, false to use it raw
 */
static void initTouchSensor(touch_pad_t* _touchPads, uint8_t _numTouchPads, float touchPadSensitivity,
                            bool denoiseEnable)
{
    ESP_LOGD("TOUCH", "Initializing touch pad");

    /* Save the list of touchpads */
    if (NULL == touchPads)
    {
        numTouchPads = _numTouchPads;
        touchPads    = malloc(sizeof(touch_pad_t) * numTouchPads);
        memcpy(touchPads, _touchPads, (sizeof(touch_pad_t) * numTouchPads));
    }

    /* Initialize touch pad peripheral. */
    ESP_ERROR_CHECK(touch_pad_init());

    /* Initialie each touch pad */
    for (uint8_t i = 0; i < numTouchPads; i++)
    {
        ESP_ERROR_CHECK(touch_pad_config(touchPads[i]));
        /* Create a mapping from touch pad to button bit */
        touchPadMap[touchPads[i]] = (TB_0 << i);
    }

    /* Initialize denoise if requested */
    if (denoiseEnable)
    {
        /* Denoise setting at TouchSensor 0. */
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
    ESP_ERROR_CHECK(touch_pad_timeout_set(true, SOC_TOUCH_PAD_THRESHOLD_MAX));

    /* Register touch interrupt ISR, enable intr type. */
    ESP_ERROR_CHECK(touch_pad_isr_register(touchsensor_interrupt_cb, NULL, TOUCH_PAD_INTR_MASK_ALL));

    /* Enable interrupts, but not TOUCH_PAD_INTR_MASK_SCAN_DONE */
    ESP_ERROR_CHECK(
        touch_pad_intr_enable(TOUCH_PAD_INTR_MASK_ACTIVE | TOUCH_PAD_INTR_MASK_INACTIVE | TOUCH_PAD_INTR_MASK_TIMEOUT));

    /* Enable touch sensor clock. Work mode is "timer trigger" */
    ESP_ERROR_CHECK(touch_pad_set_fsm_mode(TOUCH_FSM_MODE_TIMER));
    ESP_ERROR_CHECK(touch_pad_fsm_start());

    /* Wait touch sensor init done */
    vTaskDelay(50 / portTICK_PERIOD_MS);

    /* Set thresholds */
    uint32_t touch_value;
    for (int i = 0; i < numTouchPads; i++)
    {
        /* read benchmark value */
        ESP_ERROR_CHECK(touch_pad_read_benchmark(touchPads[i], &touch_value));
        /* set interrupt threshold */
        ESP_ERROR_CHECK(touch_pad_set_thresh(touchPads[i], touch_value * touchPadSensitivity));
        ESP_LOGD("TOUCH", "touch pad [%d] base %lu, thresh %lu", touchPads[i], touch_value,
                 (uint32_t)(touch_value * touchPadSensitivity));
    }

    getTouchCentroid(0, 0);
}

/**
 * Handle an interrupt triggered when a pad is touched.
 * Recognize what pad has been touched and save it in a table.
 *
 * @param arg unused
 */
static void touchsensor_interrupt_cb(void* arg)
{
    uint32_t newTpState = touchIsrState;

    BaseType_t high_task_awoken = pdFALSE;

    uint32_t intr_mask = touch_pad_read_intr_status_mask();
    // uint32_t pad_status = touch_pad_get_status();
    buttonBit_t btnBit = touchPadMap[touch_pad_get_current_meas_channel()];
    // touch_ll_filter_read_smooth(touch_pad_t touch_num, uint32_t *smooth_data)

    /* Non-touch event statuses */
    if (intr_mask & TOUCH_PAD_INTR_MASK_SCAN_DONE)
    {
        // ESP_LOGI("TOUCH", "The touch sensor group measurement is done [%d].", isrEvt.pad_num);
        return;
    }
    else if (intr_mask & TOUCH_PAD_INTR_MASK_TIMEOUT)
    {
        /* Add your exception handling in here. */
        // ESP_LOGI("TOUCH", "Touch sensor channel %d measure timeout. Skip this exception channel!!", isrEvt.pad_num);
        // ESP_ERROR_CHECK(touch_pad_timeout_resume()); // Point on the next channel to measure.
        touch_pad_timeout_resume();
        return;
    }
    /* Handle events */
    else if (intr_mask & TOUCH_PAD_INTR_MASK_ACTIVE)
    {
        // ESP_LOGI("TOUCH", "TouchSensor [%d] be activated, status mask 0x%x", isrEvt.pad_num, isrEvt.pad_status);
        newTpState |= btnBit;
    }
    else if (intr_mask & TOUCH_PAD_INTR_MASK_INACTIVE)
    {
        // ESP_LOGI("TOUCH", "TouchSensor [%d] be inactivated, status mask 0x%x", isrEvt.pad_num, isrEvt.pad_status);
        newTpState &= ~btnBit;
    }
    else
    {
        /* Shouldn't happen */
        return;
    }

    /* If there is a state change, queue it from the ISR */
    if (touchIsrState != newTpState)
    {
        // save the event
        touchIsrState = newTpState;
        // Add the current touch state
        newTpState |= pushIsrState;
        // Queue this state from the ISR
        xQueueSendFromISR(btn_evt_queue, &newTpState, &high_task_awoken);
    }

    if (high_task_awoken == pdTRUE)
    {
        portYIELD_FROM_ISR();
    }
}

/**
 * @brief Get totally raw touch sensor values from buffer.
 * NOTE: You must have touch callbacks enabled to use this.
 *
 * @param data is a pointer to an array of int32_t's to receive the raw touch data.
 * @param count is the number of ints in your array.
 * @return is the number of values that were successfully read.
 */
static int getTouchRawValues(uint32_t* rawvalues, int maxPads)
{
    if (maxPads > numTouchPads)
    {
        maxPads = numTouchPads;
    }
    for (int i = 0; i < maxPads; i++)
    {
        // If any errors, abort.
        if (touch_pad_read_raw_data(touchPads[i], &rawvalues[i]))
        {
            return 0;
        }
    }
    return maxPads;
}

/**
 * @brief Get "zeroed" touch sensor values.
 * NOTE: You must have touch callbacks enabled to use this.
 *
 * @param data is a pointer to an array of int32_t's to receive the zeroed touch data.
 * @param count is the number of ints in your array.
 * @return is the number of values that were successfully read.
 */
static int getBaseTouchVals(int32_t* data, int count)
{
    uint32_t curVals[numTouchPads];
    if (getTouchRawValues(curVals, numTouchPads) == 0)
    {
        return 0;
    }
    for (int i = 0; i < numTouchPads; i++)
    {
        if (curVals[i] <= 0)
        {
            return 0;
        }
    }

    if (count > numTouchPads)
    {
        count = numTouchPads;
    }

    // curVals is valid.
    if (NULL == baseOffsets)
    {
        baseOffsets = malloc(sizeof(baseOffsets[0]) * numTouchPads);
        for (int i = 0; i < numTouchPads; i++)
        {
            baseOffsets[i] = curVals[i] << 8;
        }
    }

    for (int i = 0; i < numTouchPads; i++)
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
 * @brief Get totally raw touch sensor values from buffer.
 * NOTE: You must have touch callbacks enabled to use this.
 *
 * @param[out] centerVal pointer to centroid of touch locaiton from 0..1024 inclusive. Cannot be NULL.
 * @param[out] intensityVal intensity of touch press. Cannot be NULL.
 * @return true if touched (centroid), false if not touched (no centroid)
 */
bool getTouchCentroid(int32_t* centerVal, int32_t* intensityVal)
{
    int32_t baseVals[numTouchPads];
    if (!centerVal || !intensityVal || getBaseTouchVals(baseVals, numTouchPads) == 0)
    {
        return false;
    }

    int peak    = -1;
    int peakBin = -1;
    for (int i = 0; i < numTouchPads; i++)
    {
        int32_t bv = baseVals[i];
        if (bv > peak)
        {
            peak    = bv;
            peakBin = i;
        }
    }

    if (peakBin < 0)
    {
        return false;
    }
    // Arbitrary, but we use 1200 as the minimum peak value.
    if (peak < 1200)
    {
        return false;
    }
    // We know our peak bin, now we need to know the average and differential of the adjacent bins.
    int leftOfPeak  = (peakBin > 0) ? baseVals[peakBin - 1] : 0;
    int rightOfPeak = (peakBin < numTouchPads - 1) ? baseVals[peakBin + 1] : 0;

    int opeak  = peak;
    int center = peakBin << 8;

    if (rightOfPeak >= leftOfPeak)
    {
        // We bend upward (or are neutral)
        rightOfPeak -= leftOfPeak;
        peak -= leftOfPeak;
        center += (rightOfPeak << 8) / (rightOfPeak + peak);

        *intensityVal = opeak + rightOfPeak;
    }
    else
    {
        // We bend downward
        leftOfPeak -= rightOfPeak;
        peak -= rightOfPeak;
        center -= (leftOfPeak << 8) / (leftOfPeak + peak);

        *intensityVal = opeak + leftOfPeak;
    }
    *centerVal = center;
    return true;
}
