// Copyright 2015 <>< Charles Lohr under the ColorChord License.

#include "sound.h"
#include "os_generic.h"
#include <stdlib.h>
// #include "parameters.h"

struct SoundDriverNull
{
    void (*CloseFn)(struct SoundDriverNull* object);
    int (*SoundStateFn)(struct SoundDriverNull* object);
    SoundCBType soundcb;
    int channelsPlay;
    int spsPlay;
    int channelsRec;
    int spsRec;
};

void* InitSoundNull(SoundCBType cb, int reqSPS, int reqChannelsRec, int reqChannelsPlay, int sugBufferSize,
                    const char* inputSelect, const char* outputSelect);
void CloseSoundNull(struct SoundDriverNull* object);
int SoundStateNull(struct SoundDriverNull* object);

void CloseSoundNull(struct SoundDriverNull* object)
{
    free(object);
}

int SoundStateNull(struct SoundDriverNull* object)
{
    return 0;
}

void* InitSoundNull(SoundCBType cb, int reqSPS, int reqChannelsRec, int reqChannelsPlay, int sugBufferSize,
                    const char* inputSelect, const char* outputSelect)
{
    struct SoundDriverNull* r = malloc(sizeof(struct SoundDriverNull));
    r->CloseFn                = CloseSoundNull;
    r->SoundStateFn           = SoundStateNull;
    r->soundcb                = cb;
    r->spsPlay                = 0; // GetParameterI( "samplerate", 44100 );
    r->channelsPlay           = 0; // GetParameterI( "channels", 2 );
    r->spsRec                 = r->spsPlay;
    r->channelsRec            = 0;
    return r;
}

REGISTER_SOUND(SoundNull, 1, "NULL", InitSoundNull);
