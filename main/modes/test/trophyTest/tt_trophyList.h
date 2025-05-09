#include "trophy.h"

// ENUM
typedef enum 
{
    CLT_DOWN,
    CLT_LEFT,
    CLT_RIGHT,
} ChecklistTask_t;

// Trophy Data

const trophyData_t testTrophies[] = {
    {
        .title       = "Trigger Trophy",
        .description = "You pressed A!",
        .imageString = "kid0.wsg",
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_EASY,
        .maxVal      = 1, // For trigger type, set to one
    },
    {
        .title       = "Additive Trophy - Testing",
        .description = "Pressed B ten times!",
        .imageString = "kid1.wsg",
        .type        = TROPHY_TYPE_ADDITIVE,
        .difficulty  = TROPHY_DIFF_HARD,
        .maxVal      = 10,
    },
    {
        .title       = "Progress Trophy",
        .description = "Hold down the up button for eight seconds",
        .imageString = "",
        .type        = TROPHY_TYPE_PROGRESS,
        .difficulty  = TROPHY_DIFF_EXTREME,
        .maxVal      = 8,
    },
    {
        .title       = "Checklist",
        .description = "This is gonna need a bunch of verification, but like has a very long description",
        .imageString = "kid1.wsg",
        .type        = TROPHY_TYPE_CHECKLIST,
        .difficulty  = TROPHY_DIFF_HARD,
        .maxVal      = 0x0007, // Three tasks, 0x01, 0x02, and 0x04
    },
    {
        .title       = "Placeholder",
        .description = "If our eyes aren't real, why can't bees fly?",
        .imageString = "",
        .type        = TROPHY_TYPE_ADDITIVE,
        .difficulty  = TROPHY_DIFF_HARD,
        .maxVal      = 12000000,
    },
    {
        .title       = "Placeholder 2",
        .description = "",
        .imageString = "kid1.wsg",
        .type        = TROPHY_TYPE_ADDITIVE,
        .difficulty  = TROPHY_DIFF_HARD,
        .maxVal      = 1024,
    },
};

// Individual mode settings

trophySettings_t tSettings = {
    .drawFromBottom   = false,
    .staticDurationUs = DRAW_STATIC_US,
    .slideDurationUs  = DRAW_SLIDE_US,
};