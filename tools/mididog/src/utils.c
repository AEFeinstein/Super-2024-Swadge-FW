#include <errno.h>
#include <inttypes.h>
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

midiFile_t* mididogTokenizeMidi(const midiFile_t* midiFile)
{
    if (NULL == midiFile)
    {
        return NULL;
    }

    // Number of events in each track
    uint32_t trackEventCounts[midiFile->trackCount];

    // The total length in bytes of all events in each track
    //uint32_t trackEventLengths[midiFile->trackCount];

    // Temporary running status bytes for each track
    //uint8_t trackRunningStatuses[midiFile->trackCount];

    midiEventStream_t* streams = calloc(midiFile->trackCount, sizeof(midiEventStream_t));

    if (NULL == streams)
    {
        fprintf(stderr, "ERR: unable to allocate %lu bytes for MIDI event streams\n", midiFile->trackCount * sizeof(midiEventStream_t));
        return NULL;
    }

    midiFileReader_t reader = {0};
    if (initMidiParser(&reader, midiFile))
    {
        midiEvent_t event;

        // Dynamically-sized stack-allocated arrays don't get zeroed automatically!
        memset(trackEventCounts, 0, sizeof(trackEventCounts));

        // 1. Count number of events in each track
        // 2. Count total length of written events in each track
        // midiNextEvent returns events in order of time, then track, so we need to basically un-collate it
        while (midiNextEvent(&reader, &event))
        {
            trackEventCounts[event.track]++;
            //trackEventLengths[event.track] += midiWriteEventWithRunningStatus(NULL, 1024, &event, &trackRunningStatuses[event.track]);
        }

        // Count up the total number of events in the file from the track totals
        uint32_t totalEvents = 0;
        for (int i = 0; i < midiFile->trackCount; i++)
        {
            totalEvents += trackEventCounts[i];
        }

        fprintf(stderr, "File has %" PRIu16 " tracks with %" PRIu32 " events total\n", midiFile->trackCount, totalEvents);

        // Allocate the events, once
        midiEvent_t* allEvents = calloc(totalEvents, sizeof(midiEvent_t));

        if (NULL == allEvents)
        {
            fprintf(stderr, "ERR: unable to allocate %lu bytes for MIDI event data\n", totalEvents * sizeof(midiEvent_t));
            deinitMidiParser(&reader);
            free(streams);
            return NULL;
        }

        midiFile_t* result = malloc(sizeof(midiFile_t));

        if (NULL == result)
        {
            fprintf(stderr, "ERR: unable to allocate %lu bytes for tokenized MIDI file\n", sizeof(midiFile_t));
            deinitMidiParser(&reader);
            free(streams);
            free(allEvents);
            return NULL;
        }

        // Clear data not used for tokenized files
        result->length = 0;
        result->tracks = NULL;

        // Kinda a hack but it's already hacky anyway. This makes sure something can free the events we allocated when the file's deallocated
        result->data = (uint8_t*)((void*)allEvents);

        // Copy the original's metadata
        result->format = midiFile->format;
        result->timeDivision = midiFile->timeDivision;
        result->trackCount = midiFile->trackCount;

        // Set the track event streams
        result->events = streams;

        // Populate each track's stream with the correct events pointer within allEvents
        uint32_t eventOffset = 0;
        for (int i = 0; i < midiFile->trackCount; i++)
        {
            midiEventStream_t* trackStream = &streams[i];
            trackStream->count = 0;
            trackStream->events = &allEvents[eventOffset];
            eventOffset += trackEventCounts[i];
        }

        // Reset the file parser so we can actually read the events
        resetMidiParser(&reader);

        // Actually copy each event to the appropriate track's event stream
        while (midiNextEvent(&reader, &event))
        {
            midiEventStream_t* trackStream = &streams[event.track];

            memcpy(&trackStream->events[trackStream->count++], &event, sizeof(midiEvent_t));
        }

        // We're done! Deinit the MIDI parser
        deinitMidiParser(&reader);

        return result;
    }

    return NULL;
}