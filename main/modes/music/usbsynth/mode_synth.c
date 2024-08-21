#include <stddef.h>
#include <stdlib.h>

#include "esp_log.h"
#include "esp_timer.h"

#include "swadge2024.h"
#include "fs_font.h"
#include "hdw-btn.h"
#include "touchUtils.h"
#include "shapes.h"
#include "trigonometry.h"
#include "linked_list.h"
#include "wheel_menu.h"
#include "hdw-nvs.h"
#include "textEntry.h"
#include "macros.h"
#include "cnfs_image.h"
#include "ctype.h"

#include "midiPlayer.h"
#include "midiFileParser.h"
#include "midiUsb.h"
#include "hashMap.h"
#include "midiData.h"

//==============================================================================
// Defines
//==============================================================================

#define NUM_FRAME_TIMES 60

#define VIZ_SAMPLE_COUNT 512

#define TEMPO_TO_BPM(t) (60000000 / (t))
// math is wild yo
#define BPM_TO_TEMPO(b) TEMPO_TO_BPM(b)

#define SYNTH_BG_COLOR c112

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    SS_VIEW = 0,
    SS_MENU,
    SS_FILE_SELECT,
} synthScreen_t;

typedef enum
{
    /// @brief D-pad selects note/channel and A/B play/stop
    BM_NOTE,
    /// @brief D-pad LR skips, UD changes tempo, A plays/pauses, B stops
    BM_PLAYBACK,
} synthButtonMode_t;

typedef enum
{
    /// @brief Touchpad control wheel menu
    TM_MENU,
    /// @brief Touchpad acts as pitch bend wheel
    TM_PITCH,
    /// @brief Touchpad scrubs through a song
    TM_SCRUB,
} synthTouchMode_t;

typedef enum
{
    VM_PRETTY  = 0,
    VM_TEXT    = 1,
    VM_GRAPH   = 2,
    VM_PACKETS = 4,
    VM_VIZ     = 8,
    VM_LYRICS  = 16,
    VM_TIMING  = 32,
} synthViewMode_t;

typedef enum
{
    MI_PLAY,
    MI_PAUSE,
    MI_STOP,
    MI_PLAYPAUSE,
    MI_FFW,
    MI_REW,
    MI_SKIP,
    MI_PREV,
    MI_REPEAT,
    MI_SHUFFLE,
} musicIcon_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    union
    {
        struct
        {
            const char* text;
            uint32_t length;
            uint32_t tempo;
        };
    };

    metaEventType_t type;
    union
    {
        uint64_t expiration;
        uint64_t timestamp;
    };
} midiTextInfo_t;

typedef struct
{
    /// @brief True if the MIDI file's text is in .KAR format
    bool karFormat;

    /// @brief Preloaded list of lyrics, in order
    list_t lyrics;

    /// @brief The file's tempo (TODO it can change)
    uint32_t tempo;

    /// @brief Number of MIDI ticks in a single note (lyric)
    uint32_t noteLength;

    /// @brief The length, in MIDI ticks, of one measure
    uint32_t measureLength;

    midiTimeSignature_t timeSignature;
} karaokeInfo_t;

typedef struct
{
    uint32_t state;
    uint32_t range;
    bool reversed;
} lfsrState_t;

typedef struct
{
    /// @brief MIDI control number
    uint8_t control;

    enum {
        /// @brief On/off controller
        CTRL_SWITCH,
        /// @brief Coarse control value, 7 high bits of 14-bit value
        CTRL_CC_MSB,
        /// @brief Fine control value, 7 low bits of 14-bit value
        CTRL_CC_LSB,
        /// @brief Control value with only a single 7-bit value, no MSB/LSB
        CTRL_7BIT,
        /// @brief Control with no actual data, only a message
        CTRL_NO_DATA,
        /// @brief Not a defined MIDI controller, reserved for future use
        CTRL_UNDEFINED,
    } type;

    const char* desc;
} midiControllerDesc_t;

typedef struct
{
    enum {
        SMT_PROGRAM,
        SMT_CHANNEL,
        SMT_CONTROLLER,
    } type;

    union {
        struct {
            uint8_t bank;
            uint8_t program;
        };
        uint8_t channel;
        const midiControllerDesc_t* controller;
    };

    const char* label;
    bool dynamicLabel;
    char shortLabel[4];

} midiMenuItemInfo_t;

/**
 * @brief For storing all synth config data in NVS
 */
typedef struct
{
    /// @brief Bitmask of which channels are being ignored
    uint16_t ignoreChannelMask;

    /// @brief Bitmask of which channels are in percussion mode
    uint16_t percChannelMask;

    /// @brief Array of program ID for each channel
    uint8_t programs[16];

    /// @brief Array of selected bank for each channel
    uint16_t banks[16];

    /// @brief The number of set controls in any channel
    uint8_t controlCounts;
} synthConfig_t;

typedef struct
{
    uint8_t control;
    uint16_t chanMask;
    uint8_t chanValues[16];
} synthControlConfig_t;

typedef struct
{
    font_t font;
    font_t betterFont;
    font_t betterOutline;

    bool pluggedIn;
    bool installed;
    bool sustain;
    bool noteSus;
    int err;
    uint8_t programs[16];
    bool perc[16];
    bool playing[16];
    uint8_t lastPackets[16][4];

    midiFile_t midiFile;
    midiPlayer_t midiPlayer;
    bool fileMode;
    const char* filename;
    char* filenameBuf;
    bool customFile;

    bool localPitch;
    uint16_t pitch;

    bool startupSeqComplete;
    int64_t noteTime;
    uint8_t startupNote;
    bool startupDrums;
    bool startSilence;
    const char* longestProgramName;
    uint8_t lastSamples[VIZ_SAMPLE_COUNT];
    int16_t sampleCount;
    int16_t graphOffset;
    uint8_t graphColor;
    uint8_t localChannel;

    // Hold timers
    struct
    {
        bool upHeld;
        bool downHeld;
        bool leftHeld;
        bool rightHeld;

        int64_t upHeldTimer;
        int64_t downHeldTimer;
        int64_t leftHeldTimer;
        int64_t rightHeldTimer;

        uint16_t lastButtonState;
    };

    synthScreen_t screen;
    synthViewMode_t viewMode;
    synthButtonMode_t buttonMode;
    synthTouchMode_t touchMode;
    bool loop;
    bool stopped;
    bool shuffle;
    bool autoplay;
    lfsrState_t shuffleState;
    int32_t shufflePos;
    int32_t headroom;

    wsg_t instrumentImages[16];
    wsg_t percussionImage;
    wsg_t magfestBankImage;

    wsg_t pauseIcon, playIcon, playPauseIcon, ffwIcon, skipIcon, loopIcon, shuffleIcon, stopIcon;

    // Tool wheel items
    wsg_t fileImage, playerImage, channelSetupImage, uiImage, volumeImage,
          buttonImage, touchImage, viewModeImage, usbModeImage, menuImage,
          pitchImage, resetImage, ignoreImage, enableImage;

    uint32_t frameTimesIdx;
    uint64_t frameTimes[NUM_FRAME_TIMES];

    list_t midiTexts;
    uint64_t nextExpiry;

    karaokeInfo_t karaoke;

    list_t customFiles;

    menu_t* menu;
    menuManiaRenderer_t* renderer;
    wheelMenuRenderer_t* wheelMenu;
    rectangle_t wheelTextArea;
    bool updateMenu;
    bool forceResetMenu;

    // Custom persistent labels for all the channel instrument submenus
    char channelInstrumentLabels[16][64];
    uint8_t menuSelectedChannel;

    hashMap_t menuMap;
    // 2 Banks
    // 128 + 6 (16) Programs
    // 16 Channels
    // <= 32 Controllers
    midiMenuItemInfo_t itemInfos[2 + 128 + 16 + 16 + 32];
    int itemInfoCount;

    // TODO can't use ARRAY_SIZE(menuItemViewOptions) here since it's not yet defined
    // So just set it to 16 and if we add 6 more views we'll need to increase it
    char menuViewShortnames[16][4];

    synthConfig_t synthConfig;
    list_t controllerSettings;

    int64_t marqueeTimer;
} synthData_t;

//==============================================================================
// Function Prototypes
//==============================================================================

static void synthEnterMode(void);
static void synthExitMode(void);
static void synthMainLoop(int64_t elapsedUs);
static void synthDacCallback(uint8_t* samples, int16_t len);

static void synthSetupPlayer(void);
static void synthApplyConfig(void);
static void synthSaveControl(uint8_t channel, uint8_t control, uint8_t value);
static uint8_t synthGetControl(uint8_t channel, uint8_t control, uint8_t defaultValue);
static uint16_t synthGetControl14bit(uint8_t channel, uint8_t control, uint16_t defValue);
static void addChannelsMenu(menu_t* menu, const synthConfig_t* config);
static void synthSetupMenu(bool forceReset);
static void preloadLyrics(karaokeInfo_t* karInfo, const midiFile_t* midiFile);
static void unloadLyrics(karaokeInfo_t* karInfo);
static void synthSetFile(const char* filename);
static void synthHandleButton(const buttonEvt_t evt);
static void handleButtonTimer(int64_t* timer, int64_t interval, int64_t elapsedUs, buttonBit_t button);
static void synthHandleInput(int64_t elapsedUs);
static bool synthIsControlSupported(const midiControllerDesc_t* control);
static void writeShortName(char* out, size_t n, const char* in);

static void drawCircleSweep(int x, int y, int r, int startAngle, int sweepDeg, paletteColor_t col);
static void drawIcon(musicIcon_t icon, paletteColor_t col, int16_t x, int16_t y, int16_t w, int16_t h);
static void drawSynthMode(int64_t elapsedUs);
static void drawBeatsMetronome(bool beats, int16_t beatsY, bool metronome, int16_t metX, int16_t metY, int16_t metR);
static void drawMidiText(bool filter, uint32_t types);
static void drawPitchWheelRect(uint8_t chIdx, int16_t x, int16_t y, int16_t w, int16_t h);
static void drawChannelInfo(const midiPlayer_t* player, uint8_t chIdx, int16_t x, int16_t y, int16_t width,
                            int16_t height);
static void drawSampleGraph(void);
static void drawSampleGraphCircular(void);
static void drawKaraokeLyrics(uint32_t ticks, karaokeInfo_t* karInfo);
static int writeMidiText(char* dest, size_t n, midiTextInfo_t* text, bool kar);
static void midiTextCallback(metaEventType_t type, const char* text, uint32_t length);
static void synthMenuCb(const char* label, bool selected, uint32_t value);
static void songEndCb(void);
static void setupShuffle(int numSongs);
static void prevSong(void);
static void nextSong(void);

static uint32_t setupLfsr(lfsrState_t* state, uint32_t range);
static void loadLfsr(lfsrState_t* state);
static void saveLfsr(const lfsrState_t* state);
static uint32_t flipToMsb(uint32_t val);
static uint32_t bitRevert(uint32_t v, uint8_t bits);
static uint32_t lfsrNext(lfsrState_t* state);
static uint32_t lfsrPrev(lfsrState_t* state);
static uint32_t lfsrCur(const lfsrState_t* state);

//==============================================================================
// Variabes
//==============================================================================

static const char readyStr[] = "Ready!";

static const char* gmProgramNames[] = {
    // Piano:
    "Acoustic Grand Piano",
    "Bright Acoustic Piano",
    "Electric Grand Piano",
    "Honky-tonk Piano",
    "Electric Piano 1",
    "Electric Piano 2",
    "Harpsichord",
    "Clavinet",

    // Chromatic Percussion:
    "Celesta",
    "Glockenspiel",
    "Music Box",
    "Vibraphone",
    "Marimba",
    "Xylophone",
    "Tubular Bells",
    "Dulcimer",

    // Organ:
    "Drawbar Organ",
    "Percussive Organ",
    "Rock Organ",
    "Church Organ",
    "Reed Organ",
    "Accordion",
    "Harmonica",
    "Tango Accordion",

    // Guitar:
    "Acoustic Guitar (nylon)",
    "Acoustic Guitar (steel)",
    "Electric Guitar (jazz)",
    "Electric Guitar (clean)",
    "Electric Guitar (muted)",
    "Overdriven Guitar",
    "Distortion Guitar",
    "Guitar harmonics",

    // Bass:
    "Acoustic Bass",
    "Electric Bass (finger)",
    "Electric Bass (pick)",
    "Fretless Bass",
    "Slap Bass 1",
    "Slap Bass 2",
    "Synth Bass 1",
    "Synth Bass 2",

    // Strings:
    "Violin",
    "Viola",
    "Cello",
    "Contrabass",
    "Tremolo Strings",
    "Pizzicato Strings",
    "Orchestral Harp",
    "Timpani",

    // Strings (continued):
    "String Ensemble 1",
    "String Ensemble 2",
    "Synth Strings 1",
    "Synth Strings 2",
    "Choir Aahs",
    "Voice Oohs",
    "Synth Voice",
    "Orchestra Hit",

    // Brass:
    "Trumpet",
    "Trombone",
    "Tuba",
    "Muted Trumpet",
    "French Horn",
    "Brass Section",
    "Synth Brass 1",
    "Synth Brass 2",

    // Reed:
    "Soprano Sax",
    "Alto Sax",
    "Tenor Sax",
    "Baritone Sax",
    "Oboe",
    "English Horn",
    "Bassoon",
    "Clarinet",

    // Pipe:
    "Piccolo",
    "Flute",
    "Recorder",
    "Pan Flute",
    "Blown Bottle",
    "Shakuhachi",
    "Whistle",
    "Ocarina",

    // Synth Lead:
    "Lead 1 (square)",
    "Lead 2 (sawtooth)",
    "Lead 3 (calliope)",
    "Lead 4 (chiff)",
    "Lead 5 (charang)",
    "Lead 6 (voice)",
    "Lead 7 (fifths)",
    "Lead 8 (bass + lead)",

    // Synth Pad:
    "Pad 1 (new age)",
    "Pad 2 (warm)",
    "Pad 3 (polysynth)",
    "Pad 4 (choir)",
    "Pad 5 (bowed)",
    "Pad 6 (metallic)",
    "Pad 7 (halo)",
    "Pad 8 (sweep)",

    // Synth Effects:
    "FX 1 (rain)",
    "FX 2 (soundtrack)",
    "FX 3 (crystal)",
    "FX 4 (atmosphere)",
    "FX 5 (brightness)",
    "FX 6 (goblins)",
    "FX 7 (echoes)",
    "FX 8 (sci-fi)",

    // Ethnic:
    "Sitar",
    "Banjo",
    "Shamisen",
    "Koto",
    "Kalimba",
    "Bag pipe",
    "Fiddle",
    "Shanai",

    // Percussive:
    "Tinkle Bell",
    "Agogo",
    "Steel Drums",
    "Woodblock",
    "Taiko Drum",
    "Melodic Tom",
    "Synth Drum",

    // Sound effects:
    "Reverse Cymbal",
    "Guitar Fret Noise",
    "Breath Noise",
    "Seashore",
    "Bird Tweet",
    "Telephone Ring",
    "Helicopter",
    "Applause",
    "Gunshot",
};

static const char* gmProgramCategoryNames[] = {
    "Piano",
    "Chromatic Percussion",
    "Organ",
    "Guitar",
    "Bass",
    "Solo Strings",
    "String Ensemble",
    "Brass",
    "Reed",
    "Pipe",
    "Synth Lead",
    "Synth Pad",
    "Synth Effects",
    "Ethnic",
    "Percussive",
    "Sound Effects",
};

static const char* gmDrumNames[] = {
    "Acoustic/Low Bass",
    "Electric/High Bass",
    "Side Stick",
    "Acoustic Snare",
    "Hand Clap",
    "Elec. Snare/Rimshot",
    "Low Floor Tom",
    "Closed Hi-Hat",
    "High Floor Tom",
    "Pedal Hi-Hat",
    "Low Tom",
    "Open Hi Hat",
    "Low Mid Tom",
    "High Mid Tom",
    "Crash Cymbal 1",
    "High Tom",
    "Ride Cymbal 1",
    "Chinese Cymbal",
    "Ride Bell",
    "Tambourine",
    "Splash Cymbal",
    "Cowbell",
    "Crash Cymbal 2",
    "Vibraslap",
    "Ride Cymbal 2",
    "High Bongo",
    "Low Bongo",
    "Mute High Conga",
    "Open High Conga",
    "Low Conga",
    "High Timbale",
    "Low Timbale",
    "High Agogo",
    "Low Agogo",
    "Cabasa",
    "Maracas",
    "Short Whistle",
    "Long Whistle",
    "Short Guiro",
    "Long Guiro",
    "Claves",
    "High Woodblock",
    "Low Woodblock",
    "Mute Cuica",
    "Open Cuica",
    "Mute Triangle",
    "Open Triangle",
};

static const char* bankNames[] = {
    "General MIDI",
    "MAGFest",
};

static const char* channelLabels[] = {
    "Channel 1",
    "Channel 2",
    "Channel 3",
    "Channel 4",
    "Channel 5",
    "Channel 6",
    "Channel 7",
    "Channel 8",
    "Channel 9",
    "Channel 10",
    "Channel 11",
    "Channel 12",
    "Channel 13",
    "Channel 14",
    "Channel 15",
    "Channel 16",
};

static const char* shortChannelLabels[] = {
    "1",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",
    "8",
    "9",
    "10",
    "11",
    "12",
    "13",
    "14",
    "15",
    "16",
};

