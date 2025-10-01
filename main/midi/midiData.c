#include "midiData.h"
#include "midiPlayer.h"

#include "waveTables.h"
#include "drums.h"

#include "macros.h"
#include "hdw-dac.h"
#include "midiNoteFreqs.h"

const midiTimbre_t acousticGrandPianoTimbre = {
    .type = WAVETABLE,
    .flags = TF_NONE,
    .waveIndex = 0,
    .waveFunc = waveTableFunc,
    .envelope = {
        // TODO: Just realized I forgot how ADSR actually works halfway through writing everything else...
        // Pretty fast attack
        .attackTime = 0,
        // Decrease attack time by 1ms for every 4 velocity value
        .attackTimeVel = 0,

        // Take a good long-ish while to reach the sustain level
        .decayTime = 0,
        // Take a bit longer the higher the velocity -- 25ms for every 16 velocity (so up to +200ms)
        .decayTimeVel = 0,

        // Sustain at 1 plus 75% of initial volume
        .sustainVol = 127,
        .sustainVolVel = 0,

        // And a not-too-short release time
        .releaseTime = 0,
        // Plus some extra time if the note was very loud initially - up to double
        .releaseTimeVel = 0,
        // Yup, I'm sure it will sound exactly like a grand piano now!
    },
    .effects = {
        .chorus = 0,
    },
    .name = "Acoustic Grand Piano",
};

const midiTimbre_t squareWaveTimbre = {
    .type = WAVE_SHAPE,
    .flags = TF_NONE,
    .shape = SHAPE_SQUARE,
    .envelope = {
        .sustainVol = 1,
        .sustainVolVel = 0x200,
    },
    .name = "Square Wave",
};

const midiTimbre_t sineWaveTimbre = {
    .type = WAVE_SHAPE,
    .flags = TF_NONE,
    .shape = SHAPE_SINE,
    .envelope = {
        .sustainVol = 1,
        .sustainVolVel = 0x200,
    },
    .name = "Sine Wave",
};

const midiTimbre_t triangleWaveTimbre = {
    .type = WAVE_SHAPE,
    .flags = TF_NONE,
    .shape = SHAPE_TRIANGLE,
    .envelope = {
        .sustainVol = 1,
        .sustainVolVel = 0x200,
    },
    .name = "Triangle Wave",
};

const midiTimbre_t sawtoothWaveTimbre = {
    .type = WAVE_SHAPE,
    .flags = TF_NONE,
    .shape = SHAPE_SAWTOOTH,
    .envelope = {
        .sustainVol = 1,
        .sustainVolVel = 0x200,
    },
    .name = "Sawtooth Wave",
};

const midiTimbre_t noiseTimbre = {
    .type = NOISE,
    .flags = TF_NONE,
    .envelope = {
        .sustainVol = 1,
        .sustainVolVel = 0x200,
    },
    .name = "Noise",
};

const midiTimbre_t magfestWaveTimbre = {
    .type = WAVETABLE,
    .flags = TF_NONE,
    .waveIndex = 0,
    .waveFunc = magfestWaveTableFunc,
    .envelope = {
        .attackTime = MS_TO_SAMPLES(8),
        // attack is .5(samples)
        .attackTimeVel = -0x80,
        .decayTime = MS_TO_SAMPLES(16),
        .decayTimeVel = 0,
        .sustainVol = 1,
        .sustainVolVel = 0x200,
        .releaseTime = MS_TO_SAMPLES(320),
        // the higher the velocity the longer the release time
        .releaseTimeVel = 0x800,
    },
    .effects = {
        .chorus = 0,
    },
    .name = "MAGFest Wave",
};

const midiTimbre_t magstockWaveTimbre = {
    .type = WAVETABLE,
    .flags = TF_NONE,
    .waveIndex = 1,
    .waveFunc = magfestWaveTableFunc,
    .envelope = {
        .attackTime = MS_TO_SAMPLES(8),
        // attack is .5(samples)
        .attackTimeVel = -0x80,
        .decayTime = MS_TO_SAMPLES(16),
        .decayTimeVel = 0,
        .sustainVol = 1,
        .sustainVolVel = 0x200,
        .releaseTime = MS_TO_SAMPLES(320),
        // the higher the velocity the longer the release time
        .releaseTimeVel = 0x800,
    },
    .effects = {
        .chorus = 0,
    },
    .name = "MAGStock Wave",
};

