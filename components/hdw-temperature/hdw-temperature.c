//==============================================================================
// Includes
//==============================================================================

#include <driver/temperature_sensor.h>

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
    temperature_sensor_config_t temp_sensor_config = TEMPERATURE_SENSOR_CONFIG_DEFAULT(0, 50);
    ESP_ERROR_CHECK(temperature_sensor_install(&temp_sensor_config, &temp_sensor));

    // ESP_LOGI(TAG, "Enable temperature sensor");
    ESP_ERROR_CHECK(temperature_sensor_enable(temp_sensor));
}

/**
 * @brief deinitialize the ESP's onboard temperature sensor
 */
void deinitTemperatureSensor(void)
{
    ESP_ERROR_CHECK(temperature_sensor_disable(temp_sensor));
    ESP_ERROR_CHECK(temperature_sensor_uninstall(temp_sensor));
    temp_sensor = NULL;
}

/**
 * @brief Power down the temperature sensor
 */
void powerDownTemperatureSensor(void)
{
    if (temp_sensor)
    {
        ESP_ERROR_CHECK(temperature_sensor_disable(temp_sensor));
    }
}

/**
 * @brief Power up the temperature sensor
 */
void powerUpTemperatureSensor(void)
{
    if (temp_sensor)
    {
        ESP_ERROR_CHECK(temperature_sensor_enable(temp_sensor));
    }
}

/**
 * @brief Get a temperature reading from the ESP's onboard temperature sensor, in Celsius
 *
 * @return A floating point temperature, or -274 if not read
 */
float readTemperatureSensor(void)
{
    float tSens_value;
    if (temp_sensor && (ESP_OK == temperature_sensor_get_celsius(temp_sensor, &tSens_value)))
    {
        return tSens_value;
    }

    // Impossible!
    return -274;
}
