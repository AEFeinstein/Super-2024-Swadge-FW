#pragma once

//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>
#include <stdbool.h>

//==============================================================================
// Enums
//==============================================================================

/**
 * @brief The possible sub-types of MIDI events
 */
typedef enum
{
    /// @brief A normal MIDI status event, such as note on or off
    MIDI_EVENT,
    /// @brief A non-MIDI meta-event from a MIDI file, such as tempo or lyrics
    META_EVENT,
    /// @brief A system-exclusive MIDI event
    SYSEX_EVENT,
} midiEventType_t;

/**
 * @brief The possible types of meta-events
 */
typedef enum
{
    SEQUENCE_NUMBER        = 0x00,
    TEXT                   = 0x01,
    COPYRIGHT              = 0x02,
    SEQUENCE_OR_TRACK_NAME = 0x03,
    INSTRUMENT_NAME        = 0x04,
    LYRIC                  = 0x05,
    MARKER                 = 0x06,
    CUE_POINT              = 0x07,
    // I'm not sure these are real???
    // PROGRAM_NAME,
    // DEVICE_NAME,
    CHANNEL_PREFIX = 0x20,
    PORT_PREFIX    = 0x21,
    END_OF_TRACK   = 0x2F,
    TEMPO          = 0x51,
    SMPTE_OFFSET   = 0x54,
    TIME_SIGNATURE = 0x58,
    KEY_SIGNATURE  = 0x59,
    PROPRIETARY    = 0x7F,
} metaEventType_t;

/// @brief The MIDI file format, which determines how to interpret the track or tracks it contains
typedef enum
{
    /// @brief One track only containing all MIDI data for any number of channels
    MIDI_FORMAT_0 = 0,
    /// @brief Multiple MIDI tracks, played simultaneously
    MIDI_FORMAT_1 = 1,
    /// @brief Multiple MIDI tracks, played sequentially
    MIDI_FORMAT_2 = 2,
} midiFileFormat_t;

//==============================================================================
// Structs
//==============================================================================

/**
 * @brief Contains basic information pointing to a MIDI track within its file data
 */
typedef struct
{
    /// @brief Total chunk length
    uint32_t length;

    /// @brief Pointer to the start of this chunk's data
    uint8_t* data;
} midiTrack_t;

/**
 * @brief Contains information which applies to the entire MIDI file
 */
typedef struct
{
    /// @brief A pointer to the start of the MIDI file
    uint8_t* data;

    /// @brief The total length of the MIDI file
    uint32_t length;

    /// @brief The MIDI file format which defines how this file's tracks are interpreted
    midiFileFormat_t format;

    /// @brief The time division of MIDI frames, either ticks per frame or ticks per quarter note
    uint16_t timeDivision;

    /// @brief The number of tracks in this file
    uint16_t trackCount;

    /// @brief An array of MIDI tracks
    midiTrack_t* tracks;
} midiFile_t;

typedef struct midiTrackState midiTrackState_t;

typedef struct
{
    /// @brief A pointer to the MIDI file currently loaded into the reader, if any
    const midiFile_t* file;

    /// @brief If true, text meta-events will be handled and sent to the MIDI player
    bool handleMetaEvents;

    /// @brief The number of divisions per midi tick
    uint16_t division;

    /// @brief The number of track states allocated
    uint8_t stateCount;

    /// @brief An array containing the internal parser state for each track
    midiTrackState_t* states;
} midiFileReader_t;

/**
 * @brief Data for a normal MIDI status event
 */
typedef struct
{
    /// @brief The MIDI status byte
    uint8_t status;

    /// @brief The data bytes of the MIDI status event.
    /// The meaning of these bytes depends on the status value.
    uint8_t data[2];
} midiStatusEvent_t;

/**
 * @brief Data for a MIDI time signature definition
 */
typedef struct
{
    /// @brief The numerator of the time signature
    uint8_t numerator;
    /// @brief The power of two of the time signature denominator (e.g. 2 for 4/4, 3 for 4/8)
    uint8_t denominator;
    /// @brief The number of MIDI clocks per metronome tick
    uint8_t midiClocksPerMetronomeTick;
    /// @brief Number of 32nd notes per 24 MIDI clocks (= 1 beat)
    uint8_t num32ndNotesPerBeat;
} midiTimeSignature_t;

/**
 * @brief Contains information for a non-MIDI meta-event from a MIDI file
 */
