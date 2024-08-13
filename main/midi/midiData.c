#include "midiData.h"
#include "midiPlayer.h"

#include "waveTables.h"
#include "drums.h"

#include "macros.h"
#include "hdw-dac.h"

const midiTimbre_t acousticGrandPianoTimbre = {
    .type = WAVETABLE,
    .flags = TF_NONE,
    .waveIndex = 0,
    .waveFunc = waveTableFunc,
    .envelope = {
        // TODO: Just realized I forgot how ADSR actually works halfway through writing everything else...
        // Pretty fast attack
        .attackTime = MS_TO_SAMPLES(64),
        // Decrease attack time by 1ms for every 4 velocity value
        .attackTimeVel = TO_FX_FRAC(-MS_TO_SAMPLES(1), 4),

        // Take a good long-ish while to reach the sustain level
        .decayTime = MS_TO_SAMPLES(75),
        // Take a bit longer the higher the velocity -- 25ms for every 16 velocity (so up to +200ms)
        .decayTimeVel = TO_FX_FRAC(MS_TO_SAMPLES(25), 16),

        // Sustain at 1 plus 75% of initial volume
        .sustainVol = 1,
        .sustainVolVel = TO_FX_FRAC(3, 4),

        // And a not-too-short release time
        .releaseTime = MS_TO_SAMPLES(100),
        // Plus some extra time if the note was very loud initially - up to double
        .releaseTimeVel = MS_TO_SAMPLES(10) << 8, //TO_FX_FRAC(MS_TO_SAMPLES(150), 63),
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

const midiTimbre_t* magfestTimbres[] = {
    &squareWaveTimbre,
    &sineWaveTimbre,
    &triangleWaveTimbre,
    &sawtoothWaveTimbre,
    &magfestWaveTimbre,
    &noiseTimbre,
};

const size_t magfestTimbreCount = ARRAY_SIZE(magfestTimbres);

const midiTimbre_t defaultDrumkitTimbre = {
    .type = NOISE,
    .flags = TF_PERCUSSION,
    .percussion = {
        .playFunc = defaultDrumkitFunc,
        // TODO: Define the data and put it here!
        .data = NULL,
    },
    .envelope = { 0 },
    .name = "Swadge Drums 0",
};

const midiTimbre_t donutDrumkitTimbre = {
    .type = NOISE,
    .flags = TF_PERCUSSION,
    .percussion = {
        .playFunc = donutDrumkitFunc,
        // This should be set though
        .data = NULL,
    },
    .envelope = { 0 },
    .name = "Donut Swadge Drums",
};