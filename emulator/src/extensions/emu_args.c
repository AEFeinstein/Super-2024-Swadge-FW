//==============================================================================
// Includes
//==============================================================================

#include <errno.h>
#include <argp.h>
#include "emu_args.h"

//==============================================================================
// Function Prototypes
//==============================================================================

static error_t parseOpt(int key, char* arg, struct argp_state* state);

//==============================================================================
// Variables
//==============================================================================

emuArgs_t emulatorArgs = {
    .fullscreen = false,
    .hideLeds   = false,

    .keymap = NULL,

    .emulateMotion      = false,
    .motionJitter       = false,
    .motionJitterAmount = 5,
    .motionDrift        = false,

    .emulateTouch = false,
};

static const char doc[] = "Emulates a swadge";

// clang-format off
static const struct argp_option options[] =
{
    { "fullscreen", 'f', 0, 0, "Open in fullscreen mode" },
    { "hide-leds",  'l', 0, 0, "Don't draw simulated LEDs next to the display" },
    { "touch",      't', 0, 0, "Simulate touch sensor readings with a virtual touchpad" },
    {0},
};
// clang-format on

static struct argp argp = {options, parseOpt, NULL, doc};

//==============================================================================
// Functions
//==============================================================================

static error_t parseOpt(int key, char* arg, struct argp_state* state)
{
    switch (key)
    {
        case 'f':
        {
            // Fullscreen
            emulatorArgs.fullscreen = true;
            break;
        }

        case 'l':
        {
            // Hide LEDs
            emulatorArgs.hideLeds = true;
            break;
        }

        case 't':
        {
            // Touchpad
            emulatorArgs.emulateTouch = true;
            break;
        }
    }

    return 0;
}

bool emuParseArgs(int argc, char** argv)
{
    // Pass nothing to input because we just have a static args struct
    // If argp_parse returns non-zero there's an error
    // So, return true as long as it's zero
    return (0 == argp_parse(&argp, argc, argv, 0, 0, NULL));
}
