//==============================================================================
// Includes
//==============================================================================

#include <stdio.h>
#include <string.h>

#include <esp_log.h>
#include <esp_timer.h>
#include <driver/gptimer.h>
#include <driver/dedic_gpio.h>
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
// Structs
//==============================================================================

/// @brief A timestamped button event
typedef struct
{
    int32_t state; /// The button state
    int32_t time;  /// The timestamp for this state
} timedEvt_t;

//==============================================================================
// Variables
//==============================================================================

/// A queue to move pushbutton reads from the ISR to the main loop
static QueueHandle_t btn_evt_queue = NULL;
/// The current state of the pushbuttons
static uint32_t buttonStates = 0;
/// The current state of the pushbuttons, used in btn_timer_isr_cb()
static volatile uint32_t pushIsrState = 0;

/// The number of buttons
static uint8_t _numPushButtons = 0;
/// A pointer to an array of GPIOs used for buttons
static const gpio_num_t* _pushButtons = NULL;

/// A bundle of GPIOs to read as button input
static dedic_gpio_bundle_handle_t bundle = NULL;

/// Timer handle used to periodically poll buttons
static gptimer_handle_t btnTimer = NULL;

//==============================================================================
// Prototypes
//==============================================================================

static bool btn_timer_isr_cb(gptimer_handle_t timer, const gptimer_alarm_event_data_t* edata, void* user_ctx);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initialize GPIO pushbuttons
 *
 * @param pushButtons A list of GPIOs with pushbuttons to initialize. The list should be in the same order as
 * ::buttonBit_t, starting at ::PB_UP
 * @param numPushButtons The number of pushbuttons to initialize
 */
void initButtons(const gpio_num_t* pushButtons, uint8_t numPushButtons)
{
    // Make sure there aren't too many
    if (numPushButtons > 31)
    {
        ESP_LOGE("BTN", "Too many buttons initialized (%d), max 31", numPushButtons);
        return;
    }

    ESP_LOGD("BTN", "initializing buttons");

    // create a queue to handle polling GPIO from ISR
    btn_evt_queue = xQueueCreate(3 * (numPushButtons), sizeof(timedEvt_t));

    // Save for init & deinit later
    if (NULL != pushButtons)
    {
        _pushButtons    = pushButtons;
        _numPushButtons = numPushButtons;
    }

    // Configure each GPIO
    for (uint8_t i = 0; i < numPushButtons; i++)
    {
        // Configure the GPIO
        gpio_config_t io_conf = {
            .pin_bit_mask = 1ULL << _pushButtons[i],
            .mode         = GPIO_MODE_INPUT,
            .pull_up_en   = true,
            .pull_down_en = false,
            .intr_type    = GPIO_INTR_DISABLE,
        };
        gpio_config(&io_conf);
    }

    // Set up the GPIO bundle and timer
    powerUpButtons();
}

/**
 * @brief Free memory used by the buttons
 */
void deinitButtons(void)
{
    powerDownButtons();

    vQueueDelete(btn_evt_queue);
}

/**
 * @brief Power down the GPIO pushbuttons
 */
void powerDownButtons(void)
{
    // Disable button timer and GPIO bundle
    ESP_ERROR_CHECK(gptimer_stop(btnTimer));
    ESP_ERROR_CHECK(gptimer_disable(btnTimer));
    ESP_ERROR_CHECK(gptimer_del_timer(btnTimer));
    ESP_ERROR_CHECK(dedic_gpio_del_bundle(bundle));
}

/**
 * @brief Power up the GPIO pushbuttons
 */
void powerUpButtons(void)
{
    // Create bundle, input only
    dedic_gpio_bundle_config_t bundle_config =
    {
        .gpio_array = _pushButtons,
        .array_size = _numPushButtons,
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
    gptimer_config_t timer_config = {
        .clk_src       = GPTIMER_CLK_SRC_DEFAULT,
        .direction     = GPTIMER_COUNT_UP,
        .resolution_hz = 1000 * 1000, // 1MHz
    };
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &btnTimer));

    gptimer_alarm_config_t config = {
        .alarm_count                = 1000, // Check every 1000 ticks of a 1MHz clock, i.e. every 1ms
        .reload_count               = 0,
        .flags.auto_reload_on_alarm = true,
    };
    ESP_ERROR_CHECK(gptimer_set_alarm_action(btnTimer, &config));

    // Configure the ISR
    gptimer_event_callbacks_t callbacks = {
        .on_alarm = btn_timer_isr_cb,
    };
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(btnTimer, &callbacks, NULL));

    // Start the timer
    ESP_ERROR_CHECK(gptimer_enable(btnTimer));
    ESP_ERROR_CHECK(gptimer_start(btnTimer));
}

/**
 * @brief Service the queue of button events that caused interrupts
 * This only returns a single event, even if there are multiple in the queue
 * This function may be called multiple times in a row to completely empty the queue
 *
 * @param evt If an event occurred, return it through this argument
 * @return true if an event occurred, false if nothing happened
 */
bool checkButtonQueue(buttonEvt_t* evt)
{
    // Check if there's an event to dequeue from the ISR
    timedEvt_t gpio_evt;
    while (xQueueReceive(btn_evt_queue, &gpio_evt, 0))
    {
        // Save the old state, set the new state
        uint32_t oldButtonStates = buttonStates;
        buttonStates             = gpio_evt.state;
        // If there was a change
        if (oldButtonStates != buttonStates)
        {
            // Figure out what the change was
            evt->button = oldButtonStates ^ buttonStates;
            evt->down   = (buttonStates > oldButtonStates);
            evt->state  = buttonStates;
            evt->time   = gpio_evt.time;

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

        timedEvt_t tEvt = {
            .state = evt,
            .time  = esp_timer_get_time(),
        };

        // Queue this state from the ISR
        xQueueSendFromISR(btn_evt_queue, &tEvt, &high_task_awoken);
    }
    // return whether we need to yield at the end of ISR
    return high_task_awoken == pdTRUE;
}
