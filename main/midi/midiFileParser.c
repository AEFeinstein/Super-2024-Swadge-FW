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
// Structs
//==============================================================================

/// @brief Contains all track-specific parsing state
struct midiTrackState
{
    /// @brief A pointer to the MIDI track itself
    const midiTrack_t* track;

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

    /// @brief Whether or not the END OF TRACK event has been read
    bool done;
};

typedef struct
{
    midiPlayer_t player;

    uint8_t trackCount;
    midiTrackState_t* trackStates;
} midiSaveState_t;

//==============================================================================
// Static Function Declarations
//==============================================================================

static int readVariableLength(uint8_t* data, uint32_t length, uint32_t* out);
static bool setupEventBuffer(midiTrackState_t* track, uint32_t length);
static bool trackParseNext(midiFileReader_t* reader, midiTrackState_t* track);
static bool parseMidiHeader(midiFile_t* file);
static void readFirstEvents(midiFileReader_t* reader);

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
    uint32_t read = 0;
    uint32_t val = 0;

    uint8_t last = 0x00;
    while (((last & 0x80) || read == 0) && read < 4 && read < length)
    {
        val <<= 7;
        last = data[read];
        val |= (last & 0x7F);

        read++;
    }

    *out = val;

    return read;
}

static bool setupEventBuffer(midiTrackState_t* track, uint32_t length)
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

