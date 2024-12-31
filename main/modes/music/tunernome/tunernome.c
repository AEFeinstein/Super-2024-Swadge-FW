/**
 * @file mode_tunernome.c
 * @author Brycey92
 * @brief All the musical tools you could want, if you only want a tuner and metronome
 * @date 2023-08-14
 */

/*============================================================================
 * Includes
 *==========================================================================*/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>

#include "embeddedNf.h"
#include "embeddedOut.h"
#include "esp_timer.h"
#include "linked_list.h"
#include "mainMenu.h"
#include "settingsManager.h"

#include "tunernome.h"

/*============================================================================
 * Defines, Structs, Enums
 *==========================================================================*/

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

// Screen curve compensation
#define CORNER_OFFSET 15

// Tuner musical and audio definitions
#define NUM_MAX_STRINGS              6 // change this if you add an instrument with more strings
#define NUM_GUITAR_STRINGS           ARRAY_SIZE(guitarNoteNames)
#define NUM_VIOLIN_STRINGS           ARRAY_SIZE(violinNoteNames)
#define NUM_UKULELE_STRINGS          ARRAY_SIZE(ukuleleNoteNames)
#define NUM_BANJO_STRINGS            ARRAY_SIZE(banjoNoteNames)
#define GUITAR_OFFSET                0
#define CHROMATIC_OFFSET             6 // adjust start point by quarter tones
#define SENSITIVITY                  5
#define TONAL_DIFF_IN_TUNE_DEVIATION 10

// Tuner screen definitions
#define TUNER_CENTER_Y                (TFT_HEIGHT - tunernome->ibm_vga8.height - 8 - CORNER_OFFSET)
#define TUNER_RADIUS                  80
#define TUNER_TEXT_Y_OFFSET           30
#define TUNER_ARROW_Y_OFFSET          8
#define TUNER_STRING_WIDTH            3
#define TUNER_STRING_X_PADDING        (TFT_WIDTH / 7)

// Metronome musical and audio definitions
#define INITIAL_BPM                   120
#define MAX_BPM                       400
#define MAX_BEATS                     16 // I mean, realistically, who's going to have more than 16 beats in a measure?
// #define METRONOME_CLICK_MS            35 // superseded by midis

// Metronome screen definitions
#define METRONOME_GRAPHIC_BASE_HEIGHT 25
#define METRONOME_GRAPHIC_Y           70
#define METRONOME_CENTER_X            (TFT_WIDTH / 2)
#define METRONOME_CENTER_Y            (TFT_HEIGHT - (2 * tunernome->ibm_vga8.height) - 10 - CORNER_OFFSET - METRONOME_GRAPHIC_BASE_HEIGHT)
#define METRONOME_RADIUS              (125 - METRONOME_GRAPHIC_BASE_HEIGHT)
#define METRONOME_ARC_DEGREES         90
#define METRONOME_ARC_MULTIPLIER      (METRONOME_ARC_DEGREES / 180.0)
#define METRONOME_WEIGHT_SIZE_RADIUS  5 // total diameter is 1 + (2 * this value)
#define METRONOME_WEIGHT_MIN_RADIUS   (METRONOME_WEIGHT_SIZE_RADIUS + 2.0) // 1px for center of weight, and 1px for tip of arm
#define METRONOME_WEIGHT_MAX_RADIUS   (METRONOME_RADIUS - METRONOME_WEIGHT_MIN_RADIUS)
#define METRONOME_WEIGHT_RADIUS_RANGE (METRONOME_WEIGHT_MAX_RADIUS - METRONOME_WEIGHT_MIN_RADIUS)

// Metronome button repeat delays
#define BPM_CHANGE_FIRST_MS           500
#define BPM_CHANGE_FAST_MS            2000
#define BPM_CHANGE_REPEAT_MS          50

// Musical constant, do not change
#define NUM_SEMITONES 12

typedef enum
{
    TN_TUNER,
    TN_METRONOME
} tnMode;

typedef enum
{
    GUITAR_TUNER = 0,
    VIOLIN_TUNER,
    UKULELE_TUNER,
    BANJO_TUNER,
    SEMITONE_0,
    SEMITONE_1,
    SEMITONE_2,
    SEMITONE_3,
    SEMITONE_4,
    SEMITONE_5,
    SEMITONE_6,
    SEMITONE_7,
    SEMITONE_8,
    SEMITONE_9,
    SEMITONE_10,
    SEMITONE_11,
    LISTENING,
    MAX_GUITAR_MODES
} tuner_mode_t;

typedef enum
{
    NO_TUNE,
    FLAT_TUNE,
    IN_TUNE,
    SHARP_TUNE
} tune_state_t;

typedef struct
{
    tnMode mode;
    tuner_mode_t curTunerMode;

    font_t ibm_vga8;
    font_t radiostars;
    font_t logbook;

    buttonBit_t lastBpmButton;
    uint32_t bpmButtonCurChangeUs;
    uint32_t bpmButtonStartUs;
    uint32_t bpmButtonAccumulatedUs;

    dft32_data dd;
    embeddedNf_data end;
    embeddedOut_data eod;
    int audioSamplesProcessed;
    uint32_t intensities_filt[CONFIG_NUM_LEDS];
    int32_t diffs_filt[CONFIG_NUM_LEDS];
    tune_state_t stringTuneStates[NUM_MAX_STRINGS];

    uint8_t beatCtr;
    int bpm;
    int32_t tAccumulatedUs;
    bool isClockwise;
    int32_t usPerBeat;
    uint8_t beatLength;

    int32_t clickTimerUs;
    uint8_t midiNote;

    uint32_t semitone_intensity_filt[NUM_SEMITONES];
    int32_t semitone_diff_filt[NUM_SEMITONES];
    int16_t tonalDiff[NUM_SEMITONES];
    int16_t intensity[NUM_SEMITONES];

    wsg_t radiostarsArrowWsg;
    wsg_t logbookArrowWsg;
    wsg_t flatWsg;
    wsg_t backgroundWsg;
    wsg_t metronomeTopWsg;
    wsg_t metronomeBottomWsg;

    int32_t blinkTimerUs;
    bool isSilent;
    bool isTouched;
    bool isShowingStrings;
} tunernome_t;

// typedef struct
// {
//     uint8_t top;
//     uint8_t bottom;
// } timeSignature;

/*============================================================================
 * Prototypes
 *==========================================================================*/

void tunernomeEnterMode(void);
void tunernomeExitMode(void);
void switchToSubmode(tnMode);
void tunernomeProcessButtons(buttonEvt_t* evt);
void modifyBpm(int16_t bpmMod);
void tunernomeSampleHandler(uint16_t* samples, uint32_t sampleCnt);
void recalcMetronome(void);
void plotInstrumentStrings(uint16_t numNotes);
void plotInstrumentNameAndNotes(const char* instrumentName, const char* const* instrumentNotes, uint16_t numNotes);
void plotTopSemiCircle(int xm, int ym, int r, paletteColor_t col);
void instrumentTunerMagic(const uint16_t freqBinIdxs[], uint16_t numStrings, led_t colors[],
                          const uint16_t stringIdxToLedIdx[]);
void tunernomeMainLoop(int64_t elapsedUs);
void ledReset(void* timer_arg);
void fasterBpmChange(void* timer_arg);
void tunernomeBackgroundDrawCb(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);

static inline int16_t getMagnitude(uint16_t idx);
static inline int16_t getDiffAround(uint16_t idx);
static inline int16_t getSemiMagnitude(int16_t idx);
static inline int16_t getSemiDiffAround(uint16_t idx);

/*============================================================================
 * Variables
 *==========================================================================*/

