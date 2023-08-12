#include "paint_draw.h"

#include <string.h>
#include "esp_heap_caps.h"

#include "musical_buzzer.h"

#include "paint_ui.h"
#include "paint_brush.h"
#include "paint_nvs.h"
#include "paint_util.h"
#include "mode_paint.h"
#include "paint_song.h"
#include "paint_help.h"


#define MAX(a, b) ((a) > (b) ? (a) : (b))

paintDraw_t* paintState;
paintHelp_t* paintHelp;

static paletteColor_t defaultPalette[] =
{
    c000, // black
    c555, // white
    c012, // dark blue
    c505, // fuchsia
    c540, // yellow
    c235, // cornflower

    c222, // light gray
    c444, // dark gray

    c500, // red
    c050, // green
    c055, // cyan
    c005, // blue
    c530, // orange?
    c503, // pink
    c350, // lime green
    c522, // salmon
};

brush_t brushes[] =
{
    { .name = "Square Pen", .mode = HOLD_DRAW,  .maxPoints = 1, .minSize = 1, .maxSize = 16, .fnDraw = paintDrawSquarePen, .iconName = "square_pen" },
    { .name = "Circle Pen", .mode = HOLD_DRAW,  .maxPoints = 1, .minSize = 1, .maxSize = 16, .fnDraw = paintDrawCirclePen, .iconName = "circle_pen" },
    { .name = "Line",       .mode = PICK_POINT, .maxPoints = 2, .minSize = 1, .maxSize = 8, .fnDraw = paintDrawLine, .iconName = "line" },
    { .name = "Bezier Curve", .mode = PICK_POINT, .maxPoints = 4, .minSize = 1, .maxSize = 8, .fnDraw = paintDrawCurve, .iconName = "curve" },
    { .name = "Rectangle",  .mode = PICK_POINT, .maxPoints = 2, .minSize = 1, .maxSize = 8, .fnDraw = paintDrawRectangle, .iconName = "rect" },
    { .name = "Filled Rectangle", .mode = PICK_POINT, .maxPoints = 2, .minSize = 0, .maxSize = 0, .fnDraw = paintDrawFilledRectangle, .iconName = "rect_filled" },
    { .name = "Circle",     .mode = PICK_POINT, .maxPoints = 2, .minSize = 1, .maxSize = 8, .fnDraw = paintDrawCircle, .iconName = "circle" },
    { .name = "Filled Circle", .mode = PICK_POINT, .maxPoints = 2, .minSize = 0, .maxSize = 0, .fnDraw = paintDrawFilledCircle, .iconName = "circle_filled" },
    { .name = "Ellipse",    .mode = PICK_POINT, .maxPoints = 2, .minSize = 1, .maxSize = 8, .fnDraw = paintDrawEllipse, .iconName = "ellipse" },
    { .name = "Polygon",    .mode = PICK_POINT_LOOP, .maxPoints = 16, .minSize = 1, .maxSize = 8, .fnDraw = paintDrawPolygon, .iconName = "polygon" },
    { .name = "Squarewave", .mode = PICK_POINT, .maxPoints = 2, .minSize = 0, .maxSize = 0, .fnDraw = paintDrawSquareWave, .iconName = "squarewave" },
    { .name = "Paint Bucket", .mode = PICK_POINT, .maxPoints = 1, .minSize = 0, .maxSize = 0, .fnDraw = paintDrawPaintBucket, .iconName = "paint_bucket" },
};

const char activeIconStr[] = "%s_active.wsg";
const char inactiveIconStr[] = "%s_inactive.wsg";

const brush_t* firstBrush = brushes;
const brush_t* lastBrush = brushes + sizeof(brushes) / sizeof(brushes[0]) - 1;

void paintDrawScreenSetup(void)
{
    PAINT_LOGD("Allocating %zu bytes for paintState", sizeof(paintDraw_t));
    paintState = calloc(sizeof(paintDraw_t), 1);

    loadFont(PAINT_TOOLBAR_FONT, &(paintState->toolbarFont));
    loadFont(PAINT_SAVE_MENU_FONT, &(paintState->saveMenuFont));
    loadFont(PAINT_SMALL_FONT, &(paintState->smallFont));
    paintState->clearScreen = true;
    paintState->blinkOn = true;
    paintState->blinkTimer = 0;


    // Set up the brush icons
    uint16_t spriteH = 0;
    char iconName[32];
    for (brush_t* brush = brushes; brush <= lastBrush; brush++)
    {
        snprintf(iconName, sizeof(iconName), activeIconStr, brush->iconName);
        if (!loadWsg(iconName, &brush->iconActive))
        {
            PAINT_LOGE("Loading icon %s failed!!!", iconName);
        }

        snprintf(iconName, sizeof(iconName), inactiveIconStr, brush->iconName);
        if (!loadWsg(iconName, &brush->iconInactive))
        {
            PAINT_LOGE("Loading icon %s failed!!!", iconName);
        }

        // Keep track of the tallest sprite for layout purposes
        if (brush->iconActive.h > spriteH)
        {
            spriteH = brush->iconActive.h;
        }

        if (brush->iconInactive.h > spriteH)
        {
            spriteH = brush->iconInactive.h;
        }
    }

    if (!loadWsg("pointer.wsg", &paintState->picksWsg))
    {
        PAINT_LOGE("Loading pointer.wsg icon failed!!!");
    }

    if (!loadWsg("brush_size.wsg", &paintState->brushSizeWsg))
    {
        PAINT_LOGE("Loading brush_size.wsg icon failed!!!");
    }

    if (!loadWsg("arrow9.wsg", &paintState->smallArrowWsg))
    {
        PAINT_LOGE("Loading arrow5.wsg icon failed!!!");
    }
    else
    {
        colorReplaceWsg(&paintState->smallArrowWsg, c555, c000);
    }

    if (!loadWsg("arrow12.wsg", &paintState->bigArrowWsg))
    {
        PAINT_LOGE("Loading arrow5.wsg icon failed!!!");
    }
    else
    {
        colorReplaceWsg(&paintState->bigArrowWsg, c555, c000);
    }

    if (!loadWsg("newfile.wsg", &paintState->newfileWsg))
    {
        PAINT_LOGE("Loading newfile.wsg icon failed!!!");
    }

    if (!loadWsg("overwrite.wsg", &paintState->overwriteWsg))
    {
        PAINT_LOGE("Loading overwrite.wsg icon failed!!!");
    }

    // Setup the margins
    // Top: Leave room for the tallest of...
    // * The save menu text plus padding above and below it
    // * The tallest sprite, plus some padding
    // * The minimum height of the color picker, plus some padding
    // ... plus, 1px for the canvas border

    uint16_t saveMenuSpace = paintState->saveMenuFont.h + 2 * PAINT_TOOLBAR_TEXT_PADDING_Y;
    uint16_t toolbarSpace = spriteH + 2;

    // text above picker bars, plus 2px margin, plus min colorbar height (6), plus 2px above and below for select borders
    uint16_t colorPickerSpace = paintState->smallFont.h + 2 + PAINT_COLOR_PICKER_MIN_BAR_H + 2 + 2;

    paintState->marginTop = MAX(saveMenuSpace, MAX(toolbarSpace, colorPickerSpace)) + 1;


    // Left: Leave room for the color boxes, their margins, their borders, and the canvas border
    paintState->marginLeft = PAINT_COLORBOX_W + PAINT_COLORBOX_MARGIN_X * 2 + 2 + 1;
    // Bottom: Leave room for the brush name, 4px margin, and the canvas border
    paintState->marginBottom = paintState->toolbarFont.h + 4 + 1;
    // Right: We just need to stay away from the rounded corner, so like, 12px?
    paintState->marginRight = 12;

    if (paintHelp != NULL)
    {
        // Set up some tutorial things that depend on basic paintState data
        paintTutorialPostSetup();

        // We're in help mode! We need some more space for the text
        paintState->marginBottom += paintHelp->helpH;
    }

    paintLoadIndex(&paintState->index);

    if (paintHelp == NULL && paintGetAnySlotInUse(paintState->index) && paintGetRecentSlot(paintState->index) != PAINT_SAVE_SLOTS)
    {
        // If there's a saved image, load that (but not in the tutorial)
        paintState->selectedSlot = paintGetRecentSlot(paintState->index);
        paintState->doLoad = true;
    }
    else
    {
        // Set up a blank canvas with the default size
        paintState->canvas.w = PAINT_DEFAULT_CANVAS_WIDTH;
        paintState->canvas.h = PAINT_DEFAULT_CANVAS_HEIGHT;

        // Automatically position the canvas in the center of the drawable area at the max scale that will fit
        paintPositionDrawCanvas();

        // load the default palette
        memcpy(paintState->canvas.palette, defaultPalette, PAINT_MAX_COLORS * sizeof(paletteColor_t));
        getArtist()->fgColor = paintState->canvas.palette[0];
        getArtist()->bgColor = paintState->canvas.palette[1];
    }

    // This assumes the first brush is a pen brush, which it always will be unless we rearrange the brush array
    paintGenerateCursorSprite(&paintState->cursorWsg, &paintState->canvas, firstBrush->minSize);

    // Init the cursors for each artist
    // TODO only do one for singleplayer?
    for (uint8_t i = 0; i < sizeof(paintState->artist) / sizeof(paintState->artist[0]); i++)
    {
        initCursor(&paintState->artist[i].cursor, &paintState->canvas, &paintState->cursorWsg);
        initPxStack(&paintState->artist[i].pickPoints);
        paintState->artist[i].brushDef = firstBrush;
        paintState->artist[i].brushWidth = firstBrush->minSize;

        setCursorSprite(&paintState->artist[i].cursor, &paintState->canvas, &paintState->cursorWsg);
        setCursorOffset(&paintState->artist[i].cursor, (paintState->canvas.xScale - paintState->cursorWsg.w) / 2, (paintState->canvas.yScale - paintState->cursorWsg.h) / 2);
        moveCursorAbsolute(getCursor(), &paintState->canvas, paintState->canvas.w / 2, paintState->canvas.h / 2);
    }

    clearPxTft();

    paintSetupTool();

    // Clear the LEDs
    // Might not be necessary here
    paintUpdateLeds();

    buzzer_stop();
    buzzer_play_bgm(&paintBgm);

    PAINT_LOGI("It's paintin' time! Canvas is %d x %d pixels!", paintState->canvas.w, paintState->canvas.h);
}

