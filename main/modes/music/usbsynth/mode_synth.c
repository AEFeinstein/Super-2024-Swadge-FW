#include <stddef.h>
#include <stdlib.h>

#include "esp_log.h"
#include "esp_timer.h"

#include "tinyusb.h"

#include "swadge2024.h"
#include "spiffs_font.h"

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


#define FINE_NOTES 1

#ifndef FINE_NOTES

static const noteFrequency_t coarseNoteFreqTable[] = {
    // Super low notes that aren't in the noteFrequency enum
    8,
    9,
    9,
    10,
    10,
    11,
    12,
    12,
    13,
    14,
    15,
    15,
    // Notes that are in noteFrequency but not a piano
    C_0,
    C_SHARP_0,
    D_0,
    D_SHARP_0,
    E_0,
    F_0,
    F_SHARP_0,
    G_0,
    G_SHARP_0,
    // Piano keys
    A_0,
    A_SHARP_0,
    B_0,
    C_1,
    C_SHARP_1,
    D_1,
    D_SHARP_1,
    E_1,
    F_1,
    F_SHARP_1,
    G_1,
    G_SHARP_1,
    A_1,
    A_SHARP_1,
    B_1,
    C_2,
    C_SHARP_2,
    D_2,
    D_SHARP_2,
    E_2,
    F_2,
    F_SHARP_2,
    G_2,
    G_SHARP_2,
    A_2,
    A_SHARP_2,
    B_2,
    C_3,
    C_SHARP_3,
    D_3,
    D_SHARP_3,
    E_3,
    F_3,
    F_SHARP_3,
    G_3,
    G_SHARP_3,
    A_3,
    A_SHARP_3,
    B_3,
    C_4,
    C_SHARP_4,
    D_4,
    D_SHARP_4,
    E_4,
    F_4,
    F_SHARP_4,
    G_4,
    G_SHARP_4,
    A_4,
    A_SHARP_4,
    B_4,
    C_5,
    C_SHARP_5,
    D_5,
    D_SHARP_5,
    E_5,
    C_SHARP_6,
    D_6,
    D_SHARP_6,
    E_6,
    F_6,
    F_SHARP_6,
    G_6,
    G_SHARP_6,
    A_6,
    A_SHARP_6,
    B_6,
    C_7,
    C_SHARP_7,
    D_7,
    D_SHARP_7,
    E_7,
    F_7,
    F_SHARP_7,
    G_7,
    G_SHARP_7,
    A_7,
    A_SHARP_7,
    B_7,
    C_8,
    // End of piano keys
    C_SHARP_8,
    D_8,
    D_SHARP_8,
    E_8,
    F_8,
    F_SHARP_8,
    G_8,
    G_SHARP_8,
    A_8,
    A_SHARP_8,
    B_8,
    C_9,
    C_SHARP_9,
    D_9,
    D_SHARP_9,
    E_9,
    F_9,
    F_SHARP_9,
    G_9,
    // End of MIDI notes - these notes are possible but only if we transpose
    G_SHARP_9,
    A_9,
    A_SHARP_9,
    B_9,
    C_10,
    C_SHARP_10,
    D_10,
    D_SHARP_10,
    E_10,
    F_10,
    F_SHARP_10,
    G_10,
    // G_SHARP_10 through B_10 are not included
};


// The following python script was used to generate the note frequencies:
/*
def note_name(num):
    names = ["A","A#","B","C","C#","D","D#","E","F","F#","G","G#"]
    A0 = 21
    if num < A0:
        return f"n{num:d}"
    else:
        semitones = (num - A0)
        note = names[semitones % 12]
        octave = semitones // 12
        return f"{note}{octave}"

for n in range(128):
    C4 = 69
    f = 440 * 2**((n-C4)/12)
    dec_part = int(f)
    frac_part = f % 1
    frac_bits = int(frac_part / (1 / 256))
    name = note_name(n)
    pad = " " * (5 - len(str(dec_part)))
    actual = (dec_part << 8 | frac_bits)
    error = ((actual / 256.0) - f) / f * 100
    #print(f"    ({dec_part} << 8){pad}| {frac_bits:3}, // {name:3s} = {f:.3f} Hz (error={error:.3f}%)")
    print(f"    0x{actual:06x}, // {name:3s} = {f:.3f} Hz")
*/

