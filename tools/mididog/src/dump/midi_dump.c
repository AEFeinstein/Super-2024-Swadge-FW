#include "midi_dump.h"

#include <inttypes.h>e
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "args.h"

bool action_dump(void)
{
    midiFile_t* file = mididogLoadPath(mdArgs.midiIn);
    midiFile_t* tokFile = mididogTokenizeMidi(file);

    /*midiFileReader_t reader = {0};
    if (initMidiParser(&reader, file))
    {
        midiEvent_t event;

        while (midiNextEvent(&reader, &event))
        {
            fprintEvent(stdout, mdArgs.multiLine ? MD_DUMP_MULTILINE : 0, &event);
        }

        deinitMidiParser(&reader);
    }*/

    for (int trackNum = 0; trackNum < tokFile->trackCount; trackNum++)
    {
        fprintf(stdout, "Track %d\n", trackNum);
        for (int eventNum = 0; eventNum < tokFile->events[trackNum].count; eventNum++)
        {
            midiEvent_t* event = &tokFile->events[trackNum].events[eventNum];

            fprintEvent(stdout, mdArgs.multiLine ? MD_DUMP_MULTILINE : 0, event);
        }
    }

    unloadMidiFile(tokFile);
    unloadMidiFile(file);
    free(file);
    
    return true;
}

void fprintEvent(FILE* stream, int flags, const midiEvent_t* event)
{
    /// Extract the multi-line flag, for convenience
    bool ml = ((flags & MD_DUMP_MULTILINE) == MD_DUMP_MULTILINE);

    if (event == NULL)
    {
        fputs("(NULL)\n", stream);
        if (ml)
        {
            putc('\n', stream);
        }
        return;
    }
    
    if (ml)
    {
        fprintf(stream, "Time (abs): %8" PRIu32 "\n", event->absTime);
        fprintf(stream, "Delta Time: %8" PRIu32 "\n", event->deltaTime);
    }
    else
    {
        fprintf(stream, "%8" PRIu32 " ", event->deltaTime);
    }

    switch (event->type)
    {
        case MIDI_EVENT:
        {
            uint8_t channel = (event->midi.status & 0x0F);
            uint8_t status = (event->midi.status & 0xF0);

            // Measure the length of the next event without writing it anywhere
            int len = midiWriteEvent(NULL, 1024, event);

            if (ml)
            {
                fputs("MIDI Event\n", stream);

                switch (status)
                {
                    case 0x80: fputs("  Note Off\n", stream); break;
                    case 0x90: fputs("  Note On\n", stream); break;
                    case 0xA0: fputs("  AfterTouch\n", stream); break;
                    case 0xB0: fputs("  Control Change\n", stream); break;
                    case 0xC0: fputs("  Program Change\n", stream); break;
                    case 0xD0: fputs("  Channel Pressure\n", stream); break;
                    case 0xE0: fputs("  Pitch Bend\n", stream); break;
                    case 0xF0:
                    {
                        if (event->midi.status > 0xF7)
                        {
                            fputs("  System Realtime\n", stream);
                        }
                        else
                        {
                            fputs("  System Common\n", stream);
                        }
                        break;
                    }
                    default:
                    {
                        fputs("  Unknown\n", stream);
                    }
                }
            }
            else
            {
                putc(' ', stream);

                fprintf(stream, "%02hhx ", event->midi.status);
                if (len > 1)
                {
                    fprintf(stream, "%02hhx ", event->midi.data[0]);
                    
                    if (len > 2)
                    {
                        fprintf(stream, "%02hhx ", event->midi.data[1]);
                    }
                    else
                    {
                        fputs("   ", stream);
                    }
                }
                else
                {
                    fputs("      ", stream);
                }

                switch (status)
                {
                    case 0x80:
                    {
                        fputs("  Note Off", stream);
                        break;
                    }
                    case 0x90:
                    {
                        fputs("  Note On", stream);
                        break;
                    }
                    case 0xA0:
                    {
                         fputs("  AfterTouch", stream);
                         break;
                    }
                    case 0xB0:
                    {
                        fputs("  Control Change", stream);
                        break;
                    }
                    case 0xC0:
                    {
                        fputs("  Program Change", stream);
                        break;
                    }
                    case 0xD0:
                    {
                        fputs("  Channel Pressure", stream);
                        break;
                    }
                    case 0xE0:
                    {
                        fputs("  Pitch Bend", stream);
                        break;
                    }
                    case 0xF0:
                    {
                        if (event->midi.status > 0xF7)
                        {
                            fputs("  System Realtime", stream);
                        }
                        else
                        {
                            fputs("  System Common", stream);
                        }
                        break;
                    }
                    default:
                    {
                        fputs("  Unknown", stream);
                        break;
                    }
                }
            }
            break;
        }

        case META_EVENT:
        {
            if (ml)
            {
                fputs("Meta Event\n", stream);
            }
            else
            {
                putc('M', stream);
            }
            break;
        }

        case SYSEX_EVENT:
        {
            if (ml)
            {
                fputs("SysEx Event\n", stream);
            }
            else
            {
                putc('X', stream);
            }
            break;
        }
    }

    // End of Line
    putc('\n', stream);
    if (ml)
    {
        putc('\n', stream);
    }
}

void printEvent(const midiEvent_t* event)
{
    fprintEvent(stdout, 0, event);
}