void paintDrawScreenCleanup(void)
{
    buzzer_stop();

    for (brush_t* brush = brushes; brush <= lastBrush; brush++)
    {
        freeWsg(&brush->iconActive);
        freeWsg(&brush->iconInactive);
    }

    freeWsg(&paintState->brushSizeWsg);
    freeWsg(&paintState->picksWsg);
    freeWsg(&paintState->bigArrowWsg);
    freeWsg(&paintState->smallArrowWsg);
    freeWsg(&paintState->newfileWsg);
    freeWsg(&paintState->overwriteWsg);

    for (uint8_t i = 0; i < sizeof(paintState->artist) / sizeof(paintState->artist[0]) ;i++)
    {
        deinitCursor(&paintState->artist[i].cursor);
        freePxStack(&paintState->artist[i].pickPoints);
    }

    paintFreeCursorSprite(&paintState->cursorWsg);
    paintFreeUndos();

    freeFont(&paintState->smallFont);
    freeFont(&paintState->saveMenuFont);
    freeFont(&paintState->toolbarFont);
    free(paintState);
}

void paintTutorialSetup(void)
{
    paintHelp = calloc(sizeof(paintHelp_t), 1);
    paintHelp->curHelp = helpSteps;
}

// gets called after paintState is allocated and has basic info, but before canvas layout is done
void paintTutorialPostSetup(void)
{
    paintHelp->helpH = PAINT_HELP_TEXT_LINES * (paintState->toolbarFont.h + 1) - 1;
}

void paintTutorialCleanup(void)
{
    free(paintHelp);
    paintHelp = NULL;
}

void paintTutorialOnEvent(void)
{
    if (paintTutorialCheckTrigger(&paintHelp->curHelp->trigger))
    {
        paintState->redrawToolbar = true;
        if (paintHelp->curHelp != lastHelp)
        {
            paintHelp->curHelp++;
            paintHelp->allButtons = 0;
            paintHelp->lastButton = 0;
            paintHelp->lastButtonDown = false;
            paintHelp->drawComplete = false;
        }
    }
    else if (paintTutorialCheckTrigger(&paintHelp->curHelp->backtrack))
    {
        paintState->redrawToolbar = true;

        // check some bonuds even though it's constant
        if (paintHelp->curHelp - paintHelp->curHelp->backtrackSteps >= helpSteps)
        {
            paintHelp->curHelp -= paintHelp->curHelp->backtrackSteps;
        }
        else
        {
            paintHelp->curHelp = helpSteps;
        }

        paintHelp->allButtons = 0;
        paintHelp->lastButton = 0;
        paintHelp->lastButtonDown = false;
            paintHelp->drawComplete = false;
    }
}

bool paintTutorialCheckTrigger(const paintHelpTrigger_t* trigger)
{
    switch (trigger->type)
    {
    case PRESS_ALL:
        return (paintHelp->allButtons & trigger->data) == trigger->data;

    case PRESS_ANY:
        return (paintHelp->curButtons & trigger->data) != 0 && paintHelp->lastButtonDown;

    case PRESS:
        return (paintHelp->curButtons & (trigger->data)) == trigger->data && paintHelp->lastButtonDown;

    case RELEASE:
        return paintHelp->lastButtonDown == false && (paintHelp->lastButton & trigger->data) == paintHelp->lastButton;

    case DRAW_COMPLETE:
        return paintHelp->drawComplete;

    case CHANGE_BRUSH:
        return !strcmp(getArtist()->brushDef->name, trigger->dataPtr) && paintHelp->curButtons == 0;

    case CHANGE_MODE:
        return paintState->buttonMode == trigger->data;

    case BRUSH_NOT:
        return strcmp(getArtist()->brushDef->name, trigger->dataPtr) && paintHelp->curButtons == 0;

    case SELECT_MENU_ITEM:
        return paintState->saveMenu == trigger->data;

    case MENU_ITEM_NOT:
        return paintState->saveMenu != trigger->data;

    case MODE_NOT:
        return paintState->buttonMode != trigger->data;

    case NO_TRIGGER:
    default:
        break;
    }

    return false;
}

void paintPositionDrawCanvas(void)
{
    // Calculate the highest scale that will fit on the screen
    uint8_t scale = paintGetMaxScale(paintState->canvas.w, paintState->canvas.h, paintState->marginLeft + paintState->marginRight, paintState->marginTop + paintState->marginBottom);

    paintState->canvas.xScale = scale;
    paintState->canvas.yScale = scale;

    paintState->canvas.x = paintState->marginLeft + (TFT_WIDTH - paintState->marginLeft - paintState->marginRight - paintState->canvas.w * paintState->canvas.xScale) / 2;
    paintState->canvas.y = paintState->marginTop + (TFT_HEIGHT - paintState->marginTop - paintState->marginBottom - paintState->canvas.h * paintState->canvas.yScale) / 2;
}