#define TRK_REMAIN() (track->track->length - (track->cur - track->track->data))
#define ERR() do { track->eventParsed = false; track->nextEvent.deltaTime = UINT32_MAX; ESP_LOGE("MIDIParser", "Error parsing at line %d and offset %" PRIuPTR " (total offset %" PRIuPTR ")", __LINE__, (track->cur - track->track->data), (track->cur - reader->file->data)); return false; } while (0)
static bool trackParseNext(midiFileReader_t* reader, midiTrackState_t* track)
{
    if (track->done)
    {
        return false;
    }

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
    if (status > 0xF7)
    {
        // System Realtime messages do not affect running status at all
    }
    else if (0xF0 <= status && status <= 0xF7)
    {
        // System Common messages clear the running status
        track->runningStatus = 0;
    }
    else if (0x80 <= status && status <= 0xEF)
    {
        // Voice Messages set the running status
        track->runningStatus = status;
    }
    else
    {
        // Anything else means we should use the previous running status, since this 'status' is actually data
        if (!track->runningStatus)
        {
            // If there's no running status then this is just invalid data
            ESP_LOGE("MIDIParser", "No running status set and unknown status given (status=0x%02" PRIx8 ")", status);
            ERR();
        }
        else
        {
            // Use the previous running status, and back up one byte
            status = track->runningStatus;
            track->cur--;
        }
    }
    // Now that running status is resolved, handle the status

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
                        track->nextEvent.meta.type = SEQUENCE_NUMBER;
                        if (metaLength == 2)
                        {
                            // Read the data as a a 16-bit int (not 14-bit since this is a meta-event)
                            track->nextEvent.meta.sequenceNumber = (track->cur[0] << 8) | track->cur[1];
                        }
                        else
                        {
                            // If the length is 0 (or otherwise not what we expect), use the track index as the sequence number
                            track->nextEvent.meta.sequenceNumber = (track - reader->states);
                        }
                        break;
                    }

                    // Known text meta-events:
                    case TEXT:
                    case COPYRIGHT:
                    case SEQUENCE_OR_TRACK_NAME:
                    case INSTRUMENT_NAME:
                    case LYRIC:
                    case MARKER:
                    case CUE_POINT:
                    // Reserved text meta-events:
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
                            // If this is a reserved text event, just set the type to TEXT
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

                            // Actually put the text in the event
                            track->nextEvent.meta.text = (char*)track->eventBuffer;

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

                    // This is considered obsolete -- it's used to specify a channel for sysex/meta events in a
                    // format 0 (single-track) file, as these do not include the channel like MIDI events do
                    case CHANNEL_PREFIX:
                    {
                        track->nextEvent.meta.type = CHANNEL_PREFIX;
                        if (metaLength == 1)
                        {
                            track->nextEvent.meta.prefix = track->cur[0];
                        }
                        else
                        {
                            track->nextEvent.meta.prefix = 0;
                        }
                        break;
                    }

                    case PORT_PREFIX:
                    {
                        track->nextEvent.meta.type = PORT_PREFIX;
                        if (metaLength == 1)
                        {
                            track->nextEvent.meta.prefix = track->cur[0];
                        }
                        else
                        {
                            track->nextEvent.meta.prefix = 0;
                        }
                        break;
                    }

                    case END_OF_TRACK:
                    {
                        track->nextEvent.meta.type = END_OF_TRACK;
                        track->done = true;
                        ESP_LOGI("MIDIParser", "End of track #%" PRIdPTR, track - reader->states);
                        break;
                    }

                    case TEMPO:
                    {
                        track->nextEvent.meta.type = TEMPO;

                        if (metaLength == 3)
                        {
                            track->nextEvent.meta.tempo = (track->cur[0] << 16) | (track->cur[1] << 8) | track->cur[2];
                            if (track->nextEvent.meta.tempo == 0)
                            {
                                ESP_LOGW("MIDIParser", "Ignoring bad tempo value of 0, using 500000 (120BPM) instead");
                                track->nextEvent.meta.tempo = 500000;
                            }
                        }
                        else
                        {
                            // 120BPM is the default -- 500,000 microseconds per quarter note
                            // So if the meta-event is malformed (it should always have 3 data bytes), go with that
                            track->nextEvent.meta.tempo = 500000;
                        }
                        break;
                    }

                    case SMPTE_OFFSET:
                    {
                        // This is the start time of the track?
                        track->nextEvent.meta.type = SMPTE_OFFSET;
                        if (metaLength == 5)
                        {
                            track->nextEvent.meta.startTime.hour = track->cur[0];
                            track->nextEvent.meta.startTime.min = track->cur[1];
                            track->nextEvent.meta.startTime.sec = track->cur[2];
                            track->nextEvent.meta.startTime.frame = track->cur[3];
                            track->nextEvent.meta.startTime.frameHundredths = track->cur[4];
                        }
                        else
                        {
                            // should be 5 bytes so if it's not, uhh, 0?
                            track->nextEvent.meta.startTime.hour = 0;
                            track->nextEvent.meta.startTime.min = 0;
                            track->nextEvent.meta.startTime.sec = 0;
                            track->nextEvent.meta.startTime.frame = 0;
                            track->nextEvent.meta.startTime.frameHundredths = 0;
                        }
                        break;
                    }

                    case TIME_SIGNATURE:
                    {
                        track->nextEvent.meta.type = TIME_SIGNATURE;
                        // OK, so apparently this is only for display purposes
                        // That's great, because I have no idea what the hell to do with any of this
                        if (metaLength == 4)
                        {
                            track->nextEvent.meta.timeSignature.numerator = track->cur[0];
                            track->nextEvent.meta.timeSignature.denominator = track->cur[1];
                            track->nextEvent.meta.timeSignature.midiClocksPerMetronomeTick = track->cur[2];
                            track->nextEvent.meta.timeSignature.num32ndNotesPerBeat = track->cur[3];
                        }
                        else
                        {
                            // Default is 4/4, cool, but what's the rest of it?
                            track->nextEvent.meta.timeSignature.numerator = 4;
                            track->nextEvent.meta.timeSignature.denominator = 2;
                            track->nextEvent.meta.timeSignature.midiClocksPerMetronomeTick = 1;
                            track->nextEvent.meta.timeSignature.num32ndNotesPerBeat = 24;
                        }
                        break;
                    }

                    case KEY_SIGNATURE:
                    {
                        track->nextEvent.meta.type = KEY_SIGNATURE;
                        if (metaLength == 2)
                        {
                            int8_t flatsOrSharps = (int8_t)(track->cur[0]);
                            if (flatsOrSharps < 0)
                            {
                                // Flats
                                track->nextEvent.meta.keySignature.flats = -flatsOrSharps;
                                track->nextEvent.meta.keySignature.sharps = 0;
                            }
                            else if (flatsOrSharps > 0)
                            {
                                // Sharps
                                track->nextEvent.meta.keySignature.sharps = flatsOrSharps;
                                track->nextEvent.meta.keySignature.flats = 0;
                            }
                            else
                            {
                                // Key of C
                                track->nextEvent.meta.keySignature.flats = 0;
                                track->nextEvent.meta.keySignature.sharps = 0;
                            }

                            track->nextEvent.meta.keySignature.minor = track->cur[1] ? true : false;
                        }
                        else
                        {
                            // Default to C major if the event is malformed
                            track->nextEvent.meta.keySignature.flats = 0;
                            track->nextEvent.meta.keySignature.sharps = 0;
                            track->nextEvent.meta.keySignature.minor = false;
                        }
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

                    default:
                    {
                        ESP_LOGE("MIDIParser", "Unknown meta-event %02" PRIx8, metaType);
                        break;
                    }
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

                track->cur += sysexLength;
            }
            else
            {
                ESP_LOGE("MIDIParser", "Unknown status byte (0xF_) 0x%02" PRIx8, status);
                // Ignore unknown?
            }
            break;
        }

        default:
        {
            ESP_LOGE("MIDIParser", "Unknown status byte (?) 0x%02" PRIx8, status);
            ERR();
        }
    }

    // Don't bother doing this earlier since we might just overwrite it
    track->nextEvent.deltaTime = deltaTime;
    track->nextEvent.absTime = track->time + deltaTime;
    track->eventParsed = true;
    return true;
}