swadgeMode_t tunernomeMode = {.modeName                 = "Tunernome",
                              .wifiMode                 = NO_WIFI,
                              .overrideUsb              = false,
                              .usesAccelerometer        = false,
                              .usesThermometer          = false,
                              .overrideSelectBtn        = false,
                              .fnEnterMode              = tunernomeEnterMode,
                              .fnExitMode               = tunernomeExitMode,
                              .fnMainLoop               = tunernomeMainLoop,
                              .fnAudioCallback          = tunernomeSampleHandler,
                              .fnBackgroundDrawCallback = tunernomeBackgroundDrawCb,
                              .fnEspNowRecvCb           = NULL,
                              .fnEspNowSendCb           = NULL,
                              .fnAdvancedUSB            = NULL};

tunernome_t* tunernome;

/*============================================================================
 * Const Variables
 *==========================================================================*/

/**
 * Indicies into fuzzed_bins[], a realtime DFT of sorts
 * fuzzed_bins[0] = A ... 1/2 steps are every 2.
 */
const uint16_t freqBinIdxsGuitar[] = {
    38, // E string needs to skip an octave... Can't read sounds this low.
    24, // A string is exactly at note #24
    34, // D = A + 5 half steps = 34
    44, // G
    52, // B
    62  // e
};

/**
 * Indicies into fuzzed_bins[], a realtime DFT of sorts
 * fuzzed_bins[0] = A ... 1/2 steps are every 2.
 */
const uint16_t freqBinIdxsViolin[] = {
    44, // G
    58, // D
    72, // A
    86  // E
};

/**
 * Indicies into fuzzed_bins[], a realtime DFT of sorts
 * fuzzed_bins[0] = A ... 1/2 steps are every 2.
 */
const uint16_t freqBinIdxsUkulele[] = {
    68, // G
    54, // C
    62, // E
    72  // A
};

/**
 * Indicies into fuzzed_bins[], a realtime DFT of sorts
 * fuzzed_bins[0] = A ... 1/2 steps are every 2.
 */
const uint16_t freqBinIdxsBanjo[] = {
    68, // G
    34, // D = A + 5 half steps = 34
    44, // G
    52, // B
    58  // D
};

const uint16_t fourNoteStringIdxToLedIdx[] = {1, 3, 5, 8};

const uint16_t fiveNoteStringIdxToLedIdx[] = {1, 2, 3, 5, 8};

const uint16_t sixNoteStringIdxToLedIdx[] = {0, 1, 3, 4, 5, 8};

const char* const guitarNoteNames[] = {"E2", "A2", "D3", "G3", "B3", "E4"};

const char* const violinNoteNames[] = {"G3", "D4", "A4", "E5"};

const char* const ukuleleNoteNames[] = {"G4", "C4", "E4", "A4"};

const char* const banjoNoteNames[] = {"G4", "D3", "G3", "B3", "D4"};

// End a string ending with "\1" to draw the flat symbol
const char* const semitoneNoteNames[]
    = {"C", "C#/D\1", "D", "D#/E\1", "E", "F", "F#/G\1", "G", "G#/A\1", "A", "A#/B\1", "B"};

static const char theWordGuitar[]      = "Guitar";
static const char theWordViolin[]      = "Violin";
static const char theWordUkulele[]     = "Ukulele";
static const char theWordBanjo[]       = "Banjo";
static const char playStringsText[]    = "Play all open strings";
static const char listeningText[]      = "Listening for a note";
static const char leftStr[]            = ": ";
static const char rightStrTuner1[]     = "Touch: Beep";
static const char rightStrTuner2[]     = "Pause: Tuner";
static const char rightStrMetronome1[] = "Touch: Strings";
static const char rightStrMetronome2[] = "Pause: Metronome";
static const char inTuneStr[]          = "In-Tune";
static const char sharpStr[]           = "Sharp";

// TODO: these should be const after being assigned
static int TUNER_FLAT_THRES_X;
static int TUNER_SHARP_THRES_X;
static int TUNER_THRES_Y;

// TODO: We can't use the mic and the speaker anymore, so I don't really know what to do with this anyway.
/*
static musicalNote_t metronome_primary_notes[] = {{
    .note   = F_SHARP_5,
    .timeMs = METRONOME_CLICK_MS,
}};
static songTrack_t metronome_primary_tracks[]  = {{
     .numNotes      = ARRAY_SIZE(metronome_primary_notes),
     .loopStartNote = 0,
     .notes         = metronome_primary_notes,
}};
song_t metronome_primary
    = {.numTracks = ARRAY_SIZE(metronome_primary_tracks), .shouldLoop = false, .tracks = metronome_primary_tracks};

static musicalNote_t metronome_secondary_notes[] = {{
    .note   = F_SHARP_4,
    .timeMs = METRONOME_CLICK_MS,
}};
static songTrack_t metronome_secondary_tracks[]  = {{
     .numNotes      = ARRAY_SIZE(metronome_secondary_notes),
     .loopStartNote = 0,
     .notes         = metronome_secondary_notes,
}};
song_t metronome_secondary
    = {.numTracks = ARRAY_SIZE(metronome_secondary_tracks), .shouldLoop = false, .tracks = metronome_secondary_tracks};
*/

/*============================================================================
 * Functions
 *==========================================================================*/

/**
 * Initializer for tunernome
 */
void tunernomeEnterMode(void)
{
    // Allocate zero'd memory for the mode
    tunernome = heap_caps_calloc(1, sizeof(tunernome_t), MALLOC_CAP_8BIT);

    loadFont("ibm_vga8.font", &tunernome->ibm_vga8, false);
    loadFont("radiostars.font", &tunernome->radiostars, false);
    loadFont("logbook.font", &tunernome->logbook, false);

    float intermedX     = cosf(TONAL_DIFF_IN_TUNE_DEVIATION * M_PI / 17);
    float intermedY     = sinf(TONAL_DIFF_IN_TUNE_DEVIATION * M_PI / 17);
    TUNER_SHARP_THRES_X = round(METRONOME_CENTER_X - (intermedX * TUNER_RADIUS));
    TUNER_FLAT_THRES_X  = round(METRONOME_CENTER_X + (intermedX * TUNER_RADIUS));
    TUNER_THRES_Y       = round(TUNER_CENTER_Y - (ABS(intermedY) * TUNER_RADIUS));

    loadWsg("arrow12.wsg", &(tunernome->radiostarsArrowWsg), false);
    loadWsg("arrow18.wsg", &(tunernome->logbookArrowWsg), false);
    loadWsg("flat_mm.wsg", &(tunernome->flatWsg), false);
    loadWsg("woodBackground.wsg", &(tunernome->backgroundWsg), false);
    loadWsg("metronomeTop.wsg", &(tunernome->metronomeTopWsg), false);
    loadWsg("metronomeBottom.wsg", &(tunernome->metronomeBottomWsg), false);

    tunernome->beatLength   = 4;
    tunernome->beatCtr      = 0;
    tunernome->bpm          = INITIAL_BPM;
    tunernome->curTunerMode = GUITAR_TUNER;

    switchToSubmode(TN_TUNER);

    InitColorChord(&tunernome->end, &tunernome->dd);

    tunernome->blinkTimerUs = 0;
    tunernome->clickTimerUs = 0;
    tunernome->isSilent     = false;
    tunernome->isShowingStrings = true;
}

/**
 * Switch internal mode
 */
