#include <stddef.h>
#include <stdlib.h>

#include "esp_log.h"
#include "esp_timer.h"

#include "tinyusb.h"

#include "swadge2024.h"
#include "spiffs_font.h"
#include "hdw-btn.h"
#include "touchUtils.h"

#include "sngPlayer.h"
#include "midiPlayer.h"

//==============================================================================
// Defines
//==============================================================================

// Define the total length of the MIDI USB Device Descriptorg
#define MIDI_CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_MIDI_DESC_LEN)

//==============================================================================
// Enums
//==============================================================================

// Interface counter
enum interface_count {
    ITF_NUM_MIDI = 0,
    ITF_NUM_MIDI_STREAMING,

    ITF_COUNT
};

// USB Endpoint numbers
enum usb_endpoints {
    // Available USB Endpoints: 5 IN/OUT EPs and 1 IN EP
    EP_EMPTY = 0,
    EPNUM_MIDI,
};

//==============================================================================
// Structs
//==============================================================================

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

    midiPlayer_t midiPlayer;

    bool localPitch;
    uint16_t pitch;

    bool startupSeqComplete;
    int64_t noteTime;
    uint8_t startupNote;
    bool startSilence;
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

//==============================================================================
// Variabes
//==============================================================================

static const char plugMeInStr[] = "Plug me into a MIDI controller!";
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
static const uint8_t midiConfigDescriptor[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_COUNT, 0, MIDI_CONFIG_TOTAL_LEN, 0, 100),
    TUD_MIDI_DESCRIPTOR(ITF_NUM_MIDI, 4, EPNUM_MIDI, (0x80 | EPNUM_MIDI), 64)
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
    sd->perc[9] = true;
    midiPlayerInit(&sd->midiPlayer);
    sd->noteTime = 200000;
    sd->pitch = 0x2000;
}

static void synthExitMode(void)
{
    freeFont(&sd->font);
    free(sd);
    sd = NULL;
}

static void synthMainLoop(int64_t elapsedUs)
{
    sd->pluggedIn = tud_ready();

    // Blank the screen
    clearPxTft();

    if (!sd->installed || sd->err != 0)
    {
        drawText(&sd->font, c500, "ERROR!", 60, 60);
    }
    else if (!sd->pluggedIn)
    {
        drawText(&sd->font, c550, plugMeInStr, (TFT_WIDTH - textWidth(&sd->font, plugMeInStr)) / 2, (TFT_HEIGHT - sd->font.height) / 2);
    }
    else if (!sd->startupSeqComplete)
    {
        sd->noteTime -= elapsedUs;

        if (sd->noteTime <= 0)
        {
            if (sd->startSilence)
            {
                if (sd->startupNote == 0x7f)
                {
                    sd->startupSeqComplete = true;
                    // 25ms of silence between the notes
                    sd->noteTime = 25000;
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
            }
            sd->startSilence = !sd->startSilence;
        }
    }
    else
    {
        drawText(&sd->font, c050, readyStr, (TFT_WIDTH - textWidth(&sd->font, readyStr)) / 2, 10);

        char packetMsg[64];
        int16_t y = 35;
        for (int ch = 0; ch < 16; ch++)
        {
            sd->playing[ch] = sd->midiPlayer.channels[ch].held || sd->midiPlayer.channels[ch].voiceStates.attack || sd->midiPlayer.channels[ch].voiceStates.decay || sd->midiPlayer.channels[ch].voiceStates.sustain || sd->midiPlayer.channels[ch].voiceStates.release;
            paletteColor_t col = sd->playing[ch] ? c555 : c222;
            const char* programName = sd->perc[ch] ? "<Drumkit>" : gmProgramNames[sd->programs[ch]];
            // Draw the program name
            drawText(&sd->font, col, programName, 10, y);

            // And the last packet for this channel
            snprintf(packetMsg, sizeof(packetMsg),
                "%02hhX %02hhX %02hhX %02hhX",
                sd->lastPackets[ch][0],
                sd->lastPackets[ch][1],
                sd->lastPackets[ch][2],
                sd->lastPackets[ch][3]);
            packetMsg[sizeof(packetMsg) - 1] = '\0';
            drawText(&sd->font, col, packetMsg, TFT_WIDTH - textWidth(&sd->font, packetMsg) - 5, y);
            y += sd->font.height + 1;
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
            midiPitchWheel(&sd->midiPlayer, 0, sd->pitch);
        }
    }
    else if (sd->localPitch)
    {
        sd->localPitch = false;
        sd->pitch = 0x2000;
        midiPitchWheel(&sd->midiPlayer, 0, sd->pitch);
    }

    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        if (evt.down && !sd->startupSeqComplete)
        {
            midiNoteOff(&sd->midiPlayer, 0, sd->startupNote, 0x7f);
            sd->startupSeqComplete = true;
        }
        // TODO: Handle key presses
    }

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
}

static void synthDacCallback(uint8_t* samples, int16_t len)
{
    midiPlayerFillBuffer(&sd->midiPlayer, samples, len);
}

static bool installUsb(void)
{
    tinyusb_config_t const tusb_cfg = {
        .device_descriptor = NULL,
        .string_descriptor = midiStringDescriptor,
        .string_descriptor_count = sizeof(midiStringDescriptor) / sizeof(midiStringDescriptor[0]),
        .external_phy = false,
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
    uint8_t header = packet[0];
    uint8_t cmd = packet[1];
    uint8_t channel = cmd & 0x0F;

    switch (header)
    {
        // No MIDI data
        case 0x0: break;

        // Note OFF
        case 0x8:
        {
            uint8_t midiKey = packet[2];
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
            uint8_t midiKey = packet[2];
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
            uint8_t controlId = packet[2];
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