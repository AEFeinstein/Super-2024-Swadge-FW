#pragma once

//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>
#include <stdbool.h>

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    MIDI_EVENT,
    META_EVENT,
    SYSEX_EVENT,
} midiEventType_t;

typedef enum
{
    SEQUENCE_NUMBER = 0x00,
    TEXT = 0x01,
    COPYRIGHT = 0x02,
    SEQUENCE_OR_TRACK_NAME = 0x03,
    INSTRUMENT_NAME = 0x04,
    LYRIC = 0x05,
    MARKER = 0x06,
    CUE_POINT = 0x07,
    // I'm not sure these are real???
    //PROGRAM_NAME,
    //DEVICE_NAME,
    CHANNEL_PREFIX = 0x20,
    END_OF_TRACK = 0x2F,
    TEMPO = 0x51,
    SMPTE_OFFSET = 0x54,
    TIME_SIGNATURE = 0x58,
    KEY_SIGNATURE = 0x59,
    PROPRIETARY = 0x7F,
} metaEventType_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct midiReaderState midiReaderState_t;

typedef struct
{
    uint8_t* data;
    uint32_t length;

    /// @brief If true, text meta-events will be handled and sent to the MIDI player
    bool handleMetaEvents;

    /// @brief The number of divisions per midi tick
    uint16_t division;

    midiReaderState_t* state;
} midiFileReader_t;

typedef struct
{
    uint8_t status;
    uint8_t data[3];
} midiStatusEvent_t;

typedef struct
{
    metaEventType_t type;
    uint32_t length;
    union {
        const char* text;
        uint8_t* data;
        uint32_t tempo;
        uint16_t sequenceNumber;

        struct {
            uint8_t hour;
            uint8_t min;
            uint8_t sec;
            uint8_t frame;
            uint8_t frameHundredths;
        } startTime;

        struct {
            /// @brief The numerator of the time signature
            uint8_t numerator;
            /// @brief The power of two of the time signature denominator (e.g. 2 for 4/4, 3 for 4/8)
            uint8_t denominator;
            /// @brief The number of MIDI clocks per metronome tick
            uint8_t midiClocksPerMetronomeTick;
            /// @brief Number of 32nd notes per 24 MIDI clocks (= 1 beat)
            uint8_t num32ndNotesPerBeat;
        } timeSignature;

        struct {
            uint8_t flats;
            uint8_t sharps;
            bool minor;
        } keySignature;
    };
} midiMetaEvent_t;

typedef struct
{
    uint16_t manufacturerId;
    uint32_t length;
    uint8_t* data;
} midiSysexEvent_t;

typedef struct
{
    /// @brief The time between this event and the previous event
    uint32_t deltaTime;

    /// @brief The absolute timestamp of this event in ticks
    uint32_t absTime;

    /// @brief The overall event type -- MIDI, Meta, or SysEx
    midiEventType_t type;

    union {
        midiStatusEvent_t midi;
        midiMetaEvent_t meta;
        midiSysexEvent_t sysex;
    };
} midiEvent_t;

//==============================================================================
// Function Declarations
//==============================================================================

/**
 * @brief Load a MIDI file from SPIFFS
 *
 * @param reader The reader to load the file into
 * @param file The name of the MIDI file to load
 * @param spiRam Whether to load the MIDI file into SPIRAM
 * @return true If the load succeeded
 * @return false If the load failed
 */
bool loadMidiFile(midiFileReader_t* reader, const char* file, bool spiRam);

/**
 * @brief Free the MIDI file loaded into the given reader
 *
 * @param reader The reader to unload the file from
 */
void unloadMidiFile(midiFileReader_t* reader);

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
 * @return false If there are no more events in this file
 */
bool midiNextEvent(midiFileReader_t* reader, midiEvent_t* event);

/**
 * @brief Process the next event in the file and play it with the given player
 *
 * @param reader The MIDI file reader to retrieve the next event from
 * @param player The MIDI player to send the next event to
 * @return true If there was a next event and it was sent
 * @return false If there are no more events in this file
 */
//bool midiPlayNext(midiFileReader_t* reader, midiPlayer_t* player);
