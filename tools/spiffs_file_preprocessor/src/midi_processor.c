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

typedef struct
{
    //* @brief Note frequency, in Hz */
    noteFrequency_t note;
    //* @brief Note duration, in ms */
    uint16_t timeMs;
} musicalNote_t;

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
    musicalNote_t* allNotes = NULL;
    uint32_t allNoteIdx     = 0;

    /* Look for the first track with notes */
    for (int trackIdx = 0; trackIdx < midiParser->nbOfTracks; trackIdx++)
    {
        /* Only process the first track with notes */
        Track* track = &(midiParser->tracks[trackIdx]);
        if (track->nbOfNotes > 0)
        {
            /* Allocate enough space for all the notes with rests in between them */
            allNotes = malloc((2 * track->nbOfNotes) * sizeof(musicalNote_t));

            /* For each note */
            for (int noteIdx = 0; noteIdx < track->nbOfNotes; noteIdx++)
            {
                /* Get a reference to this note */
                Note* note = &(track->notes[noteIdx]);

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
                allNotes[allNoteIdx].note   = midiFreqs[note->pitch];
                allNotes[allNoteIdx].timeMs = note->duration;
                allNoteIdx++;

                /* If there is a note after this one */
                if (NULL != nextNote)
                {
                    /* Check if this note ends before the next begins */
                    if (note->timeBeforeAppear + note->duration < nextNote->timeBeforeAppear)
                    {
                        /* If it does, add a rest to the output between notes */
                        allNotes[allNoteIdx].note = SILENCE;
                        allNotes[allNoteIdx].timeMs
                            = nextNote->timeBeforeAppear - (note->timeBeforeAppear + note->duration);
                        allNoteIdx++;
                    }
                }
            }

            /* Break to not process further tracks */
            break;
        }
    }

    /* Cleanup the parser */
    deleteMidiParserStruct(midiParser);

    /* If notes were parsed */
    if (NULL != allNotes)
    {
        /* Put all the uncomressed song bytes in an array with specific byte order*/
        uint8_t* uncompressedSong = malloc(4 + (4 * allNoteIdx));
        uint32_t uIdx             = 0;

        /* Write number of notes */
        uncompressedSong[uIdx++] = (allNoteIdx >> 24) & 0xFF;
        uncompressedSong[uIdx++] = (allNoteIdx >> 16) & 0xFF;
        uncompressedSong[uIdx++] = (allNoteIdx >> 8) & 0xFF;
        uncompressedSong[uIdx++] = (allNoteIdx >> 0) & 0xFF;

        /* Write each note and duration */
        for (uint32_t noteIdx = 0; noteIdx < allNoteIdx; noteIdx++)
        {
            uncompressedSong[uIdx++] = (allNotes[noteIdx].note >> 8) & 0xFF;
            uncompressedSong[uIdx++] = (allNotes[noteIdx].note >> 0) & 0xFF;
            uncompressedSong[uIdx++] = (allNotes[noteIdx].timeMs >> 8) & 0xFF;
            uncompressedSong[uIdx++] = (allNotes[noteIdx].timeMs >> 0) & 0xFF;
        }

        /* Write the compressed bytes to a file */
        writeHeatshrinkFile(uncompressedSong, uIdx, outFilePath);

        /* Cleanup */
        free(allNotes);
        free(uncompressedSong);
    }
}
