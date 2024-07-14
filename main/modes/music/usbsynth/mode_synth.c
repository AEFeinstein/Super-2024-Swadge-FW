#include <stddef.h>
#include <stdlib.h>

#include "esp_log.h"
#include "esp_timer.h"

#include "swadge2024.h"
#include "spiffs_font.h"
#include "hdw-btn.h"
#include "touchUtils.h"
#include "shapes.h"
#include "trigonometry.h"
#include "linked_list.h"
#include "wheel_menu.h"
#include "hdw-nvs.h"
#include "textEntry.h"
#include "macros.h"

#include "midiPlayer.h"
#include "midiFileParser.h"
#include "midiUsb.h"

//==============================================================================
// Defines
//==============================================================================

#define NUM_FRAME_TIMES 60

#define VIZ_SAMPLE_COUNT 512

#define TEMPO_TO_BPM(t) (60000000 / (t))
// math is wild yo
#define BPM_TO_TEMPO(b) TEMPO_TO_BPM(b)

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
    const char* text;
    uint32_t length;
    metaEventType_t type;
    uint64_t expiration;
} midiTextInfo_t;

typedef struct
{
    font_t font;
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

        int64_t upHeldTimer;
        int64_t downHeldTimer;

        uint16_t lastButtonState;
    };

    synthScreen_t screen;
    synthViewMode_t viewMode;
    synthButtonMode_t buttonMode;
    synthTouchMode_t touchMode;
    bool loop;
    bool stopped;
    int32_t headroom;

    wsg_t instrumentImages[16];
    wsg_t percussionImage;

    uint32_t frameTimesIdx;
    uint64_t frameTimes[NUM_FRAME_TIMES];

    list_t midiTexts;
    uint64_t nextExpiry;

    list_t customFiles;

    menu_t* menu;
    menuManiaRenderer_t* renderer;
    wheelMenuRenderer_t* wheelMenu;
    rectangle_t wheelTextArea;
} synthData_t;

//==============================================================================
// Function Prototypes
//==============================================================================

static void synthEnterMode(void);
static void synthExitMode(void);
static void synthMainLoop(int64_t elapsedUs);
static void synthDacCallback(uint8_t* samples, int16_t len);

static void synthSetupPlayer(void);
static void synthSetupMenu(void);
static void synthSetFile(const char* filename);
static void synthHandleButton(const buttonEvt_t evt);
static void handleButtonTimer(int64_t* timer, int64_t interval, int64_t elapsedUs, buttonBit_t button);
static void synthHandleInput(int64_t elapsedUs);

static void drawCircleSweep(int x, int y, int r, int startAngle, int sweepDeg, paletteColor_t col);
static void drawIcon(musicIcon_t icon, paletteColor_t col, int16_t x, int16_t y, int16_t w, int16_t h);
static void drawSynthMode(void);
static void drawMidiText(void);
static void drawPitchWheelRect(uint8_t chIdx, int16_t x, int16_t y, int16_t w, int16_t h);
static void drawChannelInfo(const midiPlayer_t* player, uint8_t chIdx, int16_t x, int16_t y, int16_t width,
                            int16_t height);
static void drawSampleGraph(void);
static void drawSampleGraphCircular(void);
static int writeMidiText(char* dest, size_t n, midiTextInfo_t* text);
static void midiTextCallback(metaEventType_t type, const char* text, uint32_t length);
static void synthMenuCb(const char* label, bool selected, uint32_t value);
static void songEndCb(void);

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

// Menu stuff
static const char* menuItemPlayMode   = "MIDI Mode: ";
static const char* menuItemSelectFile = "Select File...";
static const char* menuItemCustomSong = "Enter Filename...";
static const char* menuItemViewMode   = "View Mode: ";
static const char* menuItemButtonMode = "Button Controls: ";
static const char* menuItemTouchMode  = "Touchpad Controls: ";
static const char* menuItemLoop       = "Loop: ";
static const char* menuItemHeadroom   = "Mix Volume: ";

static const char* const nvsKeyMode       = "synth_playmode";
static const char* const nvsKeyViewMode   = "synth_viewmode";
static const char* const nvsKeyButtonMode = "synth_btnmode";
static const char* const nvsKeyTouchMode  = "synth_touchmode";
static const char* const nvsKeyLoop       = "synth_loop";
static const char* const nvsKeyLastSong   = "synth_lastsong";
static const char* const nvsKeyHeadroom   = "synth_headdroom";

static const char* menuItemModeOptions[] = {
    "MIDI Streaming",
    "MIDI File",
};

static const char* menuItemViewOptions[] = {
    "Pretty", "Visualizer", "Waveform", "Waveform+Table", "Waveform+Packets", "Table", "Packets",
};

static const char* menuItemButtonOptions[] = {
    "Play Note",
    "Playback",
};

