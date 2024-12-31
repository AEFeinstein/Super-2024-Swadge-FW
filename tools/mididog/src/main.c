#include <stdio.h>
#include <stdbool.h>

#include "midiPlayer.h"
#include "midiFileParser.h"

#include "args.h"

int main(int argc, char** argv)
{
    bool result = mdParseArgs(argc, argv);

    if (!result)
    {
        printf("Invalid arguments!\n");
        return 1;
    }

    printf("Midi FILE: %s\n", mdArgs.midiFile);

    return 0;
}
