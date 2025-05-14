#pragma once
#include "trophy.h"

// ENUM
typedef enum
{
    CLT_DOWN,
    CLT_LEFT,
    CLT_RIGHT,
} ChecklistTask_t;

// Trophy Data

const trophyData_t trophyTestModeTrophies[] = {
    {
        .title       = "Trigger Trophy",
        .description = "You pressed A!",
        .image       = KID_0_WSG,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_EASY,
        .maxVal      = 1, // For trigger type, set to one
    },
    {
        .title       = "Additive Trophy - Testing a very long trophy na",
        .description = "Pressed B ten times!",
        .image       = KID_1_WSG,
        .type        = TROPHY_TYPE_ADDITIVE,
        .difficulty  = TROPHY_DIFF_HARD,
        .maxVal      = 10,
    },
    {
        .title       = "Progress Trophy",
        .description = "Hold down the up button for eight seconds",
        .image       = NO_IMAGE_SET, // Hardcoded "Ignore" value
        .type        = TROPHY_TYPE_PROGRESS,
        .difficulty  = TROPHY_DIFF_EXTREME,
        .maxVal      = 8,
        .hidden      = true,
    },
    {
        .title       = "Checklist",
        .description = "This is gonna need a bunch of verification, but like has a very long description",
        .image       = KID_0_WSG,
        .type        = TROPHY_TYPE_CHECKLIST,
        .difficulty  = TROPHY_DIFF_HARD,
        .maxVal      = 0x0007, // Three tasks, 0x01, 0x02, and 0x04
    },
};

// Individual mode settings

trophySettings_t trophyTestModeTrophySettings = {
    .drawFromBottom   = false,
    .staticDurationUs = DRAW_STATIC_US * 4,
    .slideDurationUs  = DRAW_SLIDE_US,
};