const midiTimbre_t squareHitWaveTimbre = {
    .type = WAVE_SHAPE,
    .flags = TF_NONE,
    .shape = SHAPE_SQUARE,
    .envelope = {
        .attackTime = 0,
        .attackTimeVel = 0,
        .decayTime = MS_TO_SAMPLES(125),
        .decayTimeVel = 0,
        .sustainVol = 0,
        .sustainVolVel = 0,
        .releaseTime = 0,
        .releaseTimeVel = 0,
    },
    .name = "Square Hit",
};

const midiTimbre_t noiseHitTimbre = {
    .type = WAVE_SHAPE,
    .flags = TF_NONE,
    .shape = SHAPE_NOISE,
    .envelope = {
        .attackTime = 0,
        .attackTimeVel = 0,
        .decayTime = MS_TO_SAMPLES(125),
        .decayTimeVel = 0,
        .sustainVol = 0,
        .sustainVolVel = 0,
        .releaseTime = 0,
        .releaseTimeVel = 0,
    },
    .name = "Noise Hit",
};

const midiTimbre_t colossusTimbre = {
    .type = SAMPLE,
    .flags = TF_NONE,
    .sample = {
        .fIdx = COLOSSUS_BIN,
        // Loop only once
        .loop = 1,
        //
        .rate = 8192,
        .baseNote = FREQ_C_SHARP_5,
        .tune = 0,
    },
    .name = "Colossus Roar",
};

const midiTimbre_t magTimbre = {
    .type = SAMPLE,
    .flags = TF_NONE,
    .sample = {
        .fIdx = DONUT_MAG_BIN,
        // Loop only once
        .loop = 1,
        //
        .rate = 8192,
        .baseNote = FREQ_A4,
        .tune = 0,
    },
    .name = "MAG",
};

const midiTimbre_t festTimbre = {
    .type = SAMPLE,
    .flags = TF_NONE,
    .sample = {
        .fIdx = DONUT_FEST_BIN,
        // Loop only once
        .loop = 1,
        //
        .rate = 8192,
        .baseNote = FREQ_A4,
        .tune = 0,
    },
    .name = "FEST",
};

const midiTimbre_t wilhelmTimbre = {
    .type = SAMPLE,
    .flags = TF_NONE,
    .sample = {
        .fIdx = WILHELM_BIN,
        // Loop only once
        .loop = 0,
        //
        .rate = 8192,
        .baseNote = FREQ_A4,
        .tune = 0,
    },
    .name = "Wilhelm",
};

const midiTimbre_t noriTimbre = {
    .type = SAMPLE,
    .flags = TF_NONE,
    .sample = {
        .fIdx = NORI_2_BIN,
        // Loop only once
        .loop = 0,
        //
        .rate = 8192,
        .baseNote = FREQ_A4,
        .tune = 0,
    },
    .name = "Nori",
};

const midiTimbre_t* const magfestTimbres[] = {
    &squareWaveTimbre,   &sineWaveTimbre, &triangleWaveTimbre,  &sawtoothWaveTimbre, &magfestWaveTimbre,
    &magstockWaveTimbre, &noiseTimbre,    &squareHitWaveTimbre, &noiseHitTimbre,     &colossusTimbre,
    &magTimbre,          &festTimbre,     &wilhelmTimbre,       &noriTimbre,
};

const size_t magfestTimbreCount = ARRAY_SIZE(magfestTimbres);

const midiTimbre_t defaultDrumkitTimbre = {
    .type = PLAY_FUNC,
    .flags = TF_PERCUSSION,
    .playFunc = {
        .func = defaultDrumkitFunc,
        // TODO: Define the data and put it here!
        .data = NULL,
    },
    .envelope = {
        .sustainVol = 0,
        .decayTime = UINT32_MAX,
    },
    .name = "Swadge Drums 0",
};