void paintDrawScreenMainLoop(int64_t elapsedUs)
{
    paintDrawScreenPollTouch();

    // Screen Reset
    if (paintState->clearScreen)
    {
        hideCursor(getCursor(), &paintState->canvas);
        memcpy(paintState->canvas.palette, defaultPalette, PAINT_MAX_COLORS * sizeof(paletteColor_t));
        getArtist()->fgColor = paintState->canvas.palette[0];
        getArtist()->bgColor = paintState->canvas.palette[1];
        paintClearCanvas(&paintState->canvas, getArtist()->bgColor);
        paintRenderToolbar(getArtist(), &paintState->canvas, paintState, firstBrush, lastBrush);
        paintUpdateLeds();
        showCursor(getCursor(), &paintState->canvas);
        paintState->unsaved = false;
        paintState->clearScreen = false;
    }

    // Save and Load
    if (paintState->doSave || paintState->doLoad)
    {
        paintState->saveInProgress = true;

        if (paintState->doSave)
        {
            hideCursor(getCursor(), &paintState->canvas);
            paintHidePickPoints();
            paintSave(&paintState->index, &paintState->canvas, paintState->selectedSlot);
            paintDrawPickPoints();
            showCursor(getCursor(), &paintState->canvas);
        }
        else
        {
            if (paintGetSlotInUse(paintState->index, paintState->selectedSlot))
            {
                // Load from the selected slot if it's been used
                hideCursor(getCursor(), &paintState->canvas);
                paintClearCanvas(&paintState->canvas, getArtist()->bgColor);
                if(paintLoadDimensions(&paintState->canvas, paintState->selectedSlot))
                {
                    paintPositionDrawCanvas();
                    paintLoad(&paintState->index, &paintState->canvas, paintState->selectedSlot);
                    paintSetRecentSlot(&paintState->index, paintState->selectedSlot);

                    paintFreeUndos();

                    getArtist()->fgColor = paintState->canvas.palette[0];
                    getArtist()->bgColor = paintState->canvas.palette[1];

                    // Do the tool setup, which will also setup the cursor
                    paintSetupTool();

                    // Put the cursor in the middle of the screen
                    moveCursorAbsolute(getCursor(), &paintState->canvas, paintState->canvas.w / 2, paintState->canvas.h / 2);
                    showCursor(getCursor(), &paintState->canvas);
                    paintUpdateLeds();
                }
                else
                {
                    PAINT_LOGE("Slot %d has 0 dimension! Stopping load and clearing slot", paintState->selectedSlot);
                    paintClearSlot(&paintState->index, paintState->selectedSlot);
                    paintReturnToMainMenu();
                }
            }
            else
            {
                // If the slot hasn't been used yet, just clear the screen
                paintState->clearScreen = true;
            }
        }

        paintState->unsaved = false;
        paintState->doSave = false;
        paintState->doLoad = false;
        paintState->saveInProgress = false;

        paintState->buttonMode = BTN_MODE_DRAW;
        paintState->saveMenu = HIDDEN;

        paintState->redrawToolbar = true;
    }

    if (paintState->recolorPickPoints)
    {
        paintDrawPickPoints();
        paintUpdateLeds();
        paintState->recolorPickPoints = false;
    }


    if (paintState->redrawToolbar)
    {
        paintRenderToolbar(getArtist(), &paintState->canvas, paintState, firstBrush, lastBrush);
        paintState->redrawToolbar = false;
    }
    else
    {
        // Don't remember why we only do this when redrawToolbar is true
        // Oh, it's because `paintState->redrawToolbar` is mostly only set in select mode unless you press B?
        if (paintState->aHeld || paintState->aPress)
        {
            paintDoTool(getCursor()->x, getCursor()->y, getArtist()->fgColor);

            if (getArtist()->brushDef->mode != HOLD_DRAW)
            {
                paintState->aHeld = false;
            }

            paintState->aPress = false;
        }

        if (paintState->moveX || paintState->moveY || paintState->unhandledButtons)
        {
            bool clearMovement = false;

            if (!paintState->moveX && !paintState->moveY)
            {
                paintHandleDpad(paintState->unhandledButtons);
                clearMovement = true;
            }

            paintState->btnHoldTime += elapsedUs;
            if (paintState->firstMove || paintState->btnHoldTime >= BUTTON_REPEAT_TIME)
            {
                moveCursorRelative(getCursor(), &paintState->canvas, paintState->moveX, paintState->moveY);
                paintRenderToolbar(getArtist(), &paintState->canvas, paintState, firstBrush, lastBrush);

                paintState->firstMove = false;
            }

            paintState->unhandledButtons = 0;
            if (clearMovement)
            {
                paintState->moveX = 0;
                paintState->moveY = 0;
            }
        }
    }

    if (paintState->index & PAINT_ENABLE_BLINK)
    {
        if (paintState->blinkOn && paintState->blinkTimer >= BLINK_TIME_ON)
        {
            paintState->blinkTimer %= BLINK_TIME_ON;
            paintState->blinkOn = false;
            paintHidePickPoints();
        }
        else if (!paintState->blinkOn && paintState->blinkTimer >= BLINK_TIME_OFF)
        {
            paintState->blinkTimer %= BLINK_TIME_OFF;
            paintState->blinkOn = true;
            paintDrawPickPoints();
        } else if (paintState->blinkOn)
        {
            paintDrawPickPoints();
        }

        paintState->blinkTimer += elapsedUs;
    }
    else
    {
        paintDrawPickPoints();
    }

    drawCursor(getCursor(), &paintState->canvas);

    if (paintHelp != NULL)
    {
        int16_t pastColorBoxX = PAINT_COLORBOX_MARGIN_X + (paintState->canvas.x - 1 - PAINT_COLORBOX_W - PAINT_COLORBOX_MARGIN_X * 2 - 2) / 2 + PAINT_COLORBOX_W + 2 + 6;
        int16_t wrapY = paintState->canvas.y + paintState->canvas.h * paintState->canvas.yScale + 3;
        const char* rest = drawTextWordWrap(&paintState->toolbarFont, c000, paintHelp->curHelp->prompt,
                 &pastColorBoxX, &wrapY,
                 TFT_WIDTH, TFT_HEIGHT - paintState->marginBottom + paintHelp->helpH);
        if (rest)
        {
            PAINT_LOGW("Some tutorial text didn't fit: %s", rest);
        }
    }
}

void paintSaveModePrevItem(void)
{
    switch (paintState->saveMenu)
    {
        case HIDDEN:
        break;

        case UNDO:
            paintState->saveMenu = EXIT;
        break;

        case REDO:
            paintState->saveMenu = UNDO;
        break;

        case PICK_SLOT_SAVE:
        case CONFIRM_OVERWRITE:
            paintState->saveMenu = REDO;
        break;

        case PICK_SLOT_LOAD:
        case CONFIRM_UNSAVED:
            paintState->saveMenu = PICK_SLOT_SAVE;
        break;

        case EDIT_PALETTE:
        case COLOR_PICKER:
            paintState->saveMenu = PICK_SLOT_LOAD;
        break;

        case CLEAR:
        case CONFIRM_CLEAR:
            paintState->saveMenu = EDIT_PALETTE;
        break;

        case EXIT:
        case CONFIRM_EXIT:
            paintState->saveMenu = CLEAR;
        break;
    }

    paintState->saveMenuBoolOption = false;

    // Check to make sure we can actually redo
    if (paintState->saveMenu == REDO && !paintCanRedo())
    {
        // Nothing to redo, go to next
        paintState->saveMenu = UNDO;
    }

    // Check to make sure we can actually undo
    if (paintState->saveMenu == UNDO && !paintCanUndo())
    {
        paintState->saveMenu = EXIT;
    }

    // If we're selecting "Load", then make sure we can actually load a slot
    if (paintState->saveMenu == PICK_SLOT_LOAD)
    {
        // If no slots are in use, skip again
        if (!paintGetAnySlotInUse(paintState->index))
        {
            paintState->saveMenu = PICK_SLOT_SAVE;
        }
        else if (!paintGetSlotInUse(paintState->index, paintState->selectedSlot))
        {
            // Otherwise, make sure the selected slot is in use
            paintState->selectedSlot = paintGetNextSlotInUse(paintState->index, paintState->selectedSlot);
        }
    }
}

void paintSaveModeNextItem(void)
{
    switch (paintState->saveMenu)
    {
        case HIDDEN:
        break;

        case UNDO:
            paintState->saveMenu = REDO;
        break;

        case REDO:
            paintState->saveMenu = PICK_SLOT_SAVE;
        break;

        case PICK_SLOT_SAVE:
        case CONFIRM_OVERWRITE:
            paintState->saveMenu = PICK_SLOT_LOAD;
        break;

        case PICK_SLOT_LOAD:
        case CONFIRM_UNSAVED:
            paintState->saveMenu = EDIT_PALETTE;
        break;

        case EDIT_PALETTE:
        case COLOR_PICKER:
            paintState->saveMenu = CLEAR;
        break;

        case CLEAR:
        case CONFIRM_CLEAR:
            paintState->saveMenu = EXIT;
        break;

        case EXIT:
        case CONFIRM_EXIT:
            paintState->saveMenu = UNDO;
        break;
    }

    paintState->saveMenuBoolOption = false;

    // Check to make sure we can actually undo
    if (paintState->saveMenu == UNDO && !paintCanUndo())
    {
        paintState->saveMenu = REDO;
    }

    // Check to make sure we can actually redo
    if (paintState->saveMenu == REDO && !paintCanRedo())
    {
        // Nothing to redo, go to next
        paintState->saveMenu = PICK_SLOT_SAVE;
    }

    // If we're selecting "Load", then make sure we can actually load a slot
    if (paintState->saveMenu == PICK_SLOT_LOAD)
    {
        // If no slots are in use, skip again
        if (!paintGetAnySlotInUse(paintState->index))
        {
            paintState->saveMenu = EDIT_PALETTE;
        }
        else if (!paintGetSlotInUse(paintState->index, paintState->selectedSlot))
        {
            // Otherwise, make sure the selected slot is in use
            paintState->selectedSlot = paintGetNextSlotInUse(paintState->index, paintState->selectedSlot);
        }
    }
}

