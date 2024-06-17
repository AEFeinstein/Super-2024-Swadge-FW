#include "drums.h"
#include "swSynth.h"
#include "hdw-dac.h"
#include "midiNoteFreqs.h"

static int8_t linearNoiseImpulse(uint32_t length, uint32_t idx, bool* done);
static int8_t linearWaveImpulse(oscillatorShape_t shape, uq16_16 freq, uint32_t length, uint32_t idx, bool* done);
static inline int8_t sampleWaveAt(uint32_t idx, oscillatorShape_t shape, uq16_16 freq);
static int16_t adrLerp(uint32_t tick, uint32_t attackTime, uint32_t decayTime, int16_t attackLevel, int16_t decayLevel);
static uq16_16 freqLerp(uint32_t tick, uint32_t len, uq16_16 start, uq16_16 end);

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
    if (done && idx == (length - 1))
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
        return 0;
    }

    // Convert the frequency into steps
    // shoulda just cast it to an accum
    uint8_t sampleAt = (idx * (freq / (DAC_SAMPLE_RATE_HZ >> 8)) >> 16) & 0xFF;
    int8_t wave      = swSynthSampleWave(shape, sampleAt);
    int vol          = ((length - 1) - idx) / (length >> 8);
    if (done && idx == (length - 1))
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
        // The condition is to make sure we don't do  (1<<32) which is UB
        // And (n / UINT32_MAX)) is 0 for all reasonable inputs
        return 256 * attackLevel / (1 << ((tick - attackTime) / halfLife)) / 256;
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

