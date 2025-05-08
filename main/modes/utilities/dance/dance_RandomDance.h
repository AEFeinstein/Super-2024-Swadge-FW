#pragma once

#include <stdint.h>
#include "dance.h"

void danceRandomDance(uint32_t tElapsedUs, uint32_t arg __attribute__((unused)), bool reset);

#ifdef DANCE_IMPLEMENTATION

/**
 * Pick a random dance mode and call it at its period for 4.5s. Then pick
 * another random dance and repeat
 *
 * @param tElapsedUs The time elapsed since last call, in microseconds
 * @param reset      true to reset this dance's variables
 */
void danceRandomDance(uint32_t tElapsedUs, uint32_t arg __attribute__((unused)), bool reset)
{
    static int32_t random_choice = -1;
    static uint32_t tAccumulated = 0;

    if (reset)
    {
        random_choice = -1;
        tAccumulated  = 4500000;
        return;
    }

    if (-1 == random_choice)
    {
        random_choice = danceRand(getNumDances() - 3); // exclude the random mode, excluding random & none
    }

    tAccumulated += tElapsedUs;
    while (tAccumulated >= 4500000)
    {
        tAccumulated -= 4500000;
        random_choice = danceRand(getNumDances() - 3); // exclude the random & none mode
        ledDances[random_choice].func(0, ledDances[random_choice].arg, true);
    }

    ledDances[random_choice].func(tElapsedUs, ledDances[random_choice].arg, false);
}

#endif
