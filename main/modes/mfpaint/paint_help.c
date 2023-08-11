#include "paint_help.h"

#include "btn.h"

/*
 * Interactive Help Definitions
 *
 * Each step here defines a tutorial step with a help message that will
 * be displayed below the canvas, and a trigger that will cause the tutorial
 * step to be considered "completed" and move on to the next step.
 */

static const char STR_HOLD_A_TO_DRAW[] = "Cool! You can also hold A to draw while moving with the D-Pad. Let's try it! Hold A and press D-Pad DOWN";
static const char STR_RELEASE_TOUCH_PAD_TO_CONFIRM[] = "And release the TOUCH PAD to confirm!";

static const char STR_BRUSH_RECTANGLE[] = "Rectangle";
static const char STR_BRUSH_POLYGON[] = "Polygon";

const paintHelpStep_t helpSteps[] =
{
    {
        .trigger = { .type = PRESS_ALL, .data = (UP | DOWN | LEFT | RIGHT) },
        .prompt = "Welcome to MFPaint!\nLet's get started: First, use the D-Pad to move the cursor around!"
    },
    {
        .trigger = { .type = RELEASE, .data = BTN_A, },
        .prompt = "Excellent!\nNow, press A to draw something!"
    },
    {
        .trigger = { .type = PRESS, .data = (BTN_A | DOWN), },
        .prompt = STR_HOLD_A_TO_DRAW
    },
    {
        .trigger = { .type = RELEASE, .data = DOWN, },
        .prompt = STR_HOLD_A_TO_DRAW
    },
    {
        .trigger = { .type = PRESS, .data = TOUCH_ANY, },
        .prompt = "Magnificent! But what if you make a mistake? Worry not, you can UNDO! Press and hold the TOUCH PAD between Y and X..."
    },
    {
        .trigger = { .type = PRESS, .data = TOUCH_ANY | SELECT, },
        .backtrack = { .type = RELEASE, .data = TOUCH_ANY | SWIPE_LEFT | SWIPE_RIGHT | TOUCH_X | TOUCH_Y | SELECT },
        .backtrackSteps = 1,
        .prompt = "And then press SELECT to UNDO the last action"
    },
    {
        .trigger = { .type = PRESS, .data = TOUCH_ANY, },
        .prompt = "Phew! You can also REDO by holding the TOUCH PAD again..."
    },
    {
        .trigger = { .type = PRESS, .data = TOUCH_ANY | START, },
        .backtrack = { .type = RELEASE, .data = TOUCH_ANY | SWIPE_LEFT | SWIPE_RIGHT | TOUCH_X | TOUCH_Y | START },
        .backtrackSteps = 1,
        .prompt = "And then pressing START to REDO what was undone"
    },
    {
        .trigger = { .type = PRESS, .data = TOUCH_ANY, },
        .prompt = "Now, let's change the color. Press and hold the TOUCH PAD again..."
    },
    {
        .trigger = { .type = PRESS, .data = TOUCH_ANY | DOWN, },
        .backtrack = { .type = RELEASE, .data = TOUCH_ANY | SWIPE_LEFT | SWIPE_RIGHT | TOUCH_X | TOUCH_Y },
        .backtrackSteps = 1,
        .prompt = "Then, press D-Pad DOWN to change the color selection..."
    },
    {
        .trigger = { .type = RELEASE, .data = TOUCH_ANY | TOUCH_X | TOUCH_Y | SWIPE_LEFT | SWIPE_RIGHT },
        .prompt = STR_RELEASE_TOUCH_PAD_TO_CONFIRM },
    {
        .trigger = { .type = RELEASE, .data = BTN_B, },
        .prompt = "Great choice! You can also quickly swap the foreground and background colors with the B BUTTON"
    },
    {
        .trigger = { .type = RELEASE, .data = TOUCH_X, },
        .prompt = "Now, let's change the brush size. Just tap X on the TOUCH PAD to increase the brush size by 1"
    },
    {
        .trigger = { .type = RELEASE, .data = BTN_A, },
        .prompt = "Press A to draw again with the larger brush!"
    },
    {
        .trigger = { .type = RELEASE, .data = TOUCH_Y, },
        .prompt = "Wow! Now, to decrease the brush size, just tap Y on the TOUCH PAD!"
    },
    {
        .trigger = { .type = RELEASE, .data = SWIPE_LEFT, },
        .prompt = "You can also increase the brush size smoothly by swiping RIGHT (from Y to X) on the TOUCH PAD"
    },
    {
        .trigger = { .type = RELEASE, .data = SWIPE_RIGHT, },
        .prompt = "And you can decrease it smoothly by swiping LEFT (from X to Y) on the TOUCH PAD"
    },
    {
        .trigger = { .type = PRESS, .data = TOUCH_ANY, },
        .prompt = "You're ready to use the Pen brushes!\nNow, let's try a different brush. Press and hold the TOUCH PAD again..."
    },
    {
        .trigger = { .type = PRESS, .data = TOUCH_ANY | RIGHT, },
        .backtrack = { .type = RELEASE, .data = TOUCH_ANY | SWIPE_LEFT | SWIPE_RIGHT | TOUCH_X | TOUCH_Y },
        .backtrackSteps = 1,
        .prompt = "Then, press D-Pad RIGHT to change the brush..."
    },
    {
        .trigger = { .type = RELEASE, .data = TOUCH_ANY | TOUCH_X | TOUCH_Y | SWIPE_LEFT | SWIPE_RIGHT, },
        .prompt = STR_RELEASE_TOUCH_PAD_TO_CONFIRM },
    {
        .trigger = { .type = CHANGE_BRUSH, .dataPtr = STR_BRUSH_RECTANGLE },
        .prompt = "Now, choose the RECTANGLE brush!"
    },
    {
        .trigger = { .type = RELEASE, .data = BTN_A, },
        .backtrack = { .type = BRUSH_NOT, .dataPtr = STR_BRUSH_RECTANGLE },
        .backtrackSteps = 1,
        .prompt = "Now, press A to select the first corner of the rectangle..."
    },
    {
        .trigger = { .type = PRESS_ANY, .data = (UP | DOWN | LEFT | RIGHT), },
        .backtrack = { .type = BRUSH_NOT, .dataPtr = STR_BRUSH_RECTANGLE },
        .backtrackSteps = 2,
        .prompt = "Then move somewhere else..."
    },
    {
        .trigger = { .type = RELEASE, .data = BTN_A, },
        .backtrack = { .type = BRUSH_NOT, .dataPtr = STR_BRUSH_RECTANGLE },
        .backtrackSteps = 3,
        .prompt = "Press A again to pick the other coner of the rectangle. Note that the first point you picked will blink!"
    },
    {
        .trigger = { .type = CHANGE_BRUSH, .dataPtr = STR_BRUSH_POLYGON },
        .prompt = "Nice! Let's try out the POLYGON brush next."
    },
    {
        .trigger = { .type = RELEASE, .data = BTN_A, },
        .backtrack = { .type = BRUSH_NOT, .dataPtr = STR_BRUSH_POLYGON },
        .backtrackSteps = 1,
        .prompt = "Press A to select the first point of the polygon..."
    },
    {
        .trigger = { .type = RELEASE, .data = BTN_A, },
        .backtrack = { .type = BRUSH_NOT, .dataPtr = STR_BRUSH_POLYGON },
        .backtrackSteps = 2,
        .prompt = "Pick at least one more point for the polygon. Note that the first point will change color!"
    },
    {
        .trigger = { .type = DRAW_COMPLETE, },
        .backtrack = { .type = BRUSH_NOT, .dataPtr = STR_BRUSH_POLYGON },
        .backtrackSteps = 3,
        .prompt = "To finish the polygon, connect it back to the original point, or use up all the remaining picks."
    },
    {
        .trigger = { .type = PRESS, .data = START, },
        .prompt = "Good job! Now you know how to use all the brush types.\nNext, let's press START to toggle the menu"
    },
    {
        .trigger = { .type = PRESS_ANY, .data = UP | DOWN | SELECT, },
        .backtrack = { .type = SELECT_MENU_ITEM, .data = HIDDEN },
        .backtrackSteps = 1,
        .prompt = "Press UP, DOWN, or SELECT to go through the menu items"
    },
    {
        .trigger = { .type = SELECT_MENU_ITEM, .data = PICK_SLOT_SAVE, },
        .backtrack = { .type = SELECT_MENU_ITEM, .data = HIDDEN },
        .backtrackSteps = 2,
        .prompt = "Great! Now, navigate to the SAVE option"
    },
    {
        .trigger = { .type = PRESS_ANY, .data = LEFT | RIGHT, },
        .backtrack = { .type = MENU_ITEM_NOT, .data = PICK_SLOT_SAVE },
        .backtrackSteps = 1,
        .prompt = "Use D-Pad LEFT and RIGHT to switch between save slots here, or any other menu options"
    },
    {
        .trigger = { .type = PRESS_ANY, .data = BTN_A | BTN_B, },
        .backtrack = { .type = MENU_ITEM_NOT, .data = PICK_SLOT_SAVE },
        .backtrackSteps = 2,
        .prompt = "Use the A BUTTON to confirm, or the B BUTTON to cancel and go back. You'll have to confirm again before deleting any art!"
    },
    {
        .trigger = { .type = SELECT_MENU_ITEM, .data = HIDDEN, },
        .prompt = "Press START or the B BUTTON to exit the menu"
    },
    {
        .trigger = { .type = PRESS, .data = START, },
        .prompt = "Let's try editing the palette! Press START to open the menu one more time"
    },
    {
        .trigger = { .type = SELECT_MENU_ITEM, .data = EDIT_PALETTE, },
        .backtrack = { .type = SELECT_MENU_ITEM, .data = HIDDEN },
        .backtrackSteps = 1,
        .prompt = "Use UP, DOWN, and SELECT to select the EDIT PALETTE menu item"
    },
    {
        .trigger = { .type = PRESS, .data = BTN_A, },
        .backtrack = { .type = MENU_ITEM_NOT, .data = EDIT_PALETTE },
        .backtrackSteps = 1,
        .prompt = "Press the A BUTTON to begin editing the palette"
    },
    {
        .trigger = { .type = PRESS_ANY, .data = UP | DOWN, },
        .backtrack = { .type = MODE_NOT, .data = BTN_MODE_PALETTE },
        .backtrackSteps = 2,
        .prompt = "Use D-Pad UP and DOWN to select a color to edit"
    },
    {
        .trigger = { .type = PRESS_ANY, .data = LEFT | RIGHT, },
        .backtrack = { .type = MODE_NOT, .data = BTN_MODE_PALETTE },
        .backtrackSteps = 3,
        .prompt = "Use D-Pad LEFT and RIGHT to switch between the RED, GREEN, and BLUE color sliders"
    },
    {
        .trigger = { .type = RELEASE, .data = TOUCH_ANY | SELECT, },
        .backtrack = { .type = MODE_NOT, .data = BTN_MODE_PALETTE },
        .backtrackSteps = 4,
        .prompt = "Tap along the TOUCH PAD to edit the selected color slider. You can also use SELECT"
    },
    {
        .trigger = { .type = PRESS_ANY, .data = BTN_A | BTN_B, },
        .backtrack = { .type = MODE_NOT, .data = BTN_MODE_PALETTE },
        .backtrackSteps = 5,
        .prompt = "Press the A BUTTON to confirm and swap to the new color. Or, press the B BUTTON to restore the original color."
    },
    {
        .trigger = { .type = NO_TRIGGER, },
        .prompt = "That's everything.\nHappy painting!"
    },
};

const paintHelpStep_t* lastHelp = helpSteps + sizeof(helpSteps) / sizeof(helpSteps[0]) - 1;
