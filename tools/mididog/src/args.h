/**
 * @file args.h
 * @author dylwhich (dylan@whichard.com)
 * @brief Defines a struct for command-line arguments and a method to parse them.
 * @date 2024-12-30
 *
 * Once ::emuParseArgs() has been called, the arguments may be accessed at any time via ::emulatorArgs
 */
#pragma once

//==============================================================================
// Includes
//==============================================================================

#include <stdbool.h>
#include <stdint.h>

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    /// @brief The action to perform
    const char* action;

    // Shrink-specific options (filters)
    /// @brief If true, strip all text events when shrinking a file
    bool stripText;

    // MIDI
    const char* midiIn;
    const char* midiOut;

    // Output
    bool multiLine;
} midiDogArgs_t;

//==============================================================================
// Variables
//==============================================================================

extern midiDogArgs_t mdArgs;

//==============================================================================
//  Function Prototypes
//==============================================================================

bool mdParseArgs(int argc, char** argv);