int8_t defaultDrumkitFunc(percussionNote_t drum, uint32_t idx, bool* done, uint32_t scratch[4], void* data)
{
    // TODO I stopped the drums from working because they're kinda bad!
    //*done = true;
    // return 0;

    // Good for a lazer sound:
    // return adrLerp(idx, 128, (8192-128), 256, 0) * swSynthSampleWave(NOISE, idx) + finishAt(8192, idx, done);
    switch (drum)
    {
        case ACOUSTIC_BASS_DRUM_OR_LOW_BASS_DRUM:
            return adrLerp(idx, 128, (16384 - 64), 256, 12) * linearWaveImpulse(SHAPE_SINE, FREQ_B0, 16384, idx, done)
                   / 256; //+ (adrLerp(idx, 1024, 512, 32, 0) * linearWaveImpulse(SHAPE_SINE, FREQ_B_MINUS_1, 16384,
                          // idx, NULL) / 256);
            ;             // + ((idx > 100) ? 0 : (linearNoiseImpulse(1024, idx, NULL) >> 5));
            // Way too long
            return donutDrumkitFunc(drum, idx, done, scratch, data);

        case ELECTRIC_BASS_DRUM_OR_HIGH_BASS_DRUM:
            // TODO this is discontinuous and poppy at the start for some reason even though it shouldn't
            return adrLerp(idx, 128, (16384 - 64), 256, 12) /*return linearAttackExpDecay(idx, 64, 4096, 256)*/
                   * linearWaveImpulse(SHAPE_SINE, FREQ_G0, 16384, idx, done) / 256;
            // return adrLerp(idx, 64, (16384-64), 256, 0) * linearWaveImpulse(SHAPE_SINE, /* E1 */FREQ_E1, 16384, idx,
            // done) / 256;

            // Way too long
            // return donutDrumkitFunc(ELECTRIC_BASS_DRUM_OR_HIGH_BASS_DRUM, idx, done, scratch, data);

        case SIDE_STICK:
            return linearNoiseImpulse(256, idx, done);

        case ACOUSTIC_SNARE:
            // THIS SOUNDS REAL GOOD
            return adrLerp(idx, 2048, 2048, 48, 0) * sampleWaveAt(idx, SHAPE_NOISE, FREQ_G1) / 256
                   + adrLerp(idx, 128, (4096 - 128), 256, 0) * sampleWaveAt(idx, SHAPE_SINE, FREQ_G1) / 256
                   + finishAt(4096, idx, done);
            return linearNoiseImpulse(8192, idx, done);
            // Too low and long
            return donutDrumkitFunc(37, idx, done, scratch, data);

        case HAND_CLAP:
            return linearNoiseImpulse(3064, idx, done);

        case ELECTRIC_SNARE_OR_RIMSHOT:
            // just a copy of the acoustic snare for now
            return adrLerp(idx, 2048, 2048, 48, 0) * sampleWaveAt(idx, SHAPE_NOISE, FREQ_G1) / 256
                   + adrLerp(idx, 128, (4096 - 128), 256, 0) * sampleWaveAt(idx, SHAPE_SINE, FREQ_G1) / 256
                   + finishAt(4096, idx, done);
            // Generic drum, too low
            // return donutDrumkitFunc(38, idx, done, scratch, data); //{ break; }

        case LOW_FLOOR_TOM:
            return TOM(idx, 8192, 48, 256, FREQ_G1, done);

            return adrLerp(idx, 4096, 4096, 48, 0) * swSynthSampleWave(NOISE, idx & 0xFF) / 256
                   + adrLerp(idx, 128, (8192 - 128), 256, 0) * sampleWaveAt(idx, SHAPE_SINE, FREQ_G1) / 256
                   + finishAt(8192, idx, done);
            // Too long
            // return donutDrumkitFunc(39, idx, done, scratch, data); //{ break; }

        case CLOSED_HI_HAT:
            return adrLerp(idx, 128, 2048 - 256, 64, 0) * sampleWaveAt(idx, SHAPE_NOISE, FREQ_D_SHARP_8) / 256
                   + finishAt(2048, idx, done);
            // return linearNoiseImpulse(3064, idx, done);
            // return linearNoiseImpulse(3064, idx, done) / 8 + sampleWaveAt(idx, SHAPE_SINE, FREQ_D_SHARP_6 );

            // Some sort of cymbal maybe? very low and pixely though
            return donutDrumkitFunc(41, idx, done, scratch, data); //{ break; }

        case HIGH_FLOOR_TOM:
            return TOM(idx, 8192, 48, 256, FREQ_B2, done);
            return adrLerp(idx, 4096, 4096, 48, 0) * swSynthSampleWave(NOISE, idx & 0xFF) / 256
                   + adrLerp(idx, 128, (8192 - 128), 256, 0) * sampleWaveAt(idx, SHAPE_SINE, FREQ_B2) / 256
                   + finishAt(8192, idx, done);
            // Generic drum
            return donutDrumkitFunc(47, idx, done, scratch, data); //{ break; }

        case PEDAL_HI_HAT:
            return adrLerp(idx, 4096 - 256, 256, 128, 0)
                       * sampleWaveAt(idx, SHAPE_NOISE, freqLerp(idx, 4096, FREQ_D_SHARP_8, FREQ_C7)) / 256
                   + finishAt(4096, idx, done);
            /*return adrLerp(idx, 512, 6192-512, 256, 0) * sampleWaveAt(idx, SHAPE_NOISE, FREQ_D_SHARP_8) / 256
                * adrLerp(idx, 6192-512, 512, 32, 0) * sampleWaveAt(idx, SHAPE_SINE, FREQ_A_SHARP_MINUS_1) / 16 *
               sampleWaveAt(idx, SHAPE_SINE, FREQ_D_SHARP_6) / 512
                + finishAt(6192, idx, done);*/
            // Long-ish drum
            // return donutDrumkitFunc(42, idx, done, scratch, data); //{ break; }

        case LOW_TOM:
            return TOM(idx, 6144, 48, 256, FREQ_A2, done);
            // eh maybe, too long though
            // return donutDrumkitFunc(43, idx, done, scratch, data); //{ break; }

        case OPEN_HI_HAT:
            return adrLerp(idx, 256, 12288 - 256, 128, 0) * sampleWaveAt(idx, SHAPE_NOISE, FREQ_D_SHARP_8) / 256
                   + adrLerp(idx, 8192, (16384 - 8192), 32, 0)
                         * sampleWaveAt(idx, SHAPE_SINE, FREQ_C6 /*freqLerp(idx, 16384, FREQ_C3, FREQ_C6)*/) / 256
                   + finishAt(16384, idx, done);
            return donutDrumkitFunc(49, idx, done, scratch, data);
            // Generic drum, not good for this
            // return donutDrumkitFunc(44, idx, done, scratch, data); //{ break; }

        case LOW_MID_TOM:
            return TOM(idx, 6144, 48, 256, FREQ_C2, done);
            // Generic drum, too long
            // return donutDrumkitFunc(45, idx, done, scratch, data); //{ break; }

        case HIGH_MID_TOM:
            return TOM(idx, 6144, 48, 256, FREQ_E2, done);
            // Generic drum, too long
            // return donutDrumkitFunc(46, idx, done, scratch, data); //{ break; }

        case CRASH_CYMBAL_1:
            // This is the long boi, no it's not ok for a closed hi-hat...
            return donutDrumkitFunc(49, idx, done, scratch, data);

        case HIGH_TOM:
            return TOM(idx, 6144, 48, 256, FREQ_G2, done);
            // Maybe this one is OK
            return donutDrumkitFunc(48, idx, done, scratch, data); //{ break; }

        case RIDE_CYMBAL_1:
            // Longer drum, too noisy?
            return donutDrumkitFunc(40, idx, done, scratch, data); //{ break; }

        case CHINESE_CYMBAL:
            break;
            // Kick?
            // return donutDrumkitFunc(50, idx, done, scratch, data); //{ break; }

        case RIDE_BELL:
            break;
            // Generic drum, short and muted
            // return donutDrumkitFunc(51, idx, done, scratch, data); //{ break; }

        case TAMBOURINE:
            break;
            // Generic drum, very short and muted
            // return donutDrumkitFunc(52, idx, done, scratch, data); //{ break; }

        case SPLASH_CYMBAL:
            break;
            // Generic drum
            // return donutDrumkitFunc(53, idx, done, scratch, data); //{ break; }

        case COWBELL:
            break;
            // Generic drum
            // return donutDrumkitFunc(54, idx, done, scratch, data); //{ break; }

        case CRASH_CYMBAL_2:
            break;
            // This should be some sort of crash cymbal, crash cymbal 2?
            // return donutDrumkitFunc(60, idx, done, scratch, data); //{ break; }

        case VIBRASLAP:
            break;
            // Generic drum
            // return donutDrumkitFunc(56, idx, done, scratch, data); //{ break; }

        case RIDE_CYMBAL_2:
            break;
            // Also a tom, sounds more drum-y than most others
            // return donutDrumkitFunc(57, idx, done, scratch, data); //{ break; }

        case HIGH_BONGO:
            // Perhaps a tom
            return donutDrumkitFunc(58, idx, done, scratch, data); //{ break; }

        case LOW_BONGO:
            // Some sort of cymbal maybe, not too long
            return donutDrumkitFunc(59, idx, done, scratch, data); //{ break; }

        case MUTE_HIGH_CONGA:
            break;
            // We could probably use donut drum 55 for the gunshot SFX instrument
            // return donutDrumkitFunc(55, idx, done, scratch, data); //{ break; }

        case OPEN_HIGH_CONGA:
        {
            break;
        }
        case LOW_CONGA:
        {
            break;
        }
        case HIGH_TIMBALE:
        {
            break;
        }
        case LOW_TIMBALE:
        {
            break;
        }
        case HIGH_AGOGO:
        {
            break;
        }
        case LOW_AGOGO:
        {
            break;
        }
        case CABASA:
        {
            break;
        }
        case MARACAS:
        {
            break;
        }
        case SHORT_WHISTLE:
            return linearWaveImpulse(SHAPE_SINE, FREQ_C5, 4096, idx, done)
                   + sampleWaveAt(idx, SHAPE_SINE, FREQ_C7 + (idx << 16)) / 12;
        case LONG_WHISTLE:
            return linearWaveImpulse(SHAPE_SINE, FREQ_C5, 8192, idx, done)
                   + sampleWaveAt(idx, SHAPE_SINE, FREQ_C7 + (idx << 16)) / 12;
        case SHORT_GUIRO:
        {
            break;
        }
        case LONG_GUIRO:
        {
            break;
        }
        case CLAVES:
        {
            break;
        }

        case HIGH_WOODBLOCK:
            // This is also a pretty good wood block:
            /*return adrLerp(idx, 256, 2048-256, 256, 0) * (sampleWaveAt(idx, SHAPE_NOISE, FREQ_D_SHARP_8)
                + sampleWaveAt(idx, SHAPE_SINE, FREQ_D_SHARP_4 - (idx * 8)) / 4) / 256
                + finishAt(2048, idx, done);*/
            return linearWaveImpulse(SHAPE_NOISE, FREQ_E2, 2048, idx, done)
                   + sampleWaveAt(idx, SHAPE_SINE, FREQ_E2) / 4;

        case LOW_WOODBLOCK:
            return linearWaveImpulse(SHAPE_NOISE, FREQ_G0, 2048, idx, done)
                   + sampleWaveAt(idx, SHAPE_SINE, FREQ_G0) / 4;

        case MUTE_CUICA:
        {
            // Not 100% right yet, just leaving it here for now
            uint8_t sampleAt = ((idx * ((56 << 16) / (DAC_SAMPLE_RATE_HZ >> 8))) & 0xFF00) >> 8;
            int8_t sin       = swSynthSampleWave(SHAPE_SINE, sampleAt);
            int vol          = (8191 - idx) / (8192 / 256);
            if (idx == 8191)
            {
                *done = true;
            }

            int8_t res = sin * vol / 256;
            // return res;

            if (res > 64)
            {
                return 64 - res;
            }
            else if (res < -64)
            {
                return -64 + res;
            }
            else
            {
                return res;
            }
        }
        case OPEN_CUICA:
            return linearWaveImpulse(SHAPE_SINE, freqLerp(idx, 4096, FREQ_A6, FREQ_B2), 4096, idx, done);
        case MUTE_TRIANGLE:
            return linearWaveImpulse(SHAPE_SINE, FREQ_A6, 1536, idx, done);
        case OPEN_TRIANGLE:
            return linearWaveImpulse(SHAPE_SINE, FREQ_D_SHARP_7, 12288, idx, done);
    }

    *done = true;
    return 0;
}

// literally copied from the donut swadge
// TODO: have these be an instrument in a separate bank or something
static const uint8_t kit0_speed[] = {23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11};
static const uint16_t kit0_fade[] = {24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24};
static const uint8_t kit0_drop[]  = {23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11};

static const uint8_t kit1_speed[] = {14, 13, 16, 11, 10, 9, 8, 7, 6, 5, 4, 2, 2};
static const uint16_t kit1_fade[] = {128, 512, 8, 128, 128, 128, 128, 128, 255, 255, 255, 512, 1024};
static const uint8_t kit1_drop[]  = {5, 64, 4, 10, 10, 16, 16, 16, 24, 24, 24, 64, 128};

int8_t donutDrumkitFunc(percussionNote_t drum, uint32_t idx, bool* done, uint32_t scratch[4], void* data)
{
    uint8_t* speeds;
    uint8_t* fades;
    uint8_t* drops;
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
}