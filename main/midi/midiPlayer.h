#pragma once
#include "hdw-bzr.h"
#include "swSynth.h"
#include "midiFileParser.h"

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define MIDI_CHANNEL_COUNT 16
// TODO: Channel-independent dynamic voice allocation
// The number of simultaneous voices each channel can support
#define VOICE_PER_CHANNEL 3
// The number of voices reserved for percussion
#define PERCUSSION_VOICES 8
// The number of oscillators each voice gets. Maybe we'll need more than one for like, chorus?
#define OSC_PER_VOICE 1

#define MIDI_TRUE 0x7F
#define MIDI_FALSE 0x00
#define MIDI_TO_BOOL(val) (val > 63)
#define BOOL_TO_MIDI(val) (val ? MIDI_TRUE : MIDI_FALSE)

typedef enum
{
    /// @brief Streaming over USB
    MIDI_STREAMING,

    /// @brief Reading from a \c midiFileReader_t
    MIDI_FILE,
} midiPlayerMode_t;

/**
 * @brief Describes the characteristics of a particular timbre while
 *
 */
typedef struct
{
    // Time taken to ramp up to full volume
    int32_t attack;

    // Time taken for the volume to fade to the sustain volume
    int32_t decay;

    // Time it takes to silence the note after release
    int32_t release;

    // The volume of the sustain note, proportional to the original volume
    uint8_t sustain;

    /// @brief The index of the repeat loop
    //uint32_t loopStart;
    /// @brief The index at which the loop should repeat
    //uint32_t loopEnd;

} envelope_t;

typedef enum
{
    ES_STOPPED = 0,
    ES_ATTACK,
    ES_DECAY,
    ES_SUSTAIN,
    ES_RELEASE,
} envelopeState_t;

typedef enum
{
    // Roland GS Extensions
    /*HIGH_Q_OR_FILTER_SNAP = 27,
    SLAP_NOISE = 28,
    SCRATCH_PUSH = 29,
    SCRATCH_PULL = 30,
    DRUM_STICKS = 31,
    SQUARE_CLICK = 32,
    METRONOME_CLICK = 33,
    METRONOME_BELL = 34,*/
    // End Roland GS Extensions
    ACOUSTIC_BASS_DRUM_OR_LOW_BASS_DRUM = 35,
    ELECTRIC_BASS_DRUM_OR_HIGH_BASS_DRUM = 36,
    SIDE_STICK = 37,
    ACOUSTIC_SNARE = 38,
    HAND_CLAP = 39,
    ELECTRIC_SNARE_OR_RIMSHOT = 40,
    LOW_FLOOR_TOM = 41,
    CLOSED_HI_HAT = 42,
    HIGH_FLOOR_TOM = 43,
    PEDAL_HI_HAT = 44,
    LOW_TOM = 45,
    OPEN_HI_HAT = 46,
    LOW_MID_TOM = 47,
    HIGH_MID_TOM = 48,
    CRASH_CYMBAL_1 = 49,
    HIGH_TOM = 50,
    RIDE_CYMBAL_1 = 51,
    CHINESE_CYMBAL = 52,
    RIDE_BELL = 53,
    TAMBOURINE = 54,
    SPLASH_CYMBAL = 55,
    COWBELL = 56,
    CRASH_CYMBAL_2 = 57,
    VIBRASLAP = 58,
    RIDE_CYMBAL_2 = 59,
    HIGH_BONGO = 60,
    LOW_BONGO = 61,
    MUTE_HIGH_CONGA = 62,
    OPEN_HIGH_CONGA = 63,
    LOW_CONGA = 64,
    HIGH_TIMBALE = 65,
    LOW_TIMBALE = 66,
    HIGH_AGOGO = 67,
    LOW_AGOGO = 68,
    CABASA = 69,
    MARACAS = 70,
    SHORT_WHISTLE = 71,
    LONG_WHISTLE = 72,
    SHORT_GUIRO = 73,
    LONG_GUIRO = 74,
    CLAVES = 75,
    HIGH_WOODBLOCK = 76,
    LOW_WOODBLOCK = 77,
    MUTE_CUICA = 78,
    OPEN_CUICA = 79,
    MUTE_TRIANGLE = 80,
    OPEN_TRIANGLE = 81,
    // Roland GS Extensions
    /*SHAKER = 82,
    JINGLE_BELL = 83,
    BELLTREE = 84,
    CASTANETS = 85,
    MUTE_SURDO = 86,
    OPEN_SURDO = 87,*/
    // End Roland GS Extensions
} percussionNote_t;

