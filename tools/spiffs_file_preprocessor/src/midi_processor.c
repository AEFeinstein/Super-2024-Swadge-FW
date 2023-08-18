#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "heatshrink_util.h"
#include "midi_parser.h"

#include "midi_processor.h"
#include "fileUtils.h"

/* Frequencies of notes */
typedef enum __attribute__((packed))
{
    SILENCE    = 0,
    C_0        = 16,
    C_SHARP_0  = 17,
    D_0        = 18,
    D_SHARP_0  = 19,
    E_0        = 21,
    F_0        = 22,
    F_SHARP_0  = 23,
    G_0        = 25,
    G_SHARP_0  = 26,
    A_0        = 28,
    A_SHARP_0  = 29,
    B_0        = 31,
    C_1        = 33,
    C_SHARP_1  = 35,
    D_1        = 37,
    D_SHARP_1  = 39,
    E_1        = 41,
    F_1        = 44,
    F_SHARP_1  = 46,
    G_1        = 49,
    G_SHARP_1  = 52,
    A_1        = 55,
    A_SHARP_1  = 58,
    B_1        = 62,
    C_2        = 65,
    C_SHARP_2  = 69,
    D_2        = 73,
    D_SHARP_2  = 78,
    E_2        = 82,
    F_2        = 87,
    F_SHARP_2  = 93,
    G_2        = 98,
    G_SHARP_2  = 104,
    A_2        = 110,
    A_SHARP_2  = 117,
    B_2        = 123,
    C_3        = 131,
    C_SHARP_3  = 139,
    D_3        = 147,
    D_SHARP_3  = 156,
    E_3        = 165,
    F_3        = 175,
    F_SHARP_3  = 185,
    G_3        = 196,
    G_SHARP_3  = 208,
    A_3        = 220,
    A_SHARP_3  = 233,
    B_3        = 247,
    C_4        = 262,
    C_SHARP_4  = 277,
    D_4        = 294,
    D_SHARP_4  = 311,
    E_4        = 330,
    F_4        = 349,
    F_SHARP_4  = 370,
    G_4        = 392,
    G_SHARP_4  = 415,
    A_4        = 440,
    A_SHARP_4  = 466,
    B_4        = 494,
    C_5        = 523,
    C_SHARP_5  = 554,
    D_5        = 587,
    D_SHARP_5  = 622,
    E_5        = 659,
    F_5        = 698,
    F_SHARP_5  = 740,
    G_5        = 784,
    G_SHARP_5  = 831,
    A_5        = 880,
    A_SHARP_5  = 932,
    B_5        = 988,
    C_6        = 1047,
    C_SHARP_6  = 1109,
    D_6        = 1175,
    D_SHARP_6  = 1245,
    E_6        = 1319,
    F_6        = 1397,
    F_SHARP_6  = 1480,
    G_6        = 1568,
    G_SHARP_6  = 1661,
    A_6        = 1760,
    A_SHARP_6  = 1865,
    B_6        = 1976,
    C_7        = 2093,
    C_SHARP_7  = 2217,
    D_7        = 2349,
    D_SHARP_7  = 2489,
    E_7        = 2637,
    F_7        = 2794,
    F_SHARP_7  = 2960,
    G_7        = 3136,
    G_SHARP_7  = 3322,
    A_7        = 3520,
    A_SHARP_7  = 3729,
    B_7        = 3951,
    C_8        = 4186,
    C_SHARP_8  = 4435,
    D_8        = 4699,
    D_SHARP_8  = 4978,
    E_8        = 5274,
    F_8        = 5588,
    F_SHARP_8  = 5920,
    G_8        = 6272,
    G_SHARP_8  = 6645,
    A_8        = 7040,
    A_SHARP_8  = 7459,
    B_8        = 7902,
    C_9        = 8372,
    C_SHARP_9  = 8870,
    D_9        = 9397,
    D_SHARP_9  = 9956,
    E_9        = 10548,
    F_9        = 11175,
    F_SHARP_9  = 11840,
    G_9        = 12544,
    G_SHARP_9  = 13290,
    A_9        = 14080,
    A_SHARP_9  = 14917,
    B_9        = 15804,
    C_10       = 16744,
    C_SHARP_10 = 17740,
    D_10       = 18795,
    D_SHARP_10 = 19912,
    E_10       = 21096,
    F_10       = 22351,
    F_SHARP_10 = 23680,
    G_10       = 25088,
    G_SHARP_10 = 26580,
    A_10       = 28160,
    A_SHARP_10 = 29834,
    B_10       = 31609
} noteFrequency_t;

