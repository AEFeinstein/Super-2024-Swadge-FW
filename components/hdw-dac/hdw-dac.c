//==============================================================================
// Includes
//==============================================================================

#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "driver/dac_continuous.h"

//==============================================================================
// Defines
//==============================================================================

#define AUDIO_SAMPLE_RATE_HZ 32768
#define BUF_SIZE             2048
#define DMA_DESCRIPTORS      4

//==============================================================================
// Unions
//==============================================================================

typedef union
{
    uint32_t accum32; /** The oscillator value is accumulated in a 32 bit integer */
    uint8_t bytes[4]; /** The index into the sine table is accessed as a single byte */
} oscAccum_t;

//==============================================================================
// Variables
//==============================================================================

/** A 256 point sine wave */
static const uint8_t DRAM_ATTR sinTab[] = {
    127, 131, 134, 137, 140, 143, 146, 149, 152, 155, 158, 161, 164, 167, 170, 173, 176, 179, 182, 185, 187, 190,
    193, 195, 198, 201, 203, 206, 208, 210, 213, 215, 217, 219, 222, 224, 226, 228, 230, 231, 233, 235, 236, 238,
    240, 241, 242, 244, 245, 246, 247, 248, 249, 250, 251, 251, 252, 253, 253, 254, 254, 254, 254, 254, 255, 254,
    254, 254, 254, 254, 253, 253, 252, 251, 251, 250, 249, 248, 247, 246, 245, 244, 242, 241, 240, 238, 236, 235,
    233, 231, 230, 228, 226, 224, 222, 219, 217, 215, 213, 210, 208, 206, 203, 201, 198, 195, 193, 190, 187, 185,
    182, 179, 176, 173, 170, 167, 164, 161, 158, 155, 152, 149, 146, 143, 140, 137, 134, 131, 128, 124, 121, 118,
    115, 112, 109, 106, 103, 100, 97,  94,  91,  88,  85,  82,  79,  76,  73,  70,  68,  65,  62,  60,  57,  54,
    52,  49,  47,  45,  42,  40,  38,  36,  33,  31,  29,  27,  25,  24,  22,  20,  19,  17,  15,  14,  13,  11,
    10,  9,   8,   7,   6,   5,   4,   4,   3,   2,   2,   1,   1,   1,   1,   1,   0,   1,   1,   1,   1,   1,
    2,   2,   3,   4,   4,   5,   6,   7,   8,   9,   10,  11,  13,  14,  15,  17,  19,  20,  22,  24,  25,  27,
    29,  31,  33,  36,  38,  40,  42,  45,  47,  49,  52,  54,  57,  60,  62,  65,  68,  70,  73,  76,  79,  82,
    85,  88,  91,  94,  97,  100, 103, 106, 109, 112, 115, 118, 121, 124,
};

/** The handle created for the DAC */
static dac_continuous_handle_t dac_handle;

/** A queue to move dac_event_data_t from the interrupt to the main loop*/
static QueueHandle_t que;

/** Accumulators for oscillators */
static oscAccum_t osc1 = {0};
static oscAccum_t osc2 = {0};

/** Step size for the accumulators */
static uint32_t oscStep1 = ((uint64_t)(sizeof(sinTab) * 440) << 16) / (AUDIO_SAMPLE_RATE_HZ);
static uint32_t oscStep2 = ((uint64_t)(sizeof(sinTab) * 523) << 16) / (AUDIO_SAMPLE_RATE_HZ);

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
        /* Generate a wave on the fly */
        static uint8_t sawtooth[BUF_SIZE];
        for (int32_t i = 0; i < BUF_SIZE; i++)
        {
            /* Oscillate two waves and add them together */
            osc1.accum32 += oscStep1;
            osc2.accum32 += oscStep2;
            sawtooth[i] = (sinTab[osc1.bytes[2]] + sinTab[osc2.bytes[2]]) / 2;

            /* Or generate a sawtooth */
            /* sawtooth[i] = (i * 255) / BUF_SIZE; */
        }

        /* Write the data to the DAC */
        size_t loaded_bytes = 0;
        dac_continuous_write_asynchronously(dac_handle, evt_data.buf, evt_data.buf_size, //
                                            sawtooth, sizeof(sawtooth), &loaded_bytes);
    }
}
