#pragma once

#include "hdw-bzr.h"
#include "sngPlayer.h"

#if defined(CONFIG_SOUND_OUTPUT_SPEAKER)

    #define soundPlaySfx(song, channel)       spkSongPlay(0, song)
    #define soundPlayBgm(song, channel)       spkSongPlay(1, song)
    #define soundPlaySfxCb(song, channel, cb) spkSongPlayCb(0, song, cb)
    #define soundPlayBgmCb(song, channel, cb) spkSongPlayCb(1, song, cb)
    #define soundStop(reset)                  spkSongStop(reset)

    #define soundPause()  spkSongPause()
    #define soundResume() spkSongResume()

    #define soundSave()        spkSongSave()
    #define soundRestore(data) spkSongRestore(data)

    #define soundPlayNote(freq, track, vol) // TODO
    #define soundStopNote(track)            // TODO

#elif defined(CONFIG_SOUND_OUTPUT_BUZZER)

    #define soundPlaySfx(song, channel)       bzrPlaySfx(song, channel)
    #define soundPlayBgm(song, channel)       bzrPlayBgm(song, channel)
    #define soundPlaySfxCb(song, channel, cb) bzrPlaySfxCb(song, channel, cb)
    #define soundPlayBgmCb(song, channel, cb) bzrPlayBgmCb(song, channel, cb)
    #define soundStop(reset)                  bzrStop(reset)

    #define soundPause()  bzrPause()
    #define soundResume() bzrResume()

    #define soundSave()        bzrSave()
    #define soundRestore(data) bzrRestore(data)

    #define soundPlayNote(freq, track, vol) bzrPlayNote(freq, track, vol)
    #define soundStopNote(track)            bzrStopNote(track)

#endif
