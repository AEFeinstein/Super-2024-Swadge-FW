// Copyright 2015-2020 <>< Charles Lohr under the ColorChord License, MIT/x11 license or NewBSD Licenses.

#include "sound.h"
#include "os_generic.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)

    #include <windows.h>
    #include <mmsystem.h>

    #define BUFFS 3

struct SoundDriverWin
{
    // Standard header - must remain.
    void (*CloseFn)(void* object);
    int (*SoundStateFn)(void* object);
    SoundCBType callback;
    short channelsPlay;
    short channelsRec;
    int sps;
    void* opaque;

    char* sInputDev;
    char* sOutputDev;

    int buffer;
    int isEnding;
    int GOBUFFRec;
    int GOBUFFPlay;

    int recording;
    int playing;

    HWAVEIN hMyWaveIn;
    HWAVEOUT hMyWaveOut;
    WAVEHDR WavBuffIn[BUFFS];
    WAVEHDR WavBuffOut[BUFFS];
};

void* InitSoundWin(SoundCBType cb, int reqSPS, int reqChannelsRec, int reqChannelsPlay, int sugBufferSize,
                   const char* inputSelect, const char* outputSelect);
void CloseSoundWin(void* v);
int SoundStateWin(void* v);
void CALLBACK HANDLEMIC(HWAVEIN hwi, UINT umsg, DWORD dwi, DWORD hdr, DWORD dwparm);
void CALLBACK HANDLESINK(HWAVEIN hwi, UINT umsg, DWORD dwi, DWORD hdr, DWORD dwparm);

static struct SoundDriverWin* w;

void CloseSoundWin(void* v)
{
    struct SoundDriverWin* r = (struct SoundDriverWin*)v;
    int i;

    if (r)
    {
        if (r->hMyWaveIn)
        {
            waveInStop(r->hMyWaveIn); // TODO this is blocking???
            waveInReset(r->hMyWaveIn);
            for (i = 0; i < BUFFS; i++)
            {
                waveInUnprepareHeader(r->hMyWaveIn, &(r->WavBuffIn[i]), sizeof(WAVEHDR));
                free((r->WavBuffIn[i]).lpData);
            }
            waveInClose(r->hMyWaveIn);
        }

        if (r->hMyWaveOut)
        {
            waveOutPause(r->hMyWaveOut);
            waveOutReset(r->hMyWaveOut);

            for (i = 0; i < BUFFS; i++)
            {
                waveInUnprepareHeader(r->hMyWaveIn, &(r->WavBuffOut[i]), sizeof(WAVEHDR));
                free((r->WavBuffOut[i]).lpData);
            }
            waveInClose(r->hMyWaveIn);
            waveOutClose(r->hMyWaveOut);
        }
        free(r);
    }
}

int SoundStateWin(void* v)
{
    struct SoundDriverWin* soundobject = (struct SoundDriverWin*)v;

    return soundobject->recording | (soundobject->playing ? 2 : 0);
}

void CALLBACK HANDLEMIC(HWAVEIN hwi, UINT umsg, DWORD dwi, DWORD hdr, DWORD dwparm)
{
    int ctr;
    // int ob;
    long cValue;
    unsigned int maxWave = 0;

    if (w->isEnding)
        return;

    switch (umsg)
    {
        case MM_WIM_OPEN:
            printf("Mic Open.\n");
            w->recording = 1;
            break;

        case MM_WIM_DATA:
            // ob = (w->GOBUFFRec+(BUFFS))%BUFFS;
            w->callback((struct SoundDriver*)w, (short*)(w->WavBuffIn[w->GOBUFFRec]).lpData, 0, w->buffer, 0);
            waveInAddBuffer(w->hMyWaveIn, &(w->WavBuffIn[w->GOBUFFRec]), sizeof(WAVEHDR));
            w->GOBUFFRec = (w->GOBUFFRec + 1) % BUFFS;
            break;
        default:
            break;
    }
}

void CALLBACK HANDLESINK(HWAVEIN hwi, UINT umsg, DWORD dwi, DWORD hdr, DWORD dwparm)
{
    int ctr;
    long cValue;
    unsigned int maxWave = 0;

    if (w->isEnding)
        return;

    switch (umsg)
    {
        case MM_WOM_OPEN:
            printf("Sink Open.\n");
            w->playing = 1;
            break;

        case MM_WOM_DONE:
            w->callback((struct SoundDriver*)w, 0, (short*)(w->WavBuffOut[w->GOBUFFPlay]).lpData, 0, w->buffer);
            waveOutWrite(w->hMyWaveOut, &(w->WavBuffOut[w->GOBUFFPlay]), sizeof(WAVEHDR));
            w->GOBUFFPlay = (w->GOBUFFPlay + 1) % BUFFS;
            break;
        default:
            break;
    }
}

static struct SoundDriverWin* InitWinSound(struct SoundDriverWin* r)
{
    int i;
    WAVEFORMATEX wfmt;
    memset(&wfmt, 0, sizeof(wfmt));
    printf("WFMT Size (debugging temp for TCC): %d\n", (uint32_t)sizeof(wfmt));
    printf("WFMT: %d %d %d\n", r->channelsRec, r->sps, r->sps * r->channelsRec);
    w = r;

