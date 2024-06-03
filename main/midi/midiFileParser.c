//==============================================================================
// Includes
//==============================================================================

#include "midiFileParser.h"
#include "midiPlayer.h"
#include "heatshrink_helper.h"

#include <esp_log.h>
#include <esp_heap_caps.h>

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

//==============================================================================
// Defines
//==============================================================================

#define MAX_TRACK_CHUNKS 32

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    /// @brief Total chunk length
    uint32_t length;

    /// @brief Pointer to the start of this chunk's data
    uint8_t* data;

    /// @brief The pointer to the current offset within this chunk
    uint8_t* cur;

    /// @brief The accumulated delta times for this track, which nextEvent is relative to
    uint32_t time;

    /// @brief True if the next event in this channel has already been parsed
    bool eventParsed;

    /// @brief The next event, when \c eventParsed is true
    midiEvent_t nextEvent;

    /// @brief The last applicable running status command, or 0 if none
    uint8_t runningStatus;

    /// @brief A buffer for large SysEx or meta event data. Will be allocated in SPIRAM
    uint8_t* eventBuffer;

    /// @brief The size of the event buffer
    uint32_t eventBufferSize;
} chunkInfo_t;

struct midiReaderState
{
    /// @brief Offset of the next byte to read from the song file
    uint32_t offset;

    /// @brief Whether the header has already been read
    bool headerRead;

    /// @brief The MIDI file format - either 0, 1, or 2
    uint16_t format;

    /// @brief The number of tracks in this file
    uint16_t trackChunkCount;

    /// @brief An array holding info for each track in the file
    chunkInfo_t trackChunks[MAX_TRACK_CHUNKS];

    /// @brief A buffer for large SysEx or meta event data
    uint8_t eventBuffer[1024];
};

//==============================================================================
// Static Function Declarations
//==============================================================================

static int readVariableLength(uint8_t* data, uint32_t length, uint32_t* out);
static bool setupEventBuffer(chunkInfo_t* track, uint32_t length);
static bool trackParseNext(chunkInfo_t* track);
static bool parseMidiHeader(midiFileReader_t* reader);

//==============================================================================
// Variables
//==============================================================================

static const uint8_t midiHeader[] = {'M', 'T', 'h', 'd'};
static const uint8_t trackHeader[] = {'M', 'T', 'r', 'k'};
static const char defaultText[] = "<err: alloc failed>";

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Read a variable length quantity from a byte buffer
 *
 * @param[in] reader A pointer to raw MIDI data
 * @param[in] length The number of bytes in data
 * @param[out] out A pointer to a uint32_t to write the resulting value to
 * @return int The number of bytes read
 */
static int readVariableLength(uint8_t* data, uint32_t length, uint32_t* out)
{
    if (!length)
    {
        return 0;
    }

    uint32_t read = 0;
    uint32_t val = 0;

    uint8_t last = 0x00;
    do
    {
        val <<= 7;
        last = data[read];
        val |= (last & 0x7F);

        read++;
    } while ((last & 0x80) && read < 4 && read < length);

    *out = val;

    return read;
}

static bool setupEventBuffer(chunkInfo_t* track, uint32_t length)
{
    if (!track->eventBuffer || track->eventBufferSize < length)
    {
        if (track->eventBuffer)
        {
            free(track->eventBuffer);
            track->eventBuffer = NULL;
            track->eventBufferSize = 0;
        }

        void* newBuffer = heap_caps_malloc(length, MALLOC_CAP_SPIRAM);
        if (newBuffer)
        {
            track->eventBuffer = newBuffer;
            track->eventBufferSize = length;
            return true;
        }
        else
        {
            // Memory could not be allocated!
            return false;
        }
    }

    // We already have one and it's big enough! Just use that!
    return true;
}