static const midiControllerDesc_t controllerDefs[] = {
    {
        .control = 0,
        .type = CTRL_CC_MSB,
        .desc = "Bank Select",
    },
    {
        .control = 1,
        .type = CTRL_CC_MSB,
        .desc = "Modulation Wheel",
    },
    {
        .control = 2,
        .type = CTRL_CC_MSB,
        .desc = "Breath Controller",
    },
    // No 3
    {
        .control = 4,
        .type = CTRL_CC_MSB,
        .desc = "Foot Pedal",
    },
    {
        .control = 5,
        .type = CTRL_CC_MSB,
        .desc = "Portamento Time",
    },
    {
        .control = 6,
        .type = CTRL_CC_MSB,
        .desc = "Data Entry",
    },
    {
        .control = 7,
        .type = CTRL_CC_MSB,
        .desc = "Volume",
    },
    {
        .control = 8,
        .type = CTRL_CC_MSB,
        .desc = "Balance",
    },
    // No 9
    {
        .control = 10,
        .type = CTRL_CC_MSB,
        .desc = "Pan position",
    },
    {
        .control = 11,
        .type = CTRL_CC_MSB,
        .desc = "Expression",
    },
    {
        .control = 12,
        .type = CTRL_CC_MSB,
        .desc = "Effect 1",
    },
    {
        .control = 13,
        .type = CTRL_CC_MSB,
        .desc = "Effect 2",
    },
    // No 14-15
    {
        .control = 16,
        .type = CTRL_7BIT,
        .desc = "General Purpose Slider 1",
    },
    {
        .control = 17,
        .type = CTRL_7BIT,
        .desc = "General Purpose Slider 2",
    },
    {
        .control = 18,
        .type = CTRL_7BIT,
        .desc = "General Purpose Slider 3",
    },
    {
        .control = 19,
        .type = CTRL_7BIT,
        .desc = "General Purpose Slider 4",
    },
    // No 20-31
    {
        .control = 32,
        .type = CTRL_CC_LSB,
        .desc = "Bank Select",
    },
    {
        .control = 33,
        .type = CTRL_CC_LSB,
        .desc = "Modulation Wheel",
    },
    {
        .control = 34,
        .type = CTRL_CC_LSB,
        .desc = "Breath Controller",
    },
    // No 35
    {
        .control = 36,
        .type = CTRL_CC_LSB,
        .desc = "Foot Pedal",
    },
    {
        .control = 37,
        .type = CTRL_CC_LSB,
        .desc = "Portamento Time",
    },
    {
        .control = 38,
        .type = CTRL_CC_LSB,
        .desc = "Data Entry",
    },
    {
        .control = 39,
        .type = CTRL_CC_LSB,
        .desc = "Volume",
    },
    {
        .control = 40,
        .type = CTRL_CC_LSB,
        .desc = "Balance",
    },
    // No 41
    {
        .control = 42,
        .type = CTRL_CC_LSB,
        .desc = "Pan Position",
    },
    {
        .control = 43,
        .type = CTRL_CC_LSB,
        .desc = "Expression",
    },
    {
        .control = 44,
        .type = CTRL_CC_LSB,
        .desc = "Effect 1",
    },
    {
        .control = 45,
        .type = CTRL_CC_LSB,
        .desc = "Effect 2",
    },
    // No 46-63
    {
        .control = 64,
        .type = CTRL_SWITCH,
        .desc = "Hold Pedal ",
    },
    {
        .control = 65,
        .type = CTRL_SWITCH,
        .desc = "Portamento ",
    },
    {
        .control = 66,
        .type = CTRL_SWITCH,
        .desc = "Sustenuto Pedal ",
    },
    {
        .control = 67,
        .type = CTRL_SWITCH,
        .desc = "Soft Pedal ",
    },
    {
        .control = 68,
        .type = CTRL_SWITCH,
        .desc = "Legato Pedal ",
    },
    {
        .control = 69,
        .type = CTRL_SWITCH,
        .desc = "Hold 2 Pedal ",
    },
    {
        .control = 70,
        .type = CTRL_7BIT,
        .desc = "Sound Variation",
    },
    {
        .control = 71,
        .type = CTRL_7BIT,
        .desc = "Sound Timbre",
    },
    {
        .control = 72,
        .type = CTRL_7BIT,
        .desc = "Sound Release Time",
    },
    {
        .control = 73,
        .type = CTRL_7BIT,
        .desc = "Sound Attack Time",
    },
    {
        .control = 74,
        .type = CTRL_7BIT,
        .desc = "Sound Brightness",
    },
    {
        .control = 75,
        .type = CTRL_7BIT,
        .desc = "Sound Control 6",
    },
    {
        .control = 76,
        .type = CTRL_7BIT,
        .desc = "Sound Control 7",
    },
    {
        .control = 77,
        .type = CTRL_7BIT,
        .desc = "Sound Control 8",
    },
    {
        .control = 78,
        .type = CTRL_7BIT,
        .desc = "Sound Control 9",
    },
    {
        .control = 79,
        .type = CTRL_7BIT,
        .desc = "Sound Control 10",
    },
    {
        .control = 80,
        .type = CTRL_SWITCH,
        .desc = "General Purpose Button 1",
    },
    {
        .control = 81,
        .type = CTRL_SWITCH,
        .desc = "General Purpose Button 2 ",
    },
    {
        .control = 82,
        .type = CTRL_SWITCH,
        .desc = "General Purpose Button 3 ",
    },
    {
        .control = 83,
        .type = CTRL_SWITCH,
        .desc = "General Purpose Button 4 ",
    },
    // No 84-90
    {
        .control = 91,
        .type = CTRL_7BIT,
        .desc = "Effects Level",
    },
    {
        .control = 92,
        .type = CTRL_7BIT,
        .desc = "Tremolo Level",
    },
    {
        .control = 93,
        .type = CTRL_7BIT,
        .desc = "Chorus Level",
    },
    {
        .control = 94,
        .type = CTRL_7BIT,
        .desc = "Detune Level",
    },
    {
        .control = 95,
        .type = CTRL_7BIT,
        .desc = "Phaser Level",
    },
    {
        .control = 96,
        .type = CTRL_NO_DATA,
        .desc = "Data Button Increment",
    },
    {
        .control = 97,
        .type = CTRL_NO_DATA,
        .desc = "Data Button Decrement",
    },
    {
        .control = 98,
        .type = CTRL_CC_LSB,
        .desc = "Non-registered Parameter",
    },
    {
        .control = 99,
        .type = CTRL_CC_MSB,
        .desc = "Non-registered Parameter",
    },
    {
        .control = 100,
        .type = CTRL_CC_LSB,
        .desc = "Registered Parameter",
    },
    {
        .control = 101,
        .type = CTRL_CC_MSB,
        .desc = "Registered Parameter",
    },
    // No 102-119
    {
        .control = 120,
        .type = CTRL_NO_DATA,
        .desc = "All Sound Off",
    },
    {
        .control = 121,
        .type = CTRL_NO_DATA,
        .desc = "All Controllers Off",
    },
    {
        .control = 122,
        .type = CTRL_SWITCH,
        .desc = "Local Keyboard ",
    },
    {
        .control = 123,
        .type = CTRL_NO_DATA,
        .desc = "All Notes Off",
    },
    {
        .control = 124,
        .type = CTRL_NO_DATA,
        .desc = "Omni Mode Off",
    },
    {
        .control = 125,
        .type = CTRL_NO_DATA,
        .desc = "Omni Mode On",
    },
    {
        .control = 126,
        .type = CTRL_NO_DATA,
        .desc = "Monophonic Operation",
    },
    {
        .control = 127,
        .type = CTRL_NO_DATA,
        .desc = "Polyphonic Operation",
    },
};

static const paletteColor_t noteColors[] = {
    c500, // C  - Red
    c530, // C# - Orange
    c550, // D  - Yellow
    c350, // D# - Lime Green
    c050, // E  - Green
    c053, // F  - Aqua
    c055, // F# - Cyan
    c035, // G  - "Indigo"
    c005, // G# - Blue
    c305, // A  - Violet
    c505, // A# - Fuchsia
    c503, // B  - Pink
};

static const paletteColor_t textColors[] = {
    c500, // C  - Red
    c530, // C# - Orange
    c550, // D  - Yellow
    c350, // D# - Lime Green
    c050, // E  - Green
    c053, // F  - Aqua
    c055, // F# - Cyan
    c035, // G  - "Indigo"
    c005, // G# - Blue
    c305, // A  - Violet
    c505, // A# - Fuchsia
    c503, // B  - Pink
    c500, // C  - Red
    c530, // C# - Orange
    c550, // D  - Yellow
    c350, // D# - Lime Green
    c050, // E  - Green
    c053, // F  - Aqua
    c055, // F# - Cyan
    c035, // G  - "Indigo"
    c005, // G# - Blue
    c305, // A  - Violet
    c505, // A# - Fuchsia
    c503, // B  - Pink
};

// Menu stuff
static const char* menuItemPlayMode   = "Mode: ";

static const char* menuItemPlayer     = "Player";
static const char* menuItemSelectFile = "Select File...";
static const char* menuItemCustomSong = "Enter Filename...";
static const char* menuItemLoop       = "Loop: ";
static const char* menuItemShuffle    = "Shuffle: ";
static const char* menuItemAutoplay   = "Auto-play: ";
static const char* menuItemHeadroom   = "Mix Volume: ";

static const char* menuItemUi         = "Interface";
static const char* menuItemViewMode   = "View Mode: ";
static const char* menuItemButtonMode = "Button Controls: ";
static const char* menuItemTouchMode  = "Touch Controls: ";

static const char* menuItemChannels   = "Channel Setup";
static const char* menuItemIgnore     = "Enabled: ";
static const char* menuItemBank       = "Bank Select: ";
static const char* menuItemInstrument = "Instrument: ";
static const char* menuItemResetAll   = "Reset All Channels";
static const char* menuItemReset      = "Reset";

static const char* menuItemControls   = "Controllers";
static const char* menuItemPercussion = "Percussion: ";

static const char* const nvsKeyMode       = "synth_playmode";
static const char* const nvsKeyViewMode   = "synth_viewmode";
static const char* const nvsKeyButtonMode = "synth_btnmode";
static const char* const nvsKeyTouchMode  = "synth_touchmode";
static const char* const nvsKeyLoop       = "synth_loop";
static const char* const nvsKeyShuffle    = "synth_shuffle";
static const char* const nvsKeyAutoplay   = "synth_autoplay";
static const char* const nvsKeyLastSong   = "synth_lastsong";
static const char* const nvsKeyHeadroom   = "synth_headdroom";
static const char* const nvsKeyShufflePos = "synth_shufpos";
static const char* const nvsKeyIgnoreChan = "synth_ignorech";
static const char* const nvsKeyChanPerc   = "synth_chpercus";
static const char* const nvsKeySynthConf  = "synth_confblob";
static const char* const nvsKeySynthControlConf = "synth_ctrlconf";

static const char* menuItemModeOptions[] = {
    "Streaming",
    "File",
};

static const char* menuItemViewOptions[] = {
    "Pretty", "Visualizer", "Lyrics",         "Lyrics+Visualizer", "Waveform",
    "Table",  "Packets",    "Waveform+Table", "Waveform+Packets",  "Timing",
};

static const char* menuItemButtonOptions[] = {
    "Play Note",
    "Playback",
};

static const char* menuItemTouchOptions[] = {
    "Wheel Menu",
    "Pitch Bend",
    "Scrub",
};

static const char* menuItemOffOnOptions[] = {
    "Off",
    "On",
};

static const char* menuItemYesNoOptions[] = {
    "Yes",
    "No",
};

static const char* menuItemNoYesOptions[] = {
    "No",
    "Yes",
};

static const char* menuItemBankOptions[] = {
    "General MIDI",
    "MAGFest",
};

static const char* menuItemHeadroomOptions[] = {
    "0%",  "5%",  "10%", "15%", "20%", "25%", "30% (Default)", "35%", "40%", "45%",  "50%",
    "55%", "60%", "65%", "70%", "75%", "80%", "85%",           "90%", "95%", "100%",
};

static const int32_t menuItemModeValues[] = {
    0,
    1,
};

static const int32_t menuItemViewValues[] = {
    (int32_t)VM_PRETTY,
    (int32_t)VM_VIZ,
    (int32_t)VM_LYRICS,
    (int32_t)(VM_VIZ | VM_LYRICS),
    (int32_t)VM_GRAPH,
    (int32_t)VM_TEXT,
    (int32_t)VM_PACKETS,
    (int32_t)(VM_GRAPH | VM_TEXT),
    (int32_t)(VM_GRAPH | VM_PACKETS),
    (int32_t)VM_TIMING,
};

static const int32_t menuItemButtonValues[] = {
    (int32_t)BM_NOTE,
    (int32_t)BM_PLAYBACK,
};

static const int32_t menuItemTouchValues[] = {
    (int32_t)TM_MENU,
    (int32_t)TM_PITCH,
    (int32_t)TM_SCRUB,
};

static const int32_t menuItemLoopValues[] = {
    0,
    1,
};

static const int32_t menuItemShuffleValues[] = {
    0,
    1,
};

static const int32_t menuItemAutoplayValues[] = {
    0,
    1,
};

static const int32_t menuItemBankValues[] = {
    0,
    1,
};

static const int32_t menuItemControlSwitchValues[] = {
    0,
    127,
};

#define VOL_TO_HEADROOM(pct) ((pct) * 0x4000 / 100)

static const int32_t menuItemHeadroomValues[] = {
    0,
    VOL_TO_HEADROOM(10),
    VOL_TO_HEADROOM(20),
    VOL_TO_HEADROOM(30),
    VOL_TO_HEADROOM(40),
    VOL_TO_HEADROOM(50),
    MIDI_DEF_HEADROOM,
    VOL_TO_HEADROOM(70),
    VOL_TO_HEADROOM(80),
    VOL_TO_HEADROOM(90),
    VOL_TO_HEADROOM(100),
    VOL_TO_HEADROOM(110),
    VOL_TO_HEADROOM(120),
    VOL_TO_HEADROOM(130),
    VOL_TO_HEADROOM(140),
    VOL_TO_HEADROOM(150),
    VOL_TO_HEADROOM(160),
    VOL_TO_HEADROOM(170),
    VOL_TO_HEADROOM(180),
    VOL_TO_HEADROOM(190),
    VOL_TO_HEADROOM(200),
};

static settingParam_t menuItemModeBounds = {
    .def = 0,
    .min = 0,
    .max = 1,
    .key = nvsKeyMode,
};

static settingParam_t menuItemViewBounds = {
    .def = VM_PRETTY,
    .min = VM_PRETTY,
    .max = (VM_TIMING << 1) - 1,
    .key = nvsKeyViewMode,
};

static settingParam_t menuItemButtonBounds = {
    .def = BM_PLAYBACK,
    .min = BM_NOTE,
    .max = BM_PLAYBACK,
    .key = nvsKeyButtonMode,
};

static settingParam_t menuItemTouchBounds = {
    .def = TM_PITCH,
    .min = TM_PITCH,
    .max = TM_SCRUB,
    .key = nvsKeyTouchMode,
};

static settingParam_t menuItemLoopBounds = {
    .def = 0,
    .min = 0,
    .max = 1,
    .key = nvsKeyLoop,
};

static settingParam_t menuItemShuffleBounds = {
    .def = 0,
    .min = 0,
    .max = 1,
    .key = nvsKeyShuffle,
};

static settingParam_t menuItemAutoplayBounds = {
    .def = 0,
    .min = 0,
    .max = 1,
    .key = nvsKeyAutoplay,
};

static settingParam_t menuItemHeadroomBounds = {
    .def = MIDI_DEF_HEADROOM,
    .min = 0,
    .max = 0x8000,
    .key = nvsKeyHeadroom,
};

static settingParam_t menuItemIgnoreBounds = {
    .def = 0,
    .min = 0,
    .max = 1,
    .key = nvsKeyIgnoreChan,
};

static settingParam_t menuItemPercussionBounds = {
    .def = 0,
    .min = 0,
    .max = 1,
    .key = nvsKeyChanPerc,
};

static settingParam_t menuItemBankBounds = {
    .def = 0,
    .min = 0,
    .max = 1,
    .key = NULL,
};

static settingParam_t menuItemControl14bitBounds = {
    .def = 0,
    .min = 0,
    .max = 0x3FFF,
    .key = NULL,
};

static settingParam_t menuItemControl7bitBounds = {
    .def = 0,
    .min = 0,
    .max = 127,
    .key = NULL,
};

static const synthConfig_t defaultSynthConfig = {
    .ignoreChannelMask = 0,
    .percChannelMask = 0x0200, // Channel 10 set only
    .programs = {
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
    },
    .banks = {
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
    },
    .controlCounts = 0,
};

const char synthModeName[]          = "MIDI Player";
static const char intermissionMsg[] = "SWADGAOKE!";

swadgeMode_t synthMode = {
    .modeName                 = synthModeName,
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = true,
    .usesAccelerometer        = false,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = synthEnterMode,
    .fnExitMode               = synthExitMode,
    .fnMainLoop               = synthMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = NULL,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL,
    .fnDacCb                  = synthDacCallback,
};

static const uint32_t lfsrTaps[] = {
    0x3, // 2 bits
    0x6, // 3 bits
    0xC, // 4 bits
    0x14, // 5 bits
    0x30, // 6 bits
    0x60, // 7 bits
    0xB8, // 8 bits
    0x110, // 9 bits
    0x240, // 10 bits
    0x500, // 11 bits
    0xE08, // 12 bits
    0x1C80, // 13 bits
    0x3802, // 14 bits
    0x6000, // 15 bits
    0xD008, // 16 bits
};

static synthData_t* sd;

// Functions

