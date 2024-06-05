#pragma once

#include "mode_pinball.h"

#define PINBALL_GRAVITY (1 / 60.0f) ///< Gravitational constant

#define WALL_BOUNCINESS 0.5f

#define FLIPPER_UP_DEGREES_PER_FRAME 0.296705972839036f ///< Number of degrees (17) to move a flipper up per 60fps frame
#define FLIPPER_DOWN_DEGREES_PER_FRAME \
    0.174532925199433f                        ///< Number of degrees (10) to move a flipper down per 60fps frame
#define FLIPPER_UP_ANGLE   0.349065850398866f ///< Angle of a flipper (20) when actuated
#define FLIPPER_DOWN_ANGLE 0.523598775598299f ///< Angle of a flipper (30) when idle

void updateFlipperPos(pinball_t* p, pbFlipper_t* f);
void updatePinballPhysicsFrame(pinball_t* p);