void switchToSubmode(tnMode newMode)
{
    switch (newMode)
    {
        case TN_TUNER:
        {
            // Disable speaker, enable mic
            switchToMicrophone();

            tunernome->mode = newMode;

            led_t leds[CONFIG_NUM_LEDS] = {{0}};
            setLeds(leds, CONFIG_NUM_LEDS);

            clearPxTft();

            break;
        }
        case TN_METRONOME:
        {
            // Disable mic, enable speaker
            switchToSpeaker();

            // Configure MIDI for streaming
            midiPlayer_t* player      = globalMidiPlayerGet(MIDI_BGM);
            player->mode              = MIDI_STREAMING;
            player->streamingCallback = NULL;
            midiGmOn(player);
            midiPause(player, false);

            tunernome->mode = newMode;

            tunernome->isClockwise = true;
            // This assures that the first beat is a primary beat/downbeat
            tunernome->beatCtr        = tunernome->beatLength - 1;
            tunernome->tAccumulatedUs = 0;

            tunernome->lastBpmButton          = 0;
            tunernome->bpmButtonCurChangeUs   = 0;
            tunernome->bpmButtonStartUs       = 0;
            tunernome->bpmButtonAccumulatedUs = 0;

            tunernome->blinkTimerUs = 0;
            tunernome->clickTimerUs = 0;

            recalcMetronome();

            led_t leds[CONFIG_NUM_LEDS] = {{0}};
            setLeds(leds, CONFIG_NUM_LEDS);

            clearPxTft();
            break;
        }
        default:
        {
            break;
        }
    }
}

/**
 * Called when tunernome is exited
 */
void tunernomeExitMode(void)
{
    soundStop(true);

    freeFont(&tunernome->ibm_vga8);
    freeFont(&tunernome->radiostars);
    freeFont(&tunernome->logbook);

    freeWsg(&(tunernome->radiostarsArrowWsg));
    freeWsg(&(tunernome->logbookArrowWsg));
    freeWsg(&(tunernome->flatWsg));
    freeWsg(&(tunernome->backgroundWsg));
    freeWsg(&(tunernome->metronomeTopWsg));
    freeWsg(&(tunernome->metronomeBottomWsg));

    heap_caps_free(tunernome);
}

/**
 * Inline helper function to get the magnitude of a frequency bin from fuzzed_bins[]
 *
 * @param idx The index to get a magnitude from
 * @return A signed magnitude, even though fuzzed_bins[] is unsigned
 */
static inline int16_t getMagnitude(uint16_t idx)
{
    return tunernome->end.fuzzed_bins[idx];
}

/**
 * Inline helper function to get the difference in magnitudes around a given
 * frequency bin from fuzzed_bins[]
 *
 * @param idx The index to get a difference in magnitudes around
 * @return The difference in magnitudes of the bins before and after the given index
 */
static inline int16_t getDiffAround(uint16_t idx)
{
    return getMagnitude(idx + 1) - getMagnitude(idx - 1);
}

/**
 * Inline helper function to get the magnitude of a frequency bin from folded_bins[]
 *
 * @param idx The index to get a magnitude from
 * @return A signed magnitude, even though folded_bins[] is unsigned
 */
static inline int16_t getSemiMagnitude(int16_t idx)
{
    if (idx < 0)
    {
        idx += FIX_B_PER_O;
    }
    if (idx > FIX_B_PER_O - 1)
    {
        idx -= FIX_B_PER_O;
    }
    return tunernome->end.folded_bins[idx];
}

/**
 * Inline helper function to get the difference in magnitudes around a given
 * frequency bin from folded_bins[]
 *
 * @param idx The index to get a difference in magnitudes around
 * @return The difference in magnitudes of the bins before and after the given index
 */
static inline int16_t getSemiDiffAround(uint16_t idx)
{
    return getSemiMagnitude(idx + 1) - getSemiMagnitude(idx - 1);
}

/**
 * Recalculate the per-bpm values for the metronome
 */
void recalcMetronome(void)
{
    // Figure out how many microseconds are in one beat
    tunernome->usPerBeat = (60 * 1000000) / tunernome->bpm;
}

/**
 * Plot instrument strings as colored vertical lines spread evenly across the display
 * @param numNotes The number of notes in instrumentsNotes
 */
void plotInstrumentStrings(uint16_t numNotes)
{
    int16_t stringXRange = TFT_WIDTH - 2 * TUNER_STRING_X_PADDING;
    int16_t totalStringWidths = numNotes * TUNER_STRING_WIDTH;
    int16_t totalInnerSpacingWidths = stringXRange - totalStringWidths;
    int16_t perStringXOffset = TUNER_STRING_WIDTH + round(totalInnerSpacingWidths / (float) (numNotes - 1));

    for(int i = 0; i < numNotes; i++)
    {
        paletteColor_t color;
        switch(tunernome->stringTuneStates[i])
        {
            case FLAT_TUNE:
                color = c003;
                break;
            case IN_TUNE:
                color = c222;
                break;
            case SHARP_TUNE:
                color = c300;
                break;
            case NO_TUNE:
            default:
                color = c000;
                break;
        }

        int16_t stringXOffset = TUNER_STRING_X_PADDING + perStringXOffset * i;
        drawRectFilled(stringXOffset, 0, stringXOffset + (TUNER_STRING_WIDTH - 1), TFT_HEIGHT - 1, color);
    }
}

/**
 * Plot instrument name and the note names of strings, arranged to match LED positions, in middle of display
 * @param instrumentName The name of the instrument to plot to the display
 * @param instrumentNotes The note names of the strings of the instrument to plot to the display
 * @param numNotes The number of notes in instrumentsNotes
 */
void plotInstrumentNameAndNotes(const char* instrumentName, const char* const* instrumentNotes, uint16_t numNotes)
{
    // Mode name
    drawText(&tunernome->logbook, c555, instrumentName,
             (TFT_WIDTH - textWidth(&tunernome->logbook, instrumentName)) / 2,
             (TFT_HEIGHT - tunernome->logbook.height) / 2 - TUNER_TEXT_Y_OFFSET);

    // Note names of strings, arranged to match LED positions
    bool oddNumLedRows = (int)ceil(numNotes / 2.0f) % 2 == 1;
    // Left Column
    for (int i = 0; i < ceil(numNotes / 2.0f); i++)
    {
        int y;
        if (oddNumLedRows)
        {
            y = (TFT_HEIGHT - tunernome->logbook.height) / 2 + (tunernome->logbook.height + 5) * (1 - i)
                - TUNER_TEXT_Y_OFFSET;
        }
        else
        {
            y = TFT_HEIGHT / 2 + (tunernome->logbook.height + 5) * (-i) + 2 - TUNER_TEXT_Y_OFFSET;
        }

        char buf[2] = {0};
        strncpy(buf, instrumentNotes[i], 1);
        drawText(&tunernome->logbook, c555, buf,
                 (TFT_WIDTH - textWidth(&tunernome->logbook, instrumentName)) / 2
                     - textWidth(&tunernome->logbook, /*placeholder for widest note name + ' '*/ "A4 "),
                 y);

        strncpy(buf, instrumentNotes[i] + 1, 1);
        drawText(&tunernome->logbook, c555, buf,
                 (TFT_WIDTH - textWidth(&tunernome->logbook, instrumentName)) / 2
                     - textWidth(&tunernome->logbook, /*placeholder for widest octave number + ' '*/ "4 "),
                 y);
    }
    oddNumLedRows = (int)floor(numNotes / 2.0f) % 2 == 1;
    // Right Column
    for (int i = ceil(numNotes / 2.0f); i < numNotes; i++)
    {
        int y;
        if (oddNumLedRows)
        {
            y = (TFT_HEIGHT - tunernome->logbook.height) / 2
                + (tunernome->logbook.height + 5) * (i - ceil(numNotes / 2.0f) - 1) - TUNER_TEXT_Y_OFFSET;
        }
        else
        {
            y = TFT_HEIGHT / 2 + (tunernome->logbook.height + 5) * (i - ceil(numNotes / 2.0f) - 1) + 2
                - TUNER_TEXT_Y_OFFSET;
        }

        char buf[2] = {0};
        strncpy(buf, instrumentNotes[i], 1);
        drawText(&tunernome->logbook, c555, buf,
                 (TFT_WIDTH + textWidth(&tunernome->logbook, instrumentName)) / 2 + textWidth(&tunernome->logbook, " "),
                 y);

        strncpy(buf, instrumentNotes[i] + 1, 1);
        drawText(
            &tunernome->logbook, c555, buf,
            (TFT_WIDTH + textWidth(&tunernome->logbook, instrumentName)) / 2
                + textWidth(&tunernome->logbook, /*' ' + placeholder for widest note name without octave number*/ " A"),
            y);
    }

    drawText(&tunernome->radiostars, c555, playStringsText,
             (TFT_WIDTH - textWidth(&tunernome->radiostars, playStringsText)) / 2,
             TUNER_CENTER_Y - (TUNER_RADIUS + tunernome->radiostars.height) / 2);
}