void paintSaveModePrevOption(void)
{
    switch (paintState->saveMenu)
    {
        case PICK_SLOT_SAVE:
            paintState->selectedSlot = PREV_WRAP(paintState->selectedSlot, PAINT_SAVE_SLOTS);
        break;

        case PICK_SLOT_LOAD:
            paintState->selectedSlot = paintGetPrevSlotInUse(paintState->index, paintState->selectedSlot);
        break;

        case CONFIRM_OVERWRITE:
        case CONFIRM_UNSAVED:
        case CONFIRM_CLEAR:
        case CONFIRM_EXIT:
            // Just flip the state
            paintState->saveMenuBoolOption = !paintState->saveMenuBoolOption;
        break;

        case HIDDEN:
        case UNDO:
        case REDO:
        case EDIT_PALETTE:
        case COLOR_PICKER:
        case CLEAR:
        case EXIT:
        // Do nothing, there are no options here
        break;
    }
}

void paintSaveModeNextOption(void)
{
    switch (paintState->saveMenu)
    {
        case PICK_SLOT_SAVE:
            paintState->selectedSlot = NEXT_WRAP(paintState->selectedSlot, PAINT_SAVE_SLOTS);
        break;

        case PICK_SLOT_LOAD:
            paintState->selectedSlot = paintGetNextSlotInUse(paintState->index, paintState->selectedSlot);
        break;

        case CONFIRM_OVERWRITE:
        case CONFIRM_UNSAVED:
        case CONFIRM_CLEAR:
        case CONFIRM_EXIT:
            // Just flip the state
            paintState->saveMenuBoolOption = !paintState->saveMenuBoolOption;
        break;

        case HIDDEN:
        case UNDO:
        case REDO:
        case EDIT_PALETTE:
        case COLOR_PICKER:
        case CLEAR:
        case EXIT:
        // Do nothing, there are no options here
        break;
    }
}

void paintEditPaletteUpdate(void)
{
    paintState->newColor = (paintState->editPaletteR * 36 + paintState->editPaletteG * 6 + paintState->editPaletteB);
    paintState->redrawToolbar = true;
    paintUpdateLeds();
}

void paintEditPaletteSetChannelValue(uint8_t val)
{
    *(paintState->editPaletteCur) = val % 6;
    paintEditPaletteUpdate();
}

// void paintEditPaletteDecChannel(void)
// {
//     *(paintState->editPaletteCur) = PREV_WRAP(*(paintState->editPaletteCur), 6);
//     paintEditPaletteUpdate();
// }

void paintEditPaletteIncChannel(void)
{
    *(paintState->editPaletteCur) = NEXT_WRAP(*(paintState->editPaletteCur), 6);
    paintEditPaletteUpdate();
}

void paintEditPaletteNextChannel(void)
{
    if (paintState->editPaletteCur == &paintState->editPaletteR)
    {
        paintState->editPaletteCur = &paintState->editPaletteG;
    }
    else if (paintState->editPaletteCur == &paintState->editPaletteG)
    {
        paintState->editPaletteCur = &paintState->editPaletteB;
    }
    else
    {
        paintState->editPaletteCur = &paintState->editPaletteR;
    }
}

void paintEditPalettePrevChannel(void)
{
    if (paintState->editPaletteCur == &paintState->editPaletteR)
    {
        paintState->editPaletteCur = &paintState->editPaletteB;
    }
    else if (paintState->editPaletteCur == &paintState->editPaletteG)
    {
        paintState->editPaletteCur = &paintState->editPaletteR;
    }
    else
    {
        paintState->editPaletteCur = &paintState->editPaletteG;
    }
}

void paintEditPaletteSetupColor(void)
{
    paletteColor_t col = paintState->canvas.palette[paintState->paletteSelect];
    paintState->editPaletteCur = &paintState->editPaletteR;
    paintState->editPaletteR = col / 36;
    paintState->editPaletteG = (col / 6) % 6;
    paintState->editPaletteB = col % 6;
    paintState->newColor = col;
    paintEditPaletteUpdate();
}

void paintEditPalettePrevColor(void)
{
    paintState->paletteSelect = PREV_WRAP(paintState->paletteSelect, PAINT_MAX_COLORS);
    paintEditPaletteSetupColor();
}

void paintEditPaletteNextColor(void)
{
    paintState->paletteSelect = NEXT_WRAP(paintState->paletteSelect, PAINT_MAX_COLORS);
    paintEditPaletteSetupColor();
}

void paintEditPaletteConfirm(void)
{
    paintStoreUndo(&paintState->canvas);

    // Save the old color, and update the palette with the new color
    paletteColor_t old = paintState->canvas.palette[paintState->paletteSelect];
    paletteColor_t new = paintState->newColor;
    paintState->canvas.palette[paintState->paletteSelect] = new;

    // Only replace the color on the canvas if the old color is no longer in the palette
    bool doReplace = true;
    for (uint8_t i = 0; i < PAINT_MAX_COLORS; i++)
    {
        if (paintState->canvas.palette[i] == old)
        {
            doReplace = false;
            break;
        }
    }

    // This color is no longer in the palette, so replace it with the new one
    if (doReplace)
    {
        // Make sure the FG/BG colors aren't outsie of the palette
        if (getArtist()->fgColor == old)
        {
            getArtist()->fgColor = new;
        }

        if (getArtist()->bgColor == old)
        {
            getArtist()->bgColor = new;
        }

        hideCursor(getCursor(), &paintState->canvas);
        paintHidePickPoints();

        // And replace it within the canvas
        paintColorReplace(&paintState->canvas, old, new);
        paintState->unsaved = true;

        paintDrawPickPoints();
        showCursor(getCursor(), &paintState->canvas);
    }
}

void paintPaletteModeButtonCb(const buttonEvt_t* evt)
{
    if (evt->down)
    {
        paintState->redrawToolbar = true;
        switch (evt->button)
        {
            case PB_A:
            {
                // Don't do anything? Confirm change?
                paintEditPaletteConfirm();
                break;
            }

            case PB_B:
            {
                // Revert back to the original color
                if (paintState->newColor != paintState->canvas.palette[paintState->paletteSelect])
                {
                    paintEditPaletteSetupColor();
                }
                else
                {
                    paintState->paletteSelect = 0;
                    paintState->buttonMode = BTN_MODE_DRAW;
                    paintState->saveMenu = HIDDEN;
                }
                break;
            }

            case PB_START:
            {
                // Handled in button up
                break;
            }

            case PB_SELECT:
            {
                // {R/G/B}++
                // We will normally use the touchpad for this
                paintEditPaletteIncChannel();
                break;
            }

            case PB_UP:
            {
                // Prev color
                paintEditPalettePrevColor();
                break;
            }

            case PB_DOWN:
            {
                // Next color
                paintEditPaletteNextColor();
                break;
            }

            case PB_LEFT:
            {
                // Swap between R, G, and B
                paintEditPalettePrevChannel();
                break;
            }

            case PB_RIGHT:
            {
                // Swap between R, G, and B
                paintEditPaletteNextChannel();
                break;
            }
        }
    }
    else
    {
        // Button up
        if (evt->button == PB_START)
        {
            // Return to draw mode
            paintState->paletteSelect = 0;
            paintState->buttonMode = BTN_MODE_DRAW;
            paintState->saveMenu = HIDDEN;
        }
    }
}