static void synthEnterMode(void)
{
    sd = calloc(1, sizeof(synthData_t));
    loadFont("ibm_vga8.font", &sd->font, true);
    loadFont("sonic.font", &sd->betterFont, true);
    makeOutlineFont(&sd->betterFont, &sd->betterOutline, true);

    sd->perc[9] = true;
    midiPlayerInit(&sd->midiPlayer);
    sd->noteTime                        = 200000;
    sd->pitch                           = 0x2000;
    sd->longestProgramName              = gmProgramNames[24];
    sd->nextExpiry                      = 200000;
    sd->midiPlayer.songFinishedCallback = songEndCb;
    sd->midiPlayer.textMessageCallback  = midiTextCallback;

    // Read all the NVS values
    int32_t nvsRead = 0;
    // Playback Mode
    if (!readNvs32(nvsKeyMode, &nvsRead))
    {
        nvsRead = 0;
    }
    sd->fileMode = nvsRead ? true : false;

    // View mode
    if (!readNvs32(nvsKeyViewMode, &nvsRead))
    {
        nvsRead = (int32_t)VM_PRETTY;
    }
    sd->viewMode = (synthViewMode_t)nvsRead;

    // Button mode
    if (!readNvs32(nvsKeyButtonMode, &nvsRead))
    {
        nvsRead = (int32_t)BM_PLAYBACK;
    }
    sd->buttonMode = (synthButtonMode_t)nvsRead;

    // Touch mode
    if (!readNvs32(nvsKeyTouchMode, &nvsRead))
    {
        nvsRead = (int32_t)TM_PITCH;
    }
    sd->touchMode = (synthTouchMode_t)nvsRead;

    // Loop
    if (!readNvs32(nvsKeyLoop, &nvsRead))
    {
        nvsRead = 0;
    }
    sd->loop            = nvsRead ? true : false;
    sd->midiPlayer.loop = sd->loop;

    if (!readNvs32(nvsKeyShuffle, &nvsRead))
    {
        nvsRead = 0;
    }
    sd->shuffle = nvsRead ? true : false;

    if (!readNvs32(nvsKeyShufflePos, &nvsRead) || !nvsRead)
    {
        nvsRead = 0;
    }
    sd->shufflePos = nvsRead;

    if (!readNvs32(nvsKeyAutoplay, &nvsRead))
    {
        nvsRead = 0;
    }
    sd->autoplay = nvsRead ? true : false;

    if (!readNvs32(nvsKeyHeadroom, &nvsRead))
    {
        nvsRead = MIDI_DEF_HEADROOM;
    }
    sd->headroom            = nvsRead;
    sd->midiPlayer.headroom = sd->headroom;

    bool useDefaultConfig = true;
    size_t configBlobLen;
    if (readNvsBlob(nvsKeySynthConf, NULL, &configBlobLen))
    {
        if (configBlobLen > sizeof(synthConfig_t))
        {
            // Too big, don't know what to do, ignore
            ESP_LOGE("Synth", "Config blob length is too large, ignoring");
        }
        else
        {
            if (configBlobLen < sizeof(synthConfig_t))
            {
                ESP_LOGW("Synth", "Config blob length is shorter than expected, could be caused by update. filling rest with zeroes");
                memset((((char*)&sd->synthConfig) + configBlobLen), 0, sizeof(synthConfig_t) - configBlobLen);
            }

            if (readNvsBlob(nvsKeySynthConf, &sd->synthConfig, &configBlobLen))
            {
                useDefaultConfig = false;
            }
        }
    }

    if (useDefaultConfig)
    {
        memcpy(&sd->synthConfig, &defaultSynthConfig, sizeof(synthConfig_t));
    }

    if (sd->synthConfig.controlCounts)
    {
        size_t controlBlobLen;
        if (readNvsBlob(nvsKeySynthControlConf, NULL, &controlBlobLen))
        {
            if (controlBlobLen == (sd->synthConfig.controlCounts * sizeof(synthControlConfig_t)))
            {
                synthControlConfig_t configs[sd->synthConfig.controlCounts];

                // Expected size matches
                if (readNvsBlob(nvsKeySynthControlConf, &configs, &controlBlobLen))
                {
                    for (int blobIdx = 0; blobIdx < sd->synthConfig.controlCounts; blobIdx++)
                    {
                        synthControlConfig_t* copy = (synthControlConfig_t*)malloc(sizeof(synthControlConfig_t));
                        if (copy)
                        {
                            memcpy(copy, &configs[blobIdx], sizeof(synthControlConfig_t));
                            push(&sd->controllerSettings, copy);
                        }
                    }
                }
            }
        }
    }

    if (sd->fileMode)
    {
        size_t savedNameLen;
        if (readNvsBlob(nvsKeyLastSong, NULL, &savedNameLen))
        {
            sd->filenameBuf = malloc(savedNameLen < 128 ? 128 : savedNameLen + 1);

            if (readNvsBlob(nvsKeyLastSong, sd->filenameBuf, &savedNameLen))
            {
                sd->filenameBuf[savedNameLen] = '\0';
                sd->customFile                = true;
                sd->filename                  = sd->filenameBuf;
                synthSetFile(sd->filename);
            }
            else
            {
                ESP_LOGI("Synth", "Failed to load filename");
                free(sd->filenameBuf);
                sd->filenameBuf = NULL;
            }
        }
        else
        {
            ESP_LOGI("Synth", "No filename saved");
        }
    }

    sd->screen = SS_VIEW;

    const cnfsFileEntry* files = getCnfsFiles();
    for (const cnfsFileEntry* file = files; file < files + getCnfsNumFiles(); file++)
    {
        if ((strlen(file->name) > 4
             && (!strcmp(&file->name[strlen(file->name) - 4], ".mid") || !strcmp(&file->name[strlen(file->name) - 4], ".kar")))
            || (strlen(file->name) > 5 && !strcmp(&file->name[strlen(file->name) - 5], ".midi")))
        {
            // No longer strictly necessary with CNFS, but let's keep it how it was
            char* copyStr = strdup(file->name);
            if (copyStr)
            {
                // Insert the file into the list in a sorted manner
                bool added = false;

                node_t* last = NULL;
                node_t* node = sd->customFiles.first;

                while (node != NULL)
                {
                    if (strcasecmp((char*)node->val, copyStr) >= 0)
                    {
                        break;
                    }

                    last = node;
                    node = node->next;
                }

                addAfter(&sd->customFiles, copyStr, last);
            }
        }
    }

    sd->wheelTextArea.pos.x  = 15;
    sd->wheelTextArea.pos.y  = TFT_HEIGHT - sd->betterFont.height - 2;
    sd->wheelTextArea.width  = TFT_WIDTH - 30;
    sd->wheelTextArea.height = sd->betterFont.height + 2;

    //synthSetupMenu();

    // Use smol font for menu items, there might be a lot
    sd->renderer = initMenuManiaRenderer(NULL, NULL, &sd->font);
    sd->wheelMenu = initWheelMenu(&sd->betterFont, 90, &sd->wheelTextArea);
    sd->wheelMenu->unselR = 16;

    hashInit(&sd->menuMap, 512);

    synthSetupMenu(true);
    setupShuffle(sd->customFiles.length);
    synthSetupPlayer();

    sd->startupSeqComplete = true;
    sd->startupNote        = 60;

    // GM Instrument Category Images
    loadWsg("piano.wsg", &sd->instrumentImages[0], true);
    loadWsg("chromatic_percussion.wsg", &sd->instrumentImages[1], true);
    loadWsg("organ.wsg", &sd->instrumentImages[2], true);
    loadWsg("guitar.wsg", &sd->instrumentImages[3], true);
    loadWsg("bass.wsg", &sd->instrumentImages[4], true);
    loadWsg("solo_strings.wsg", &sd->instrumentImages[5], true);
    loadWsg("ensemble.wsg", &sd->instrumentImages[6], true);
    loadWsg("brass.wsg", &sd->instrumentImages[7], true);
    loadWsg("reed.wsg", &sd->instrumentImages[8], true);
    loadWsg("pipe.wsg", &sd->instrumentImages[9], true);
    loadWsg("synth_lead.wsg", &sd->instrumentImages[10], true);
    loadWsg("synth_pad.wsg", &sd->instrumentImages[11], true);
    loadWsg("synth_effects.wsg", &sd->instrumentImages[12], true);
    loadWsg("ethnic.wsg", &sd->instrumentImages[13], true);
    loadWsg("percussive.wsg", &sd->instrumentImages[14], true);
    loadWsg("sound_effects.wsg", &sd->instrumentImages[15], true);

    // Percussion channel image
    loadWsg("percussion.wsg", &sd->percussionImage, true);

    // Custom bank image
    loadWsg("magfest_bank.wsg", &sd->magfestBankImage, true);

    // Play/Pause/Etc Icons
    loadWsg("pause.wsg", &sd->pauseIcon, true);
    loadWsg("play.wsg", &sd->playIcon, true);
    loadWsg("playpause.wsg", &sd->playPauseIcon, true);
    loadWsg("fast_forward.wsg", &sd->ffwIcon, true);
    loadWsg("skip.wsg", &sd->skipIcon, true);
    loadWsg("loop.wsg", &sd->loopIcon, true);
    loadWsg("shuffle.wsg", &sd->shuffleIcon, true);
    loadWsg("stop.wsg", &sd->stopIcon, true);

    // Images for Wheel Menu
    loadWsg("open_song.wsg", &sd->fileImage, true);
    loadWsg("player.wsg", &sd->playerImage, true);
    loadWsg("channels.wsg", &sd->channelSetupImage, true);
    loadWsg("interface.wsg", &sd->uiImage, true);
    loadWsg("midi_volume.wsg", &sd->volumeImage, true);
    loadWsg("button_a.wsg", &sd->buttonImage, true);
    loadWsg("touchpad.wsg", &sd->touchImage, true);
    loadWsg("view_mode.wsg", &sd->viewModeImage, true);
    loadWsg("usb_mode.wsg", &sd->usbModeImage, true);
    loadWsg("hamburger.wsg", &sd->menuImage, true);
    loadWsg("pitch_wheel.wsg", &sd->pitchImage, true);
    loadWsg("reset.wsg", &sd->resetImage, true);
    loadWsg("ignore.wsg", &sd->ignoreImage, true);
    loadWsg("enable.wsg", &sd->enableImage, true);

    // MAXIMUM SPEEEEED
    setFrameRateUs(0);
}

static void synthExitMode(void)
{
    midiTextInfo_t* textInfo = NULL;
    while (NULL != (textInfo = pop(&sd->midiTexts)))
    {
        free(textInfo);
    }

    freeWsg(&sd->fileImage);
    freeWsg(&sd->playerImage);
    freeWsg(&sd->channelSetupImage);
    freeWsg(&sd->uiImage);
    freeWsg(&sd->volumeImage);
    freeWsg(&sd->buttonImage);
    freeWsg(&sd->touchImage);
    freeWsg(&sd->viewModeImage);
    freeWsg(&sd->usbModeImage);
    freeWsg(&sd->menuImage);
    freeWsg(&sd->pitchImage);
    freeWsg(&sd->resetImage);
    freeWsg(&sd->ignoreImage);
    freeWsg(&sd->enableImage);

    freeWsg(&sd->pauseIcon);
    freeWsg(&sd->playIcon);
    freeWsg(&sd->playPauseIcon);
    freeWsg(&sd->ffwIcon);
    freeWsg(&sd->skipIcon);
    freeWsg(&sd->loopIcon);
    freeWsg(&sd->shuffleIcon);
    freeWsg(&sd->stopIcon);

    freeWsg(&sd->magfestBankImage);
    freeWsg(&sd->percussionImage);
    for (int i = 0; i < 16; i++)
    {
        freeWsg(&sd->instrumentImages[i]);
    }

    deinitWheelMenu(sd->wheelMenu);
    deinitMenuManiaRenderer(sd->renderer);
    deinitMenu(sd->menu);

    // Clean up dynamic strings allocated for the controllers
    hashIterator_t iter = {0};
    while (hashIterate(&sd->menuMap, &iter))
    {
        char* controllerKey = (char*)iter.key;
        midiMenuItemInfo_t* info = (midiMenuItemInfo_t*)iter.value;

        hashIterRemove(&sd->menuMap, &iter);

        if (info->dynamicLabel)
        {
            free(info->label);
            info->label = NULL;
            info->dynamicLabel = false;
        }
    }

    hashDeinit(&sd->menuMap);

    unloadLyrics(&sd->karaoke);
    unloadMidiFile(&sd->midiFile);
    midiPlayerReset(&sd->midiPlayer);

    // Unload the filename if it was dynamic
    if (sd->filenameBuf)
    {
        free(sd->filenameBuf);
        sd->filenameBuf = NULL;
    }

    synthControlConfig_t* control;
    while (NULL != (control = pop(&sd->controllerSettings)))
    {
        free(control);
    }

    char* customFilename;
    while (NULL != (customFilename = pop(&sd->customFiles)))
    {
        free(customFilename);
    }

    freeFont(&sd->betterOutline);
    freeFont(&sd->betterFont);
    freeFont(&sd->font);
    free(sd);
    sd = NULL;
}

static void drawCircleSweep(int x, int y, int r, int startAngle, int sweepDeg, paletteColor_t col)
{
    // This is gonna be much slower than doing it the way e.g. drawCircleQuadrants does
    // But... oh well
    int lastX  = -1;
    int lastY  = -1;
    bool first = true;

    for (int theta = startAngle + 1; theta != startAngle + sweepDeg; theta++)
    {
        int cx = x + getCos1024(theta % 360) * r / 1024;
        int cy = y - getSin1024(theta % 360) * r / 1024;

        if (!first)
        {
            drawLineFast(lastX, lastY, cx, cy, col);
        }
        else
        {
            first = false;
        }

        lastX = cx;
        lastY = cy;
    }
}

static void drawIcon(musicIcon_t icon, paletteColor_t col, int16_t x, int16_t y, int16_t w, int16_t h)
{
    switch (icon)
    {
        case MI_PLAY:
        {
            // Draw triangle
            drawWsgSimple(&sd->playIcon, x, y);
            break;
        }
        case MI_PAUSE:
        {
            // Two vertical bars
            drawWsgSimple(&sd->pauseIcon, x, y);
            break;
        }
        case MI_STOP:
        {
            // Square
            drawWsgSimple(&sd->stopIcon, x, y);
            break;
        }
        case MI_PLAYPAUSE:
        {
            // Vertical bar
            // Triangle
            drawWsgSimple(&sd->playPauseIcon, x, y);
            break;
        }
        case MI_FFW:
        {
            // Two triangles
            drawWsgSimple(&sd->ffwIcon, x, y);
            // Two triangles (TODO)
            break;
        }
        case MI_REW:
        {
            // Reverse of FFW
            drawWsg(&sd->ffwIcon, x, y, true, false, 0);
            break;
        }
        case MI_SKIP:
        {
            // Triangle left
            // Vertical bar right
            drawWsgSimple(&sd->skipIcon, x, y);
            break;
        }
        case MI_PREV:
        {
            // Vertical bar left
            // Trangle right (facing left)
            drawWsg(&sd->skipIcon, x, y, true, false, 0);
            break;
        }
        case MI_REPEAT:
        {
            // 3/4 of a circle, then we add an arrow at the end
            // Kinda funky but close
            drawWsgSimple(&sd->loopIcon, x, y);

            break;
        }
        case MI_SHUFFLE:
        {
            // something like this
            /*
            ----  --->
                \/
            ____/\___>
            */
            /*int crossOffset = (w / 6);
            int cl          = x + (w / 2) - crossOffset;
            int cr          = x + (w / 2) + crossOffset;
            int yTop        = y + h / 8;
            int yBot        = y + h - h / 8;
            drawLineFast(x, yTop, cl, yTop, col);
            drawLineFast(x, yBot, cl, yBot, col);
            drawLineFast(cl, yTop, cr, yBot, col);
            // drawLineFast(cl, yBot, cr, yTop, col);
            drawLine(cl, yBot, cr, yTop, col, 2);
            drawLineFast(cr, yBot, x + w, yBot, col);
            drawLineFast(cr, yTop, x + w, yTop, col);*/
            drawWsgSimple(&sd->shuffleIcon, x, y);
            break;
        }
    }
}

static void synthMainLoop(int64_t elapsedUs)
{
    sd->marqueeTimer += elapsedUs;
    if (sd->updateMenu && !wheelMenuActive(sd->menu, sd->wheelMenu))
    {
        synthSetupMenu(sd->forceResetMenu);
        sd->updateMenu = false;
        sd->forceResetMenu = false;
    }

    if (sd->screen == SS_MENU)
    {
        drawMenuMania(sd->menu, sd->renderer, elapsedUs);
    }
    else if (sd->screen == SS_FILE_SELECT)
    {
        fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, SYNTH_BG_COLOR);
        drawSynthMode(elapsedUs);

        if (!textEntryDraw(elapsedUs))
        {
            // Entry is finished
            if (sd->filenameBuf && *sd->filenameBuf)
            {
                synthSetFile(sd->filenameBuf);
                if (sd->fileMode)
                {
                    // Setting file succeeded
                    sd->customFile = true;
                    sd->filename   = sd->filenameBuf;
                }
            }
            else
            {
                free(sd->filenameBuf);
                sd->filenameBuf = NULL;
                sd->fileMode    = false;
                sd->filename    = NULL;
            }
            sd->screen = SS_VIEW;
        }
    }
    else if (sd->screen == SS_VIEW)
    {
        // Blank the screen
        fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, SYNTH_BG_COLOR);

        if (!sd->fileMode && (!sd->installed || sd->err != 0))
        {
            drawText(&sd->font, c500, "ERROR!", 60, 60);
        }
        else if (!sd->startupSeqComplete && !sd->fileMode)
        {
            sd->noteTime -= elapsedUs;

            if (!sd->startupDrums)
            {
                if (sd->noteTime <= 0)
                {
                    if (sd->startSilence)
                    {
                        if (sd->startupNote == 0x7f)
                        {
                            sd->startupDrums = true;
                            sd->startSilence = true;
                            sd->startupNote  = ACOUSTIC_BASS_DRUM_OR_LOW_BASS_DRUM;
                            // give the drums a second
                            sd->noteTime = 1000000;
                        }
                        else
                        {
                            // 50ms of note
                            midiNoteOn(&sd->midiPlayer, 0, ++sd->startupNote, 0x7f);
                            sd->noteTime = 50000;
                        }
                    }
                    else
                    {
                        midiNoteOff(&sd->midiPlayer, 0, sd->startupNote, 0x7f);
                        // 25ms of silence between the notes
                        sd->noteTime = 25000;
                    }
                    sd->startSilence = !sd->startSilence;
                }
            }
            else
            {
                if (sd->noteTime <= 0)
                {
                    // Just play each drum note with a quarter-second gap between
                    midiNoteOn(&sd->midiPlayer, 9, sd->startupNote++, 0x7f);
                    sd->noteTime = 250000;

                    if (ACOUSTIC_BASS_DRUM_OR_LOW_BASS_DRUM <= sd->startupNote && sd->startupNote <= OPEN_TRIANGLE)
                    {
                        const char* drumName = gmDrumNames[sd->startupNote - ACOUSTIC_BASS_DRUM_OR_LOW_BASS_DRUM];
                        midiTextCallback(TEXT, drumName, strlen(drumName));
                    }

                    if (sd->startupNote > OPEN_TRIANGLE)
                    {
                        sd->startupSeqComplete = true;
                    }
                }
            }
        }

        drawSynthMode(elapsedUs);
    }

    // Delete any expired texts -- but only every so often, to prevent it from being weird and jumpy
    uint64_t now = esp_timer_get_time();
    if (now >= sd->nextExpiry)
    {
        node_t* curNode = sd->midiTexts.first;
        while (curNode != NULL)
        {
            midiTextInfo_t* curInfo = curNode->val;
            if (curInfo->expiration <= now)
            {
                node_t* tmp             = curNode->next;
                midiTextInfo_t* removed = removeEntry(&sd->midiTexts, curNode);
                if (removed)
                {
                    free(removed);
                }
                curNode = tmp;
            }
            else
            {
                curNode = curNode->next;
            }
        }

        sd->nextExpiry = now + 2000000;
    }

    // And handle specifically the lyrics differently

    // Plan:
    // - Starting at the <topLyric>, loop through the lyrics.
    // - IF the lyrics are in .KAR format (check for this earlier), treat the lyrics as a whole paragraph
    // - IF they don't, use some musical length of time that makes sense to define a whole block of lyrics (measure?)
    // - Once a whole paragraph has been expired for whatever amount of time, drop it - advance <topLyric> past it
    // - Keep advancing forward until we get to the start of the next block of not-yet-played lyrics
    // - If those lyrics are very far in the future (>= 2 measures or something?) don't show em
    // - If those lyrics are nearby in the future, show the whole group
    // - Maybe keep it to 2 paragraphs at once? 2 or 3 measures? idk

    // just a little scope for the frame timer I stole from pinball
    {
        int32_t startIdx  = (sd->frameTimesIdx + 1) % NUM_FRAME_TIMES;
        uint32_t tElapsed = sd->frameTimes[sd->frameTimesIdx] - sd->frameTimes[startIdx];
        if (0 != tElapsed)
        {
            uint32_t fps = (1000000 * NUM_FRAME_TIMES) / tElapsed;

            char tmp[16];
            snprintf(tmp, sizeof(tmp) - 1, "%" PRIu32, fps);
            drawText(&sd->font, c555, tmp, 35, 2);
        }
    }

    synthHandleInput(elapsedUs);

    sd->frameTimesIdx                 = (sd->frameTimesIdx + 1) % NUM_FRAME_TIMES;
    sd->frameTimes[sd->frameTimesIdx] = esp_timer_get_time();
}