/**
 * Instrument-agnostic tuner magic. Updates LEDs
 * @param freqBinIdxs An array of the indices of notes for the instrument's strings. See freqBinIdxsGuitar for an
 * example.
 * @param numStrings The number of strings on the instrument, also the number of elements in freqBinIdxs and
 * stringIdxToLedIdx, if applicable
 * @param colors The RGB colors of the LEDs to set
 * @param stringIdxToLedIdx A remapping from each index into freqBinIdxs (same index into stringIdxToLedIdx), to the
 * index of an LED to map that string/freqBinIdx to. Set to NULL to skip remapping.
 */
void instrumentTunerMagic(const uint16_t freqBinIdxs[], uint16_t numStrings, led_t colors[],
                          const uint16_t stringIdxToLedIdx[])
{
    uint32_t i;
    for (i = 0; i < numStrings; i++)
    {
        // Pick out the current magnitude and filter it
        tunernome->intensities_filt[i] = (getMagnitude(freqBinIdxs[i] + GUITAR_OFFSET) + tunernome->intensities_filt[i])
                                         - (tunernome->intensities_filt[i] >> 5);

        // Pick out the difference around current magnitude and filter it too
        tunernome->diffs_filt[i] = (getDiffAround(freqBinIdxs[i] + GUITAR_OFFSET) + tunernome->diffs_filt[i])
                                   - (tunernome->diffs_filt[i] >> 5);

        // This is the magnitude of the target frequency bin, cleaned up
        int16_t intensity = (tunernome->intensities_filt[i] >> SENSITIVITY) - 40; // drop a baseline.
        intensity         = CLAMP(intensity, 0, 255);

        // This is the tonal difference. You "calibrate" out the intensity.
        int16_t tonalDiff = (tunernome->diffs_filt[i] >> SENSITIVITY) * 200 / (intensity + 1);

        int32_t red, grn, blu;
        // Is the note in tune, i.e. is the magnitude difference in surrounding bins small?
        if ((ABS(tonalDiff) < TONAL_DIFF_IN_TUNE_DEVIATION))
        {
            // Note is in tune, make it white
            red = 255;
            grn = 255;
            blu = 255;
            tunernome->stringTuneStates[i] = IN_TUNE;
        }
        else
        {
            // Check if the note is sharp or flat
            if (tonalDiff > 0)
            {
                // Note too sharp, make it red
                red = 255;
                grn = blu = 255 - (tonalDiff) * 15;
                tunernome->stringTuneStates[i] = SHARP_TUNE;
            }
            else
            {
                // Note too flat, make it blue
                blu = 255;
                grn = red = 255 - (-tonalDiff) * 15;
                tunernome->stringTuneStates[i] = FLAT_TUNE;
            }

            // Make sure LED output isn't more than 255
            red = CLAMP(red, INT_MIN, 255);
            grn = CLAMP(grn, INT_MIN, 255);
            blu = CLAMP(blu, INT_MIN, 255);
        }

        // Scale each LED's brightness by the filtered intensity for that bin
        red = (red >> 3) * (intensity >> 3);
        grn = (grn >> 3) * (intensity >> 3);
        blu = (blu >> 3) * (intensity >> 3);

        // Ensure each channel is between 0 and 255
        red = CLAMP(red, 0, 255);
        grn = CLAMP(grn, 0, 255);
        blu = CLAMP(blu, 0, 255);

        // Set the LED
        colors[(stringIdxToLedIdx != NULL) ? stringIdxToLedIdx[i] : i].r = red;
        colors[(stringIdxToLedIdx != NULL) ? stringIdxToLedIdx[i] : i].g = grn;
        colors[(stringIdxToLedIdx != NULL) ? stringIdxToLedIdx[i] : i].b = blu;

        // Unset the tune state for the string if its LED is off
        if(red == 0 && grn == 0 && blu == 0)
        {
            tunernome->stringTuneStates[i] = NO_TUNE;
        }
    }
}

/**
 * This is called periodically to render the OLED image
 *
 * @param elapsedUs The time elapsed since the last time this function was
 *                  called. Use this value to determine when it's time to do
 *                  things
 */
