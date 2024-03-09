/*! \file hdw-temperature.h
 *
 * \section temperature_design Design Philosophy
 *
 * Temperature sensor code  code is based on <a
 * href="https://github.com/espressif/esp-idf/tree/v5.2.1/examples/peripherals/temp_sensor">Temperature Sensor
 * Example</a>.
 *
 * \section temperature_usage Usage
 *
 * You don't need to call initTemperatureSensor(). The system does at the appropriate time.
 * If you're not using the sensor, you can call deinitTemperatureSensor() to disable it.
 *
 * Call readTemperatureSensor() to get the chip's temperature in celsius.
 *
 * \section temperature_example Example
 *
 * \code{.c}
 * printf("%f\n", readTemperatureSensor());
 * \endcode
 */

#ifndef _HDW_TEMPERATURE_
#define _HDW_TEMPERATURE_

void initTemperatureSensor(void);
void deinitTemperatureSensor(void);
float readTemperatureSensor(void);

#endif