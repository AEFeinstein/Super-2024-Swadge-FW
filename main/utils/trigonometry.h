#ifndef _TRIGONOMETRY_H_
#define _TRIGONOMETRY_H_

#include <stdint.h>

extern const int16_t sin1024[360];
extern const uint16_t tan1024[90];

int16_t getSin1024(int16_t degree);
int16_t getCos1024(int16_t degree);
int32_t getTan1024(int16_t degree);

#endif