const midiTimbre_t donutDrumkitTimbre = {
    .type = PLAY_FUNC,
    .flags = TF_PERCUSSION,
    .playFunc = {
        .func = donutDrumkitFunc,
        // This should be set though
        .data = NULL,
    },
    .envelope = {
        .sustainVol = 127,
    },
    .name = "Donut Swadge Drums",
};

// convert original sample numbers to account for sample rate changing
// #define SAMPLE_NUM_CONV(count, origRate, targetRate) ((count) * (targetRate) / (origRate))
#define SECONDS_CONV(whole, microseconds) ((whole) * 16384 + (microseconds) * 16384u / 1000000u)
// #define PITCH_HZ(whole, thousandths)                 ((whole) << 16 | ((thousandths) * (1 << 16) / 1000))

const midiTimbre_t mmx011Vibraphone = {
    .type = SAMPLE,
    .flags = TF_NONE,
    .sample = {
        .fIdx = SF_MMX_011_VIBRAPHONE_BIN,
        .loop = 0,
        .loopStart = 64,
        .loopEnd = 96,
        .rate = 28928,
        // pitch keycenter=82, plus tune=50
        .baseNote = FREQ_A_SHARP_5,
        .tune = 50,
    },
    .name = "MMX Bell Synth",
    .envelope = {
        .attackTime = SECONDS_CONV(0, 1000), // 1ms
        // .releaseTime = SECONDS_CONV(2, 70530), // 2.070530s
        .releaseTime = SECONDS_CONV(0, 675300), // ~2.070530s / 4
        .sustainVol = 127,
    },
};

const midiTimbre_t mmx017Organ = {
    .type = SAMPLE,
    .flags = TF_NONE,
    .sample = {
        .fIdx = SF_MMX_017_ORGAN_BIN,
        .loop = 0,
        .loopStart = 771,
        .loopEnd = 898,
        // original rate: 14336
        .rate = 14336,
        // pitch keycenter=70, tune=65
        .baseNote = FREQ_A_SHARP_4,
        .tune = 65,
    },
    .name = "MMX Organ",
    .envelope = {
        .attackTime = SECONDS_CONV(0, 1000), // 1ms
        //.attackTimeVel = -2048,
        //LFO freq: 8.176
        .sustainVol = 127,
        //.releaseTime = 8192,
        // 128 ticks per velocity, up to 128*128 == 16384 ticks = 1s
        .releaseTimeVel = 256 << 8,
    },
};

const midiTimbre_t mmx024AcousticGuitar = {
    .type = SAMPLE,
    .flags = TF_NONE,
    .sample = {
        .fIdx = SF_MMX_024_ACOUSTIC_GUITAR_BIN,
        .loop = 0,
        .loopStart = 559,
        .loopEnd = 671,
        .rate = 37760,
        // keycenter=77, tune=57
        .baseNote = FREQ_F4,
        .tune = 57,
    },
    .name = "MMX Acoustic Guitar",
    .envelope = {
        .attackTime = SECONDS_CONV(0, 1000),
        .decayTime = SECONDS_CONV(2, 70530), // 2.07053s
        // LFO freq: 8.176
        .sustainVol = 1,
        .releaseTime = SECONDS_CONV(1, 35530), // 2.07053s / 2
    },
};

