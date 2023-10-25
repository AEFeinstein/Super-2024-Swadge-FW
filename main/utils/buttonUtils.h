#ifndef _BUTTON_UTILS_H_
#define _BUTTON_UTILS_H_

#include <stdbool.h>
#include <stdint.h>

#include "hdw-btn.h"

/**
 * @brief Structure holding state needed to repeat held button presses
 */
typedef struct
{
    /// @brief Bitmask of all buttons which will repeat when held
    uint16_t repeatMask;

    /// @brief The time in microseconds a button must be held initially before it repeats
    int64_t repeatDelay;

    /// @brief The time in microseconds between each button repeat after the initial one
    int64_t repeatInterval;

    /// @brief Last held state of the buttons
    uint16_t state;

    /// @brief The time each held button was pressed
    int64_t holdStartTimes[8];

    /// @brief The time each button was last repeated
    int64_t lastRepeats[8];
} buttonRepeatState_t;

bool checkButtonQueueRepeat(buttonRepeatState_t* repeatState, buttonEvt_t* evt);
uint8_t getButtonIndex(buttonBit_t button);

#endif