static const char* menuItemTouchOptions[] = {
    "Pitch Bend",
    "Scrub",
};

static const char* menuItemLoopOptions[] = {
    "Off",
    "On",
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
    (int32_t)VM_GRAPH,
    (int32_t)(VM_GRAPH | VM_TEXT),
    (int32_t)(VM_GRAPH | VM_PACKETS),
    (int32_t)VM_TEXT,
    (int32_t)VM_PACKETS,
};

static const int32_t menuItemButtonValues[] = {
    (int32_t)BM_NOTE,
    (int32_t)BM_PLAYBACK,
};

static const int32_t menuItemTouchValues[] = {
    (int32_t)TM_PITCH,
    (int32_t)TM_SCRUB,
};

static const int32_t menuItemLoopValues[] = {
    0,
    1,
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
    .max = (VM_VIZ << 1) - 1,
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

static settingParam_t menuItemHeadroomBounds = {
    .def = MIDI_DEF_HEADROOM,
    .min = 0,
    .max = 0x8000,
    .key = nvsKeyHeadroom,
};

const char synthModeName[] = "MIDI Player";

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

static synthData_t* sd;

// Functions

static void synthEnterMode(void)
{
    sd = calloc(1, sizeof(synthData_t));
    loadFont("ibm_vga8.font", &sd->font, true);
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

    if (!readNvs32(nvsKeyHeadroom, &nvsRead))
    {
        nvsRead = MIDI_DEF_HEADROOM;
    }
    sd->headroom            = nvsRead;
    sd->midiPlayer.headroom = sd->headroom;

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

    sd->menu = initMenu(synthModeName, synthMenuCb);
    // Use smol font for men items, there might be a lot
    sd->renderer = initMenuManiaRenderer(NULL, NULL, &sd->font);

    sd->wheelTextArea.pos.x  = 15;
    sd->wheelTextArea.pos.y  = TFT_HEIGHT - sd->font.height * 2 - 2 - 15;
    sd->wheelTextArea.width  = TFT_WIDTH - 30;
    sd->wheelTextArea.height = sd->font.height * 2 + 2;
    sd->wheelMenu            = initWheelMenu(&sd->font, 90, &sd->wheelTextArea);

    synthSetupMenu();
    synthSetupPlayer();

    sd->startupSeqComplete = true;
    sd->startupNote        = 60;

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
    loadWsg("percussion.wsg", &sd->percussionImage, true);

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

    freeWsg(&sd->percussionImage);
    for (int i = 0; i < 16; i++)
    {
        freeWsg(&sd->instrumentImages[i]);
    }

    deinitWheelMenu(sd->wheelMenu);
    deinitMenuManiaRenderer(sd->renderer);
    deinitMenu(sd->menu);

    unloadMidiFile(&sd->midiFile);
    midiPlayerReset(&sd->midiPlayer);

    // Unload the filename if it was dynamic
    if (sd->filenameBuf)
    {
        free(sd->filenameBuf);
        sd->filenameBuf = NULL;
    }

    char* customFilename;
    while (NULL != (customFilename = pop(&sd->customFiles)))
    {
        free(customFilename);
    }

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
            drawTriangleOutlined(x, y, x + w, y + h / 2, x, y + h, col, col);
            break;
        }
        case MI_PAUSE:
        {
            // Two vertical bars
            fillDisplayArea(x + (w / 8), y, x + (w / 4), y + h, col);
            fillDisplayArea(x + w - (w / 4), y, x + w - (w / 8), y + h, col);
            break;
        }
        case MI_STOP:
        {
            // Square
            fillDisplayArea(x, y, x + w, y + h, col);
            break;
        }
        case MI_PLAYPAUSE:
        {
            // Vertical bar
            fillDisplayArea(x + (w / 8), y, x + (w / 4), y + h, col);
            // Triangle
            drawTriangleOutlined(x + w - (w / 4), y, x + w, y + h / 2, x + w - (w / 4), y + h, col, col);
            break;
        }
        case MI_FFW:
        {
            // Two triangles
            fillDisplayArea(x + (w / 8), y, x + (w / 4), y + h, col);
            // Two triangles (TODO)
            break;
        }
        case MI_REW:
        {
            // Reverse of FFW
            break;
        }
        case MI_SKIP:
        {
            // Triangle left
            drawTriangleOutlined(x, y, x + w - (w / 4), y + h / 2, x, y + h, col, col);
            // Vertical bar right
            fillDisplayArea(x + w - (w / 8), y, x + w - (w / 4), y + h, col);
            break;
        }
        case MI_PREV:
        {
            // Vertical bar left
            fillDisplayArea(x + (w / 8), y, x + (w / 4), y + h, col);
            // Trangle right (facing left)
            drawTriangleOutlined(x, y + h / 2, x + w - (w / 4), y, x + w - (w / 4), y + h, col, col);
            break;
        }
        case MI_REPEAT:
        {
            // 3/4 of a circle, then we add an arrow at the end
            drawCircleSweep(x + w / 2, y + h - h / 2, ((w < h) ? w : h) / 2, 45, 270, col);
            // Kinda funky but close
            drawTriangleOutlined(x + w / 2 + w / 4, y + h / 4, x + w, y, x + w, y + h / 2, col, col);

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
            int crossOffset = (w / 6);
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
            drawLineFast(cr, yTop, x + w, yTop, col);
            break;
        }
    }
}

