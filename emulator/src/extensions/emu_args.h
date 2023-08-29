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

    bool lock;

    const char* startMode;
    uint32_t modeSwitchTime;

    bool emulateMotion;
    bool motionJitter;
    uint16_t motionJitterAmount;
    bool motionDrift;

    // Touch Extension

    bool emulateTouch;

    // Replay Extension

    /// @brief Whether or not to record the inputs to a file
    bool record;

    /// @brief Whether or not to play back recorded inputs from a file
    bool playback;

    /// @brief Name of the file to record inputs to, or NULL for the default
    const char* recordFile;

    /// @brief Name of the file to replay inputs from
    const char* replayFile;
} emuArgs_t;

//==============================================================================
// Variables
//==============================================================================

extern emuArgs_t emulatorArgs;

//==============================================================================
//  Function Prototypes
//==============================================================================

bool emuParseArgs(int argc, char** argv);