    wfmt.wFormatTag      = WAVE_FORMAT_PCM;
    wfmt.nChannels       = r->channelsRec;
    wfmt.nAvgBytesPerSec = r->sps * r->channelsRec;
    wfmt.nBlockAlign     = r->channelsRec * 2;
    wfmt.nSamplesPerSec  = r->sps;
    wfmt.wBitsPerSample  = 16;
    wfmt.cbSize          = 0;

    long dwdeviceR, dwdeviceP;
    char* ptr;
    dwdeviceR = r->sInputDev ? strtol(r->sInputDev, &ptr, 10) : (long)WAVE_MAPPER;
    dwdeviceP = r->sOutputDev ? strtol(r->sOutputDev, &ptr, 10) : (long)WAVE_MAPPER;

    if (r->channelsRec)
    {
        printf("In Wave Devs: %d; WAVE_MAPPER: %d; Selected Input: %ld\n", waveInGetNumDevs(), WAVE_MAPPER, dwdeviceR);
        int p = waveInOpen(&r->hMyWaveIn, dwdeviceR, &wfmt, (intptr_t)(&HANDLEMIC), 0, CALLBACK_FUNCTION);
        if (p)
        {
            fprintf(stderr, "Error performing waveInOpen.  Received code: %d\n", p);
        }
        printf("waveInOpen: %d\n", p);
        for (i = 0; i < BUFFS; i++)
        {
            memset(&(r->WavBuffIn[i]), 0, sizeof(r->WavBuffIn[i]));
            (r->WavBuffIn[i]).dwBufferLength = r->buffer * 2 * r->channelsRec;
            (r->WavBuffIn[i]).dwLoops        = 1;
            (r->WavBuffIn[i]).lpData         = (char*)calloc(1, r->buffer * r->channelsRec * 2);
            printf("buffer gen size: %d: %p\n", r->buffer * r->channelsRec * 2, (r->WavBuffIn[i]).lpData);
            p = waveInPrepareHeader(r->hMyWaveIn, &(r->WavBuffIn[i]), sizeof(WAVEHDR));
            printf("WIPr: %d\n", p);
            waveInAddBuffer(r->hMyWaveIn, &(r->WavBuffIn[i]), sizeof(WAVEHDR));
            printf("WIAr: %d\n", p);
        }
        p = waveInStart(r->hMyWaveIn);
        if (p)
        {
            fprintf(stderr, "Error performing waveInStart.  Received code %d\n", p);
        }
    }

    wfmt.nChannels       = r->channelsPlay;
    wfmt.nAvgBytesPerSec = r->sps * r->channelsPlay;
    wfmt.nBlockAlign     = r->channelsPlay * 2;

    if (r->channelsPlay)
    {
        printf("Out Wave Devs: %d; WAVE_MAPPER: %d; Selected Input: %ld\n", waveOutGetNumDevs(), WAVE_MAPPER,
               dwdeviceP);
        int p = waveOutOpen(&r->hMyWaveOut, dwdeviceP, &wfmt, (intptr_t)(void*)(&HANDLESINK), (intptr_t)r,
                            CALLBACK_FUNCTION);
        if (p)
        {
            fprintf(stderr, "Error performing waveOutOpen. Received code: %d\n", p);
        }
        printf("waveOutOpen: %d\n", p);
        for (i = 0; i < BUFFS; i++)
        {
            memset(&(r->WavBuffOut[i]), 0, sizeof(r->WavBuffOut[i]));
            (r->WavBuffOut[i]).dwBufferLength = r->buffer * 2 * r->channelsPlay;
            (r->WavBuffOut[i]).dwLoops        = 1;
            (r->WavBuffOut[i]).lpData         = (char*)calloc(1, r->buffer * r->channelsPlay * 2);
            p = waveOutPrepareHeader(r->hMyWaveOut, &(r->WavBuffOut[i]), sizeof(WAVEHDR));
            waveOutWrite(r->hMyWaveOut, &(r->WavBuffOut[i]), sizeof(WAVEHDR));
        }
    }

    return r;
}

void* InitSoundWin(SoundCBType cb, int reqSPS, int reqChannelsRec, int reqChannelsPlay, int sugBufferSize,
                   const char* inputSelect, const char* outputSelect)
{
    struct SoundDriverWin* r = (struct SoundDriverWin*)calloc(1, sizeof(struct SoundDriverWin));

    r->CloseFn      = CloseSoundWin;
    r->SoundStateFn = SoundStateWin;
    r->callback     = cb;
    r->sps          = reqSPS;
    r->channelsPlay = reqChannelsPlay;
    r->channelsRec  = reqChannelsRec;
    r->buffer       = sugBufferSize;
    r->sInputDev    = inputSelect ? strdup(inputSelect) : 0;
    r->sOutputDev   = outputSelect ? strdup(outputSelect) : 0;

    r->recording = 0;
    r->playing   = 0;
    r->isEnding  = 0;

    r->GOBUFFPlay = 0;
    r->GOBUFFRec  = 0;

    return InitWinSound(r);
}

REGISTER_SOUND(SoundWin, 10, "WIN", InitSoundWin);

#endif