static void synthSetupPlayer(void)
{
    if (!sd->installed)
    {
        sd->installed = installMidiUsb();
    }

    if (sd->fileMode)
    {
        sd->midiPlayer.loop                = sd->loop;
        sd->midiPlayer.textMessageCallback = midiTextCallback;
    }
    else
    {
        sd->midiPlayer.streamingCallback = usbMidiCallback;
        sd->midiPlayer.mode              = MIDI_STREAMING;
        midiPause(&sd->midiPlayer, false);
    }

    synthApplyConfig();
}

static void synthApplyConfig(void)
{
    sd->midiPlayer.headroom = sd->headroom;

    for (int i = 0; i < 16; i++)
    {
        uint16_t channelBit = (1 << i);
        sd->midiPlayer.channels[i].percussion = (sd->synthConfig.percChannelMask & channelBit) ? true : false;
        sd->midiPlayer.channels[i].ignore = (sd->synthConfig.ignoreChannelMask & channelBit) ? true : false;
        sd->programs[i] = sd->synthConfig.programs[i] & 0x7F;
        // This technically doesn't take effect properly until we call setProgram()!
        sd->midiPlayer.channels[i].bank = sd->synthConfig.banks[i];
        midiSetProgram(&sd->midiPlayer, i, sd->synthConfig.programs[i] & 0x7F);
    }

    int maxConfCount = sd->controllerSettings.length;
    synthControlConfig_t configBlob[maxConfCount];
    int confCount = 0;
    for (node_t* controlNode = sd->controllerSettings.first; controlNode != NULL && confCount < maxConfCount; controlNode = controlNode->next)
    {
        synthControlConfig_t* inConf = (synthControlConfig_t*)controlNode->val;
        if (inConf)
        {
            // Apply the controller
            for (int chIdx = 0; chIdx < 16; chIdx++)
            {
                if (inConf->chanMask & (1 << chIdx))
                {
                    midiControlChange(&sd->midiPlayer, chIdx, (midiControl_t)inConf->control, inConf->chanValues[chIdx]);
                }
            }
            memcpy(&configBlob[confCount++], inConf, sizeof(synthControlConfig_t));
        }
    }

    if (confCount > 0)
    {
        writeNvsBlob(nvsKeySynthControlConf, configBlob, sizeof(synthControlConfig_t) * confCount);
    }

    sd->synthConfig.controlCounts = confCount;

    // Save to NVS
    writeNvsBlob(nvsKeySynthConf, &sd->synthConfig, sizeof(synthConfig_t));
}

static void synthSaveControl(uint8_t channel, uint8_t control, uint8_t value)
{
    for (node_t* controlNode = sd->controllerSettings.first;
         controlNode != NULL;
         controlNode = controlNode->next)
    {
        synthControlConfig_t* conf = (synthControlConfig_t*)controlNode->val;
        if (conf && conf->control == control)
        {
            conf->chanMask |= (1 << channel);
            conf->chanValues[channel] = value;
            return;
        }
    }

    // Not found, add one
    synthControlConfig_t* conf = calloc(1, sizeof(synthControlConfig_t));
    if (conf)
    {
        conf->control = control;
        conf->chanMask = (1 << channel);
        conf->chanValues[channel] = value;

        push(&sd->controllerSettings, conf);
    }
}

static uint8_t synthGetControl(uint8_t channel, uint8_t control, uint8_t defValue)
{
    for (node_t* controlNode = sd->controllerSettings.first;
         controlNode != NULL;
         controlNode = controlNode->next)
    {
        synthControlConfig_t* conf = (synthControlConfig_t*)controlNode->val;
        if (conf && conf->control == control)
        {
            if (conf->chanMask & (1 << channel))
            {
                return conf->chanValues[channel];
            }
            else
            {
                return defValue;
            }
        }
    }

    // Not found
    return defValue;
}

static uint16_t synthGetControl14bit(uint8_t channel, uint8_t control, uint16_t defValue)
{
    uint8_t msbControl = (uint8_t)control & ~(1 << 5);
    uint8_t lsbControl = (uint8_t)control | (1 << 5);

    if (control >= 64)
    {
        // There are a couple other MSB/LSB controllers, and those have an odd MSB and an even LSB
        msbControl = (uint8_t)control | 1;
        lsbControl = (uint8_t)control & ~1;
    }

    uint16_t result = synthGetControl(channel, msbControl, (defValue >> 7) & 0x7F);
    result <<= 7;
    result |= synthGetControl(channel, lsbControl, defValue & 0x7F);

    return result;
}

static void addChannelsMenu(menu_t* menu, const synthConfig_t* config)
{
    // TODO kinda a hack but I'm sure I'll remember to update it
    int rotTopMenu = 3;

    menu = startSubMenu(menu, menuItemChannels);
    wheelMenuSetItemInfo(sd->wheelMenu, menuItemChannels, &sd->channelSetupImage, rotTopMenu++, NO_SCROLL);

    // Pre-add the item infos for the programs
    for (int gmProg = 0; gmProg < 128; gmProg++)
    {
        midiMenuItemInfo_t* itemInfo = &sd->itemInfos[sd->itemInfoCount++];

        itemInfo->type = SMT_PROGRAM;
        itemInfo->bank = 0;
        itemInfo->program = gmProg;
        itemInfo->label = gmProgramNames[gmProg];
        itemInfo->dynamicLabel = false;
        writeShortName(itemInfo->shortLabel, sizeof(itemInfo->shortLabel), itemInfo->label);
        wheelMenuSetItemInfo(sd->wheelMenu, itemInfo->label, NULL, gmProg % 8, NO_SCROLL);
        wheelMenuSetItemTextIcon(sd->wheelMenu, itemInfo->label, itemInfo->shortLabel);
        wheelMenuSetItemSize(sd->wheelMenu, itemInfo->label, -1, -1, WM_SHAPE_ROUNDED_RECT);

        hashPut(&sd->menuMap, gmProgramNames[gmProg], itemInfo);
    }

    for (int gmCategory = 0; gmCategory < 16; gmCategory++)
    {
        wheelMenuSetItemInfo(sd->wheelMenu, gmProgramCategoryNames[gmCategory], &sd->instrumentImages[gmCategory], gmCategory, NO_SCROLL);
        wheelMenuSetItemColor(sd->wheelMenu, gmProgramCategoryNames[gmCategory], c000, c333);
        wheelMenuSetItemSize(sd->wheelMenu, gmProgramCategoryNames[gmCategory], -1, -1, WM_SHAPE_DEFAULT);
    }

    for (int magProg = 0; magProg < magfestTimbreCount; magProg++)
    {
        midiMenuItemInfo_t* itemInfo = &sd->itemInfos[sd->itemInfoCount++];

        itemInfo->type = SMT_PROGRAM;
        itemInfo->bank = 1;
        itemInfo->program = magProg;
        itemInfo->label = magfestTimbres[magProg]->name;
        itemInfo->dynamicLabel = false;
        writeShortName(itemInfo->shortLabel, sizeof(itemInfo->shortLabel), itemInfo->label);
        wheelMenuSetItemInfo(sd->wheelMenu, itemInfo->label, NULL, magProg, NO_SCROLL);
        wheelMenuSetItemTextIcon(sd->wheelMenu, itemInfo->label, itemInfo->shortLabel);
        wheelMenuSetItemSize(sd->wheelMenu, itemInfo->label, -1, -1, WM_SHAPE_ROUNDED_RECT);

        hashPut(&sd->menuMap, itemInfo->label, itemInfo);
    }

    // Controllers are pretty complicated so just do those the first time with the rest of their logic
    bool controllersSetUp = false;
    int totalControls = 0;

    for (int chIdx = 0; chIdx < 16; chIdx++)
    {
        midiMenuItemInfo_t* itemInfo = &sd->itemInfos[sd->itemInfoCount++];
        itemInfo->type = SMT_CHANNEL;
        itemInfo->channel = chIdx;
        itemInfo->label = channelLabels[chIdx];
        itemInfo->dynamicLabel = false;
        hashPut(&sd->menuMap, channelLabels[chIdx], itemInfo);

        menu = startSubMenu(menu, channelLabels[chIdx]);
        wheelMenuSetItemInfo(sd->wheelMenu, channelLabels[chIdx], NULL, (16-chIdx) % 16, NO_SCROLL);
        wheelMenuSetItemTextIcon(sd->wheelMenu, channelLabels[chIdx], shortChannelLabels[chIdx]);

        addSettingsOptionsItemToMenu(menu, menuItemIgnore, menuItemYesNoOptions, menuItemModeValues, ARRAY_SIZE(menuItemModeValues), &menuItemIgnoreBounds, (config->ignoreChannelMask & (1 << chIdx)) ? 1 : 0);
        addSettingsOptionsItemToMenu(menu, menuItemPercussion, menuItemNoYesOptions, menuItemModeValues, ARRAY_SIZE(menuItemModeValues), &menuItemPercussionBounds, (config->percChannelMask & (1 << chIdx)) ? 1 : 0);
        addSettingsOptionsItemToMenu(menu, menuItemBank, menuItemBankOptions, menuItemBankValues, ARRAY_SIZE(menuItemBankValues), &menuItemBankBounds, config->banks[chIdx]);

        if (0 == (config->percChannelMask & (1 << chIdx)))
        {
            // Instrument select submenu
            char* nameBuffer = sd->channelInstrumentLabels[chIdx];
            wsg_t* chanInstrumentIcon = NULL;

            if (config->banks[chIdx] == 0)
            {
                snprintf(nameBuffer, 64, "%s%s", menuItemInstrument, gmProgramNames[config->programs[chIdx]]);
                chanInstrumentIcon = &sd->instrumentImages[config->programs[chIdx] / 8];

                // "Instrument: <name>" item
                menu = startSubMenu(menu, nameBuffer);
                for (int category = 0; category < 16; category++)
                {
                    // Category select menu
                    menu = startSubMenu(menu, gmProgramCategoryNames[category]);
                    for (int instrument = 0; instrument < 8; instrument++)
                    {
                        // Instrument within category
                        addSingleItemToMenu(menu, gmProgramNames[category * 8 + instrument]);
                    }
                    // End category sub-menu
                    menu = endSubMenu(menu);
                }

                // End instruments submenu
                menu = endSubMenu(menu);
            }
            else
            {
                uint8_t program = config->programs[chIdx];
                if (program >= magfestTimbreCount)
                {
                    program = 0;
                }

                chanInstrumentIcon = &sd->magfestBankImage;

                snprintf(nameBuffer, 64, "%s%s", menuItemInstrument, magfestTimbres[program]->name);
                // "Instrument: <name>" item
                menu = startSubMenu(menu, nameBuffer);

                for (int instrument = 0; instrument < magfestTimbreCount; instrument++)
                {
                    addSingleItemToMenu(menu, magfestTimbres[instrument]->name);
                }
                menu = endSubMenu(menu);
            }

            wheelMenuSetItemInfo(sd->wheelMenu, nameBuffer, chanInstrumentIcon, 4, NO_SCROLL);
            wheelMenuSetItemSize(sd->wheelMenu, nameBuffer, -1, -1, WM_SHAPE_DEFAULT);
        }

        menu = startSubMenu(menu, menuItemControls);

        for (int ctrlIdx = 0; ctrlIdx < ARRAY_SIZE(controllerDefs); ctrlIdx++)
        {
            const midiControllerDesc_t* controller = &controllerDefs[ctrlIdx];
            if (synthIsControlSupported(controller))
            {
                char* labelStr = controller->desc;
                bool dynamicLabel = false;

                switch (controller->type)
                {
                    case CTRL_SWITCH:
                    {
                        // Options menu item, on/off
                        addSettingsOptionsItemToMenu(menu, labelStr, menuItemOffOnOptions, menuItemControlSwitchValues, ARRAY_SIZE(menuItemControlSwitchValues), &menuItemControl7bitBounds,
                                                     synthGetControl(chIdx, controller->control, midiGetControlValue(&sd->midiPlayer, chIdx, (midiControl_t)controller->control)));
                        break;
                    }

                    case CTRL_CC_MSB:
                    {
                        // Settings value item
                        addSettingsItemToMenu(menu, labelStr, &menuItemControl14bitBounds, synthGetControl14bit(chIdx, controller->control, midiGetControlValue14bit(&sd->midiPlayer, chIdx, (midiControl_t)controller->control)));
                        break;
                    }

                    case CTRL_CC_LSB:
                    {
                        // Ignore because we'll handle the whole controller in CTRL_CC_MSB
                        break;
                    }

                    case CTRL_7BIT:
                    case CTRL_UNDEFINED:
                    {
                        // Settings value item, 0-127
                        addSettingsItemToMenu(menu, labelStr, &menuItemControl7bitBounds, synthGetControl(chIdx, controller->control, midiGetControlValue(&sd->midiPlayer, chIdx, (midiControl_t)controller->control)));
                        break;
                    }

                    case CTRL_NO_DATA:
                    {
                        // Single menu item, A just triggers it
                        addSingleItemToMenu(menu, labelStr);
                        break;
                    }
                }

                if (!controllersSetUp || dynamicLabel)
                {
                    midiMenuItemInfo_t* itemInfo = &sd->itemInfos[sd->itemInfoCount++];
                    itemInfo->type = SMT_CONTROLLER;
                    itemInfo->controller = controller;
                    itemInfo->label = labelStr;
                    itemInfo->dynamicLabel = dynamicLabel;
                    wheelMenuSetItemInfo(sd->wheelMenu, labelStr, NULL, totalControls++, NO_SCROLL);
                    // Use rounded rectangle, stretched to fit
                    wheelMenuSetItemSize(sd->wheelMenu, labelStr, -1, -1, WM_SHAPE_ROUNDED_RECT);
                    writeShortName(itemInfo->shortLabel, sizeof(itemInfo->shortLabel), labelStr);
                    wheelMenuSetItemTextIcon(sd->wheelMenu, labelStr, itemInfo->shortLabel);

                    // If the label IS dynamic, we don't actually change it without resetting the menu
                    // So, don't worry about the hash key getting out of sync here
                    hashPut(&sd->menuMap, labelStr, itemInfo);
                }
            }
        }

        controllersSetUp = true;

        menu = endSubMenu(menu);

        addSingleItemToMenu(menu, menuItemReset);

        // End channel submenu
        menu = endSubMenu(menu);
    }

    wheelMenuSetItemInfo(sd->wheelMenu, menuItemIgnore, (sd->synthConfig.ignoreChannelMask & (1 << sd->menuSelectedChannel)) ? &sd->ignoreImage : &sd->enableImage, 0, NO_SCROLL);
    wheelMenuSetItemInfo(sd->wheelMenu, menuItemYesNoOptions[0], &sd->enableImage, 0, NO_SCROLL);
    wheelMenuSetItemInfo(sd->wheelMenu, menuItemYesNoOptions[1], &sd->ignoreImage, 1, NO_SCROLL);

    wheelMenuSetItemInfo(sd->wheelMenu, menuItemOffOnOptions[0], &sd->ignoreImage, 1, NO_SCROLL);
    wheelMenuSetItemInfo(sd->wheelMenu, menuItemOffOnOptions[1], &sd->enableImage, 0, NO_SCROLL);

    wheelMenuSetItemInfo(sd->wheelMenu, menuItemPercussion, &sd->percussionImage, 1, NO_SCROLL);
    wheelMenuSetItemColor(sd->wheelMenu, menuItemPercussion, c000, c333);
    wheelMenuSetItemSize(sd->wheelMenu, menuItemPercussion, -1, -1, WM_SHAPE_DEFAULT);

    wheelMenuSetItemInfo(sd->wheelMenu, menuItemNoYesOptions[0], &sd->ignoreImage, 1, NO_SCROLL);
    wheelMenuSetItemInfo(sd->wheelMenu, menuItemNoYesOptions[1], &sd->enableImage, 0, NO_SCROLL);

    wheelMenuSetItemInfo(sd->wheelMenu, menuItemBank, (sd->synthConfig.banks[sd->menuSelectedChannel & 0xF] == 0) ? NULL : &sd->magfestBankImage, 2, NO_SCROLL);
    wheelMenuSetItemTextIcon(sd->wheelMenu, menuItemBank, (sd->synthConfig.banks[sd->menuSelectedChannel & 0xF] == 0) ? "GM" : NULL);
    wheelMenuSetItemSize(sd->wheelMenu, menuItemBank, -1, -1, WM_SHAPE_DEFAULT);

    wheelMenuSetItemInfo(sd->wheelMenu, menuItemBankOptions[0], NULL, 0, NO_SCROLL);
    wheelMenuSetItemTextIcon(sd->wheelMenu, menuItemBankOptions[0], "GM");
    wheelMenuSetItemSize(sd->wheelMenu, menuItemBankOptions[0], -1, -1, WM_SHAPE_DEFAULT);
    wheelMenuSetItemInfo(sd->wheelMenu, menuItemBankOptions[1], &sd->magfestBankImage, 1, NO_SCROLL);
    wheelMenuSetItemSize(sd->wheelMenu, menuItemBankOptions[1], -1, -1, WM_SHAPE_DEFAULT);

    wheelMenuSetItemInfo(sd->wheelMenu, menuItemControls, &sd->uiImage, 3, NO_SCROLL);

    wheelMenuSetItemInfo(sd->wheelMenu, menuItemReset, &sd->resetImage, 5, NO_SCROLL);

    // End "Channels: " submenu
    menu = endSubMenu(menu);

    addSingleItemToMenu(menu, menuItemResetAll);
    wheelMenuSetItemInfo(sd->wheelMenu, menuItemResetAll, &sd->resetImage, rotTopMenu++, NO_SCROLL);
}