void paintSaveModeButtonCb(const buttonEvt_t* evt)
{
    if (evt->down)
    {
        //////// Save menu button down
        paintState->redrawToolbar = true;
        switch (evt->button)
        {
            case PB_A:
            {
                switch (paintState->saveMenu)
                {
                    case UNDO:
                    {
                        paintUndo(&paintState->canvas);
                        if (paintState->undoHead != NULL && paintState->undoHead->prev == NULL)
                        {
                            paintState->saveMenu = REDO;
                        }
                        break;
                    }

                    case REDO:
                    {
                        paintRedo(&paintState->canvas);
                        if (paintState->undoHead != NULL && paintState->undoHead->next == NULL)
                        {
                            paintState->saveMenu = UNDO;
                        }
                        break;
                    }

                    case PICK_SLOT_SAVE:
                    {
                        if (paintGetSlotInUse(paintState->index, paintState->selectedSlot))
                        {
                            paintState->saveMenuBoolOption = false;
                            paintState->saveMenu = CONFIRM_OVERWRITE;
                        }
                        else
                        {
                            paintState->doSave = true;
                        }
                        break;
                    }

                    case PICK_SLOT_LOAD:
                    {
                        if (paintState->unsaved)
                        {
                            paintState->saveMenuBoolOption = false;
                            paintState->saveMenu = CONFIRM_UNSAVED;
                        }
                        else
                        {
                            paintState->doLoad = true;
                        }
                        break;
                    }

                    case CONFIRM_OVERWRITE:
                    {
                        if (paintState->saveMenuBoolOption)
                        {
                            paintState->doSave = true;
                            paintState->saveMenu = HIDDEN;
                        }
                        else
                        {
                            paintState->saveMenu = PICK_SLOT_SAVE;
                        }
                        break;
                    }

                    case CONFIRM_UNSAVED:
                    {
                        if (paintState->saveMenuBoolOption)
                        {
                            paintState->doLoad = true;
                            paintState->saveMenu = HIDDEN;
                        }
                        else
                        {
                            paintState->saveMenu = PICK_SLOT_LOAD;
                        }
                        break;
                    }

                    case EDIT_PALETTE:
                    {
                        paintState->saveMenu = COLOR_PICKER;
                        paintState->buttonMode = BTN_MODE_PALETTE;
                        paintState->paletteSelect = 0;
                        paintEditPaletteSetupColor();
                        break;
                    }

                    case CONFIRM_CLEAR:
                    {
                        if (paintState->saveMenuBoolOption)
                        {
                            paintStoreUndo(&paintState->canvas);
                            paintState->clearScreen = true;
                            paintState->saveMenu = HIDDEN;
                            paintState->buttonMode = BTN_MODE_DRAW;
                        }
                        else
                        {
                            paintState->saveMenu = CLEAR;
                        }
                        break;
                    }

                    case CONFIRM_EXIT:
                    {
                        if (paintState->saveMenuBoolOption)
                        {
                            paintReturnToMainMenu();
                        }
                        else
                        {
                            paintState->saveMenu = EXIT;
                        }
                        break;
                    }

                    case CLEAR:
                    {
                        if (paintState->unsaved)
                        {
                            paintState->saveMenuBoolOption = false;
                            paintState->saveMenu = CONFIRM_CLEAR;
                        }
                        else
                        {
                            paintStoreUndo(&paintState->canvas);
                            paintState->clearScreen = true;
                            paintState->saveMenu = HIDDEN;
                            paintState->buttonMode = BTN_MODE_DRAW;
                        }
                        break;
                    }
                    case EXIT:
                    {
                        if (paintState->unsaved)
                        {
                            paintState->saveMenuBoolOption = false;
                            paintState->saveMenu = CONFIRM_EXIT;
                        }
                        else
                        {
                            paintReturnToMainMenu();
                        }
                        break;
                    }
                    // These cases shouldn't actually happen
                    case HIDDEN:
                    case COLOR_PICKER:
                    {
                        paintState->buttonMode = BTN_MODE_DRAW;
                        break;
                    }
                }
                break;
            }

            case PB_UP:
            {
                paintSaveModePrevItem();
                break;
            }

            case PB_DOWN:
            case PB_SELECT:
            {

                paintSaveModeNextItem();
                break;
            }

            case PB_LEFT:
            {
                paintSaveModePrevOption();
                break;
            }

            case PB_RIGHT:
            {
                paintSaveModeNextOption();
                break;
            }

            case PB_B:
            {
                // Exit save menu
                paintState->saveMenu = HIDDEN;
                paintState->buttonMode = BTN_MODE_DRAW;
                break;
            }

            case PB_START:
            // Handle this in button up
            break;
        }
    }
    else
    {
        //////// Save mode button release
        if (evt->button == PB_START)
        {
            // Exit save menu
            paintState->saveMenu = HIDDEN;
            paintState->buttonMode = BTN_MODE_DRAW;
            paintState->redrawToolbar = true;
        }
    }
}

void paintSelectModeButtonCb(const buttonEvt_t* evt)
{
    if (!evt->down)
    {
        //////// Select-mode button release
        switch (evt->button)
        {
            case PB_SELECT:
            {
                if (paintCanUndo())
                {
                    paintUndo(&paintState->canvas);
                }
                break;
            }

            case PB_START:
            {
                if (paintCanRedo())
                {
                    paintRedo(&paintState->canvas);
                }
                break;
            }

            case PB_UP:
            {
                // Select previous color
                paintState->redrawToolbar = true;
                paintState->paletteSelect = PREV_WRAP(paintState->paletteSelect, PAINT_MAX_COLORS);
                paintUpdateLeds();
                break;
            }

            case PB_DOWN:
            {
                // Select next color
                paintState->redrawToolbar = true;
                paintState->paletteSelect = NEXT_WRAP(paintState->paletteSelect, PAINT_MAX_COLORS);
                paintUpdateLeds();
                break;
            }

            case PB_LEFT:
            {
                // Select previous brush
                paintPrevTool();
                paintState->redrawToolbar = true;
                break;
            }

            case PB_RIGHT:
            {
                // Select next brush
                paintNextTool();
                paintState->redrawToolbar = true;
                break;
            }


            case PB_A:
            {
                // Increase brush size / next variant
                paintIncBrushWidth(1);
                paintState->redrawToolbar = true;
                break;
            }

            case PB_B:
            {
                // Decrease brush size / prev variant
                paintDecBrushWidth(1);
                paintState->redrawToolbar = true;
                break;
            }
        }
    }
}

