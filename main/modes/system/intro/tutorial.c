#include "tutorial.h"
#include "hdw-btn.h"
#include "touchUtils.h"
#include "macros.h"

#include <string.h>
#include <esp_timer.h>

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

    // Begin timbre initialization stuff
    memcpy(&state->timbre, getTimbreForProgram(false, 1, 1), sizeof(midiTimbre_t));
    state->timbre.envelope.attackTime    = 0;
    state->timbre.envelope.attackTimeVel = 0;
    // Using decay instead of release, with a sustain volume of 0, makes the note behave more like percussion
    // (no note off is necessary)
    // The control is set in increments of 10ms, so this is equivalent to setting release CC to 60
    state->timbre.envelope.decayTime    = MS_TO_SAMPLES(20); // 600
    state->timbre.envelope.decayTimeVel = (MS_TO_SAMPLES(150) << 8) / 127;
    // Setting sustain volume to 0 means the note ends on its own after decay
    state->timbre.envelope.sustainVol  = 0;
    state->timbre.envelope.releaseTime = 0;
    // End timbre initialization stuff
}

// Call with each button press
void tutorialOnButton(tutorialState_t* state, const buttonEvt_t* evt)
{
    state->allButtons |= evt->state;
    state->curButtons     = evt->state;
    state->lastButton     = evt->button;
    state->lastButtonDown = evt->down;
}

