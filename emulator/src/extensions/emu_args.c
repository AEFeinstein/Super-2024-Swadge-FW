//==============================================================================
// Includes
//==============================================================================

// Includes mostly for getopt
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "getopt_win.h"

#include "emu_args.h"
#include "macros.h"

//==============================================================================
// Defines
//==============================================================================

///< The column to print short options aligned to
#define HELP_SHORT_COL 1

///< The column to print long options aligned to
#define HELP_LONG_COL 5

///< The column to print the description to
#define HELP_DESC_COL 29

///< The maximum width of the help text before wrapping to the next line
#define HELP_WRAP_COL 100

///< The column to start wrapped usage lines at
#define HELP_USAGE_COL 12

//==============================================================================
// Structs
//==============================================================================

/**
 * @brief Struct to define an argument's help options
 *
 */
typedef struct
{
    /**
     * @brief The short option character for this option, or 0 if it has none
     */
    char shortOpt;

    /**
     * @brief The long option name for this option, or NULL if it has none
     */
    const char* longOpt;

    /**
     * @brief If this option has a parameter, the documentation for it
     */
    const char* argDoc;

    /**
     * @brief The description of this option that will be shown in the help message
     */
    const char* doc;
} optDoc_t;

//==============================================================================
// Function Prototypes
//==============================================================================

static bool handleArgument(const char* optName, const char* arg, int optVal);
static void printHelp(const char* progName);
static void printUsage(const char* progName);
static const optDoc_t* findOptDoc(char shortOpt, const char* longOpt);
static const struct option* findOption(char shortOpt, const char* longOpt);
static void getOptionsStr(char* buffer, int buflen);
static void printColWordWrap(const char* text, int* col, int startCol, int wrapCol);

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

static const char mainDoc[] = "Emulates a swadge";

// Long argument name definitions
// These MUST be defined here, so that they are
// the same in both options and argDocs
static const char argFullscreen[] = "fullscreen";
static const char argHideLeds[]   = "hide-leds";
static const char argTouch[]      = "touch";
static const char argHelp[]       = "help";
static const char argUsage[]      = "usage";

// clang-format off
/**
 * @brief The option definitions for getopt
 */
static const struct option options[] =
{
    { argFullscreen, no_argument, (int*)&emulatorArgs.fullscreen,   true },
    { argHideLeds,   no_argument, (int*)&emulatorArgs.hideLeds,     true },
    { argTouch,      no_argument, (int*)&emulatorArgs.emulateTouch, true },
    { argHelp,       no_argument, NULL,                             'h'  },
    { argUsage,      no_argument, NULL,                             0    },

    {0},
};

/**
 * @brief Documentation strings for all options
 */
static const optDoc_t argDocs[] =
{
    {'f', argFullscreen, NULL, "Open in fullscreen mode" },
    { 0,  argHideLeds,   NULL, "Don't draw simulated LEDs next to the display" },
    {'t', argTouch,      NULL, "Simulate touch sensor readings with a virtual touchpad" },
    {'h', argHelp,       NULL, "Give this help list" },
    { 0,  argUsage,      NULL, "Give a short usage message" },
};
// clang-format on

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Handle a command-line option
 *
 * @param optName The name of the option
 * @param arg The string value of this option's argument, or NULL
 * @param optVal The value set in the struct, usually the short option
 * @return true If the argument was handled successfully
 * @return false If there was an error or the program should exit
 */
static bool handleArgument(const char* optName, const char* arg, int optVal)
{
    // Handle arguments with no short-option like this:
    // if (optName == argUsage)
    //{ doSomething(); return true }

    // Handle options with a short-option here:
    switch (optVal)
    {
            // case 'x':
            // doSomething();
            // return true;

        default:
            return false;
    }

    return false;
}

/**
 * @brief Print out the help text for this program, describing all options.
 *
 * @param progName The name of the executable to use in the help message
 */
