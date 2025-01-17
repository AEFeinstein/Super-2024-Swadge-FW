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
    float fakeFps;
    bool fakeTime;

    bool fullscreen;
    bool hideLeds;

    bool fuzz;
    bool fuzzButtons;
    bool fuzzTime;
    bool fuzzTouch;
    bool fuzzMotion;

    bool headless;

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

    /// @brief A value to use to manually seed the random number generator
    uint32_t seed;

    /// @brief Whether to display an FPS counter
    bool showFps;

    /// @brief Whether VSync is enabled
    bool vsync;

    // MIDI
    const char* midiFile;

    // Joystick
    const char* joystick;

    // Joystick config preset name
    const char* jsPreset;
} emuArgs_t;

//==============================================================================
// Variables
//==============================================================================

extern emuArgs_t emulatorArgs;

//==============================================================================
//  Function Prototypes
//==============================================================================

bool emuParseArgs(int argc, char** argv);