void tunernomeMainLoop(int64_t elapsedUs)
{
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        tunernomeProcessButtons(&evt);
    }

    // Check for touch events
    int32_t phi, r, intensity;
    bool touched = getTouchJoystick(&phi, &r, &intensity);
    if (!tunernome->isTouched && touched)
    {
        switch(tunernome->mode)
        {
            case TN_TUNER:
                // On any tap, toggle string display
                tunernome->isShowingStrings = !tunernome->isShowingStrings;
                break;
            case TN_METRONOME:
                // On any tap, toggle silence
                tunernome->isSilent = !tunernome->isSilent;
                break;
        }
    }
    tunernome->isTouched = touched;

    switch (tunernome->mode)
    {
        default:
        case TN_TUNER:
        {
            // Current note/mode in middle of display
            switch (tunernome->curTunerMode)
            {
                case GUITAR_TUNER:
                {
                    if(tunernome->isShowingStrings)
                    {
                        plotInstrumentStrings(NUM_GUITAR_STRINGS);
                    }
                    plotInstrumentNameAndNotes(theWordGuitar, guitarNoteNames, NUM_GUITAR_STRINGS);
                    break;
                }
                case VIOLIN_TUNER:
                {
                    if(tunernome->isShowingStrings)
                    {
                        plotInstrumentStrings(NUM_VIOLIN_STRINGS);
                    }
                    plotInstrumentNameAndNotes(theWordViolin, violinNoteNames, NUM_VIOLIN_STRINGS);
                    break;
                }
                case UKULELE_TUNER:
                {
                    if(tunernome->isShowingStrings)
                    {
                        plotInstrumentStrings(NUM_UKULELE_STRINGS);
                    }
                    plotInstrumentNameAndNotes(theWordUkulele, ukuleleNoteNames, NUM_UKULELE_STRINGS);
                    break;
                }
                case BANJO_TUNER:
                {
                    if(tunernome->isShowingStrings)
                    {
                        plotInstrumentStrings(NUM_BANJO_STRINGS);
                    }
                    plotInstrumentNameAndNotes(theWordBanjo, banjoNoteNames, NUM_BANJO_STRINGS);
                    break;
                }
                case LISTENING:
                {
                    // Find the note that has the highest intensity. Must be larger than 100
                    int16_t maxIntensity = 100;
                    int8_t semitoneNum   = -1;
                    for (uint8_t semitone = 0; semitone < NUM_SEMITONES; semitone++)
                    {
                        if (tunernome->intensity[semitone] > maxIntensity)
                        {
                            maxIntensity = tunernome->intensity[semitone];
                            semitoneNum  = semitone;
                        }
                    }

                    drawText(&tunernome->radiostars, c555, listeningText,
                             (TFT_WIDTH - textWidth(&tunernome->radiostars, listeningText)) / 2,
                             TUNER_CENTER_Y - (TUNER_RADIUS + tunernome->radiostars.height) / 2);

                    led_t leds[CONFIG_NUM_LEDS] = {{0}};

                    // If some note is intense
                    if (-1 != semitoneNum)
                    {
                        // Plot text on top of everything else
                        bool shouldDrawFlat
                            = (semitoneNoteNames[semitoneNum][strlen(semitoneNoteNames[semitoneNum]) - 1] == 1);
                        char buf[5] = {0};
                        strncpy(buf, semitoneNoteNames[semitoneNum], 4);
                        int16_t tWidth = textWidth(&tunernome->logbook, buf);
                        if (shouldDrawFlat)
                        {
                            tWidth += tunernome->flatWsg.w + 1;
                        }
                        int16_t textEnd = drawText(&tunernome->logbook, c555, buf, (TFT_WIDTH - tWidth) / 2 + 1,
                                                   (TFT_HEIGHT - tunernome->logbook.height) / 2 - TUNER_TEXT_Y_OFFSET);

                        // Append the wsg for a flat
                        if (shouldDrawFlat)
                        {
                            drawWsg(&tunernome->flatWsg, textEnd,
                                    (TFT_HEIGHT - tunernome->logbook.height) / 2 - TUNER_TEXT_Y_OFFSET, false, false,
                                    0);
                        }

                        // Set the LEDs to a colorchord-like value
                        uint32_t toneColor = ECCtoHEX((semitoneNum * 256) / NUM_SEMITONES, 0xFF, 0x80);
                        for (uint8_t i = 0; i < CONFIG_NUM_LEDS; i++)
                        {
                            leds[i].r = (toneColor >> 0) & 0xFF;
                            leds[i].g = (toneColor >> 8) & 0xFF;
                            leds[i].b = (toneColor >> 16) & 0xFF;
                        }
                    }

                    // Set LEDs, this may turn them off
                    setLeds(leds, CONFIG_NUM_LEDS);
                    break;
                }
                case MAX_GUITAR_MODES:
                    break;
                case SEMITONE_0:
                case SEMITONE_1:
                case SEMITONE_2:
                case SEMITONE_3:
                case SEMITONE_4:
                case SEMITONE_5:
                case SEMITONE_6:
                case SEMITONE_7:
                case SEMITONE_8:
                case SEMITONE_9:
                case SEMITONE_10:
                case SEMITONE_11:
                default:
                {
                    // Draw tuner needle based on the value of tonalDiff, which is at most -32768 to 32767
                    // clamp it to the range -180 -> 180
                    int16_t clampedTonalDiff
                        = CLAMP(tunernome->tonalDiff[tunernome->curTunerMode - SEMITONE_0] / 2, -180, 180);

                    // If the signal isn't intense enough, don't move the needle
                    if (tunernome->semitone_intensity_filt[tunernome->curTunerMode - SEMITONE_0] < 1000)
                    {
                        clampedTonalDiff = -180;
                    }

                    // Find the end point of the unit-length needle
                    float intermedX = sinf(clampedTonalDiff * M_PI / 360.0f);
                    float intermedY = cosf(clampedTonalDiff * M_PI / 360.0f);

                    // Find the actual end point of the full-length needle
                    int x = round(METRONOME_CENTER_X + (intermedX * TUNER_RADIUS));
                    int y = round(TUNER_CENTER_Y - (intermedY * TUNER_RADIUS));

                    // Plot the needle
                    drawLine(METRONOME_CENTER_X, TUNER_CENTER_Y, x, y, c555, 0);
                    // Plot dashed lines indicating the 'in tune' range
                    drawLine(METRONOME_CENTER_X, TUNER_CENTER_Y, TUNER_FLAT_THRES_X, TUNER_THRES_Y, c555, 2);
                    drawLine(METRONOME_CENTER_X, TUNER_CENTER_Y, TUNER_SHARP_THRES_X, TUNER_THRES_Y, c555, 2);
                    // Plot a semicircle around it all
                    drawCircleQuadrants(METRONOME_CENTER_X, TUNER_CENTER_Y, TUNER_RADIUS, false, false, true, true,
                                        c555);

                    // Plot text on top of everything else
                    uint8_t semitoneNum = (tunernome->curTunerMode - SEMITONE_0);
                    bool shouldDrawFlat
                        = (semitoneNoteNames[semitoneNum][strlen(semitoneNoteNames[semitoneNum]) - 1] == 1);
                    char buf[5] = {0};
                    strncpy(buf, semitoneNoteNames[semitoneNum], 4);
                    int16_t tWidth = textWidth(&tunernome->logbook, buf);
                    if (shouldDrawFlat)
                    {
                        tWidth += tunernome->flatWsg.w + 1;
                    }
                    int16_t textEnd = drawText(&tunernome->logbook, c555, buf, (TFT_WIDTH - tWidth) / 2 + 1,
                                               (TFT_HEIGHT - tunernome->logbook.height) / 2 - TUNER_TEXT_Y_OFFSET);

                    // Append the wsg for a flat
                    if (shouldDrawFlat)
                    {
                        drawWsg(&tunernome->flatWsg, textEnd,
                                (TFT_HEIGHT - tunernome->logbook.height) / 2 - TUNER_TEXT_Y_OFFSET, false, false, 0);
                    }
                    break;
                }
            }

            // Instructions at top of display
            drawText(&tunernome->radiostars, c234, "Flat", CORNER_OFFSET, CORNER_OFFSET);
            drawText(&tunernome->radiostars, c555, inTuneStr,
                     (TFT_WIDTH - textWidth(&tunernome->radiostars, inTuneStr)) / 2, CORNER_OFFSET);
            drawText(&tunernome->radiostars, c511, sharpStr,
                     TFT_WIDTH - textWidth(&tunernome->radiostars, sharpStr) - CORNER_OFFSET, CORNER_OFFSET);

            // A/B/Start button functions at bottom of display
            int16_t afterText = drawText(&tunernome->ibm_vga8, c151, "A", CORNER_OFFSET,
                                         TFT_HEIGHT - tunernome->ibm_vga8.height - CORNER_OFFSET);
            afterText         = drawText(&tunernome->ibm_vga8, c555, "/", afterText,
                                         TFT_HEIGHT - tunernome->ibm_vga8.height - CORNER_OFFSET);
            afterText         = drawText(&tunernome->ibm_vga8, c511, "B", afterText,
                                         TFT_HEIGHT - tunernome->ibm_vga8.height - CORNER_OFFSET);
            afterText         = drawText(&tunernome->ibm_vga8, c555, leftStr, afterText,
                                         TFT_HEIGHT - tunernome->ibm_vga8.height - CORNER_OFFSET);

            char gainStr[16] = {0};
            snprintf(gainStr, sizeof(gainStr) - 1, "Gain: %d", getMicGainSetting());
            drawText(&tunernome->ibm_vga8, c555, gainStr, afterText,
                     TFT_HEIGHT - tunernome->ibm_vga8.height - CORNER_OFFSET);

            uint16_t widestRightStrMetronomeWidth
                = MAX(textWidth(&tunernome->ibm_vga8, rightStrMetronome1), textWidth(&tunernome->ibm_vga8, rightStrMetronome2));

            // Draw text to toggle string display
            drawText(&tunernome->ibm_vga8, c555, rightStrMetronome1, TFT_WIDTH - widestRightStrMetronomeWidth - CORNER_OFFSET,
                     TFT_HEIGHT - (2 * tunernome->ibm_vga8.height) - 2 - CORNER_OFFSET);

            // Draw text to toggle beeps
            drawText(&tunernome->ibm_vga8, c555, rightStrMetronome2, TFT_WIDTH - widestRightStrMetronomeWidth - CORNER_OFFSET,
                     TFT_HEIGHT - tunernome->ibm_vga8.height - CORNER_OFFSET);

            // Up/Down arrows in middle of display around current note/mode
            drawWsg(&(tunernome->radiostarsArrowWsg), (TFT_WIDTH - tunernome->radiostarsArrowWsg.w) / 2 + 1,
                    (TFT_HEIGHT - tunernome->logbook.height) / 2 - tunernome->radiostarsArrowWsg.h
                        - TUNER_ARROW_Y_OFFSET - TUNER_TEXT_Y_OFFSET,
                    false, false, 0);
            drawWsg(&(tunernome->radiostarsArrowWsg), (TFT_WIDTH - tunernome->radiostarsArrowWsg.w) / 2 + 1,
                    (TFT_HEIGHT + tunernome->logbook.height) / 2 + TUNER_ARROW_Y_OFFSET - TUNER_TEXT_Y_OFFSET, false,
                    true, 0);

            break;
        }
        case TN_METRONOME:
        {
            // BPM at top of display
            char bpmStr[16];
            sprintf(bpmStr, "%d BPM", tunernome->bpm);
            int16_t beforeText = (TFT_WIDTH - textWidth(&tunernome->logbook, bpmStr)) / 2;
            int16_t afterText  = drawText(&tunernome->logbook, c555, bpmStr, beforeText, 5);

            drawWsg(&(tunernome->logbookArrowWsg), beforeText - tunernome->logbookArrowWsg.w - 8, 5, false, false, 0);
            drawWsg(&(tunernome->logbookArrowWsg), afterText + 7, 5, false, true, 0);

            // Draw the current beat it's on
            float beatXSpacing = ((float)TFT_WIDTH / (float)tunernome->beatLength);
            uint8_t beatRadius = 12;
            if (tunernome->beatLength > 11)
            {
                beatRadius = 8;
            }
            for (uint8_t i = 0; i < tunernome->beatLength; i++)
            {
                // Am aware of the fact that the spacing is off when you add more circles.
                // Oh well.
                uint16_t beatX = (beatXSpacing * i) + (beatXSpacing / 2);
                if (tunernome->beatCtr == i)
                {
                    drawCircleFilled(beatX, 48, beatRadius, c555);
                }
                else
                {
                    drawCircle(beatX, 48, beatRadius, c555);
                }
            }

            // Draw the amount of beats
            char beatStr[16];
            sprintf(beatStr, "< Beats %d >", tunernome->beatLength);
            drawText(&tunernome->ibm_vga8, c555, beatStr, CORNER_OFFSET,
                     TFT_HEIGHT - tunernome->ibm_vga8.height - CORNER_OFFSET);

            uint16_t widestRightStrTunerWidth
                = MAX(textWidth(&tunernome->ibm_vga8, rightStrTuner1), textWidth(&tunernome->ibm_vga8, rightStrTuner2));

            // Draw text to toggle beeps
            drawText(&tunernome->ibm_vga8, c555, rightStrTuner1, TFT_WIDTH - widestRightStrTunerWidth - CORNER_OFFSET,
                     TFT_HEIGHT - (2 * tunernome->ibm_vga8.height) - 2 - CORNER_OFFSET);

            // Draw text to switch to tuner mode
            drawText(&tunernome->ibm_vga8, c555, rightStrTuner2, TFT_WIDTH - widestRightStrTunerWidth - CORNER_OFFSET,
                     TFT_HEIGHT - tunernome->ibm_vga8.height - CORNER_OFFSET);

            // Turn off LEDs after blink is over
            if (tunernome->blinkTimerUs > 0)
            {
                tunernome->blinkTimerUs -= elapsedUs;
                if (tunernome->blinkTimerUs <= 0)
                {
                    led_t leds[CONFIG_NUM_LEDS] = {{0}};
                    setLeds(leds, CONFIG_NUM_LEDS);
                }
            }

			// Metronome arm reversal logic and blink start logic
            bool shouldBlink = false;
            // If the arm is sweeping clockwise
            if (tunernome->isClockwise)
            {
                // Add to tAccumulatedUs
                tunernome->tAccumulatedUs += elapsedUs;
                // If it's crossed the threshold for one beat
                if (tunernome->tAccumulatedUs >= tunernome->usPerBeat)
                {
                    // Flip the metronome arm
                    tunernome->isClockwise = false;
                    // Start counting down by subtracting the excess time from tAccumulatedUs
                    tunernome->tAccumulatedUs
                        = tunernome->usPerBeat - (tunernome->tAccumulatedUs - tunernome->usPerBeat);
                    // Blink LED Tick color
                    shouldBlink = true;
                }
            }
            else
            {
                // Subtract from tAccumulatedUs
                tunernome->tAccumulatedUs -= elapsedUs;
                // If it's crossed the threshold for one beat
                if (tunernome->tAccumulatedUs <= 0)
                {
                    // Flip the metronome arm
                    tunernome->isClockwise = true;
                    // Start counting up by flipping the excess time from negative to positive
                    tunernome->tAccumulatedUs = -(tunernome->tAccumulatedUs);
                    // Blink LED Tock color
                    shouldBlink = true;
                }
            }

            // Count down a timer to turn the click off
            if (!tunernome->isSilent)
            {
                if (tunernome->clickTimerUs > 0)
                {
                    tunernome->clickTimerUs -= elapsedUs;
                    if (tunernome->clickTimerUs <= 0)
                    {
                        midiNoteOff(globalMidiPlayerGet(MIDI_BGM), 0, tunernome->midiNote, 0x7F);
                    }
                }
            }

            // Turn LEDs on at start of blink
            if (shouldBlink)
            {
                // Add one to the beat counter
                tunernome->beatCtr = (tunernome->beatCtr + 1) % tunernome->beatLength;

                // Start all LEDs as being off
                led_t leds[CONFIG_NUM_LEDS] = {{0}};

                if (0 == tunernome->beatCtr)
                {
                    tunernome->midiNote = 60; // C4
                    for (int i = 0; i < CONFIG_NUM_LEDS; i++)
                    {
                        leds[i].r = 0x40;
                        leds[i].g = 0xFF;
                        leds[i].b = 0x00;
                    }
                }
                else
                {
                    const uint8_t offBeatLeds[] = {1, 3, 5, 8};
                    tunernome->midiNote         = 69; // A4
                    for (int i = 0; i < ARRAY_SIZE(offBeatLeds); i++)
                    {
                        leds[offBeatLeds[i]].r = 0x40;
                        leds[offBeatLeds[i]].g = 0x00;
                        leds[offBeatLeds[i]].b = 0xFF;
                    }
                }

                if (!tunernome->isSilent)
                {
                    // Click  here
                    midiNoteOn(globalMidiPlayerGet(MIDI_BGM), 0, tunernome->midiNote, 0x7F);
                    tunernome->clickTimerUs = DEFAULT_FRAME_RATE_US * 3;
                }

                setLeds(leds, CONFIG_NUM_LEDS);
                tunernome->blinkTimerUs = DEFAULT_FRAME_RATE_US * 3;
            }

            // Runs after the up or down button is held long enough in metronome mode.
            // Repeats the button press while the button is held.
            if (tunernome->lastBpmButton != 0)
            {
                if (tunernome->bpmButtonAccumulatedUs == 0)
                {
                    tunernome->bpmButtonAccumulatedUs = esp_timer_get_time() - tunernome->bpmButtonStartUs;
                    tunernome->bpmButtonCurChangeUs   = tunernome->bpmButtonAccumulatedUs;
                }
                else
                {
                    tunernome->bpmButtonAccumulatedUs += elapsedUs;
                    tunernome->bpmButtonCurChangeUs += elapsedUs;
                }

                if (tunernome->bpmButtonAccumulatedUs >= BPM_CHANGE_FIRST_MS * 1000)
                {
                    if (tunernome->bpmButtonCurChangeUs >= BPM_CHANGE_REPEAT_MS * 1000)
                    {
                        int16_t mod = 1;
                        if (tunernome->bpmButtonAccumulatedUs >= BPM_CHANGE_FAST_MS * 1000)
                        {
                            mod = 3;
                        }
                        switch (tunernome->lastBpmButton)
                        {
                            case PB_UP:
                            {
                                modifyBpm(mod);
                                break;
                            }
                            case PB_DOWN:
                            {
                                modifyBpm(-mod);
                                break;
                            }
                            case PB_LEFT:
                            case PB_RIGHT:
                            case PB_A:
                            case PB_B:
                            case PB_START:
                            case PB_SELECT:
                            default:
                            {
                                break;
                            }
                        }

                        tunernome->bpmButtonCurChangeUs = 0;
                    }
                }
            }

            // Draw metronome top behind all other metronome parts
            drawWsg(&(tunernome->metronomeTopWsg), (TFT_WIDTH - tunernome->metronomeTopWsg.w) / 2, METRONOME_GRAPHIC_Y, false, false, 0);

            // Draw metronome arm based on the value of tAccumulatedUs, which is between (0, usPerBeat)
            float intermedX = cosf(METRONOME_ARC_MULTIPLIER * tunernome->tAccumulatedUs * M_PI / tunernome->usPerBeat + METRONOME_ARC_MULTIPLIER * 0.5 * M_PI);
            float intermedY = ABS(sinf(METRONOME_ARC_MULTIPLIER * tunernome->tAccumulatedUs * M_PI / tunernome->usPerBeat + METRONOME_ARC_MULTIPLIER * 0.5 * M_PI));
            int16_t armX    = round(METRONOME_CENTER_X - (intermedX * METRONOME_RADIUS));
            int16_t armY    = round(METRONOME_CENTER_Y - (intermedY * METRONOME_RADIUS));
            drawLine(METRONOME_CENTER_X, METRONOME_CENTER_Y, armX, armY, c445, 0);

            // Draw metronome balance weight
            float weightRadiusPercent = (MAX_BPM - tunernome->bpm) / ((float) (MAX_BPM - 1));
            float weightRadius = weightRadiusPercent * METRONOME_WEIGHT_RADIUS_RANGE + METRONOME_WEIGHT_MIN_RADIUS;
            int16_t weightX = round(METRONOME_CENTER_X - (intermedX * weightRadius));
            int16_t weightY = round(METRONOME_CENTER_Y - (intermedY * weightRadius));
            //printf("weightRadiusPercent = %f, x = %"PRIi16", y = %"PRIi16"\n", weightRadiusPercent, weightX, weightY);
            //drawLineScaled(METRONOME_CENTER_X, METRONOME_CENTER_Y, armX, armY, c445, 0, -METRONOME_CENTER_X, 0, 2, 1);
            drawCircleFilled(weightX, weightY, METRONOME_WEIGHT_SIZE_RADIUS, c222);
            
            // Draw metronome base in front of all other metronome parts
            drawWsg(&(tunernome->metronomeBottomWsg), (TFT_WIDTH - tunernome->metronomeBottomWsg.w) / 2, METRONOME_GRAPHIC_Y, false, false, 0);
            break;
        }
    }
}

