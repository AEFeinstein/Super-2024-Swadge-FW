#pragma once

#include "pinball_typedef.h"

void jsSimulate(jsScene_t* scene, int32_t elapsedUs);
bool ballLineIntersection(jsBall_t* ball, jsLine_t* line);
