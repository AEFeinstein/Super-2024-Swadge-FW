#pragma once
#include "midiPlayer.h"

/**
 * @brief Produces sounds for a standard drumkit according to the General MIDI standard
 *
 * @param drum The MIDI note corresponding to the drum to play
 * @param idx The sample index, which should start at 0 for each note and increase by one every sample
 * @param done A pointer to a boolean which will be set to true when the drum is finished sounding
 * @param scratch Not used by this drumkit
 * @param data Not used by this drumkit
 * @return int8_t The signed 8-bit sample generated for this tick of the drumkit
 */
int8_t defaultDrumkitFunc(percussionNote_t drum, uint32_t idx, bool* done, uint32_t scratch[4], void* data);

/**
 * @brief Produces sounds for the drumkit that was included on the King Donut synth swadge
 *
 * This drumkit does not use the General MIDI drum note numbers
 *
 * @param drum The drum index, between ::ACOUSTIC_BASS_DRUM_OR_LOW_BASS_DRUM and ::HIGH_BONGO, inclusive
 * @param idx The sample index
 * @param done A pointer to a boolean which will be set to true when the drum is finished sounding
 * @param scratch Scratch space which is used to faithfully reproduce the drum sounds
 * @param data Not used by this drumkit
 * @return int8_t The signed 8-bit sample generated for this tick of the drumkit
 */
int8_t donutDrumkitFunc(percussionNote_t drum, uint32_t idx, bool* done, uint32_t scratch[4], void* data);

#ifdef BAKE_DRUMS
void bakeDrums(void);
#endif