static void synthSetupMenu(bool forceReset)
{
    bool restore = false;
    const char* menuState[8] = {0};

    if (sd->menu != NULL)
    {
        restore = !forceReset && !wheelMenuActive(sd->menu, sd->wheelMenu);
        menuSavePosition(menuState, ARRAY_SIZE(menuState), sd->menu);
        deinitMenu(sd->menu);
        sd->menu = NULL;

        // Clean up dynamic strings allocated for the controllers
        hashIterator_t iter = {0};
        while (hashIterate(&sd->menuMap, &iter))
        {
            char* controllerKey = (char*)iter.key;
            midiMenuItemInfo_t* info = (midiMenuItemInfo_t*)iter.value;

            hashIterRemove(&sd->menuMap, &iter);

            if (info->dynamicLabel)
            {
                free(info->label);
                info->label = NULL;
                info->dynamicLabel = false;
            }
        }

        sd->itemInfoCount = 0;
    }

    sd->menu      = initMenu(synthModeName, synthMenuCb);

    int rotTopMenu = 0;
    addSettingsOptionsItemToMenu(sd->menu, menuItemPlayMode, menuItemModeOptions, menuItemModeValues, 2,
                                 &menuItemModeBounds, sd->fileMode);
    wheelMenuSetItemInfo(sd->wheelMenu, menuItemPlayMode, (sd->fileMode ? &sd->fileImage : &sd->usbModeImage), rotTopMenu++, SCROLL_HORIZ);
    wheelMenuSetItemInfo(sd->wheelMenu, menuItemModeOptions[0], &sd->usbModeImage, 0, NO_SCROLL);
    wheelMenuSetItemInfo(sd->wheelMenu, menuItemModeOptions[1], &sd->fileImage, 1, NO_SCROLL);

    wheelMenuSetItemInfo(sd->wheelMenu, NULL, &sd->percussionImage, UINT8_MAX, NO_SCROLL);

    // Start "Player" Menu
    sd->menu = startSubMenu(sd->menu, menuItemPlayer);
    wheelMenuSetItemInfo(sd->wheelMenu, menuItemPlayer, &sd->playerImage, rotTopMenu++, NO_SCROLL);

    int rotPlayerMenu = 0;

    if (sd->fileMode)
    {
        // Start File list
        sd->menu = startSubMenu(sd->menu, menuItemSelectFile);
        wheelMenuSetItemInfo(sd->wheelMenu, menuItemSelectFile, &sd->fileImage, rotPlayerMenu++, NO_SCROLL);

        addSingleItemToMenu(sd->menu, menuItemCustomSong);

        for (node_t* node = sd->customFiles.first; node != NULL; node = node->next)
        {
            addSingleItemToMenu(sd->menu, (char*)node->val);
        }

        // End File list
        sd->menu = endSubMenu(sd->menu);
    }

    // Player Menu, continued
    addSettingsOptionsItemToMenu(sd->menu, menuItemLoop, menuItemOffOnOptions, menuItemLoopValues,
                                 ARRAY_SIZE(menuItemLoopValues), &menuItemLoopBounds, sd->loop);
    wheelMenuSetItemInfo(sd->wheelMenu, menuItemLoop, &sd->loopIcon, rotPlayerMenu++, SCROLL_HORIZ);

    addSettingsOptionsItemToMenu(sd->menu, menuItemShuffle, menuItemOffOnOptions, menuItemShuffleValues,
                                 ARRAY_SIZE(menuItemShuffleValues), &menuItemShuffleBounds, sd->shuffle);
    wheelMenuSetItemInfo(sd->wheelMenu, menuItemShuffle, &sd->shuffleIcon, rotPlayerMenu++, SCROLL_HORIZ);

    addSettingsOptionsItemToMenu(sd->menu, menuItemAutoplay, menuItemOffOnOptions, menuItemAutoplayValues,
                                 ARRAY_SIZE(menuItemAutoplayValues), &menuItemAutoplayBounds, sd->autoplay);
    wheelMenuSetItemInfo(sd->wheelMenu, menuItemAutoplay, &sd->playPauseIcon, rotPlayerMenu++, SCROLL_HORIZ);

    addSettingsOptionsItemToMenu(sd->menu, menuItemHeadroom, menuItemHeadroomOptions, menuItemHeadroomValues,
                                 ARRAY_SIZE(menuItemHeadroomValues), &menuItemHeadroomBounds, sd->headroom);
    wheelMenuSetItemInfo(sd->wheelMenu, menuItemHeadroom, &sd->volumeImage, rotPlayerMenu++, SCROLL_HORIZ_R | ZOOM_GAUGE);

    // End Player Menu
    sd->menu = endSubMenu(sd->menu);

    // Start "Interface" Menu
    sd->menu = startSubMenu(sd->menu, menuItemUi);
    wheelMenuSetItemInfo(sd->wheelMenu, menuItemUi, &sd->uiImage, rotTopMenu++, NO_SCROLL);

    int rotUiMenu = 0;
    addSettingsOptionsItemToMenu(sd->menu, menuItemViewMode, menuItemViewOptions, menuItemViewValues,
                                 ARRAY_SIZE(menuItemViewValues), &menuItemViewBounds, sd->viewMode);
    wheelMenuSetItemInfo(sd->wheelMenu, menuItemViewMode, &sd->viewModeImage, rotUiMenu++, NO_SCROLL);
    for (int vmNum = 0; vmNum < ARRAY_SIZE(menuItemViewOptions); vmNum++)
    {
        wheelMenuSetItemInfo(sd->wheelMenu, menuItemViewOptions[vmNum], NULL, vmNum, NO_SCROLL);
        writeShortName(sd->menuViewShortnames[vmNum], sizeof(sd->menuViewShortnames[vmNum]), menuItemViewOptions[vmNum]);
        wheelMenuSetItemTextIcon(sd->wheelMenu, menuItemViewOptions[vmNum], sd->menuViewShortnames[vmNum]);
    }

    addSettingsOptionsItemToMenu(sd->menu, menuItemButtonMode, menuItemButtonOptions, menuItemButtonValues,
                                 ARRAY_SIZE(menuItemButtonValues), &menuItemButtonBounds, sd->buttonMode);
    wheelMenuSetItemInfo(sd->wheelMenu, menuItemButtonMode, &sd->buttonImage, rotUiMenu++, NO_SCROLL);
    wheelMenuSetItemInfo(sd->wheelMenu, menuItemButtonOptions[0], &sd->fileImage, 0, NO_SCROLL);
    wheelMenuSetItemInfo(sd->wheelMenu, menuItemButtonOptions[1], &sd->playPauseIcon, 1, NO_SCROLL);

    addSettingsOptionsItemToMenu(sd->menu, menuItemTouchMode, menuItemTouchOptions, menuItemTouchValues,
                                 ARRAY_SIZE(menuItemTouchValues), &menuItemTouchBounds, sd->touchMode);
    wheelMenuSetItemInfo(sd->wheelMenu, menuItemTouchMode, &sd->touchImage, rotUiMenu++, NO_SCROLL);
    wheelMenuSetItemInfo(sd->wheelMenu, menuItemTouchOptions[0], &sd->menuImage, 0, NO_SCROLL);
    wheelMenuSetItemInfo(sd->wheelMenu, menuItemTouchOptions[1], &sd->pitchImage, 1, NO_SCROLL);
    wheelMenuSetItemInfo(sd->wheelMenu, menuItemTouchOptions[2], &sd->skipIcon, 2, NO_SCROLL);


    // End "Interface" Menu
    sd->menu = endSubMenu(sd->menu);

    // Add "Channel Setup" menu at top level
    addChannelsMenu(sd->menu, &sd->synthConfig);

    addSingleItemToMenu(sd->menu, mnuBackStr);
    wheelMenuSetItemInfo(sd->wheelMenu, mnuBackStr, NULL, UINT8_MAX, NO_SCROLL);
    wheelMenuSetItemColor(sd->wheelMenu, mnuBackStr, c500, c300);
    wheelMenuSetItemTextIcon(sd->wheelMenu, mnuBackStr, "x");

    if (restore)
    {
        sd->menu = menuRestorePosition(menuState, ARRAY_SIZE(menuState), sd->menu);
    }
}

static void preloadLyrics(karaokeInfo_t* karInfo, const midiFile_t* midiFile)
{
    bool karFormat     = false;
    uint32_t eventMask = (1 << COPYRIGHT) | (1 << SEQUENCE_OR_TRACK_NAME) | (1 << LYRIC) | (1 << TEXT);
    midiFileReader_t reader;

    if (initMidiParser(&reader, midiFile))
    {
        reader.handleMetaEvents = true;
        uint32_t tempo          = 500000;

        midiTextInfo_t* lastInfo = NULL;
        uint8_t textTrack = 0;
        bool skipOtherTracks = false;
        midiEvent_t event;

        while (midiNextEvent(&reader, &event))
        {
            if (event.type == META_EVENT && event.meta.type < 0xF && 0 != ((1 << event.meta.type) & eventMask))
            {
                if (!karFormat && (event.meta.type == LYRIC || event.meta.type == TEXT) && event.meta.length > 0)
                {
                    char start = event.meta.text[0];
                    char end   = event.meta.text[event.meta.length - 1];
                    if (start == '\\' || end == '\\' || start == '/' || end == '/')
                    {
                        karFormat = true;
                    }
                }

                if (!skipOtherTracks && lastInfo && textTrack != event.track)
                {
                    if (lastInfo->timestamp == event.absTime && !strncmp(lastInfo->text, event.meta.text, MIN(lastInfo->length, event.meta.length)))
                    {
                        printf("Duplicated text events (%s) on channel %" PRIu8 " and %" PRIu8 "! Skipping.\n", event.meta.text, textTrack, event.track);
                        skipOtherTracks = true;
                        continue;
                    }
                }
                else if (skipOtherTracks && event.track != textTrack)
                {
                    continue;
                }

                // TODO we could save a couple bytes if we parsed the file an additional time to check how many events
                // there are in total...
                midiTextInfo_t* info = (midiTextInfo_t*)malloc(sizeof(midiTextInfo_t));

                if (info)
                {
                    info->text      = event.meta.text;
                    info->length    = event.meta.length;
                    info->type      = event.meta.type;
                    info->tempo     = tempo;
                    info->timestamp = event.absTime;

                    push(&karInfo->lyrics, info);

                    if (event.absTime != 0)
                    {
                        lastInfo = info;
                        textTrack = event.track;
                    }
                }
            }
            else if (event.type == META_EVENT && event.meta.type == TEMPO)
            {
                tempo = event.meta.tempo;
            }
            else if (event.type == META_EVENT && event.meta.type == TIME_SIGNATURE)
            {
                /*uint8_t beatsPerMeasure = event.meta.timeSignature.numerator;
                // I don't understand how this isn't the same thing as the denominator?
                uint32_t quarterNotesPerBeat = event.meta.timeSignature.num32ndNotesPerBeat / 8;
                //uint32_t typeOfNotes = (1 << event.meta.timeSignature.denominator);
                karInfo->measureLength = beatsPerMeasure;
                karInfo->noteLength = quarterNotesPerBeat;*/

                memcpy(&karInfo->timeSignature, &event.meta.timeSignature, sizeof(midiTimeSignature_t));
            }
        }

        deinitMidiParser(&reader);
        karInfo->karFormat = karFormat;

        if (!karInfo->timeSignature.midiClocksPerMetronomeTick)
        {
            karInfo->timeSignature.midiClocksPerMetronomeTick = 24;
        }

        if (!karInfo->timeSignature.num32ndNotesPerBeat)
        {
            karInfo->timeSignature.num32ndNotesPerBeat = 8;
        }
    }
}

static void unloadLyrics(karaokeInfo_t* karInfo)
{
    void* textInfo = NULL;
    while (NULL != (textInfo = pop(&karInfo->lyrics)))
    {
        free(textInfo);
    }
}

static void synthSetFile(const char* filename)
{
    // First: stop and reset the MIDI player
    midiPlayerReset(&sd->midiPlayer);

    synthSetupPlayer();

    // Next: Free any text that might reference the song file still
    midiTextInfo_t* textInfo = NULL;
    while (NULL != (textInfo = pop(&sd->midiTexts)))
    {
        free(textInfo);
    }

    unloadLyrics(&sd->karaoke);

    // Finally: Unload the MIDI file itself
    unloadMidiFile(&sd->midiFile);

    // Set the default values for time signature
    sd->karaoke.timeSignature.numerator                  = 4;
    sd->karaoke.timeSignature.denominator                = 2;
    sd->karaoke.timeSignature.num32ndNotesPerBeat        = 8;
    sd->karaoke.timeSignature.midiClocksPerMetronomeTick = 24;

    if (NULL != filename)
    {
        // Cleanup done, now load the new file
        if (loadMidiFile(filename, &sd->midiFile, true))
        {
            sd->fileMode = true;

            midiPlayerReset(&sd->midiPlayer);
            synthSetupPlayer();
            midiSetFile(&sd->midiPlayer, &sd->midiFile);
            preloadLyrics(&sd->karaoke, &sd->midiFile);

            // And tell it to play immediately
            midiPause(&sd->midiPlayer, false);

            writeNvsBlob(nvsKeyLastSong, filename, strlen(filename));
            sd->stopped = false;
        }
        else
        {
            // We failed to open the file
            sd->fileMode    = false;
            const char* msg = "Failed to open MIDI file!";
            midiTextCallback(TEXT, msg, strlen(msg));
        }
    }
}

