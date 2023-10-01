#include "paint_draw.h"

#include <string.h>
#include "esp_heap_caps.h"
#include "esp_random.h"

#include "hdw-bzr.h"
#include "hdw-btn.h"
#include "touchUtils.h"
#include "spiffs_wsg.h"

#include "paint_ui.h"
#include "paint_brush.h"
#include "paint_nvs.h"
#include "paint_util.h"
#include "mode_paint.h"
#include "paint_song.h"
#include "paint_help.h"
#include "paint_browser.h"

#include "wheel_menu.h"

#include "macros.h"

#define PAINT_DIE(msg, ...) do \
{ \
    PAINT_LOGE(msg, __VA_ARGS__); \
    char customMsg[128]; \
    snprintf(customMsg, sizeof(customMsg), msg, __VA_ARGS__); \
    if (paintState->dialogCustomDetail) \
    { \
        free(paintState->dialogCustomDetail); \
    } \
    paintState->dialogCustomDetail = malloc(strlen(customMsg) + 1); \
    strcpy(paintState->dialogCustomDetail, customMsg); \
    paintState->dialogMessageDetail = paintState->dialogCustomDetail; \
    paintState->showDialogBox = true; \
    paintState->fatalError = true; \
    paintState->dialogBox->icon = &paintState->dialogErrorWsg; \
    paintState->dialogMessageTitle = dialogErrorTitleStr; \
    paintSetupDialog(DIALOG_ERROR); \
} while(0)

#define PAINT_WSG(fn, var) do \
{ \
    if (!loadWsg(fn, var, false)) \
    { \
        PAINT_DIE("%sLoading %s icon failed!!!", dialogErrorDetailStr, fn); \
        return; \
    } \
} while (0)

static void paintToolWheelCb(const char* label, bool selected, uint32_t settingVal);
static void paintSetupColorWheel(void);
static void paintSetupDialog(paintDialog_t dialog);
static void paintDialogCb(const char* label);
static void paintSetupBrowser(bool save);
static void paintBrowserCb(const char* nvsKey);

paintDraw_t* paintState;
paintHelp_t* paintHelp;

static const char paintDefaultFilename[] = "untitled_%02" PRIu8 ".mfp";

static const char toolWheelTitleStr[]   = "Tool Wheel";
static const char toolWheelBrushStr[]   = "Select Brush";
static const char toolWheelColorStr[]   = "Select Color";
static const char toolWheelSizeStr[]    = "Brush Size";
static const char toolWheelOptionsStr[] = "More";
static const char toolWheelUndoStr[]    = "Undo";
static const char toolWheelRedoStr[]    = "Redo";

static const char toolWheelSaveStr[]    = "Save";
static const char toolWheelLoadStr[]    = "Load";
static const char toolWheelNewStr[]     = "New";
static const char toolWheelPaletteStr[] = "Edit Colors";
static const char toolWheelExitStr[]    = "Quit Drawing";


static const char dialogUnsavedTitleStr[] = "Unsaved Changes!";
static const char dialogOverwriteTitleStr[] = "Overwrite Existing?";
static const char dialogErrorTitleStr[] = "Error!";

static const char dialogUnsavedDetailStr[] = "There are unsaved changes to the current drawing! Continue without saving?";
static const char dialogOverwriteDetailStr[] = "This file already exists! Overwrite it with the current drawing?";
static const char dialogErrorDetailStr[] = "Fatal Error! ";

static const char dialogOptionCancelStr[] = "Cancel";
static const char dialogOptionSaveStr[] = "Save";
static const char dialogOptionSaveAsStr[] = "Save as...";
static const char dialogOptionExitStr[] = "Quit";
static const char dialogOptionOkStr[] = "OK";

