/**
 * @file mode_2048.h
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief A game of 2048 for 2024-2025 Swadge hardware
 * @version 1.1.3
 * @date 2024-06-28
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once

//==============================================================================
//  Version 1.2 goals:
//==============================================================================

/*
- Animations
    - Slide
    - Sparkles for merges
    - Zoom in for new
- Fix bugs in 1.1
*/

//==============================================================================
//  Version 1.3 goals:
//==============================================================================

/*
- Tilt controls
  - Control selection
  - Deadzone
  - Tilt and return to neutral to slide
  - Allow for tilt recalibration with B
- Refactor files into multiple files
- New, custom sounds
*/

//==============================================================================
// Includes
//==============================================================================

#include <math.h>
#include <esp_random.h>

#include "swadge2024.h"
#include "textEntry.h"

//==============================================================================
// Extern variables
//==============================================================================

extern swadgeMode_t t48Mode;