static void drawSynthMode(int64_t elapsedUs)
{
    if (sd->viewMode & VM_VIZ)
    {
        drawSampleGraphCircular();
    }

    if (sd->viewMode & VM_GRAPH)
    {
        drawSampleGraph();
    }

#define IS_DRUM(note) ((ACOUSTIC_BASS_DRUM_OR_LOW_BASS_DRUM <= note) && (note <= OPEN_TRIANGLE))
    drawText(&sd->font, c050, readyStr, (TFT_WIDTH - textWidth(&sd->font, readyStr)) / 2, 3);

    char packetMsg[64];
    int16_t textY = 15;
    for (int ch = 0; ch < 16; ch++)
    {
        midiChannel_t* channel = &sd->midiPlayer.channels[ch];
        bool percussion        = channel->percussion;
        voiceStates_t* states = (percussion ? &sd->midiPlayer.percVoiceStates : &sd->midiPlayer.poolVoiceStates);
        sd->playing[ch] = (0 != (channel->allocedVoices & (states->on | states->held | states->sustenuto)));

        paletteColor_t col = sd->playing[ch] ? c555 : c222;

        if (sd->viewMode == VM_PRETTY)
        {
            wsg_t* image
                = percussion ? &sd->percussionImage : &sd->instrumentImages[sd->midiPlayer.channels[ch].program / 8];
            if (sd->midiPlayer.channels[ch].bank != 0)
            {
                image = &sd->magfestBankImage;
            }

            int16_t x    = ((ch % 8) + 1) * (TFT_WIDTH - image->w * 8) / 9 + image->w * (ch % 8);
            int16_t imgY = (ch < 8) ? 30 : TFT_HEIGHT - 30 - 32;
            drawWsgSimple(image, x, imgY);

            if (sd->midiPlayer.channels[ch].held)
            {
                col = c050;
            }

            if (sd->playing[ch])
            {
                drawRect(x, imgY, x + 32, imgY + 32, col);
            }

            if (sd->midiPlayer.channels[ch].sustenuto)
            {
                drawLine(x, imgY + 31, x + 31, imgY + 31, c500, 2);
            }

            int16_t infoY = (ch < 8) ? (imgY + 32 + 2) : (imgY - 16 - 2);
            drawChannelInfo(&sd->midiPlayer, ch, x, infoY, 32, 16);
            drawPitchWheelRect(ch, x, infoY - 1, 32, 18);
        }
        else if (sd->viewMode & (VM_PACKETS | VM_TEXT))
        {
            const char* programName = percussion
                                          ? ((sd->localChannel == 9 && IS_DRUM(sd->startupNote))
                                                 ? gmDrumNames[sd->startupNote - ACOUSTIC_BASS_DRUM_OR_LOW_BASS_DRUM]
                                                 : sd->midiPlayer.channels[ch].timbre.name)
                                          : gmProgramNames[sd->midiPlayer.channels[ch].program];
            if (sd->midiPlayer.channels[ch].bank != 0)
            {
                programName = sd->midiPlayer.channels[ch].timbre.name;
            }
            // Draw the program name
            drawText(&sd->font, col, programName, 10, textY);
        }

        if (sd->viewMode & VM_PACKETS)
        {
            // And the last packet for this channel
            snprintf(packetMsg, sizeof(packetMsg), "%02hhX %02hhX %02hhX %02hhX", sd->lastPackets[ch][0],
                     sd->lastPackets[ch][1], sd->lastPackets[ch][2], sd->lastPackets[ch][3]);
            packetMsg[sizeof(packetMsg) - 1] = '\0';
            drawText(&sd->font, col, packetMsg, TFT_WIDTH - textWidth(&sd->font, packetMsg) - 10, textY);
        }
        else if (sd->viewMode & VM_TEXT)
        {
            drawChannelInfo(&sd->midiPlayer, ch, textWidth(&sd->font, sd->longestProgramName) + 4, textY - 2,
                            TFT_WIDTH - (textWidth(&sd->font, sd->longestProgramName) + 4), 16);
        }

        textY += sd->font.height + 4;
    }

    if (sd->viewMode & VM_LYRICS)
    {
        drawKaraokeLyrics(
            SAMPLES_TO_MIDI_TICKS(sd->midiPlayer.sampleCount, sd->midiPlayer.tempo, sd->midiPlayer.reader.division),
            &sd->karaoke);
        drawBeatsMetronome(true, TFT_HEIGHT - 20, true, TFT_WIDTH / 2, TFT_HEIGHT, 15);
    }

    if (sd->viewMode == VM_PRETTY)
    {
        uint16_t pitch = sd->localPitch ? sd->pitch : sd->midiPlayer.channels[sd->localChannel].pitchBend;
        int16_t deg    = (360 + ((pitch - 0x2000) * 90 / 0x1FFF)) % 360;
        drawCircleQuadrants(0, TFT_HEIGHT / 2, 16, true, false, false, true, (pitch == 0x2000) ? c222 : c555);
        drawLineFast(0, TFT_HEIGHT / 2, 16 * getCos1024(deg) / 1024, TFT_HEIGHT / 2 - (16 * getSin1024(deg) / 1024),
                     c500);

        drawMidiText(false, 0);

#define ICON_SIZE (16)

        int16_t iconPosR = TFT_WIDTH / 2;
        int16_t iconPosL = iconPosR - ICON_SIZE - 1;
        if (sd->fileMode && sd->filename && *sd->filename)
        {
            int16_t textW = textWidth(&sd->font, sd->filename);
            int16_t textX = MAX(15, (TFT_WIDTH - textW) / 2);

            if (textW > TFT_WIDTH - (ICON_SIZE + 3) * 2 - 10)
            {
                textX = ICON_SIZE + 3 + 5;
                drawTextMarquee(&sd->font, c034, sd->filename, textX, TFT_HEIGHT - sd->font.height - 15, TFT_WIDTH - ICON_SIZE - 3 - 5, &sd->marqueeTimer);
                iconPosR = TFT_WIDTH - ICON_SIZE - 2 - 5;
                iconPosL = 1;
            }
            else
            {
                iconPosR = drawText(&sd->font, c034, sd->filename, textX, TFT_HEIGHT - sd->font.height - 15);
                iconPosL = textX - ICON_SIZE - 1;
            }
        }

        musicIcon_t statusIcon = MI_PAUSE;
        if (sd->midiPlayer.paused)
        {
            if (sd->stopped)
            {
                statusIcon = MI_STOP;
            }
            else
            {
                statusIcon = MI_PAUSE;
            }
        }
        else
        {
            statusIcon = MI_PLAY;
        }
        drawIcon(statusIcon, c555, iconPosR, TFT_HEIGHT - 15 - ICON_SIZE, ICON_SIZE, ICON_SIZE);

        if (sd->loop || sd->midiPlayer.loop)
        {
            drawIcon(MI_REPEAT, c555, iconPosL, TFT_HEIGHT - 15 - ICON_SIZE, ICON_SIZE, ICON_SIZE);
        }
    }
    else
    {
        char countsBuf[16];
        // Display the number of clipped samples
        snprintf(countsBuf, sizeof(countsBuf), "%" PRIu32, sd->midiPlayer.clipped);
        drawText(&sd->font, c500, countsBuf, TFT_WIDTH - textWidth(&sd->font, countsBuf) - 15,
                 TFT_HEIGHT - sd->font.height - 15);
    }

    // Draw BPM
    char tempoStr[16];
    snprintf(tempoStr, sizeof(tempoStr), "%" PRIu32 " BPM", TEMPO_TO_BPM(sd->midiPlayer.tempo));
    drawText(&sd->font, c500, tempoStr, TFT_WIDTH - textWidth(&sd->font, tempoStr) - 35, 3);

    if (sd->viewMode & VM_TIMING)
    {
        midiTimeSignature_t* ts = &sd->karaoke.timeSignature;
        if (!ts->midiClocksPerMetronomeTick || !ts->num32ndNotesPerBeat || !ts->numerator || !sd->fileMode)
        {
            const char* noTimeMsg = "No Time Signature";
            drawText(&sd->font, c500, noTimeMsg, (TFT_WIDTH - textWidth(&sd->font, noTimeMsg)) / 2,
                     (TFT_HEIGHT - sd->font.height) / 2);
        }
        else
        {
            char buffer[64] = {0};

            // division tells us the number of MIDI ticks (or SMTPE frames) per QUARTER NOTE only!
            // the tempo tells us how many microseconds are between each quarter note
            // that's sufficient for playing back the song!
            // also keep in mind that in MIDI terminology, a quarter note IS a beat
            // SO, tempo is in <us/beat> and division is in <ticks/beat>

            // So, we can calculate the current tick without any time signature info:
            uint32_t tick = SAMPLES_TO_MIDI_TICKS(sd->midiPlayer.sampleCount, sd->midiPlayer.tempo,
                                                  sd->midiPlayer.reader.division);

            // enter, time signatures:
            // They tell us "musically" how everything arranged
            // Numerator is just the top value of the time signature, aka beats per bar
            // A beat is just a note, but which type is determined by the denominator
            // A bar is like, a little bit of a row of sheet music or something

            // Denominator tells us the type of notes in each bar -- so, 4/4 means 4 1/4 notes,
            // 3/4 means a bar is 3 1/4 quarter notes, 4/8 means a bar is 4 eighth notes, 1/16 means
            // a bar is one sixteenth note, 4/6 means a bar is 6 dotted eighth notes (aka 1/6 notes)
            // In MIDI, Denominator is actually log2(1/denominator), so 0 means a whole note (1/(1<<0))
            // 1 means a half note, 2 means a quarter note, etc.
            // So, we really just need to convert this to quarter notes, and then we can easily get ticks

            // NEXT UP: "midiClocksPerMetronomeClick" is... maybe irrelevant? or maybe it tells us how
            // long a measure is or something... idk

            // The last value, 32nd notes per beat, is really there to tell you which notes the song's
            // "notation" uses, I think. So if the song is notated with quarter notes, you would use 8
            // here, meaning that every 4th 32nd note is actually annotated. A value of 32 would mean
            // every 32nd note is notated. a value of 1 means a beat is in whole notes.
            // At least, maybe. I don't know enough (any) music theory to understand any of the explanations.
            // dammit jim, i'm a programmer, not a musician!

            // So using the last value, we can determine how many notes we "care about"
            // This is not necessarily quarter notes but song notes
            uint32_t dispNotesPerBar = ts->numerator * ts->num32ndNotesPerBeat / 8;
            // The actual number of MIDI ticks in a single bar
            // TODO this is not correct?
            uint32_t ticksPerBar = ts->numerator;

            snprintf(buffer, sizeof(buffer), "Signature: %" PRIu8 "/%d", ts->numerator, 1 << ts->denominator);
            drawText(&sd->font, c555, buffer, 18, 60);

            snprintf(buffer, sizeof(buffer), "Beats/Bar: %" PRIu8, ts->numerator);
            drawText(&sd->font, c555, buffer, 18, 75);

            snprintf(buffer, sizeof(buffer), "Ticks/Beat: %" PRIu16, sd->midiFile.timeDivision);
            drawText(&sd->font, c555, buffer, 18, 90);

            snprintf(buffer, sizeof(buffer), "32nd Notes/Beat: %" PRIu8, ts->num32ndNotesPerBeat);
            drawText(&sd->font, c555, buffer, 18, 105);

            int curMeasure = (tick / ts->midiClocksPerMetronomeTick); // * div;
            int curNote    = (tick / ts->midiClocksPerMetronomeTick) % (ts->num32ndNotesPerBeat / 8);
            int curBeat    = (tick / sd->midiFile.timeDivision);

            snprintf(buffer, sizeof(buffer), "File Division: %" PRIu16 " ticks", sd->midiFile.timeDivision);
            drawText(&sd->font, c555, buffer, 18, 120);

            snprintf(buffer, sizeof(buffer), "Cur tick: %" PRIu32, tick);
            drawText(&sd->font, c555, buffer, 18, 140);
            snprintf(buffer, sizeof(buffer), "Cur beat: %d", curBeat);
            drawText(&sd->font, c555, buffer, 18, 155);

            // Maybe??
            // So, take the numerator (which is # quarter notes) and convert it to tell us the number of notes per beat

            snprintf(buffer, sizeof(buffer), "Cur bar: %" PRIu32, curBeat / dispNotesPerBar);
            drawText(&sd->font, c555, buffer, 18, 170);

            // Draw a mini metronome
            drawBeatsMetronome(true, 190, true, TFT_WIDTH / 2, TFT_HEIGHT - 25, 20);
        }
    }

    if (wheelMenuActive(sd->menu, sd->wheelMenu))
    {
        fillDisplayArea(sd->wheelTextArea.pos.x, sd->wheelTextArea.pos.y - 2,
                        sd->wheelTextArea.pos.x + sd->wheelTextArea.width,
                        sd->wheelTextArea.pos.y + sd->wheelTextArea.height + 2,
                        c025);
        drawTriangleOutlined(sd->wheelTextArea.pos.x, sd->wheelTextArea.pos.y - 2,
                            sd->wheelTextArea.pos.x - sd->betterFont.height / 2, sd->wheelTextArea.pos.y + sd->wheelTextArea.height / 2,
                            sd->wheelTextArea.pos.x, sd->wheelTextArea.pos.y + sd->wheelTextArea.height + 2,
                            c025, c025);
        drawTriangleOutlined(sd->wheelTextArea.pos.x + sd->wheelTextArea.width, sd->wheelTextArea.pos.y - 2,
                            sd->wheelTextArea.pos.x + sd->wheelTextArea.width + sd->betterFont.height / 2, sd->wheelTextArea.pos.y + sd->wheelTextArea.height / 2,
                            sd->wheelTextArea.pos.x + sd->wheelTextArea.width, sd->wheelTextArea.pos.y + sd->wheelTextArea.height + 2,
                            c025, c025);
        drawWheelMenu(sd->menu, sd->wheelMenu, elapsedUs);
    }
}

static void drawBeatsMetronome(bool beats, int16_t beatsY, bool metronome, int16_t metX, int16_t metY, int16_t metR)
{
    midiTimeSignature_t* ts = &sd->karaoke.timeSignature;

    if (!sd->fileMode || !sd->midiFile.data || !ts->midiClocksPerMetronomeTick || !ts->num32ndNotesPerBeat || !ts->numerator)
    {
        return;
    }

    uint32_t tick
        = SAMPLES_TO_MIDI_TICKS(sd->midiPlayer.sampleCount, sd->midiPlayer.tempo, sd->midiPlayer.reader.division);
    int curBeat              = (tick / sd->midiFile.timeDivision);
    uint32_t dispNotesPerBar = ts->numerator * ts->num32ndNotesPerBeat / 8;

    // Draw some circles...
    for (int i = 0; i < dispNotesPerBar; i++)
    {
        if (i == (curBeat % dispNotesPerBar))
        {
            drawCircleFilled((i + 1) * TFT_WIDTH / (dispNotesPerBar + 1), beatsY, 5, c500);
        }
        drawCircle((i + 1) * TFT_WIDTH / (dispNotesPerBar + 1), beatsY, 5, c555);
    }

    // Draw a mini metronome
    // Angle on each side of the metronome
    // The metronome will travel 4x this angle over its rotation
    int metAngle = 90;

    // Need to modulo the current tick count by a single beat
    int ticksPerBeat = sd->midiFile.timeDivision * (ts->num32ndNotesPerBeat / 8);
    int noteProgress = tick % (ticksPerBeat * 2);

    // now turn noteProgress into an angle
    //  0% --> 180
    // 25% --> 135
    // 50% --> 90
    // 75% --> 45
    // 100% -> 0
    // 125% -> 45
    // 150% -> 90
    // 175% -> 135
    // 200% -> 180
    // ... needs to be different for odd notes and even notes i think

    int curAngle = 90 - (90 - noteProgress * metAngle * 2 / (ticksPerBeat));
    // There's a way to do this without branching but I am lazy
    if (curAngle > 180)
    {
        curAngle = 360 - curAngle;
    }

    drawLineFast(metX, metY, metX + getCos1024(curAngle) * metR / 1024, metY - getSin1024(curAngle) * metR / 1024,
                 c555);
}

static void drawKaraokeLyrics(uint32_t ticks, karaokeInfo_t* karInfo)
{
    if (!sd->fileMode || !sd->midiFile.data || !sd->midiFile.timeDivision)
    {
        return;
    }

    int msgLen = 0;
    char textMessages[1024];
    textMessages[0] = '\0';

    paletteColor_t midiTextColor = c550;
    bool colorSet                = false;

    uint32_t now          = ticks;
    uint32_t curBeat      = (now / sd->midiFile.timeDivision);
    uint32_t notesPerBar  = karInfo->timeSignature.numerator * karInfo->timeSignature.num32ndNotesPerBeat / 8;
    uint32_t ticksPerBeat = sd->midiFile.timeDivision * (karInfo->timeSignature.num32ndNotesPerBeat) / 8;
    uint32_t ticksPerBar  = notesPerBar * ticksPerBeat;
    uint32_t noteProgress = now % (ticksPerBeat);

    uint32_t noteLength    = ticksPerBeat;
    uint32_t barStartTime  = (now / ticksPerBar) * ticksPerBar;
    uint32_t noteStartTime = curBeat * sd->midiFile.timeDivision;

    // printf("%d ticks per beat\n", ticksPerBeat);

    int curStage = 0;

    // Timer for very long pauses
    bool drawBar       = true;
    uint32_t nearLyric = 0;
    uint32_t farLyric  = 0;

    // We want all the lyrics that were part of the last 2 bars (not including the current one)
    int32_t oldCutoff = barStartTime - ticksPerBar + 1;
    // And also any lyrics that are part of the next 3 bars (which includes the current one)
    int32_t newCutoff = barStartTime + ticksPerBar * 3 - 1;

    int32_t curCutoff      = noteStartTime - noteLength / 4;
    int32_t curAfterCutoff = noteStartTime + noteLength / 2;

    // TODO we should just draw a bar of lyrics, then
    const int16_t startX = 10;
    const int16_t startY = 25;

    int16_t x = startX;
    int16_t y = startY;

    const char* remaining = NULL;
    bool curNoteReached   = false;
    int lastLyricBar      = 0;

    // TODO LIST:
    // - Consistent positioning -- use (measure % 2) or something to pick a consistent half of the screen
    // - Properly handle songs with overlapping lyrics for multiple tracks, which currently get interleaved (almost!)
    // - Show the progress bar until the next lyrics start, not just until they're shown
    // - Don't insert a newline if there are no spaces between two lyrics

#define FLUSH()                                                                                                        \
    do                                                                                                                 \
    { /* DEBUG: drawLineFast(x, y, x, y + sd->betterFont.height, c500); */                                             \
        remaining = drawTextWordWrapFixed(&sd->betterFont, curNoteReached ? c555 : c550, textMessages, startX, startY, \
                                          &x, &y, TFT_WIDTH - startX, TFT_HEIGHT - startY);                            \
        msgLen    = 0;                                                                                                 \
        textMessages[0] = '\0';                                                                                        \
    } while (0)

    node_t* curNode = karInfo->lyrics.first;
    while (curNode != NULL && msgLen + 1 < sizeof(textMessages))
    {
        midiTextInfo_t* curInfo  = curNode->val;
        midiTextInfo_t* nextInfo = curNode->next ? ((midiTextInfo_t*)curNode->next->val) : NULL;
        int curLyricBar = curInfo->timestamp / ticksPerBar;
        // lyricLength used as the timer for text progress
        int lyricLength = noteLength;

        if (curInfo->timestamp < oldCutoff)
        {
            // Lyric is older than 2 bars
            nearLyric = curInfo->timestamp;
            // skip without drawing
        }
        else if (curInfo->timestamp <= noteStartTime - noteLength)
        {
            // Lyric is older than 1 note

            // Add a newline between measures, for files without KAR-style formatting
            if (!karInfo->karFormat && lastLyricBar != curLyricBar && msgLen < sizeof(textMessages))
            {
                textMessages[msgLen++] = '\n';
                textMessages[msgLen] = '\0';
            }

            // Draw entire lyric
            msgLen
                += writeMidiText(textMessages + msgLen, sizeof(textMessages) - msgLen - 1, curInfo, karInfo->karFormat);

            // Lyrics are on screen currently, so no need to draw big progress bar
            drawBar = false;
        }
        else if (curInfo->timestamp < now)
        {
            // Lyric is in the past
            bool drawNoteProgress = false;

            // Lyrics are on screen currently, so no need to draw big progress bar
            drawBar = false;

            if (nextInfo && nextInfo->timestamp <= now)
            {
                // NEXT lyric is also in the past

                // Add a newline between measures, for files without KAR-style formatting
                if (!karInfo->karFormat && lastLyricBar != curLyricBar && msgLen < sizeof(textMessages))
                {
                    textMessages[msgLen++] = '\n';
                    textMessages[msgLen] = '\0';
                }

                // Draw this entire lyric
                msgLen += writeMidiText(textMessages + msgLen, sizeof(textMessages) - msgLen - 1, curInfo,
                                        karInfo->karFormat);
            }
            else
            {
                // No next lyric or lyric is not in the past

                if (nextInfo && (nextInfo->timestamp - curInfo->timestamp) < noteLength)
                {
                    // NEXT lyric is less than 1 beat after this one (but still in the future)
                    // Set the timer to the difference
                    lyricLength = nextInfo->timestamp - curInfo->timestamp;
                }

                // Print any old pending messages
                // Also empties out the buffer so we can use it
                FLUSH();

                // Add a newline between measures, for files without KAR-style formatting
                if (!karInfo->karFormat && lastLyricBar != curLyricBar && msgLen < sizeof(textMessages))
                {
                    textMessages[msgLen++] = '\n';
                    textMessages[msgLen] = '\0';
                }

                // Write the message into the buffer
                msgLen += writeMidiText(textMessages + msgLen, sizeof(textMessages) - msgLen - 1, curInfo,
                                        karInfo->karFormat);
                // Make a temporary pointer to the buffer text so we can move it if needed
                char* cur = textMessages;

                // If there's a leading newline or anything, everything gets messed up, so handle it
                while (*cur == '\n')
                {
                    x = startX;
                    y += sd->betterFont.height + 1;
                    cur++;
                }

                while (x == startX && *cur == ' ')
                {
                    cur++;
                }

                // Figure out the progress bar width
                int w        = textWidth(&sd->betterFont, cur);
                int progress = w * (now - curInfo->timestamp) / lyricLength;

                // If the lyric won't fit on screen, handle wrapping here
                if (x + w >= TFT_WIDTH - startX)
                {
                    x = startX;
                    y += sd->betterFont.height + 1;
                }

                while (x == startX && *cur == ' ')
                {
                    cur++;
                }

                if (y >= TFT_HEIGHT - startY)
                {
                    // TODO: Instead of doing this, make a sliding window for previous lyrics
                    // and only draw as far back as space permits without cutting off the current
                    // or future lyrics
                    break;
                }

                // Draw the yellow portion of the text up to the progress point
                drawTextBounds(&sd->betterFont, c550, cur, x, y, 0, 0, x + progress, TFT_HEIGHT);
                // Draw the white portion of the text after the progress point
                drawTextBounds(&sd->betterFont, c555, cur, x, y, x + progress, 0, TFT_WIDTH, TFT_HEIGHT);
                x = drawText(&sd->betterOutline, c505, cur, x, y);

                // Reset the buffer since we've printed the text already
                msgLen          = 0;
                textMessages[0] = '\0';

                curNoteReached = true;
            }
        }
        else if (curInfo->timestamp < newCutoff)
        {
            drawBar = false;

            if (!curNoteReached)
            {
                FLUSH();
                if (remaining)
                {
                    break;
                }
            }

            curNoteReached = true;
            // Note is less than (3 bars - 1 tick) in the future

            // Add a newline between measures, for files without KAR-style formatting
            if (!karInfo->karFormat && lastLyricBar != curLyricBar && msgLen < sizeof(textMessages))
            {
                textMessages[msgLen++] = '\n';
                textMessages[msgLen] = '\0';
            }

            msgLen
                += writeMidiText(textMessages + msgLen, sizeof(textMessages) - msgLen - 1, curInfo, karInfo->karFormat);
        }
        else
        {
            // Note is 3 bars or more in the future
            farLyric = curInfo->timestamp;

            // Don't continue searching
            break;
        }

        lastLyricBar = curLyricBar;
        curNode      = curNode->next;
    }

    if (drawBar && nearLyric != farLyric)
    {
        // There's a long rest with no lyrics, draw a progress bar
        if ((farLyric - now) <= (farLyric - nearLyric))
        {
            int w = (TFT_WIDTH - 60) * (farLyric - now) / (farLyric - nearLyric);
            drawRect(30, TFT_HEIGHT - 40, TFT_WIDTH - 30, TFT_HEIGHT - 30, c550);
            fillDisplayArea(30, TFT_HEIGHT - 40, 30 + CLAMP(TFT_WIDTH - 60 - w, 0, TFT_WIDTH - 60), TFT_HEIGHT - 30, c550);
        }

        // Draw a fun thingy
        int16_t intermissionX = (TFT_WIDTH - textWidth(&sd->betterFont, intermissionMsg)) / 2;
        drawTextMulticolored(&sd->betterFont, intermissionMsg, intermissionX, (TFT_HEIGHT - sd->betterFont.height) / 2,
                             textColors
                                 + (((now - nearLyric) * notesPerBar / (noteLength)) % (ARRAY_SIZE(textColors) / 2)),
                             ARRAY_SIZE(textColors) / 2, 16);
        drawText(&sd->betterOutline, c555, intermissionMsg, intermissionX, (TFT_HEIGHT - sd->betterOutline.height) / 2);
    }

    FLUSH();
}