void paintDrawScreenPollTouch()
{
    int32_t centroid, intensity;

    if (getTouchCentroid(&centroid, &intensity))
    {
        // Bar is touched
        switch (paintState->buttonMode)
        {
            case BTN_MODE_DRAW:
            case BTN_MODE_SELECT:
            {
                paintState->lastTouch = centroid;

                // Set up variables for swiping
                if (!paintState->touchDown)
                {
                    // Beginning of swipe
                    paintState->touchDown = true;
                    paintState->firstTouch = centroid;

                    // Store the original brush width
                    paintState->startBrushWidth = getArtist()->brushWidth;
                    paintEnterSelectMode();

                    // Only call this here to prevent making a ton of unnecessary calls to paintTutorialOnEvent()
                    if (paintHelp != NULL)
                    {
                        // Don't worry about X or Y, we'll only decide those on release I guess
                        paintHelp->allButtons |= TOUCH_ANY;
                        // Replace the touch buttons, but not any of the real buttons
                        paintHelp->curButtons = TOUCH_ANY | (paintHelp->curButtons & (PB_UP | PB_DOWN | PB_LEFT | PB_RIGHT | PB_A | PB_B | PB_START | PB_SELECT));
                        paintHelp->lastButton = TOUCH_ANY;
                        paintHelp->lastButtonDown = true;
                        paintTutorialOnEvent();
                    }
                }
                else
                {
                    // We're mid-swipe
                    int32_t swipeMagnitude = ((paintState->firstTouch - centroid) * PAINT_MAX_BRUSH_SWIPE) / 1024;
                    int32_t newWidth = paintState->startBrushWidth - swipeMagnitude;

                    if (newWidth < 0)
                    {
                        newWidth = 0;
                    }
                    else if (newWidth > UINT8_MAX)
                    {
                        newWidth = UINT8_MAX;
                    }

                    paintSetBrushWidth((uint8_t)(newWidth));
                }
                break;
            }

            case BTN_MODE_PALETTE:
            {
                paintState->touchDown = true;
                // Don't do anything for tutorial until release
                uint8_t index = ((centroid * 5 + 512) / 1024);
                PAINT_LOGD("Centroid: %d, Intensity: %d, Index: %d", centroid, intensity, index);
                paintEditPaletteSetChannelValue(index);
                break;
            }

            case BTN_MODE_SAVE:
            break;
        }
    }
    else
    {
        // Bar is not touched
        // Do not use centroid/intensity here
        // And only do anything if paintState->touchDown is still true
        if (paintState->touchDown)
        {
            paintState->touchDown = false;

            switch (paintState->buttonMode)
            {
                case BTN_MODE_DRAW:
                case BTN_MODE_SELECT:
                {
                    int32_t swipeMagnitude = ((paintState->firstTouch - paintState->lastTouch) * PAINT_MAX_BRUSH_SWIPE) / 1024;
                    PAINT_LOGD("End swipe: %d", swipeMagnitude);
                    if (swipeMagnitude == 0)
                    {
                        // Tap! But only if we started on X or Y
                        if (paintState->firstTouch < (1024 / 5))
                        {
                            paintDecBrushWidth(1);
                        }
                        else if (paintState->firstTouch > (1024 * 4 / 5))
                        {
                            paintIncBrushWidth(1);
                        }
                    }

                    if (paintHelp != NULL)
                    {
                        paintHelp->curButtons = paintHelp->curButtons & (PB_UP | PB_DOWN | PB_LEFT | PB_RIGHT | PB_A | PB_B | PB_START | PB_SELECT);
                        paintHelp->lastButtonDown = false;

                        if (swipeMagnitude == 0)
                        {
                            if (paintState->firstTouch < (1024 / 5))
                            {
                                paintHelp->lastButton = TOUCH_Y;
                            }
                            else if (paintState->firstTouch > (1024 * 4 / 5))
                            {
                                paintHelp->lastButton = TOUCH_X;
                            }
                            else
                            {
                                paintHelp->lastButton = TOUCH_ANY;
                            }
                        }
                        else if (swipeMagnitude > 0)
                        {
                            paintHelp->lastButton = SWIPE_RIGHT;
                        }
                        else // swipeMagnitude < 0
                        {
                            paintHelp->lastButton = SWIPE_LEFT;
                        }

                        paintTutorialOnEvent();
                    }

                    paintExitSelectMode();
                    break;
                }

                case BTN_MODE_PALETTE:
                {
                    // Only do something in tutorial mode
                    if (paintHelp != NULL)
                    {
                        paintHelp->curButtons = paintHelp->curButtons & (PB_UP | PB_DOWN | PB_LEFT | PB_RIGHT | PB_A | PB_B | PB_START | PB_SELECT);
                        paintHelp->lastButtonDown = false;
                        paintHelp->lastButton = TOUCH_ANY;
                        paintTutorialOnEvent();
                    }

                    break;
                }

                case BTN_MODE_SAVE:
                break;
            }
        }
    }
}

void paintDrawScreenTouchCb(const buttonEvt_t* evt)
{
    // Call the centroid polling function here too
    paintDrawScreenPollTouch();
}

void paintDrawScreenButtonCb(const buttonEvt_t* evt)
{
    if (paintHelp != NULL)
    {
        paintHelp->allButtons |= evt->state;
        paintHelp->lastButton = evt->button;
        paintHelp->lastButtonDown = evt->down;
        // Keep the touch buttons in place but replace everything else with the button state
        paintHelp->curButtons = evt->state | (paintHelp->curButtons & (TOUCH_ANY | TOUCH_X | TOUCH_Y | SWIPE_LEFT | SWIPE_RIGHT));
    }

    switch (paintState->buttonMode)
    {
        case BTN_MODE_DRAW:
        {
            paintDrawModeButtonCb(evt);
            break;
        }

        case BTN_MODE_SELECT:
        {
            paintSelectModeButtonCb(evt);
            break;
        }

        case BTN_MODE_SAVE:
        {
            paintSaveModeButtonCb(evt);
            break;
        }

        case BTN_MODE_PALETTE:
        {
            paintPaletteModeButtonCb(evt);
            break;
        }
    }

    if (paintHelp != NULL)
    {
        paintTutorialOnEvent();
    }
}

void paintDrawModeButtonCb(const buttonEvt_t* evt)
{
    if (evt->down)
    {
        // Draw mode buttons
        switch (evt->button)
        {
            case PB_SELECT:
            // SELECT no longer does anything
            break;

            case PB_A:
            {
                // Draw
                paintState->aHeld = true;
                paintState->aPress = true;
                break;
            }

            case PB_B:
            {
                // Swap the foreground and background colors
                paintSwapFgBgColors();

                paintState->redrawToolbar = true;
                paintState->recolorPickPoints = true;
                break;
            }

            case PB_UP:
            case PB_DOWN:
            case PB_LEFT:
            case PB_RIGHT:
            {
                paintHandleDpad(evt->state & (PB_UP | PB_DOWN | PB_LEFT | PB_RIGHT));
                paintState->firstMove = true;
                break;
            }

            case PB_START:
            // Don't do anything until start is released to avoid conflicting with EXIT
            break;
        }
    }
    else
    {
        //////// Draw mode button release
        switch (evt->button)
        {
            case PB_START:
            {
                if (!paintState->saveInProgress)
                {
                    // Enter the save menu
                    paintState->buttonMode = BTN_MODE_SAVE;
                    paintState->saveMenu = PICK_SLOT_SAVE;
                    paintState->redrawToolbar = true;

                    // Don't let the cursor keep moving
                    paintState->moveX = 0;
                    paintState->moveY = 0;
                    paintState->btnHoldTime = 0;
                    paintState->aHeld = false;
                }
                break;
            }

            case PB_A:
            {
                // Stop drawing
                paintState->aHeld = false;
                break;
            }

            case PB_B:
            // Do nothing; color swap is handled on button down
            break;

            case PB_UP:
            case PB_DOWN:
            case PB_LEFT:
            case PB_RIGHT:
            {
                paintHandleDpad(evt->state & (PB_UP | PB_DOWN | PB_LEFT | PB_RIGHT));
                break;
            }

            case PB_SELECT:
            // This is handled in BTN_MODE_SELECT already
            break;
        }
    }
}

void paintHandleDpad(uint16_t state)
{
    paintState->unhandledButtons |= state;

    if (!(state & PB_UP) != !(state & PB_DOWN))
    {
        // Up or down, but not both, are pressed
        paintState->moveY = (state & PB_DOWN) ? 1 : -1;
    }
    else
    {
        paintState->moveY = 0;
    }

    if (!(state & PB_LEFT) != !(state & PB_RIGHT))
    {
        // Left or right, but not both, are pressed
        paintState->moveX = (state & PB_RIGHT) ? 1 : -1;
    }
    else
    {
        paintState->moveX = 0;
    }

    if (!state)
    {
        // Reset the button hold time if all D-pad buttons are released
        // This lets you make turns quickly instead of waiting for the repeat timeout in the middle
        paintState->btnHoldTime = 0;
    }
}

void paintFreeUndos(void)
{
    paintState->undoHead = NULL;
    for (node_t* undo = paintState->undoList.first; undo != NULL; undo = undo->next)
    {
        paintUndo_t* val = undo->val;
        free(val);
    }
    clear(&paintState->undoList);
}