// The number of physical buzzers
#define MAX_NUM_CHANNELS 2

/**
 * @brief Enum for which buzzer a note should play out of
 */
typedef enum
{
    BZR_LEFT,  ///< Play out of the left buzzer
    BZR_RIGHT, ///< Play out of the right buzzer
    BZR_STEREO ///< Play out of both buzzers
} buzzerPlayChannel_t;

/**
 * @brief A single note and duration to be played on the buzzer
 */
typedef struct
{
    noteFrequency_t note; ///< Note frequency, in Hz
    uint32_t timeMs;      ///< Note duration, in ms
} musicalNote_t;

typedef struct
{
    int8_t majorKey;
    int8_t minorKey;
    int tempo; // uS per quarter note
    uint8_t timeSigNumerator;
    uint8_t timeSigDenominator;
    uint8_t timeSigClocksPerClick;
    uint8_t timeSig32ndNotePerQuarter;
} midiParams_t;

/* Look up table from MIDI pitch index to frequency (Hz) */
static const uint32_t midiFreqs[128] = {
    8,    9,    9,    10,   10,   11,   12,   12,   13,   14,   15,    15,    16,    17,   18,   19,   21,   22,   23,
    25,   26,   28,   29,   31,   33,   35,   37,   39,   41,   44,    46,    49,    52,   55,   58,   62,   65,   69,
    73,   78,   82,   87,   93,   98,   104,  110,  117,  123,  131,   139,   147,   156,  165,  175,  185,  196,  208,
    220,  233,  247,  262,  277,  294,  311,  330,  349,  370,  392,   415,   440,   466,  494,  523,  554,  587,  622,
    659,  698,  740,  784,  831,  880,  932,  988,  1047, 1109, 1175,  1245,  1319,  1397, 1480, 1568, 1661, 1760, 1865,
    1976, 2093, 2217, 2349, 2489, 2637, 2794, 2960, 3136, 3322, 3520,  3729,  3951,  4186, 4435, 4699, 4978, 5274, 5588,
    5920, 6272, 6645, 7040, 7459, 7902, 8372, 8870, 9397, 9956, 10548, 11175, 11840, 12544};

/**
 * @brief
 *
 * @param midiParser
 */
static void checkMidiEvents(MidiParser* midiParser, unsigned long int lastNoteStart, unsigned long int thisNoteStart,
                            midiParams_t* params);

/**
 * Process a MIDI file and write a compressed song (.sng)
 *
 * @param infile The MIDI file to process
 * @param outdir The output file name
 */