static void drawMidiText(bool filter, uint32_t types)
{
    int msgLen = 0;
    char textMessages[1024];
    textMessages[0] = '\0';

    paletteColor_t midiTextColor = c550;
    bool colorSet                = false;

    node_t* curNode = sd->midiTexts.first;
    while (curNode != NULL && msgLen + 1 < sizeof(textMessages))
    {
        midiTextInfo_t* curInfo = curNode->val;

        if (filter && !((1 << curInfo->type) & types))
        {
            curNode = curNode->next;
            continue;
        }

        if (!colorSet)
        {
            switch (curInfo->type)
            {
                case TEXT:
                    midiTextColor = c555;
                    break;

                case COPYRIGHT:
                    midiTextColor = c050;
                    break;

                case SEQUENCE_OR_TRACK_NAME:
                case INSTRUMENT_NAME:
                    midiTextColor = c005;
                    break;

                case LYRIC:
                    midiTextColor = c550;
                    break;

                case MARKER:
                case CUE_POINT:
                default:
                    midiTextColor = c333;
                    break;
            }

            colorSet = true;
        }

        if (*textMessages != '\0')
        {
            if (*curInfo->text == '/' || *curInfo->text == '\\')
            {
                textMessages[msgLen++] = '\n';
            }
            else if (*curInfo->text != ' ')
            {
                textMessages[msgLen++] = ' ';
            }
        }
        msgLen += writeMidiText(textMessages + msgLen, sizeof(textMessages) - msgLen - 1, curInfo, false);

        curNode = curNode->next;
    }
    textMessages[msgLen] = '\0';

    int16_t x = 18;
    int16_t y = 80;
    drawTextWordWrap(&sd->font, midiTextColor, textMessages, &x, &y, TFT_WIDTH - 18, TFT_HEIGHT - 60);
}

static void drawPitchWheelRect(uint8_t chIdx, int16_t x, int16_t y, int16_t w, int16_t h)
{
    uint16_t pitch = sd->midiPlayer.channels[chIdx].pitchBend;

    if (pitch != 0x2000)
    {
        int16_t deg   = (360 + ((pitch - 0x2000) * 90 / 0x1FFF)) % 360;
        int16_t lineY = y + h / 2 - (h * getSin1024(deg) / 2048);
        drawRect(x, y, x + w, y + h + 1, c555);
        drawLineFast(x + 1, lineY, x + w - 2, lineY, c500);
    }
}

static void synthDacCallback(uint8_t* samples, int16_t len)
{
    midiPlayerFillBuffer(&sd->midiPlayer, samples, len);

    // Leave the visualizer doing something interesting while paused but not stopped
    if (!sd->midiPlayer.paused || sd->stopped)
    {
        memcpy(sd->lastSamples, samples, MIN(len, VIZ_SAMPLE_COUNT));
        sd->sampleCount = MIN(len, VIZ_SAMPLE_COUNT);
    }
}

static void synthHandleButton(const buttonEvt_t evt)
{
    if (sd->buttonMode == BM_NOTE)
    {
        if (evt.down)
        {
            // let us play notes
            switch (evt.button)
            {
                case PB_UP:
                case PB_DOWN:
                {
                    sd->localChannel = sd->localChannel ? 0 : 9;
                    break;
                }

                case PB_LEFT:
                {
                    if (sd->startupNote > 0)
                    {
                        sd->startupNote--;
                    }
                    break;
                }

                case PB_RIGHT:
                {
                    if (sd->startupNote < 0x7F)
                    {
                        sd->startupNote++;
                    }
                    break;
                }

                case PB_SELECT:
                    break;
                case PB_START:
                {
                    break;
                }

                case PB_A:
                {
                    if (sd->localChannel == 9
                        && (ACOUSTIC_BASS_DRUM_OR_LOW_BASS_DRUM <= sd->startupNote && sd->startupNote <= OPEN_TRIANGLE))
                    {
                        const char* drumName = gmDrumNames[sd->startupNote - ACOUSTIC_BASS_DRUM_OR_LOW_BASS_DRUM];
                        midiTextCallback(TEXT, drumName, strlen(drumName));
                    }
                    midiNoteOn(&sd->midiPlayer, sd->localChannel, sd->startupNote, 0x7F);
                    break;
                }

                case PB_B:
                {
                    midiNoteOff(&sd->midiPlayer, sd->localChannel, sd->startupNote, 0x7F);
                    break;
                }
            }
        }
    }
    else if (sd->buttonMode == BM_PLAYBACK)
    {
        if (evt.down)
        {
            switch (evt.button)
            {
                case PB_UP:
                {
                    midiSetTempo(&sd->midiPlayer, BPM_TO_TEMPO(1 + TEMPO_TO_BPM(sd->midiPlayer.tempo)));
                    if (!sd->upHeld)
                    {
                        sd->upHeld      = true;
                        sd->upHeldTimer = 500000;
                    }
                    break;
                }

                case PB_DOWN:
                {
                    // Tempo down
                    if (TEMPO_TO_BPM(sd->midiPlayer.tempo) > 1)
                    {
                        midiSetTempo(&sd->midiPlayer, BPM_TO_TEMPO(TEMPO_TO_BPM(sd->midiPlayer.tempo) - 1));
                        if (!sd->downHeld)
                        {
                            sd->downHeld      = true;
                            sd->downHeldTimer = 500000;
                        }
                    }
                    break;
                }

                case PB_LEFT:
                {
                    // Seek Left
                    const uint32_t seekLeftAmt = 5 * DAC_SAMPLE_RATE_HZ;

                    if (!sd->leftHeld)
                    {
                        sd->leftHeld = true;
                        sd->leftHeldTimer = 500000;
                    }
                    else if (sd->fileMode && sd->midiPlayer.sampleCount > seekLeftAmt)
                    {
                        midiSeek(&sd->midiPlayer, SAMPLES_TO_MIDI_TICKS(sd->midiPlayer.sampleCount - seekLeftAmt, sd->midiPlayer.tempo, sd->midiPlayer.reader.division));
                    }
                    else if (sd->fileMode && sd->midiPlayer.sampleCount > (DAC_SAMPLE_RATE_HZ / 2))
                    {
                        // Restart song
                        midiSeek(&sd->midiPlayer, 0);
                        synthApplyConfig();
                    }
                    else
                    {
                        prevSong();
                    }
                    break;
                }

                case PB_RIGHT:
                {

                    if (!sd->rightHeld)
                    {
                        sd->rightHeld = true;
                        sd->rightHeldTimer = 500000;
                    }
                    else
                    {
                        // Seek Right
                        const uint32_t seekRightAmt = DAC_SAMPLE_RATE_HZ;
                        midiSeek(&sd->midiPlayer, SAMPLES_TO_MIDI_TICKS(sd->midiPlayer.sampleCount + seekRightAmt, sd->midiPlayer.tempo, sd->midiPlayer.reader.division));
                    }
                    break;
                }

                case PB_SELECT:
                    // Reserved
                    break;

                case PB_START:
                    // Handled by caller
                    break;

                case PB_A:
                {
                    // Pause/Unpause
                    if (sd->fileMode)
                    {
                        if (sd->stopped && sd->filename)
                        {
                            synthSetFile(sd->filename);
                        }
                        else
                        {
                            midiPause(&sd->midiPlayer, !sd->midiPlayer.paused);
                        }
                    }
                    else
                    {
                        synthApplyConfig();
                    }
                    break;
                }

                case PB_B:
                {
                    if (sd->fileMode)
                    {
                        if (sd->midiPlayer.paused)
                        {
                            sd->stopped = true;
                            midiPlayerReset(&sd->midiPlayer);
                            synthSetupPlayer();
                        }
                        else
                        {
                            midiPause(&sd->midiPlayer, true);
                        }
                    }
                    else
                    {
                        midiAllSoundOff(&sd->midiPlayer);
                    }
                    break;
                }
            }
        }
        else
        {
            if (evt.button == PB_UP)
            {
                // Stop increasing BPM
                sd->upHeld      = false;
                sd->upHeldTimer = 0;
            }
            else if (evt.button == PB_DOWN)
            {
                // Stop decreasing BPM
                sd->downHeld      = false;
                sd->downHeldTimer = 0;
            }
            else if (evt.button == PB_LEFT)
            {
                if (sd->leftHeld && sd->leftHeldTimer > 100000)
                {
                    // The button wasn't held, just clicked
                    if (sd->midiPlayer.sampleCount > DAC_SAMPLE_RATE_HZ)
                    {
                        // If you click left after the song has played for 1s, restart first
                        midiSeek(&sd->midiPlayer, 0);
                    }
                    else
                    {
                        // If you click within the first 1s of the song, go to the previous song
                        prevSong();
                    }
                }

                // Stop seeking left
                sd->leftHeld = false;
                sd->leftHeldTimer = 0;
            }
            else if (evt.button == PB_RIGHT)
            {
                if (sd->rightHeld && sd->rightHeldTimer > 100000)
                {
                    // The button wasn't held, just clicked
                    nextSong();
                }

                // Stop seeking right
                sd->rightHeld = false;
                sd->rightHeldTimer = 0;
            }
        }
    }
}

static void handleButtonTimer(int64_t* timer, int64_t interval, int64_t elapsedUs, buttonBit_t button)
{
    buttonEvt_t evt;
    while (*timer < elapsedUs)
    {
        evt.state  = sd->lastButtonState;
        evt.down   = true;
        evt.button = button;
        synthHandleButton(evt);

        *timer += interval;
    }
    *timer -= elapsedUs;
}

static void synthHandleInput(int64_t elapsedUs)
{
    int32_t phi, r, intensity;
    bool isWheelMenuActive = wheelMenuActive(sd->menu, sd->wheelMenu);

    if (isWheelMenuActive || sd->touchMode == TM_MENU)
    {
        if (sd->screen == SS_VIEW)
        {
            if (getTouchJoystick(&phi, &r, &intensity))
            {
                sd->menu = wheelMenuTouch(sd->menu, sd->wheelMenu, phi, r);
            }
            else
            {
                sd->menu = wheelMenuTouchRelease(sd->menu, sd->wheelMenu);
            }

            bool nowActive = wheelMenuActive(sd->menu, sd->wheelMenu);
            if (nowActive && !isWheelMenuActive)
            {
                // Menu just became active, reset it?
                sd->updateMenu = true;
            }
            else if (isWheelMenuActive && !nowActive)
            {
                sd->updateMenu = true;
                sd->forceResetMenu = true;
            }
            isWheelMenuActive = nowActive;
        }
    }
    else if (sd->touchMode == TM_PITCH)
    {
        if (getTouchJoystick(&phi, &r, &intensity))
        {
            int32_t x, y;
            getTouchCartesian(phi, r, &x, &y);

            if (sd->touchMode == TM_PITCH)
            {
                uint16_t pitch = (0x3FFF * y) / 1023;
                sd->localPitch = true;
                if (pitch != sd->pitch)
                {
                    sd->pitch = pitch;
                    for (uint8_t ch = 0; ch < 16; ch++)
                    {
                        midiPitchWheel(&sd->midiPlayer, ch, sd->pitch);
                    }
                }
            }
        }
        else if (sd->localPitch)
        {
            // Touchpad released after we set local pitch value
            sd->localPitch = false;
            sd->pitch      = 0x2000;
            for (uint8_t ch = 0; ch < 16; ch++)
            {
                midiPitchWheel(&sd->midiPlayer, ch, sd->pitch);
            }
        }
    }
    else if (sd->touchMode == TM_SCRUB)
    {
        // NYI
    }

    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        sd->lastButtonState = evt.state;

        if (evt.down && !sd->startupSeqComplete)
        {
            if (!sd->startupDrums)
            {
                midiNoteOff(&sd->midiPlayer, 0, sd->startupNote, 0x7f);
                sd->startupDrums = true;
                sd->startupNote  = ACOUSTIC_BASS_DRUM_OR_LOW_BASS_DRUM;
            }
            else
            {
                midiNoteOff(&sd->midiPlayer, 0, sd->startupNote, 0x7f);
                sd->startupSeqComplete = true;
            }
        }
        else if (sd->startupSeqComplete)
        {
            if (isWheelMenuActive)
            {
                sd->menu = wheelMenuButton(sd->menu, sd->wheelMenu, &evt);
            }
            else
            {
                switch (sd->screen)
                {
                    case SS_VIEW:
                    {
                        if (evt.down && evt.button == PB_START)
                        {
                            sd->screen = SS_MENU;
                            synthSetupMenu(false);
                        }
                        synthHandleButton(evt);
                        break;
                    }

                    case SS_MENU:
                    {
                        if (evt.down && (evt.button == PB_START || (!sd->menu->parentMenu && evt.button == PB_B)))
                        {
                            sd->screen = SS_VIEW;
                        }
                        else
                        {
                            sd->menu = menuButton(sd->menu, evt);
                        }
                        break;
                    }

                    case SS_FILE_SELECT:
                    {
                        if (evt.down)
                        {
                            textEntryInput(evt.down, evt.button);
                        }
                    }
                }
            }
        }
    }

    if (sd->screen == SS_VIEW)
    {
        if (sd->upHeld)
        {
            handleButtonTimer(&sd->upHeldTimer, 100000, elapsedUs, PB_UP);
        }

        if (sd->downHeld)
        {
            handleButtonTimer(&sd->downHeldTimer, 100000, elapsedUs, PB_DOWN);
        }

        if (sd->leftHeld)
        {
            handleButtonTimer(&sd->leftHeldTimer, 1500000, elapsedUs, PB_LEFT);
        }

        if (sd->rightHeld)
        {
            handleButtonTimer(&sd->rightHeldTimer, 100000, elapsedUs, PB_RIGHT);
        }
    }
}

static bool synthIsControlSupported(const midiControllerDesc_t* control)
{
    switch (control->control)
    {
        case 64: // Hold Pedal
        case 66: // Sustenuto Pedal
        case 72: // Release Time
        case 73: // Attack Time
        // TODO: case 91: // Effects (Reverb)
        case 93: // Chorus level
        case 120: // All sound off
        case 121: // All controllers off
        case 123: // All notes off
            return true;

        default:
            return false;
    }
}

static void writeShortName(char* out, size_t n, const char* in)
{
    int written = 0;
    const char* cur = in;
    bool firstChar = true;

    while (written < n - 1 && *cur)
    {
        if (firstChar && isalnum(*cur))
        {
            out[written++] = *cur;
            firstChar = false;
        }
        else if (!isalnum(*cur))
        {
            firstChar = true;
        }
        cur++;
    }

    out[written] = '\0';
}

static paletteColor_t noteToColor(uint8_t note)
{
    return noteColors[note % ARRAY_SIZE(noteColors)];
}

static void drawChannelInfo(const midiPlayer_t* player, uint8_t chIdx, int16_t x, int16_t y, int16_t width,
                            int16_t height)
{
    const midiChannel_t* chan   = &player->channels[chIdx];
    const midiVoice_t* voices   = chan->percussion ? player->percVoices : player->poolVoices;
    const voiceStates_t* states = chan->percussion ? &player->percVoiceStates : &player->poolVoiceStates;
    uint8_t voiceCount          = chan->percussion ? PERCUSSION_VOICES : __builtin_popcount(chan->allocedVoices);

    // ok here's the plan
    // we're gonna draw a little bar graph for each voice
    // the bar extends vertically
    // then we draw the note name over top
    // the bar measures volume

#define BAR_SPACING ((voiceCount > 16) ? 0 : 1)
#define BAR_WIDTH   (MAX((width - BAR_SPACING * (voiceCount - 1)) / voiceCount, 1))

#define BAR_HEIGHT (height)

    int i              = 0;
    uint32_t voiceBits = chan->allocedVoices;
    while (voiceBits)
    {
        uint8_t voiceIdx = __builtin_ctz(voiceBits);
        int16_t x0       = x + ((chan->percussion ? voiceIdx : i++) * (BAR_WIDTH + BAR_SPACING));
        int16_t x1       = x0 + BAR_WIDTH;

        if (chan->percussion || voices[voiceIdx].oscillators[0].cVol > 0 || voices[voiceIdx].oscillators[0].tVol > 0)
        {
            int16_t barH = MAX((chan->percussion ? (voices[voiceIdx].velocity << 1 | 1) : voices[voiceIdx].oscillators[0].cVol) * BAR_HEIGHT / 255, 1);

            fillDisplayArea(x0, y + (BAR_HEIGHT - barH), x1, y + BAR_HEIGHT, noteToColor(voices[voiceIdx].note));
        }

        voiceBits &= ~(1 << voiceIdx);
    }
}

static void drawSampleGraph(void)
{
    // Draw sample graph
    SETUP_FOR_TURBO();

    for (int n = 0; n < 256; n++)
    {
        int16_t x = (TFT_WIDTH - 256) / 2 + n;
        int16_t y = TFT_HEIGHT / 2 + ((int16_t)sd->lastSamples[n] - 128) * ((TFT_HEIGHT - 16) / 2) / 128;

        // Top line
        TURBO_SET_PIXEL(x, (TFT_HEIGHT / 2) - (TFT_HEIGHT - 16) / 2, c500);

        // Mid-top line
        TURBO_SET_PIXEL(x, (TFT_HEIGHT / 2) - (TFT_HEIGHT - 16) / 4, c050);

        // Mid-bottom line
        TURBO_SET_PIXEL(x, (TFT_HEIGHT / 2) + (TFT_HEIGHT - 16) / 4, c050);

        // Bottom line
        TURBO_SET_PIXEL(x, (TFT_HEIGHT / 2) + (TFT_HEIGHT - 16) / 2, c500);

        // int16_t y = ((TFT_HEIGHT / 2) + (TFT_HEIGHT - 16) / 2) - (sd->lastSamples[n] * ((TFT_HEIGHT - 16) / 2) /
        // 255);
        TURBO_SET_PIXEL(x, y, c555);
    }
}

static void drawSampleGraphCircular(void)
{
    int16_t radiusMax = MIN(TFT_WIDTH, TFT_HEIGHT) / 2 - 10;
    int16_t radiusMin = 0;

    int16_t radiusAvg = (radiusMax - radiusMin) / 2;

    int16_t lastX = -1;
    int16_t lastY = -1;

    for (int n = 0; n < sd->sampleCount; n++)
    {
        int16_t sample     = (int16_t)sd->lastSamples[n] - 128;
        paletteColor_t col = paletteHsvToHex(sd->graphColor++, 255, 255);

        int16_t x
            = (TFT_WIDTH / 2) + getCos1024((n + sd->graphOffset) % 360) * (radiusAvg + sample * radiusAvg / 127) / 1024;
        int16_t y = (TFT_HEIGHT / 2)
                    - getSin1024((n + sd->graphOffset) % 360) * (radiusAvg + sample * radiusAvg / 127) / 1024;

        if (lastX != -1 && lastY != -1)
        {
            drawLineFast(lastX, lastY, x, y, col);
        }
        lastX = x;
        lastY = y;
    }

    sd->graphOffset = (sd->graphOffset + sd->sampleCount) % 360;
}

