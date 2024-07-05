#ifndef _PLATFORM_MIDI_COREMIDI_H_
#define _PLATFORM_MIDI_COREMIDI_H_

int platform_midi_init_coremidi(const char* name);
void platform_midi_deinit_coremidi(void);
int platform_midi_read_coremidi(unsigned char * out, int size);
int platform_midi_avail_coremidi(void);
int platform_midi_write_coremidi(unsigned char* buf, int size);

// TODO don't do it like this
#define PLATFORM_MIDI_INIT(name) platform_midi_init_coremidi(name)
#define PLATFORM_MIDI_DEINIT() platform_midi_deinit_coremidi()
#define PLATFORM_MIDI_READ(out, size) platform_midi_read_coremidi(out, size)
#define PLATFORM_MIDI_AVAIL() platform_midi_avail_coremidi()
#define PLATFORM_MIDI_WRITE(buf, size) platform_midi_write_coremidi(buf, size)

#ifdef PLATFORM_MIDI_IMPLEMENTATION
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <MacTypes.h>
#include <CoreMIDI/CoreMIDI.h>

#ifndef PLATFORM_MIDI_EVENT_BUFFER_ITEMS
#define PLATFORM_MIDI_EVENT_BUFFER_ITEMS 32
#endif

#ifndef PLATFORM_MIDI_EVENT_BUFFER_SIZE
#define PLATFORM_MIDI_EVENT_BUFFER_SIZE 1024
#endif

MIDIClientRef coremidi_client;
MIDIPortRef coremidi_in_port;

void (^platform_midi_receive_callback)(const MIDIEventList* events, void* refcon) = ^{
    printf("Recieved %u MIDI packets from CoreAudio\n", events->numPackets);

    for (unsigned int i = 0; i < events->numPackets; i++)
    {
        unsigned char data[16];
        int written = platform_midi_convert_ump(data, sizeof(data), events->packet[i].words, events->packet[i].wordCount);
        platform_midi_push_packet(data, written);
    }
};

int platform_midi_init_coremidi(const char* name)
{
    /* name: The client name */
    /* notifyProc: an optional callback for system changes */
    /* notifyRefCon: a nullable refCon for notifyRefCon*/
    CFString nameCf = CFStringCreateWithCString(NULL, name, UTF8);
    OSStatus result = MIDIClientCreate(nameCf, NULL, NULL, &coremidi_client);

    if (0 != result)
    {
        printf("Failed to initialize CoreMIDI driver\n");
        return 0;
    }

    if (!platform_midi_buffer_init())
    {
        printf("Failed to allocate memory for event buffer in CoreMIDI driver\n");
        MIDIClientDispose(coremidi_client);
        coremidi_client = 0;

        return 0;
    }

    result = MIDIInputPortCreateWithProtocol(
        coremidi_client,
        CFStringCreateWithCStringNoCopy(0, "listen:in", UTF8),
        kMIDIProtocol_1_0,
        &coremidi_in_port,
        platform_midi_receive_callback
    );

    if (0 != result)
    {
        printf("Failed to create CoreMIDI input port\n");

        platform_midi_buffer_deinit();

        MIDIClientDispose(coremidi_client);
        coremidi_client = 0;
        return 0;
    }

    return 1;
}

void platform_midi_deinit_coremidi(void)
{
    OSStatus result = MIDIPortDispose(coremidi_in_port);
    coremidi_in_port = 0;

    if (0 != result)
    {
        printf("failed to destroy CoreMIDI input port\n");
    }

    platform_midi_buffer_deinit();

    result = MIDIClientDispose(coremidi_client);
    coremidi_client = 0;

    if (0 != result)
    {
        printf("Failed to deinitialize CoreMIDI driver\n");
    }
}

int platform_midi_read_coremidi(unsigned char * out, int size)
{
    return platform_midi_pop_packet(out, size);
}

int platform_midi_avail_coremidi(void)
{
    return platform_midi_packet_count();
}

int platform_midi_write_coremidi(unsigned char* buf, int size)
{
    printf("platform_midi_write_coremidi() not implemented\n");
    return 0;
}

#endif

#endif