const midiTimbre_t mmx029OverdrivenGuitar = {
    .type = SAMPLE,
    .flags = TF_NONE,
    .sample = {
        .fIdx = SF_MMX_029_OVERDRIVEN_GUITAR_BIN,
        .loop = 0,
        .loopStart = 503,
        .loopEnd = 599,
        .rate = 25728,
        // keycenter=73, tune=56
        .baseNote = FREQ_C_SHARP_4,
        .tune = 56,
    },
    .name = "MMX Overdrive Guitar",
    .envelope = {
        .attackTime = SECONDS_CONV(0, 1000),
        .sustainVol = 127,
    },
};
const midiTimbre_t mmx030DistortedGuitar = {
    .type = SAMPLE,
    .flags = TF_NONE,
    .sample = {
        .fIdx = SF_MMX_030_DISTORTION_GUITAR_BIN,
        .loop = 0,
        .loopStart = 6491,
        .loopEnd = 8085,
        // original rate: 23168 Hz
        .rate = 23168,
        // keycenter=52, tune=49
        .baseNote = FREQ_E2,
        .tune = 49,
    },
    .name = "MMX Distortion Guitar",
    .envelope = {
        .attackTime = SECONDS_CONV(0, 1000),
        .sustainVol = 127,
    },
};
const midiTimbre_t mmx036SlapBass = {
    .type = SAMPLE,
    .flags = TF_NONE,
    .sample = {
        .fIdx = SF_MMX_036_SLAP_BASS_BIN,
        .loop = 0,
        .loopStart = 1445,
        .loopEnd = 3120,
        // original rate: 13440 Hz
        .rate = 13440,
        .baseNote = FREQ_B2,
        .tune = 45,
    },
    .name = "MMX Slap Bass",
    .envelope = {
        .attackTime = SECONDS_CONV(0, 1000),
        .sustainVol = 127,
    },
};
const midiTimbre_t mmx038SynthBass = {
    .type = SAMPLE,
    .flags = TF_NONE,
    .sample = {
        .fIdx = SF_MMX_038_SYNTH_BASS_BIN,
        .loop = 0,
        .loopStart = 1247,
        .loopEnd = 1391,
        // original rate: 8960 Hz
        .rate = 8960,
        // keycenter=48, tune=83
        .baseNote = FREQ_C2,
        .tune = 83,
    },
    .name = "MMX Synth Bass",
    .envelope = {
        .attackTime = SECONDS_CONV(0, 1000),
        .sustainVol = 127,
        .sustainVolVel = 0,
    }
};
const midiTimbre_t mmx048Strings = {
    .type = SAMPLE,
    .flags = TF_NONE,
    .sample = {
        .fIdx = SF_MMX_048_STRINGS_BIN,
        .loop = 0,
        .loopStart = 1409,
        .loopEnd = 5411,
        // Original rate: 30720 Hz
        .rate = 30720,
        // keycenter=76, tune=55
        .baseNote = FREQ_E4,
        .tune = 55,
    },
    .name = "MMX Strings",
    .envelope = {
        .attackTime = SECONDS_CONV(0, 1000),
        .sustainVol = 127,
    },
};
const midiTimbre_t mmx055OrchestraHit = {
    .type = SAMPLE,
    .flags = TF_NONE,
    .sample = {
        .fIdx = SF_MMX_055_ORCHESTRA_HIT_BIN,
        // Play sample ONCE
        .loop = 1,
        // original rate: 16512 Hz
        .rate = 16512,
        // keycenter=61, tune=55
        .baseNote = FREQ_C_SHARP_3,
        .tune = 55,
        .loopStart = 0,
        .loopEnd = 0,
    },
    .name = "MMX Orchestra Hit",
    .envelope = {
        .attackTime = SECONDS_CONV(0, 1000),
        .sustainVol = 127,
    },
};
const midiTimbre_t mmx062SynthBrass = {
    .type = SAMPLE,
    .flags = TF_NONE,
    .sample = {
        .fIdx = SF_MMX_062_SYNTH_BRASS_BIN,
        .loop = 0,
        .loopStart = 4479,
        .loopEnd = 4543,
        // original rate: 28928
        .rate = 28928,
        // keycenter=58, tune=50
        .baseNote = FREQ_A_SHARP_3,
        .tune = 50,
    },
    .name = "MMX Synth Brass",
    .envelope = {
        .attackTime = SECONDS_CONV(0, 1000),
        .sustainVol = 127,
    },
};
const midiTimbre_t mmx080SquareWave = {
    .type = SAMPLE,
    .flags = TF_NONE,
    .sample = {
        .fIdx = SF_MMX_080_SQUARE_WAVE_BIN,
        .loop = 0,
        .loopStart = 17,
        .loopEnd = 80,
        // original rate: 28928
        .rate = 28928,
        // keycenter=82, tune=50
        .baseNote = FREQ_A_SHARP_5,
        .tune = 50,
    },
    .name = "MMX Square Wave",
    .envelope = {
        .attackTime = SECONDS_CONV(0, 1000),
        .sustainVol = 127,
    },
};
const midiTimbre_t mmx081SawWave = {
    .type = SAMPLE,
    .flags = TF_NONE,
    .sample = {
        .fIdx = SF_MMX_081_SAWTOOTH_WAVE_BIN,
        .loop = 0,
        // original rate: 13696 Hz
        .loopStart = 235,
        .loopEnd = 5857,
        .rate = 13696,
        .baseNote = FREQ_G3,
        .tune = 50,
    },
    .name = "MMX Saw Wave",
    .envelope = {
        .attackTime = SECONDS_CONV(0, 1000),
        .sustainVol = 127,
    },
};
const midiTimbre_t mmx082SynthLead = {
    .type = SAMPLE,
    .flags = TF_NONE,
    .sample = {
        .fIdx = SF_MMX_082_SYNTH_LEAD_BIN,
        .loop = 0,
        .loopStart = 47,
        .loopEnd = 111,
        // original rate: 87040 Hz
        .rate = 87040,
        // keycenter=89, tune=43
        .baseNote = FREQ_F5,
        .tune = 45,
    },
    .name = "MMX Synth Lead",
    .envelope = {
        .attackTime = SECONDS_CONV(0, 1000),
        .sustainVol = 127,
    },
};
const midiTimbre_t mmx083SynthLead2 = {
    .type = SAMPLE,
    .flags = TF_NONE,
    .sample = {
        .fIdx = SF_MMX_083_SYNTH_LEAD_2_BIN,
        .loop = 0,
        .loopStart = 47,
        .loopEnd = 79,
        // original rate: 12032 Hz
        .rate = 12032,
        // keycenter=67, tune=69
        .baseNote = FREQ_G3,
        .tune = 69,
    },
    .name = "MMX Synth Lead 2",
    .envelope = {
        .attackTime = SECONDS_CONV(0, 1000),
        .sustainVol = 127,
    },
};
const midiTimbre_t mmx119ReverseCymbal = {
    .type = SAMPLE,
    .flags = TF_NONE,
    .sample = {
        .fIdx = CRASHCYMBAL_BIN,
        .loop = 0,
        .loopStart = 2095,
        .loopEnd = 4575,
        // original rate: 16640 Hz
        .rate = 16640,
        // keycenter=60
        .baseNote = FREQ_C3,
    },
    .name = "MMX Reverse Cymbal",
    .envelope = {
        .attackTime = SECONDS_CONV(1, 380320u),
        .sustainVol = 1,
    },
};

