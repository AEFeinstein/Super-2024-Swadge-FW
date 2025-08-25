#include "drums.h"
#include "swSynth.h"
#include "hdw-dac.h"
#include "midiNoteFreqs.h"
#include "midiUtil.h"
#include "cnfs.h"

#define USE_BAKED_DRUMS

#ifdef USE_BAKED_DRUMS
    #include "bakedDrums.h"
#endif

#ifndef USE_BAKED_DRUMS

    #define FREQ_HZ(whole) (((whole) & 0xFFFFu) << 16)
    #define FREQ_HZ_FRAC(flhz) \
        ((((uint32_t)(flhz)) << 16) | ((uint32_t)(((flhz) - ((float)((uint32_t)(flhz)))) * 65536.0)))

static int8_t linearNoiseImpulse(uint32_t length, uint32_t idx, bool* done);
static int8_t linearWaveImpulse(oscillatorShape_t shape, uq16_16 freq, uint32_t length, uint32_t idx, bool* done);
static inline int8_t sampleWaveAt(uint32_t idx, oscillatorShape_t shape, uq16_16 freq);
static inline int8_t sampleTable(uint32_t idx, int8_t* table, uint16_t len, uq16_16 freq);
static int16_t adrLerp(uint32_t tick, uint32_t attackTime, uint32_t decayTime, int16_t attackLevel, int16_t decayLevel);
static uq16_16 freqLerp(uint32_t tick, uint32_t len, uq16_16 start, uq16_16 end);
static uq16_16 tremolo(uint32_t tick, uint32_t period, uq16_16 freq, int8_t centRange);

/**
 * @brief Return random noise sampled with a linearly decreasing volume
 *
 * @param length The number of samples it will take for the volume to reach 0
 * @param idx The current sample
 * @param done A pointer to a boolean which, if non-NULL, will be set to true when the volume reaches 0
 * @return int8_t A signed 8-bit sample of noise
 */
static int8_t linearNoiseImpulse(uint32_t length, uint32_t idx, bool* done)
{
    int vol = ((length - 1) - idx) / (length >> 8);
    if (done && idx >= (length - 1))
    {
        *done = true;
    }

    return swSynthSampleWave(SHAPE_NOISE, idx) * vol / 256;
}

/**
 * @brief Return a wave sampled with a linearly decreasing volume
 *
 * @param shape The shape of the wave
 * @param freq The frequency to sample the wave at
 * @param length The number of samples it will take for the volume to reach 0
 * @param idx The current sample index
 * @param done A pointer to a boolean which, if non-NULL, will be set to true when the volume reaches 0
 * @return int8_t A signed 8-bit sample of the wave
 */
static int8_t linearWaveImpulse(oscillatorShape_t shape, uq16_16 freq, uint32_t length, uint32_t idx, bool* done)
{
    if (idx >= length)
    {
        if (done)
        {
            *done = true;
        }
        return 0;
    }

    // Convert the frequency into steps
    // shoulda just cast it to an accum
    uint8_t sampleAt = (idx * (freq / (DAC_SAMPLE_RATE_HZ >> 8)) >> 16) & 0xFF;
    int8_t wave      = swSynthSampleWave(shape, sampleAt);
    int vol          = ((length - 1) - idx) / (length >> 8);
    if (done && idx >= (length - 1))
    {
        *done = true;
    }

    int8_t res = wave * vol / 256;
    return res;
}

/**
 * @brief Sample a wave at a given frequency
 *
 * @param idx The offset of the tick to sample at
 * @param shape The shape of the wave to sample
 * @param freq The frequency at which to sample the wave
 * @return int8_t A signed 8-bit sample of the wave
 */
static inline int8_t sampleWaveAt(uint32_t idx, oscillatorShape_t shape, uq16_16 freq)
{
    uint8_t sampleAt = (idx * (freq / (DAC_SAMPLE_RATE_HZ >> 8)) >> 16) & 0xFF;
    int8_t wave      = swSynthSampleWave(shape, sampleAt);
    return wave;
}

static inline int8_t sampleTable(uint32_t idx, int8_t* table, uint16_t len, uq16_16 freq)
{
    uint16_t sampleAt = (idx * (freq / (DAC_SAMPLE_RATE_HZ >> 8)) >> 16) % len;
    return table[sampleAt];
}

/**
 * @brief Return a value which increases linearly from 0 to \c attackLevel over a period of \c attackTime and then
 * decreases linearly to \c decayLevel over a period of \c decayTime.
 *
 * @param tick The current tick number
 * @param attackTime The number of ticks it should take to reach attackLevel
 * @param decayTime The number of ticks, after attack, that it should
 * @param attackLevel The value to reach after attackTime ticks
 * @param decayLevel The value to reach decayTime ticks after attackLevel is reached
 * @return int16_t The result
 */
static int16_t adrLerp(uint32_t tick, uint32_t attackTime, uint32_t decayTime, int16_t attackLevel, int16_t decayLevel)
{
    if (tick < attackTime)
    {
        // lerp from
        return attackLevel * tick / (attackTime - 1);
    }
    else if (tick < (attackTime + decayTime))
    {
        return attackLevel - ((attackLevel - decayLevel) * (tick - attackTime) / (decayTime - 1));
    }
    else
    {
        return decayLevel;
    }
}

