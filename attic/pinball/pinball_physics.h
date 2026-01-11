#pragma once

#include "pinball_typedef.h"

void pbSimulate(pbScene_t* scene, int32_t elapsedUs);
bool ballLineIntersection(pbBall_t* ball, pbLine_t* line);
