//==============================================================================
// Includes
//==============================================================================

#include "hdw-temperature.h"

//==============================================================================
// Variables
//==============================================================================

static temperature_sensor_handle_t temp_sensor = NULL;

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initialize the ESP's onboard temperature sensor
 */
void initTemperatureSensor(void)
{
    temperature_sensor_config_t temp_sensor_config = TEMPERATURE_SENSOR_CONFIG_DEFAULT(-10, 80);
    ESP_ERROR_CHECK(temperature_sensor_install(&temp_sensor_config, &temp_sensor));

    // ESP_LOGI(TAG, "Enable temperature sensor");
    ESP_ERROR_CHECK(temperature_sensor_enable(temp_sensor));
}

/**
 * @brief deinitialize the ESP's onboard temperature sensor
 */
void deinitTemperatureSensor(void)
{
    temperature_sensor_disable(temp_sensor);
    temperature_sensor_uninstall(temp_sensor);
}

/**
 * @brief Get a temperature reading from the ESP's onboard temperature sensor
 *
 * @return A floating point temperature, or -274 if not read
 */
float readTemperatureSensor(void)
{
    float tsens_value;
    if (ESP_OK == temperature_sensor_get_celsius(temp_sensor, &tsens_value))
    {
        return tsens_value;
    }

    // Impossible!
    return -274;
}