#else

// Note frequencies, in UQ24.8 format
/*static const uint32_t noteFreqTable[] = {
    0x00082d, // n0  = 8.176 Hz
    0x0008a9, // n1  = 8.662 Hz
    0x00092d, // n2  = 9.177 Hz
    0x0009b9, // n3  = 9.723 Hz
    0x000a4d, // n4  = 10.301 Hz
    0x000ae9, // n5  = 10.913 Hz
    0x000b8f, // n6  = 11.562 Hz
    0x000c3f, // n7  = 12.250 Hz
    0x000cfa, // n8  = 12.978 Hz
    0x000dc0, // n9  = 13.750 Hz
    0x000e91, // n10 = 14.568 Hz
    0x000f6f, // n11 = 15.434 Hz
    0x00105a, // n12 = 16.352 Hz
    0x001152, // n13 = 17.324 Hz
    0x00125a, // n14 = 18.354 Hz
    0x001372, // n15 = 19.445 Hz
    0x00149a, // n16 = 20.602 Hz
    0x0015d3, // n17 = 21.827 Hz
    0x00171f, // n18 = 23.125 Hz
    0x00187f, // n19 = 24.500 Hz
    0x0019f4, // n20 = 25.957 Hz
    0x001b80, // A0  = 27.500 Hz
    0x001d22, // A#0 = 29.135 Hz
    0x001ede, // B0  = 30.868 Hz
    0x0020b4, // C0  = 32.703 Hz
    0x0022a5, // C#0 = 34.648 Hz
    0x0024b5, // D0  = 36.708 Hz
    0x0026e4, // D#0 = 38.891 Hz
    0x002934, // E0  = 41.203 Hz
    0x002ba7, // F0  = 43.654 Hz
    0x002e3f, // F#0 = 46.249 Hz
    0x0030ff, // G0  = 48.999 Hz
    0x0033e9, // G#0 = 51.913 Hz
    0x003700, // A1  = 55.000 Hz
    0x003a45, // A#1 = 58.270 Hz
    0x003dbc, // B1  = 61.735 Hz
    0x004168, // C1  = 65.406 Hz
    0x00454b, // C#1 = 69.296 Hz
    0x00496a, // D1  = 73.416 Hz
    0x004dc8, // D#1 = 77.782 Hz
    0x005268, // E1  = 82.407 Hz
    0x00574e, // F1  = 87.307 Hz
    0x005c7f, // F#1 = 92.499 Hz
    0x0061ff, // G1  = 97.999 Hz
    0x0067d3, // G#1 = 103.826 Hz
    0x006e00, // A2  = 110.000 Hz
    0x00748a, // A#2 = 116.541 Hz
    0x007b78, // B2  = 123.471 Hz
    0x0082d0, // C2  = 130.813 Hz
    0x008a97, // C#2 = 138.591 Hz
    0x0092d5, // D2  = 146.832 Hz
    0x009b90, // D#2 = 155.563 Hz
    0x00a4d0, // E2  = 164.814 Hz
    0x00ae9d, // F2  = 174.614 Hz
    0x00b8ff, // F#2 = 184.997 Hz
    0x00c3ff, // G2  = 195.998 Hz
    0x00cfa7, // G#2 = 207.652 Hz
    0x00dc00, // A3  = 220.000 Hz
    0x00e914, // A#3 = 233.082 Hz
    0x00f6f1, // B3  = 246.942 Hz
    0x0105a0, // C3  = 261.626 Hz
    0x01152e, // C#3 = 277.183 Hz
    0x0125aa, // D3  = 293.665 Hz
    0x013720, // D#3 = 311.127 Hz
    0x0149a0, // E3  = 329.628 Hz
    0x015d3a, // F3  = 349.228 Hz
    0x0171fe, // F#3 = 369.994 Hz
    0x0187fe, // G3  = 391.995 Hz
    0x019f4e, // G#3 = 415.305 Hz
    0x01b800, // A4  = 440.000 Hz
    0x01d229, // A#4 = 466.164 Hz
    0x01ede2, // B4  = 493.883 Hz
    0x020b40, // C4  = 523.251 Hz
    0x022a5d, // C#4 = 554.365 Hz
    0x024b54, // D4  = 587.330 Hz
    0x026e41, // D#4 = 622.254 Hz
    0x029341, // E4  = 659.255 Hz
    0x02ba74, // F4  = 698.456 Hz
    0x02e3fd, // F#4 = 739.989 Hz
    0x030ffd, // G4  = 783.991 Hz
    0x033e9c, // G#4 = 830.609 Hz
    0x037000, // A5  = 880.000 Hz
    0x03a453, // A#5 = 932.328 Hz
    0x03dbc4, // B5  = 987.767 Hz
    0x041680, // C5  = 1046.502 Hz
    0x0454bb, // C#5 = 1108.731 Hz
    0x0496a8, // D5  = 1174.659 Hz
    0x04dc82, // D#5 = 1244.508 Hz
    0x052682, // E5  = 1318.510 Hz
    0x0574e9, // F5  = 1396.913 Hz
    0x05c7fa, // F#5 = 1479.978 Hz
    0x061ffb, // G5  = 1567.982 Hz
    0x067d38, // G#5 = 1661.219 Hz
    0x06e000, // A6  = 1760.000 Hz
    0x0748a7, // A#6 = 1864.655 Hz
    0x07b788, // B6  = 1975.533 Hz
    0x082d01, // C6  = 2093.005 Hz
    0x08a976, // C#6 = 2217.461 Hz
    0x092d51, // D6  = 2349.318 Hz
    0x09b904, // D#6 = 2489.016 Hz
    0x0a4d05, // E6  = 2637.020 Hz
    0x0ae9d3, // F6  = 2793.826 Hz
    0x0b8ff4, // F#6 = 2959.955 Hz
    0x0c3ff6, // G6  = 3135.963 Hz
    0x0cfa70, // G#6 = 3322.438 Hz
    0x0dc000, // A7  = 3520.000 Hz
    0x0e914f, // A#7 = 3729.310 Hz
    0x0f6f11, // B7  = 3951.066 Hz
    0x105a02, // C7  = 4186.009 Hz
    0x1152ec, // C#7 = 4434.922 Hz
    0x125aa2, // D7  = 4698.636 Hz
    0x137208, // D#7 = 4978.032 Hz
    0x149a0a, // E7  = 5274.041 Hz
    0x15d3a6, // F7  = 5587.652 Hz
    0x171fe9, // F#7 = 5919.911 Hz
    0x187fed, // G7  = 6271.927 Hz
    0x19f4e0, // G#7 = 6644.875 Hz
    0x1b8000, // A8  = 7040.000 Hz
    0x1d229e, // A#8 = 7458.620 Hz
    0x1ede22, // B8  = 7902.133 Hz
    0x20b404, // C8  = 8372.018 Hz
    0x22a5d8, // C#8 = 8869.844 Hz
    0x24b545, // D8  = 9397.273 Hz
    0x26e410, // D#8 = 9956.063 Hz
    0x293414, // E8  = 10548.082 Hz
    0x2ba74d, // F8  = 11175.303 Hz
    0x2e3fd2, // F#8 = 11839.822 Hz
    0x30ffda, // G8  = 12543.854 Hz
};*/

#endif

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
    else
    {
        drawText(&sd->font, c050, readyStr, (TFT_WIDTH - textWidth(&sd->font, readyStr)) / 2, 10);

        char packetMsg[64];
        int16_t y = 35;
        for (int ch = 0; ch < 16; ch++)
        {
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

    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
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
            uint16_t range = (packet[3] << 8) | packet[2];
            midiPitchWheel(&sd->midiPlayer, channel, range);
            break;
        }
    }
}