/**
 * @brief Linearly interpolate between two frequencies
 *
 * @param tick The current tick
 * @param len The number of ticks the transition will take
 * @param start The frequency at 0 ticks
 * @param end The frequency at \c end ticks
 * @return uq16_16 The frequency, linearly interpolated betwene \c start and \c end
 */
static uq16_16 freqLerp(uint32_t tick, uint32_t len, uq16_16 start, uq16_16 end)
{
    return start + (end - start) * tick / len;
}

static uq16_16 tremolo(uint32_t tick, uint32_t period, uq16_16 freq, int8_t centRange)
{
    int32_t cents = sampleWaveAt(tick, SHAPE_SINE, 16384 / period) * centRange / 128;
    return bendPitchFreq(freq, cents);
}

/**
 * @brief Return a value that linearly increases until a value is reached, then decays exponentially
 *
 * @param tick The current tick
 * @param attackTime The number of ticks it will take to reach \c attackLevel
 * @param halfLife The number of ticks it will take for the volume to halve once \c attackLevel is reached
 * @param attackLevel The value to return after \c attackTime
 * @return int16_t A value between 0 and \c attackLevel
 */
static int16_t linearAttackExpDecay(uint32_t tick, uint32_t attackTime, uint32_t halfLife, int16_t attackLevel)
{
    if (tick < attackTime)
    {
        return attackLevel * tick / (attackTime - 1);
    }
    else if ((tick - attackTime) / halfLife < 32)
    {
        // The condition is to make sure we don't do (1<<32) which is UB
        // And (n / UINT32_MAX)) is 0 for all reasonable inputs
        // And (1<<31) is UB for signed values, so make sure it's unsigned!
        return 256 * attackLevel / (1u << ((tick - attackTime) / halfLife)) / 256;
    }
    else
    {
        return 0;
    }
}

/**
 * @brief Sets the value of a pointer to a boolean to true once the specified tick is reached
 *
 * @param finishTime The tick at which \c done should be set to true
 * @param idx The current tick
 * @param done A pointer to the boolean value to set
 * @return int8_t 0, always
 */
static inline int8_t finishAt(uint32_t finishTime, uint32_t idx, bool* done)
{
    if (idx >= finishTime)
    {
        *done = true;
    }

    // return a 0 for convenience
    return 0;
}

// noiseVol=48, sineVol=256, len=8192, freq=G1

    #define TOM(idx, len, noiseVol, sineVol, freq, done)                                             \
        adrLerp(idx, (len) / 2, (len) / 2, noiseVol, 0) * swSynthSampleWave(NOISE, idx & 0xFF) / 256 \
            + adrLerp(idx, 128, ((len)-128), sineVol, 0) * sampleWaveAt(idx, SHAPE_SINE, freq) / 256 \
            + finishAt(len, idx, done)

    /* defaultDrumkitFunc() defines values in terms of samples at 32768hz
     * The actual sample rate is half that, so we double the sample index to match
     */
    #define DRUMKIT_SAMPLE_RATE_HZ 32768
    #define DRUMKIT_SAMPLE_FACTOR  (DRUMKIT_SAMPLE_RATE_HZ / DAC_SAMPLE_RATE_HZ)

#endif

