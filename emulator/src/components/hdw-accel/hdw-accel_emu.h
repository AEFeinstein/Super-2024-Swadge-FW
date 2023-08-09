#pragma once

#include <stdint.h>

void emulatorSetAccelerometer(int16_t x, int16_t y, int16_t z);
void emulatorGetAccelerometer(int16_t* x, int16_t* y, int16_t* z);
void emulatorGetAccelerometerRange(int16_t* min, int16_t* max);
void emulatorSetAccelerometerRotation(int16_t value, uint16_t yaw, uint16_t pitch, uint16_t roll);