static paletteColor_t defaultPalette[] = {
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

brush_t brushes[] = {
    {.name      = "Square Pen",
     .mode      = HOLD_DRAW,
     .maxPoints = 1,
     .minSize   = 1,
     .maxSize   = 16,
     .fnDraw    = paintDrawSquarePen,
     .iconName  = "square_pen"},
    {.name      = "Circle Pen",
     .mode      = HOLD_DRAW,
     .maxPoints = 1,
     .minSize   = 1,
     .maxSize   = 16,
     .fnDraw    = paintDrawCirclePen,
     .iconName  = "circle_pen"},
    {.name      = "Line",
     .mode      = PICK_POINT,
     .maxPoints = 2,
     .minSize   = 1,
     .maxSize   = 8,
     .fnDraw    = paintDrawLine,
     .iconName  = "line"},
    {.name      = "Bezier Curve",
     .mode      = PICK_POINT,
     .maxPoints = 4,
     .minSize   = 1,
     .maxSize   = 8,
     .fnDraw    = paintDrawCurve,
     .iconName  = "curve"},
    {.name      = "Rectangle",
     .mode      = PICK_POINT,
     .maxPoints = 2,
     .minSize   = 1,
     .maxSize   = 8,
     .fnDraw    = paintDrawRectangle,
     .iconName  = "rect"},
    {.name      = "Filled Rectangle",
     .mode      = PICK_POINT,
     .maxPoints = 2,
     .minSize   = 0,
     .maxSize   = 0,
     .fnDraw    = paintDrawFilledRectangle,
     .iconName  = "rect_filled"},
    {.name      = "Circle",
     .mode      = PICK_POINT,
     .maxPoints = 2,
     .minSize   = 1,
     .maxSize   = 8,
     .fnDraw    = paintDrawCircle,
     .iconName  = "circle"},
    {.name      = "Filled Circle",
     .mode      = PICK_POINT,
     .maxPoints = 2,
     .minSize   = 0,
     .maxSize   = 0,
     .fnDraw    = paintDrawFilledCircle,
     .iconName  = "circle_filled"},
    {.name      = "Ellipse",
     .mode      = PICK_POINT,
     .maxPoints = 2,
     .minSize   = 1,
     .maxSize   = 8,
     .fnDraw    = paintDrawEllipse,
     .iconName  = "ellipse"},
    {.name      = "Polygon",
     .mode      = PICK_POINT_LOOP,
     .maxPoints = 16,
     .minSize   = 1,
     .maxSize   = 8,
     .fnDraw    = paintDrawPolygon,
     .iconName  = "polygon"},
    {.name      = "Squarewave",
     .mode      = PICK_POINT,
     .maxPoints = 2,
     .minSize   = 0,
     .maxSize   = 0,
     .fnDraw    = paintDrawSquareWave,
     .iconName  = "squarewave"},
    {.name      = "Paint Bucket",
     .mode      = PICK_POINT,
     .maxPoints = 1,
     .minSize   = 0,
     .maxSize   = 0,
     .fnDraw    = paintDrawPaintBucket,
     .iconName  = "paint_bucket"},
};

const char activeIconStr[]   = "%s_active.wsg";
const char inactiveIconStr[] = "%s_inactive.wsg";

const brush_t* firstBrush = brushes;
const brush_t* lastBrush  = brushes + sizeof(brushes) / sizeof(brushes[0]) - 1;

static void paintSetupColorWheel(void)
{
    for (uint8_t i = 0; i < PAINT_MAX_COLORS; ++i)
    {
        char* colorLabel = paintState->colorNames[i];
        snprintf(colorLabel, sizeof(paintState->colorNames[0]), "#%02X%02X%02X",
                 (paintState->canvas.palette[i] / 36) * 51, ((paintState->canvas.palette[i] / 6) % 6) * 51,
                 (paintState->canvas.palette[i] % 6) * 51);
        wheelMenuSetItemInfo(paintState->toolWheelRenderer, colorLabel, NULL, i, NO_SCROLL);
        wheelMenuSetItemColor(paintState->toolWheelRenderer, colorLabel, paintState->canvas.palette[i],
                              paintState->canvas.palette[i]);
    }
}

void paintDrawScreenSetup(void)
{
    PAINT_LOGD("Allocating %" PRIu32 " bytes for paintState", (uint32_t)sizeof(paintDraw_t));
    paintState = calloc(sizeof(paintDraw_t), 1);

    loadFont(PAINT_TOOLBAR_FONT, &(paintState->toolbarFont), false);
    loadFont(PAINT_SMALL_FONT, &(paintState->smallFont), false);
    paintState->clearScreen = true;
    paintState->blinkOn     = true;
    paintState->blinkTimer  = 0;

    // Load icons for the dialog box
    PAINT_WSG("error.wsg", &paintState->dialogErrorWsg);
    PAINT_WSG("info.wsg", &paintState->dialogInfoWsg);

    // Initialize with no icon to start, and default to error
    paintState->dialogBox = initDialogBox(dialogErrorTitleStr, dialogErrorDetailStr, &paintState->dialogErrorWsg, paintDialogCb);
    paintSetupDialog(DIALOG_ERROR);
    paintState->showDialogBox = false;
    paintState->fatalError = false;

    paintState->browser.callback = paintBrowserCb;
    paintState->browser.cols = 4;

    // Set up the brush icons
    uint16_t spriteH = 0;
    char iconName[32];
    for (brush_t* brush = brushes; brush <= lastBrush; brush++)
    {
        snprintf(iconName, sizeof(iconName), activeIconStr, brush->iconName);
        PAINT_WSG(iconName, &brush->iconActive);

        snprintf(iconName, sizeof(iconName), inactiveIconStr, brush->iconName);
        PAINT_WSG(iconName, &brush->iconInactive);

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

    PAINT_WSG("pointer.wsg", &paintState->picksWsg);
    PAINT_WSG("brush_size.wsg", &paintState->brushSizeWsg);

    PAINT_WSG("arrow9.wsg", &paintState->smallArrowWsg);
    colorReplaceWsg(&paintState->smallArrowWsg, c555, c000);

    PAINT_WSG("arrow12.wsg", &paintState->bigArrowWsg);
    colorReplaceWsg(&paintState->bigArrowWsg, c555, c000);

    PAINT_WSG("newfile.wsg", &paintState->newfileWsg);
    PAINT_WSG("overwrite.wsg", &paintState->overwriteWsg);

    paintState->marginTop = TFT_CORNER_RADIUS * 2 / 3;

    // Left: Leave room for the color boxes, their margins, their borders, and the canvas border
    paintState->marginLeft = TFT_CORNER_RADIUS * 2 / 3;
    // Bottom: Leave room for the brush name/icon/color boxes, 4px margin, and the canvas border
    paintState->marginBottom
        = MAX(brushes->iconActive.h + 1, MAX(paintState->toolbarFont.height + PAINT_COLORBOX_MARGIN_X + 1,
                                             PAINT_COLORBOX_H + PAINT_COLORBOX_H / 2 + 2 + PAINT_COLORBOX_MARGIN_X));
    // Right: We just need to stay away from the rounded corner, so like, 12px?
    paintState->marginRight = TFT_CORNER_RADIUS * 2 / 3;

    if (paintHelp != NULL)
    {
        // Set up some tutorial things that depend on basic paintState data
        paintTutorialPostSetup();

        // We're in help mode! We need some more space for the text
        paintState->marginBottom += paintHelp->helpH;
    }

    paintLoadIndex(&paintState->index);

    if (paintHelp == NULL && paintGetLastSlot(paintState->slotKey))
    {
        // If there's a saved image, load that (but not in the tutorial)
        paintState->doLoad = true;
        strncpy(paintState->selectedSlotKey, paintState->slotKey, sizeof(paintState->selectedSlotKey));
        PAINT_LOGI("Opening %s", paintState->selectedSlotKey);
    }
    else
    {
        for (uint8_t i = 0; i <= UINT8_MAX; ++i)
        {
            snprintf(paintState->slotKey, sizeof(paintState->slotKey), paintDefaultFilename, i);

            if (!paintSlotExists(paintState->slotKey))
            {
                PAINT_LOGI("Opening new file as %s", paintState->slotKey);
                break;
            }

            // Give up and make a totally random one
            if (i == UINT8_MAX)
            {
                for (uint8_t n = 0; n < sizeof(paintState->slotKey) - 5; ++n)
                {
                    uint8_t rng = esp_random() % 16;
                    paintState->slotKey[n] = (rng > 9) ? ('a' + rng - 10) : '0' + rng;
                }
                paintState->slotKey[sizeof(paintState->slotKey) - 5] = '\0';
                //memcpy(paintState->slotKey + sizeof(paintState->slotKey) - 5, ".mfp", 5);
                break;
            }
        }

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
        paintState->artist[i].brushDef   = firstBrush;
        paintState->artist[i].brushWidth = firstBrush->minSize;

        setCursorSprite(&paintState->artist[i].cursor, &paintState->canvas, &paintState->cursorWsg);
        setCursorOffset(&paintState->artist[i].cursor, (paintState->canvas.xScale - paintState->cursorWsg.w) / 2,
                        (paintState->canvas.yScale - paintState->cursorWsg.h) / 2);
        moveCursorAbsolute(getCursor(), &paintState->canvas, paintState->canvas.w / 2, paintState->canvas.h / 2);
    }

    clearPxTft();

    paintSetupTool();

    // Clear the LEDs
    // Might not be necessary here
    paintUpdateLeds();

    bzrStop(true);
    bzrPlayBgm(&paintBgm, BZR_LEFT);

    // Set up the tool wheel
    paintState->showToolWheel     = false;
    paintState->toolWheelWaiting  = false;
    paintState->toolWheel         = initMenu(toolWheelTitleStr, paintToolWheelCb);
    paintState->toolWheelRenderer = initWheelMenu(&paintState->toolbarFont, 90, &paintState->toolWheelLabelBox);

    paintState->toolWheelLabelBox.x      = TFT_CORNER_RADIUS;
    paintState->toolWheelLabelBox.y      = 4;
    paintState->toolWheelLabelBox.width  = TFT_WIDTH - TFT_CORNER_RADIUS * 2;
    paintState->toolWheelLabelBox.height = 20;

    // Tool wheel icons
    PAINT_WSG("wheel_brush.wsg", &paintState->wheelBrushWsg);
    PAINT_WSG("wheel_color.wsg", &paintState->wheelColorWsg);
    PAINT_WSG("wheel_size.wsg", &paintState->wheelSizeWsg);
    PAINT_WSG("wheel_options.wsg", &paintState->wheelSettingsWsg);
    PAINT_WSG("wheel_undo.wsg", &paintState->wheelUndoWsg);
    PAINT_WSG("wheel_redo.wsg", &paintState->wheelRedoWsg);
    PAINT_WSG("wheel_save.wsg", &paintState->wheelSaveWsg);
    PAINT_WSG("wheel_open.wsg", &paintState->wheelOpenWsg);
    PAINT_WSG("wheel_new.wsg", &paintState->wheelNewWsg);
    PAINT_WSG("wheel_exit.wsg", &paintState->wheelExitWsg);
    PAINT_WSG("wheel_palette.wsg", &paintState->wheelPaletteWsg);

    // Top: Sub-menu for Brush
    paintState->toolWheel = startSubMenu(paintState->toolWheel, toolWheelBrushStr);
    wheelMenuSetItemInfo(paintState->toolWheelRenderer, toolWheelBrushStr, &paintState->wheelBrushWsg, 0, SCROLL_HORIZ);

    for (const brush_t* brush = brushes; brush <= lastBrush; brush++)
    {
        addSingleItemToMenu(paintState->toolWheel, brush->name);
        wheelMenuSetItemInfo(paintState->toolWheelRenderer, brush->name, &brush->iconInactive, brush - brushes,
                             NO_SCROLL);
    }

    paintState->toolWheel = endSubMenu(paintState->toolWheel);

    // Left: Sub-menu for Color
    paintState->toolWheel = startSubMenu(paintState->toolWheel, toolWheelColorStr);
    wheelMenuSetItemInfo(paintState->toolWheelRenderer, toolWheelColorStr, &paintState->wheelColorWsg, 1,
                         SCROLL_VERT_R);

    // Add the actual menu items (only once)
    for (uint8_t i = 0; i < PAINT_MAX_COLORS; ++i)
    {
        addSingleItemToMenu(paintState->toolWheel, paintState->colorNames[i]);
    }

    // Set up the infos for the color wheel
    paintSetupColorWheel();

    paintState->toolWheel = endSubMenu(paintState->toolWheel);

    // Bottom-left: Undo
    addSingleItemToMenu(paintState->toolWheel, toolWheelUndoStr);
    wheelMenuSetItemInfo(paintState->toolWheelRenderer, toolWheelUndoStr, &paintState->wheelUndoWsg, 2, NO_SCROLL);

    // Bottom-right: Redo
    addSingleItemToMenu(paintState->toolWheel, toolWheelRedoStr);
    wheelMenuSetItemInfo(paintState->toolWheelRenderer, toolWheelRedoStr, &paintState->wheelRedoWsg, 4, NO_SCROLL);

    // Bottom: Options sub-menu
    paintState->toolWheel = startSubMenu(paintState->toolWheel, toolWheelOptionsStr);
    wheelMenuSetItemInfo(paintState->toolWheelRenderer, toolWheelOptionsStr, &paintState->wheelSettingsWsg, 3,
                         NO_SCROLL);

    // Options menu

    // Top: Load
    addSingleItemToMenu(paintState->toolWheel, toolWheelLoadStr);
    wheelMenuSetItemInfo(paintState->toolWheelRenderer, toolWheelLoadStr, &paintState->wheelOpenWsg, 0, NO_SCROLL);

    // Left: New
    addSingleItemToMenu(paintState->toolWheel, toolWheelNewStr);
    wheelMenuSetItemInfo(paintState->toolWheelRenderer, toolWheelNewStr, &paintState->wheelNewWsg, 1, NO_SCROLL);

    // Bottom-left: Exit/Quit
    addSingleItemToMenu(paintState->toolWheel, toolWheelExitStr);
    wheelMenuSetItemInfo(paintState->toolWheelRenderer, toolWheelExitStr, &paintState->wheelExitWsg, 2, NO_SCROLL);

    // Bottom-right: Edit palette
    paintState->toolWheel = startSubMenu(paintState->toolWheel, toolWheelPaletteStr);
    wheelMenuSetItemInfo(paintState->toolWheelRenderer, toolWheelPaletteStr, &paintState->wheelPaletteWsg, 3, NO_SCROLL);

    // Save this submenu so we can check if we're there or not
    paintState->editPaletteWheel = paintState->toolWheel;

    // Add the color menu items again, but this time underneath the edit palette option
    // The wheel options will apply alreday based on the labels (weird right?)
    for (uint8_t i = 0; i < PAINT_MAX_COLORS; ++i)
    {
        addSingleItemToMenu(paintState->toolWheel, paintState->colorNames[i]);
    }

    // End edit palette, Back to the Options menu
    paintState->toolWheel = endSubMenu(paintState->toolWheel);

    // Right: Save
    addSingleItemToMenu(paintState->toolWheel, toolWheelSaveStr);
    wheelMenuSetItemInfo(paintState->toolWheelRenderer, toolWheelSaveStr, &paintState->wheelSaveWsg, 4, NO_SCROLL);

    // Back to the top-level menu
    paintState->toolWheel = endSubMenu(paintState->toolWheel);


    // Right: Up/Down for Size
    settingParam_t sizeBounds = {
        .min = getArtist()->brushDef->minSize,
        .max = getArtist()->brushDef->maxSize,
        .def = getArtist()->brushWidth,
        .key = NULL,
    };
    addSettingsItemToMenu(paintState->toolWheel, toolWheelSizeStr, &sizeBounds, sizeBounds.def);
    paintState->toolWheelBrushSizeItem = paintState->toolWheel->items->last->val;
    wheelMenuSetItemInfo(paintState->toolWheelRenderer, toolWheelSizeStr, &paintState->wheelSizeWsg, 5, SCROLL_VERT);
}

void paintDrawScreenCleanup(void)
{
    bzrStop(true);

    deinitWheelMenu(paintState->toolWheelRenderer);
    deinitMenu(paintState->toolWheel);

    freeWsg(&paintState->wheelColorWsg);
    freeWsg(&paintState->wheelBrushWsg);
    freeWsg(&paintState->wheelSizeWsg);
    freeWsg(&paintState->wheelSettingsWsg);
    freeWsg(&paintState->wheelUndoWsg);
    freeWsg(&paintState->wheelRedoWsg);
    freeWsg(&paintState->wheelSaveWsg);
    freeWsg(&paintState->wheelOpenWsg);
    freeWsg(&paintState->wheelNewWsg);
    freeWsg(&paintState->wheelExitWsg);
    freeWsg(&paintState->wheelPaletteWsg);

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

    for (uint8_t i = 0; i < sizeof(paintState->artist) / sizeof(paintState->artist[0]); i++)
    {
        deinitCursor(&paintState->artist[i].cursor);
        freePxStack(&paintState->artist[i].pickPoints);
    }

    if (paintState->storedCanvas)
    {
        free(paintState->storedCanvas);
    }

    paintFreeCursorSprite(&paintState->cursorWsg);
    paintFreeUndos();

    resetImageBrowser(&paintState->browser);

    deinitDialogBox(paintState->dialogBox);
    if (paintState->dialogCustomDetail)
    {
        free(paintState->dialogCustomDetail);
        paintState->dialogMessageDetail = NULL;
        paintState->dialogCustomDetail = NULL;
    }

    freeWsg(&paintState->dialogErrorWsg);
    freeWsg(&paintState->dialogInfoWsg);

    freeFont(&paintState->smallFont);
    freeFont(&paintState->toolbarFont);
    free(paintState);
}

void paintTutorialSetup(void)
{
    paintHelp          = calloc(sizeof(paintHelp_t), 1);
    paintHelp->curHelp = helpSteps;
}

// gets called after paintState is allocated and has basic info, but before canvas layout is done
void paintTutorialPostSetup(void)
{
    paintHelp->helpH = PAINT_HELP_TEXT_LINES * (paintState->toolbarFont.height + 1) - 1;
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
            paintHelp->allButtons     = 0;
            paintHelp->lastButton     = 0;
            paintHelp->lastButtonDown = false;
            paintHelp->drawComplete   = false;
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

        paintHelp->allButtons     = 0;
        paintHelp->lastButton     = 0;
        paintHelp->lastButtonDown = false;
        paintHelp->drawComplete   = false;
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
            return paintHelp->lastButtonDown == false
                   && (paintHelp->lastButton & trigger->data) == paintHelp->lastButton;

        case DRAW_COMPLETE:
            return paintHelp->drawComplete;

        case CHANGE_BRUSH:
            return !strcmp(getArtist()->brushDef->name, trigger->dataPtr) && paintHelp->curButtons == 0;

        case CHANGE_MODE:
            return paintState->buttonMode == trigger->data;

        case BRUSH_NOT:
            return strcmp(getArtist()->brushDef->name, trigger->dataPtr) && paintHelp->curButtons == 0;

        case SELECT_MENU_ITEM:
            //return paintState->saveMenu == trigger->data;

        case MENU_ITEM_NOT:
            //return paintState->saveMenu != trigger->data;

        case MODE_NOT:
            //return paintState->buttonMode != trigger->data;

        case NO_TRIGGER:
        default:
            break;
    }

    return false;
}

void paintPositionDrawCanvas(void)
{
    // Calculate the highest scale that will fit on the screen
    uint8_t scale
        = paintGetMaxScale(paintState->canvas.w, paintState->canvas.h, paintState->marginLeft + paintState->marginRight,
                           paintState->marginTop + paintState->marginBottom);

    paintState->canvas.xScale = scale;
    paintState->canvas.yScale = scale;

    paintState->canvas.x = paintState->marginLeft
                           + (TFT_WIDTH - paintState->marginLeft - paintState->marginRight
                              - paintState->canvas.w * paintState->canvas.xScale)
                                 / 2;
    paintState->canvas.y = paintState->marginTop
                           + (TFT_HEIGHT - paintState->marginTop - paintState->marginBottom
                              - paintState->canvas.h * paintState->canvas.yScale)
                                 / 2;

    PAINT_LOGI("Scaled image to %" PRIu8, scale);
}

void paintDrawScreenMainLoop(int64_t elapsedUs)
{
    if (paintState->fatalError)
    {
        clearPxTft();
        if (paintState->showDialogBox)
        {
            drawDialogBox(paintState->dialogBox, &paintState->toolbarFont, &paintState->toolbarFont, DIALOG_CENTER, DIALOG_CENTER, DIALOG_AUTO, DIALOG_AUTO, 6);
        }
        return;
    }

    if (!paintState->showDialogBox && !paintState->showBrowser)
    {
        paintDrawScreenPollTouch();
    }

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
        paintState->unsaved     = false;
        paintState->clearScreen = false;
    }

    // Save and Load
    if (*paintState->selectedSlotKey && (paintState->doSave || paintState->doLoad))
    {
        paintExitSelectMode();

        if (paintState->doSave)
        {
            hideCursor(getCursor(), &paintState->canvas);
            paintHidePickPoints();

            paintSaveNamed(paintState->selectedSlotKey, &paintState->canvas);
            //paintSave(&paintState->index, &paintState->canvas, paintState->selectedSlot);

            paintDrawPickPoints();
            showCursor(getCursor(), &paintState->canvas);
        }
        else
        {
            if (paintSlotExists(paintState->selectedSlotKey))
            {
                // Load from the selected slot if it's been used
                hideCursor(getCursor(), &paintState->canvas);
                paintClearCanvas(&paintState->canvas, getArtist()->bgColor);
                if (paintLoadDimensionsNamed(paintState->selectedSlotKey, &paintState->canvas))
                {
                    paintPositionDrawCanvas();
                    paintLoadNamed(paintState->selectedSlotKey, &paintState->canvas);
                    paintSetLastSlot(paintState->selectedSlotKey);
                    strncpy(paintState->slotKey, paintState->selectedSlotKey, sizeof(paintState->slotKey));

                    paintFreeUndos();

                    getArtist()->fgColor = paintState->canvas.palette[0];
                    getArtist()->bgColor = paintState->canvas.palette[1];

                    // Do the tool setup, which will also setup the cursor
                    paintSetupTool();

                    // Put the cursor in the middle of the screen
                    moveCursorAbsolute(getCursor(), &paintState->canvas, paintState->canvas.w / 2,
                                       paintState->canvas.h / 2);
                    showCursor(getCursor(), &paintState->canvas);
                    paintUpdateLeds();
                }
                else
                {
                    PAINT_LOGE("Slot %s has 0 dimension! Stopping load and clearing slot",
                               paintState->selectedSlotKey);
                    //paintClearSlot(&paintState->index, paintState->selectedSlot);
                    paintReturnToMainMenu();
                }
            }
            else
            {
                // If the slot hasn't been used yet, just clear the screen
                paintState->clearScreen = true;
            }
        }

        paintState->unsaved        = false;
        paintState->doSave         = false;
        paintState->doLoad         = false;

        paintState->buttonMode = BTN_MODE_DRAW;

        paintState->redrawToolbar = true;
    }

    if (paintState->recolorPickPoints)
    {
        paintDrawPickPoints();
        paintUpdateLeds();
        paintState->recolorPickPoints = false;
    }

    // TODO render toolbar always
    // paintRenderToolbar(getArtist(), &paintState->canvas, paintState, firstBrush, lastBrush);

    if (paintState->showBrowser || paintState->showDialogBox || wheelMenuActive(paintState->toolWheel, paintState->toolWheelRenderer))
    {
        paintEnterSelectMode();

        paintClearCanvas(&paintState->canvas, PAINT_TOOLBAR_BG);

        if (paintState->showBrowser)
        {
            drawImageBrowser(&paintState->browser, &paintState->toolbarFont, false);
        }
        else
        {
            paintRenderToolbar(getArtist(), &paintState->canvas, paintState, firstBrush, lastBrush);

            if (paintState->showDialogBox)
            {
                //drawDialogBox(paintState->dialogBox, &paintState->toolbarFont, &paintState->toolbarFont, 30, 30, TFT_WIDTH - 60, TFT_HEIGHT - 60, 5);
                drawDialogBox(paintState->dialogBox, &paintState->toolbarFont, &paintState->toolbarFont, DIALOG_CENTER, DIALOG_CENTER, DIALOG_AUTO, DIALOG_AUTO, 6);
            }
            else
            {
                drawWheelMenu(paintState->toolWheel, paintState->toolWheelRenderer, elapsedUs);
            }
        }
    }
    else
    {
        paintExitSelectMode();
        paintRenderToolbar(getArtist(), &paintState->canvas, paintState, firstBrush, lastBrush);
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
            }
            else if (paintState->blinkOn)
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
    }

    if (paintHelp != NULL)
    {
        int16_t pastColorBoxX = PAINT_COLORBOX_MARGIN_X
                                + (paintState->canvas.x - 1 - PAINT_COLORBOX_W - PAINT_COLORBOX_MARGIN_X * 2 - 2) / 2
                                + PAINT_COLORBOX_W + 2 + 6;
        int16_t wrapY = paintState->canvas.y + paintState->canvas.h * paintState->canvas.yScale + 3;
        const char* rest
            = drawTextWordWrap(&paintState->toolbarFont, c000, paintHelp->curHelp->prompt, &pastColorBoxX, &wrapY,
                               TFT_WIDTH, TFT_HEIGHT - paintState->marginBottom + paintHelp->helpH);
        if (rest)
        {
            PAINT_LOGW("Some tutorial text didn't fit: %s", rest);
        }
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
    paletteColor_t col         = paintState->canvas.palette[paintState->paletteSelect];
    paintState->editPaletteCur = &paintState->editPaletteR;
    paintState->editPaletteR   = col / 36;
    paintState->editPaletteG   = (col / 6) % 6;
    paintState->editPaletteB   = col % 6;
    paintState->newColor       = col;
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
    paletteColor_t old                                    = paintState->canvas.palette[paintState->paletteSelect];
    paletteColor_t new                                    = paintState->newColor;
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
                    paintState->buttonMode    = BTN_MODE_DRAW;
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
            paintState->buttonMode    = BTN_MODE_DRAW;
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

void paintDrawScreenPollTouch(void)
{
    int32_t centroid, intensity;
    int32_t phi, r, y;

    if (getTouchJoystick(&phi, &r, &intensity))
    {
        getTouchCartesian(phi, r, &centroid, &y);

        paintState->toolWheel = wheelMenuTouch(paintState->toolWheel, paintState->toolWheelRenderer, phi, r);
        return;

        /////////////////////////////// old code below, probs delete

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
                    paintState->touchDown  = true;
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
                        paintHelp->curButtons
                            = TOUCH_ANY
                              | (paintHelp->curButtons
                                 & (PB_UP | PB_DOWN | PB_LEFT | PB_RIGHT | PB_A | PB_B | PB_START | PB_SELECT));
                        paintHelp->lastButton     = TOUCH_ANY;
                        paintHelp->lastButtonDown = true;
                        paintTutorialOnEvent();
                    }
                }
                else
                {
                    // We're mid-swipe
                    int32_t swipeMagnitude = ((paintState->firstTouch - centroid) * PAINT_MAX_BRUSH_SWIPE) / 1024;
                    int32_t newWidth       = paintState->startBrushWidth - swipeMagnitude;

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
                // PAINT_LOGD("Centroid: %d, Intensity: %d, Index: %d", centroid, intensity, index);
                paintEditPaletteSetChannelValue(index);
                break;
            }
        }
    }
    else
    {
        paintState->toolWheel = wheelMenuTouchRelease(paintState->toolWheel, paintState->toolWheelRenderer);
    }
}

