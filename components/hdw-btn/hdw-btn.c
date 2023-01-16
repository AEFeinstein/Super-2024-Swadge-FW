/*! \file hdw-btn.c
 *
 * TODO Explain how to use buttons!
 */

//==============================================================================
// Includes
//==============================================================================

#include <stdio.h>

#include "esp_log.h"
#include "driver/gptimer.h"
#include "driver/dedic_gpio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "hdw-btn.h"

//==============================================================================
// Defines
//==============================================================================

/// The number of samples kept in history to debounce buttons
#define DEBOUNCE_HIST_LEN 5

//==============================================================================
// Prototypes
//==============================================================================

static bool btn_timer_isr_cb(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx);

//==============================================================================
// Variables
//==============================================================================

/// A bundle of GPIOs to read as button input
static dedic_gpio_bundle_handle_t bundle = NULL;

/// A queue to move button reads from the ISR to the main loop
static QueueHandle_t gpio_evt_queue = NULL;

/// The current state of the buttons
static uint32_t buttonStates = 0;

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initialize the given GPIOs as inputs for buttons
 * The GPIOs are polled on a hardware timer
 *
 * @param numButtons The number of GPIOs to initialize as buttons
 * @param ... A list of GPIOs to initialize as buttons
 */
void initButtons(uint8_t numButtons, ...)
{
    ESP_LOGD("BTN", "initializing buttons");

    // Make sure there aren't too many
    if(numButtons > 31)
    {
        ESP_LOGE("BTN", "Too many buttons initialized (%d), max 31", numButtons);
        return;
    }

    // Make a list of all the button GPIOs
    int bundle_gpios[numButtons];

    // For each GPIO
    va_list ap;
    va_start(ap, numButtons);
    for(uint8_t i = 0; i < numButtons; i++)
    {
        // Get the GPIO, put it in a bundle
        bundle_gpios[i] = va_arg(ap, gpio_num_t);

        // Configure the GPIO
        gpio_config_t io_conf =
        {
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = true,
            .pull_down_en = false,
        };
        io_conf.pin_bit_mask = 1ULL << bundle_gpios[i];
        gpio_config(&io_conf);
    }
    va_end(ap);

    // Create bundle, input only
    dedic_gpio_bundle_config_t bundle_config =
    {
        .gpio_array = bundle_gpios,
        .array_size = numButtons,
        .flags = {
            .in_en = 1,
            .in_invert = 1,
            .out_en = 0,
            .out_invert = 0
        }
    };
    ESP_ERROR_CHECK(dedic_gpio_new_bundle(&bundle_config, &bundle));

    // Get initial state
    buttonStates = dedic_gpio_bundle_read_in(bundle);

    // create a queue to handle polling GPIO from ISR
    gpio_evt_queue = xQueueCreate(64, sizeof(uint32_t));

    // Initialize the timer
    gptimer_handle_t gptimer = NULL;
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000 * 1000, // 1MHz
    };
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer));

    gptimer_alarm_config_t config = {
        .alarm_count = 1000,
        .reload_count = 0,
        .flags.auto_reload_on_alarm = true
    };
    ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer, &config));

    // Configure the ISR
    gptimer_event_callbacks_t callbacks = {
        .on_alarm = btn_timer_isr_cb
    };
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(gptimer, &callbacks, NULL));

    // Start the timer
    ESP_ERROR_CHECK(gptimer_enable(gptimer));
    ESP_ERROR_CHECK(gptimer_start(gptimer));
}

/**
 * @brief Free memory used by the buttons
 */
void deinitButtons(void)
{
    // Nothing allocated
}

/**
 * @brief Interrupt called by the hardware timer to poll buttons
 *
 * @param[in] timer Timer handle created by `gptimer_new_timer()`
 * @param[in] edata Alarm event data, fed by driver
 * @param[in] user_ctx User data, passed from `gptimer_register_event_callbacks()`
 * @return Whether a high priority task has been waken up by this function
 */
static bool IRAM_ATTR btn_timer_isr_cb(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx)
{
    // Static variable lives forever!
    static uint32_t lastEvt = 0;
    static uint32_t evtHist[DEBOUNCE_HIST_LEN] = {0};
    static uint32_t evtIdx = 0;

    BaseType_t high_task_awoken = pdFALSE;

    // Read GPIOs
    uint32_t evt = dedic_gpio_bundle_read_in(bundle);

    // Store the event in a ring
    evtHist[evtIdx] = evt;
    evtIdx = (evtIdx + 1) % DEBOUNCE_HIST_LEN;

    // Look for any difference in the debounce history
    for(int32_t ei = 0; ei < (DEBOUNCE_HIST_LEN - 1); ei++)
    {
        // Exclusive OR
        if(evtHist[ei] ^ evtHist[ei + 1])
        {
            // There is a difference, so return.
            // this is still debouncing
            return false;
        }
    }
    // No difference in the history, accept this input

    // Only queue changes
    if(lastEvt != evt)
    {
        xQueueSendFromISR(gpio_evt_queue, &evt, &high_task_awoken);
        // save the event
        lastEvt = evt;
    }
    // return whether we need to yield at the end of ISR
    return high_task_awoken == pdTRUE;
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
    while (xQueueReceive(gpio_evt_queue, &gpio_evt, 0))
    {
        // Save the old state, set the new state
        uint32_t oldButtonStates = buttonStates;
        buttonStates = gpio_evt;
        // If there was a change
        if(oldButtonStates != buttonStates)
        {
            // Figure out what the change was
            evt->button = oldButtonStates ^ buttonStates;
            evt->down = (buttonStates > oldButtonStates);
            evt->state = buttonStates;

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
