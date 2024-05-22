#include <stddef.h>
#include <stdlib.h>

#include "esp_log.h"
#include "esp_timer.h"

#include "tinyusb.h"
//#include <class/midi/midi_device.h>

#include "swadge2024.h"
#include "spiffs_font.h"

#include "sngPlayer.h"

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
    uint8_t lastPacket[4];
} synthData_t;

//==============================================================================
// Function Prototypes
//==============================================================================

static void synthEnterMode(void);
static void synthExitMode(void);
static void synthMainLoop(int64_t elapsedUs);

static bool installUsb(void);
static void handlePacket(uint8_t packet[4]);

//==============================================================================
// Variabes
//==============================================================================

static const char plugMeInStr[] = "Plug me into a MIDI controller!";
static const char readyStr[] = "Ready!";

static const noteFrequency_t noteFreqTable[] = {
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
};

static synthData_t* sd;

// Functions

static void synthEnterMode(void)
{
    sd = calloc(1, sizeof(synthData_t));
    loadFont("ibm_vga8.font", &sd->font, false);
    sd->installed = installUsb();
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
        snprintf(packetMsg, sizeof(packetMsg),
                "Last packet: %02hhX %02hhX %02hhX %02hhX",
                sd->lastPacket[0],
                sd->lastPacket[1],
                sd->lastPacket[2],
                sd->lastPacket[3]);
        packetMsg[sizeof(packetMsg) - 1] = '\0';

        drawText(&sd->font, c555, packetMsg, (TFT_WIDTH - textWidth(&sd->font, packetMsg)) / 2, 120);
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
            ESP_LOGI("MIDI", "We got a packet! %hhX %hhX %hhX %hhX", packet[0], packet[1], packet[2], packet[3]);
            memcpy(sd->lastPacket, packet, sizeof(packet));
        }
    }
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

    // TODO: add more channels and get rid of this
    channel = 0;

    switch (header)
    {
        // No MIDI data
        case 0x0: break;

        // Note OFF
        case 0x8:
        {
            if (!sd->sustain)
            {
                spkStopNote(channel);
            }
            else
            {
                sd->noteSus = true;
            }
            break;
        }

        // Note ON
        case 0x9:
        {
            uint8_t midiKey = packet[2];
            uint8_t velocity = packet[3];
            spkPlayNote(noteFreqTable[midiKey], channel, velocity << 1);
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
                    sd->sustain = controlVal ? true : false;

                    if (!sd->sustain && sd->noteSus)
                    {
                        spkStopNote(0);
                        sd->noteSus = false;
                    }
                    break;
                }
            }
            break;
        }

        // Pitch bend
        case 0xE:
        {
            uint16_t range = (packet[3] << 8) | packet[2];
            break;
        }
    }
}