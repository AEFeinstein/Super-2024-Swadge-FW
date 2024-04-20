#pragma once

#include "mode_pinball.h"

#define FLIPPER_UP_DEGREES_PER_FRAME   17 ///< Number of degrees to move a flipper up per 60fps frame
#define FLIPPER_DOWN_DEGREES_PER_FRAME 10 ///< Number of degrees to move a flipper down per 60fps frame
#define FLIPPER_UP_ANGLE               20 ///< Angle of a flipper when actuated
#define FLIPPER_DOWN_ANGLE             30 ///< Angle of a flipper when idle

void updateFlipperPos(pbFlipper_t* p);
void updatePinballPhysicsFrame(pinball_t* p);

circle_t intCircle(pbCircle_t pbc);
line_t intLine(pbLine_t pbl);
rectangle_t intRect(pbRect_t pbr);
