#include "midiUsb.h"

#include <esp_log.h>

#include "swadge2024.h"
#include "tinyusb.h"

//==============================================================================
// Defines
//==============================================================================

// Define the total length of the MIDI USB Device Descriptorg
#define MIDI_CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_MIDI_DESC_LEN)

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

// Static function declarations
static bool handlePacket(midiEvent_t* event, const uint8_t packet[4]);

//==============================================================================
// Variables
//==============================================================================

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

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Attempt to convert a TinyUSB MIDI packet to a midiEvent_t
 *
 * @param[out] event A pointer to the MIDI event to update
 * @param[in] packet The TinyUSB MIDI packet to convert
 * @return true if an event was written
 * @return false if the packet could not be parsed into an event
 */
static bool handlePacket(midiEvent_t* event, const uint8_t packet[4])
{
    uint8_t header = packet[0];
    uint8_t cmd    = packet[1];

    switch (header)
    {
        // No MIDI data
        case 0x0:
            return false;

        // Statuses with two data bytes
        case 0x8: // Note OFF
        case 0x9: // Note ON
        case 0xB: // Control Change
        case 0xE: // Pitch bend
        {
            event->type         = MIDI_EVENT;
            event->midi.status  = cmd;
            event->midi.data[0] = packet[2];
            event->midi.data[1] = packet[3];
            return true;
        }

        // Statuses with one data byte
        case 0xC: // Program Select
        case 0xD: // Channel Pressure
        {
            event->type         = MIDI_EVENT;
            event->midi.status  = cmd;
            event->midi.data[0] = packet[2];
            return true;
        }

        case 0xF: // System
        {
            event->type         = SYSEX_EVENT;
            event->sysex.data   = NULL;
            event->sysex.length = 0;
            event->sysex.prefix = 0;
            return true;
        }

        default:
        {
            // Idk?
            return false;
        }
    }
}

bool usbMidiCallback(midiEvent_t* event)
{
    uint8_t packet[4] = {0, 0, 0, 0};

    while (tud_ready() && tud_midi_available())
    {
        if (tud_midi_packet_read(packet))
        {
            if (packet[0])
            {
                return handlePacket(event, packet);
            }
        }
    }

    return false;
}

bool installMidiUsb(void)
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
        ESP_LOGE("MIDI-USB", "Cannot install MIDI USB driver: error %d", result);

        return false;
    }

    return true;
}
