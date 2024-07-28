//==============================================================================
// Includes
//==============================================================================

#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "hdw-battmon.h"

//==============================================================================
// Variables
//==============================================================================

/// @brief Tag for debugging
const static char* TAG = "BMN";

/// @brief The ADC channel to monitor the battery voltage on
static adc_channel_t battMonChannel;
/// @brief The handle to monitor the battery voltage
static adc_oneshot_unit_handle_t adc_handle;
/// @brief true if calibration was performed, false if it was not
static bool is_calibrated;
/// @brief The handle to calibrate the ADC readings
static adc_cali_handle_t adc_cali_handle = NULL;
/// @brief true if the ADC is initialized, false if it is not
static bool is_initialized = false;

/// @brief A history of raw ADC readings
static int adc_raw[2][10];
/// @brief A history of calibrated voltage ADC readings
static int voltage[2][10];

//==============================================================================
// Function Prototypes
//==============================================================================

static bool adc_calibration_init(adc_unit_t unit, adc_atten_t atten, adc_cali_handle_t* out_handle);
static void adc_calibration_deinit(adc_cali_handle_t handle);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initialize a oneshot analog-to-digital unit to read the battery voltage
 *
 * @param gpio The GPIO to initialize the ADC on
 */
void initBattmon(gpio_num_t gpio)
{
    adc_unit_t unit;
    adc_channel_t channel;
    if (ESP_OK == adc_oneshot_io_to_channel(gpio, &unit, &channel))
    {
        // Save channel to read later
        battMonChannel = channel;

        // ADC init
        adc_oneshot_unit_init_cfg_t init_config1 = {
            .unit_id = unit,
        };
        ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc_handle));

        // ADC config
        adc_oneshot_chan_cfg_t config = {
            .bitwidth = ADC_BITWIDTH_DEFAULT,
            .atten    = ADC_ATTEN_DB_12,
        };
        ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, battMonChannel, &config));

        // ADC calibration
        is_calibrated = adc_calibration_init(unit, ADC_ATTEN_DB_12, &adc_cali_handle);

        // Set it as initialized
        is_initialized = true;
    }
}

/**
 * @brief Deinitialize the analog-to-digital unit used to read the battery voltage
 */
void deinitBattmon(void)
{
    if (is_initialized)
    {
        // Tear Down
        ESP_ERROR_CHECK(adc_oneshot_del_unit(adc_handle));
        if (is_calibrated)
        {
            adc_calibration_deinit(adc_cali_handle);
        }
        is_initialized = false;
    }
}

/**
 * @brief Read and return the battery voltage
 *
 * @return The voltage if read, or -1 if there was an error
 */
int readBattmon(void)
{
    if (is_initialized)
    {
        ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, battMonChannel, &adc_raw[0][0]));
        // ESP_LOGI(TAG, "ADC%d Channel[%d] Raw Data: %d", ADC_UNIT_1 + 1, battMonChannel, adc_raw[0][0]);
        if (is_calibrated)
        {
            ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc_cali_handle, adc_raw[0][0], &voltage[0][0]));
            // ESP_LOGI(TAG, "ADC%d Channel[%d] Cali Voltage: %d mV", ADC_UNIT_1 + 1, battMonChannel, voltage[0][0]);
            return voltage[0][0];
        }
    }
    return -1;
}

/**
 * @brief Initialize analog-to-digital calibration. This is required to get voltage readings rather than raw readings
 *
 * @param unit The ADC that your ADC raw results are from.
 * @param atten ADC attenuation that your ADC raw results use.
 * @param out_handle The ADC calibration handle is returned through this argument.
 * @return true if the unit was calibrated, false if it was not.
 */
static bool adc_calibration_init(adc_unit_t unit, adc_atten_t atten, adc_cali_handle_t* out_handle)
{
    adc_cali_handle_t handle = NULL;
    esp_err_t ret            = ESP_FAIL;
    bool calibrated          = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    // ESP32-C3, ESP32-S3 support this
    if (!calibrated)
    {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Curve Fitting");
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id  = unit,
            .atten    = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
        if (ret == ESP_OK)
        {
            calibrated = true;
        }
    }
#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    // ESP32, ESP32-S2 support this
    if (!calibrated)
    {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Line Fitting");
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id  = unit,
            .atten    = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
        if (ret == ESP_OK)
        {
            calibrated = true;
        }
    }
#endif

    *out_handle = handle;
    if (ret == ESP_OK)
    {
        ESP_LOGI(TAG, "Calibration Success");
    }
    else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated)
    {
        ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
    }
    else
    {
        ESP_LOGE(TAG, "Invalid arg or no memory");
    }

    return calibrated;
}

/**
 * @brief Deinitialize analog-to-digital calibration
 *
 * @param handle The ADC calibration handle to deinitialize
 */
static void adc_calibration_deinit(adc_cali_handle_t handle)
{
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    ESP_LOGI(TAG, "deregister %s calibration scheme", "Curve Fitting");
    ESP_ERROR_CHECK(adc_cali_delete_scheme_curve_fitting(handle));
#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    ESP_LOGI(TAG, "deregister %s calibration scheme", "Line Fitting");
    ESP_ERROR_CHECK(adc_cali_delete_scheme_line_fitting(handle));
#endif
}