static void printHelp(const char* progName)
{
    printf("Usage: %s [OPTION...]\n%s\n", progName, mainDoc);

    const optDoc_t* end = (argDocs + ARRAY_SIZE(argDocs));
    for (const optDoc_t* doc = argDocs; doc != end; doc++)
    {
        int col                     = 0;
        const struct option* option = findOption(doc->shortOpt, doc->longOpt);
        bool arg                    = false;
        bool argOptional            = false;

        // option should never be NULL, but might as well check
        if (NULL != option)
        {
            if (option->has_arg == required_argument)
            {
                arg = true;
            }
            else if (option->has_arg == optional_argument)
            {
                arg         = true;
                argOptional = true;
            }
        }

        // Print out the short option, if any
        if (doc->shortOpt)
        {
            // Align with the short-opt column, plus a guaranteed space
            do
            {
                putchar(' ');
            } while (++col < HELP_SHORT_COL);
            col += printf("-%c,", doc->shortOpt);
        }

        // Print out the long option, if any
        if (doc->longOpt)
        {
            // Align with the long-opt column, plus a guaranteed space
            do
            {
                putchar(' ');
            } while (++col < HELP_LONG_COL);
            col += printf("--%s", doc->longOpt);
        }

        // Print out the argument documentation, if it has an arg
        if (arg)
        {
            // Put brackets around an optional arg
            if (argOptional)
            {
                putchar('[');
                col++;
            }

            // Print the arg doc string if it exists, otherwise a generic one
            if (doc->argDoc)
            {
                col += printf("=%s", doc->argDoc);
            }
            else
            {
                col += printf("=OPTION");
            }

            // Close the optional bracket
            if (argOptional)
            {
                putchar(']');
                col++;
            }
        }

        // Finally, print out the actual description, wrapping to the correct column
        if (doc->doc)
        {
            // Align to the description column
            do
            {
                putchar(' ');
            } while (++col < HELP_DESC_COL);

            printColWordWrap(doc->doc, &col, HELP_DESC_COL, HELP_WRAP_COL);
        }

        // Newline after each option
        putchar('\n');
    }
}

/**
 * @brief Prints a brief usage message which lists valid options only
 *
 * @param progName The executable name to use in the usage message
 */
static void printUsage(const char* progName)
{
    char shortOpts[ARRAY_SIZE(argDocs) + 1];
    char buffer[128];
    char* shortOut = shortOpts;
    int col        = 0;

    // Make sure it's null-terminated no matter what
    *(shortOut) = '\0';

    col += printf("Usage: %s", progName);

    // Do this as 2 steps to keep the code shorter
    // First step: We build the short-options string out, then print it
    // Second step: We print out each argument's long options
    for (uint8_t step = 0; step < 2; step++)
    {
        const optDoc_t* end = (argDocs + ARRAY_SIZE(argDocs));
        for (const optDoc_t* doc = argDocs; doc != end; doc++)
        {
            const struct option* option = findOption(doc->shortOpt, doc->longOpt);

            if (step == 0)
            {
                // Just build up the short-opt string
                if (doc->shortOpt)
                {
                    // Use the short-opt from the docs if we have it
                    *(shortOut++) = doc->shortOpt;
                }
                else if (option && option->val && option->val > ' ' && option->val <= '~')
                {
                    // Otherwise, check if the option was found and has a printable val.
                    *(shortOut++) = option->val;
                }
            }
            else
            {
                // Print out the long-opt string
                bool arg         = false;
                bool argOptional = false;

                if (NULL != option)
                {
                    if (option->has_arg == required_argument)
                    {
                        arg = true;
                    }
                    else if (option->has_arg == optional_argument)
                    {
                        arg         = true;
                        argOptional = true;
                    }
                }

                if (arg || doc->argDoc)
                {
                    // Write the option with its argument hint
                    const char* argName = doc->argDoc ? doc->argDoc : "OPTION";
                    snprintf(buffer, sizeof(buffer) - 1, argOptional ? " [--%s[=%s]]" : "[--%s=%s]", doc->longOpt,
                             argName);
                }
                else
                {
                    // Write the option name, no arguments
                    snprintf(buffer, sizeof(buffer) - 1, " [--%s]", doc->longOpt);
                }
                printColWordWrap(buffer, &col, HELP_USAGE_COL, HELP_WRAP_COL);
            }
        }

        if (step == 0)
        {
            // Print out the short-opt string we just finished building
            *(shortOut) = '\0';
            snprintf(buffer, sizeof(buffer) - 1, " [-%s]", shortOpts);
            printColWordWrap(buffer, &col, HELP_USAGE_COL, HELP_WRAP_COL);
        }
        else
        {
            // Print a line at the end of the usage
            printf("\n");
        }
    }
}

/**
 * @brief Finds an option's documentation info by either short or long opt
 *
 * NOTE: This does not perform a string comparison for longOpt, so the same constant
 * string must be used.
 *
 * @param shortOpt If non-zero, any option with this short option will be returned
 * @param longOpt If non-NULL, any option with this long option will be returned
 * @return const optDoc_t*
 */