/**
 * @brief The sample source for an instrument
 *
 */
typedef enum
{
    WAVETABLE,
    SAMPLE,
    NOISE,
} timbreType_t;

/**
 * @brief A bitfield which may contain various flags for a timbre
 */
typedef enum
{
    /// @brief No flags
    TF_NONE = 0,
    /// @brief This timbre plays percussion sounds (percussionNote_t) rather than melodic notes
    TF_PERCUSSION = 1,
    /// @brief This timbre represents a monophonic instrument
    TF_MONO = 2,
} timbreFlags_t;

/**
 * @brief A function that returns samples for a percussion timbre rather than a melodic one
 *
 * @param drum The percussion instrument to generate sound for
 * @param idx The monotonic sample index within this note. Will not repeat for any particular note.
 * @param[out] done A pointer to a boolean to be set to 1 when the drum sample is finished playing
 * @param[in,out] scratch A pointer to an array of 4 uint32_t that will persist for the duration of the note
 * @param data A pointer to user-defined data which may be used in sample generation
 */
typedef int8_t (*percussionFunc_t)(percussionNote_t drum, uint32_t idx, bool* done, uint32_t scratch[4], void* data);

/**
 * @brief A function to handle text meta-messages from playing MIDI files
 *
 * @param type The type of meta-message
 * @param text The message text
 */
typedef void (*midiTextCallback_t)(metaEventType_t type, const char* text);

/**
 * @brief Defines the sound characteristics of a particular instrument.
 */
typedef struct
{
    /// @brief The source of samples for this instrument
    timbreType_t type;

    /// @brief Flags bitfield for this timbre
    timbreFlags_t flags;

    union
    {
        /// @brief The index of this timbre's wave in the table, when type is WAVETABLE
        uint16_t waveIndex;

        struct {
            /// @brief The frequency of the base sample to be used when pitch shifting

            // This should just always be C4? (440 << 8)
            //uint32_t freq = (440 << 8);

            /// @brief A pointer to this timbre's sample data, when type is SAMPLE
            int8_t* data;

            /// @brief The length of the sample in bytes
            uint32_t count;

            /// @brief The sample rate divisor. Each audio sample will be played this many times.
            /// The audio data's actual sample rate should be (32768 / rate)
            uint8_t rate;
        } sample;

        struct {
            /// @brief A callback to call for drum data
            percussionFunc_t playFunc;
            /// @brief User data to pass to the drumkit
            void* data;
        } percussion;
    };

    /// @brief The ASDR characterstics of this timbre
    envelope_t envelope;

    /// @brief The name of this timbre, if any
    const char* name;
} midiTimbre_t;

/**
 * @brief Tracks the state of a single voice, playing a single note.
 *
 * A single voice could use multiple oscillators to achieve a particular timbre.
 */
typedef struct
{
    /// @brief The number of ticks remaining before transitioning to the next state
    uint32_t transitionTicks;

    /// @brief The target volume of this tick
    uint8_t targetVol;

    /// @brief The monotonic tick counter for playback of sampled timbres
    uint32_t sampleTick;

    /// @brief The MIDI note number for the sound being played
    uint8_t note;

    /// @brief The synthesizer oscillators used to generate the sounds
    synthOscillator_t oscillators[OSC_PER_VOICE];

    /// @brief An array of scratch data for percussion functions to use
    // TODO union this with the oscillators? They shouldn't both be used
    // But we need to make sure the oscillators don't get summed
    uint32_t percScratch[4];

    /// @brief The timbre of this voice, which defines its musical characteristics
    // TODO: Should this be a pointer instead?
    // We may be asked to modify envelope, etc. so maybe just memcpy into here?
    // program change isn't a super lightweight event anyhow
    midiTimbre_t* timbre;
} midiVoice_t;