#define TRK_REMAIN() (track->length - (track->cur - track->data))
#define ERR() do { track->eventParsed = false; track->nextEvent.deltaTime = UINT32_MAX; return false; } while (0)
static bool trackParseNext(chunkInfo_t* track)
{
    // All events are preceded by a delta-time, which is a variable-length quantity
    uint32_t deltaTime;
    int read = readVariableLength(track->cur, TRK_REMAIN(), &deltaTime);
    if (!read)
    {
        // No delta-time found
        ERR();
    }

    // Increase the track read offset
    track->cur += read;

    // Don't overflow that buffer
    if (!TRK_REMAIN())
    {
        ERR();
    }

    // Read the status byte of the next event
    uint8_t status = *(track->cur++);

    // Handle running status
    if (!(status & 0xF0))
    {
        if (!track->runningStatus)
        {
            // No running status set, so something is malformed
            ERR();
        }

        // Set status to the real value
        status = track->runningStatus;

        // Un-read that byte!
        // I'm not going to check if (track->cur > track->data) because how else would runningStatus be set?
        track->cur--;
    }
    else
    {
        track->runningStatus = status;
    }

    // If the top nibble is set, this is a status
    switch (status & 0xF0)
    {
        case 0x80: // Note OFF
        case 0x90: // Note ON
        case 0xA0: // AfterTouch
        case 0xB0: // Control Change
        case 0xE0: // Pitch bend
        {
            track->nextEvent.type = MIDI_EVENT;
            track->nextEvent.midi.status = status;

            // This message has two data bytes
            // The rest of the events have 2 data bytes
            if (TRK_REMAIN() < 2)
            {
                ERR();
            }

            track->nextEvent.midi.data[0] = *(track->cur++);
            track->nextEvent.midi.data[1] = *(track->cur++);
            track->nextEvent.midi.data[2] = 0;
            break;
        }

        case 0xC0: // Program Select
        case 0xD0: // Channel Pressure
        {
            track->nextEvent.type = MIDI_EVENT;
            track->nextEvent.midi.status = status;

            // This message has one data byte
            if (!TRK_REMAIN())
            {
                ERR();
            }

            track->nextEvent.midi.data[0] = *(track->cur++);
            track->nextEvent.midi.data[1] = 0;
            track->nextEvent.midi.data[2] = 0;
            break;
        }

        // This is a System event
        case 0xF0:
        {
            // These events reset the running status
            track->runningStatus = 0;

            if (status == 0xFF)
            {
                // Meta Event!
                // First, read the type byte
                if (!TRK_REMAIN()) { ERR(); }
                uint8_t metaType = *(track->cur++);


                uint32_t metaLength;
                // Read the data length
                read = readVariableLength(track->cur, TRK_REMAIN(), &metaLength);
                if (!read)
                {
                    ERR();
                }

                track->cur += read;

                track->nextEvent.type = META_EVENT;
                track->nextEvent.meta.length = metaLength;

                if (TRK_REMAIN() < metaLength)
                {
                    ERR();
                }

                switch (metaType)
                {
                    case SEQUENCE_NUMBER:
                    {
                        // Sequence Number
                        break;
                    }
                    case TEXT:
                    case COPYRIGHT:
                    case SEQUENCE_OR_TRACK_NAME:
                    case INSTRUMENT_NAME:
                    case LYRIC:
                    case MARKER:
                    case CUE_POINT:
                    // Reserved text meta-events
                    case 0x08:
                    case 0x09:
                    case 0x0A:
                    case 0x0B:
                    case 0x0C:
                    case 0x0D:
                    case 0x0E:
                    case 0x0F:
                    {
                        if (metaType > CUE_POINT)
                        {
                            track->nextEvent.meta.type = TEXT;
                        }
                        else
                        {
                            track->nextEvent.meta.type = (metaEventType_t)metaType;
                        }

                        // Not nul-terminated... so we gotta copy it
                        // Check if there's already a buffer allocated with sufficient space (plus a NUL terminator)
                        if (setupEventBuffer(track, metaLength + 1))
                        {
                            memcpy(track->eventBuffer, track->cur, metaLength);
                            // NUL-terminate the string
                            track->eventBuffer[metaLength] = 0;

                            // Add the NUL-terminator, and zero out any unused memory in the buffer
                            //memset(track->eventBuffer + metaLength, 0, track->eventBufferSize - metaLength);
                        }
                        else
                        {
                            // Memory could not be allocated, just put an error string in the event so it's valid
                            track->nextEvent.meta.text = defaultText;
                        }

                        break;
                    }

                    case CHANNEL_PREFIX:
                    {
                        break;
                    }

                    case END_OF_TRACK:
                    {
                        break;
                    }

                    case TEMPO:
                    {
                        break;
                    }

                    case SMPTE_OFFSET:
                    {
                        break;
                    }

                    case TIME_SIGNATURE:
                    {
                        break;
                    }

                    case KEY_SIGNATURE:
                    {
                        break;
                    }

                    case PROPRIETARY:
                    {
                        //track->nextEvent.meta.data = track->cur;
                        if (setupEventBuffer(track, metaLength + 1))
                        {
                            memcpy(track->eventBuffer, track->cur, metaLength);
                            // NUL-terminate the string
                            track->eventBuffer[metaLength] = 0;

                            // Add the NUL-terminator, and zero out any unused memory in the buffer
                            //memset(track->eventBuffer + metaLength, 0, track->eventBufferSize - metaLength);
                        }
                        break;
                    }

                    default: break;
                }

                track->cur += metaLength;
            }
            else if (status == 0xF0 || status == 0xF7)
            {
                // SysEx Event!
                // F0 and F7 are basically the same, except for F0 the F0 is prepended to the message data
                // and for F7, the message data is taken as-is

                // A sysex event in a MIDI file is different from one in streaming mode (i.e. USB), because there is no
                // length byte transmitted in streaming mode, so we need to check the manufacturer length there

                // TODO: Move this to the streaming parser
                /*
                // We'll need to read at least one more byte
                //if (!TRK_REMAIN()) { ERR(); }
                uint16_t manufacturer = *(track->cur++);
                if (!manufacturer)
                {
                    if (TRK_REMAIN() < 2) { ERR(); }
                    // A manufacturer ID of 0 means the ID is actually in the next 2 bytes
                    manufacturer = *(track->cur++);
                    manufacturer <<= 7;
                    manufacturer |= *(track->cur++);
                }
                else
                {
                    // Technically 0x00 0x00 0x41 is considered a different manufacturer from the single-byte value 0x41
                    // So in that case just put a 1 in the 15th bit that's otherwise unused
                    manufacturer |= (1 << 15);
                }*/

                uint32_t sysexLength;
                read = readVariableLength(track->cur, TRK_REMAIN(), &sysexLength);
                if (!read)
                {
                    ERR();
                }
                track->cur += read;

                if (TRK_REMAIN() < sysexLength)
                {
                    ERR();
                }

                track->nextEvent.type = SYSEX_EVENT;
                // Add one to account for the preceding F0 if needed
                if (setupEventBuffer(track, sysexLength + ((status == 0xF0) ? 1 : 0)))
                {
                    uint8_t* dest = track->eventBuffer;
                    if (status == 0xF0)
                    {
                        // When status is F0, add the F0 before copying the rest
                        *(dest++) = 0xF0;
                    }

                    // The length bytes don't go in, since an actual sysex message doesn't have those
                    memcpy(dest, track->cur, sysexLength);
                    track->nextEvent.sysex.data = track->eventBuffer;
                    track->nextEvent.sysex.length = sysexLength;
                }
                else
                {
                    // Failed to allocate memory. Sad.
                    track->nextEvent.sysex.data = NULL;
                    track->nextEvent.sysex.length = 0;
                    track->nextEvent.sysex.manufacturerId = 0;
                }
            }
            else
            {
                // Ignore unknown?
            }
            break;
        }

        default:
        {
            ERR();
        }
    }

    // Don't bother doing this earlier since we might just overwrite it
    track->nextEvent.deltaTime = deltaTime;
    track->eventParsed = true;
    return true;
}

