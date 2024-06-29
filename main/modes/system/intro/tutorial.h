#ifndef _TUTORIAL_H_
#define _TUTORIAL_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "swadge2024.h"
#include "hdw-btn.h"
#include "touchUtils.h"

typedef struct tutorialTrigger tutorialTrigger_t;
typedef struct tutorialStep tutorialStep_t;
typedef struct tutorialState tutorialState_t;

/**
 * @brief A callback which is called when a trigger activates and the tutorial moves to the next step
 * @param prev The tutorial step whose trigger was satisfied and is now complete
 * @param next The new current tutorial step to be checked
 * @param backtrack Whether the activated trigger was a backtracking trigger or not
 */
typedef void (*tutorialStepCb)(const tutorialState_t* state, const tutorialStep_t* prev, const tutorialStep_t* next,
                               bool backtrack);

/**
 * @brief A callback which is called to check whether a custom trigger (CUSTOM_TRIGGER_DATA and CUSTOM_TRIGGER)
 * @param state The tutorial state object
 * @param trigger The trigger to check, with type `CUSTOM_TRIGGER`.
 * @return true if the trigger's conditions were met
 * @return false if the trigger's conditions were not met
 */
typedef bool (*tutorialTriggerCb)(const tutorialState_t* state, const tutorialTrigger_t* trigger);

typedef enum
{
    // The trigger will never be satisfied
    NO_TRIGGER = 0,

    // Press all of the given buttons at least once, in any order
    BUTTON_PRESS_ALL,

    // Press any one of the given buttons
    BUTTON_PRESS_ANY,

    // Press all of the given buttons at once
    BUTTON_PRESS,

    // Release the given button
    BUTTON_RELEASE,

    // Touch all of the given zones at least once, in any order
    TOUCH_ZONE_ALL,

    // Touch any one of given zones
    TOUCH_ZONE_ANY,

    // A number of CCW (or CW if negative) spin degrees
    TOUCH_SPIN,

    // An amount of time (in intData) passes
    TIME_PASSED,

    // A custom check is triggered, with extra data stored in `
    CUSTOM_TRIGGER,
} tutorialTriggerType_t;

// should end up being 32B each, max including padding (4B for type, 8B for one member, 8B for another member, 4B
// padding)
struct tutorialTrigger
{
    tutorialTriggerType_t type;
    union
    {
        /// @brief A button or buttons
        buttonBit_t buttons;

        touchJoystick_t touchZones;

        /// @brief Generic signed int data
        int64_t intData;

        /// @brief Generic unsigned int data
        uint64_t uintData;

        /// @brief Generic void pointer data
        const void* ptrData;

        struct
        {
            union
            {
                /// @brief Custom pointer data
                void* ptr;

                /// @brief Custom int data
                int64_t data;
            };
            tutorialTriggerCb checkFn;
        } custom;

        const tutorialTrigger_t* subTrigger;

        /// @brief Pointers to child triggers for binary operators
        struct
        {
            const tutorialTrigger_t* left;
            const tutorialTrigger_t* right;
        } subTriggerPair;

        /// @brief Pointer to an array of other triggers
        struct
        {
            const tutorialTrigger_t* items;
            size_t count;
        } subTriggerList;
    };
};

typedef struct tutorialStep
{
    tutorialTrigger_t trigger;
    tutorialTrigger_t backtrack;
    /// @brief The number of steps to go back when the backtrack trigger activates
    // protip: set backtrackSteps to 0 to reset the current progress
    uint8_t backtrackSteps;

    /// @brief An optional message to replace the next step's detail temporarily after backtracking
    const char* backtrackMessage;
    /// @brief The time, in microseconds, that the backtrack message should remain for
    int64_t backtrackMessageTime;

    /// @brief The main prompt
    const char* title;
    /// @brief Some detail, if any
    const char* detail;
    /// @brief The minimum amount of time to show the text, in seconds
    int cooldown;
} tutorialStep_t;

typedef struct tutorialState
{
    buttonBit_t allButtons;
    buttonBit_t curButtons;
    buttonBit_t lastButton;
    bool lastButtonDown;

    int32_t touchPhi;
    int32_t touchRadius;
    int32_t touchIntensity;

    touchJoystick_t allTouchZones;
    touchJoystick_t curTouchZone;

    // touchSpinState_t touchSpin;

    int64_t stepStartTime;

    const tutorialStep_t* steps;
    size_t stepCount;

    const tutorialStep_t* curStep;

    const char* tempMessage;
    int64_t tempMessageExpiry;

    tutorialStepCb stepCbFunc;

    /// @brief Custom data to be used by the step callback or custom trigger callbacks
    void* data;
} tutorialState_t;

// Called to initialize the tutorial state
void tutorialSetup(tutorialState_t* state, tutorialStepCb stepCbFunc, const tutorialStep_t steps[], size_t count,
                   void* data);

// Call with each button press
void tutorialOnButton(tutorialState_t* state, const buttonEvt_t* evt);

// Call when touch is detected
void tutorialOnTouch(tutorialState_t* state, int32_t phi, int32_t r, int32_t intensity);

// Call once per frame, after input handling, to check the current step's triggers
void tutorialCheckTriggers(tutorialState_t* state);

#endif
