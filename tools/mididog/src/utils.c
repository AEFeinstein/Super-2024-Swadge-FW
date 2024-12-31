#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "midiFileParser.h"

#include "utils.h"

midiFile_t* mididogLoadFile(FILE* stream)
{
    // Seek to end of file
    errno = 0;
    if (0 != fseek(stream, 0, SEEK_END))
    {
        fprintf(stderr, "ERR: Error code %d when reading MIDI file: %s\n", errno, strerror(errno));
        return NULL;
    }

    // Get new position in file to determine length
    errno = 0;
    long len = ftell(stream);
    if (len < 0)
    {
        fprintf(stderr, "ERR: Error code %d when reading MIDI file: %s\n", errno, strerror(errno));
        return NULL;
    }

    // Rewind to the beginning of the file
    errno = 0;
    if (0 != fseek(stream, 0, SEEK_SET))
    {
        fprintf(stderr, "ERR: Error code %d when reading MIDI file: %s\n", errno, strerror(errno));
        return NULL;
    }

    uint8_t* data = malloc(len);

    if (NULL == data)
    {
        fprintf(stderr, "ERR: Failed to allocate %ld bytes for loading MIDI file\n", len);
        return NULL;
    }

    size_t read = 0;

    do
    {
        read += fread(data + read, 1, len - read, stream);

        if (read < len)
        {
            int err = ferror(stream);

            // Ignore EAGAIN since it's not a "real" error in this situation
            if (err != 0 && err != EAGAIN)
            {
                fprintf(stderr, "ERR: Error code %d when reading MIDI file from stream: %s\n", err, strerror(err));
                free(data);
                return NULL;
            }

            if (feof(stream))
            {
                fprintf(stderr, "ERR: Reached EOF when reading MIDI file from stream\n");
                free(data);
                return NULL;
            }
        }
    } while (read < len);

    midiFile_t* result = calloc(1, sizeof(midiFile_t));

    if (NULL == result)
    {
        fprintf(stderr, "ERR: Unable to allocate %zu bytes for MIDI file object\n", sizeof(midiFile_t));
        free(data);
        return NULL;
    }

    if (!loadMidiData(data, len, result))
    {
        fprintf(stderr, "ERR: Unable to parse MIDI file from stream\n");
        free(result);
        free(data);
        return NULL;
    }

    return result;
}

midiFile_t* mididogLoadPath(const char* path)
{
    FILE * stream = NULL;

    if (!strcmp("-", path))
    {
        stream = stdin;
    }
    else
    {
        stream = fopen(path, "rb");
    }

    if (stream != NULL)
    {
        midiFile_t* result = mididogLoadFile(stream);
        if (stream != stdin)
        {
            fclose(stream);
        }
        return result;
    }

    return NULL;
}

bool mididogWriteFile(const midiFile_t* data, FILE* stream)
{
    uint8_t buffer[64];
    size_t len = 0;
    size_t written = 0;

    len = midiWriteHeader(buffer, sizeof(buffer), data);

    written = fwrite(buffer, 1, len, stream);

    if (written < len)
    {
        int err = ferror(stream);
        if (err != 0)
        {
            fprintf(stderr, "ERR: Error code %d when writing MIDI file header to stream: %s\n", err, strerror(err));
            return false;
        }

        if (feof(stream))
        {
            fprintf(stderr, "ERR: Reached EOF when writing MIDI file header to stream\n");
            return false;
        }
    }

    if (data->data != NULL)
    {
        // Write data out directly
        written = 0;
        // TODO

        return false;
    }
    else
    {
        // Write data from event stream
        for (uint8_t trackNum = 0; trackNum < data->trackCount; trackNum++)
        {
            // TODO: Write track chunk header
            // - Includes calculating full length of track

            uint8_t runningStatus = 0;

            const midiEventStream_t* events = &data->events[trackNum];
            for (uint32_t eventNum = 0; eventNum < events->count; eventNum++)
            {
                const midiEvent_t* event = &events->events[eventNum];

                // TODO: Write relative timestamp

                len = midiWriteEventWithRunningStatus(buffer, sizeof(buffer), event, &runningStatus);
                written = fwrite(buffer, 1, len, stream);

                if (written < len)
                {
                    int err = ferror(stream);
                    if (err != 0)
                    {
                        fprintf(stderr, "ERR: Error code %d when writing MIDI event (track %d, event #%d) to stream: %s\n", err, trackNum, eventNum, strerror(err));
                        return false;
                    }

                    if (feof(stream))
                    {
                        fprintf(stderr, "ERR: Reached EOF when writing MIDI event (track %d, event #%d) to stream\n", trackNum, eventNum);
                        return false;
                    }
                }
            }
        }

        return true;
    }
}

bool mididogWritePath(const midiFile_t* data, const char* path)
{
    FILE* stream = NULL;
    
    // Check for stdout, special case
    if (!strcmp("-", path))
    {
        stream = stdout;
    }
    else
    {
        stream = fopen(path, "wb");
    }
    
    if (stream != NULL)
    {
        bool result = mididogWriteFile(data, stream);
        if (stream != stdout && stream != stderr)
        {
            fclose(stream);
        }
        return result;
    }

    return false;
}
