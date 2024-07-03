// Copyright 2015-2020 <>< Charles Lohr under the MIT/x11, NewBSD or ColorChord License.  You choose.

#ifndef _SOUND_H
#define _SOUND_H

#define MAX_SOUND_DRIVERS 10

struct SoundDriver;

// NOTE: Some drivers have synchronous duplex mode, other drivers will use two different callbacks.  If ether is
// unavailable, it will be NULL. I.e. if `out` is null, only use in to read.  If in is null, only place samples in out.
typedef void (*SoundCBType)(struct SoundDriver* sd, short* in, short* out, int samplesr, int samplesp);

typedef void*(SoundInitFn)(SoundCBType cb, int reqSPS, int reqChannelsRec, int reqChannelsPlay, int sugBufferSize,
                           const char* inputSelect, const char* outputSelect);

struct SoundDriver
{
    void (*CloseFn)(void* object);
    int (*SoundStateFn)(void* object);
    SoundCBType callback;
    short channelsPlay;
    short channelsRec;
    int sps;
    void* opaque;

    // More fields may exist on a per-sound-driver basis
};

// Accepts:
// If DriverName = 0 or empty, will try to find best driver.
//
//  reqSPS = 44100 is guaranteed
//  reqChannelsRec = 1 or 2 guaranteed.
//  reqChannelsPlay = 1 or 2 guaranteed. NOTE: Some systems require ChannelsPlay == ChannelsRec!
//  sugBufferSize = No promises.
//  inputSelect = No standardization, NULL is OK for default.
//  outputSelect = No standardization, NULL is OK for default.

struct SoundDriver* InitSound(const char* driver_name, SoundCBType cb, int reqSPS, int reqChannelsRec,
                              int reqChannelsPlay, int sugBufferSize, const char* inputSelect,
                              const char* outputSelect);

int SoundState(struct SoundDriver*
                   soundobject); // returns bitmask.  1 if mic recording, 2 if play back running, 3 if both running.
void CloseSound(struct SoundDriver* soundobject);

// Called by various sound drivers.  Notice priority must be greater than 0.  Priority of 0 or less will not register.
// This is an internal function.  Applications shouldnot call it.
void RegSound(int priority, const char* name, SoundInitFn* fn);

#define REGISTER_SOUND(sounddriver, priority, name, function) \
    void REGISTER##sounddriver();                             \
    void __attribute__((constructor)) REGISTER##sounddriver() \
    {                                                         \
        RegSound(priority, name, function);                   \
    }

#endif