static const optDoc_t* findOptDoc(char shortOpt, const char* longOpt)
{
    const optDoc_t* end = (argDocs + ARRAY_SIZE(argDocs));
    for (const optDoc_t* doc = argDocs; doc != end; doc++)
    {
        if ((shortOpt && doc->shortOpt == shortOpt) || (longOpt && doc->longOpt == longOpt))
        {
            return doc;
        }
    }

    return NULL;
}

/**
 * @brief Finds an option's ::option definition by either short or long opt
 *
 * @param shortOpt If non-zero, any option with this short option will be returned
 * @param longOpt If non-NULL, any option with this long option will be returned
 * @return const struct option
 */
static const struct option* findOption(char shortOpt, const char* longOpt)
{
    const struct option* opt = options;

    while (opt->name != NULL)
    {
        if ((opt->val > ' ' && opt->val <= '~' && opt->val == shortOpt) || opt->name == longOpt)
        {
            return opt;
        }

        opt++;
    }

    return NULL;
}

/**
 * @brief Write the value for the short \c options argument to getopt_long()
 *
 * \c buffer should have at least 3 characters for each option, plus 2
 *
 * @param buffer A buffer to write the option string into
 * @param buflen The maximum number of characters to write to \c buffer
 */
static void getOptionsStr(char* buffer, int buflen)
{
    // pointer to our output into buffer
    char* out = buffer;

    if (buflen < 2)
    {
        return;
    }

// Macro to make it easy to properly check for buffer overruns
#define CHECK_BUFFER                \
    if (out + 1 == buffer + buflen) \
    {                               \
        break;                      \
    }

    // If we want to stop getopt from printing its own errors for missing args,
    // we can add a colon to the beginning of the string:
    *(out++) = ':';

    const optDoc_t* end = (argDocs + ARRAY_SIZE(argDocs));
    for (const optDoc_t* doc = argDocs; doc != end; doc++)
    {
        const struct option* option = findOption(doc->shortOpt, doc->longOpt);

        if (doc->shortOpt)
        {
            *(out++) = doc->shortOpt;
            CHECK_BUFFER

            // option should never be NULL, but might as well check
            if (NULL != option)
            {
                if (option->has_arg == required_argument)
                {
                    *(out++) = ':';
                    CHECK_BUFFER
                }
                else if (option->has_arg == optional_argument)
                {
                    // Optional arguments get two colons following
                    *(out++) = ':';
                    CHECK_BUFFER
                    *(out++) = ':';
                    CHECK_BUFFER
                }
            }
        }
    }

#undef CHECK_BUFFER

    *out = '\0';
}

/**
 * @brief Print a string to stdout with word-wrap and indentation
 *
 * @param text The string to print
 * @param col A pointer to an integer that holds the current colunm to print at
 * @param startCol The position to resume writing at after wrapping
 * @param wrapCol The column to start wrapping at
 */
static void printColWordWrap(const char* text, int* col, int startCol, int wrapCol)
{
    const char* ptr = text;
    bool firstLine  = false;
    int nextSpace, nextBreak;
    char buf[64];

    if (*col < startCol)
    {
        firstLine = true;
    }

    while (*ptr)
    {
        if (!firstLine)
        {
            // Unless this is the first line, skip to the starting column
            while (*col < startCol)
            {
                putchar(' ');
                (*col)++;
            }
        }

        // Handle embedded newlines here so we can indent properly
        if (*ptr == '\n')
        {
            // Newline!
            firstLine = false;
            putchar(*(ptr++));
            *col = 0;
            continue;
        }

        // Find the next space in the string
        nextSpace = strchr(ptr, ' ') - ptr;

        // Copy a chunk of text into our buffer
        strncpy(buf, ptr, sizeof(buf) - 1);
        buf[sizeof(buf) - 1] = '\0';

        // Worst-case, break the string at the end of the buffer
        nextBreak = strlen(buf);

        if (nextSpace >= 0 && nextSpace < nextBreak)
        {
            // The next space is before the buffer end, so end right after that
            nextBreak = nextSpace + 1;
        }

        buf[nextBreak] = '\0';

        // Check if the remaining string is too long to fit without a force-break
        if (startCol + strlen(buf) > wrapCol)
        {
            // It is, so force-break it
            while ((*col) + strlen(buf) > wrapCol)
            {
                buf[--nextBreak] = '\0';
            }
        }

        // Now the string won't fit or we're out of buffer, so wrap
        if ((*col) + strlen(buf) > wrapCol || nextBreak == 0)
        {
            // Wrap!
            firstLine = false;
            putchar('\n');
            *col = 0;
            continue;
        }

        // Actually print out the buffer and move the text pointer
        *col += printf("%s", buf);
        ptr += nextBreak;
    }
}

