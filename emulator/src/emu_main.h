#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {
    bool fullscreen;
    bool hideLeds;

    bool emulateMotion;
    bool motionJitter;
    bool motionDrift;

    int16_t motionZ;
    int16_t motionX;
    uint16_t motionJitterAmount;

    bool emulateTouch;
} emuArgs_t;

extern emuArgs_t emulatorArgs;

#define WARN_UNIMPLEMENTED()                           \
    {                                                  \
        static bool printed = false;                   \
        if (!printed)                                  \
        {                                              \
            printed = true;                            \
            printf("%s is UNIMPLEMENTED\n", __func__); \
        }                                              \
    }
