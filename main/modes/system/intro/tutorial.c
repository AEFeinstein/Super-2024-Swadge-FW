#include "tutorial.h"
#include "hdw-btn.h"
#include "touchUtils.h"
#include "macros.h"

#include <string.h>
#include <esp_timer.h>
#include <esp_log.h>

// Static Function Prototypes

// Call once a frame for each trigger after handling normal button actions
static bool tutorialCheckTrigger(tutorialState_t* state, const tutorialTrigger_t* trigger);

// Called to initialize the tutorial state
void tutorialSetup(tutorialState_t* state, tutorialStepCb stepCbFunc, const tutorialStep_t steps[], size_t count,
                   void* data)
{
    // Make sure the state is 0-initialized
    memset(state, 0, sizeof(tutorialState_t));

    state->stepCbFunc    = stepCbFunc;
    state->steps         = steps;
    state->stepCount     = count;
    state->curStep       = steps;
    state->data          = data;
    state->stepStartTime = esp_timer_get_time();
}

// Call with each button press
void tutorialOnButton(tutorialState_t* state, const buttonEvt_t* evt)
{
    state->allButtons |= evt->state;
    state->curButtons     = evt->state;
    state->lastButton     = evt->button;
    state->lastButtonDown = evt->down;
}

// Call when touch is detected
void tutorialOnTouch(tutorialState_t* state, int32_t phi, int32_t r, int32_t intensity)
{
    state->touchPhi       = phi;
    state->touchRadius    = r;
    state->touchIntensity = intensity;

    touchJoystick_t zone = getTouchJoystickZones(phi, r, true, true);
    state->allTouchZones |= zone;
    state->curTouchZone = zone;

    if (intensity)
    {
        getTouchSpins(&state->spinState, phi, r);
    }
    else
    {
        memset(&state->spinState, 0, sizeof(touchSpinState_t));
    }
}

// Call when motion is detected
void tutorialOnMotion(tutorialState_t* state, int16_t x, int16_t y, int16_t z)
{
    if (x > 240)
    {
        state->orientations[0] = true;
    }

    if (x < -240)
    {
        state->orientations[1] = true;
    }

    if (y > 240)
    {
        state->orientations[2] = true;
    }

    if (y < -240)
    {
        state->orientations[3] = true;
    }

    if (z > 240)
    {
        state->orientations[4] = true;
    }

    if (z < -240)
    {
        state->orientations[5] = true;
    }
}

// Call when the microphone is sampled
void tutorialOnSound(tutorialState_t* state, int32_t energy)
{
    // Check for a pass
    if (energy > 100000)
    {
        state->loudSound = true;
    }
}

// Call once per frame, after input handling, to check the current step's triggers
void tutorialCheckTriggers(tutorialState_t* state)
{
    if (!state || !state->curStep)
    {
        return;
    }

    const tutorialStep_t* prev = NULL;
    const tutorialStep_t* next = NULL;
    bool backtrack             = false;

    if (tutorialCheckTrigger(state, &state->curStep->trigger))
    {
        if ((state->curStep - state->steps) < state->stepCount)
        {
            prev = state->curStep;

            state->curStep++;
            state->allButtons = 0;
            // state->lastButton = 0;
            // state->lastButtonDown = false;
            state->stepStartTime     = esp_timer_get_time();
            state->tempMessage       = NULL;
            state->tempMessageExpiry = 0;
            memset(&state->orientations, false, sizeof(state->orientations));
            state->loudSound = false;

            next = state->curStep;
        }
    }
    else if (tutorialCheckTrigger(state, &state->curStep->backtrack))
    {
        prev = state->curStep;
        // check the bounds just in case
        if (state->curStep->backtrackSteps < (state->curStep - state->steps))
        {
            state->curStep -= state->curStep->backtrackSteps;
        }
        else
        {
            // Go to the first step
            state->curStep = state->steps;
        }

        state->allButtons     = 0;
        state->lastButton     = 0;
        state->lastButtonDown = false;
        state->stepStartTime  = esp_timer_get_time();

        if (NULL != prev->backtrackMessage)
        {
            state->tempMessage = prev->backtrackMessage;
            // If the
            state->tempMessageExpiry
                = (prev->backtrackMessageTime == 0) ? 0 : state->stepStartTime + prev->backtrackMessageTime;
        }
        else
        {
            state->tempMessage       = NULL;
            state->tempMessageExpiry = 0;
        }

        next      = state->curStep;
        backtrack = true;
    }

    // Call the callback if it exists and needs to be called
    if (NULL != state->stepCbFunc && (NULL != prev || NULL != next))
    {
        state->stepCbFunc(state, prev, next, backtrack);
    }
}

static bool tutorialCheckTrigger(tutorialState_t* state, const tutorialTrigger_t* trigger)
{
    switch (trigger->type)
    {
        case NO_TRIGGER:
            return false;

        case BUTTON_PRESS_ALL:
            return (state->allButtons & trigger->buttons) == trigger->buttons;

        case BUTTON_PRESS_ANY:
            return (state->curButtons & trigger->buttons) != 0 && state->lastButtonDown;

        case BUTTON_PRESS:
            return (state->lastButton & (trigger->buttons)) == trigger->buttons && state->lastButtonDown;

        case BUTTON_RELEASE:
            return state->lastButtonDown == false && (state->lastButton & trigger->buttons) == state->lastButton;

        case TOUCH_ZONE_ALL:
            return (state->allTouchZones & trigger->touchZones) == trigger->touchZones;

        case TOUCH_ZONE_ANY:
            return (state->curTouchZone & trigger->touchZones) == trigger->touchZones;

        case TOUCH_SPIN:
            return trigger->intData == state->spinState.spins;

        case IMU_SHAKE:
        {
            int32_t orientationsHit = 0;
            for (int32_t i = 0; i < ARRAY_SIZE(state->orientations); i++)
            {
                if (state->orientations[i])
                {
                    orientationsHit++;
                }
            }
            return orientationsHit >= 4;
        }

        case MIC_LOUD:
            return state->loudSound;

        case TIME_PASSED:
            return (state->stepStartTime + trigger->intData) <= esp_timer_get_time();

        case CUSTOM_TRIGGER:
            return trigger->custom.checkFn(state, trigger);

        default:
            break;
    }

    return false;
}