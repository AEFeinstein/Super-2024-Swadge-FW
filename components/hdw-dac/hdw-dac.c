//==============================================================================
// Includes
//==============================================================================

#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "hdw-dac.h"

//==============================================================================
// Defines
//==============================================================================

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

/** The GPIO which controls amplifier shutdown */
static gpio_num_t shdnGpio;

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
 * @param channel The output channel (pin) for the ADC
 * @param shdn_gpio The GPIO that controls the amplifier's shutdown
 * @param cb A callback function which will be called to request samples from the application
 */
void initDac(dac_channel_mask_t channel, gpio_num_t shdn_gpio, fnDacCallback_t cb)
{
    /* Save the callback */
    dacCb = cb;

    /* Configure the DAC */
    dac_continuous_config_t cont_cfg = {
        .chan_mask = channel,                // DAC_CHANNEL_MASK_CH0 is GPIO_NUM_17, DAC_CHANNEL_MASK_CH1 is GPIO_NUM_18
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

    /* Initialize the GPIO of shutdown pin */
    shdnGpio                       = shdn_gpio;
    gpio_config_t shdn_gpio_config = {
        .mode         = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << shdn_gpio,
    };
    ESP_ERROR_CHECK(gpio_config(&shdn_gpio_config));
    ESP_ERROR_CHECK(gpio_set_level(shdn_gpio, 0));
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
    if (NULL != dacIsrQueue)
    {
        /* If there is an event to receive, receive it */
        dac_event_data_t evt_data;
        while (xQueueReceive(dacIsrQueue, &evt_data, 0))
        {
            /* Ask the application to fill a buffer */
            dacCb(tmpDacBuf, evt_data.buf_size);

            /* Write the data DMA so that it is sent out the DAC */
            size_t loaded_bytes = 0;
            dac_continuous_write_asynchronously(dac_handle, evt_data.buf, evt_data.buf_size, //
                                                tmpDacBuf, evt_data.buf_size, &loaded_bytes);
            /* assume loaded_bytes == DAC_BUF_SIZE */
        }
    }
}

/**
 * @brief Set the shutdown state of the DAC
 *
 * @param shutdown true to shut down the DAC, false to enable it
 */
void setDacShutdown(bool shutdown)
{
    if (shutdown)
    {
        ESP_ERROR_CHECK(gpio_set_level(shdnGpio, 1));
        dacStop();
    }
    else
    {
        ESP_ERROR_CHECK(gpio_set_level(shdnGpio, 0));
        dacStart();
    }
}