static bool parseMidiHeader(midiFileReader_t* reader)
{
    midiReaderState_t* state = reader->state;

    if (!state->headerRead)
    {
        if (reader->length < (8 + 6))
        {
            ESP_LOGE("MIDIParser", "Not a MIDI file! Length insufficient for header");
        }

        // I mean, offset should be 0
        if (memcmp(reader->data + state->offset, midiHeader, sizeof(midiHeader)))
        {
            ESP_LOGE("MIDIParser", "Not a MIDI file! Header does not match: %hhx, %hhx, %hhx, %hhx", reader->data[state->offset], reader->data[state->offset+1], reader->data[state->offset+2], reader->data[state->offset+3]);
            // Not a MIDI file!
            return false;
        }

        state->offset += sizeof(midiHeader);
        uint32_t chunkLen = (reader->data[state->offset] << 24)
            | (reader->data[state->offset + 1] << 16)
            | (reader->data[state->offset + 2] << 8)
            | reader->data[state->offset + 3];

        state->offset += 4;

        uint16_t format = (reader->data[state->offset] << 8) | reader->data[state->offset + 1];
        state->offset += 2;

        if (format > 2)
        {
            ESP_LOGE("MIDIParser", "Unsupported MIDI file format: %" PRIu16, format);
            // Unsupported format! Technically we're supposed to try and parse still.
            return false;
        }
        state->format = format;

        uint16_t trackChunkCount = (reader->data[state->offset] << 8) | reader->data[state->offset + 1];
        state->offset += 2;
        state->trackChunkCount = trackChunkCount;
        if (trackChunkCount >= MAX_TRACK_CHUNKS)
        {
            ESP_LOGE("MIDIParser", "MIDI file track count exceeds maximum of %d: %" PRIu16, MAX_TRACK_CHUNKS, trackChunkCount);
        }

        uint16_t division = (reader->data[state->offset] << 8) | reader->data[state->offset + 1];
        state->offset += 2;

        if (division & 0x8000)
        {
            // MIDI Spec Sez:
            // Bits 14 thru 8 contain one of the four values -24, -25, -29, or -30, corresponding to
            // the four standard SMPTE and MIDI time code formats (-29 corresponds to 30 drop frame)
            int8_t smpte = (((int16_t)division) >> 8) & 0xFF;
            uint8_t framesPerSecond = -smpte;

            // -24: 24fps
            // -25: 25fps
            // -29: 29.97fps (30 drop frame)
            // -30: 30fps

            // If we get 29.97...
            // Wikipedia sez:
            // In order to make an hour of timecode match an hour on the clock, drop-frame timecode
            // skips frame numbers 0 and 1 of the first second of every minute, except when the number
            // of minutes is divisible by ten.
            // TODO: Screw that, I'm just going to round it to 30 for now
            if (framesPerSecond == 29)
            {
                // GET YOUR UGLY FRAMERATE OUT OF HERE
                framesPerSecond = 30;
            }

            // positive timecode division
            uint8_t ticksPerFrame = (division & 0xFF);
        }
        else
        {
            // ticks per quarter note
            uint16_t ticksPerQuarterNote = (division & 0x7FFF);
        }

        // TODO: Actually do something with the timing info

        // Skip anything extra that might be in the header
        if (state->offset < chunkLen)
        {
            state->offset = chunkLen - (sizeof(midiHeader) + 4);
            if (state->offset >= reader->length)
            {
                ESP_LOGE("MIDIParser", "Header length exceeds data length");
                return false;
            }
        }

        state->headerRead = true;

        if (format == 0 && trackChunkCount != 1)
        {
            ESP_LOGE("MIDIParser", "Type-0 MIDI file should have 1 track, instead it has: %" PRIu16, trackChunkCount);
            return false;
        }

        // Current offset should be at the next track
        uint8_t* ptr = reader->data + state->offset;

        // Regardless, if there are multiple tracks we want to grab pointers to them all
        int i = 0;
        while (i < trackChunkCount)
        {
            // Check to make sure we don't overrun the buffer reading the header
            if ((ptr - reader->data) + 8 >= reader->length)
            {
                ESP_LOGE("MIDIParser", "Reached end of file unexpectedly while reading track chunk %d", i);
                return false;
            }

            // Check the header of this chunk
            if (memcmp(ptr, trackHeader, sizeof(trackHeader)))
            {
                ESP_LOGW("MIDIParser", "Start of chunk %d did not contain track chunk header", i);

                // We should ignore unknown chunk types
                ptr += sizeof(trackHeader);

                // Read this chunk's header -- all chunks should have this
                uint32_t unkChunkLen = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];
                ptr += 4;

                // Advance to the start of the next chunk
                ptr += unkChunkLen;

                // Don't increment trackChunkCount!
                continue;
            }

            ptr += sizeof(trackHeader);

            uint32_t trackChunkLen = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];
            ptr += 4;

            // Save track length and offset information
            state->trackChunks[i].length = trackChunkLen;
            state->trackChunks[i].data = ptr;
            state->trackChunks[i].cur = ptr;

            // Advance the pointer to the start of the next chunk
            ptr += trackChunkLen;

            state->trackChunks[i].eventParsed = trackParseNext(&state->trackChunks[i]);

            // Handle empty tracks, basically
            if (!state->trackChunks[i].eventParsed)
            {
                state->trackChunks[i].nextEvent.deltaTime = UINT32_MAX;
            }

            i++;
        }

        if (ptr - reader->data > reader->length)
        {
            ESP_LOGE("MIDIParser", "Reached end of file unexpectedly while reading track chunk %d", i);
            return false;
        }

        // Header has been read and all track offsets have been loaded
        // And all track data chunks should be safe
    }

    ESP_LOGW("MIDIParser", "Not yet implemented, but the header looked OK!");
    return true;
}

