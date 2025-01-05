#pragma once

#include "midiFileParser.h"

/**
 * @brief Check for and return the next MIDI event from USB
 *
 * @param[out] event A pointer to an event
 * @return true if an event was written
 * @return false if no event was written
 */
bool usbMidiCallback(midiEvent_t* event);

/**
 * @brief Install the MIDI USB driver
 *
 * @return true if the driver was successfully installed
 * @return false if the driver installation failed
 */
bool installMidiUsb(void);