void process_midi(const char* infile, const char* outdir)
{
    /* Determine if the output file already exists */
    char outFilePath[128] = {0};
    strcat(outFilePath, outdir);
    strcat(outFilePath, "/");
    strcat(outFilePath, get_filename(infile));

    /* Change the file extension */
    char* dotptr = strrchr(outFilePath, '.');
    snprintf(&dotptr[1], strlen(dotptr), "sng");

    if (doesFileExist(outFilePath))
    {
        /* printf("Output for %s already exists\n", infile); */
        return;
    }

    /* Parse the MIDI file */
    MidiParser* midiParser = parseMidi(infile, false, true);

    /* Declare variables to translate the notes to */
    uint8_t channelIdx                     = 0;
    musicalNote_t* notes[MAX_NUM_CHANNELS] = {NULL, NULL};
    uint32_t noteIdxs[MAX_NUM_CHANNELS]    = {0, 0};
    uint32_t totalLength[MAX_NUM_CHANNELS] = {0, 0};
    midiParams_t params                    = {0};

    /* Look for the first track with notes */
    for (int trackIdx = 0; trackIdx < midiParser->nbOfTracks; trackIdx++)
    {
        /* Only process the first track with notes */
        Track* track = &(midiParser->tracks[trackIdx]);
        if (track->nbOfNotes > 0)
        {
            if (channelIdx < MAX_NUM_CHANNELS)
            {
                /* Allocate enough space for all the notes with rests in between them */
                notes[channelIdx] = calloc((2 * track->nbOfNotes), sizeof(musicalNote_t));

                /* For each note */
                unsigned long int lastNoteStart = track->notes[0].timeBeforeAppear;
                for (int noteIdx = 0; noteIdx < track->nbOfNotes; noteIdx++)
                {
                    /* Get a reference to this note */
                    Note* note = &(track->notes[noteIdx]);

                    /* Before processing the note, check for events*/
                    checkMidiEvents(midiParser, lastNoteStart, note->timeBeforeAppear, &params);

                    /* Get a reference to the next note, if it exists */
                    Note* nextNote = NULL;
                    if ((noteIdx + 1) < track->nbOfNotes)
                    {
                        nextNote = &(track->notes[noteIdx + 1]);
                    }

                    /* If there is a note after this one */
                    if (NULL != nextNote)
                    {
                        /* Check if this note ends after the next begins */
                        if ((note->timeBeforeAppear + note->duration) > nextNote->timeBeforeAppear)
                        {
                            /* If it does, shorten this note's duration to not overlap */
                            note->duration = (nextNote->timeBeforeAppear - note->timeBeforeAppear);
                        }
                    }

                    /* Save this note */
                    notes[channelIdx][noteIdxs[channelIdx]].note = midiFreqs[note->pitch];
                    notes[channelIdx][noteIdxs[channelIdx]].timeMs
                        = (params.tempo * note->duration) / (1000 * midiParser->ticks);
                    totalLength[channelIdx] += notes[channelIdx][noteIdxs[channelIdx]].timeMs;
                    noteIdxs[channelIdx]++;

                    /* If there is a note after this one */
                    if (NULL != nextNote)
                    {
                        /* Check if this note ends before the next begins */
                        if (note->timeBeforeAppear + note->duration < nextNote->timeBeforeAppear)
                        {
                            /* If it does, add a rest to the output between notes */
                            notes[channelIdx][noteIdxs[channelIdx]].note = SILENCE;
                            notes[channelIdx][noteIdxs[channelIdx]].timeMs
                                = (params.tempo
                                   * (nextNote->timeBeforeAppear - (note->timeBeforeAppear + note->duration)))
                                  / (1000 * midiParser->ticks);
                            totalLength[channelIdx] += notes[channelIdx][noteIdxs[channelIdx]].timeMs;
                            noteIdxs[channelIdx]++;
                        }
                    }

                    // Save this for the next loop
                    lastNoteStart = note->timeBeforeAppear;
                }
                // Move to the next channel
                channelIdx++;
            }
        }
    }

    if(totalLength[0] < totalLength[1])
    {
        // Pad out track 0 to be the same length as track 1
        notes[0][noteIdxs[0]].note = SILENCE;
        notes[0][noteIdxs[0]].timeMs = totalLength[1] - totalLength[0];
        noteIdxs[0]++;
    }
    else if (totalLength[0] > totalLength[1])
    {
        // Pad out track 1 to be the same length as track 0
        notes[1][noteIdxs[1]].note = SILENCE;
        notes[1][noteIdxs[1]].timeMs = totalLength[1] - totalLength[0];
        noteIdxs[1]++;
    }

    /* Cleanup the parser */
    deleteMidiParserStruct(midiParser);

    /* If notes were parsed */
    if (NULL != notes[0])
    {
        uint32_t totalSongBytes = 4; // Four bytes for the number of channels
        for (uint8_t ch = 0; ch < channelIdx; ch++)
        {
            totalSongBytes += 4;                  // Number of notes per channel
            totalSongBytes += (4 * noteIdxs[ch]); // Space for the nodes
        }

        /* Put all the uncomressed song bytes in an array with specific byte order*/
        uint8_t* uncompressedSong = calloc(1, totalSongBytes);
        uint32_t uIdx             = 0;

        /* Write number of channels */
        uncompressedSong[uIdx++] = (channelIdx >> 24) & 0xFF;
        uncompressedSong[uIdx++] = (channelIdx >> 16) & 0xFF;
        uncompressedSong[uIdx++] = (channelIdx >> 8) & 0xFF;
        uncompressedSong[uIdx++] = (channelIdx >> 0) & 0xFF;

        for (uint8_t ch = 0; ch < channelIdx; ch++)
        {
            /* Write number of notes */
            uncompressedSong[uIdx++] = (noteIdxs[ch] >> 24) & 0xFF;
            uncompressedSong[uIdx++] = (noteIdxs[ch] >> 16) & 0xFF;
            uncompressedSong[uIdx++] = (noteIdxs[ch] >> 8) & 0xFF;
            uncompressedSong[uIdx++] = (noteIdxs[ch] >> 0) & 0xFF;

            /* Write each note and duration */
            for (uint32_t noteIdx = 0; noteIdx < noteIdxs[ch]; noteIdx++)
            {
                uncompressedSong[uIdx++] = (notes[ch][noteIdx].note >> 8) & 0xFF;
                uncompressedSong[uIdx++] = (notes[ch][noteIdx].note >> 0) & 0xFF;
                uncompressedSong[uIdx++] = (notes[ch][noteIdx].timeMs >> 8) & 0xFF;
                uncompressedSong[uIdx++] = (notes[ch][noteIdx].timeMs >> 0) & 0xFF;
            }
        }

        /* Write the compressed bytes to a file */
        writeHeatshrinkFile(uncompressedSong, uIdx, outFilePath);

        /* Cleanup */
        for (uint8_t ch = 0; ch < channelIdx; ch++)
        {
            free(notes[ch]);
        }
        free(uncompressedSong);
    }
}