int8_t defaultDrumkitFunc(percussionNote_t drum, uint32_t idx, bool* done, uint32_t scratch[4], void* data)
{
#ifdef USE_BAKED_DRUMS

    if (drum < ACOUSTIC_BASS_DRUM_OR_LOW_BASS_DRUM || drum > OPEN_TRIANGLE)
    {
        // Drum not implemented
        *done = true;
        return 0;
    }
    else
    {
        int32_t dIdx = drum - ACOUSTIC_BASS_DRUM_OR_LOW_BASS_DRUM;
        if (idx >= bakedDrumsLens[dIdx])
        {
            // Drum sample finished
            *done = true;
            return 0;
        }
        else
        {
            // Drum sample playing
            return bakedDrums[dIdx][idx];
        }
    }

#else

    // Speed up the IDX to match the actual sample rate
    idx *= DRUMKIT_SAMPLE_FACTOR;

    // Good for a lazer sound:
    // return adrLerp(idx, 128, (8192-128), 256, 0) * swSynthSampleWave(NOISE, idx) + finishAt(8192, idx, done);
    switch (drum)
    {
        // B2
        case ACOUSTIC_BASS_DRUM_OR_LOW_BASS_DRUM:
            return adrLerp(idx, 128, (16384 - 64), 256, 12) * linearWaveImpulse(SHAPE_SINE, FREQ_B0, 16384, idx, done)
                   / 256;

        // C2
        case ELECTRIC_BASS_DRUM_OR_HIGH_BASS_DRUM:
            return adrLerp(idx, 128, (16384 - 64), 256, 12) /*return linearAttackExpDecay(idx, 64, 4096, 256)*/
                   * linearWaveImpulse(SHAPE_SINE, FREQ_G0, 16384, idx, done) / 256;

        // C#2
        case SIDE_STICK:
            return linearNoiseImpulse(256, idx, done);

        // D2
        case ACOUSTIC_SNARE:
            return adrLerp(idx, 2048, 2048, 48, 0) * sampleWaveAt(idx, SHAPE_NOISE, FREQ_G1) / 256
                   + adrLerp(idx, 128, (4096 - 128), 256, 0) * sampleWaveAt(idx, SHAPE_SINE, FREQ_G1) / 256
                   + finishAt(4096, idx, done);

        // D#2
        case HAND_CLAP:
            return linearNoiseImpulse(3064, idx, done);

        // E2
        case ELECTRIC_SNARE_OR_RIMSHOT:
            // TODO make this different from ACOUSTIC_SNARE
            return adrLerp(idx, 2048, 2048, 48, 0) * sampleWaveAt(idx, SHAPE_NOISE, FREQ_G1) / 256
                   + adrLerp(idx, 128, (4096 - 128), 256, 0) * sampleWaveAt(idx, SHAPE_SINE, FREQ_G1) / 256
                   + finishAt(4096, idx, done);

        // F2
        case LOW_FLOOR_TOM:
            return TOM(idx, 8192, 48, 256, FREQ_G1, done);

        // F#2
        case CLOSED_HI_HAT:
            return adrLerp(idx, 128, 2048 - 256, 64, 0) * sampleWaveAt(idx, SHAPE_NOISE, FREQ_D_SHARP_8) / 256
                   + finishAt(2048, idx, done);

        // G2
        case HIGH_FLOOR_TOM:
            return TOM(idx, 8192, 48, 256, FREQ_B2, done);

        // G#2
        case PEDAL_HI_HAT:
            return adrLerp(idx, 4096 - 256, 256, 128, 0)
                       * sampleWaveAt(idx, SHAPE_NOISE, freqLerp(idx, 4096, FREQ_D_SHARP_8, FREQ_C7)) / 256
                   + finishAt(4096, idx, done);
            /*return adrLerp(idx, 512, 6192-512, 256, 0) * sampleWaveAt(idx, SHAPE_NOISE, FREQ_D_SHARP_8) / 256
                * adrLerp(idx, 6192-512, 512, 32, 0) * sampleWaveAt(idx, SHAPE_SINE, FREQ_A_SHARP_MINUS_1) / 16 *
               sampleWaveAt(idx, SHAPE_SINE, FREQ_D_SHARP_6) / 512
                + finishAt(6192, idx, done);*/

        // A3
        case LOW_TOM:
            return TOM(idx, 6144, 48, 256, FREQ_A2, done);

        // A#3
        case OPEN_HI_HAT:
            return adrLerp(idx, 256, 12288 - 256, 128, 0) * sampleWaveAt(idx, SHAPE_NOISE, FREQ_D_SHARP_8) / 256
                   + adrLerp(idx, 8192, (16384 - 8192), 32, 0)
                         * sampleWaveAt(idx, SHAPE_SINE, FREQ_C6 /*freqLerp(idx, 16384, FREQ_C3, FREQ_C6)*/) / 256
                   + finishAt(16384, idx, done);

        // B3
        case LOW_MID_TOM:
            return TOM(idx, 6144, 48, 256, FREQ_C2, done);

        // C3
        case HIGH_MID_TOM:
            return TOM(idx, 6144, 48, 256, FREQ_E2, done);

        // C#3
        // NOT DONE
        case CRASH_CYMBAL_1:
            return adrLerp(idx, 128, 24576 - 128, 256, 0) * sampleWaveAt(idx, SHAPE_NOISE, FREQ_C4) / 256
                   + adrLerp(idx, 8192, (24578 - 8912), 32, 0) * sampleWaveAt(idx, SHAPE_SINE, FREQ_C6) / 256
                   + finishAt(24578, idx, done);

        // D3
        case HIGH_TOM:
            return TOM(idx, 6144, 48, 256, FREQ_G2, done);

        // D#3
        // NOT DONE
        case RIDE_CYMBAL_1:
            return adrLerp(idx, 128, 16384, 32, 1) * sampleWaveAt(idx, SHAPE_NOISE, FREQ_B3) / 256
                   + adrLerp(idx, 8192, 32768 - 8912, 64, 0)
                         * sampleWaveAt(idx, SHAPE_SINE, tremolo(idx, 4096, FREQ_B4, 90)) / 256
                   + adrLerp(idx, 4096, 32768 - 4096, 24, 0)
                         * sampleWaveAt(idx, SHAPE_SINE, tremolo(idx, 256, FREQ_F_SHARP_3, 45)) / 256
                   + finishAt(32768, idx, done);
            break;

        // E3
        // NOT DONE
        case CHINESE_CYMBAL:
            break;

        // F3
        case RIDE_BELL:
            return adrLerp(idx, 64, 65536 - 64, 256, 0) * sampleWaveAt(idx, SHAPE_SINE, tremolo(idx, 4096, FREQ_F4, 90))
                       / 256 * sampleWaveAt(idx, SHAPE_SINE, FREQ_HZ(11)) / 256
                   + adrLerp(idx, 128, 65536 - 128, 128, 0) * sampleWaveAt(idx, SHAPE_SINE, FREQ_D3) / 256
                         * sampleWaveAt(idx, SHAPE_SINE, FREQ_HZ(17)) / 256
                   + finishAt(65536, idx, done);

        // F#3
        case TAMBOURINE:
            return adrLerp(idx, 128, 3032 - 128, 256, 0) * sampleWaveAt(idx, SHAPE_NOISE, FREQ_A6) / 256
                       * sampleWaveAt(idx, SHAPE_SINE, FREQ_HZ(16)) / 128
                   + adrLerp(idx, 1516, 1516, 48, 0)
                         * sampleWaveAt(idx, SHAPE_SINE, freqLerp(idx, 3032, FREQ_A4, FREQ_A7)) / 256
                   + finishAt(3032, idx, done);

        // G3
        case SPLASH_CYMBAL:
            return linearAttackExpDecay(idx, 64, 4096, 256) * sampleWaveAt(idx, SHAPE_NOISE, FREQ_G2) / 256
                   + finishAt(16384, idx, done);

        // G#3
        // NOT DONE
        case COWBELL:
            /*return linearWaveImpulse(SHAPE_NOISE, FREQ_E2, 4096, idx, done) / 2
                   + sampleWaveAt(idx, SHAPE_SINE, FREQ_E2) / 4 + sampleWaveAt(idx, SHAPE_SINE, FREQ_A2) / 8;*/

            return adrLerp(idx, 256, 4096 - 256, 256, 0)
                       * (sampleWaveAt(idx, SHAPE_NOISE, FREQ_D_SHARP_8) / 2
                          + sampleWaveAt(idx, SHAPE_SINE, freqLerp(idx, 4096, FREQ_F_SHARP_4, FREQ_D_SHARP_3)) / 4)
                       / 128
                   + finishAt(4096, idx, done);

        // A4
        // NOT DONE
        case CRASH_CYMBAL_2:
            // This should be some sort of crash cymbal, crash cymbal 2?
            return donutDrumkitFunc(60, idx, done, scratch, data);

        // A#4
        case VIBRASLAP:
            return sampleWaveAt(idx, SHAPE_SINE, freqLerp(idx, 8192, FREQ_HZ(24), FREQ_HZ(6)))
                       * sampleWaveAt(idx, SHAPE_NOISE, freqLerp(idx, 8192, FREQ_D4, FREQ_D1)) / 128
                   + finishAt(8192, idx, done);

        // B4
        case RIDE_CYMBAL_2:
            return adrLerp(idx, 128, 16384, 32, 1) * sampleWaveAt(idx, SHAPE_NOISE, FREQ_E3) / 256
                   + adrLerp(idx, 8192, 32768 - 8912, 64, 0)
                         * sampleWaveAt(idx, SHAPE_SINE, tremolo(idx, 4096, FREQ_E4, 90)) / 256
                   + adrLerp(idx, 4096, 32768 - 4096, 24, 0)
                         * sampleWaveAt(idx, SHAPE_SINE, tremolo(idx, 256, FREQ_A_SHARP_4, 45)) / 256
                   + finishAt(32768, idx, done);

        // C4
        // NOT DONE
        case HIGH_BONGO:
            return TOM(idx, 3000, 48, 256, FREQ_G4, done);

        // C#4
        // NOT DONE
        case LOW_BONGO:
            return TOM(idx, 3000, 72, 256, FREQ_E4, done);

        // D4
        // NOT DONE
        case MUTE_HIGH_CONGA:
            break;

        // D#4
        // NOT DONE
        case OPEN_HIGH_CONGA:
        {
            break;
        }

        // E4
        // NOT DONE
        case LOW_CONGA:
        {
            break;
        }

        // F4
        // NOT DONE
        case HIGH_TIMBALE:
        {
            break;
        }

        // F#4
        // NOT DONE
        case LOW_TIMBALE:
        {
            break;
        }

        // G4
        // NOT DONE
        case HIGH_AGOGO:
        {
            break;
        }

        // G#4
        // NOT DONE
        case LOW_AGOGO:
        {
            break;
        }

        // A5
        // NOT DONE
        case CABASA:
            // At .5Hz, we get a single half sine wave
            return sampleWaveAt(idx, SHAPE_SINE, FREQ_HZ_FRAC(.5 / 8.0))
                       * sampleWaveAt(idx, SHAPE_NOISE, FREQ_C_SHARP_7) / 128
                   + finishAt(32768 / 8, idx, done);

        // A#5
        // NOT DONE
        case MARACAS:
            // Just a single maraca shake
            return sampleWaveAt(idx, SHAPE_SAWTOOTH, FREQ_HZ(2)) * sampleWaveAt(idx, SHAPE_NOISE, FREQ_A3) / 256
                   + finishAt(3064, idx, done);

        // B5
        case SHORT_WHISTLE:
            return linearWaveImpulse(SHAPE_SINE, FREQ_C5, 4096, idx, done)
                   + sampleWaveAt(idx, SHAPE_SINE, FREQ_C7 + (idx << 16)) / 12;

        // C5
        case LONG_WHISTLE:
            return linearWaveImpulse(SHAPE_SINE, FREQ_C5, 8192, idx, done)
                   + sampleWaveAt(idx, SHAPE_SINE, FREQ_C7 + (idx << 16)) / 12;

        // C#5
        case SHORT_GUIRO:
            return sampleWaveAt(idx, SHAPE_SINE, FREQ_HZ(12))
                       * sampleWaveAt(idx, SHAPE_NOISE, freqLerp(idx, 4096, FREQ_C_SHARP_4, FREQ_C_SHARP_3)) / 256
                   + finishAt(1536, idx, done);

        // D5
        case LONG_GUIRO:
        {
            return sampleWaveAt(idx, SHAPE_SINE, FREQ_HZ(20))
                       * sampleWaveAt(idx, SHAPE_NOISE, freqLerp(idx, 4096, FREQ_C_SHARP_4, FREQ_C_SHARP_3)) / 256
                   + finishAt(4096, idx, done);
            break;
        }

        // D#5
        case CLAVES:
        {
            return adrLerp(idx, 256, 1024 - 256, 256, 0)
                       * (sampleWaveAt(idx, SHAPE_NOISE, FREQ_D_SHARP_8) / 8
                          + sampleWaveAt(idx, SHAPE_SINE, FREQ_E5 - (idx * 8)) / 4)
                       / 128
                   + finishAt(1024, idx, done);
            break;
        }

        // E5
        case HIGH_WOODBLOCK:
            // This is also a pretty good wood block:
            return adrLerp(idx, 256, 2048 - 256, 256, 0)
                       * (sampleWaveAt(idx, SHAPE_NOISE, FREQ_D_SHARP_8) / 2
                          + sampleWaveAt(idx, SHAPE_SINE, FREQ_F_SHARP_4 - (idx * 8)) / 4)
                       / 128
                   + finishAt(2048, idx, done);

        // F5
        case LOW_WOODBLOCK:
            return adrLerp(idx, 256, 2048 - 256, 256, 0)
                       * (sampleWaveAt(idx, SHAPE_NOISE, FREQ_D_SHARP_8) / 2
                          + sampleWaveAt(idx, SHAPE_SINE, FREQ_D_SHARP_4 - (idx * 8)) / 4)
                       / 128
                   + finishAt(2048, idx, done);

        // F#5
        // NOT DONE
        case MUTE_CUICA:
            break;

        // G5
        // NOT DONE
        case OPEN_CUICA:
            return linearWaveImpulse(SHAPE_SINE, freqLerp(idx, 4096, FREQ_A6, FREQ_B2), 4096, idx, done);

        // G#5
        case MUTE_TRIANGLE:
            return linearWaveImpulse(SHAPE_SINE, FREQ_A6, 1536, idx, done);

        // A6
        case OPEN_TRIANGLE:
            return linearWaveImpulse(SHAPE_SINE, FREQ_D_SHARP_7, 12288, idx, done);
    }

    *done = true;
    return 0;
#endif
}

