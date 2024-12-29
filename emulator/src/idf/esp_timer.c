//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include <time.h>

#include "linked_list.h"

#include "esp_timer.h"
#include "esp_log.h"
#include "esp_timer_emu.h"
#include "esp_heap_caps.h"

//==============================================================================
// Variables
//==============================================================================

static list_t* timerList                 = NULL;
static unsigned long boot_time_in_micros = 0;
static unsigned long pause_start_micros  = 0;
static unsigned long total_pause_micros  = 0;
static bool useRealTime                  = true;
static int64_t fakeTime                  = 0;

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Set the time of 'boot'
 *
 * @return ESP_OK
 */
esp_err_t esp_timer_init(void)
{
    // Log when the program starts
    struct timespec ts;
    if (0 != clock_gettime(CLOCK_MONOTONIC, &ts))
    {
        ESP_LOGE("EMU", "Clock err");
        return 0;
    }
    boot_time_in_micros = (ts.tv_sec * 1000000) + (ts.tv_nsec / 1000);

    // Create an empty list of timers
    timerList = calloc(1, sizeof(list_t));

    return ESP_OK;
}

/**
 * @brief De-initialize esp_timer library
 *
 * @note Normally this function should not be called from applications
 *
 * @return
 *      - ESP_OK on success
 *      - ESP_ERR_INVALID_STATE if not yet initialized
 */
esp_err_t esp_timer_deinit(void)
{
    if (timerList)
    {
        void* val;
        while (NULL != (val = shift(timerList)))
        {
            free(val);
        }

        clear(timerList);
        free(timerList);
        return ESP_OK;
    }
    return ESP_ERR_INVALID_STATE;
}

/**
 * @brief Get the time since 'boot' in microseconds
 *
 * @return the time since 'boot' in microseconds
 */
int64_t esp_timer_get_time(void)
{
    if (useRealTime)
    {
        if (pause_start_micros > 0)
        {
            // pretend time stopped after pause_start_micros if it's set
            return pause_start_micros;
        }

        struct timespec ts;
        if (0 != clock_gettime(CLOCK_MONOTONIC, &ts))
        {
            ESP_LOGE("EMU", "Clock err");
            return 0;
        }
        // Return the real time minus adjustment for any previous pauses
        return ((ts.tv_sec * 1000000) + (ts.tv_nsec / 1000)) - boot_time_in_micros - total_pause_micros;
    }
    else
    {
        return fakeTime;
    }
}

/**
 * @brief Create an esp_timer instance
 *
 * @note When done using the timer, delete it with esp_timer_delete function.
 *
 * @param create_args   Pointer to a structure with timer creation arguments.
 *                      Not saved by the library, can be allocated on the stack.
 * @param[out] out_handle  Output, pointer to esp_timer_handle_t variable which
 *                         will hold the created timer handle.
 *
 * @return
 *      - ESP_OK on success
 *      - ESP_ERR_INVALID_ARG if some of the create_args are not valid
 *      - ESP_ERR_INVALID_STATE if esp_timer library is not initialized yet
 *      - ESP_ERR_NO_MEM if memory allocation fails
 */
esp_err_t esp_timer_create(const esp_timer_create_args_t* create_args, esp_timer_handle_t* out_handle)
{
    if (NULL == *out_handle)
    {
        // Allocate memory for a timer
        (*out_handle) = (esp_timer_handle_t)heap_caps_calloc(1, sizeof(struct esp_timer), MALLOC_CAP_8BIT);
    }

    // Initialize the timer
    (*out_handle)->callback = create_args->callback;
    (*out_handle)->arg      = create_args->arg;
    (*out_handle)->alarm    = 0;
    (*out_handle)->period   = 0;
    if (create_args->skip_unhandled_events)
    {
        (*out_handle)->flags |= FL_SKIP_UNHANDLED_EVENTS;
    }

    // #if WITH_PROFILING
    //     (*out_handle)->name = create_args->name;
    // #endif

#ifdef CONFIG_ESP_TIMER_SUPPORTS_ISR_DISPATCH_METHOD
    if (ESP_TIMER_ISR == create_args->dispatch_method)
    {
        (*out_handle)->flags |= FL_ISR_DISPATCH_METHOD;
    }
#endif

    // Link the node
    push(timerList, *out_handle);

    return ESP_OK;
}

