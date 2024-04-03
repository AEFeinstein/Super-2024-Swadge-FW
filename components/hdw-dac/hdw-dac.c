//==============================================================================
// Includes
//==============================================================================

#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "driver/dac_continuous.h"
#include "hdw-dac.h"

//==============================================================================
// Defines
//==============================================================================

/** The size of each buffer to fill with DAC samples */
#define DAC_BUF_SIZE 2048

/** The number of buffers to use. The more buffers, the longer latency */
#define DMA_DESCRIPTORS 4

//==============================================================================
// Variables
//==============================================================================

/** The handle created for the DAC */
static dac_continuous_handle_t dac_handle = NULL;

/** A queue to move dac_event_data_t from the interrupt to the main loop*/
static QueueHandle_t dacIsrQueue = NULL;

/** A callback which will request DAC samples from the application */
static fnDacCallback_t dacCb = NULL;

/** A temporary buffer for the application to fill with audio samples before */
static uint8_t tmpDacBuf[DAC_BUF_SIZE] = {0};

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
    QueueHandle_t queue = (QueueHandle_t)user_data;
    BaseType_t need_awoke;
    /* When the queue is full, drop the oldest item */
    if (xQueueIsQueueFullFromISR(queue))
    {
        dac_event_data_t dummy;
        xQueueReceiveFromISR(queue, &dummy, &need_awoke);
    }
    /* Send the event from callback */
    xQueueSendFromISR(queue, event, &need_awoke);
    return need_awoke;
}

/**
 * @brief Initialize the DAC
 *
 * @param cb A callback function which will be called to request samples from the application
 */
void initDac(fnDacCallback_t cb)
{
    /* Save the callback */
    dacCb = cb;

    /* Configure the DAC */
    dac_continuous_config_t cont_cfg = {
        .chan_mask = DAC_CHANNEL_MASK_CH0,   // This is GPIO_NUM_17
        .desc_num  = DMA_DESCRIPTORS,        // The number of DMA descriptors
        .buf_size  = DAC_BUF_SIZE,           // The size of each MDA buffer
        .freq_hz   = DAC_SAMPLE_RATE_HZ,     // The frequency of DAC conversion
        .offset    = 0,                      // DC Offset automatically applied
        .clk_src   = DAC_DIGI_CLK_SRC_APB,   // DAC_DIGI_CLK_SRC_APB is 77hz->MHz and always available
                                             // DAC_DIGI_CLK_SRC_APLL is 6Hz -> MHz but may be used by other peripherals
        .chan_mode = DAC_CHANNEL_MODE_SIMUL, // Doesn't matter for single channel output
    };

    /* Allocate continuous channels */
    ESP_ERROR_CHECK(dac_continuous_new_channels(&cont_cfg, &dac_handle));

    /* Create a queue to transport the interrupt event data */
    dacIsrQueue = xQueueCreate(DMA_DESCRIPTORS, sizeof(dac_event_data_t));

    /* Register callbacks for conversion events */
    dac_event_callbacks_t cbs = {
        .on_convert_done = dac_on_convert_done_callback,
        .on_stop         = NULL,
    };
    ESP_ERROR_CHECK(dac_continuous_register_event_callback(dac_handle, &cbs, dacIsrQueue));
}

/**
 * @brief Deinitialize the DAC and free memory
 */
void deinitDac(void)
{
    /* Stop the DAC */
    dacStop();

    /* Free resources */
    ESP_ERROR_CHECK(dac_continuous_del_channels(dac_handle));
    vQueueDelete(dacIsrQueue);

    /* NULL the callback */
    dacCb = NULL;
}

/**
 * @brief Start the DAC. This will cause samples to be requested from the application.
 */
void dacStart(void)
{
    /* Enable and start the continuous channels */
    ESP_ERROR_CHECK(dac_continuous_enable(dac_handle));
    ESP_ERROR_CHECK(dac_continuous_start_async_writing(dac_handle));
}

/**
 * @brief Stop the DAC.
 */
void dacStop(void)
{
    /* Stop and disable the continuous channels */
    ESP_ERROR_CHECK(dac_continuous_stop_async_writing(dac_handle));
    ESP_ERROR_CHECK(dac_continuous_disable(dac_handle));
}

/**
 * @brief Poll the queue to see if any buffers need to be filled with audio samples
 */
void dacPoll(void)
{
    /* If there is an event to receive, receive it */
    dac_event_data_t evt_data;
    while (xQueueReceive(dacIsrQueue, &evt_data, 0))
    {
        /* Ask the application to fill a buffer */
        dacCb(&tmpDacBuf, evt_data.buf_size);

        /* Write the data DMA so that it is sent out the DAC */
        size_t loaded_bytes = 0;
        dac_continuous_write_asynchronously(dac_handle, evt_data.buf, evt_data.buf_size, //
                                            tmpDacBuf, evt_data.buf_size, &loaded_bytes);
        /* assume loaded_bytes == DAC_BUF_SIZE */
    }
}
