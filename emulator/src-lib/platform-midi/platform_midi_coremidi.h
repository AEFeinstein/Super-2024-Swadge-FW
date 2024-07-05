#ifndef _PLATFORM_MIDI_COREMIDI_H_
#define _PLATFORM_MIDI_COREMIDI_H_

int platform_midi_init_coremidi(const char* name);
void platform_midi_deinit_coremidi(void);
int platform_midi_read_coremidi(unsigned char * out, int size);
int platform_midi_avail_coremidi(void);
int platform_midi_write_coremidi(unsigned char* buf, int size);

#ifdef PLATFORM_MIDI_IMPLEMENTATION
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <MacTypes.h>
#include <CoreMidi.h>

#ifndef PLATFORM_MIDI_EVENT_BUFFER_ITEMS
#define PLATFORM_MIDI_EVENT_BUFFER_ITEMS 32
#endif

#ifndef PLATFORM_MIDI_EVENT_BUFFER_SIZE
#define PLATFORM_MIDI_EVENT_BUFFER_SIZE 1024
#endif

struct platform_midi_coremidi_packet_info
{
    unsigned int offset;
    unsigned int length;
};

MIDIClientRef coremidi_client;
MIDIPortRef coremidi_in_port;
unsigned char *coremidi_buffer;
struct platform_midi_coremidi_packet_info coremidi_packets[PLATFORM_MIDI_EVENT_BUFFER_ITEMS];
unsigned int coremidi_packet_avail;
unsigned int coremidi_packet_read_pos;
unsigned int coremidi_packet_write_pos;
unsigned int coremidi_buffer_offset;
unsigned int coremidi_buffer_end;

static void platform_midi_receive_callback(MIDIEventList* events, void* refcon)
{
    printf("Recieved %u MIDI packets from CoreAudio\n", events->numPackets);

    for (unsigned int i = 0; i < events->numPackets; i++)
    {
        if ((coremidi_packet_write_pos + 1) % PLATFORM_MIDI_EVENT_BUFFER_ITEMS == coremidi_packet_read_pos)
        {
            printf("Warn: MIDI packet buffer is full, dropping an event");
            // The buffer is full... gotta drop a packet
            coremidi_buffer_offset = (coremidi_buffer_offset + coremidi_packets[coremidi_packet_read_pos].length) % PLATFORM_MIDI_EVENT_BUFFER_SIZE;
            coremidi_packet_read_pos = (coremidi_packet_read_pos + 1) % PLATFORM_MIDI_EVENT_BUFFER_ITEMS;
        }

        coremidi_packets[coremidi_packet_write_pos].offset = coremidi_buffer_end;
        coremidi_packets[coremidi_packet_write_pos].length = events->packet[i].wordCount * sizeof(uint32_t);

        // TODO check for overrunning the actual buffer
        coremidi_buffer_end = (coremidi_buffer_end + coremidi_packets[coremidi_packet_write_pos].length) % PLATFORM_MIDI_EVENT_BUFFER_SIZE;

        coremidi_packet_write_pos = (coremidi_packet_write_pos + 1) % PLATFORM_MIDI_EVENT_BUFFER_ITEMS;
    }
}

int platform_midi_init_coremidi(const char* name)
{
    /* name: The client name */
    /* notifyProc: an optional callback for system changes */
    /* notifyRefCon: a nullable refCon for notifyRefCon*/
    OSStatus result = MIDIClientCreate(name, NULL, NULL, &coremidi_client);

    if (0 != result)
    {
        printf("Failed to initialize CoreMIDI driver\n");
        return 0;
    }

    coremidi_buffer = malloc(PLATFORM_MIDI_EVENT_BUFFER_SIZE);
    if (0 == coremidi_buffer)
    {
        printf("Failed to allocate memory for event buffer in CoreMIDI driver\n");
        MIDIClientDispose(coremidi_client);
        coremidi_client = 0;

        return 0;
    }

    coremidi_packet_avail = 0;
    coremidi_packet_read_pos = 0;
    coremidi_packet_write_pos = 0;
    coremidi_buffer_offset = 0;

    result = MIDIInputPortCreateWithProtocol(
        &coremidi_client,
        "listen:in",
        kMIDIProtocol_1_0,
        &coremidi_in_port,
        platform_midi_receive_callback
    );

    if (0 != result)
    {
        printf("Failed to create CoreMIDI input port\n");

        free(coremidi_buffer);
        coremidi_buffer = 0;

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

    free(coremidi_buffer);
    coremidi_buffer = 0;

    if (0 != result)
    {
        printf("failed to destroy CoreMIDI input port\n")
    }

    result = MIDIClientDispose(coremidi_client);
    coremidi_client = 0;

    if (0 != result)
    {
        printf("Failed to deinitialize CoreMIDI driver\n");
    }
}

int platform_midi_read_coremidi(unsigned char * out, int size)
{
    if (coremidi_packet_read_pos == coremidi_packet_write_pos)
    {
        return 0;
    }

    struct platform_midi_coremidi_packet_info *packet = &coremidi_packets[coremidi_packet_read_pos];
    int toCopy = (size < packet->length) ? size : packet->length;

    // TODO detect large events overrunning the buffer

    if (packet->offset + packet->length > PLATFORM_MIDI_EVENT_BUFFER_SIZE)
    {
        // event is split across the end and beginning of the buffer
        int endCopy = (packet->offset + toCopy <= PLATFORM_MIDI_EVENT_BUFFER_SIZE) ? toCopy : PLATFORM_MIDI_EVENT_BUFFER_SIZE - packet->offset;
        int startCopy = packet->length - endCopy;

        memcpy(out, &coremidi_buffer[packet->offset], endCopy);
        memcpy(out + endCopy, coremidi_buffer, startCopy);
        read = toCopy;
    }
    else
    {
        memcpy(out, size, toCopy);
    }

    coremidi_buffer_offset = (coremidi_buffer_offset + packet->length) % PLATFORM_MIDI_EVENT_BUFFER_SIZE;
    coremidi_packet_read_pos = (coremidi_packet_read_pos + 1) % PLATFORM_MIDI_EVENT_BUFFER_ITEMS;

    return toCopy;
}

int platform_midi_avail_coremidi(void)
{
    if (coremidi_packet_read_pos == coremidi_packet_write_pos)
    {
        return 0;
    }
    else if (coremidi_packet_read_pos < coremidi_packet_write_pos)
    {
        return coremidi_packet_write_pos - coremidi_packet_read_pos;
    }
    else
    {
        return PLATFORM_MIDI_EVENT_BUFFER_ITEMS - coremidi_packet_read_pos - coremidi_packet_write_pos;
    }
}

int platform_midi_write_coremidi(unsigned char* buf, int size)
{
    printf("platform_midi_write_coremidi() not implemented\n");
    return 0;
}

#endif

#endif