/**
 * @brief Delete an esp_timer instance
 *
 * The timer must be stopped before deleting. A one-shot timer which has expired
 * does not need to be stopped.
 *
 * @param timer timer handle allocated using esp_timer_create
 * @return
 *      - ESP_OK on success
 *      - ESP_ERR_INVALID_STATE if the timer is running
 */
esp_err_t esp_timer_delete(const esp_timer_handle_t timer)
{
    for (node_t* node = timerList->first; NULL != node; node = node->next)
    {
        if (node->val == timer)
        {
            heap_caps_free(node->val);
            removeEntry(timerList, node);
            break;
        }
    }

    return ESP_OK;
}

/**
 * @brief Stop the timer
 *
 * This function stops the timer previously started using esp_timer_start_once
 * or esp_timer_start_periodic.
 *
 * @param timer timer handle created using esp_timer_create
 * @return
 *      - ESP_OK on success
 *      - ESP_ERR_INVALID_STATE if the timer is not running
 */
esp_err_t esp_timer_stop(esp_timer_handle_t timer)
{
    timer->alarm  = 0;
    timer->period = 0;
    return ESP_OK;
}

/**
 * @brief Start one-shot timer
 *
 * Timer should not be running when this function is called.
 *
 * @param timer timer handle created using esp_timer_create
 * @param timeout_us timer timeout, in microseconds relative to the current moment
 * @return
 *      - ESP_OK on success
 *      - ESP_ERR_INVALID_ARG if the handle is invalid
 *      - ESP_ERR_INVALID_STATE if the timer is already running
 */
esp_err_t esp_timer_start_once(esp_timer_handle_t timer, uint64_t timeout_us)
{
    timer->alarm  = timeout_us;
    timer->period = 0;
    return ESP_OK;
}

/**
 * @brief Start a periodic timer
 *
 * Timer should not be running when this function is called. This function will
 * start the timer which will trigger every 'period' microseconds.
 *
 * @param timer timer handle created using esp_timer_create
 * @param period timer period, in microseconds
 * @return
 *      - ESP_OK on success
 *      - ESP_ERR_INVALID_ARG if the handle is invalid
 *      - ESP_ERR_INVALID_STATE if the timer is already running
 */
esp_err_t esp_timer_start_periodic(esp_timer_handle_t timer, uint64_t period)
{
    timer->alarm  = period;
    timer->period = period;
    return ESP_OK;
}

/**
 * @brief Check running timers and call any that expire
 *
 * @param elapsed_us The elapsed time in microseconds since this was last called
 */
void check_esp_timer(uint64_t elapsed_us)
{
    if (0 == timerList->length)
    {
        // Nothing linked, so return
        return;
    }

    for (node_t* node = timerList->first; NULL != node; node = node->next)
    {
        bool timerExpired      = false;
        esp_timer_handle_t tmr = node->val;
        if (tmr->alarm > elapsed_us)
        {
            tmr->alarm -= elapsed_us;

            if (0 == tmr->alarm)
            {
                // If it decrements to 0 exactly, expire
                timerExpired = true;
            }
        }
        else if (tmr->alarm)
        {
            // If there's less time on left than elapsed, expire
            timerExpired = true;
        }

        if (timerExpired)
        {
            // Call the callback
            tmr->callback(tmr->arg);
            // Reset the timer if periodic
            if (tmr->period)
            {
                tmr->alarm = tmr->period;
            }
        }
    }
}

void emuSetUseRealTime(bool val)
{
    useRealTime = val;
}

void emuSetEspTimerTime(int64_t time)
{
    fakeTime = time;
}

/**
 * @brief Pause the emulator's timer. When paused, time will reman frozen until emuTimerUnpause() is called.
 *
 * The main purpose of this is to allow pausing the emulator for a long time without accumulating clock drift
 */
void emuTimerPause(void)
{
    if (pause_start_micros == 0)
    {
        pause_start_micros = esp_timer_get_time();
    }
}

/**
 * @brief Unpause the emulator's timer. When unpaused, time will resume from when emuTimerPause() was called
 *
 */
void emuTimerUnpause(void)
{
    if (pause_start_micros > 0)
    {
        unsigned long start_micros = pause_start_micros;
        pause_start_micros         = 0;

        total_pause_micros += (esp_timer_get_time() - start_micros);
    }
}

/**
 * @brief Returns whether or not the emulator timer is currently paused
 *
 * @return true If the emulator timer is paused
 * @return false If the emulator timer is not paused
 */
bool emuTimerIsPaused(void)
{
    return (pause_start_micros > 0);
}
