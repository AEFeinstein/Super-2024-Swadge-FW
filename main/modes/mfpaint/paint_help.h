#ifndef _PAINT_HELP_H_
#define _PAINT_HELP_H_

#include "paint_common.h"
#include <stddef.h>
#include <stdbool.h>

#include "paint_type.h"

typedef enum
{
    TOUCH_ANY   = 0x0100,
    TOUCH_X     = 0x0200,
    TOUCH_Y     = 0x0400,
    SWIPE_LEFT  = 0x0800,
    SWIPE_RIGHT = 0x1000,
} virtualButton_t;

// Triggers for advancing the tutorial step
typedef enum
{
    NO_TRIGGER,

    // Press all of the given buttons at least once, in any order
    PRESS_ALL,

    // Press any one of the give buttons
    PRESS_ANY,

    // Press all of the given buttons at once
    PRESS,

    // Release the given button
    RELEASE,

    // A tool has drawn (not just picked a point)
    DRAW_COMPLETE,

    CHANGE_BRUSH,
    SELECT_MENU_ITEM,
    CHANGE_MODE,

    // Inverse of CHANGE_BRUSH
    BRUSH_NOT,

    // Inverse of SELECT_MENU_ITEM
    MENU_ITEM_NOT,

    // Inverse of CHANGE_MODE
    MODE_NOT,
} paintHelpTriggerType_t;

typedef struct
{
    paintHelpTriggerType_t type;
    const void* dataPtr;
    int64_t data;
} paintHelpTrigger_t;

typedef enum
{
    IND_NONE,
    IND_BOX,
    IND_ARROW,
} paintHelpIndicatorType_t;

typedef struct
{
    paintHelpIndicatorType_t type;

    union {
        struct { uint16_t x0, y0, x1, y1; } box;
        struct { uint16_t x, y; int dir; } arrow;
    };
} paintHelpIndicator_t;

typedef struct
{
    paintHelpTrigger_t trigger;

    // If this trigger is met, go back to a previous step
    paintHelpTrigger_t backtrack;

    // The number of steps to go back if the backtrack trigger is met
    uint8_t backtrackSteps;

    const char* prompt;
} paintHelpStep_t;

typedef struct
{
    const paintHelpStep_t* curHelp;
    uint16_t allButtons;
    uint16_t curButtons;
    uint16_t lastButton;
    bool lastButtonDown;

    bool drawComplete;

    uint16_t helpH;
} paintHelp_t;

extern const paintHelpStep_t helpSteps[];
extern const paintHelpStep_t* lastHelp;

#endif