const noteSampleMap_t mmxDrumSampleMap[] = {
    {
        .noteStart = 36,
        .noteEnd = 36,
        .sample = {
            .baseNote = FREQ_C1,
            .tune = 0,
            .fIdx = KICK_BIN,
            .rate = 30976,
            .loop = 1,
            .loopStart = 8,
            .loopEnd = 3991,
        },
        .envelope = {
            .attackTime = SECONDS_CONV(0, 1000),
            .sustainVol = 127,
        },
    },
    {
        .noteStart = 37,
        .noteEnd = 37,
        .sample = {
            .baseNote = FREQ_C_SHARP_1,
            .tune = 0,
            .fIdx = HIGHQ_BIN,
            .rate = 8576,
            .loop = 1,
            .loopStart = 8,
            .loopEnd = 1991,
        },
        .envelope = {
            .attackTime = SECONDS_CONV(0, 1000),
            .sustainVol = 127,
        },
    },
    {
        .noteStart = 38,
        .noteEnd = 40,
        .sample = {
            .baseNote = FREQ_B1,
            .tune = 0,
            .fIdx = SNARE_BIN,
            .rate = 25984,
            .loop = 1,
            .loopStart = 8,
            .loopEnd = 6007,
        },
        .envelope = {
            .attackTime = SECONDS_CONV(0, 1000),
            .sustainVol = 127,
        },
    },
    {
        .noteStart = 41,
        .noteEnd = 41,
        .sample = {
            .baseNote = FREQ_F1,
            .tune = 0,
            .fIdx = POWERSNARE_BIN,
            .rate = 32256,
            .loop = 0,
            .loopStart = 2095,
            .loopEnd = 5087,
        },
        .envelope = {
            .attackTime = SECONDS_CONV(0, 1000),
            .decayTime = SECONDS_CONV(0, 771105u),
            .sustainVol = 0,
            .releaseTime = SECONDS_CONV(0, 771105u),
            // TODO: implement hold
            //.holdTime = SECONDS_CONV(0, 84008)
        },
    },
    {
        .noteStart = 42,
        .noteEnd = 55,
        .sample = {
            .baseNote = FREQ_G_SHARP_1,
            .tune = 0,
            .fIdx = OPENHIHAT_BIN,
            .rate = 20224,
            .loop = 0,
            .loopStart = 47,
            .loopEnd = 2543,
        },
        .envelope = {
            .attackTime = SECONDS_CONV(0, 1000),
            .decayTime = SECONDS_CONV(2, 64500u),
            .sustainVol = 0,
            .releaseTime = SECONDS_CONV(1, 16125u),
            // no hold here
        },
    },
    {
        .noteStart = 56,
        .noteEnd = 72,
        .sample = {
            .baseNote = FREQ_A3,
            .tune = 0,
            .fIdx = CRASHCYMBAL_BIN,
            .rate = 16640,
            .loop = 0,
            .loopStart = 2095,
            .loopEnd = 4575,
        },
        {
            .attackTime = SECONDS_CONV(0, 1000),
            .decayTime = SECONDS_CONV(1, 224170),
            .sustainVol = 0,
        },
    },
    {
        .noteStart = 73,
        .noteEnd = 85,
        .sample = {
            .baseNote = FREQ_F_SHARP_4,
            .tune = 0,
            .fIdx = SYNTHTOM_BIN,
            .rate = 30976,
            .loop = 1,
            .loopStart = 8,
            .loopEnd = 3495,
        },
        {
            .attackTime = SECONDS_CONV(0, 1000),
            .sustainVol = 127,
        },
    },
    {
        .noteStart = 86,
        .noteEnd = 102,
        .sample = {
            .baseNote = FREQ_G5,
            .tune = 0,
            .fIdx = TOM_BIN,
            .rate = 14464,
            .loop = 1,
            .loopStart = 8,
            .loopEnd = 2999,
        },
        {
            .attackTime = SECONDS_CONV(0, 1000),
            .sustainVol = 127,
        },
    },
};

const midiTimbre_t mmxDrumkitTimbre = {
    .type = MULTI_SAMPLE,
    .flags = TF_PERCUSSION,
    .multiSample = {
        .map = mmxDrumSampleMap,
        .count = ARRAY_SIZE(mmxDrumSampleMap),
    },
    .envelope = {
        .attackTime = 0,
        .sustainVol = 127,
        .releaseTime = 0,
    },
    .name = "MMX Drumkit",
};

const midiTimbre_t* const mmxTimbres[] = {
    &mmx011Vibraphone, &mmx017Organ,     &mmx024AcousticGuitar, &mmx029OverdrivenGuitar, &mmx030DistortedGuitar,
    &mmx036SlapBass,   &mmx038SynthBass, &mmx048Strings,        &mmx055OrchestraHit,     &mmx062SynthBrass,
    &mmx080SquareWave, &mmx081SawWave,   &mmx082SynthLead,      &mmx083SynthLead2,       &mmx119ReverseCymbal,
};

const uint8_t mmxTimbreMap[] = {
    11, 17, 24, 29, 30, 36, 38, 48, 50, 62, 80, 81, 82, 83, 119,
};

const size_t mmxTimbreCount = sizeof(mmxTimbres) / sizeof(*mmxTimbres);