/**
 * @brief Holds several bitfields that track the state of each voice for fast access.
 * This may be used for dynamic voice allocation, and to minimize the impact of note stealing
 * if we run out of voices.
 */
typedef struct
{
    /// @brief Whether this note is set to on via MIDI, regardless of if it's making sound
    uint32_t on;

    /*
    /// @brief Bitfield of voices currently in the attack stage
    uint32_t attack;

    /// @brief Bitfield of voices currently in the sustain stage
    uint32_t sustain;

    /// @brief Bitfield of voices currently in the decay stage
    uint32_t decay;

    /// @brief Bitfield of voices currently in the release stage
    uint32_t release;
    */

    /// @brief Bitfield of voices which are being held by the pedal
    uint32_t held;
}
voiceStates_t;

/**
 * @brief Tracks the state of a single MIDI channel
 */
typedef struct
{
    /// @brief The 14-bit volume level for this channel only
    uint16_t volume;

    /// @brief The ID of the program (timbre) set for this channel
    uint8_t program;

    /// @brief The actual current timbre definition which the program ID corresponds to
    midiTimbre_t timbre;

    /// @brief The state of each voice allocated to this channel
    // TODO: This will eventually be replaced by a single bank of voices which are dynamically allocated at the MIDI player level
    midiVoice_t voices[VOICE_PER_CHANNEL];

    /// @brief This channel's voice state bitmap
    voiceStates_t voiceStates;

    /// @brief The 14-bit pitch wheel value
    uint16_t pitchBend;

    /// @brief Whether this channel is reserved for percussion.
    bool percussion;

    /// @brief Whether notes will be held
    bool held;

} midiChannel_t;

/**
 * @brief Tracks the state of the entire MIDI apparatus.
 */
typedef struct
{
    /// @brief The global 14-bit volume level
    uint16_t volume;

    /// @brief The state of all MIDI channels
    midiChannel_t channels[MIDI_CHANNEL_COUNT];

    /// @brief The voices reserved for percussion
    midiVoice_t percVoices[PERCUSSION_VOICES];

    /// @brief The percussion voice state bitmap
    voiceStates_t percVoiceStates;

    /// @brief A bitmap to track which percussion voices have special notes playing
    /// This includes all 3 hi-hats (open, closed, and pedal), short and long whistle and guiro,
    /// and mute and open cuica and triangle. This is 5 states bitpacked into 6 bits each.
    uint32_t percSpecialStates;

    /// @brief An array holding a pointer to every oscillator
    synthOscillator_t* allOscillators[(MIDI_CHANNEL_COUNT * VOICE_PER_CHANNEL + PERCUSSION_VOICES) * OSC_PER_VOICE];

    /// @brief The total number of oscillators in the array. Could be less than the max if some are unused
    uint16_t oscillatorCount;

    /// @brief Whether this player is playing a song or a MIDI stream
    midiPlayerMode_t mode;

    /// @brief A MIDI reader to use for file playback, when in MIDI_FILE mode
    midiFileReader_t* reader;

    /// @brief A callback to call when a text meta-message is received
    midiTextCallback_t textMessageCallback;

    /// @brief Number of samples that were clipped
    uint32_t clipped;

    /// @brief The number of samples elapsed in the songs
    uint64_t sampleCount;

    /// @brief The next event in the MIDI file, which occurs after the current time
    midiEvent_t pendingEvent;

    /// @brief True if pendingEvent is valid, false if it must be updated
    bool eventAvailable;

    /// @brief The number of microseconds per quarter note
    uint32_t tempo;
} midiPlayer_t;

