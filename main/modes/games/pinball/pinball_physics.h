#pragma once

#include "mode_pinball.h"

void updatePaddlePos(pbPaddle_t* p);
void updatePinballPhysicsFrame(pinball_t* p);

circle_t intCircle(pbCircle_t pbc);
line_t intLine(pbLine_t pbl);
rectangle_t intRect(pbRect_t pbr);