#ifndef USE_BAKED_DRUMS

    #define DONUT_SAMPLE_RATE_HZ 8192
    #define SAMPLE_FACTOR        (DAC_SAMPLE_RATE_HZ / DONUT_SAMPLE_RATE_HZ)

// literally copied from the donut swadge
// TODO: have these be an instrument in a separate bank or something
static const uint8_t kit0_speed[] = {23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11};
static const uint16_t kit0_fade[] = {24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24};
static const uint8_t kit0_drop[]  = {23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11};

static const uint8_t kit1_speed[] = {14, 13, 16, 11, 10, 9, 8, 7, 6, 5, 4, 2, 2};
static const uint16_t kit1_fade[] = {128, 512, 8, 128, 128, 128, 128, 128, 255, 255, 255, 512, 1024};
static const uint8_t kit1_drop[]  = {5, 64, 4, 10, 10, 16, 16, 16, 24, 24, 24, 64, 128};

#endif

int8_t donutDrumkitFunc(percussionNote_t drum, uint32_t idx, bool* done, uint32_t scratch[4], void* data)
{
#ifdef USE_BAKED_DRUMS

    int32_t len           = 0;
    const int8_t* samples = NULL;
    if (ACOUSTIC_BASS_DRUM_OR_LOW_BASS_DRUM <= drum && drum <= LOW_MID_TOM)
    {
        len     = bakedDonutDrumsLens[0];
        samples = bakedDonutDrums[0];
    }
    else if (HIGH_MID_TOM <= drum && drum <= HIGH_BONGO)
    {
        len     = bakedDonutDrumsLens[1];
        samples = bakedDonutDrums[1];
    }
    else if (LOW_BONGO == drum)
    {
        // Colossus Roar
        len     = bakedDonutDrumsLens[2];
        samples = bakedDonutDrums[2];
    }
    else if (SHORT_WHISTLE == drum)
    {
        // MAG
        len     = bakedDonutDrumsLens[3];
        samples = bakedDonutDrums[3];
    }
    else if (LONG_WHISTLE == drum)
    {
        // FEST
        len     = bakedDonutDrumsLens[4];
        samples = bakedDonutDrums[4];
    }
    else
    {
        // not a real drum, ignore
        *done = true;
        return 0;
    }

    if (idx >= len)
    {
        *done = true;
        return 0;
    }
    else
    {
        return samples[idx];
    }

#else
    const uint8_t* speeds;
    const uint16_t* fades;
    const uint8_t* drops;
    uint8_t offset = 0;

    // That's right, remember King Donut? Well he's back! In drum form.
    if (ACOUSTIC_BASS_DRUM_OR_LOW_BASS_DRUM <= drum && drum <= LOW_MID_TOM)
    {
        speeds = kit0_speed;
        fades  = kit0_fade;
        drops  = kit0_drop;
        offset = drum - ACOUSTIC_BASS_DRUM_OR_LOW_BASS_DRUM;
    }
    else if (HIGH_MID_TOM <= drum && drum <= HIGH_BONGO)
    {
        speeds = kit1_speed;
        fades  = kit1_fade;
        drops  = kit1_drop;
        offset = drum - HIGH_MID_TOM;
    }
    else if (LOW_BONGO == drum)
    {
        // Colossus Roar
        static bool colossusLoaded           = false;
        static const uint8_t* colossusSample = NULL;
        static size_t colossusLen            = 0;
        if (!colossusLoaded)
        {
            colossusSample = cnfsGetFile(COLOSSUS_BIN, &colossusLen);
            colossusLoaded = true;
        }

        if (idx / SAMPLE_FACTOR >= (colossusLen - 1))
        {
            *done = true;
        }

        return (int)colossusSample[idx / SAMPLE_FACTOR] - 128;
    }
    else if (SHORT_WHISTLE == drum)
    {
        // MAG
        static bool magLoaded           = false;
        static const uint8_t* magSample = NULL;
        static size_t magLen            = 0;
        if (!magLoaded)
        {
            // TODO: Use .raw instead of .bin, and try on-the-fly
            magSample = cnfsGetFile(DONUT_MAG_BIN, &magLen);
            magLoaded = true;
        }

        // Set done to true on or after the last sample
        if (idx / SAMPLE_FACTOR >= (magLen - 1))
        {
            *done = true;
        }

        return magSample[idx / SAMPLE_FACTOR];
    }
    else if (LONG_WHISTLE == drum)
    {
        // FEST
        static bool festLoaded           = false;
        static const uint8_t* festSample = NULL;
        static size_t festLen            = 0;
        if (!festLoaded)
        {
            festSample = cnfsGetFile(DONUT_FEST_BIN, &festLen);
            festLoaded = true;
        }

        if (idx / SAMPLE_FACTOR >= (festLen - 1))
        {
            *done = true;
        }

        return festSample[idx / SAMPLE_FACTOR];
    }
    else
    {
        // not a real drum, ignore
        *done = true;
        return 0;
    }

    // convert real ticks into 'drum ticks' after dividing to compensate for being faster than the atmega168
    uint32_t ticks = idx / speeds[offset];
    int8_t wave    = 0;

    if (idx % speeds[offset])
    {
        wave = ((int)(scratch[0])) - 128;
    }
    else
    {
        wave       = swSynthSampleWave(SHAPE_NOISE, idx);
        scratch[0] = wave + 128;
    }

    int volume = 200;
    if (ticks > fades[offset])
    {
        volume -= 2 * (ticks - fades[offset]) / drops[offset];
    }

    if (volume == 0)
    {
        *done = true;
    }

    return wave * volume >> 8;
#endif
}

