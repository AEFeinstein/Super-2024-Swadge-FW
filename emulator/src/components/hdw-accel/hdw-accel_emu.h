#pragma once

#include <stdint.h>

void emulatorSetAccelerometer(int16_t x, int16_t y, int16_t z);
void emulatorSetAccelerometerRotation(int16_t value, uint16_t zRotation, uint16_t xRotation);
