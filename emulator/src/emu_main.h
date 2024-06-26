#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define WARN_UNIMPLEMENTED()                           \
    {                                                  \
        static bool printed = false;                   \
        if (!printed)                                  \
        {                                              \
            printed = true;                            \
            printf("%s is UNIMPLEMENTED\n", __func__); \
        }                                              \
    }

void emulatorQuit(void);
void plotRoundedCorners(uint32_t* bitmapDisplay, int w, int h, int r, uint32_t col);