// Call while on the linear touch tutorial. It's the game logic for pong.
void tutorialOnLinearTouch(int64_t elapsedUs, tutorialState_t* state, int32_t* angleTimer,
                           linearTouch_t linearTouchState[2])
{
    // update the paddles either with the touch positions or with a sine wave while not touching.
    *angleTimer += elapsedUs;
    int minDeg = 70;
    int maxDeg = 125;
    int input;
    if (linearTouchState[1].touched)
    {
        input = linearTouchState[1].position << 1;
    }
    else
    {
        input = getSin1024((*angleTimer >> 14) % 359) + 1024;
    }
    int deg = input * (maxDeg - minDeg) / 2048 + minDeg;
    // four decimal bits represent floating point.
    state->pongState.leftPos
        = (vec_t){(140 + (273 * getSin1024(deg) / 4096)) << 4, (120 - (273 * getCos1024(deg) / 4096)) << 4};
    minDeg = 235;
    maxDeg = 290;
    if (linearTouchState[0].touched)
    {
        input = (1024 - linearTouchState[0].position) << 1;
    }
    else
    {
        input = getSin1024(((*angleTimer >> 14) + 90) % 359) + 1024;
    }
    deg = input * (maxDeg - minDeg) / 2048 + minDeg;
    state->pongState.rightPos
        = (vec_t){(140 + (273 * getSin1024(deg) / 4096)) << 4, (120 - (273 * getCos1024(deg) / 4096)) << 4};
    if (!linearTouchState[0].touched || !linearTouchState[1].touched)
    {
        return;
    }

    // initialize puck position and velocity
    if (!state->pongState.initialized)
    {
        state->pongState.volleyCount = 0;
        state->pongState.puckPos     = (vec_t){140 << 4, 120 << 4};
        // This velocity is some general dirction going right. From 30 degrees to 150 degrees.
        state->pongState.puckVel = rotateVec2d((vec_t){0, -400}, (esp_random() % 120) + 30);
        if (esp_random() % 2) // 50% chance to send it to the left.
        {
            state->pongState.puckVel.x *= -1;
        }
        state->pongState.initialized = true;
    }
    // update puck position
    state->pongState.puckPos.x += state->pongState.puckVel.x * elapsedUs >> 20;
    state->pongState.puckPos.y += state->pongState.puckVel.y * elapsedUs >> 20;
    // walls collision check
    if (state->pongState.puckPos.y < 1440 || state->pongState.puckPos.y > 2608)
    {
        midiPlayer_t* player = globalMidiPlayerGet(MIDI_SFX);
        midiPause(player, false);
        player->headroom = 0x8000; // max volume
        // Setting the channel to something > 15 means the note will not be affected by a song changing MIDI controls.
        soundNoteOn(player, 15, 78, 127, &state->timbre, false);

        state->pongState.puckPos.y = CLAMP(state->pongState.puckPos.y, 1440, 2608);
        state->pongState.puckVel.y *= -1;
    }

    // puck-paddles collision checks
    if (sqMagVec2d(subVec2d(state->pongState.puckPos, state->pongState.leftPos)) < 40000)
    {
        midiPlayer_t* player = globalMidiPlayerGet(MIDI_SFX);
        midiPause(player, false);
        player->headroom = 0x8000; // max volume
        // Setting the channel to something > 15 means the note will not be affected by a song changing MIDI controls.
        soundNoteOn(player, 15, 83, 127, &state->timbre, false);

        state->pongState.volleyCount++;
        vec_t norm = subVec2d(state->pongState.puckPos, state->pongState.leftPos);

        fastNormVec(&norm.x, &norm.y);
        state->pongState.puckPos = divVec2d(mulVec2d(norm, 5), 6);
        state->pongState.puckPos = addVec2d(state->pongState.leftPos, state->pongState.puckPos);
        // Reflect the velocity vector along the normal
        // See http://www.sunshine2k.de/articles/coding/vectorreflection/vectorreflection.html
        // It gets a 10% increase in velocity per hit. hence * 11/10.
        state->pongState.puckVel = divVec2d(
            mulVec2d(subVec2d(state->pongState.puckVel,
                              divVec2d(mulVec2d(norm, (2 * dotVec2d(state->pongState.puckVel, norm))), 65536)),
                     11),
            10);
    }
    if (sqMagVec2d(subVec2d(state->pongState.puckPos, state->pongState.rightPos)) < 40000)
    {
        midiPlayer_t* player = globalMidiPlayerGet(MIDI_SFX);
        midiPause(player, false);
        player->headroom = 0x8000; // max volume
        // Setting the channel to something > 15 means the note will not be affected by a song changing MIDI controls.
        soundNoteOn(player, 15, 76, 127, &state->timbre, false);

        state->pongState.volleyCount++;
        vec_t norm = subVec2d(state->pongState.puckPos, state->pongState.rightPos);

        fastNormVec(&norm.x, &norm.y);
        state->pongState.puckPos = divVec2d(mulVec2d(norm, 5), 6);
        state->pongState.puckPos = addVec2d(state->pongState.rightPos, state->pongState.puckPos);
        state->pongState.puckVel = divVec2d(
            mulVec2d(subVec2d(state->pongState.puckVel,
                              divVec2d(mulVec2d(norm, (2 * dotVec2d(state->pongState.puckVel, norm))), 65536)),
                     11),
            10);
    }
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
    state->orientation[0] = x;
    state->orientation[1] = y;
    state->orientation[2] = z;
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
            memset(&state->orientation, 0, sizeof(state->orientation));
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

        case TOUCH_PONG:
            return state->pongState.initialized == 0 && state->pongState.volleyCount >= 300;

        case TOUCH_SPIN:
            return trigger->intData == state->spinState.spins;

        case IMU_ORIENT:
        {
            // For each axis
            for (int32_t i = 0; i < 3; i++)
            {
                // If the trigger is positive
                if (trigger->orientation[i] > 0)
                {
                    // Return false if the state is less than the trigger
                    if (state->orientation[i] < trigger->orientation[i])
                    {
                        return false;
                    }
                }
                // If the trigger is negative
                else if (trigger->orientation[i] < 0)
                {
                    // Return false if the state is greater than the trigger
                    if (state->orientation[i] > trigger->orientation[i])
                    {
                        return false;
                    }
                }
            }
            // All checks passed
            return true;
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