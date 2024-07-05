#ifndef _PLATFORM_MIDI_WINDOWS_H_
#define _PLATFORM_MIDI_WINDOWS_H_

int platform_midi_init_winmm(const char* name);
void platform_midi_deinit_winmm(void);
int platform_midi_read_winmm(unsigned char * out, int size);
int platform_midi_avail_winmm(void);
int platform_midi_write_winmm(unsigned char* buf, int size);

#define PLATFORM_MIDI_INIT(name) platform_midi_init_winmm(name)
#define PLATFORM_MIDI_DEINIT() platform_midi_deinit_winmm()
#define PLATFORM_MIDI_READ(out, size) platform_midi_read_winmm(out, size)
#define PLATFORM_MIDI_AVAIL() platform_midi_avail_winmm()
#define PLATFORM_MIDI_WRITE(buf, size) platform_midi_write_winmm(buf, size)

#ifdef PLATFORM_MIDI_IMPLEMENTATION
#include <windows.h>
#include <WinDef.h>
#include <IntSafe.h>
#include <BaseTsd.h>
#include <mmeapi.h>

LPHMIDIIN phmi;

void CALLBACK platform_midi_winmm_callback(HMIDIIN midiIn, UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    // All the MIDI data is in dwParam1
    // TODO: Should dwParam1 be dereferenced?
    unsigned char status = dwParam1 & 0xFF;
    unsigned char dataByte1 = (dwParam1 & 0xFF00) >> 8;
    unsigned char dataByte2 = (dwParam1 & 0xFF0000) >> 16;

    unsigned int packetLen = 1;

    // This API expands all running status bytes, so don't worry about that
    switch ((status & 0xF0) >> 4)
    {
        case 0xF:
        {
            if (status == 0xF1 || status == 0xF3)
            {
                packetLen = 2;
            }
            else if (status == 0xF2)
            {
                packetLen = 3;
            }
            else
            {
                packetLen = 1;
            }
            break;
        }

        case 0xC:
        case 0xD:
        {
            packetLen = 2;
            break;
        }

        default:
        {
            packetLen = 3;
            break;
        }
    }

    unsigned char out[3];
    out[0] = status;
    if (packetLen > 1)
    {
        out[1] = dataByte1;

        if (packetLen > 2)
        {
            out[2] = dataByte2;
        }
    }

    platform_midi_push_packet(out, packetLen);
}


int platform_midi_init_winmm(const char* name)
{
    MMRESULT result = midiInOpen(&phmi, 0, platform_midi_winmm_callback, NULL, CALLBACK_FUNCTION);
    printf("midiInOpen() == %d\n", result);
    printf("midiInStart() == %d\n", midiInStart(phmi));
    return 0;
}

void platform_midi_deinit_winmm(void)
{
    printf("midiInStop() == %d\n", midiInStop(phmi));
    MMRESULT result = midiInClose(&phmi);
    phmi = 0;
    printf("platform_midi_deinit_winmm() result: %d\n", result);
}

int platform_midi_read_winmm(unsigned char * out, int size)
{
    return platform_midi_pop_packet(out, size);
}

int platform_midi_avail_winmm(void)
{
    return platform_midi_packet_count();
}

int platform_midi_write_winmm(unsigned char* buf, int size)
{
    printf("platform_midi_write_winmm() not implemented\n");
    return 0;
}

#endif

#endif