/**
 * @brief Check for midi events to process in between notes, after the last one and before or equal to the current time
 *
 * @param midiParser The midi parser
 * @param lastNoteStart The time the last note was played
 * @param thisNoteStart The time the current note was played
 * @param params The parameters to write to
 */
static void checkMidiEvents(MidiParser* midiParser, unsigned long int lastNoteStart, unsigned long int thisNoteStart,
                            midiParams_t* params)
{
    /* For each track */
    for (int trackIdx = 0; trackIdx < midiParser->nbOfTracks; trackIdx++)
    {
        Track* track = &(midiParser->tracks[trackIdx]);
        /* For each event */
        for (int evtIdx = 0; evtIdx < track->nbOfEvents; evtIdx++)
        {
            Event* evt = &track->events[evtIdx];
            // printf("%5d: %s\n", evt->timeToAppear, evt->type);

            // If this event occurs between the notes we're at, process it
            if ((0 == lastNoteStart && 0 == thisNoteStart && 0 == evt->timeToAppear)
                || (lastNoteStart < evt->timeToAppear && evt->timeToAppear <= thisNoteStart))
            {
                switch (evt->type)
                {
                    case MidiNewTimeSignature:
                    {
                        // printf(
                        // "        Tempo infos: time signature %i/%i 1/4 note is %i ticks %i\n",
                        //     ,
                        //     ((unsigned char *)evt->infos)[1],
                        //     ((unsigned char *)evt->infos)[2],
                        //     ((unsigned char *)evt->infos)[3]
                        // );
                        params->timeSigNumerator          = ((unsigned char*)evt->infos)[0];
                        params->timeSigDenominator        = ((unsigned char*)evt->infos)[1];
                        params->timeSigClocksPerClick     = ((unsigned char*)evt->infos)[2];
                        params->timeSig32ndNotePerQuarter = ((unsigned char*)evt->infos)[3];
                        break;
                    }
                    case MidiNewKeySignature:
                    {
                        // printf(
                        // 	"        Key signature %i %i\n",
                        // 	((char *)evt->infos)[0],
                        // 	((char *)evt->infos)[1]
                        // );
                        params->majorKey = ((char*)evt->infos)[0];
                        params->minorKey = ((char*)evt->infos)[1];
                        break;
                    }
                    case MidiTempoChanged:
                    {
                        // printf("        Tempo changed: %i\n", *(int *)evt->infos);
                        params->tempo = *(int*)evt->infos;
                        break;
                    }
                    case MidiTextEvent:
                    case MidiSequenceNumber:
                    case MidiNewLyric:
                    case MidiNewMarker:
                    case MidiNewCuePoint:
                    case MidiNewChannelPrefix:
                    case MidiPortChange:
                    case MidiSMTPEOffset:
                    case MidiSequencerSpecificEvent:
                    case MidiNoteReleased:
                    case MidiNotePressed:
                    case MidiPolyphonicPressure:
                    case MidiControllerValueChanged:
                    case MidiProgramChanged:
                    case MidiPressureOfChannelChanged:
                    case MidiPitchBendChanged:
                    {
                        break;
                    }
                }
            }
        }
        // printf("---\n");
    }
}
