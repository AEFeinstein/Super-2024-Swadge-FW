#include <stddef.h>
#include <stdlib.h>

#include "esp_log.h"
#include "esp_timer.h"

#include "tinyusb.h"

#include "swadge2024.h"
#include "spiffs_font.h"
#include "hdw-btn.h"
#include "touchUtils.h"
#include "shapes.h"
#include "trigonometry.h"
#include "linked_list.h"

#include "sngPlayer.h"
#include "midiPlayer.h"
#include "midiFileParser.h"

//==============================================================================
// Defines
//==============================================================================

// Define the total length of the MIDI USB Device Descriptorg
#define MIDI_CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_MIDI_DESC_LEN)

#define NUM_FRAME_TIMES 60

//==============================================================================
// Enums
//==============================================================================

// Interface counter
enum interface_count
{
    ITF_NUM_MIDI = 0,
    ITF_NUM_MIDI_STREAMING,

    ITF_COUNT
};

// USB Endpoint numbers
enum usb_endpoints
{
    // Available USB Endpoints: 5 IN/OUT EPs and 1 IN EP
    EP_EMPTY = 0,
    EPNUM_MIDI,
};

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    const char* text;
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
    midiFileReader_t midiFileReader;
    bool fileMode;

    bool localPitch;
    uint16_t pitch;

    bool startupSeqComplete;
    int64_t noteTime;
    uint8_t startupNote;
    bool startupDrums;
    bool startSilence;
    const char* longestProgramName;
    uint8_t lastSamples[256];
    uint8_t localChannel;

    enum
    {
        VM_PRETTY  = 0,
        VM_TEXT    = 1,
        VM_GRAPH   = 2,
        VM_PACKETS = 4,
    } viewMode;

    wsg_t instrumentImages[16];
    wsg_t percussionImage;

    uint32_t frameTimesIdx;
    uint64_t frameTimes[NUM_FRAME_TIMES];

    list_t midiTexts;
    uint64_t nextExpiry;
} synthData_t;

//==============================================================================
// Function Prototypes
//==============================================================================

static void synthEnterMode(void);
static void synthExitMode(void);
static void synthMainLoop(int64_t elapsedUs);
static void synthDacCallback(uint8_t* samples, int16_t len);

static bool installUsb(void);
static void handlePacket(uint8_t packet[4]);
static void drawChannelInfo(const midiPlayer_t* player, uint8_t chIdx, int16_t x, int16_t y, int16_t width,
                            int16_t height);
static void drawSampleGraph(void);
static void midiTextCallback(metaEventType_t type, const char* text);

//==============================================================================
// Variabes
//==============================================================================

static const char plugMeInStr[] = "Plug me into a MIDI controller!";
static const char readyStr[]    = "Ready!";

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

/**
 * @brief MIDI Device String descriptor
 */
static const char* midiStringDescriptor[5] = {
    // array of pointer to string descriptors
    (char[]){0x09, 0x04},    // 0: is supported language is English (0x0409)
    "MAGFest",               // 1: Manufacturer
    "Swadge Synthesizer",    // 2: Product
    "123456",                // 3: Serials
    "Swadge MIDI interface", // 4: MIDI
};

/**
 * @brief MIDI Device Config Descriptor
 */
static const uint8_t midiConfigDescriptor[]
    = {TUD_CONFIG_DESCRIPTOR(1, ITF_COUNT, 0, MIDI_CONFIG_TOTAL_LEN, 0, 100),
       TUD_MIDI_DESCRIPTOR(ITF_NUM_MIDI, 4, EPNUM_MIDI, (0x80 | EPNUM_MIDI), 64)};

// just yolo'd these color values, they're probably awful
static const paletteColor_t noteColors[] = {
    c550, // C
    c350, // C#
    c130, // D
    c050, // D#
    c043, // E
    c015, // F
    c005, // F#
    c035, // G
    c055, // G#
    c500, // A
    c510, // A#
    c430, // B
};

