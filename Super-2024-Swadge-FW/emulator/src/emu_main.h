#pragma once

#include <stdbool.h>
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
