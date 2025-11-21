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
    int fakeTime;

    int fullscreen;
    int hideLeds;

    int fuzz;
    int fuzzButtons;
    int fuzzTime;
    int fuzzTouch;
    int fuzzMotion;

    int headless;

    /// @brief Name of the keymap to use, or NULL if none
    const char* keymap;

    int lock;

    const char* startMode;
    uint32_t modeSwitchTime;

    bool emulateMotion;
    bool motionJitter;
    uint16_t motionJitterAmount;
    bool motionDrift;

    // Touch Extension

    int emulateTouch;

    // Replay Extension

    /// @brief Whether or not to record the inputs to a file
    int record;

    /// @brief Whether or not to play back recorded inputs from a file
    int playback;

    /// @brief Name of the file to record inputs to, or NULL for the default
    const char* recordFile;

    /// @brief Name of the file to replay inputs from
    const char* replayFile;

    /// @brief A value to use to manually seed the random number generator
    int seed;

    /// @brief Whether to display an FPS counter
    int showFps;

    /// @brief Whether VSync is enabled
    int vsync;

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