#ifdef BAKE_DRUMS

    #include <stdio.h>
    #include <inttypes.h>

const char* getDrumName(percussionNote_t n)
{
    const char* names[] = {"ACOUSTIC_BASS_DRUM_OR_LOW_BASS_DRUM",
                           "ELECTRIC_BASS_DRUM_OR_HIGH_BASS_DRUM",
                           "SIDE_STICK",
                           "ACOUSTIC_SNARE",
                           "HAND_CLAP",
                           "ELECTRIC_SNARE_OR_RIMSHOT",
                           "LOW_FLOOR_TOM",
                           "CLOSED_HI_HAT",
                           "HIGH_FLOOR_TOM",
                           "PEDAL_HI_HAT",
                           "LOW_TOM",
                           "OPEN_HI_HAT",
                           "LOW_MID_TOM",
                           "HIGH_MID_TOM",
                           "CRASH_CYMBAL_1",
                           "HIGH_TOM",
                           "RIDE_CYMBAL_1",
                           "CHINESE_CYMBAL",
                           "RIDE_BELL",
                           "TAMBOURINE",
                           "SPLASH_CYMBAL",
                           "COWBELL",
                           "CRASH_CYMBAL_2",
                           "VIBRASLAP",
                           "RIDE_CYMBAL_2",
                           "HIGH_BONGO",
                           "LOW_BONGO",
                           "MUTE_HIGH_CONGA",
                           "OPEN_HIGH_CONGA",
                           "LOW_CONGA",
                           "HIGH_TIMBALE",
                           "LOW_TIMBALE",
                           "HIGH_AGOGO",
                           "LOW_AGOGO",
                           "CABASA",
                           "MARACAS",
                           "SHORT_WHISTLE",
                           "LONG_WHISTLE",
                           "SHORT_GUIRO",
                           "LONG_GUIRO",
                           "CLAVES",
                           "HIGH_WOODBLOCK",
                           "LOW_WOODBLOCK",
                           "MUTE_CUICA",
                           "OPEN_CUICA",
                           "MUTE_TRIANGLE",
                           "OPEN_TRIANGLE"};
    return names[n - ACOUSTIC_BASS_DRUM_OR_LOW_BASS_DRUM];
}

