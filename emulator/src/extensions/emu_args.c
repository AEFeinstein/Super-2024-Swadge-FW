//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include "emu_args.h"

//==============================================================================
// Macros
//==============================================================================

/**
 * @brief Generates a conditional to check if the current argument matches argName.
 *
 * @param argName Must be a string literal
 */
#define MATCH_ARG(argName) (!strncmp(arg, "--" argName, sizeof("--" argName)))

/**
 * @brief Generates an error if there is no parameter value given for this argument
 */
#define REQUIRE_PARAM(name)                                                             \
    if (param == NULL)                                                                  \
    {                                                                                   \
        fprintf(stderr, "Error: Missing required parameter for argument '%s'\n", name); \
        return false;                                                                   \
    }                                                                                   \
    else                                                                                \
    {                                                                                   \
        valUsed = true;                                                                 \
    }

///< Returns the string value of the current argument's parameter
#define STR_VALUE(name) (valUsed = true, param)

///< Returns the integer value of the current argument's parameter
#define INT_VALUE(name) _##name##Value

///< Parses the named argument as an integer or generates an error if not
#define REQUIRE_INT(name)                                                                                \
    errno              = 0;                                                                              \
    int _##name##Value = strtol(param, NULL, 0);                                                         \
    if (errno != 0)                                                                                      \
    {                                                                                                    \
        fprintf(stderr, "Error: Invalid integer parameter value '%s' for argument '%s'\n", param, name); \
        return false;                                                                                    \
    }                                                                                                    \
    else                                                                                                 \
    {                                                                                                    \
        valUsed = true;                                                                                  \
    }

//==============================================================================
// Defines
//==============================================================================

#define ARG_FULLSCREEN     "fullscreen"
#define ARG_HIDE_LEDS      "hide-leds"
#define ARG_HELP           "help"

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

static const char helpUsage[]
    = "usage: %s [--fullscreen] [--hide-leds] [--help]\n"
      "Emulates a swadge\n"
      "\n"
      "--" ARG_FULLSCREEN "\t\topen in fullscreen mode\n"
      "--" ARG_HIDE_LEDS "\t\tdon't draw simulated LEDs on the sides of the window\n"
      "--" ARG_HELP "\t\t\tdisplay this help message and exit\n";

//==============================================================================
// Functions
//==============================================================================

bool emuParseArgs(int argc, char** argv)
{
    for (int n = 1; n < argc; n++)
    {
        char* arg    = argv[n];
        char* param  = NULL;
        bool valUsed = false;

        // If there's another argument, and it doesn't start with a '-'
        if (n + 1 < argc && strncmp(argv[n + 1], "-", 1))
        {
            param = argv[n + 1];
        }

        if (MATCH_ARG(ARG_FULLSCREEN))
        {
            emulatorArgs.fullscreen = true;
        }
        else if (MATCH_ARG(ARG_HIDE_LEDS))
        {
            emulatorArgs.hideLeds = true;
        }
        else if (MATCH_ARG(ARG_HELP))
        {
            printf(helpUsage, *argv);

            // Return false to stop execution after printing help
            return false;
        }
        else
        {
            fprintf(stderr, "Warning: Unrecognized argument '%s'\n", arg);
        }

        // If we used a parameter value here, don't try to parse it as an argument
        if (valUsed && param != NULL)
        {
            n++;
        }
    }

    return true;
}
