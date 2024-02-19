//==============================================================================
// Includes
//==============================================================================

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "driver/dac_continuous.h"

//==============================================================================
// Defines
//==============================================================================

#define AUDIO_SAMPLE_RATE_HZ 48000
#define BUF_SIZE             2048
#define DMA_DESCRIPTORS      8

//==============================================================================
// Variables
//==============================================================================

/** The handle created for the DAC */
static dac_continuous_handle_t dac_handle;

/** A queue to move dac_event_data_t from the interrupt to the main loop*/
static QueueHandle_t que;

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Callback for DAC conversion events
 *
 * @param handle [in] DAC channel handle, created from dac_continuous_new_channels()
 * @param event [in] DAC event data
 * @param user_data [in] User registered context, passed from dac_continuous_register_event_callback()
 * @return Whether a high priority task has been waken up by this callback function
 */
static bool IRAM_ATTR dac_on_convert_done_callback(dac_continuous_handle_t handle, const dac_event_data_t* event,
                                                   void* user_data)
{
    QueueHandle_t que = (QueueHandle_t)user_data;
    BaseType_t need_awoke;
    /* When the queue is full, drop the oldest item */
    if (xQueueIsQueueFullFromISR(que))
    {
        dac_event_data_t dummy;
        xQueueReceiveFromISR(que, &dummy, &need_awoke);
    }
    /* Send the event from callback */
    xQueueSendFromISR(que, event, &need_awoke);
    return need_awoke;
}

/**
 * @brief Initialize the DAC
 */
void dacInit(void)
{
    dac_continuous_config_t cont_cfg = {
        .chan_mask = DAC_CHANNEL_MASK_CH0,   // This is GPIO_NUM_17
        .desc_num  = DMA_DESCRIPTORS,        // The number of DMA descriptors
        .buf_size  = BUF_SIZE,               // The size of each MDA buffer
        .freq_hz   = AUDIO_SAMPLE_RATE_HZ,   // The frequency of DAC conversion
        .offset    = 0,                      // DC Offset automatically applied
        .clk_src   = DAC_DIGI_CLK_SRC_APB,   // APB is 77hz->MHz and always available
                                             // DAC_DIGI_CLK_SRC_APLL is 6Hz -> MHz but may be used by other peripherals
        .chan_mode = DAC_CHANNEL_MODE_SIMUL, // Doesn't matter for single channel output
    };

    /* Allocate continuous channels */
    ESP_ERROR_CHECK(dac_continuous_new_channels(&cont_cfg, &dac_handle));

    /* Create a queue to transport the interrupt event data */
    que = xQueueCreate(10, sizeof(dac_event_data_t));

    /* Register callbacks for conversion events */
    dac_event_callbacks_t cbs = {
        .on_convert_done = dac_on_convert_done_callback,
        .on_stop         = NULL,
    };
    ESP_ERROR_CHECK(dac_continuous_register_event_callback(dac_handle, &cbs, que));

    /* Enable and start the continuous channels */
    ESP_ERROR_CHECK(dac_continuous_enable(dac_handle));
    ESP_ERROR_CHECK(dac_continuous_start_async_writing(dac_handle));
}

/**
 * @brief Poll the queue to see if it needs to be filled with audio samples
 */
void dacPoll(void)
{
    /* If there is an event to receive, receive it */
    dac_event_data_t evt_data;
    if (xQueueReceive(que, &evt_data, 0))
    {
        /* Generate a sawtooth on the fly */
        static uint8_t sawtooth[BUF_SIZE];
        for (int32_t i = 0; i < BUF_SIZE; i++)
        {
            sawtooth[i] = (i * 255) / BUF_SIZE;
        }

        /* Write the data to the DAC */
        size_t loaded_bytes = 0;
        dac_continuous_write_asynchronously(dac_handle, evt_data.buf, evt_data.buf_size, //
                                            sawtooth, sizeof(sawtooth), &loaded_bytes);
    }
}