const char synthModeName[] = "USB MIDI Synth";

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
    loadFont("ibm_vga8.font", &sd->font, false);
    sd->installed = installUsb();
    sd->perc[9]   = true;
    midiPlayerInit(&sd->midiPlayer);
    sd->noteTime           = 200000;
    sd->pitch              = 0x2000;
    sd->longestProgramName = gmProgramNames[24];
    sd->nextExpiry         = 200000;

    sd->fileMode = true;
    if (sd->fileMode)
    {
        if (loadMidiFile(&sd->midiFile, "all_star.midi", false))
        {
            if (initMidiParser(&sd->midiFileReader, &sd->midiFile))
            {
                sd->midiPlayer.textMessageCallback = midiTextCallback;
                midiSetFile(&sd->midiPlayer, &sd->midiFileReader);
            }
            else
            {
                ESP_LOGE("Synth", "Could not init MIDI parser");
            }
        }
        else
        {
            ESP_LOGE("Synth", "Could not load MIDI file");
        }
    }

    loadWsg("piano.wsg", &sd->instrumentImages[0], false);
    loadWsg("chromatic_percussion.wsg", &sd->instrumentImages[1], false);
    loadWsg("organ.wsg", &sd->instrumentImages[2], false);
    loadWsg("guitar.wsg", &sd->instrumentImages[3], false);
    loadWsg("bass.wsg", &sd->instrumentImages[4], false);
    loadWsg("solo_strings.wsg", &sd->instrumentImages[5], false);
    loadWsg("ensemble.wsg", &sd->instrumentImages[6], false);
    loadWsg("brass.wsg", &sd->instrumentImages[7], false);
    loadWsg("reed.wsg", &sd->instrumentImages[8], false);
    loadWsg("pipe.wsg", &sd->instrumentImages[9], false);
    loadWsg("synth_lead.wsg", &sd->instrumentImages[10], false);
    loadWsg("synth_pad.wsg", &sd->instrumentImages[11], false);
    loadWsg("synth_effects.wsg", &sd->instrumentImages[12], false);
    loadWsg("ethnic.wsg", &sd->instrumentImages[13], false);
    loadWsg("percussive.wsg", &sd->instrumentImages[14], false);
    loadWsg("sound_effects.wsg", &sd->instrumentImages[15], false);
    loadWsg("percussion.wsg", &sd->percussionImage, false);

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
    unloadMidiFile(&sd->midiFile);
    freeFont(&sd->font);
    free(sd);
    sd = NULL;
}

