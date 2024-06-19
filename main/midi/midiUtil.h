#pragma once

#include <stdint.h>
#include "fp_math.h"

/**
 * @brief Calculate the new note frequency resulting from a pitch bend
 *
 * @param freq The frequency, in Hertz, of the note
 * @param bendCents The number of cents to bend the note, between -100 and 100 inclusive
 * @return uq16_16 The note frequency as a UQ16.16 value
 */
uq16_16 bendPitchFreq(uq16_16 freq, int32_t bendCents);