/**
 * @brief Initialize or reset the MIDI player
 *
 * This includes setting up the dac callback
 *
 * @param player
 */
void midiPlayerInit(midiPlayer_t* player);

/**
 * @brief Fill a buffer with the next set of samples from the MIDI player. This should be called by the
 * callback passed into initDac(). Samples are generated at sampling rate of ::DAC_SAMPLE_RATE_HZ
 *
 * @param samples An array of unsigned 8-bit samples to fill
 * @param len The length of the array to fill
 */
void midiPlayerFillBuffer(midiPlayer_t* player, uint8_t* samples, int16_t len);

/**
 * @brief Stop all sound immediately. This is not affected by the sustain pedal.
 *
 * @param player The player to stop
 */
void midiAllSoundOff(midiPlayer_t* player);

/**
 * @brief Tun off all notes which are currently on, as though midiNoteOff() were called
 * for each note. This respects the sustain pedal.
 *
 * @param player The MIDI player
 * @param channel The MIDI channel on which to stop notes
 */
void midiAllNotesOff(midiPlayer_t* player, uint8_t channel);

/**
 * @brief Begin playing a note on a given MIDI channel.
 *
 * Using a velocity of \c 0 will stop the note.
 *
 * @param player The MIDI player
 * @param channel The MIDI channel on which to start the note
 * @param note The note number to play
 * @param velocity The note velocity which affects its volume.
 */
void midiNoteOn(midiPlayer_t* player, uint8_t channel, uint8_t note, uint8_t velocity);

/**
 * @brief Stop playing a particular note on a given MIDI channel
 *
 * @param player The MIDI player
 * @param channel The MIDI channel on which to stop the note
 * @param note The note number to stop
 * @param velocity [NYI] The release velocity which affects the note's release time
 */
void midiNoteOff(midiPlayer_t* player, uint8_t channel, uint8_t note, uint8_t velocity);

/**
 * @brief Change the program (instrument) on a given MIDI channel
 *
 * @param player The MIDI player
 * @param channel The MIDI channel whose program will be changed
 * @param program The program ID, from 0-127, to set for this channel
 */
void midiSetProgram(midiPlayer_t* player, uint8_t channel, uint8_t program);

/**
 * @brief Set the hold pedal status.
 *
 * This is a convenience method for midiControlChange(player, channel, CONTROL_HOLD (64), val ? 127:0)
 *
 * @param player The MIDI player
 * @param channel The MIDI channel to set the hold status for
 * @param val The sustain pedal value. Values 0-63 are OFF, and 64-127 are ON.
 */
void midiSustain(midiPlayer_t* player, uint8_t channel, uint8_t val);

/**
 * @brief Set a MIDI control value
 *
 * @param player The MIDI player
 * @param channel The channel to set the control on
 * @param control The control number to set
 * @param val The control value, from 0-127 whose meaning depends on the control number
 */
void midiControlChange(midiPlayer_t* player, uint8_t channel, uint8_t control, uint8_t val);

/**
 * @brief Set the pitch wheel value on a given MIDI channel
 *
 * By default, the center of the pitch wheel is \c 0x2000. A value of \c 0x0000 transposes
 * one step up, while a value of \c 0x3FFF transposes one step up.
 *
 * [NYI] The range of the pitch wheel can be changed using the registered parameters, with
 * MSB being the range in (+/-)semitones and LSB being the range in (+/-) cents
 *
 * @param player The MIDI player
 * @param channel The MIDI channel to change the pitch wheel for
 * @param value The pitch wheel value, from 0 to 0x3FFF (14-bits)
 */
void midiPitchWheel(midiPlayer_t* player, uint8_t channel, uint16_t value);

/**
 * @brief Configure this MIDI player to read from a MIDI file
 *
 * @param player The MIDI player
 * @param reader The MIDI reader that contains the MIDI file to be played
 */
void midiSetFile(midiPlayer_t* player, midiFileReader_t* reader);
