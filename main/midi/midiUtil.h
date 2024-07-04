#pragma once

#include <stdint.h>
#include "fp_math.h"

/**
 * @brief Calculate the frequency of a MIDI note bent by a pitch wheel setting
 *
 * @param noteId The MIDI note ID to bend
 * @param pitchWheel The 14-bit MIDI pitch wheel value
 * @return uq16_16 The note frequency as a UQ16.16 value
 */
uq16_16 bendPitchWheel(uint8_t noteId, uint16_t pitchWheel);

/**
 * @brief Calculate the new note frequency resulting from a pitch bend
 *
 * @param freq The frequency, in Hertz, of the note
 * @param bendCents The number of cents to bend the note, between -100 and 100 inclusive
 * @return uq16_16 The note frequency as a UQ16.16 value
 */
uq16_16 bendPitchFreq(uq16_16 freq, int32_t bendCents);
