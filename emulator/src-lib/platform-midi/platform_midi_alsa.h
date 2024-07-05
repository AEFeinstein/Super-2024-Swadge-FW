#ifndef _PLATFORM_MIDI_ALSA_H_
#define _PLATFORM_MIDI_ALSA_H_

#include <alsa/asoundlib.h>
#include <stdio.h>

int platform_midi_init_alsa(const char* name);
void platform_midi_deinit_alsa(void);
int platform_midi_read_alsa(unsigned char * out, int size);
int platform_midi_avail_alsa(void);
int platform_midi_write_alsa(unsigned char* buf, int size);

#define PLATFORM_MIDI_INIT(name) platform_midi_init_alsa(name)
#define PLATFORM_MIDI_DEINIT() platform_midi_deinit_alsa()
#define PLATFORM_MIDI_READ(out, size) platform_midi_read_alsa(out, size)
#define PLATFORM_MIDI_AVAIL() platform_midi_avail_alsa()
#define PLATFORM_MIDI_WRITE(buf, size) platform_midi_write_alsa(buf, size)

#ifdef PLATFORM_MIDI_IMPLEMENTATION

snd_seq_t *seq_handle;
snd_midi_event_t *event_parser;
int in_port;
int alsa_init = 0;

int platform_midi_init_alsa(const char* name)
{
    if (0 != snd_seq_open(&seq_handle, "default", SND_SEQ_OPEN_INPUT, SND_SEQ_NONBLOCK))
    {
        // Error!
        printf("Failed to initialize ALSA driver\n");
        return 0;
    }

    printf("Sequncer initialized\n");

    if (0 != snd_seq_set_client_name(seq_handle, name))
    {
        printf("Failed to set client name\n");
    }

    printf("Client name set to %s\n", name);

    in_port = snd_seq_create_simple_port(seq_handle, "listen:in",
                      SND_SEQ_PORT_CAP_WRITE|SND_SEQ_PORT_CAP_SUBS_WRITE,
                      SND_SEQ_PORT_TYPE_APPLICATION);

    if (0 != snd_midi_event_new(64, &event_parser))
    {
        printf("Failed to create MIDI parser\n");
    }

    printf("Done initializing MIDI!\n");

    alsa_init = 1;
    return 1;
}

void platform_midi_deinit_alsa(void)
{
    if (alsa_init)
    {
        snd_midi_event_free(event_parser);
        event_parser = 0;

        snd_seq_delete_port(seq_handle, in_port);
        in_port = 0;

        snd_seq_close(seq_handle);
        seq_handle = 0;

        alsa_init = 0;
    }
}

int platform_midi_read_alsa(unsigned char * out, int size)
{
    snd_seq_event_t *ev = NULL;

    int result = snd_seq_event_input(seq_handle, &ev);
    if (result == -EAGAIN)
    {
        return 0;
    }

    long convertResult = snd_midi_event_decode(event_parser, out, size, ev);
    if (convertResult < 0)
    {
        printf("Err: couldn't convert ALSA event to MIDI: %ld\n", convertResult);
        return -1;
    }
    else
    {
        return convertResult;
    }
}

int platform_midi_avail_alsa(void)
{
    return snd_seq_event_input_pending(seq_handle, 1);
}

int platform_midi_write_alsa(unsigned char* buf, int size)
{
    // NYI
    return 0;
}
#endif

#endif