/**
 * @brief Call this function to print out all the drum samples
 *
 * This "bakes" the drums in, rather than calculating them on the fly
 *
 * It should be run on the emulator, not firmware.
 */
void bakeDrums(void)
{
    printf("#include \"bakedDrums.h\"\n\n");
    int32_t lengths[OPEN_TRIANGLE - ACOUSTIC_BASS_DRUM_OR_LOW_BASS_DRUM + 1] = {0};
    for (percussionNote_t n = ACOUSTIC_BASS_DRUM_OR_LOW_BASS_DRUM; n <= OPEN_TRIANGLE; n++)
    {
        int32_t idx         = 0;
        bool done           = false;
        uint32_t scratch[4] = {0};
        printf("static const int8_t %s_SAMPLES[] = {\n", getDrumName(n));
        while (!done)
        {
            int8_t sample = defaultDrumkitFunc(n, idx, &done, scratch, NULL);
            if (!done)
            {
                printf("%" PRId8 ", ", sample);
                idx++;
            }
        }
        printf("};\n\n");

        lengths[n - ACOUSTIC_BASS_DRUM_OR_LOW_BASS_DRUM] = idx;
    }

    printf("const int8_t* bakedDrums[] = {\n");
    for (percussionNote_t n = ACOUSTIC_BASS_DRUM_OR_LOW_BASS_DRUM; n <= OPEN_TRIANGLE; n++)
    {
        printf("%s_SAMPLES, ", getDrumName(n));
    }
    printf("};\n");

    printf("const int32_t bakedDrumsLens[] = {\n");
    for (percussionNote_t n = ACOUSTIC_BASS_DRUM_OR_LOW_BASS_DRUM; n <= OPEN_TRIANGLE; n++)
    {
        printf("%" PRId32 ", ", lengths[n - ACOUSTIC_BASS_DRUM_OR_LOW_BASS_DRUM]);
    }
    printf("};\n");

    #define NUM_DONUT_DRUMS 5
    percussionNote_t donutDrums[NUM_DONUT_DRUMS]
        = {ACOUSTIC_BASS_DRUM_OR_LOW_BASS_DRUM, HIGH_MID_TOM, LOW_BONGO, SHORT_WHISTLE, LONG_WHISTLE};

    int32_t donutLengths[NUM_DONUT_DRUMS] = {0};

    for (int32_t nIdx = 0; nIdx < NUM_DONUT_DRUMS; nIdx++)
    {
        percussionNote_t n = donutDrums[nIdx];

        int32_t idx         = 0;
        bool done           = false;
        uint32_t scratch[4] = {0};
        printf("static const int8_t DONUT_%d_SAMPLES[] = {\n", nIdx);
        while (!done)
        {
            int8_t sample = donutDrumkitFunc(n, idx, &done, scratch, NULL);
            if (!done)
            {
                printf("%" PRId8 ", ", sample);
                idx++;
            }
        }
        donutLengths[nIdx] = idx;
        printf("};\n");
    }

    printf("const int8_t* bakedDonutDrums[] = {\n");
    for (int32_t nIdx = 0; nIdx < NUM_DONUT_DRUMS; nIdx++)
    {
        printf("DONUT_%d_SAMPLES,", nIdx);
    }
    printf("};\n");

    printf("const int32_t bakedDonutDrumsLens[] = {\n");
    for (int32_t nIdx = 0; nIdx < NUM_DONUT_DRUMS; nIdx++)
    {
        printf("%" PRId32 ", ", donutLengths[nIdx]);
    }
    printf("};\n");
}

