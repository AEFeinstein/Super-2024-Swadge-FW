#pragma once

#include <stdint.h>
#include "dance.h"

typedef struct
{
    int32_t random_choice;
    uint32_t tAccumulated;
    void* childState;
} danceRandomDanceState_t;

void danceRandomDance(uint32_t tElapsedUs, uint32_t arg, bool reset, bool useColor, void* state);
void danceRandomDanceDeinit(void* state);

#ifdef DANCE_IMPLEMENTATION

static void danceRandomDanceFreeChild(danceRandomDanceState_t* danceState)
{
    if (danceState->childState)
    {
        const ledDance_t* selected = danceState->random_choice >= 0 ? &ledDances[danceState->random_choice] : NULL;
        if (selected && selected->deinit)
        {
            selected->deinit(danceState->childState);
        }
        heap_caps_free(danceState->childState);
        danceState->childState = NULL;
    }
}

static void danceRandomDanceSelect(danceRandomDanceState_t* danceState, bool useColor)
{
    danceRandomDanceFreeChild(danceState);
    do
    {
        danceState->random_choice = danceRand(getNumDances() - 3);
    } while (useColor && !ledDances[danceState->random_choice].userColor);

    const ledDance_t* selected = &ledDances[danceState->random_choice];
    if (selected->stateSize)
    {
        danceState->childState = heap_caps_calloc(1, selected->stateSize, MALLOC_CAP_8BIT);
    }
    selected->func(0, selected->arg, true, selected->userColor && useColor, danceState->childState);
}

/**
 * Pick a random dance mode and call it at its period for 4.5s. Then pick
 * another random dance and repeat
 *
 * @param tElapsedUs The time elapsed since last call, in microseconds
 * @param reset      true to reset this dance's variables
 */
void danceRandomDance(uint32_t tElapsedUs, uint32_t arg, bool reset, bool useColor, void* state)
{
    danceRandomDanceState_t* danceState = state;

    if (reset)
    {
        danceRandomDanceFreeChild(danceState);
        danceState->random_choice = -1;
        danceState->tAccumulated  = 4500000;
        return;
    }

    if (-1 == danceState->random_choice)
    {
        danceRandomDanceSelect(danceState, useColor);
    }

    danceState->tAccumulated += tElapsedUs;
    while (danceState->tAccumulated >= 4500000)
    {
        danceState->tAccumulated -= 4500000;
        danceRandomDanceSelect(danceState, useColor);
    }

    const ledDance_t* selected = &ledDances[danceState->random_choice];
    selected->func(tElapsedUs, selected->userColor && useColor ? arg : selected->arg, false,
                   selected->userColor && useColor, danceState->childState);
}

void danceRandomDanceDeinit(void* state)
{
    danceRandomDanceState_t* danceState = state;
    if (danceState)
    {
        danceRandomDanceFreeChild(danceState);
    }
}

#endif
