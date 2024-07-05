#ifndef _PLATFORM_MIDI_H_
#define _PLATFORM_MIDI_H_

#define PLATFORM_MIDI_SUPPORTED 1

#if defined(__linux) || defined(__linux__) || defined(linux) || defined(__LINUX__)
#define PLATFORM_MIDI_ALSA 1
#define PLATFORM_MIDI_BUFFERED 0
#elif defined(__APPLE__)
#define PLATFORM_MIDI_COREMIDI 1
#define PLATFORM_MIDI_BUFFERED 1
#else
#define PLATFORM_MIDI_WINMM 1
#define PLATFORM_MIDI_BUFFERED 1
#endif

#if PLATFORM_MIDI_BUFFERED
#ifndef PLATFORM_MIDI_EVENT_BUFFER_ITEMS
#define PLATFORM_MIDI_EVENT_BUFFER_ITEMS 32
#endif

#ifndef PLATFORM_MIDI_EVENT_BUFFER_SIZE
#define PLATFORM_MIDI_EVENT_BUFFER_SIZE 1024
#endif

struct platform_midi_packet_info
{
    unsigned int offset;
    unsigned int length;
};

#ifdef PLATFORM_MIDI_IMPLEMENTATION
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
unsigned char *platform_midi_buffer;
struct platform_midi_packet_info platform_midi_packets[PLATFORM_MIDI_EVENT_BUFFER_ITEMS];
unsigned int platform_midi_packet_read_pos;
unsigned int platform_midi_packet_write_pos;
unsigned int platform_midi_buffer_offset;
unsigned int platform_midi_buffer_end;

#define platform_midi_buffer_empty() (platform_midi_packet_read_pos == platform_midi_packet_write_pos)

static void platform_midi_push_packet(unsigned char *data, unsigned int length)
{
    if ((platform_midi_packet_write_pos + 1) % PLATFORM_MIDI_EVENT_BUFFER_ITEMS == platform_midi_packet_read_pos)
    {
        printf("Warn: MIDI packet buffer is full, dropping an event\n");
        // The buffer is full... gotta drop a packet
        platform_midi_buffer_offset = (platform_midi_buffer_offset + platform_midi_packets[platform_midi_packet_read_pos].length) % PLATFORM_MIDI_EVENT_BUFFER_SIZE;
        platform_midi_packet_read_pos = (platform_midi_packet_read_pos + 1) % PLATFORM_MIDI_EVENT_BUFFER_ITEMS;
    }

    platform_midi_packets[platform_midi_packet_write_pos].offset = platform_midi_buffer_end;
    platform_midi_packets[platform_midi_packet_write_pos].length = length;

    if (platform_midi_buffer_end + length <= PLATFORM_MIDI_EVENT_BUFFER_SIZE)
    {
        memcpy(&platform_midi_buffer[platform_midi_buffer_end], data, length);
    }
    else
    {
        unsigned int startLen = PLATFORM_MIDI_EVENT_BUFFER_SIZE - platform_midi_buffer_end;
        memcpy(&platform_midi_buffer[platform_midi_buffer_end], data, startLen);
        memcpy(platform_midi_buffer, data + startLen, length - startLen);
    }

    platform_midi_buffer_end = (platform_midi_buffer_end + length) % PLATFORM_MIDI_EVENT_BUFFER_SIZE;
    platform_midi_packet_write_pos = (platform_midi_packet_write_pos + 1) % PLATFORM_MIDI_EVENT_BUFFER_ITEMS;
}

