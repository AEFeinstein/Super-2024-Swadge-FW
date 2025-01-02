#include "midi_dump.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "args.h"

bool action_dump(void)
{
    midiFile_t* file = mididogLoadPath(mdArgs.midiIn);

    if (NULL == file)
    {
        return false;
    }

    midiFile_t* tokFile = mididogTokenizeMidi(file);
    if (NULL == tokFile)
    {
        unloadMidiFile(file);
        free(file);
        return false;
    }

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
                    case 0x80:
                    {
                        char noteName[16] = {0};
                        writeNoteName(noteName, sizeof(noteName), 0, event->midi.data[0]);

                        fputs("  Note Off\n", stream);
                        fprintf(stream, "   - Channel: %hhx\n", channel + 1);
                        fprintf(stream, "   - Note: %hhu (%s)\n", event->midi.data[0], noteName);
                        break;
                    }
                    case 0x90:
                    {
                        char noteName[16] = {0};
                        writeNoteName(noteName, sizeof(noteName), 0, event->midi.data[0]);

                        if (event->midi.data[1] == 0)
                        {
                            fputs("  Note On*\n", stream);
                        }
                        else
                        {
                            fputs("  Note On\n", stream);
                        }
                        fprintf(stream, "   - Channel: %hhx\n", channel + 1);
                        fprintf(stream, "   - Note: %hhu (%s)\n", event->midi.data[0], noteName);
                        fprintf(stream, "   - Velocity: %hhu\n", event->midi.data[1]);
                        break;
                    }
                    case 0xA0:
                    {
                        char noteName[16] = {0};
                        writeNoteName(noteName, sizeof(noteName), 0, event->midi.data[0]);

                        fputs("  AfterTouch\n", stream);
                        fprintf(stream, "   - Note: %hhu (%s)\n", event->midi.data[0], noteName);
                        fprintf(stream, "   - Velocity: %hhu\n", event->midi.data[1]);
                        
                        break;
                    }
                    case 0xB0:
                    {
                        fputs("  Control Change\n", stream);
                        fprintf(stream, "   - Control: %hhu\n", event->midi.data[0]);
                        fprintf(stream, "   - Value: %1$hhu\n (0x%1$hhx)\n", event->midi.data[1]);
                        break;
                    }
                    case 0xC0:
                    {
                        fputs("  Program Change\n", stream);
                        fprintf(stream, "   - Program: %hhu\n", event->midi.data[0]);
                        break;
                    }
                    case 0xD0:
                    {
                        fputs("  Channel Pressure\n", stream);
                        fprintf(stream, "   - Pressure: %hhu\n", event->midi.data[0]);
                        break;
                    }
                    case 0xE0:
                    {
                        // Get 14-bit value for pitch bend
                        uint16_t pitch = ((event->midi.data[1] & 0x7F) << 7) | (event->midi.data[0] & 0x7F);
                        int32_t bendCents = (((int16_t)-0x2000) + pitch) * 100 / 0x1FFF;
                        fputs("  Pitch Bend\n", stream);

                        fprintf(stream, "   - Pitch Bend: %04" PRIx16 " (%+" PRId32 " cents)\n", pitch, bendCents);
                        break;
                    }
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

            if (ml)
            {
                
            }
            else
            {
                bool text = false;
                switch (event->meta.type)
                {
                    case SEQUENCE_NUMBER:
                    {
                        fprintf(stream, " Seq#");
                        text = true;
                        break;
                    }
                    case TEXT:
                    {
                        fprintf(stream, " Text");
                        text = true;
                        break;
                    }
                    case COPYRIGHT:
                    {
                        fprintf(stream, " (C)");
                        text = true;
                        break;
                    }
                    case SEQUENCE_OR_TRACK_NAME:
                    {
                        fprintf(stream, " Name");
                        text = true;
                        break;
                    }
                    case INSTRUMENT_NAME:
                    {
                        fprintf(stream, " Inst");
                        text = true;
                        break;
                    }
                    case LYRIC:
                    {
                        fprintf(stream, " Lyric");
                        text = true;
                        break;
                    }
                    case MARKER:
                    {
                        fprintf(stream, " Marker");
                        text = true;
                        break;
                    }
                    case CUE_POINT:
                    {
                        fprintf(stream, " Cue");
                        text = true;
                        break;
                    }
                    // Other text, unknown specific types
                    case 8:
                    case 9:
                    case 10:
                    case 11:
                    case 12:
                    case 13:
                    case 14:
                    case 15:
                    {
                        fprintf(stream, " Text?");
                        text = true;
                        break;
                    }
                    case CHANNEL_PREFIX:
                    {
                        fprintf(stream, " Ch. Prefix");
                        break;
                    }
                    case PORT_PREFIX:
                    {
                        fprintf(stream, " Port Prefix");
                        break;
                    }
                    case END_OF_TRACK:
                    {
                        fprintf(stream, " EOT");
                        break;
                    }
                    case TEMPO:
                    {
                        fprintf(stream, " Tempo %d", event->meta.tempo);
                        break;
                    }
                    case SMPTE_OFFSET:
                    {
                        fprintf(stream, " SMPTE");
                        break;
                    }
                    case TIME_SIGNATURE:
                    {
                        fprintf(stream, " TimeSig");
                        break;
                    }
                    case KEY_SIGNATURE:
                    {
                        fprintf(stream, " KeySig");
                        break;
                    }
                    case PROPRIETARY:
                    {
                        fprintf(stream, " Proprietary");
                        break;
                    }
                }

                if (text)
                {
                    if (event->meta.length < 64)
                    {
                        fprintf(stream, " \"%.*s\"", (int)event->meta.length, event->meta.text);
                    }
                    else
                    {
                        fprintf(stream, " \"%.*s\" ...", 64, event->meta.text);
                    }
                }
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
        // Extra blank line to end a multi-line output
        putc('\n', stream);
    }
}

void printEvent(const midiEvent_t* event)
{
    fprintEvent(stdout, 0, event);
}