static void synthMainLoop(int64_t elapsedUs)
{
    sd->pluggedIn = tud_ready();

    uint8_t packet[4] = {0, 0, 0, 0};
    while (tud_ready() && tud_midi_available())
    {
        if (tud_midi_packet_read(packet))
        {
            handlePacket(packet);
            if (packet[0])
            {
                memcpy(sd->lastPackets[packet[1] & 0xF], packet, sizeof(packet));
            }
        }
    }

    // Blank the screen
    clearPxTft();

    if (!sd->installed || sd->err != 0)
    {
        drawText(&sd->font, c500, "ERROR!", 60, 60);
    }
    else if (!sd->pluggedIn)
    {
        drawText(&sd->font, c550, plugMeInStr, (TFT_WIDTH - textWidth(&sd->font, plugMeInStr)) / 2,
                 (TFT_HEIGHT - sd->font.height) / 2);
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
                // Just play each drum note with a half-second gap between
                midiNoteOn(&sd->midiPlayer, 9, sd->startupNote++, 0x7f);
                sd->noteTime = 100000;

                if (sd->startupNote > OPEN_TRIANGLE)
                {
                    sd->startupSeqComplete = true;
                }
            }
        }
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
            drawChannelInfo(&sd->midiPlayer, ch, x, (ch < 8) ? (imgY + 32 + 2) : (imgY - 16 - 2), 32, 16);
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
        snprintf(tempoStr, sizeof(tempoStr), "%" PRIu32 " BPM", (60000000 / sd->midiPlayer.tempo));
        drawText(&sd->font, c500, tempoStr, TFT_WIDTH - textWidth(&sd->font, tempoStr) - 15,
                 (TFT_HEIGHT - sd->font.height) / 2);

        char textMessages[1024];
        textMessages[0] = '\0';

        paletteColor_t midiTextColor = c550;
        bool colorSet                = false;

        node_t* curNode = sd->midiTexts.first;
        while (curNode != NULL)
        {
            midiTextInfo_t* curInfo = curNode->val;
            if (strlen(textMessages) + strlen(curInfo->text) + 1 >= sizeof(textMessages))
            {
                // don't overflow that buffer
                break;
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
                    strcat(textMessages, "\n");
                }
                else if (*curInfo->text != ' ')
                {
                    strcat(textMessages, " ");
                }
            }

            strcat(textMessages, curInfo->text);
            curNode = curNode->next;
        }

        int16_t x = 18;
        int16_t y = 80;
        drawTextWordWrap(&sd->font, c550, textMessages, &x, &y, TFT_WIDTH - 18, TFT_HEIGHT - 60);
    }
    else
    {
        char countsBuf[16];
        // Display the number of clipped samples
        snprintf(countsBuf, sizeof(countsBuf), "%" PRIu32, sd->midiPlayer.clipped);
        drawText(&sd->font, c500, countsBuf, TFT_WIDTH - textWidth(&sd->font, countsBuf) - 15,
                 TFT_HEIGHT - sd->font.height - 15);
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

    int32_t phi, r, intensity;
    if (getTouchJoystick(&phi, &r, &intensity))
    {
        int32_t x, y;
        getTouchCartesian(phi, r, &x, &y);
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
    else if (sd->localPitch)
    {
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
        if (evt.down && !sd->startupDrums)
        {
            midiNoteOff(&sd->midiPlayer, 0, sd->startupNote, 0x7f);
            sd->startupDrums = true;
            sd->startupNote  = ACOUSTIC_BASS_DRUM_OR_LOW_BASS_DRUM;
        }
        else if (evt.down && sd->startupDrums && !sd->startupSeqComplete)
        {
            midiNoteOff(&sd->midiPlayer, 0, sd->startupNote, 0x7f);
            sd->startupSeqComplete = true;
        }
        else if (evt.down && sd->startupSeqComplete)
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
                    if (sd->viewMode == VM_PRETTY)
                    {
                        sd->viewMode = VM_GRAPH;
                    }
                    else if (sd->viewMode == VM_GRAPH)
                    {
                        sd->viewMode |= VM_TEXT;
                    }
                    else if (sd->viewMode == (VM_GRAPH | VM_TEXT))
                    {
                        sd->viewMode = (VM_GRAPH | VM_PACKETS);
                    }
                    else if (sd->viewMode == (VM_GRAPH | VM_PACKETS))
                    {
                        sd->viewMode = VM_TEXT;
                    }
                    else if (sd->viewMode == VM_TEXT)
                    {
                        sd->viewMode = VM_PACKETS;
                    }
                    else if (sd->viewMode == VM_PACKETS)
                    {
                        sd->viewMode = VM_PRETTY;
                    }
                    break;
                }

                case PB_A:
                {
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

    sd->frameTimesIdx                 = (sd->frameTimesIdx + 1) % NUM_FRAME_TIMES;
    sd->frameTimes[sd->frameTimesIdx] = esp_timer_get_time();
}

static void synthDacCallback(uint8_t* samples, int16_t len)
{
    midiPlayerFillBuffer(&sd->midiPlayer, samples, len);
    memcpy(sd->lastSamples, samples, MIN(len, 256));
}

static bool installUsb(void)
{
    tinyusb_config_t const tusb_cfg = {
        .device_descriptor        = NULL,
        .string_descriptor        = midiStringDescriptor,
        .string_descriptor_count  = sizeof(midiStringDescriptor) / sizeof(midiStringDescriptor[0]),
        .external_phy             = false,
        .configuration_descriptor = midiConfigDescriptor,
    };
    esp_err_t result = tinyusb_driver_install(&tusb_cfg);

    if (result != ESP_OK)
    {
        ESP_LOGE("MIDI", "Error %d", result);
        sd->err = (int)result;

        return false;
    }

    return true;
}

static void handlePacket(uint8_t packet[4])
{
    uint8_t header  = packet[0];
    uint8_t cmd     = packet[1];
    uint8_t channel = cmd & 0x0F;

    switch (header)
    {
        // No MIDI data
        case 0x0:
            break;

        // Note OFF
        case 0x8:
        {
            uint8_t midiKey  = packet[2];
            uint8_t velocity = packet[3];
            midiNoteOff(&sd->midiPlayer, channel, midiKey, velocity);
            /*if (!sd->sustain)
            {
                sd->playing[channel] = false;
                spkStopNote2(0, channel);
            }
            else
            {
                sd->noteSus = true;
            }*/
            break;
        }

        // Note ON
        case 0x9:
        {
            uint8_t midiKey  = packet[2];
            uint8_t velocity = packet[3];
            midiNoteOn(&sd->midiPlayer, channel, midiKey, velocity);
            /*sd->playing[channel] = true;
#ifdef FINE_NOTES
            spkPlayNoteFine(noteFreqTable[midiKey], 0, channel, velocity << 1);
#else
            spkPlayNote(coarseNoteFreqTable[midiKey], channel, velocity << 1);
#endif*/
            break;
        }

        // Control change
        case 0xB:
        {
            uint8_t controlId  = packet[2];
            uint8_t controlVal = packet[3];
            switch (controlId)
            {
                // Sustain
                case 0x40:
                {
                    midiSustain(&sd->midiPlayer, channel, controlVal);
                    /*
                    sd->sustain = controlVal > 63;

                    if (!sd->sustain && sd->noteSus)
                    {
                        sd->playing[channel] = false;
                        spkStopNote2(0, channel);
                        sd->noteSus = false;
                    }*/
                    break;
                }

                // All sounds off (120)
                case 0x78:
                {
                    midiAllSoundOff(&sd->midiPlayer);
                    /*sd->noteSus = false;
                    for (int chan = 0; chan < 16; chan++)
                    {
                        sd->playing[channel] = false;
                        spkStopNote2(0, chan);
                    }*/
                    break;
                }

                // All notes off (123)
                case 0x7B:
                {
                    midiAllNotesOff(&sd->midiPlayer, channel);
                    /*for (int chan = 0; chan < 16; chan++)
                    {
                        if (!sd->sustain)
                        {
                            sd->playing[chan] = false;
                            spkStopNote2(0, chan);
                        }
                        else
                        {
                            sd->noteSus = true;
                        }
                    }*/
                    break;
                }
            }
            break;
        }

        // Program Select
        case 0xC:
        {
            uint8_t program = packet[2];
            midiSetProgram(&sd->midiPlayer, channel, program);
            /*sd->programs[channel] = program;

            sd->playing[channel] = false;
            spkStopNote2(0, channel);

            if (!sd->perc[channel])
            {
                spkSongSetWave(0, channel, program);
            }*/

            break;
        }

        // Pitch bend
        case 0xE:
        {
            uint16_t range = ((packet[3] & 0x7F) << 7) | (packet[2] & 0x7F);
            ESP_LOGI("Synth", "Pitch: %hx", range);
            midiPitchWheel(&sd->midiPlayer, channel, range);
            break;
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

void drawSampleGraph(void)
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

static void midiTextCallback(metaEventType_t type, const char* text)
{
    if (!text)
    {
        return;
    }

    void* infoAndText = malloc(sizeof(midiTextInfo_t) + strlen(text) + 1);

    if (infoAndText)
    {
        midiTextInfo_t* info = (midiTextInfo_t*)infoAndText;
        char* newText        = (char*)(infoAndText + sizeof(midiTextInfo_t));
        info->text           = newText;
        info->type           = type;
        // make it last for 2 seconds
        info->expiration = esp_timer_get_time() + 5000000;
        strcpy(newText, text);

        push(&sd->midiTexts, infoAndText);
    }
}