bool loadMidiFile(midiFileReader_t* reader, const char* file, bool spiRam)
{
    uint32_t size;
    uint8_t* data = readHeatshrinkFile(file, &size, spiRam);
    if (data != NULL)
    {
        ESP_LOGI("MIDIFileParser", "Song %s has %" PRIu32 " bytes", file, size);
        reader->data = data;
        reader->length = (uint32_t)size;
        reader->state = calloc(1, sizeof(struct midiReaderState));
        return true;
    }
    else
    {
        return false;
    }
}

void unloadMidiFile(midiFileReader_t* reader)
{
    free(reader->state);
    free(reader->data);

    reader->state = NULL;
    reader->data = NULL;
    reader->length = 0;
}

uint32_t midiNextEventTime(midiFileReader_t* reader)
{
    return UINT32_MAX;
}

bool midiNextEvent(midiFileReader_t* reader, midiEvent_t* event)
{
    uint32_t minTime = UINT32_MAX;
    // Pointer to the next track
    chunkInfo_t* nextTrack = NULL;

    if (!reader->state->headerRead)
    {
        if (!parseMidiHeader(reader))
        {
            return false;
        }
    }

    // TODO: This treats all formats like a format 1 (simultaneous)
    for (int i = 0; i < reader->state->trackChunkCount; i++)
    {
        chunkInfo_t* info = &reader->state->trackChunks[i];

        if (!info->eventParsed && info->nextEvent.deltaTime != UINT32_MAX)
        {
            info->eventParsed = trackParseNext(info);
        }

        // Check if we either already have a parsed event waiting, or are able to parse one now
        // Short-circuiting will make sure we only parse another event when needed and permitted
        if (info->eventParsed || (info->nextEvent.deltaTime != UINT32_MAX && trackParseNext(info)))
        {
            // info->nextEvent has now been set by trackParseNext()
            if (!info->nextEvent.deltaTime)
            {
                // The delta-time is 0! Just return this event immediately
                *event = info->nextEvent;

                // Consume the event!
                info->eventParsed = false;
                return true;
            }
            else if (info->time + info->nextEvent.deltaTime < minTime)
            {
                // This is the most-next event, so set minTime to its time
                // TODO: add this to time when consumed
                minTime = info->time + info->nextEvent.deltaTime;
                nextTrack = info;
            }
        }
    }

    if (minTime == UINT32_MAX)
    {
        // This value means we didn't find any more events in any of the channels
        return false;
    }

    // TODO should this be a memcpy? the compiler will probably take care of it
    *event = nextTrack->nextEvent;
    nextTrack->eventParsed = false;
    nextTrack->time += event->deltaTime;
    return true;
}

/*bool midiPlayNext(midiFileReader_t* reader, midiPlayer_t* next)
{
    return false;
}*/
