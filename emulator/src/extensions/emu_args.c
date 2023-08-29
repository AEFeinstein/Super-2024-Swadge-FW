//==============================================================================
// Includes
//==============================================================================

// Includes mostly for getopt
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <getopt.h>
//#include "getopt_win.h"

#include "emu_args.h"
#include "macros.h"

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

static void printHelp(const char* progName);
static void printUsage(const char* progName);
static const optDoc_t* findOptDoc(char shortOpt, const char* longOpt);
static const struct option* findOption(char shortOpt, const char* longOpt);
static void getOptionsStr(char* buffer, int buflen);
static void printColWordWrap(const char* text, int *col, int startCol, int wrapCol);

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

static const char argFullscreen[] = "fullscreen";
static const char argHideLeds[]   = "hide-leds";
static const char argTouch[]      = "touch";
static const char argHelp[]       = "help";
static const char argUsage[]      = "usage";

// clang-format off
static const struct option options[] =
{
    { argFullscreen, no_argument, (int*)&emulatorArgs.fullscreen,   true },
    { argHideLeds,   no_argument, (int*)&emulatorArgs.hideLeds,     true },
    { argTouch,      no_argument, (int*)&emulatorArgs.emulateTouch, true },
    { argHelp,       no_argument, NULL,                             'h'  },
    { argUsage,      no_argument, NULL,                             0    },

    {0},
};
// clang-format on

/**
 * @brief Documentation strings for all options
 *
 */
static const optDoc_t argDocs[] =
{
    { 'f', argFullscreen, NULL, "Open in fullscreen mode" },
    {  0,  argHideLeds,   NULL, "Don't draw simulated LEDs next to the display" },
    { 't', argTouch,      NULL, "Simulate touch sensor readings with a virtual touchpad" },
    { 'h', argHelp,       NULL, "Give this help list" },
    {  0,  argUsage,      NULL, "Give a short usage message" },
};

//==============================================================================
// Functions
//==============================================================================

/*Usage: swadge_emulator [OPTION...]
Emulates a swadge

  -f, --fullscreen           Open in fullscreen mode
  -l, --hide-leds            Don't draw simulated LEDs next to the display
  -t, --touch                Simulate touch sensor readings with a virtual
                             touchpads
  -?, --help                 Give this help list
      --usage                Give a short usage message
*/

static void printHelp(const char* progName)
{
    int shortCol = 1;
    int longCol = 5;
    int textCol = 29;
    int margin = 80;

    printf("Usage: %s [OPTION...]\n%s\n", progName, mainDoc);

#define INDENT(COLNUM) do { putchar(' '); } while (++col < COLNUM)

    const optDoc_t* end = (argDocs + ARRAY_SIZE(argDocs));
    for (const optDoc_t* doc = argDocs; doc != end; doc++)
    {
        int col = 0;
        const struct option* option = findOption(doc->shortOpt, doc->longOpt);
        bool arg = false;
        bool argOptional = false;

        // option should never be NULL, but might as well check
        if (NULL != option)
        {
            if (option->has_arg == required_argument)
            {
                arg = true;
            }
            else if (option->has_arg == optional_argument)
            {
                arg = true;
                argOptional = true;
            }
        }

        if (doc->shortOpt)
        {
            INDENT(shortCol);
            col += printf("-%c,", doc->shortOpt);
        }

        if (doc->longOpt)
        {
            INDENT(longCol);
            col += printf("--%s", doc->longOpt);
        }

        if (arg)
        {
            if (argOptional)
            {
                putchar('[');
                col++;
            }

            if (doc->argDoc)
            {
                col += printf("=%s", doc->argDoc);
            }
            else
            {
                col += printf("=OPTION");
            }

            if (argOptional)
            {
                putchar(']');
                col++;
            }
        }

        if (doc->doc)
        {
            printColWordWrap(doc->doc, &col, textCol, margin);
        }

        putchar('\n');
    }

#undef INDENT
}

/*Usage: swadge_emulator [-flt?] [--fullscreen] [--hide-leds] [--touch] [--help]
            [--usage]
*/

