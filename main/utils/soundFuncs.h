/*! \file soundFuncs.h
 *
 * \section soundFuncs_design Design Philosophy
 *
 * Swadge hardware may have buzzers or a DAC driven speaker, but not both at the same time. These macros should be used
 * to play songs in a hardware-agnostic way. The configuration may be changed using <tt>idf.py menuconfig</tt> in the
 * "Swadge Configuration" menu's "Select Sound Output" option.
 *
 * See sngPlayer.h and hdw-bzr.h for the DAC speaker and buzzer respective functions that these macros route to.
 */

#pragma once

#include "midiPlayer.h"

#if defined(CONFIG_SOUND_OUTPUT_SPEAKER)

    /**
     * @brief Play a song as a sound effect. For buzzers, sound effects interrupt background music and may be played on
     * a specific channel. For the DAC speakers, both play at the same time and there is no channel selection.
     *
     * Calls spkSongPlay() or bzrPlaySfx()
     *
     * @param song    The song to play
     * @param channel The channel (L/R/Stereo) to play on, ignored for DAC speakers
     */
    #define soundPlaySfx(song, channel) globalMidiPlayerPlaySong(song, MIDI_SFX)

    /**
     * @brief Play a song as background music. For buzzers, background music may be interrupted by sound effects and may
     * be played on a specific channel. For the DAC speakers, both play at the same time and there is no channel
     * selection.
     *
     * Calls spkSongPlay() or bzrPlayBgm()
     *
     * @param song    The song to play
     * @param channel The channel (L/R/Stereo) to play on, ignored for DAC speakers
     */
    #define soundPlayBgm(song, channel) globalMidiPlayerPlaySong(song, MIDI_BGM)

    /**
     * @brief Just like soundPlaySfx(), but with a callback which is called when the song ends
     *
     * Calls spkSongPlayCb() or bzrPlaySfxCb()
     *
     * @param song    The song to play
     * @param channel The channel (L/R/Stereo) to play on, ignored for DAC speakers
     * @param cb      A callback called when the song finishes
     */
    #define soundPlaySfxCb(song, channel, cb) globalMidiPlayerPlaySongCb(song, MIDI_SFX, cb)

    /**
     * @brief Just like soundPlayBgm(), but with a callback which is called when the song ends
     *
     * Calls spkSongPlayCb() or bzrPlayBgmCb()
     *
     * @param song    The song to play
     * @param channel The channel (L/R/Stereo) to play on, ignored for DAC speakers
     * @param cb      A callback called when the song finishes
     */
    #define soundPlayBgmCb(song, channel, cb) globalMidiPlayerPlaySongCb(song, MIDI_BGM, cb)

    /**
     * @brief Stop all playing songs
     *
     * Calls spkSongStop() or bzrStop()
     *
     * @param reset true to clear out song data as well
     */
    #define soundStop(reset) globalMidiPlayerStop(reset)

    /**
     * @brief Pause all songs. This stops output and may be resumed from that point in the song later
     *
     * Calls spkSongPause() or bzrPause()
     */
    #define soundPause() globalMidiPlayerPauseAll()

    /**
     * @brief Resume all songs. This should be called after soundPause()
     *
     * Calls soundResume() or bzrResume()
     */
    #define soundResume() globalMidiPlayerResumeAll()

    /**
     * @brief Stop all songs and return a void* containing all the state. The state must be freed or restored with
     * soundRestore() later. This is useful to stop the audio output, play something completely different, and resume
     * the original output later.
     *
     * Calls globalMidiSave() or bzrSave()
     *
     * @return A void* containing song state
     */
    #define soundSave() globalMidiSave()

    /**
     * @brief Restore state and resume audio output
     *
     * Calls spkSongRestore() or bzrRestore()
     *
     * @param data The state saved with spkSongSave()
     */
    #define soundRestore(data) globalMidiRestore(data)

    /**
     * @brief Play a specific note
     *
     * Calls midiNoteOn() or bzrPlayNote()
     *
     * @param freq The frequency of the note to play
     * @param channel The channel (L/R/Stereo) to play on, ignored for DAC speakers
     * @param vol The volume of the note to play
     *
     */
    #define soundPlayNote(freq, channel, vol) midiNoteOn(globalMidiPlayerGet(channel), 0, freq, vol)

    /**
     * @brief Stop a specific note
     *
     * Calls spkStopNote() or bzrStopNote()
     *
     * @param channel The channel (L/R/Stereo) to stop, ignored for DAC speakers
     */
    #define soundStopNote(channel) midiNoteOff(globalMidiPlayerGet(channel), 0, freq, vol)

    /**
     * @brief Return the MIDI player used for SFX
     *
     */
    #define soundGetPlayerSfx() globalMidiPlayerGet(0)

    /**
     * @brief Return the MIDI player used for BGM
     *
     */
    #define soundGetPlayerBgm() globalMidiPlayerGet(1)

#elif defined(CONFIG_SOUND_OUTPUT_BUZZER)
    #error "Buzzer is no longer supported, get with the times!"
#endif
