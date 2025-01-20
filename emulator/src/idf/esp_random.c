#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>

#include <esp_random.h>
#include <esp_random_emu.h>
#include "ext_replay.h"
#include "esp_timer.h"

static bool seeded       = false;
static bool seedValueSet = false;
static unsigned int seed = 0;

uint32_t esp_random(void)
{
    if (!seeded)
    {
        printf("rand() seeding...\n");
        if (!seedValueSet)
        {
            pid_t pid = getpid();
            seed      = time(NULL) ^ pid;
        }

        seeded = true;
        srand(seed);

        printf("Random Seed: %" PRIu32 "\n", seed);

        emulatorRecordRandomSeed(seed);
    }
    uint32_t val = rand();
    return val;
}

void emulatorSetEspRandomSeed(uint32_t seed_)
{
    seed = seed_;

    if (seedValueSet)
    {
        seeded = true;
        srand(seed);
    }

    seedValueSet = true;
}

unsigned int emulatorGetEspRandomSeed(void)
{
    return seed;
}
