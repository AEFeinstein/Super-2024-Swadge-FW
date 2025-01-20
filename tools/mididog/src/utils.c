#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "midiFileParser.h"

#include "utils.h"
#include "midi_dump.h"

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

    /*len = midiWriteHeader(buffer, sizeof(buffer), data);

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
    }*/

    if (data->data != NULL)
    {
        // Write data out directly
        written = 0;
        // TODO
        len = data->length;

        do
        {
            written += fwrite(data->data + written, 1, len - written, stream);
            if (written < len)
            {
                int err = ferror(stream);

                // Ignore EAGAIN since it's not a "real" error in this situation
                if (err != 0 && err != EAGAIN)
                {
                    fprintf(stderr, "ERR: Error code %d when writing MIDI file to stream: %s\n", err, strerror(err));
                    return false;
                }

                if (feof(stream))
                {
                    fprintf(stderr, "ERR: Reached EOF when writing MIDI file to stream\n");
                    return false;
                }
            }
        } while (written < len);

        return true;
    }
    else
    {
        return false;
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

midiFile_t* mididogUnTokenizeMidi(const midiFile_t* midiFile)
{
    if (NULL == midiFile)
    {
        return NULL;
    }

    const midiEvent_t eot = {
        .deltaTime = 0,
        .type = META_EVENT,
        .meta = {
            .type = END_OF_TRACK,
            .length = 0,
        }
    };

    const bool skipEmptyTracks = true;

    // The total length in bytes of all events in each track
    uint32_t trackEventLengths[midiFile->trackCount];
    bool emptyTracks[midiFile->trackCount];
    uint16_t emptyTrackCount = 0;
    
    int totalFileLength = 0;

    // Measure header length
    uint32_t headerLength = midiWriteHeader(NULL, 1024, midiFile);

    totalFileLength += headerLength;
    
    int tmp = 0;

    for (int trackNum = 0; trackNum < midiFile->trackCount; trackNum++)
    {
        trackEventLengths[trackNum] = 0;
        uint8_t runningStatus = 0;
        uint32_t nextDeltaTime = 0;
        bool endOfTrack = false;
        bool empty = true;

        fprintf(stderr, "track %d has %d events\n", trackNum, midiFile->events[trackNum].count);

        for (int eventNum = 0; eventNum < midiFile->events[trackNum].count; eventNum++)
        {
            midiEvent_t* event = &midiFile->events[trackNum].events[eventNum];

            if (event->type == NO_EVENT)
            {
                // Skip any no-op events, but make sure to account for their delta time
                nextDeltaTime += event->deltaTime;
                continue;
            }

            // Account for the delta time
            trackEventLengths[trackNum] += tmp = writeVariableLength(NULL, 1024, event->deltaTime + nextDeltaTime);
            //fprintf(stderr, "variable length quantity %d computed with %d bytes\n", event->deltaTime, tmp);
            // Account for actual event
            trackEventLengths[trackNum] += tmp = midiWriteEventWithRunningStatus(NULL, 1024, event, &runningStatus);
            //fprintf(stderr, "computed event with length of %d\n", tmp);
            //fprintEvent(stderr, 0, event);

            nextDeltaTime = 0;

            if (event->type == META_EVENT && event->meta.type == END_OF_TRACK)
            {
                endOfTrack = true;
            }
            else
            {
                empty = false;
            }
        }

        // Ensure the track actually has an end of track event
        if (!endOfTrack)
        {
            fprintf(stderr, "Track %d is missing end-of-track event\n", trackNum);
            // running status
            trackEventLengths[trackNum] += writeVariableLength(NULL, 1024, eot.deltaTime);
            trackEventLengths[trackNum] += midiWriteEventWithRunningStatus(NULL, 1024, &eot, &runningStatus);
        }

        emptyTracks[trackNum] = empty;

        if (empty && skipEmptyTracks)
        {
            emptyTrackCount++;
            trackEventLengths[trackNum] = 0;
        }
        else
        {
            totalFileLength += 8;
            totalFileLength += trackEventLengths[trackNum];
        }
    }

    midiFile_t* result = malloc(sizeof(midiFile_t));
    if (NULL == result)
    {
        fprintf(stderr, "ERR: unable to allocate %lu bytes for un-tokenized MIDI file\n", sizeof(midiFile_t));
        //free(fileData);
        return NULL;
    }

    uint16_t validTrackCount = midiFile->trackCount - emptyTrackCount;

    midiTrack_t* tracks = calloc(validTrackCount, sizeof(midiTrack_t));
    if (NULL == tracks)
    {
        fprintf(stderr, "ERR: unable to allocate %" PRIu32 " bytes for un-tokenized MIDI file data\n", totalFileLength);
        free(result);
        return NULL;
    }

    uint8_t* fileData = malloc(totalFileLength);
    if (NULL == fileData)
    {
        fprintf(stderr, "ERR: unable to allocate %" PRIu32 " bytes for un-tokenized MIDI file data\n", totalFileLength);
        free(tracks);
        free(result);
        return NULL;
    }

    // Clear data not used for normal files
    result->events = NULL;

    // Kinda a hack but it's already hacky anyway. This makes sure something can free the events we allocated when the file's deallocated
    result->data = fileData;
    result->length = totalFileLength;

    // Copy the original's metadata
    result->format = midiFile->format;
    result->timeDivision = midiFile->timeDivision;
    result->trackCount = validTrackCount;
    result->tracks = tracks;

    int offset = 0;

    offset += midiWriteHeader(fileData + offset, totalFileLength - offset, result);
    fprintf(stderr, "offset=%d after writing header\n", offset);
    
    for (int trackNum = 0; trackNum < validTrackCount; trackNum++)
    {
        uint8_t runningStatus = 0;
        uint32_t nextDeltaTime = 0;
        bool endOfTrack = false;
        
        if (skipEmptyTracks && emptyTracks[trackNum])
        {
            continue;
        }

        // Write track chunk header
        // Write magic bytes
        memcpy(fileData + offset, "MTrk", 4);
        offset += 4;
        
        // Write data length
        fileData[offset++] = (trackEventLengths[trackNum] >> 24) & 0xFFu;
        fileData[offset++] = (trackEventLengths[trackNum] >> 16) & 0xFFu;
        fileData[offset++] = (trackEventLengths[trackNum] >> 8) & 0xFFu;
        fileData[offset++] = (trackEventLengths[trackNum]) & 0xFFu;

        int trackStartOffset = offset;

        result->tracks[trackNum].data = fileData + offset;
        result->tracks[trackNum].length = trackEventLengths[trackNum];


        // Write actual events
        for (int eventNum = 0; eventNum < midiFile->events[trackNum].count; eventNum++)
        {
            midiEvent_t* event = &midiFile->events[trackNum].events[eventNum];


            if (event->type == NO_EVENT)
            {
                nextDeltaTime += event->deltaTime;
                continue;
            }

            // Account for the delta time
            tmp = writeVariableLength(fileData + offset, totalFileLength - offset, event->deltaTime + nextDeltaTime);
            offset += tmp;
            fprintf(stderr, "variable length quantity %d written with %d bytes, new offset=%d\n", event->deltaTime, tmp, offset);
            fprintf(stderr, "%1$d (%1$x) --> ", event->deltaTime);
            for (int i = 0; i < tmp; i++)
            {
                fprintf(stderr, "0x%02hhx ", fileData[offset - tmp + i]);
            }
            fputc('\n', stderr);
            

            // Account for actual event
            tmp = midiWriteEventWithRunningStatus(fileData + offset, totalFileLength - offset, event, &runningStatus);
            offset += tmp;
            fprintf(stderr, "wrote event with length of %d, new offset=%d\n", tmp, offset);
            fprintEvent(stderr, 0, event);

            if (offset >= 8)
            {
                fprintf(stderr, "Last 8 bytes: %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx\n", fileData[offset - 8], fileData[offset - 7], fileData[offset - 6], fileData[offset - 5], fileData[offset - 4], fileData[offset - 3], fileData[offset - 2], fileData[offset - 1]);
            }

            if (event->type == META_EVENT && event->meta.type == END_OF_TRACK)
            {
                endOfTrack = true;
            }

            nextDeltaTime = 0;
        }

        // Add missing end-of-track event
        if (!endOfTrack)
        {
            fprintf(stderr, "Adding missing end-of-track to track %d\n", trackNum);
            offset += writeVariableLength(fileData + offset, totalFileLength - offset, eot.deltaTime);
            offset += midiWriteEventWithRunningStatus(fileData + offset, totalFileLength - offset, &eot, &runningStatus);
        }

        if (offset - trackStartOffset != result->tracks[trackNum].length)
        {
            fprintf(stderr, "ERR: Output track does not have correc length!?? We said it would be %u, but now it's actually %d\n", result->tracks[trackNum].length, offset - trackStartOffset);
        }
    }

    return result;
}

static const char* noteNames[] = {"A","A#","B","C","C#","D","D#","E","F","F#","G","G#"};
static const uint8_t noteA0 = 21;

int writeNoteName(char* output, int max, int flags, uint8_t note)
{
    // Calculate semitones above A0
    int16_t semitones = (int16_t)note - (int16_t)noteA0;    

    const char* name = noteNames[(12 + semitones % 12) % 12];
    int16_t octave = semitones / 12;

    return snprintf(output, max, "%s%" PRId16, name, octave);
}
