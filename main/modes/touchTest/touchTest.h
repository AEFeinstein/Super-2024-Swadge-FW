#ifndef _TOUCH_TEST_MODE_H_
#define _TOUCH_TEST_MODE_H_

#include "swadge2024.h"

extern swadgeMode_t touchTestMode;

void touchDrawCircle(font_t* font, const char* label, int16_t x, int16_t y, int16_t r, int16_t segs, bool center,
                     bool touched, touchJoystick_t val);

#endif