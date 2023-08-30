/**
 * @file emu_args.h
 * @author dylwhich (dylan@whichard.com)
 * @brief Defines a struct for command-line arguments and a method to parse them.
 * @date 2023-08-08
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
    bool fullscreen;
    bool hideLeds;

    bool fuzz;
    bool fuzzButtons;
    bool fuzzTouch;
    bool fuzzMotion;

    /// @brief Name of the keymap to use, or NULL if none
    const char* keymap;

    bool emulateMotion;
    bool motionJitter;
    uint16_t motionJitterAmount;
    bool motionDrift;

    bool emulateTouch;
} emuArgs_t;

//==============================================================================
// Variables
//==============================================================================

extern emuArgs_t emulatorArgs;

//==============================================================================
//  Function Prototypes
//==============================================================================

bool emuParseArgs(int argc, char** argv);