/**
 * @brief Parse the emulator's arguments and set up ::emulatorArgs
 *
 * @param argc The number of command-line arguments
 * @param argv The command-line arguments array
 * @return true If the command-line arguments were successfully parsed
 * @return false If there was an error parsing the arguments
 */
bool emuParseArgs(int argc, char** argv)
{
    const char* executableName       = *argv;
    const char* prettyExecutableName = executableName + strlen(executableName);

    // Prettify the executable name by working backwards to strip everything
    //  before the first slash or backslash
    while (prettyExecutableName > executableName && *(prettyExecutableName - 1) != '/'
           && *(prettyExecutableName - 1) != '\\')
    {
        --prettyExecutableName;
    }

    // Get a buffer big enough to hold all possible option chars
    char shortOpts[ARRAY_SIZE(options) * 3 + 4];
    *shortOpts = '\0';

    // Generate the short-option string that getopt wants from our own doc struct
    getOptionsStr(shortOpts, sizeof(shortOpts));

    while (true)
    {
        int optIndex                = -1;
        int optVal                  = getopt_long(argc, argv, shortOpts, options, &optIndex);
        const struct option* option = NULL;
        const char* optName         = NULL;
        const char* optArg          = optarg;

        if (optVal < 0)
        {
            // No more options
            break;
        }
        else if (optVal == '?')
        {
            // Handle unknown argument
            // exit(1);
            return false;
        }
        else if (optVal == ':')
        {
            // Unknown argument, print custom error message
            // (Not In Use)
            printf("%1$s: unrecognized option '%3$s'\nTry `%2$s --help` or `%2$s --usage` for more\ninformation.\n",
                   executableName, prettyExecutableName, optarg);
        }

        // This was a short option,
        if (optIndex != -1)
        {
            option = options + optIndex;
        }

        const optDoc_t* optDoc = findOptDoc(optVal, option ? option->name : NULL);

        if (NULL == option && optVal != 0)
        {
            // Try to find an option with the short-option
            option = findOption(optVal, optDoc ? optDoc->longOpt : NULL);

            if (NULL != option)
            {
                optName = option->name;
                if (NULL != option->flag)
                {
                    // A long-opt matching this short-option was found
                    //
                    // Do what getopt would  have done for the long opt:
                    // Set the flag to the option val
                    // This we we don't need to handle this manually anyway just for the short opt
                    *(option->flag) = option->val;
                }
            }
        }

        if (NULL == optName && NULL != optDoc)
        {
            optName = optDoc->longOpt;
        }

        if(!optArg
           && NULL != argv[optind]
           && '-' != *(argv[optind]))
        {
            // This makes optional arguments work even if you don't connect them with the '='
           optArg = argv[optind++];
        }

        // Ok, now for the case of an option which has no short-opt in the getopt options struct,
        // but we have assigned one with the optDocs. So, we should... if we have a short opt,
        // look for a long opt which also has that option
        // and, if the option has a non-null flag, set that to the value.

        int optKey = optVal;

        if (optVal == 0)
        {
            // This option has no short opt, or at least is being called by its long opt
            // Let's... see if there's anything else we can do.
            if (NULL != optDoc && optDoc->shortOpt > 0)
            {
                // The optDocs have a short option that the getopt options did not!
                // So, replace optVal with that to make processing easier
                optKey = optDoc->shortOpt;
            }
            else
            {
                // There is no optDoc or it has no shortOpt, just use 0
                optKey = 0;
            }
        }

        if (optKey == '?')
        {
            return false;
        }
        else if (optKey == 'h')
        {
            printHelp(prettyExecutableName);
            return false;
        }
        else if (optName == argUsage)
        {
            printUsage(prettyExecutableName);
            return false;
        }

        if (!handleArgument(optName ? optName : NULL, optArg, optKey))
        {
            // handleArgument() returned error, so should we
            return false;
        }
    }

    return true;
}