void paintDrawScreenButtonCb(const buttonEvt_t* evt)
{
    if (paintState->fatalError)
    {
        if (paintState->showDialogBox)
        {
            dialogBoxButton(paintState->dialogBox, evt);
        }
        return;
    }

    if (paintHelp != NULL)
    {
        paintHelp->allButtons |= evt->state;
        paintHelp->lastButton     = evt->button;
        paintHelp->lastButtonDown = evt->down;
        // Keep the touch buttons in place but replace everything else with the button state
        paintHelp->curButtons
            = evt->state | (paintHelp->curButtons & (TOUCH_ANY | TOUCH_X | TOUCH_Y | SWIPE_LEFT | SWIPE_RIGHT));
    }

    if (paintState->showBrowser)
    {
        if (evt->down && evt->button == PB_B)
        {
            paintState->showBrowser = false;
        }
        else
        {
            imageBrowserButton(&paintState->browser, evt);
        }
    }
    else if (paintState->showDialogBox)
    {
        dialogBoxButton(paintState->dialogBox, evt);
    }
    else if (wheelMenuActive(paintState->toolWheel, paintState->toolWheelRenderer))
    {
        PAINT_LOGI("Menu is active, sending it a button press");
        wheelMenuButton(paintState->toolWheel, paintState->toolWheelRenderer, evt);
    }
    else
    {
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

            case BTN_MODE_PALETTE:
            {
                paintPaletteModeButtonCb(evt);
                break;
            }
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
                // Don't do anything until start is released to avoid conflicting with EXIT
                break;

            case PB_A:
            {
                // Draw
                paintState->aHeld  = true;
                paintState->aPress = true;
                break;
            }

            case PB_B:
            {
                // Swap the foreground and background colors
                paintSwapFgBgColors();

                paintState->redrawToolbar     = true;
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
            {
                // We're gonna enter the quick-save menu... save the canvas!
                paintSaveCanvas(&paintState->canvas);
                break;
            }
        }
    }
    else
    {
        //////// Draw mode button release
        switch (evt->button)
        {
            case PB_SELECT:
            {
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

            case PB_START:
                // Re-draw the canvas in case the quick-settings menu just closed
                paintRestoreCanvas(&paintState->canvas);
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
        PAINT_LOGD("Deleted %" PRIu8 " dangling undos after changing history", deleted);
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
        undoData     = undoMem;
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

bool paintCanUndo(void)
{
    // We can undo as long as one of these is true:
    //  - The undoHead is NULL and undoList.last is NOT NULL
    //  - The undoHead is NOT NULL and undoHead->prev is NOT NULL
    return (paintState->undoHead == NULL && paintState->undoList.last != NULL)
           || (paintState->undoHead != NULL && paintState->undoHead->prev != NULL);
}

bool paintCanRedo(void)
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

    PAINT_LOGD("Undid %" PRIu32 " bytes!", (uint32_t)pxSize);

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

bool paintSaveCanvas(paintCanvas_t* canvas)
{
    // Allocate a new paintUndo_t to store the canvas
    paintUndo_t* undoData = paintState->storedCanvas;

    // Calculate the amount of space we wolud need to store the canvas pixels
    size_t pxSize = paintGetStoredSize(canvas);

    if (!undoData)
    {
        // Allocate memory for the undo data struct and its pixel data in one go
        void* undoMem = heap_caps_malloc(sizeof(paintUndo_t) + pxSize, MALLOC_CAP_SPIRAM);
        if (undoMem != NULL)
        {
            // Alloc succeeded, use the data
            undoData     = undoMem;
            undoData->px = (uint8_t*)undoMem + sizeof(paintUndo_t);
        }
        else
        {
            // Alloc failed, reuse the first undo data
            undoData = shift(&paintState->undoList);
        }
    }

    if (!undoData)
    {
        PAINT_LOGD("Failed to allocate or reuse undo data! Can't save canvas");
        // There's no data at all! We're completely out of space!
        return false;
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

    paintState->storedCanvas = undoData;

    return true;
}

void paintRestoreCanvas(paintCanvas_t* canvas)
{
    if (NULL == paintState->storedCanvas)
    {
        return;
    }

    // Don't restore palette
    hideCursor(getCursor(), canvas);

    size_t pxSize = paintGetStoredSize(canvas);
    paintDeserialize(canvas, paintState->storedCanvas->px, 0, pxSize);
    memcpy(paintState->canvas.palette, paintState->storedCanvas->palette, sizeof(paletteColor_t) * PAINT_MAX_COLORS);

    // feels weird to do this inside the undo functions... but it's probably ok? we've already undone anyway
    showCursor(getCursor(), canvas);
}

void paintDoTool(uint16_t x, uint16_t y, paletteColor_t col)
{
    hideCursor(getCursor(), &paintState->canvas);
    bool drawNow    = false;
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

    pushPxScaled(&getArtist()->pickPoints, getCursor()->x, getCursor()->y, paintState->canvas.x, paintState->canvas.y,
                 paintState->canvas.xScale, paintState->canvas.yScale);

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
            if (pxStackSize(&getArtist()->pickPoints) > 1 && getPx(&getArtist()->pickPoints, 0, &firstPick)
                && peekPx(&getArtist()->pickPoints, &lastPick) && firstPick.x == lastPick.x
                && firstPick.y == lastPick.y)
            {
                // If this isn't the first pick, and it's in the same position as the first pick, we're done!
                drawNow = true;
            }
            else if (isLastPick)
            {
                // Special case: If we're on the next-to-last possible point, we have to add the start again as the last
                // point
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

        while (popPxScaled(&getArtist()->pickPoints, paintState->canvas.xScale, paintState->canvas.yScale))
            ;

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
        paintState->blinkOn    = false;
    }

    showCursor(getCursor(), &paintState->canvas);
    paintRenderToolbar(getArtist(), &paintState->canvas, paintState, firstBrush, lastBrush);
}

void paintSetupTool(void)
{
    // Reset the brush params
    if (getArtist()->brushWidth < getArtist()->brushDef->minSize)
    {
        getArtist()->brushWidth     = getArtist()->brushDef->minSize;
        paintState->startBrushWidth = getArtist()->brushWidth;
    }
    else if (getArtist()->brushWidth > getArtist()->brushDef->maxSize)
    {
        getArtist()->brushWidth     = getArtist()->brushDef->maxSize;
        paintState->startBrushWidth = getArtist()->brushWidth;
    }

    hideCursor(getCursor(), &paintState->canvas);
    paintHidePickPoints();
    switch (getArtist()->brushDef->mode)
    {
        case HOLD_DRAW:
        {
            // Regenerate the cursor if it's not been set yet or if the brush's size is different from the cursor's size
            if (paintState->cursorWsg.px == NULL
                || paintState->cursorWsg.w != (getArtist()->brushWidth * paintState->canvas.xScale + 2)
                || paintState->cursorWsg.h != (getArtist()->brushWidth * paintState->canvas.yScale + 2))
            {
                paintFreeCursorSprite(&paintState->cursorWsg);
                paintGenerateCursorSprite(&paintState->cursorWsg, &paintState->canvas, getArtist()->brushWidth);
            }

            setCursorSprite(getCursor(), &paintState->canvas, &paintState->cursorWsg);

            // Center the cursor, accounting for even and odd cursor sizes
            setCursorOffset(
                getCursor(),
                -((paintState->cursorWsg.w) / 2) + getArtist()->brushWidth % 2 * paintState->canvas.xScale / 2,
                -((paintState->cursorWsg.h) / 2) + getArtist()->brushWidth % 2 * paintState->canvas.yScale / 2
            );
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
    while (popPxScaled(&getArtist()->pickPoints, paintState->canvas.xScale, paintState->canvas.yScale))
        ;
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
    if (paintState->buttonMode == BTN_MODE_SELECT && paintState->showToolWheel)
    {
        return;
    }

    if (!paintState->showToolWheel)
    {
        paintSaveCanvas(&paintState->canvas);
        paintState->showToolWheel = true;
    }

    paintState->buttonMode    = BTN_MODE_SELECT;
    paintState->redrawToolbar = true;
    paintState->aHeld         = false;
    paintState->moveX         = 0;
    paintState->moveY         = 0;
    paintState->btnHoldTime   = 0;
    paintSetupColorWheel();
}

void paintExitSelectMode(void)
{
    if (paintState->showBrowser)
    {
        paintState->showBrowser = false;
    }

    if (paintState->showDialogBox)
    {
        paintState->showDialogBox = false;
    }

    if (paintState->buttonMode == BTN_MODE_DRAW && !paintState->showToolWheel)
    {
        return;
    }

    // Exit select mode
    paintState->buttonMode = BTN_MODE_DRAW;

    if (paintState->showToolWheel)
    {
        paintRestoreCanvas(&paintState->canvas);
        paintState->showToolWheel = false;
    }

    // Set the current selection as the FG color and rearrange the rest
    paintUpdateRecents(paintState->paletteSelect);
    paintState->paletteSelect = 0;

    paintState->redrawToolbar = true;
}

void paintUpdateRecents(uint8_t selectedIndex)
{
    getArtist()->fgColor = paintState->canvas.palette[selectedIndex];

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

    for (uint8_t i = 0; i < CONFIG_NUM_LEDS; i++)
    {
        paintState->leds[i].b = (rgb >> 0) & 0xFF;
        paintState->leds[i].g = (rgb >> 8) & 0xFF;
        paintState->leds[i].r = (rgb >> 16) & 0xFF;
    }

    setLeds(paintState->leds, CONFIG_NUM_LEDS);
}

void paintDrawPickPoints(void)
{
    pxVal_t point;
    for (size_t i = 0; i < pxStackSize(&getArtist()->pickPoints); i++)
    {
        if (getPx(&getArtist()->pickPoints, i, &point))
        {
            bool invert = (i == 0 && getArtist()->brushDef->mode == PICK_POINT_LOOP)
                          && pxStackSize(&getArtist()->pickPoints) > 1;
            drawRectFilled(point.x, point.y, point.x + paintState->canvas.xScale + 1,
                           point.y + paintState->canvas.yScale + 1,
                           invert ? getContrastingColor(point.col) : getArtist()->fgColor);
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
            drawRectFilled(point.x, point.y, point.x + paintState->canvas.xScale + 1,
                           point.y + paintState->canvas.yScale + 1, point.col);
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

static void paintToolWheelCb(const char* label, bool selected, uint32_t settingVal)
{
    if (NULL == label)
    {
        return;
    }

    if (selected)
    {
        PAINT_LOGI("Selected tool wheel item %s", label);
        if (toolWheelUndoStr == label)
        {
            paintExitSelectMode();
            if (paintCanUndo())
            {
                paintUndo(&paintState->canvas);
            }
        }
        else if (toolWheelRedoStr == label)
        {
            paintExitSelectMode();
            if (paintCanRedo())
            {
                paintRedo(&paintState->canvas);
            }
        }
        else if (toolWheelSaveStr == label)
        {
            if (paintSlotExists(paintState->slotKey))
            {
                paintSetupDialog(DIALOG_CONFIRM_OVERWRITE);
            }
            else
            {
                strncpy(paintState->selectedSlotKey, paintState->slotKey, sizeof(paintState->selectedSlotKey));
                paintState->doSave = true;
                paintExitSelectMode();
            }
        }
        else if (toolWheelLoadStr == label)
        {
            if (paintState->unsaved)
            {
                paintSetupDialog(DIALOG_CONFIRM_UNSAVED_LOAD);
            }
            else
            {
                paintSetupBrowser(false);
            }
        }
        else if (toolWheelNewStr == label)
        {
            if (paintState->unsaved)
            {
                paintSetupDialog(DIALOG_CONFIRM_UNSAVED_CLEAR);
            }
            else
            {
                paintExitSelectMode();
                paintStoreUndo(&paintState->canvas);
                paintState->clearScreen = true;
            }
        }
        else if (toolWheelExitStr == label)
        {
            if (paintState->unsaved)
            {
                paintSetupDialog(DIALOG_CONFIRM_UNSAVED_EXIT);
            }
            else
            {
                paintReturnToMainMenu();
            }
        }
        // Check if the label is one of the color name strings
        else if (NULL != label && paintState->colorNames[0] <= label
                 && label <= paintState->colorNames[PAINT_MAX_COLORS - 1])
        {
            uint8_t colorIndex        = (label - *paintState->colorNames) / sizeof(*paintState->colorNames);

            // Set palette color
            paintState->paletteSelect = colorIndex;

            if (paintState->toolWheel == paintState->editPaletteWheel)
            {
                paintEditPaletteSetupColor();
            }
        }
    }
    else
    {
        if (toolWheelBrushStr == label)
        {
            paintEnterSelectMode();

            getArtist()->brushDef = (firstBrush + settingVal);
            paintSetupTool();
            paintState->redrawToolbar = true;
        }
        else if (toolWheelColorStr == label)
        {
            paintEnterSelectMode();
            // Select previous color
            paintState->redrawToolbar = true;
            paintState->paletteSelect = settingVal;
            paintUpdateLeds();
        }
        else if (toolWheelSizeStr == label)
        {
            paintEnterSelectMode();
            paintSetBrushWidth(settingVal);
        }
        else if (paintState->colorNames[0] <= label && label <= paintState->colorNames[PAINT_MAX_COLORS - 1])
        {
            paintEnterSelectMode();
            // color, do something?
            uint8_t colorIndex        = (label - *paintState->colorNames) / sizeof(*paintState->colorNames);
            paintState->paletteSelect = colorIndex;
        }
        else
        {
            // Check for all the brush names
            for (const brush_t* brush = firstBrush; brush <= lastBrush; brush++)
            {
                if (brush->name == label)
                {
                    paintEnterSelectMode();
                    getArtist()->brushDef = brush;
                    paintSetupTool();
                    paintState->redrawToolbar = true;
                    return;
                }
            }

            // Something else:
        }
        PAINT_LOGI("Moved to tool wheel item %s", label);
    }
}

static void paintSetupDialog(paintDialog_t dialog)
{
    paintState->showDialogBox = true;
    if (dialog != DIALOG_ERROR && paintState->dialog == dialog)
    {
        return;
    }

    paintState->dialog = dialog;

    const char* title;
    const char* detail;
    const wsg_t* icon = NULL;

    // Clear any previous options
    dialogBoxReset(paintState->dialogBox);

    switch (dialog)
    {
        case DIALOG_CONFIRM_UNSAVED_CLEAR:
        case DIALOG_CONFIRM_UNSAVED_LOAD:
        case DIALOG_CONFIRM_UNSAVED_EXIT:
        {
            title = dialogUnsavedTitleStr;
            detail = dialogUnsavedDetailStr;

            // Unsaved! Continue? Cancel, Save, Save as... OK
            dialogBoxAddOption(paintState->dialogBox, dialogOptionCancelStr, NULL, OPTHINT_CANCEL | OPTHINT_DEFAULT);
            dialogBoxAddOption(paintState->dialogBox, dialogOptionSaveStr, NULL, OPTHINT_NORMAL);
            dialogBoxAddOption(paintState->dialogBox, dialogOptionSaveAsStr, NULL, OPTHINT_NORMAL);
            dialogBoxAddOption(paintState->dialogBox, dialogOptionOkStr, NULL, OPTHINT_OK);
            break;
        }

        case DIALOG_CONFIRM_OVERWRITE:
        {
            title = dialogOverwriteTitleStr;
            detail = dialogOverwriteDetailStr;

            // Already exists! Overwrite? Cancel, Save As... Ok
            dialogBoxAddOption(paintState->dialogBox, dialogOptionCancelStr, NULL, OPTHINT_CANCEL | OPTHINT_DEFAULT);
            dialogBoxAddOption(paintState->dialogBox, dialogOptionSaveAsStr, NULL, OPTHINT_NORMAL);
            dialogBoxAddOption(paintState->dialogBox, dialogOptionOkStr, NULL, OPTHINT_OK);
            break;
        }

        case DIALOG_ERROR:
        default:
        {
            // Error! Exit
            title = paintState->dialogMessageTitle ? paintState->dialogMessageTitle : dialogErrorTitleStr;
            detail = paintState->dialogMessageDetail ? paintState->dialogMessageDetail : dialogErrorDetailStr;
            icon = &paintState->dialogErrorWsg;

            dialogBoxAddOption(paintState->dialogBox, dialogOptionExitStr, NULL, OPTHINT_OK | OPTHINT_DEFAULT);
            break;
        }

        case DIALOG_MESSAGE:
        {
            title = paintState->dialogMessageTitle;
            detail = paintState->dialogMessageDetail;
        }
    }

    // Actually set the new title/detail/icon
    paintState->dialogBox->title = title;
    paintState->dialogBox->detail = detail;
    paintState->dialogBox->icon = icon;
}

static void paintDialogCb(const char* label)
{
    PAINT_LOGI("Dialog option %s chosen!", label);
    if (dialogOptionCancelStr == label)
    {
        // Cancel, so just go back to where we were
        // TODO- Should this exit select mode or just go back to the tool wheel?
        paintExitSelectMode();
    }
    else if (dialogOptionSaveStr == label)
    {
        // TODO switch to named slots
        if (paintSlotExists(paintState->slotKey))
        {
            paintSetupDialog(DIALOG_CONFIRM_OVERWRITE);
        }
        else
        {
            // Do actual save
            paintState->doSave = true;
        }
    }
    else if (dialogOptionSaveAsStr == label)
    {
        paintSetupBrowser(true);
    }
    else if (dialogOptionExitStr == label)
    {
        paintReturnToMainMenu();
    }
    else if (dialogOptionOkStr == label)
    {
        switch (paintState->dialog)
        {
            case DIALOG_CONFIRM_UNSAVED_CLEAR:
            {
                paintExitSelectMode();
                paintStoreUndo(&paintState->canvas);
                paintState->clearScreen = true;
                break;
            }

            case DIALOG_CONFIRM_UNSAVED_LOAD:
            {
                paintSetupBrowser(false);
                break;
            }

            case DIALOG_CONFIRM_UNSAVED_EXIT:
            {
                paintReturnToMainMenu();
                break;
            }

            case DIALOG_CONFIRM_OVERWRITE:
            {
                paintState->doSave = true;
                strncpy(paintState->selectedSlotKey, paintState->slotKey, sizeof(paintState->selectedSlotKey));
                paintExitSelectMode();
                break;
            }

            case DIALOG_MESSAGE:
            case DIALOG_ERROR:
            {
                paintExitSelectMode();
                break;
            }
        }
    }
}

static void paintSetupBrowser(bool save)
{
    static bool done = false;

    if (!done)
    {
        saveWsgNvs("paint_img", "color.mfp", &paintState->wheelColorWsg);
        writeNamespaceNvsBlob("paint_pal", "color.mfp", paintState->canvas.palette, sizeof(paintState->canvas.palette));

        //PAINT_LOGI("res: %s", res ? "true" : "false");
        saveWsgNvs("paint_img", "settings.mfp", &paintState->wheelSettingsWsg);
        writeNamespaceNvsBlob("paint_pal", "settings.mfp", paintState->canvas.palette, sizeof(paintState->canvas.palette));

        saveWsgNvs("paint_img", "arrow.mfp", &paintState->bigArrowWsg);
        writeNamespaceNvsBlob("paint_pal", "arrow.mfp", paintState->canvas.palette, sizeof(paintState->canvas.palette));
        done = true;
    }

    resetImageBrowser(&paintState->browser);
    setupImageBrowser(&paintState->browser, "paint_img", NULL);
    paintState->showDialogBox = false;
    paintState->showBrowser = true;
    paintState->browserSave = save;
}

static void paintBrowserCb(const char* nvsKey)
{
    PAINT_LOGI("Selected %s", nvsKey);
    strncpy(paintState->selectedSlotKey, nvsKey, sizeof(paintState->selectedSlotKey) - 1);
    PAINT_LOGI("selectedSlotKey is %s", paintState->selectedSlotKey);

    if (paintState->browserSave)
    {
        paintState->doSave = true;
    }
    else
    {
        paintState->doLoad = true;
    }
}
