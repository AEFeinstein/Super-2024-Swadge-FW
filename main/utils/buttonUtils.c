#include "buttonUtils.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "hdw-btn.h"
#include "swadge2024.h"

/**
 * @brief Check the button queue, repeating configured buttons
 *
 * @param[in,out] repeatState A pointer to the state, which must be passed to subsequent calls
 * @param[out] evt If an event occurred or was repeated, return it through this argument
 * @return true if an event occurred or was repeated, false if not
 */
bool checkButtonQueueRepeat(buttonRepeatState_t* repeatState, buttonEvt_t* evt)
{
    int64_t now = esp_timer_get_time();

    if (checkButtonQueueWrapper(evt))
    {
        // Update state to reflect new event
        // Check that the button is repeatable
        if (evt->button == (evt->button & repeatState->repeatMask))
        {
            if ((evt->button == (repeatState->state & evt->button)) && !evt->down)
            {
                // Button is already pressed, and this was a release event
                repeatState->holdStartTimes[getButtonIndex(evt->button)] = 0;
                repeatState->lastRepeats[getButtonIndex(evt->button)] = 0;
            }
            else if ((0 == (repeatState->state & evt->button)) && evt->down)
            {
                // Button is not pressed, and this was a press event
                 repeatState->holdStartTimes[getButtonIndex(evt->button)] = now;
                 repeatState->lastRepeats[getButtonIndex(evt->button)] = 0;
            }
            // We ignore any other combination because, well, that shouldn't happen
        }

        // Save the true button state for later
        repeatState->state = evt->state;

        // Return true since a button event was returned
        return true;
    }
    else
    {
        // No new events in the queue, check any currently held buttons
        uint16_t pendingButtons = (repeatState->repeatMask & repeatState->state);

        // Keep check
        while (0 != pendingButtons)
        {
            // Get the next button's index
            uint8_t i = getButtonIndex(pendingButtons);
            // And remove that button from the bitset
            pendingButtons &= ~(1 << i);

            // Check if the button should have started repeating
            if ((repeatState->holdStartTimes[i] != 0)
                && (repeatState->holdStartTimes[i] + repeatState->repeatDelay <= now))
            {

                // It should, but has it?
                if ((repeatState->lastRepeats[i] == 0)
                     || (repeatState->lastRepeats[i] + repeatState->repeatInterval <= now))
                {
                    // Either it hasn't repeated yet, or the next repeat is due

                    // Update the repeat time so we don't just do it again
                    repeatState->lastRepeats[i] = now;

                    // Synthesize the button repeat event and immediately return
                    evt->state = repeatState->state;
                    evt->button = (1 << i);
                    evt->down = true;
                    return true;
                }
            }
        }

        // No repeats were detected so no buttons were synthesized
        return false;
    }
}

/**
 * @brief Return the index of the button value, or the index of the first set button in a bitmask
 *
 * @param button The button
 * @return uint8_t
 */
uint8_t getButtonIndex(buttonBit_t button)
{
    return __builtin_ctz(button);
}