void paintStoreUndo(paintCanvas_t* canvas)
{
    // If paintState->undoHead is set, we need to clear all the previous undos to delete the alternate timeline
    uint8_t deleted = 0;
    while (paintState->undoHead != NULL)
    {
        // Save the next pointer before the node gets freed
        node_t* next = paintState->undoHead->next;

        paintUndo_t* delUndo = removeEntry(&paintState->undoList, paintState->undoHead);

        // Free the undo data pixels and then the struct itself
        free(delUndo);

        paintState->undoHead = next;
        deleted++;
    }
    if (deleted > 0)
    {
        PAINT_LOGD("Deleted %d dangling undos after changing history", deleted);
    }
    // paintState->undoHead should now be NULL

    // Allocate a new paintUndo_t to store the canvas
    paintUndo_t* undoData;
    // Calculate the amount of space we wolud need to store the canvas pixels
    size_t pxSize = paintGetStoredSize(canvas);

    // Allocate memory for the undo data struct and its pixel data in one go
    void* undoMem = heap_caps_malloc(sizeof(paintUndo_t) + pxSize, MALLOC_CAP_SPIRAM);
    if (undoMem != NULL)
    {
        // Alloc succeeded, use the data
        undoData = undoMem;
        undoData->px = (uint8_t*)undoMem + sizeof(paintUndo_t);
    }
    else
    {
        // Alloc failed, reuse the first undo data
        undoData = shift(&paintState->undoList);
    }

    if (!undoData)
    {
        PAINT_LOGD("Failed to allocate or reuse undo data! Canceling undo");
        // There's no undo data at all! We're completely out of space!
        return;
    }

    // Save the palette
    memcpy(undoData->palette, canvas->palette, sizeof(paletteColor_t) * PAINT_MAX_COLORS);

    bool cursorVisible = getCursor()->show;
    if (cursorVisible)
    {
        hideCursor(getCursor(), canvas);
    }
    // Save the pixel data
    paintSerialize(undoData->px, canvas, 0, pxSize);

    if (cursorVisible)
    {
        showCursor(getCursor(), canvas);
    }

    push(&paintState->undoList, undoData);
}

// Delete the oldest undo entry, if one exists. Returns true if some space was made available, false otherwise.
bool paintMaybeSacrificeUndoForHeap(void)
{
    if (paintState->undoList.first != NULL)
    {
        // Don't leave a bad pointer in undoHead
        // This has to be done *before* calling shift()
        if (paintState->undoHead != NULL && paintState->undoHead == paintState->undoList.first)
        {
            paintState->undoHead = paintState->undoHead->next;
        }

        paintUndo_t* delUndo = shift(&paintState->undoList);

        free(delUndo);

        return true;
    }

    return false;
}

bool paintCanUndo()
{
    // We can undo as long as one of these is true:
    //  - The undoHead is NULL and undoList.last is NOT NULL
    //  - The undoHead is NOT NULL and undoHead->prev is NOT NULL
    return (paintState->undoHead == NULL && paintState->undoList.last != NULL) || (paintState->undoHead != NULL && paintState->undoHead->prev != NULL);
}

bool paintCanRedo()
{
    // We can redo as long as all of these are true:
    //  - The undoHead is NOT NULL
    //  - There is another undo after undoHead (that's what contains the state we want to return to)
    return paintState->undoHead != NULL && paintState->undoHead->next != NULL;
}

void paintApplyUndo(paintCanvas_t* canvas)
{
    if (paintState->undoHead == NULL)
    {
        // If we've undone everything, or there's nothing to undo, exit early
        PAINT_LOGD("Not undoing because undoHead is NULL");
        return;
    }

    hideCursor(getCursor(), canvas);

    paintUndo_t* undo = paintState->undoHead->val;

    memcpy(canvas->palette, undo->palette, sizeof(paletteColor_t) * PAINT_MAX_COLORS);
    getArtist()->fgColor = canvas->palette[0];
    getArtist()->bgColor = canvas->palette[1];

    size_t pxSize = paintGetStoredSize(canvas);
    paintDeserialize(canvas, undo->px, 0, pxSize);

    PAINT_LOGD("Undid %zu bytes!", pxSize);

    // feels weird to do this inside the undo functions... but it's probably ok? we've already undone anyway
    showCursor(getCursor(), canvas);
}

void paintUndo(paintCanvas_t* canvas)
{
    if (paintState->undoHead == NULL)
    {
        // We have not undone anything else yet -- use the last element in the undo list
        node_t* head = paintState->undoList.last;

        // Also, since this is the first undo, save the current state so that we can return to it with redo
        paintStoreUndo(canvas);

        paintState->undoHead = head;
    }
    else
    {
        // We have already undone something! Undo the previous action.
        paintState->undoHead = paintState->undoHead->prev;
    }

    paintApplyUndo(canvas);
}

void paintRedo(paintCanvas_t* canvas)
{
    if (paintState->undoHead == NULL)
    {
        // We have not undone anything else -- so there's nothing to redo?
    }
    else
    {
        // We have already undone something, so there's something to redo
        paintState->undoHead = paintState->undoHead->next;
    }

    paintApplyUndo(canvas);
}

void paintDoTool(uint16_t x, uint16_t y, paletteColor_t col)
{
    hideCursor(getCursor(), &paintState->canvas);
    bool drawNow = false;
    bool isLastPick = false;

    // Determine if this is the last pick for the tool
    // This is so we don't draw a pick-marker that will be immediately removed
    switch (getArtist()->brushDef->mode)
    {
        case PICK_POINT:
        isLastPick = (pxStackSize(&getArtist()->pickPoints) + 1 == getArtist()->brushDef->maxPoints);
        break;

        case PICK_POINT_LOOP:
        isLastPick = pxStackSize(&getArtist()->pickPoints) + 1 == getArtist()->brushDef->maxPoints - 1;
        break;

        case HOLD_DRAW:
        break;

        default:
        break;
    }

    pushPxScaled(&getArtist()->pickPoints, getCursor()->x, getCursor()->y, paintState->canvas.x, paintState->canvas.y, paintState->canvas.xScale, paintState->canvas.yScale);

    if (getArtist()->brushDef->mode == HOLD_DRAW)
    {
        drawNow = true;
    }
    else if (getArtist()->brushDef->mode == PICK_POINT || getArtist()->brushDef->mode == PICK_POINT_LOOP)
    {
        // Save the pixel underneath the selection, then draw a temporary pixel to mark it
        // But don't bother if this is the last pick point, since it will never actually be seen

        if (getArtist()->brushDef->mode == PICK_POINT_LOOP)
        {
            pxVal_t firstPick, lastPick;
            if (pxStackSize(&getArtist()->pickPoints) > 1 && getPx(&getArtist()->pickPoints, 0, &firstPick) && peekPx(&getArtist()->pickPoints, &lastPick) && firstPick.x == lastPick.x && firstPick.y == lastPick.y)
            {
                // If this isn't the first pick, and it's in the same position as the first pick, we're done!
                drawNow = true;
            }
            else if (isLastPick)
            {
                // Special case: If we're on the next-to-last possible point, we have to add the start again as the last point
                pushPx(&getArtist()->pickPoints, firstPick.x, firstPick.y);

                drawNow = true;
            }
        }
        // only for non-loop brushes
        else if (pxStackSize(&getArtist()->pickPoints) == getArtist()->brushDef->maxPoints)
        {
            drawNow = true;
        }
    }

    if (drawNow)
    {
        // Allocate an array of point_t for the canvas pick points
        size_t pickCount = pxStackSize(&getArtist()->pickPoints);
        point_t canvasPickPoints[sizeof(point_t) * pickCount];

        // Convert the pick points into an array of canvas-coordinates
        paintConvertPickPointsScaled(&getArtist()->pickPoints, &paintState->canvas, canvasPickPoints);

        while (popPxScaled(&getArtist()->pickPoints, paintState->canvas.xScale, paintState->canvas.yScale));

        // Save the current state before we draw, but only do it on the first press if we're using a HOLD_DRAW pen
        if (getArtist()->brushDef->mode != HOLD_DRAW || paintState->aPress)
        {
            paintStoreUndo(&paintState->canvas);
        }

        paintState->unsaved = true;
        getArtist()->brushDef->fnDraw(&paintState->canvas, canvasPickPoints, pickCount, getArtist()->brushWidth, col);

        if (paintHelp != NULL)
        {
            paintHelp->drawComplete = true;
        }
    }
    else
    {
        // A bit counterintuitively, this will restart the blink timer on the next frame
        paintState->blinkTimer = BLINK_TIME_OFF;
        paintState->blinkOn = false;
    }

    showCursor(getCursor(), &paintState->canvas);
    paintRenderToolbar(getArtist(), &paintState->canvas, paintState, firstBrush, lastBrush);
}