static void synthMainLoop(int64_t elapsedUs)
{
    if (sd->screen == SS_MENU)
    {
        drawMenuMania(sd->menu, sd->renderer, elapsedUs);
    }
    else if (sd->screen == SS_FILE_SELECT)
    {
        clearPxTft();
        drawSynthMode();

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
        clearPxTft();

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

        drawSynthMode();
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
    if (sd->fileMode)
    {
        sd->midiPlayer.loop                = sd->loop;
        sd->midiPlayer.textMessageCallback = midiTextCallback;
    }
    else
    {
        if (!sd->installed)
        {
            sd->installed = installMidiUsb();
        }
        sd->midiPlayer.streamingCallback = usbMidiCallback;
        sd->midiPlayer.mode              = MIDI_STREAMING;
    }

    sd->midiPlayer.headroom = sd->headroom;
}

static void synthSetupMenu(void)
{
    addSettingsOptionsItemToMenu(sd->menu, menuItemPlayMode, menuItemModeOptions, menuItemModeValues, 2,
                                 &menuItemModeBounds, sd->fileMode);

    sd->menu = startSubMenu(sd->menu, menuItemSelectFile);
    addSingleItemToMenu(sd->menu, menuItemCustomSong);
    void* dirh = NULL;
    char outbuf[128];
    while (spiffsListFiles(outbuf, sizeof(outbuf), &dirh) > 0)
    {
        if ((strlen(outbuf) > 4
             && (!strcmp(&outbuf[strlen(outbuf) - 4], ".mid") || !strcmp(&outbuf[strlen(outbuf) - 4], ".kar")))
            || (strlen(outbuf) > 7 && !strcmp(&outbuf[strlen(outbuf)] - 5, ".midi")))
        {
            char* copyStr = strdup(outbuf);
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
    for (node_t* node = sd->customFiles.first; node != NULL; node = node->next)
    {
        addSingleItemToMenu(sd->menu, (char*)node->val);
    }

    sd->menu = endSubMenu(sd->menu);

    addSettingsOptionsItemToMenu(sd->menu, menuItemViewMode, menuItemViewOptions, menuItemViewValues,
                                 ARRAY_SIZE(menuItemViewValues), &menuItemViewBounds, sd->viewMode);
    addSettingsOptionsItemToMenu(sd->menu, menuItemButtonMode, menuItemButtonOptions, menuItemButtonValues,
                                 ARRAY_SIZE(menuItemButtonValues), &menuItemButtonBounds, sd->buttonMode);
    addSettingsOptionsItemToMenu(sd->menu, menuItemTouchMode, menuItemTouchOptions, menuItemTouchValues,
                                 ARRAY_SIZE(menuItemTouchValues), &menuItemTouchBounds, sd->touchMode);
    addSettingsOptionsItemToMenu(sd->menu, menuItemLoop, menuItemLoopOptions, menuItemLoopValues,
                                 ARRAY_SIZE(menuItemLoopValues), &menuItemLoopBounds, sd->loop);
    addSettingsOptionsItemToMenu(sd->menu, menuItemHeadroom, menuItemHeadroomOptions, menuItemHeadroomValues,
                                 ARRAY_SIZE(menuItemHeadroomValues), &menuItemHeadroomBounds, sd->headroom);
}

static void synthSetFile(const char* filename)
{
    // First: stop and reset the MIDI player
    midiPlayerReset(&sd->midiPlayer);

    // TODO: make applyPlayerSettings() function instead
    synthSetupPlayer();

    // Next: Free any text that might reference the song file still
    midiTextInfo_t* textInfo = NULL;
    while (NULL != (textInfo = pop(&sd->midiTexts)))
    {
        free(textInfo);
    }

    // Finally: Unload the MIDI file itself
    unloadMidiFile(&sd->midiFile);

    if (NULL != filename)
    {
        // Cleanup done, now load the new file
        if (loadMidiFile(filename, &sd->midiFile, true))
        {
            sd->fileMode = true;

            midiSetFile(&sd->midiPlayer, &sd->midiFile);

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

static void drawSynthMode(void)
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
        sd->playing[ch]
            = 0
              != (channel->allocedVoices
                  & (percussion ? (sd->midiPlayer.percVoiceStates.on || sd->midiPlayer.percVoiceStates.held)
                                : (sd->midiPlayer.poolVoiceStates.on | sd->midiPlayer.poolVoiceStates.held)));
        paletteColor_t col = sd->playing[ch] ? c555 : c222;

        if (sd->viewMode == VM_PRETTY)
        {
            wsg_t* image
                = percussion ? &sd->percussionImage : &sd->instrumentImages[sd->midiPlayer.channels[ch].program / 8];
            int16_t x    = ((ch % 8) + 1) * (TFT_WIDTH - image->w * 8) / 9 + image->w * (ch % 8);
            int16_t imgY = (ch < 8) ? 30 : TFT_HEIGHT - 30 - 32;
            drawWsgSimple(image, x, imgY);
            if (sd->playing[ch])
            {
                drawRect(x, imgY, x + 32, imgY + 32, col);
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
                                                 : "<Percussion>")
                                          : gmProgramNames[sd->midiPlayer.channels[ch].program];
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

    if (sd->viewMode == VM_PRETTY)
    {
        uint16_t pitch = sd->localPitch ? sd->pitch : sd->midiPlayer.channels[sd->localChannel].pitchBend;
        int16_t deg    = (360 + ((pitch - 0x2000) * 90 / 0x1FFF)) % 360;
        drawCircleQuadrants(0, TFT_HEIGHT / 2, 16, true, false, false, true, (pitch == 0x2000) ? c222 : c555);
        drawLineFast(0, TFT_HEIGHT / 2, 16 * getCos1024(deg) / 1024, TFT_HEIGHT / 2 - (16 * getSin1024(deg) / 1024),
                     c500);

        char tempoStr[16];
        snprintf(tempoStr, sizeof(tempoStr), "%" PRIu32 " BPM", TEMPO_TO_BPM(sd->midiPlayer.tempo));
        drawText(&sd->font, c500, tempoStr, TFT_WIDTH - textWidth(&sd->font, tempoStr) - 35, 3);

        drawMidiText();

#define ICON_SIZE (sd->font.height)

        int16_t iconPosR = TFT_WIDTH / 2;
        int16_t iconPosL = iconPosR - ICON_SIZE - 1;
        if (sd->fileMode && sd->filename && *sd->filename)
        {
            int16_t textX = (TFT_WIDTH - textWidth(&sd->font, sd->filename)) / 2;
            iconPosR      = drawText(&sd->font, c034, sd->filename, textX, TFT_HEIGHT - sd->font.height - 15);
            iconPosL      = textX - ICON_SIZE - 1;
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
}

static void drawMidiText(void)
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
        msgLen += writeMidiText(textMessages + msgLen, sizeof(textMessages) - msgLen - 1, curInfo);

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
    memcpy(sd->lastSamples, samples, MIN(len, VIZ_SAMPLE_COUNT));
    sd->sampleCount = MIN(len, VIZ_SAMPLE_COUNT);
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
                    // TODO
                    break;
                }

                case PB_RIGHT:
                {
                    // Seek Right
                    // TODO
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
                    break;
                }

                case PB_B:
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
    else if (sd->touchMode == TM_PITCH && sd->localPitch)
    {
        // Touchpad released after we set local pitch value
        sd->localPitch = false;
        sd->pitch      = 0x2000;
        for (uint8_t ch = 0; ch < 16; ch++)
        {
            midiPitchWheel(&sd->midiPlayer, ch, sd->pitch);
        }
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
            switch (sd->screen)
            {
                case SS_VIEW:
                {
                    if (evt.down && evt.button == PB_START)
                    {
                        sd->screen = SS_MENU;
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
    }
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

        if (voices[voiceIdx].targetVol > 0 && ((states->held | states->on) & (1 << voiceIdx)))
        {
            int16_t barH = MAX((voices[voiceIdx].targetVol) * BAR_HEIGHT / 255, 1);

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
 * @return int The number of bytes written to dest
 */
static int writeMidiText(char* dest, size_t n, midiTextInfo_t* text)
{
    char* p            = dest;
    const char* bufEnd = dest + n;

    const char* s      = text->text;
    const char* srcEnd = s + text->length;

    while (p < bufEnd && s < srcEnd)
    {
        if (*s >= ' ' && *s <= '~')
        {
            *p++ = *s;
        }

        s++;
    }

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
    if (label == menuItemPlayMode)
    {
        if (selected && value != sd->fileMode)
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

                if (!sd->installed)
                {
                    sd->installed = installMidiUsb();
                }

                synthSetupPlayer();
                midiPause(&sd->midiPlayer, false);

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
        // Song items
        if (selected)
        {
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
        sd->stopped = true;
    }
}