#endif

int8_t mmxDrumkitFunc(percussionNote_t drum, uint32_t idx, bool* done, uint32_t scratch[4], void* data)
{
    switch ((int)drum)
    {
        case ELECTRIC_BASS_DRUM_OR_HIGH_BASS_DRUM:
        {
            // Kick
            static bool kickLoaded           = false;
            static const uint8_t* kickSample = NULL;
            static size_t kickLen            = 0;
            if (!kickLoaded)
            {
                kickSample = cnfsGetFile(KICK_BIN, &kickLen);
                kickLoaded = true;
            }

            if (idx >= kickLen - 1)
            {
                *done = true;
            }

            return kickSample[idx];
        }

        case SIDE_STICK:
        {
            // High-Q
            static bool highqLoaded           = false;
            static const uint8_t* highqSample = NULL;
            static size_t highqLen            = 0;
            if (!highqLoaded)
            {
                highqSample = cnfsGetFile(HIGHQ_BIN, &highqLen);
                highqLoaded = true;
            }

            if (idx >= highqLen - 1)
            {
                // TODO loop
                *done = true;
            }

            return highqSample[idx];
        }

        case ACOUSTIC_SNARE:
        case HAND_CLAP:
        case ELECTRIC_SNARE_OR_RIMSHOT:
        {
            // Snare
            static bool snareLoaded           = false;
            static const uint8_t* snareSample = NULL;
            static size_t snareLen            = 0;
            if (!snareLoaded)
            {
                snareSample = cnfsGetFile(SNARE_BIN, &snareLen);
                snareLoaded = true;
            }

            if (idx >= snareLen - 1)
            {
                // TODO loop
                *done = true;
            }

            return snareSample[idx];
        }

        case LOW_FLOOR_TOM:
        {
            // PowerSnare
            // Loop!
            // loop_start=2095
            // loop_end=5087
            // end=5087
            // 2095 * 16384 / 37760 = ??? idk that's why we have computer
            static bool powerSnareLoaded           = false;
            static const uint8_t* powerSnareSample = NULL;
            static size_t powerSnareLen            = 0;
            const size_t loopStart                 = 2095 * 16384 / 37760;
            const size_t loopEnd                   = 5087 * 16384 / 37760;
            const size_t hold                      = 0 * 16384 + (8401 * 16384 / 100000);
            const size_t decay                     = 0 * 16384 + (77111 * 16384 / 100000);
            if (!powerSnareLoaded)
            {
                powerSnareSample = cnfsGetFile(POWERSNARE_BIN, &powerSnareLen);
                powerSnareLoaded = true;
            }

            int level = 256;
            if (idx > powerSnareLen)
            {
                if (idx > hold)
                {
                    level *= (decay);
                    level /= (idx - hold);

                    if (idx > hold + decay)
                    {
                        *done = true;
                    }
                }

                idx = (idx - powerSnareLen) % (loopEnd - loopStart) + loopStart;
                //*done = true;
            }

            return (powerSnareSample[idx] * level) >> 8;
        }

        case OPEN_HI_HAT:
        case LOW_MID_TOM:
        case HIGH_MID_TOM:
        case CRASH_CYMBAL_1:
        case HIGH_TOM:
        case RIDE_CYMBAL_1:
        case CHINESE_CYMBAL:
        case RIDE_BELL:
        case TAMBOURINE:
        case SPLASH_CYMBAL:
        {
            // openHiHat
            static bool openHiHatLoaded           = false;
            static const uint8_t* openHiHatSample = NULL;
            static size_t openHiHatLen            = 0;
            const size_t loopStart                = 47 * 16384 / 37760;
            const size_t loopEnd                  = 2543 * 16384 / 37760;
            const size_t decay                    = 2 * 16384 + (6456 * 16384 / 100000);

            if (!openHiHatLoaded)
            {
                openHiHatSample = cnfsGetFile(OPENHIHAT_BIN, &openHiHatLen);
                openHiHatLoaded = true;
            }

            int level = 256;
            if (idx > openHiHatLen)
            {
                level *= decay;
                level /= idx;
                if (idx >= decay)
                {
                    *done = true;
                }
                idx = (idx - openHiHatLen) % (loopEnd - loopStart) + loopStart;
                //*done = true;
            }

            return (openHiHatSample[idx] * level) >> 8;
        }

        case COWBELL:
        case CRASH_CYMBAL_2:
        case VIBRASLAP:
        case RIDE_CYMBAL_2:
        case HIGH_BONGO:
        case LOW_BONGO:
        case MUTE_HIGH_CONGA:
        case OPEN_HIGH_CONGA:
        case LOW_CONGA:
        case HIGH_TIMBALE:
        case LOW_TIMBALE:
        case HIGH_AGOGO:
        case LOW_AGOGO:
        case CABASA:
        case MARACAS:
        case SHORT_WHISTLE:
        case LONG_WHISTLE:
        {
            // crashCymbal
            static bool crashCymbalLoaded           = false;
            static const uint8_t* crashCymbalSample = NULL;
            static size_t crashCymbalLen            = 0;
            const size_t loopStart                  = 2095;
            const size_t loopEnd                    = 4575;
            const size_t hold                       = 0 * 16384 + (2090 * 16384 / 100000);
            const size_t decay                      = 3 * 16384 + (4482 * 16384 / 100000);
            if (!crashCymbalLoaded)
            {
                crashCymbalSample = cnfsGetFile(CRASHCYMBAL_BIN, &crashCymbalLen);
                crashCymbalLoaded = true;
            }

            int level = 256;
            if (idx > crashCymbalLen)
            {
                if (idx > hold)
                {
                    level *= (decay);
                    level /= (idx - hold);

                    if (idx > hold + decay)
                    {
                        *done = true;
                    }
                }
                idx = (idx - crashCymbalLen) % (loopEnd - loopStart) + loopStart;
                //*done = true;
            }

            return (crashCymbalSample[idx] * level) >> 8;
        }

        case SHORT_GUIRO: // 73
        case LONG_GUIRO:
        case CLAVES:
        case HIGH_WOODBLOCK:
        case LOW_WOODBLOCK:
        case MUTE_CUICA:
        case OPEN_CUICA:
        case MUTE_TRIANGLE:
        case OPEN_TRIANGLE:
        case 82:
        case 83:
        case 84:
        case 85:
        {
            // synthTom
            static bool synthTomLoaded           = false;
            static const uint8_t* synthTomSample = NULL;
            static size_t synthTomLen            = 0;
            if (!synthTomLoaded)
            {
                synthTomSample = cnfsGetFile(SYNTHTOM_BIN, &synthTomLen);
                synthTomLoaded = true;
            }

            if (idx >= synthTomLen - 1)
            {
                *done = true;
            }

            return synthTomSample[idx];
        }

        case 86:
        case 87:
        case 88:
        case 89:
        case 90:
        case 91:
        case 92:
        case 93:
        case 94:
        case 95:
        case 96:
        case 97:
        case 98:
        case 99:
        case 100:
        case 101:
        case 102:
        {
            // tom
            static bool tomLoaded           = false;
            static const uint8_t* tomSample = NULL;
            static size_t tomLen            = 0;
            if (!tomLoaded)
            {
                tomSample = cnfsGetFile(TOM_BIN, &tomLen);
                tomLoaded = true;
            }

            if (idx >= tomLen - 1)
            {
                *done = true;
            }

            return tomSample[idx];
        }

        default:
        {
            *done = true;
            return 0;
        }
    }
}
