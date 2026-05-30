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

static void seedIfUnseeded(void)
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
}

uint32_t esp_random(void)
{
    seedIfUnseeded();
    uint32_t val = rand();
    return val;
}

void esp_fill_random(void* buf, size_t len)
{
    seedIfUnseeded();
    uint8_t* buf8 = (uint8_t*)buf;
    for (size_t i = 0; i < len; i++)
    {
        buf8[i] = esp_random();
    }
}

void emulatorSetEspRandomSeed(uint32_t seed_)
{
    seed = seed_;

    if (seedValueSet)
    {
        srand(seed);
    }

    seedValueSet = true;
}

unsigned int emulatorGetEspRandomSeed(void)
{
    return seed;
}
