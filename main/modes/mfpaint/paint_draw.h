#ifndef _PAINT_DRAW_H_
#define _PAINT_DRAW_H_

#include "palette.h"
#include "swadge2024.h"

#include "paint_common.h"
#include "paint_brush.h"
#include "paint_help.h"

extern paintDraw_t* paintState;
extern const char* dialogErrorTitleStr;

// Mostly for PAINT_DIE
void paintSetupDialog(paintDialog_t dialog);

// Mode callback delegates
void paintDrawScreenMainLoop(int64_t elapsedUs);
void paintDrawScreenButtonCb(const buttonEvt_t* evt);
void paintDrawScreenPollTouch(void);
void paintPaletteModeButtonCb(const buttonEvt_t* evt);
void paintSaveModeButtonCb(const buttonEvt_t* evt);
void paintSelectModeButtonCb(const buttonEvt_t* evt);
void paintDrawModeButtonCb(const buttonEvt_t* evt);

// Palette mode helpers
void paintEditPaletteUpdate(void);
void paintEditPaletteSetChannelValue(uint8_t val);
void paintEditPaletteDecChannel(void);
void paintEditPaletteIncChannel(void);
void paintEditPaletteNextChannel(void);

void paintEditPalettePrevChannel(void);
void paintEditPaletteSetupColor(uint8_t index);
void paintEditPalettePrevColor(void);
void paintEditPaletteNextColor(void);
void paintEditPaletteUpdateCanvas(void);
void paintEditPaletteConfirm(void);

// Save menu helpers
void paintSaveModePrevItem(void);
void paintSaveModeNextItem(void);
void paintSaveModePrevOption(void);
void paintSaveModeNextOption(void);

// Setup/cleanup functions
void paintDrawScreenSetup(void);
void paintDrawScreenCleanup(void);
void paintTutorialSetup(void);
void paintTutorialPostSetup(void);
void paintTutorialCleanup(void);
void paintTutorialOnEvent(void);
bool paintTutorialCheckTrigger(const paintHelpTrigger_t* trigger);

void paintPositionDrawCanvas(void);

void paintHandleDpad(uint16_t state);
void paintFreeUndos(void);
void paintStoreUndo(paintCanvas_t* canvas, paletteColor_t fg, paletteColor_t bg);
bool paintMaybeSacrificeUndoForHeap(void);
bool paintCanUndo(void);
bool paintCanRedo(void);
void paintApplyUndo(paintCanvas_t* canvas);
void paintUndo(paintCanvas_t* canvas);
void paintRedo(paintCanvas_t* canvas);
void paintDoTool(uint16_t x, uint16_t y, paletteColor_t col, bool partial);
void paintSwapFgBgColors(void);
void paintResetButtons(void);
void paintUpdateRecents(uint8_t selectedIndex);
void paintUpdateLeds(void);

// Brush Helper Functions
void paintSetupTool(void);
void paintPrevTool(void);
void paintNextTool(void);
void paintSetBrushWidth(uint8_t width);
void paintDecBrushWidth(uint8_t dec);
void paintIncBrushWidth(uint8_t inc);

// Artist helpers
paintArtist_t* getArtist(void);
paintCursor_t* getCursor(void);

#endif
