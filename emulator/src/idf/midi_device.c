//--------------------------------------------------------------------+
// Application API (Multiple Interfaces)
// CFG_TUD_MIDI > 1
//--------------------------------------------------------------------+

#if defined(__clang__) || (defined(__GNUC__) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ > 5))))
    #pragma GCC diagnostic push
#endif
#ifdef __GNUC__
    #pragma GCC diagnostic ignored "-Wmissing-prototypes"
    #pragma GCC diagnostic ignored "-Wunused-function"
#endif

#define PLATFORM_MIDI_IMPLEMENTATION
#include "platform_midi.h"

#if defined(__clang__) || (defined(__GNUC__) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ > 5))))
    #pragma GCC diagnostic pop
#endif

#include "midi_device.h"
#include "midi_device_emu.h"

#include "tinyusb.h"
#include <stdbool.h>
#include <stdint.h>

static uint8_t runningStatus                   = 0;
static struct platform_midi_driver* midiDriver = NULL;

// Check if midi interface is mounted
bool tud_midi_n_mounted(uint8_t itf)
{
    return NULL != midiDriver;
}

// Get the number of bytes available for reading
uint32_t tud_midi_n_available(uint8_t itf, uint8_t cable_num)
{
    return midiDriver ? platform_midi_avail(midiDriver) : 0;
}

// Read byte stream              (legacy)
uint32_t tud_midi_n_stream_read(uint8_t itf, uint8_t cable_num, void* buffer, uint32_t bufsize)
{
    return midiDriver ? platform_midi_read(midiDriver, (unsigned char*)buffer, bufsize) : 0;
}

// Write byte Stream             (legacy)
uint32_t tud_midi_n_stream_write(uint8_t itf, uint8_t cable_num, uint8_t const* buffer, uint32_t bufsize)
{
    int written = midiDriver ? platform_midi_write(midiDriver, buffer, bufsize) : 0;

    if (written > 0)
    {
        return (uint32_t)written;
    }

    return 0;
}

// Read event packet             (4 bytes)
bool tud_midi_n_packet_read(uint8_t itf, uint8_t packet[4])
{
    uint8_t real_packet[12];
    int read = midiDriver ? platform_midi_read(midiDriver, real_packet, sizeof(real_packet)) : 0;
    if (read > 0)
    {
        printf("Packet: ");
        for (int i = 0; i < read; i++)
        {
            printf("%hhx, ", real_packet[i]);
        }
        printf("\n");

        // Normally start reading after the status byte
        int dataOffset = 1;

        uint8_t status = real_packet[0];

        if (status > 0xF7)
        {
            // Realtime message, does not affect running status
        }
        else if (0xF0 <= status && status <= 0xF7)
        {
            runningStatus = 0;
        }
        else if (0x80 <= status && status <= 0xEF)
        {
            runningStatus = status;
        }
        else
        {
            if (runningStatus == 0)
            {
                printf("Invalid MIDI status byte %hhx (no running status)\n", status);
                return false;
            }

            status = runningStatus;
            // no need to skip the status byte now
            dataOffset = 0;
        }

        packet[0] = (status & 0xF0) >> 4;
        packet[1] = status;

        for (int i = 0; i < 2 && (dataOffset + i) < read; i++)
        {
            packet[i + 2] = real_packet[dataOffset + i];
        }

        return true;
    }
    else
    {
        packet[0] = 0;
        packet[1] = 0;
        packet[2] = 0;
        packet[3] = 0;
        return false;
    }
}

// Write event packet            (4 bytes)
bool tud_midi_n_packet_write(uint8_t itf, uint8_t const packet[4])
{
    return tud_midi_n_stream_write(itf, 0, packet, 4);
}

//--------------------------------------------------------------------+
// Application API (Single Interface)
//--------------------------------------------------------------------+
bool tud_midi_mounted(void)
{
    return tud_midi_n_mounted(0);
}

uint32_t tud_midi_available(void)
{
    return tud_midi_n_available(0, 0);
}

uint32_t tud_midi_stream_read(void* buffer, uint32_t bufsize)
{
    return tud_midi_n_stream_read(0, 0, buffer, bufsize);
}

uint32_t tud_midi_stream_write(uint8_t cable_num, uint8_t const* buffer, uint32_t bufsize)
{
    return tud_midi_n_stream_write(0, cable_num, buffer, bufsize);
}

bool tud_midi_packet_read(uint8_t packet[4])
{
    return tud_midi_n_packet_read(0, packet);
}

bool tud_midi_packet_write(uint8_t const packet[4])
{
    return tud_midi_n_packet_write(0, packet);
}

//--------------------------------------------------------------------+
// Application Callback API (weak is optional)
//--------------------------------------------------------------------+
void tud_midi_rx_cb(uint8_t itf)
{
}

//--------------------------------------------------------------------+
// Internal Class Driver API
//--------------------------------------------------------------------+
static const char* clientName = 0;

void setMidiClientName(const char* name)
{
    clientName = name;
}
void midid_init(void)
{
    printf("Initializing MIDI!\n");
    if (midiDriver == NULL)
    {
        midiDriver = platform_midi_init(clientName ? "Swadge Emulator MIDI" : clientName);
    }
}

void midid_reset(uint8_t rhport)
{
    platform_midi_deinit(midiDriver);
    midiDriver = NULL;
}

uint16_t midid_open(uint8_t rhport, tusb_desc_interface_t const* itf_desc, uint16_t max_len)
{
    return 0;
}

bool midid_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const* request)
{
    return false;
}

bool midid_xfer_cb(uint8_t rhport, uint8_t edpt_addr, xfer_result_t result, uint32_t xferred_bytes)
{
    return false;
}
