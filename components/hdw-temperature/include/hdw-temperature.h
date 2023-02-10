#ifndef _HDW_TEMPERATURE_
#define _HDW_TEMPERATURE_

#include <driver/temperature_sensor.h>

void initTemperatureSensor(void);
void deinitTemperatureSensor(void);
float readTemperatureSensor(void);

#endif