typedef struct
{
    /// @brief The type of this MIDI meta-event
    metaEventType_t type;

    /// @brief The number of data bytes this meta event contains
    uint32_t length;

    union
    {
        /// @brief Contains text data, when \c {type <= 0x0F}
        /// @warning This data is \b NOT NUL-terminated
        const char* text;

        /// @brief Contains binary data, when type is ::PROPRIETARY
        const uint8_t* data;

        /// @brief Contains a tempo, in microseconds per quarter note, when type is ::TEMPO
        uint32_t tempo;

        /// @brief Contains a sequence number for this track, when type is ::SEQUENCE_NUMBER
        uint16_t sequenceNumber;

        /// @brief Contains a channel or port prefix, when type is ::CHANNEL_PREFIX or ::PORT_PREFIX respectively
        uint8_t prefix;

        /// @brief Contains the start time of this track, when type is ::SMPTE_OFFSET
        struct
        {
            uint8_t hour;
            uint8_t min;
            uint8_t sec;
            uint8_t frame;
            uint8_t frameHundredths;
        } startTime;

        /// @brief Contains time signature data, when type is ::TIME_SIGNATURE
        midiTimeSignature_t timeSignature;

        /// @brief Contains key signature data, when type is ::KEY_SIGNATURE
        /// @note At most one of \c flats or \c sharps will contain a nonzero value.
        struct
        {
            /// @brief The number of flats in the key
            uint8_t flats;

            /// @brief The number of sharps in the key
            uint8_t sharps;

            /// @brief True for a minor key, false for a major key
            bool minor;
        } keySignature;
    };
} midiMetaEvent_t;

/**
 * @brief Contains information for a MIDI System Exclusive event
 */
typedef struct
{
    /// @brief The manufacturer ID embedded in the SysEx event
    /// @note When this is a single-byte manufacturer ID, the most-significant bit (15) will be set
    uint16_t manufacturerId;

    /// @brief The length of the data contained in this SysEx event
    uint32_t length;

    /// @brief A byte to prefix to the actual data, if non-zero
    uint8_t prefix;

    /// @brief A pointer to the data for this SysEx event.
    const uint8_t* data;
} midiSysexEvent_t;

/**
 * @brief Contains information for an entire MIDI event or non-MIDI meta-event
 */
typedef struct
{
    /// @brief The time between this event and the previous event
    uint32_t deltaTime;

    /// @brief The absolute timestamp of this event in ticks
    uint32_t absTime;

    /// @brief The overall event type -- MIDI, Meta, or SysEx
    midiEventType_t type;

    /// @brief The index of the track which contains this event
    uint8_t track;

    union
    {
        /// @brief The MIDI status event data, when type is ::MIDI_EVENT
        midiStatusEvent_t midi;
        /// @brief The non-MIDI meta-event data, when type is ::META_EVENT
        midiMetaEvent_t meta;
        /// @brief The MIDI System Exclusive event data, when type is ::SYSEX_EVENT
        midiSysexEvent_t sysex;
    };
} midiEvent_t;

//==============================================================================
// Function Declarations
//==============================================================================

/**
 * @brief Load a MIDI file from the filesystem
 *
 * @param file A pointer to a midiFile_t struct to load the file into
 * @param name The name of the MIDI file to load
 * @param spiRam Whether to load the MIDI file into SPIRAM
 * @return true If the load succeeded
 * @return false If the load failed
 */
bool loadMidiFile(const char* name, midiFile_t* file, bool spiRam);

/**
 * @brief Free the data associated with the given MIDI file
 *
 * @param file A pointer to the MIDI file to be unloaded
 */
void unloadMidiFile(midiFile_t* file);

/**
 * @brief Initialize or reset the MIDI file reader with a particular file.
 *
 * @param reader A pointer to the MIDI file reader to initialize
 * @param file A pointer to the MIDI file to load
 * @return true if the MIDI file reader was initialized
 * @return false if an error occurred while allocating data for the MIDI file reader
 */
bool initMidiParser(midiFileReader_t* reader, const midiFile_t* file);

/**
 * @brief Set a new file for the MIDI file reader. The reader's state will be reset.
 *
 * @param reader A pointer to the MIDI file reader to set the file of
 * @param file A pointer to the MIDI file to load
 */
void midiParserSetFile(midiFileReader_t* reader, const midiFile_t* file);

/**
 * @brief Reset the state of the MIDI parser without deinitializing it or changing the file.
 *
 * The next event returned from this MIDI parser will be the first event in the file again.
 *
 * @param reader A pointer to the MIDI file reader to reset
 */
void resetMidiParser(midiFileReader_t* reader);

/**
 * @brief Deinitialize the MIDI file reader and free any memory it has allocated
 *
 * @param reader The MIDI file reader to deinitialize
 */
void deinitMidiParser(midiFileReader_t* reader);

/**
 * @brief Return the start time of the next event in the MIDI file being read
 *
 * @param reader The reader to check
 * @return uint32_t The timestamp of the next event, or UINT32_MAX if there is none
 */
uint32_t midiNextEventTime(midiFileReader_t* reader);

/**
 * @brief Process and retrieve the next MIDI event in the file
 *
 * @param reader The reader to read the event from
 * @param event A pointer to a MIDI event to be updated with the next event
 * @return true If event data was written to event
 * @return false If there are no more events in this file or there was a fatal parse error
 */
bool midiNextEvent(midiFileReader_t* reader, midiEvent_t* event);

/**
 * @brief Writes a MIDI event to a byte buffer
 *
 * @param out The byte array
 * @param max The maximum number of bytes to write
 * @param event The event to write
 * @return int The number of bytes written
 */
int midiWriteEvent(uint8_t* out, int max, const midiEvent_t* event);