/**
 * @brief Parse the information contained in the MIDI header and write it to the file struct
 *
 * @param file The MIDI file struct to write header data into
 * @return true if the MIDI file contains a valid header and was successfully parsed
 * @return false if the MIDI file header could not be parsed
 */
static bool parseMidiHeader(midiFile_t* file)
{
    uint32_t offset = 0;

    if (file->length < (8 + 6))
    {
        ESP_LOGE("MIDIParser", "Not a MIDI file! Length insufficient for header");
    }

    // I mean, offset should be 0
    if (memcmp(file->data + offset, midiHeader, sizeof(midiHeader)))
    {
        ESP_LOGE("MIDIParser", "Not a MIDI file! Header does not match: %" PRIx8 ", %" PRIx8 ", %" PRIx8 ", %" PRIx8 "", file->data[offset], file->data[offset+1], file->data[offset+2], file->data[offset+3]);
        // Not a MIDI file!
        return false;
    }

    offset += sizeof(midiHeader);
    uint32_t chunkLen = (file->data[offset] << 24)
        | (file->data[offset + 1] << 16)
        | (file->data[offset + 2] << 8)
        | file->data[offset + 3];

    offset += 4;

    uint16_t format = (file->data[offset] << 8) | file->data[offset + 1];
    offset += 2;

    if (format > 2)
    {
        ESP_LOGE("MIDIParser", "Unsupported MIDI file format: %" PRIu16, format);
        // Unsupported format! Technically we're supposed to try and parse still.
        return false;
    }
    file->format = (midiFileFormat_t)format;

    uint16_t trackChunkCount = (file->data[offset] << 8) | file->data[offset + 1];
    offset += 2;
    file->trackCount = trackChunkCount;
    file->tracks = calloc(trackChunkCount, sizeof(midiTrack_t));
    if (NULL == file->tracks)
    {
        ESP_LOGE("MIDIParser", "Could not allocate data for MIDI file with %" PRIu16 " tracks", trackChunkCount);
        return false;
    }

    uint16_t division = (file->data[offset] << 8) | file->data[offset + 1];
    offset += 2;

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
        file->timeDivision = ticksPerFrame;
    }
    else
    {
        // ticks per quarter note
        uint16_t ticksPerQuarterNote = (division & 0x7FFF);
        file->timeDivision = ticksPerQuarterNote;
    }

    // TODO: Actually do something with the timing info

    // Skip anything extra that might be in the header
    if (offset < chunkLen)
    {
        offset = chunkLen - (sizeof(midiHeader) + 4);
        if (offset >= file->length)
        {
            ESP_LOGE("MIDIParser", "Header length exceeds data length");
            return false;
        }
    }

    if (format == 0 && trackChunkCount != 1)
    {
        ESP_LOGE("MIDIParser", "Type-0 MIDI file should have 1 track, instead it has: %" PRIu16, trackChunkCount);
        return false;
    }

    // Current offset should be at the next track
    uint8_t* ptr = file->data + offset;

    // Regardless, if there are multiple tracks we want to grab pointers to them all
    int i = 0;
    while (i < trackChunkCount)
    {
        // Check to make sure we don't overrun the buffer reading the header
        if ((ptr - file->data) + 8 >= file->length)
        {
            ESP_LOGE("MIDIParser", ":%d Reached end of file unexpectedly while reading track chunk %d", __LINE__, i);
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

        if (ptr + trackChunkLen - file->data > file->length)
        {
            ESP_LOGW("MIDIParser", "Track chunk %d claims length of %" PRIu32 " but there are only %" PRId32 " bytes remaining in the file", i, trackChunkLen, (int32_t)(file->length - (ptr - file->data)));
            trackChunkLen = file->length - (ptr - file->data);
        }

        // Save track length and offset information
        file->tracks[i].length = trackChunkLen;
        file->tracks[i].data = ptr;

        // Advance the pointer to the start of the next chunk
        ptr += trackChunkLen;

        i++;
    }

    if (ptr - file->data > file->length)
    {
        ESP_LOGE("MIDIParser", ":%d Reached end of file unexpectedly while reading track chunk %d", __LINE__, i);
        //ESP_LOGE("MIDIParser", "Track chunk claims length of")
        return false;
    }

    // Header has been read and all track offsets have been loaded
    // And all track data chunks should be safe
    return true;
}

