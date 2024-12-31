#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "midiPlayer.h"
#include "midiFileParser.h"
#include "macros.h"

#include "args.h"

#include "midi_dump.h"

typedef struct
{
    const char* name;
    bool (*func)(void);
} mdAction_t;

static const mdAction_t actions[] = {
    {"dump", action_dump},
};

int main(int argc, char** argv)
{
    bool result = mdParseArgs(argc, argv);

    if (!result)
    {
        printf("Invalid arguments!\n");
        return 1;
    }

    const mdAction_t* selectedAction = actions;

    for (const mdAction_t* act = actions; act < (act + ARRAY_SIZE(actions)); act++)
    {
        // Compare only up to the length of the argument value, so abbreviated commands work
        if (!strncmp(act->name, mdArgs.action, strlen(mdArgs.action)))
        {
            selectedAction = act;
            break;
        }
    }

    printf("Midi FILE: %s\n", mdArgs.midiIn);

    fprintf(stderr, "%s %s\n", selectedAction->name, mdArgs.midiIn);

    bool actionResult = selectedAction->func();
    if (!actionResult)
    {
        fprintf(stderr, "%s failed!\n", selectedAction->name);
        return 2;
    }

    return 0;
}
