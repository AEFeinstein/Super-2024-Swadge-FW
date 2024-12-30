#include "midiData.h"
#include "midiPlayer.h"

#include "waveTables.h"
#include "drums.h"

#include "macros.h"
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
        // Config will be replaced by .data and .count at load time
        .config = {
            .sampleName = "colossus.bin",
        },
        // Loop only once
        .loop = 1,
        //
        .rate = 8192,
        .baseNote = FREQ_C_SHARP_5,
    },
    .name = "Colossus Roar",
};

const midiTimbre_t magTimbre = {
    .type = SAMPLE,
    .flags = TF_NONE,
    .sample = {
        // Config will be replaced by .data and .count at load time
        .config = {
            .sampleName = "donut_mag.bin",
        },
        // Loop only once
        .loop = 1,
        //
        .rate = 8192,
        .baseNote = FREQ_A4,
    },
    .name = "MAG",
};

const midiTimbre_t festTimbre = {
    .type = SAMPLE,
    .flags = TF_NONE,
    .sample = {
        // Config will be replaced by .data and .count at load time
        .config = {
            .sampleName = "donut_fest.bin",
        },
        // Loop only once
        .loop = 1,
        //
        .rate = 8192,
        .baseNote = FREQ_A4,
    },
    .name = "FEST",
};

const midiTimbre_t wilhelmTimbre = {
    .type = SAMPLE,
    .flags = TF_NONE,
    .sample = {
        // Config will be replaced by .data and .count at load time
        .config = {
            .sampleName = "wilhelm.bin",
        },
        // Loop only once
        .loop = 0,
        //
        .rate = 8192,
        .baseNote = FREQ_A4,
    },
    .name = "Wilhelm",
};

const midiTimbre_t noriTimbre = {
    .type = SAMPLE,
    .flags = TF_NONE,
    .sample = {
        // Config will be replaced by .data and .count at load time
        .config = {
            .sampleName = "nori2.bin",
        },
        // Loop only once
        .loop = 0,
        //
        .rate = 8192,
        .baseNote = FREQ_A4,
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