void paintSetupTool(void)
{
    // Reset the brush params
    if (getArtist()->brushWidth < getArtist()->brushDef->minSize)
    {
        getArtist()->brushWidth = getArtist()->brushDef->minSize;
        paintState->startBrushWidth = getArtist()->brushWidth;
    }
    else if (getArtist()->brushWidth > getArtist()->brushDef->maxSize)
    {
        getArtist()->brushWidth = getArtist()->brushDef->maxSize;
        paintState->startBrushWidth = getArtist()->brushWidth;
    }

    hideCursor(getCursor(), &paintState->canvas);
    paintHidePickPoints();
    switch (getArtist()->brushDef->mode)
    {
        case HOLD_DRAW:
        {
            // Regenerate the cursor if it's not been set yet or if the brush's size is different from the cursor's size
            if (paintState->cursorWsg.px == NULL || paintState->cursorWsg.w != (getArtist()->brushWidth * paintState->canvas.xScale + 2) || paintState->cursorWsg.h != (getArtist()->brushWidth * paintState->canvas.yScale + 2))
            {
                paintFreeCursorSprite(&paintState->cursorWsg);
                paintGenerateCursorSprite(&paintState->cursorWsg, &paintState->canvas, getArtist()->brushWidth);
            }

            setCursorSprite(getCursor(), &paintState->canvas, &paintState->cursorWsg);
            // Center the cursor, accounting for even and odd cursor sizes
            setCursorOffset(getCursor(), -(paintState->cursorWsg.w / 2) + getArtist()->brushWidth % 2, -(paintState->cursorWsg.h / 2) + getArtist()->brushWidth % 2);
            break;
        }

        case PICK_POINT:
        case PICK_POINT_LOOP:
        {
            setCursorSprite(getCursor(), &paintState->canvas, &paintState->picksWsg);
            // Place the top-right pixel of the pointer 1px inside the target pixel
            setCursorOffset(getCursor(), -paintState->picksWsg.w + 1, paintState->canvas.yScale - 1);
            break;
        }
    }

    // Undraw and hide any stored temporary pixels
    while (popPxScaled(&getArtist()->pickPoints, paintState->canvas.xScale, paintState->canvas.yScale));
    showCursor(getCursor(), &paintState->canvas);
}

void paintPrevTool(void)
{
    if (getArtist()->brushDef == firstBrush)
    {
        getArtist()->brushDef = lastBrush;
    }
    else
    {
        getArtist()->brushDef--;
    }

    paintSetupTool();
}

void paintNextTool(void)
{
    if (getArtist()->brushDef == lastBrush)
    {
        getArtist()->brushDef = firstBrush;
    }
    else
    {
        getArtist()->brushDef++;
    }

    paintSetupTool();
}

void paintSetBrushWidth(uint8_t width)
{
    if (width < getArtist()->brushDef->minSize)
    {
        getArtist()->brushWidth = getArtist()->brushDef->minSize;
    }
    else if (width > getArtist()->brushDef->maxSize)
    {
        getArtist()->brushWidth = getArtist()->brushDef->maxSize;
    }
    else
    {
        getArtist()->brushWidth = width;
    }

    paintSetupTool();
    paintState->redrawToolbar = true;
}

void paintDecBrushWidth(uint8_t dec)
{
    if (getArtist()->brushWidth <= dec || getArtist()->brushWidth <= getArtist()->brushDef->minSize)
    {
        getArtist()->brushWidth = getArtist()->brushDef->minSize;
    }
    else
    {
        getArtist()->brushWidth -= dec;
    }

    paintSetupTool();
    paintState->redrawToolbar = true;
}

void paintIncBrushWidth(uint8_t inc)
{
    getArtist()->brushWidth += inc;

    if (getArtist()->brushWidth > getArtist()->brushDef->maxSize)
    {
        getArtist()->brushWidth = getArtist()->brushDef->maxSize;
    }

    paintSetupTool();
    paintState->redrawToolbar = true;
}

void paintSwapFgBgColors(void)
{
    uint8_t fgIndex = 0, bgIndex = 0;
    swap(&getArtist()->fgColor, &getArtist()->bgColor);

    for (uint8_t i = 0; i < PAINT_MAX_COLORS; i++)
    {
        if (paintState->canvas.palette[i] == getArtist()->fgColor)
        {
            fgIndex = i;
        }
        else if (paintState->canvas.palette[i] == getArtist()->bgColor)
        {
            bgIndex = i;
        }
    }

    for (uint8_t i = fgIndex; i > 0; i--)
    {
        if (i == bgIndex)
        {
            continue;
        }
        paintState->canvas.palette[i] = paintState->canvas.palette[i - 1 + ((i < bgIndex) ? 1 : 0)];
    }

    paintState->canvas.palette[0] = getArtist()->fgColor;

    paintUpdateLeds();
    paintDrawPickPoints();
}

void paintEnterSelectMode(void)
{
    if (paintState->buttonMode == BTN_MODE_SELECT)
    {
        return;
    }

    paintState->buttonMode = BTN_MODE_SELECT;
    paintState->redrawToolbar = true;
    paintState->aHeld = false;
    paintState->moveX = 0;
    paintState->moveY = 0;
    paintState->btnHoldTime = 0;
}

void paintExitSelectMode(void)
{
    if (paintState->buttonMode == BTN_MODE_DRAW)
    {
        return;
    }

    // Exit select mode
    paintState->buttonMode = BTN_MODE_DRAW;

    // Set the current selection as the FG color and rearrange the rest
    paintUpdateRecents(paintState->paletteSelect);
    paintState->paletteSelect = 0;

    paintState->redrawToolbar = true;
}

void paintUpdateRecents(uint8_t selectedIndex)
{
    getArtist()->fgColor = paintState->canvas.palette[selectedIndex];

    for (uint8_t i = selectedIndex; i > 0; i--)
    {
        paintState->canvas.palette[i] = paintState->canvas.palette[i-1];
    }
    paintState->canvas.palette[0] = getArtist()->fgColor;

    paintUpdateLeds();

    // If there are any pick points, update their color to reduce confusion
    paintDrawPickPoints();
}

void paintUpdateLeds(void)
{
    uint32_t rgb = 0;

    // Only set the LED color if LEDs are enabled
    if (paintState->index & PAINT_ENABLE_LEDS)
    {
        if (paintState->buttonMode == BTN_MODE_PALETTE)
        {
            // Show the edited color if we're editing the palette
            rgb = paletteToRGB(paintState->newColor);
        }
        else if (paintState->buttonMode == BTN_MODE_SELECT)
        {
            // Show the selected color if we're picking colors
            rgb = paletteToRGB(paintState->canvas.palette[paintState->paletteSelect]);
        }
        else
        {
            // Otherwise, use the current draw color
            rgb = paletteToRGB(getArtist()->fgColor);
        }
    }

    for (uint8_t i = 0; i < NUM_LEDS; i++)
    {
        paintState->leds[i].b = (rgb >>  0) & 0xFF;
        paintState->leds[i].g = (rgb >>  8) & 0xFF;
        paintState->leds[i].r = (rgb >> 16) & 0xFF;
    }

    setLeds(paintState->leds, NUM_LEDS);
}

void paintDrawPickPoints(void)
{
    pxVal_t point;
    for (size_t i = 0; i < pxStackSize(&getArtist()->pickPoints); i++)
    {
        if (getPx(&getArtist()->pickPoints, i, &point))
        {
            bool invert = (i == 0 && getArtist()->brushDef->mode == PICK_POINT_LOOP) && pxStackSize(&getArtist()->pickPoints) > 1;
            plotRectFilled(point.x, point.y, point.x + paintState->canvas.xScale + 1, point.y + paintState->canvas.yScale + 1, invert ? getContrastingColor(point.col) : getArtist()->fgColor);
        }
    }
}

void paintHidePickPoints(void)
{
    pxVal_t point;
    for (size_t i = 0; i < pxStackSize(&getArtist()->pickPoints); i++)
    {
        if (getPx(&getArtist()->pickPoints, i, &point))
        {
            plotRectFilled(point.x, point.y, point.x + paintState->canvas.xScale + 1, point.y + paintState->canvas.yScale + 1, point.col);
        }
    }
}

paintArtist_t* getArtist(void)
{
    // TODO: Take player order into account
    return paintState->artist;
}

paintCursor_t* getCursor(void)
{
    // TODO Take player order into account
    return &paintState->artist->cursor;
}
