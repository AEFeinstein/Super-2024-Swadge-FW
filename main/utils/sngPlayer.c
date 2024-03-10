#include "sngPlayer.h"
#include <inttypes.h>

song_t* playingSong;

synthOscillator_t oscs[2];
uint32_t samplesRemaining[2];
uint32_t cNoteIdx[2];

void playSongSpk(song_t* song)
{
    playingSong = song;

    swSynthSetShape(&oscs[0], SHAPE_SINE);
    swSynthSetShape(&oscs[1], SHAPE_SAWTOOTH);

    for (int tIdx = 0; tIdx < playingSong->numTracks; tIdx++)
    {
        oscs[tIdx].cVol = 255;
        swSynthSetVolume(&oscs[tIdx], 255);
        swSynthSetFade(&oscs[tIdx], NOT_FADING);

        samplesRemaining[tIdx] = 0;
        cNoteIdx[tIdx]         = 0;

        songTrack_t* track  = &playingSong->tracks[tIdx];
        musicalNote_t* note = &track->notes[cNoteIdx[tIdx]];

        samplesRemaining[tIdx] = (note->timeMs * AUDIO_SAMPLE_RATE_HZ) / 1000;
        if (SILENCE == note->note)
        {
            swSynthSetFade(&oscs[tIdx], FADING_OUT);
        }
        else
        {
            swSynthSetFreq(&oscs[tIdx], note->note);
            swSynthSetFade(&oscs[tIdx], FADING_IN);
        }
    }
}

void sngPlayerFillBuffer(uint8_t* samples, int16_t len)
{
    for (int32_t sIdx = 0; sIdx < len; sIdx++)
    {
        samples[sIdx] = swSynthMixOscillators(oscs, playingSong->numTracks);

        for (int32_t tIdx = 0; tIdx < playingSong->numTracks; tIdx++)
        {
            samplesRemaining[tIdx]--;
            if (0 == samplesRemaining[tIdx])
            {
                songTrack_t* track = &playingSong->tracks[tIdx];

                cNoteIdx[tIdx] = (cNoteIdx[tIdx] + 1) % track->numNotes;

                musicalNote_t* note = &track->notes[cNoteIdx[tIdx]];

                samplesRemaining[tIdx] = (note->timeMs * AUDIO_SAMPLE_RATE_HZ) / 1000;
                if (SILENCE == note->note)
                {
                    swSynthSetFade(&oscs[tIdx], FADING_OUT);
                }
                else
                {
                    swSynthSetFreq(&oscs[tIdx], note->note);
                    swSynthSetFade(&oscs[tIdx], FADING_IN);
                }
            }
        }
    }
}