static void printUsage(const char* progName)
{
    char shortOpts[ARRAY_SIZE(argDocs) + 1];
    char buffer[128];
    char* shortOut = shortOpts;
    int col = 0;

    // Make sure it's null-terminated no matter what
    *(shortOut) = '\0';

    col += printf("Usage: %s", progName);

    for (uint8_t step = 0; step < 2; step++)
    {
        const optDoc_t* end = (argDocs + ARRAY_SIZE(argDocs));
        for (const optDoc_t* doc = argDocs; doc != end; doc++)
        {
            const struct option* option = findOption(doc->shortOpt, doc->longOpt);

            if (step == 0)
            {
                if (doc->shortOpt)
                {
                    *(shortOut++) = doc->shortOpt;
                }
                else if (option && option->val && option->val > ' ' && option->val <= '~')
                {
                    *(shortOut++) = option->val;
                }
            }
            else
            {
                snprintf(buffer, sizeof(buffer) - 1, " [--%s]", doc->longOpt);
                printColWordWrap(buffer, &col, 12, 80);
            }
        }

        if (step == 0)
        {
            *(shortOut) = '\0';
            snprintf(buffer, sizeof(buffer) - 1, " [-%s]", shortOpts);
            printColWordWrap(buffer, &col, 0, 80);
        }
        else
        {
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
        if ((opt->val > ' ' && opt->val <= '~' && opt->val == shortOpt)
            || opt->name == longOpt)
        {
            return opt;
        }

        opt++;
    }

    return NULL;
}

/**
 * @brief Write the value for the \c options argument to getopt_long()
 *
 * @param buffer
 * @param buflen
 */
static void getOptionsStr(char* buffer, int buflen)
{
    // pointer to our output into buffer
    char* out = buffer;

    if (buflen == 0)
    {
        return;
    }

// Macro to make it easy to properly check for buffer overruns
#define CHECK_BUFFER if (out + 1 == buffer + buflen) { break; }

    // If we want to stop getopt from printing its own errors for missing args,
    // we can add a colon to the beginning of the string:
    // *(out++) = ':';
    // CHECK_BUFFER
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

static void printColWordWrap(const char* text, int *col, int startCol, int wrapCol)
{
    const char* ptr = text;
    int nextSpace, nextBreak;
    char buf[64];

    while (*ptr)
    {
        while (*col < startCol)
        {
            putchar(' ');
            (*col)++;
        }

        if (*ptr == '\n')
        {
            // Newline!
            putchar(*(ptr++));
            *col = 0;
            continue;
        }

        nextSpace = strchr(ptr, ' ') - ptr;

        strncpy(buf, ptr, sizeof(buf) - 1);
        buf[sizeof(buf) - 1] = '\0';

        nextBreak = strlen(buf);

        if (nextSpace >= 0 && nextSpace < nextBreak)
        {
            nextBreak = nextSpace + 1;
        }

        buf[nextBreak] = '\0';

        if (startCol + strlen(buf) > wrapCol)
        {
            while ((*col) + strlen(buf) > wrapCol)
            {
                buf[--nextBreak] = '\0';
            }
        }

        if ((*col) + strlen(buf) > wrapCol || nextBreak == 0)
        {
            putchar('\n');
            *col = 0;
            continue;
        }

        *col += printf("%s", buf);
        ptr += nextBreak;
    }
}

bool emuParseArgs(int argc, char** argv)
{
    const char* executableName = *argv;
    const char* prettyExecutableName = executableName + strlen(executableName);

    // Prettify the executable name by working backwards to strip everything
    //  before the first slash or backslash
    while (prettyExecutableName > executableName
           && *(prettyExecutableName - 1) != '/'
           && *(prettyExecutableName - 1) != '\\')
    {
        --prettyExecutableName;
    }

    // Get a buffer big enough to hold all possible option chars
    char shortOpts[ARRAY_SIZE(options) * 2 + 1];
    *shortOpts = '\0';

    // Generate the short-option string that getopt wants from our own doc struct
    getOptionsStr(shortOpts, sizeof(shortOpts));

    while (true)
    {
        int optIndex = -1;
        int optVal = getopt_long(argc, argv, shortOpts, options, &optIndex);
        const struct option* option = NULL;

        if (optVal < 0)
        {
            // No more options
            break;
        }
        else if (optVal == '?')
        {
            // Handle unknown argument
            //exit(1);
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
                // There is no optDoc or it has no shortOpt
                // So, let's... just use the string constant pointer. It's fine.
                optKey = option ? (int)option->name : '?';
            }
        }

        if (optKey == argUsage)
        {
            printUsage(prettyExecutableName);
            return false;
        }
        else if (optKey == 'h')
        {
            printHelp(prettyExecutableName);
            return false;
        }
    }

    return true;
}
