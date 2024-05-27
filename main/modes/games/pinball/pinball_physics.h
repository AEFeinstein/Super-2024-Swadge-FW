#pragma once

#include "mode_pinball.h"

#define PINBALL_GRAVITY (1 / 60.0f) ///< Gravitational constant

#define FLIPPER_UP_DEGREES_PER_FRAME   17 ///< Number of degrees to move a flipper up per 60fps frame
#define FLIPPER_DOWN_DEGREES_PER_FRAME 10 ///< Number of degrees to move a flipper down per 60fps frame
#define FLIPPER_UP_ANGLE               20 ///< Angle of a flipper when actuated
#define FLIPPER_DOWN_ANGLE             30 ///< Angle of a flipper when idle

void updateFlipperPos(pbFlipper_t* f);
void updatePinballPhysicsFrame(pinball_t* p);