static void readFirstEvents(midiFileReader_t* reader)
{
    for (int i = 0; i < reader->file->trackCount; i++)
    {
        // Parse the first event from each track?
        reader->states[i].eventParsed = trackParseNext(reader, &reader->states[i]);

        // Handle empty/invalid tracks
        if (!reader->states[i].eventParsed)
        {
            reader->states[i].done = true;
            reader->states[i].nextEvent.deltaTime = UINT32_MAX;
        }
    }
}

bool loadMidiFile(const char* name, midiFile_t* file, bool spiRam)
{
    uint32_t size;
    uint8_t* data = readHeatshrinkFile(name, &size, spiRam);
    if (data != NULL)
    {
        ESP_LOGI("MIDIFileParser", "Song %s has %" PRIu32 " bytes", name, size);
        file->data = data;
        file->length = (uint32_t)size;
        if (parseMidiHeader(file))
        {
            return true;
        }
        else
        {
            // Parsing failed, so free the data and return false
            if (file->tracks != NULL)
            {
                // TODO should this be handled in the parser?
                free(file->tracks);
                file->tracks = NULL;
            }
            free(data);
            memset(file, 0, sizeof(midiFile_t));
            return false;
        }
    }
    else
    {
        return false;
    }
}

void unloadMidiFile(midiFile_t* file)
{
    free(file->tracks);
    free(file->data);
    memset(file, 0, sizeof(midiFile_t));
}

bool initMidiParser(midiFileReader_t* reader, midiFile_t* file)
{
    reader->states = calloc(file->trackCount, sizeof(midiTrackState_t));
    if (NULL == reader->states)
    {
        free(reader->states);
        reader->states = NULL;
        return false;
    }

    reader->stateCount = file->trackCount;
    reader->file = file;
    midiParserSetFile(reader, file);

    // Success!
    return true;
}

void midiParserSetFile(midiFileReader_t* reader, midiFile_t* file)
{
    if (reader->states != NULL)
    {
        free(reader->states);
        reader->states = NULL;
    }

    reader->states = calloc(file->trackCount, sizeof(midiTrackState_t));

    // Initialize the reader's internal per-track parsing states
    for (int i = 0; i < file->trackCount; i++)
    {
        reader->states[i].track = &file->tracks[i];
        reader->states[i].cur = file->tracks[i].data;
    }

    reader->file = file;
    reader->division = file->timeDivision;

    readFirstEvents(reader);
}

/**
 * @brief Reset the MIDI parser state, without changing the file
 *
 * @param reader The reader to reset
 */