/**
 * @brief Writes the given MIDI text to the destination buffer, without adding a NUL-terminator byte
 *
 * This also strips any characters that the swadge cannot print
 *
 * @param dest The buffer to write to
 * @param n The maximum number of bytes to write to the buffer
 * @param text A pointer to a struct containing text info
 * @param kar  True if .KAR formatting should be handled
 * @return int The number of bytes written to dest
 */
static int writeMidiText(char* dest, size_t n, midiTextInfo_t* text, bool kar)
{
    char* p            = dest;
    const char* bufEnd = dest + n;

    const char* s      = text->text;
    const char* srcEnd = s + text->length;

    while (p < bufEnd && s < srcEnd)
    {
        bool skip = false;
        if (kar && (s == text->text || s == srcEnd - 1))
        {
            // First or last char -- handle the special chars
            if (*s == '\\' || *s == '/')
            {
                // Line break or paragraph break
                *p++ = '\n';

                if (p >= bufEnd)
                {
                    break;
                }
                skip = true;
            }
            if (*s == '/')
            {
                // Paragraph break
                *p++ = '\n';

                if (p >= bufEnd)
                {
                    break;
                }
                skip = true;
            }
            if (*s == '-')
            {
                // Skip it
                skip = true;
            }
        }

        if (!skip && *s >= ' ' && *s <= '~')
        {
            *p++ = *s;
        }

        s++;
    }

    *p = 0;

    return p - dest;
}

static void midiTextCallback(metaEventType_t type, const char* text, uint32_t length)
{
    if (!text || !length)
    {
        return;
    }

    midiTextInfo_t* info = (midiTextInfo_t*)malloc(sizeof(midiTextInfo_t));

    if (info)
    {
        info->text   = text;
        info->length = length;
        info->type   = type;
        // make it last for 2 seconds
        info->expiration = esp_timer_get_time() + 5000000;

        push(&sd->midiTexts, info);
    }
}

static void synthMenuCb(const char* label, bool selected, uint32_t value)
{
    printf("synthMenuCb(%s, %s, %" PRIu32 ")\n", label, selected ? "true" : "false", value);
    if (NULL == label)
    {
        if (selected)
        {
            // Wheel Menu Closed! Reset the menu
            sd->updateMenu = true;
            //sd->forceResetMenu = true;
        }
    }
    else if (label == mnuBackStr)
    {
        if (selected && !sd->menu->parentMenu)
        {
            sd->updateMenu = true;
            sd->forceResetMenu = true;
            sd->screen = SS_VIEW;
        }
    }
    else if (label == menuItemPlayMode)
    {
        if (value != sd->fileMode)
        {
            if (value)
            {
                sd->fileMode = true;

                // fileMode
                if (sd->filename != NULL && *sd->filename)
                {
                    synthSetFile(sd->filename);
                    if (sd->fileMode)
                    {
                        // Loading was successful
                        if (sd->filenameBuf)
                        {
                            free(sd->filenameBuf);
                            sd->filenameBuf = NULL;
                        }

                        sd->filename   = label;
                        sd->customFile = false;
                        writeNvs32(nvsKeyMode, sd->fileMode);
                    }
                }
                else
                {
                    // Open the file select menu
                    sd->menu = menuNavigateToItem(sd->menu, menuItemSelectFile);
                    sd->menu = menuSelectCurrentItem(sd->menu);
                }
            }
            else
            {
                sd->fileMode = false;

                // not file mode
                synthSetFile(NULL);
                midiPlayerReset(&sd->midiPlayer);

                synthSetupPlayer();

                writeNvs32(nvsKeyMode, sd->fileMode);
            }
        }
    }
    else if (label == menuItemViewMode)
    {
        if (value != sd->viewMode)
        {
            writeNvs32(nvsKeyViewMode, value);
        }

        sd->viewMode = (synthViewMode_t)value;
        if (selected)
        {
            sd->screen = SS_VIEW;
        }
    }
    else if (label == menuItemButtonMode)
    {
        if (value != sd->buttonMode)
        {
            writeNvs32(nvsKeyButtonMode, value);
        }

        sd->buttonMode = (synthButtonMode_t)value;
        if (selected)
        {
            sd->screen = SS_VIEW;
        }
    }
    else if (label == menuItemTouchMode)
    {
        if (value != sd->touchMode)
        {
            writeNvs32(nvsKeyTouchMode, value);
        }

        sd->touchMode = (synthTouchMode_t)value;
        if (selected)
        {
            sd->screen = SS_VIEW;
        }
    }
    else if (label == menuItemLoop)
    {
        if (value != sd->loop)
        {
            writeNvs32(nvsKeyLoop, value);
        }
        if (selected)
        {
            sd->screen = SS_VIEW;
        }

        sd->loop            = value ? true : false;
        sd->midiPlayer.loop = sd->loop;
    }
    else if (label == menuItemShuffle)
    {
        if (value != sd->shuffle)
        {
            writeNvs32(nvsKeyShuffle, value);
        }
        if (selected)
        {
            sd->screen = SS_VIEW;
        }
        sd->shuffle = value ? true : false;
    }
    else if (label == menuItemAutoplay)
    {
        if (value != sd->autoplay)
        {
            writeNvs32(nvsKeyAutoplay, value);
        }
        if (selected)
        {
            sd->screen = SS_VIEW;
        }
        sd->autoplay = value ? true : false;
    }
    else if (label == menuItemCustomSong)
    {
        if (selected)
        {
            sd->screen = SS_FILE_SELECT;
            if (!sd->filenameBuf)
            {
                sd->filenameBuf = calloc(1, 128);
            }

            if (!sd->filenameBuf)
            {
                ESP_LOGE("Synth", "Could not allocate memory for text buffer!");
            }
            else
            {
                textEntryInit(&sd->font, 128, sd->filenameBuf);
                textEntrySetBGTransparent();
                textEntrySetShadowboxColor(true, c000);
            }
        }
    }
    else if (label == menuItemIgnore)
    {
        if (value)
        {
            sd->synthConfig.ignoreChannelMask |= (1 << sd->menuSelectedChannel);
        }
        else
        {
            sd->synthConfig.ignoreChannelMask &= ~(1 << sd->menuSelectedChannel);
        }
        synthApplyConfig();
    }
    else if (label == menuItemPercussion)
    {
        if (value != ((sd->synthConfig.percChannelMask & (1 << sd->menuSelectedChannel)) ? 1 : 0))
        {
            if (value)
            {
                sd->synthConfig.percChannelMask |= (1 << sd->menuSelectedChannel);
            }
            else
            {
                sd->synthConfig.percChannelMask &= ~(1 << sd->menuSelectedChannel);
            }
            synthApplyConfig();

            // Update the menu because percussion has different items
            sd->updateMenu = true;
        }
    }
    else if (label == menuItemBank)
    {
        if (value != sd->synthConfig.banks[sd->menuSelectedChannel])
        {
            sd->synthConfig.banks[sd->menuSelectedChannel] = value;
            sd->synthConfig.programs[sd->menuSelectedChannel] = 0;
            synthApplyConfig();

            // Update the menu because different banks have different instruments
            sd->updateMenu = true;
        }
    }
    else if (label == menuItemReset)
    {
        if (selected)
        {
            sd->synthConfig.banks[sd->menuSelectedChannel] = 0;
            sd->synthConfig.programs[sd->menuSelectedChannel] = 0;
            sd->synthConfig.ignoreChannelMask &= ~(1 << sd->menuSelectedChannel);
            if (sd->menuSelectedChannel == 9)
            {
                sd->synthConfig.percChannelMask |= (1 << sd->menuSelectedChannel);
            }
            else
            {
                sd->synthConfig.percChannelMask &= ~(1 << sd->menuSelectedChannel);
            }
            for (node_t* node = sd->controllerSettings.first; node != NULL; node = node->next)
            {
                synthControlConfig_t* config = (synthControlConfig_t*)node->val;
                if (config)
                {
                    config->chanMask &= ~(1 << sd->menuSelectedChannel);
                }
            }
            synthApplyConfig();
            sd->updateMenu = true;
        }
    }
    else if (label == menuItemResetAll)
    {
        if (selected)
        {
            memcpy(&sd->synthConfig, &defaultSynthConfig, sizeof(synthConfig_t));
            synthControlConfig_t* control;
            while (NULL != (control = pop(&sd->controllerSettings)))
            {
                free(control);
            }
            synthApplyConfig();
            sd->updateMenu = true;
        }
    }
    else if (label == menuItemHeadroom
             || (menuItemHeadroomOptions <= label
                 && label <= (menuItemHeadroomOptions + ARRAY_SIZE(menuItemHeadroomOptions))))
    {
        sd->headroom            = value;
        sd->midiPlayer.headroom = sd->headroom;
        writeNvs32(nvsKeyHeadroom, value);
    }
    else
    {
        // Mapped menu items
        midiMenuItemInfo_t* itemInfo = (midiMenuItemInfo_t*)hashGet(&sd->menuMap, label);

        if (itemInfo != NULL)
        {
            printf("Found item from map for %s\n", label);
            switch (itemInfo->type)
            {
                case SMT_PROGRAM:
                {
                    if (selected)
                    {
                        sd->synthConfig.banks[sd->menuSelectedChannel] = itemInfo->bank;
                        sd->synthConfig.programs[sd->menuSelectedChannel] = itemInfo->program;
                        char* nameBuffer = sd->channelInstrumentLabels[sd->menuSelectedChannel];
                        snprintf(nameBuffer, 64, "%s%s", menuItemInstrument, label);
                        synthApplyConfig();
                        sd->updateMenu = true;
                    }
                    break;
                }

                case SMT_CHANNEL:
                {
                    sd->menuSelectedChannel = itemInfo->channel;
                    break;
                }

                case SMT_CONTROLLER:
                {
                    const midiControllerDesc_t* desc = itemInfo->controller;
                    bool saveControl = true;

                    if (desc->type == CTRL_NO_DATA)
                    {
                        saveControl = false;
                        if (selected)
                        {
                            midiControlChange(&sd->midiPlayer, sd->menuSelectedChannel, desc->control, 0);
                        }
                    }
                    else
                    {
                        if (desc->type == CTRL_SWITCH)
                        {
                            midiControlChange(&sd->midiPlayer, sd->menuSelectedChannel, desc->control, BOOL_TO_MIDI(value));
                        }
                        else if (desc->type == CTRL_CC_LSB || desc->type == CTRL_CC_MSB)
                        {
                            // Most of the MSB/LSB controllers are in the 0-63 range, with MSB in 0-15 and LSB in 32-47
                            uint8_t msbControl = desc->control & ~(1 << 5);
                            uint8_t lsbControl = desc->control | (1 << 5);

                            if (desc->control > 64)
                            {
                                // There are a couple other MSB/LSB controllers, and those have an odd MSB and an even LSB
                                msbControl = desc->control | 1;
                                lsbControl = desc->control & ~1;
                            }

                            midiControlChange(&sd->midiPlayer, sd->menuSelectedChannel, msbControl, (value >> 7) & 0x7F);
                            midiControlChange(&sd->midiPlayer, sd->menuSelectedChannel, lsbControl, value & 0x7F);
                        }
                        else if (desc->type == CTRL_7BIT)
                        {
                            midiControlChange(&sd->midiPlayer, sd->menuSelectedChannel, desc->control, value & 0x7F);
                        }
                    }

                    if (saveControl)
                    {
                        synthSaveControl(sd->menuSelectedChannel, desc->control, value);
                    }
                    break;
                }
            }
        }
        else if (selected)
        {
            printf("No map item found for label %s\n", label);
            // Song items
            for (node_t* node = sd->customFiles.first; node != NULL; node = node->next)
            {
                char* str = (char*)node->val;
                if (label == str)
                {
                    synthSetFile(label);

                    if (sd->fileMode)
                    {
                        sd->filename = label;
                        writeNvs32(nvsKeyMode, (int32_t)sd->fileMode);
                    }
                }
            }
        }
    }
}

static void songEndCb(void)
{
    if (sd->loop)
    {
        midiSetFile(&sd->midiPlayer, &sd->midiFile);
        midiPause(&sd->midiPlayer, false);
    }
    else
    {
        if (sd->autoplay)
        {
            nextSong();
            midiPause(&sd->midiPlayer, false);
        }
        else
        {
            sd->stopped = true;
        }
    }
}

static void setupShuffle(int numSongs)
{
    loadLfsr(&sd->shuffleState);
    sd->shufflePos = lfsrCur(&sd->shuffleState);
}

static void prevSong(void)
{
    const char* pickedSong = NULL;

    if (sd->shuffle)
    {
        // Get the number of bits needed for the current number of songs, minimumn 2
        uint32_t pos = lfsrPrev(&sd->shuffleState);

        sd->shufflePos = (int32_t)pos;
        saveLfsr(&sd->shuffleState);
        writeNvs32(nvsKeyShufflePos, sd->shufflePos);

        node_t* node = sd->customFiles.first;
        for (int i = 0; i < sd->shufflePos; i++)
        {
            if (!node)
            {
                break;
            }

            node = node->next;
        }

        if (node && node->val)
        {
            pickedSong = (const char*)node->val;
            synthSetFile(pickedSong);
        }
    }
    else
    {
        if (sd->filename)
        {
            for (node_t* node = sd->customFiles.first; node != NULL; node = node->next)
            {
                if (!strcmp((const char*)node->val, sd->filename))
                {
                    if (node->prev && node->prev->val)
                    {

                        pickedSong = (const char*)node->prev->val;
                        synthSetFile(pickedSong);
                    }
                    else if (sd->loop && sd->customFiles.last)
                    {
                        if (sd->customFiles.last->val)
                        {
                            pickedSong = (const char*)sd->customFiles.last->val;
                            synthSetFile(pickedSong);
                        }
                    }

                    break;
                }
            }
        }
    }

    if (sd->fileMode && pickedSong)
    {
        sd->filename = pickedSong;
        writeNvs32(nvsKeyMode, (int32_t)sd->fileMode);
        synthSetupPlayer();
    }
}

static void nextSong(void)
{
    const char* pickedSong = NULL;

    if (sd->shuffle)
    {
        // Get the number of bits needed for the current number of songs, minimumn 2
        uint32_t pos = lfsrNext(&sd->shuffleState);
        sd->shufflePos = (int32_t)pos;
        saveLfsr(&sd->shuffleState);
        writeNvs32(nvsKeyShufflePos, sd->shufflePos);

        node_t* node = sd->customFiles.first;
        for (int i = 0; i < sd->shufflePos; i++)
        {
            if (!node)
            {
                break;
            }

            node = node->next;
        }

        if (node && node->val)
        {
            pickedSong = (const char*)node->val;
            synthSetFile(pickedSong);
        }
    }
    else
    {
        if (sd->filename)
        {
            for (node_t* node = sd->customFiles.first; node != NULL; node = node->next)
            {
                if (!strcmp((const char*)node->val, sd->filename))
                {
                    if (node->next && node->next->val)
                    {

                        pickedSong = (const char*)node->next->val;
                        synthSetFile(pickedSong);

                    }
                    else if (sd->loop && sd->customFiles.first)
                    {
                        if (sd->customFiles.first->val)
                        {
                            pickedSong = (const char*)sd->customFiles.first->val;
                            synthSetFile(pickedSong);
                        }
                    }

                    break;
                }
            }
        }
    }

    if (sd->fileMode && pickedSong)
    {
        sd->filename = pickedSong;
        writeNvs32(nvsKeyMode, (int32_t)sd->fileMode);
        synthSetupPlayer();
    }
}

static void saveLfsr(const lfsrState_t* state)
{
    writeNvs32("lfsrState", (int32_t)state->state);
    writeNvs32("lfsrRange", (int32_t)state->range);
    writeNvs32("lfsrRev", state->reversed);
}

static void loadLfsr(lfsrState_t* state)
{
    int32_t nvsRead;

    if (!readNvs32("lfsrRange", &nvsRead))
    {
        nvsRead = sd->customFiles.length;
    }
    state->range = (uint32_t)nvsRead;

    if (!readNvs32("lfsrRev", &nvsRead))
    {
        nvsRead = 1;
    }
    state->reversed = nvsRead ? true : false;

    if (!readNvs32("lfsrState", &nvsRead))
    {
        nvsRead = (int32_t)(state->range - 1);
    }
    state->state = (uint32_t)nvsRead;
}

static uint32_t setupLfsr(lfsrState_t* state, uint32_t range)
{
    state->state = range - 1;
    state->range = range;
    state->reversed = false;

    return lfsrNext(state);
}

static uint32_t flipToMsb(uint32_t val)
{
    if (!val)
    {
        // can't clz(0)
        return 0;
    }

    uint8_t bits = 32 - __builtin_clz(val);
    uint32_t result = 1 << (bits - 1);

    for (int i = 0; i < bits - 1; i++)
    {
        if (val & (1 << i))
        {
            result |= 1 << (bits - i - 2);
        }
    }

    return result;
}

static uint32_t bitRevert(uint32_t v, uint8_t bits)
{
    uint8_t i = 0;
    uint32_t lsb = 0;
    uint32_t n = 0;

    for (i = 0; i < bits; i++)
    {
        lsb = v & 1;
        v >>= 1;
        n <<= 1;
        n |= lsb;
    }

    return n;
}

static uint32_t lfsrNext(lfsrState_t* state)
{
    uint16_t bits = 32 - __builtin_clz((state->range + 1) | 0x2);
    uint16_t val = state->state & 0xFFFFu;

    if (state->reversed)
    {
        state->reversed = false;
        val = bitRevert(state->state, bits);
    }

    do {
        uint16_t bit = 0;
        for (int bitc = 0; bitc < bits; bitc++)
        {
            if (lfsrTaps[bits-2] & (1 << bitc))
            {
                bit ^= (val >> (bits - bitc - 1));
            }
        }

        bit &= 1;
        val = (val >> 1) | (bit << (bits-1));
    } while (val > state->range);

    state->state = val;

    return val - 1;
}

static uint32_t lfsrPrev(lfsrState_t* state)
{
    uint16_t bits = 32 - __builtin_clz((state->range + 1) | 0x2);
    uint16_t val = state->state & 0xFFFFu;
    uint32_t taps = flipToMsb(lfsrTaps[bits-2]);

    if (!state->reversed)
    {
        state->reversed = true;
        val = bitRevert(state->state, bits);
    }

    do {
        uint16_t bit = 0;
        for (int bitc = 0; bitc < bits; bitc++)
        {
            if ((taps) & (1 << bitc))
            {
                bit ^= (val >> (bits - bitc - 1));
            }
        }

        bit &= 1;
        val = (val >> 1) | (bit << (bits-1));
    } while (bitRevert(val, bits) > state->range);

    state->state = val;

    return bitRevert(val, bits) - 1;
}

static uint32_t lfsrCur(const lfsrState_t* state)
{
    uint16_t bits = 32 - __builtin_clz((state->range + 1) | 0x2);
    return (state->reversed ? bitRevert(state->state, bits) : state->state) - 1;
}