/**
 * TODO
 *
 * @param evt The button event that occurred
 */
void tunernomeProcessButtons(buttonEvt_t* evt)
{
    switch (tunernome->mode)
    {
        default:
        case TN_TUNER:
        {
            if (evt->down)
            {
                switch (evt->button)
                {
                    case PB_UP:
                    {
                        tunernome->curTunerMode = (tunernome->curTunerMode + 1) % MAX_GUITAR_MODES;
                        break;
                    }
                    case PB_DOWN:
                    {
                        if (0 == tunernome->curTunerMode)
                        {
                            tunernome->curTunerMode = MAX_GUITAR_MODES - 1;
                        }
                        else
                        {
                            tunernome->curTunerMode--;
                        }
                        break;
                    }
                    case PB_A:
                    {
                        // Cycle microphone sensitivity
                        incMicGainSetting();
                        break;
                    }
                    case PB_B:
                    {
                        // Cycle microphone sensitivity
                        decMicGainSetting();
                        break;
                    }
                    case PB_START:
                    {
                        switchToSubmode(TN_METRONOME);
                        break;
                    }
                    // TODO: left/right to cycle common tunings per-instrument?
                    case PB_LEFT:
                    case PB_RIGHT:
                    case PB_SELECT:
                    default:
                    {
                        break;
                    }
                }
            }
            break;
        }
        case TN_METRONOME:
        {
            if (evt->down)
            {
                switch (evt->button)
                {
                    case PB_UP:
                    {
                        modifyBpm(1);
                        tunernome->lastBpmButton          = evt->button;
                        tunernome->bpmButtonStartUs       = esp_timer_get_time();
                        tunernome->bpmButtonCurChangeUs   = 0;
                        tunernome->bpmButtonAccumulatedUs = 0;
                        break;
                    }
                    case PB_DOWN:
                    {
                        modifyBpm(-1);
                        tunernome->lastBpmButton          = evt->button;
                        tunernome->bpmButtonStartUs       = esp_timer_get_time();
                        tunernome->bpmButtonCurChangeUs   = 0;
                        tunernome->bpmButtonAccumulatedUs = 0;
                        break;
                    }
                    case PB_LEFT:
                    {
                        if (tunernome->beatLength > 1)
                        {
                            tunernome->beatLength -= 1;
                        }
                        break;
                    }
                    case PB_RIGHT:
                    {
                        if (tunernome->beatLength < MAX_BEATS)
                        {
                            tunernome->beatLength += 1;
                        }
                        break;
                    }
                    case PB_START:
                    {
                        switchToSubmode(TN_TUNER);
                        break;
                    }
                    case PB_SELECT:
                    case PB_A:
                    case PB_B:
                    default:
                    {
                        break;
                    }
                }
            }
            else
            {
                // Stop button repetition and reset related variables
                switch (evt->button)
                {
                    case PB_UP:
                    case PB_DOWN:
                    {
                        if (evt->button == tunernome->lastBpmButton)
                        {
                            tunernome->lastBpmButton          = 0;
                            tunernome->bpmButtonStartUs       = 0;
                            tunernome->bpmButtonCurChangeUs   = 0;
                            tunernome->bpmButtonAccumulatedUs = 0;
                        }
                        break;
                    }
                    case PB_LEFT:
                    case PB_RIGHT:
                    case PB_A:
                    case PB_B:
                    case PB_START:
                    case PB_SELECT:
                    default:
                    {
                        break;
                    }
                }
            }
            break;
        }
    }
}