void resetMidiParser(midiFileReader_t* reader)
{
    for (int i = 0; i < reader->stateCount; i++)
    {
        reader->states[i].done = false;
        reader->states[i].eventParsed = false;
        reader->states[i].runningStatus = 0;
        if (reader->states[i].track != NULL)
        {
            reader->states[i].cur = reader->states[i].track->data;
        }
        else
        {
            reader->states[i].cur = NULL;
            reader->states[i].done = true;
        }
        memset(&reader->states[i].nextEvent, 0, sizeof(midiEvent_t));
        reader->states[i].time = 0;
    }

    if (reader->file != NULL)
    {
        reader->division = reader->file->timeDivision;
        readFirstEvents(reader);
    }
}

void deinitMidiParser(midiFileReader_t* reader)
{
    midiTrackState_t* states = reader->states;
    uint8_t stateCount = reader->stateCount;

    reader->stateCount = 0;
    reader->file = NULL;
    reader->states = NULL;

    if (states != NULL)
    {
        for (int i = 0; i < stateCount; i++)
        {
            if (states[i].eventBuffer != NULL)
            {
                uint8_t* buf = states[i].eventBuffer;
                states[i].eventBuffer = NULL;
                free(buf);
            }
        }
    }

    free(reader->states);
}

bool midiNextEvent(midiFileReader_t* reader, midiEvent_t* event)
{
    uint32_t minTime = UINT32_MAX;
    // Pointer to the next track
    struct midiTrackState* nextTrack = NULL;

    if (!reader->file)
    {
        return false;
    }

    // TODO: This treats all formats like a format 1 (simultaneous)
    for (int i = 0; i < reader->file->trackCount; i++)
    {
        midiTrackState_t* info = &reader->states[i];

        if (!info->eventParsed && info->nextEvent.deltaTime != UINT32_MAX)
        {
            // Avoid trying and failing to parse a track we've already finished
            if (info->done)
            {
                continue;
            }
            info->eventParsed = trackParseNext(reader, info);
        }

        // Check if we either already have a parsed event waiting, or are able to parse one now
        // Short-circuiting will make sure we only parse another event when needed and permitted
        // TODO doesn't this basically do the same thing as the block above?
        if (info->eventParsed || (info->nextEvent.deltaTime != UINT32_MAX && trackParseNext(reader, info)))
        {
            // info->nextEvent has now been set by trackParseNext()
            if (!info->nextEvent.deltaTime /*|| reader->file->format == MIDI_FORMAT_2*/)
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

void* globalMidiSave(void)
{
    // TODO: There are multiple allocs here, so the return value _can't_ safely be free()'d by others
    midiSaveState_t* saveState = malloc(NUM_GLOBAL_PLAYERS * sizeof(midiSaveState_t));

    for (int i = 0; i < NUM_GLOBAL_PLAYERS; i++)
    {
        midiPlayer_t* player = globalMidiPlayerGet(i);
        memcpy(&saveState[i].player, player, sizeof(midiPlayer_t));

        if (player->reader.file != NULL)
        {
            saveState[i].trackCount = player->reader.file->trackCount;
            saveState[i].trackStates = malloc(saveState[i].trackCount * sizeof(midiTrackState_t));

            // Overwrite the copy with the newly allocated pointer, since the current one may be free'd
            saveState[i].player.reader.states = saveState[i].trackStates;

            for (int trackIdx = 0; trackIdx < saveState[i].trackCount; trackIdx++)
            {
                const midiTrackState_t* stateOrig = &player->reader.states[trackIdx];
                midiTrackState_t* stateCopy = &saveState[i].trackStates[trackIdx];

                memcpy(stateCopy, stateOrig, sizeof(midiTrackState_t));
                if (stateCopy->eventBuffer != NULL)
                {
                    stateCopy->eventBuffer = malloc(stateCopy->eventBufferSize);
                    memcpy(stateCopy->eventBuffer, stateOrig->eventBuffer, stateCopy->eventBufferSize);
                }
            }
        }
    }

    return saveState;
}

void globalMidiRestore(void* data)
{
    midiSaveState_t* saveState = (midiSaveState_t*)data;

    for (int i = 0; i < NUM_GLOBAL_PLAYERS; i++)
    {
        midiPlayer_t* player = globalMidiPlayerGet(i);
        midiPlayerReset(player);

        memcpy(player, &saveState[i].player, sizeof(midiPlayer_t));
    }

    // Do not free any of the individual save state data, since it's now just the real data
    // Just free the container
    free(data);
}