static int platform_midi_convert_ump(unsigned char *out, unsigned int maxlen, unsigned int *umpWords, unsigned int wordCount)
{
    unsigned int written = 0;
    unsigned char type  = (umpWords[0] & 0xF0000000) >> 28;
    unsigned char group = (umpWords[0] & 0x0F000000) >> 24;

    switch (type)
    {
        case 0x01:
        {
            // System real-time / system common
            unsigned char status = (umpWords[0] & 0x00FF0000) >> 20;
            unsigned int packetLen = 1;
            switch (status)
            {
                case 0xF1:
                case 0xF3:
                {
                    packetLen = 2;
                    break;
                }

                case 0xF2:
                {
                    packetLen = 3;
                    break;
                }

                default:
                break;
            }

            if (written + packetLen > maxlen)
            {
                return written;
            }
            out[written++] = status;

            if (packetLen > 1)
            {
                out[written++] = (umpWords[0] & 0x0000FF00) >> 8;

                if (packetLen > 2)
                {
                    out[written++] = (umpWords[0] & 0x000000FF);
                }
            }
            break;
        }

        case 0x02:
        {
            // Channel voice
            unsigned char status = (umpWords[0] & 0x00F00000) >> 20;
            unsigned int packetLen = 3;
            switch (status)
            {
                case 0xC:
                case 0xD:
                {
                    packetLen = 2;
                    break;
                }

                default:
                break;
            }

            if (written + packetLen > maxlen)
            {
                return written;
            }

            out[written++] = (umpWords[0] & 0x00FF0000) >> 16;
            out[written++] = (umpWords[0] & 0x0000FF00) >> 8;

            if (packetLen > 2)
            {
                out[written++] = umpWords[0] & 0x000000FF;
            }
            break;
        }

        case 0x03:
        {
            // Data (SysEx)
            // TODO
            break;
        }
    }

    return written;
}

static int platform_midi_packet_count(void)
{
    if (platform_midi_packet_read_pos == platform_midi_packet_write_pos)
    {
        return 0;
    }
    else if (platform_midi_packet_read_pos < platform_midi_packet_write_pos)
    {
        return platform_midi_packet_write_pos - platform_midi_packet_read_pos;
    }
    else
    {
        return PLATFORM_MIDI_EVENT_BUFFER_ITEMS - platform_midi_packet_read_pos - platform_midi_packet_write_pos;
    }
}

static int platform_midi_pop_packet(unsigned char *out, unsigned int size)
{
    if (platform_midi_packet_read_pos == platform_midi_packet_write_pos)
    {
        return 0;
    }

    struct platform_midi_packet_info *packet = &platform_midi_packets[platform_midi_packet_read_pos];
    int toCopy = (size < packet->length) ? size : packet->length;

    // TODO detect large events overrunning the buffer

    if (packet->offset + packet->length > PLATFORM_MIDI_EVENT_BUFFER_SIZE)
    {
        // event is split across the end and beginning of the buffer
        int endCopy = (packet->offset + toCopy <= PLATFORM_MIDI_EVENT_BUFFER_SIZE) ? toCopy : PLATFORM_MIDI_EVENT_BUFFER_SIZE - packet->offset;
        int startCopy = packet->length - endCopy;

        memcpy(out, &platform_midi_buffer[packet->offset], endCopy);
        memcpy(out + endCopy, platform_midi_buffer, startCopy);
    }
    else
    {
        memcpy(out, &platform_midi_buffer[packet->offset], toCopy);
    }

    platform_midi_buffer_offset = (platform_midi_buffer_offset + packet->length) % PLATFORM_MIDI_EVENT_BUFFER_SIZE;
    platform_midi_packet_read_pos = (platform_midi_packet_read_pos + 1) % PLATFORM_MIDI_EVENT_BUFFER_ITEMS;

    return toCopy;
}

static int platform_midi_buffer_init(void)
{
    platform_midi_packet_read_pos = 0;
    platform_midi_packet_write_pos = 0;
    platform_midi_buffer_offset = 0;
    platform_midi_buffer_end = 0;

    platform_midi_buffer = malloc(PLATFORM_MIDI_EVENT_BUFFER_SIZE);

    if (!platform_midi_buffer)
    {
        printf("Failed to allocate memory for Platform MIDI event buffer\n");
        return 0;
    }

    return 1;
}

static void platform_midi_buffer_deinit(void)
{
    free(platform_midi_buffer);
    platform_midi_buffer = 0;
}
#endif
#endif

#ifdef PLATFORM_MIDI_ALSA
#include "platform_midi_alsa.h"
#endif

#ifdef PLATFORM_MIDI_COREMIDI
#include "platform_midi_coremidi.h"
#endif

#ifdef PLATFORM_MIDI_WINMM
#include "platform_midi_winmm.h"
#endif

#endif