/**
 * Increases the bpm by bpmMod, may be negative
 *
 * @param bpmMod The amount to change the BPM
 */
void modifyBpm(int16_t bpmMod)
{
    tunernome->bpm = CLAMP(tunernome->bpm + bpmMod, 1, MAX_BPM);
    double ratio = tunernome->tAccumulatedUs / (double) tunernome->usPerBeat;
    recalcMetronome();
    tunernome->tAccumulatedUs = round(tunernome->usPerBeat * ratio);
}

/**
 * This function is called whenever audio samples are read from the
 * microphone (ADC) and are ready for processing. Samples are read at 8KHz.
 *
 * @param samples A pointer to 12 bit audio samples
 * @param sampleCnt The number of samples read
 */
void tunernomeSampleHandler(uint16_t* samples, uint32_t sampleCnt)
{
    if (tunernome->mode == TN_TUNER)
    {
        for (uint32_t i = 0; i < sampleCnt; i++)
        {
            PushSample32(&tunernome->dd, samples[i]);
        }
        tunernome->audioSamplesProcessed += sampleCnt;

        // If at least 128 samples have been processed
        if (tunernome->audioSamplesProcessed >= 128)
        {
            // Colorchord magic
            HandleFrameInfo(&tunernome->end, &tunernome->dd);

            led_t colors[CONFIG_NUM_LEDS] = {{0}};

            switch (tunernome->curTunerMode)
            {
                case GUITAR_TUNER:
                {
                    instrumentTunerMagic(freqBinIdxsGuitar, NUM_GUITAR_STRINGS, colors, sixNoteStringIdxToLedIdx);
                    break;
                }
                case VIOLIN_TUNER:
                {
                    instrumentTunerMagic(freqBinIdxsViolin, NUM_VIOLIN_STRINGS, colors, fourNoteStringIdxToLedIdx);
                    break;
                }
                case UKULELE_TUNER:
                {
                    instrumentTunerMagic(freqBinIdxsUkulele, NUM_UKULELE_STRINGS, colors, fourNoteStringIdxToLedIdx);
                    break;
                }
                case BANJO_TUNER:
                {
                    instrumentTunerMagic(freqBinIdxsBanjo, NUM_BANJO_STRINGS, colors, fiveNoteStringIdxToLedIdx);
                    break;
                }
                case MAX_GUITAR_MODES:
                    break;
                case SEMITONE_0:
                case SEMITONE_1:
                case SEMITONE_2:
                case SEMITONE_3:
                case SEMITONE_4:
                case SEMITONE_5:
                case SEMITONE_6:
                case SEMITONE_7:
                case SEMITONE_8:
                case SEMITONE_9:
                case SEMITONE_10:
                case SEMITONE_11:
                case LISTENING:
                default:
                {
                    for (uint8_t semitone = 0; semitone < NUM_SEMITONES; semitone++)
                    {
                        // uint8_t semitoneIdx = (tunernome->curTunerMode - SEMITONE_0) * 2;
                        uint8_t semitoneIdx = semitone * 2;
                        // Pick out the current magnitude and filter it
                        tunernome->semitone_intensity_filt[semitone]
                            = (getSemiMagnitude(semitoneIdx + CHROMATIC_OFFSET)
                               + tunernome->semitone_intensity_filt[semitone])
                              - (tunernome->semitone_intensity_filt[semitone] >> 5);

                        // Pick out the difference around current magnitude and filter it too
                        tunernome->semitone_diff_filt[semitone] = (getSemiDiffAround(semitoneIdx + CHROMATIC_OFFSET)
                                                                   + tunernome->semitone_diff_filt[semitone])
                                                                  - (tunernome->semitone_diff_filt[semitone] >> 5);

                        // This is the magnitude of the target frequency bin, cleaned up
                        tunernome->intensity[semitone]
                            = (tunernome->semitone_intensity_filt[semitone] >> SENSITIVITY) - 40; // drop a baseline.
                        tunernome->intensity[semitone] = CLAMP(tunernome->intensity[semitone], 0, 255);

                        // This is the tonal difference. You "calibrate" out the intensity.
                        tunernome->tonalDiff[semitone] = (tunernome->semitone_diff_filt[semitone] >> SENSITIVITY) * 200
                                                         / (tunernome->intensity[semitone] + 1);
                    }

                    if (tunernome->curTunerMode < LISTENING)
                    {
                        // tonal diff is -32768 to 32767. if its within -10 to 10 (now defined as
                        // TONAL_DIFF_IN_TUNE_DEVIATION), it's in tune. positive means too sharp, negative means too
                        // flat intensity is how 'loud' that frequency is, 0 to 255. you'll have to play around with
                        // values
                        int32_t red, grn, blu;
                        // Is the note in tune, i.e. is the magnitude difference in surrounding bins small?
                        if ((ABS(tunernome->tonalDiff[tunernome->curTunerMode - SEMITONE_0])
                             < TONAL_DIFF_IN_TUNE_DEVIATION))
                        {
                            // Note is in tune, make it white
                            red = 255;
                            grn = 255;
                            blu = 255;
                        }
                        else
                        {
                            // Check if the note is sharp or flat
                            if (tunernome->tonalDiff[tunernome->curTunerMode - SEMITONE_0] > 0)
                            {
                                // Note too sharp, make it red
                                red = 255;
                                grn = blu = 255
                                            - (tunernome->tonalDiff[tunernome->curTunerMode - SEMITONE_0]
                                               - TONAL_DIFF_IN_TUNE_DEVIATION)
                                                  * 15;
                            }
                            else
                            {
                                // Note too flat, make it blue
                                blu = 255;
                                grn = red = 255
                                            - (-(tunernome->tonalDiff[tunernome->curTunerMode - SEMITONE_0]
                                                 + TONAL_DIFF_IN_TUNE_DEVIATION))
                                                  * 15;
                            }

                            // Make sure LED output isn't more than 255
                            red = CLAMP(red, INT_MIN, 255);
                            grn = CLAMP(grn, INT_MIN, 255);
                            blu = CLAMP(blu, INT_MIN, 255);
                        }

                        // Scale each LED's brightness by the filtered intensity for that bin
                        red = (red >> 3) * (tunernome->intensity[tunernome->curTunerMode - SEMITONE_0] >> 3);
                        grn = (grn >> 3) * (tunernome->intensity[tunernome->curTunerMode - SEMITONE_0] >> 3);
                        blu = (blu >> 3) * (tunernome->intensity[tunernome->curTunerMode - SEMITONE_0] >> 3);

                        // Set the LED, ensure each channel is between 0 and 255
                        uint32_t i;
                        for (i = 0; i < CONFIG_NUM_LEDS; i++)
                        {
                            colors[i].r = CLAMP(red, 0, 255);
                            colors[i].g = CLAMP(grn, 0, 255);
                            colors[i].b = CLAMP(blu, 0, 255);
                        }
                    }

                    break;
                }
            }

            if (LISTENING != tunernome->curTunerMode)
            {
                // Draw the LEDs
                setLeds(colors, CONFIG_NUM_LEDS);
            }
            // Reset the sample count
            tunernome->audioSamplesProcessed = 0;
        }
    }
}

/**
 * @brief Draw a portion of the background when requested
 *
 * @param x The X offset to draw
 * @param y The Y offset to draw
 * @param w The width to draw
 * @param h The height to draw
 * @param up The current number of the update call
 * @param upNum The total number of update calls for this frame
 */
void tunernomeBackgroundDrawCb(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
    paletteColor_t* const src = tunernome->backgroundWsg.px;
    paletteColor_t* dst = getPxTftFramebuffer();
    for(int16_t row = y; row < y + h; row++)
    {
        memcpy(&dst[row * TFT_WIDTH + x], &src[row * TFT_WIDTH + x], w);
    }
}
