//==============================================================================
// Includes
//==============================================================================

#include <inttypes.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "hdw-dac.h"

#if SOC_DAC_SUPPORTED
#include "driver/dac_continuous.h"
#else
#include "driver/i2s_pdm.h"
#endif

//==============================================================================
// Defines
//==============================================================================

/** The number of buffers to use. The more buffers, the longer latency */
#define DMA_DESCRIPTORS 4

//==============================================================================
// Variables
//==============================================================================

/** A callback which will request DAC samples from the application */
static fnDacCallback_t dacCb = NULL;

#if SOC_DAC_SUPPORTED

/** The handle created for the DAC */
static dac_continuous_handle_t dac_handle = NULL;

/** A queue to move dac_event_data_t from the interrupt to the main loop*/
static QueueHandle_t dacIsrQueue = NULL;

/** A temporary buffer for the application to fill with audio samples before */
static uint8_t tmpDacBuf[DAC_BUF_SIZE] = {0};

/** The GPIO which controls amplifier shutdown */
static gpio_num_t shdnGpio = GPIO_NUM_NC;

/** Keep track of if the DAC is writing or not */
static bool dacWriting = false;

#else

static i2s_chan_handle_t                dac_chan;        // I2S tx channel handler

#endif


//==============================================================================
// Functions
//==============================================================================

#if SOC_DAC_SUPPORTED
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

#else

static bool IRAM_ATTR i2s_isr_callback(i2s_chan_handle_t handle, i2s_event_data_t *event, void *user_ctx)
{
	uint8_t samples[event->size];
	if( dacCb )
		dacCb( samples, sizeof(samples) );
	else
		memset( samples, 0x7f, sizeof(samples) );

	// Do not block.
	size_t writ; // Discarded
	ESP_ERROR_CHECK(i2s_channel_write( dac_chan, samples, sizeof(samples), &writ, 0));
	return true; // claim high prio task was woken.  XXX TODO: Explore.
}

#endif

/**
 * @brief Initialize the DAC
 *
 * @param spk_gpio The output pin for the speaker (DAC or PDM)
 * @param shdn_gpio The GPIO that controls the amplifier's shutdown
 * @param cb A callback function which will be called to request samples from the application
 */
