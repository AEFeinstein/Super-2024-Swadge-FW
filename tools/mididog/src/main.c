#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "midiPlayer.h"
#include "midiFileParser.h"
#include "macros.h"

#include "args.h"
#include "utils.h"

#include "midi_dump.h"

typedef struct
{
    const char* name;
    bool (*func)(void);
} mdAction_t;

static bool action_shrink(void);

static const mdAction_t actions[] = {
    {"dump", action_dump},
    {"shrink", action_shrink},
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
        if (act->name && mdArgs.action && !strncmp(act->name, mdArgs.action, strlen(mdArgs.action)))
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

static bool action_shrink(void)
{
    if (!mdArgs.midiIn)
    {
        fprintf(stderr, "ERR: --input file is required for shrink!\n");
        return false;
    }

    if (!mdArgs.midiOut)
    {
        mdArgs.midiOut = "-";
    }

    midiFile_t* file = mididogLoadPath(mdArgs.midiIn);
    midiFile_t* tokFile = mididogTokenizeMidi(file);

    // Do any manipulations to the file events here, directly
    for (int trackNum = 0; trackNum < tokFile->trackCount; trackNum++)
    {
        for (int eventNum = 0; eventNum < tokFile->events[trackNum].count; eventNum++)
        {
            midiEvent_t* event = &tokFile->events[trackNum].events[eventNum];

            if (event->type == MIDI_EVENT && (event->midi.status & 0xF0) == 0x80)
            {
                // Change NOTE OFF to NOTE ON with velocity
                event->midi.status = 0x90 | (event->midi.status & 0x0F);
                event->midi.data[1] = 0;
            }
            else if (event->type == META_EVENT && (((uint8_t)event->meta.type) <= 0x0F) && mdArgs.stripText)
            {
                printf("Stripping text\n");
                event->type = NO_EVENT;
            }
        }
    }


    // End manipulations

    midiFile_t* outFile = mididogUnTokenizeMidi(tokFile);
    if (!mididogWritePath(outFile, mdArgs.midiOut))
    {
        fprintf(stderr, "ERR: failed to write MIDI file to output %s\n", mdArgs.midiOut);
    }

    unloadMidiFile(outFile);
    free(outFile);

    unloadMidiFile(tokFile);
    free(tokFile);
    
    unloadMidiFile(file);
    free(file);
    
    return true;
}