void initDac(gpio_num_t spk_gpio, gpio_num_t shdn_gpio, fnDacCallback_t cb)
{
#if SOC_DAC_SUPPORTED
    // Both DAC and PDM use the amplifier's shutdown pin
    if (GPIO_NUM_NC == shdnGpio)
    {
        /* Initialize the GPIO of shutdown pin */
        shdnGpio                       = shdn_gpio;
        gpio_config_t shdn_gpio_config = {
            .mode         = GPIO_MODE_OUTPUT,
            .pin_bit_mask = 1ULL << shdn_gpio,
        };
        ESP_ERROR_CHECK(gpio_config(&shdn_gpio_config));
        ESP_ERROR_CHECK(gpio_set_level(shdn_gpio, 0));
    }

    // If the DAC isn't initialized
    if (!dac_handle)
    {
        /* Save the callback */
        dacCb = cb;

        /* Pick the channel from the GPIO */
        dac_channel_mask_t channel = 0;
        if (GPIO_NUM_17 == spk_gpio)
        {
            channel = DAC_CHANNEL_MASK_CH0;
        }
        else if (GPIO_NUM_18 == spk_gpio)
        {
            channel = DAC_CHANNEL_MASK_CH1;
        }
        else
        {
            // Not a valid configuration
            return;
        }

        /* Configure the DAC */
        dac_continuous_config_t cont_cfg = {
            .chan_mask = channel,            // DAC_CHANNEL_MASK_CH0 is GPIO_NUM_17, DAC_CHANNEL_MASK_CH1 is GPIO_NUM_18
            .desc_num  = DMA_DESCRIPTORS,    // The number of DMA descriptors
            .buf_size  = DAC_BUF_SIZE,       // The size of each MDA buffer
            .freq_hz   = DAC_SAMPLE_RATE_HZ, // The frequency of DAC conversion
            .offset    = 0,                  // DC Offset automatically applied
            .clk_src   = DAC_DIGI_CLK_SRC_APB, // DAC_DIGI_CLK_SRC_APB is 77hz->MHz and always available
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
#else

    i2s_chan_config_t tx_chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
	ESP_ERROR_CHECK(i2s_new_channel(&tx_chan_cfg, &dac_chan, NULL));

    i2s_pdm_tx_config_t tx_pdm_cfg = {
        .clk_cfg  = I2S_PDM_TX_CLK_DAC_DEFAULT_CONFIG(DAC_SAMPLE_RATE_HZ), //TODO: *128 maybe?
        .slot_cfg = I2S_PDM_TX_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_8BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .clk  = GPIO_NUM_NC,
            .dout2= GPIO_NUM_NC,
            .dout = spk_gpio,
            .invert_flags = { 0 },
        },
    };

    ESP_ERROR_CHECK(i2s_channel_init_pdm_tx_mode(dac_chan, &tx_pdm_cfg));

    dacCb = cb;

	i2s_event_callbacks_t ecb = {
		.on_sent = i2s_isr_callback
	};

	ESP_ERROR_CHECK(i2s_channel_register_event_callback(dac_chan, &ecb, 0 ));


#endif
}

/**
 * @brief Deinitialize the DAC and free memory
 */
void deinitDac(void)
{
#if SOC_DAC_SUPPORTED
    if (dac_handle)
    {
        /* Stop the DAC */
        dacStop();

        /* Free resources */
        ESP_ERROR_CHECK(dac_continuous_del_channels(dac_handle));
        dac_handle = NULL;
        vQueueDelete(dacIsrQueue);
        dacIsrQueue = NULL;

        /* NULL the callback */
        dacCb = NULL;
    }
#else
	i2s_del_channel(dac_chan);
#endif
}

/**
 * @brief Power down the DAC (speaker)
 */
void powerDownDac(void)
{
    setDacShutdown(true);
}

/**
 * @brief Power up the DAC (speaker)
 */
void powerUpDac(void)
{
    setDacShutdown(false);
}

/**
 * @brief Start the DAC. This will cause samples to be requested from the application.
 */
void dacStart(void)
{
#if SOC_DAC_SUPPORTED
    if (dac_handle && !dacWriting)
    {
        /* Enable and start the continuous channels */
        ESP_ERROR_CHECK(dac_continuous_enable(dac_handle));
        ESP_ERROR_CHECK(dac_continuous_start_async_writing(dac_handle));
        dacWriting = true;
    }
#else
	//i2s_chan_info_t ci;
	//ESP_ERROR_CHECK(i2s_channel_get_info(dac_chan, &ci));
	//ci.total_dma_buf_size is not available in this version of the IDF?

	uint8_t samples[512]; // XXX TODO Update
	if( dacCb )
		dacCb( samples, sizeof(samples) );
	else
		memset( samples, 0x7f, sizeof(samples) );

	size_t writ = 0; // unused.
	ESP_ERROR_CHECK(i2s_channel_preload_data(dac_chan, samples, sizeof(samples), &writ));

	i2s_channel_enable(dac_chan);
#endif
}

/**
 * @brief Stop the DAC.
 */
void dacStop(void)
{
#if SOC_DAC_SUPPORTED
    if (dac_handle && dacWriting)
    {
        /* Stop and disable the continuous channels */
        ESP_ERROR_CHECK(dac_continuous_stop_async_writing(dac_handle));
        ESP_ERROR_CHECK(dac_continuous_disable(dac_handle));
        dacWriting = false;
    }
#else
    i2s_channel_disable(dac_chan);
#endif
}

/**
 * @brief Poll the queue to see if any buffers need to be filled with audio samples
 */
void dacPoll(void)
{
#if SOC_DAC_SUPPORTED
    if (dac_handle && dacIsrQueue && dacWriting)
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
#else
    //#warning "TODO: Main loop for PDM output"
	// No need to poll.
#endif
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
        dacStop();
#if SOC_DAC_SUPPORTED
        ESP_ERROR_CHECK(gpio_set_level(shdnGpio, 1));
#endif
    }
    else
    {
#if SOC_DAC_SUPPORTED
        ESP_ERROR_CHECK(gpio_set_level(shdnGpio, 0));
#endif
        dacStart();
    }
}
