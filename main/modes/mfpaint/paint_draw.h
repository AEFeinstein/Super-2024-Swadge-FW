#ifndef _PAINT_DRAW_H_
#define _PAINT_DRAW_H_

#include "palette.h"
#include "swadge_util.h"
#include "swadgeMode.h"

#include "paint_common.h"
#include "paint_brush.h"
#include "paint_help.h"

extern paintDraw_t* paintState;

// Mode callback delegates
void paintDrawScreenMainLoop(int64_t elapsedUs);
void paintDrawScreenButtonCb(const buttonEvt_t* evt);
void paintDrawScreenPollTouch(void);
void paintDrawScreenTouchCb(const touch_event_t* evt);
void paintPaletteModeButtonCb(const buttonEvt_t* evt);
void paintSaveModeButtonCb(const buttonEvt_t* evt);
void paintSelectModeButtonCb(const buttonEvt_t* evt);
void paintDrawModeButtonCb(const buttonEvt_t* evt);

// Palette mode helpers
void paintEditPaletteUpdate(void);
void paintEditPaletteSetChannelValue(uint8_t val);
// void paintEditPaletteDecChannel(void);
void paintEditPaletteIncChannel(void);
void paintEditPaletteNextChannel(void);

void paintEditPalettePrevChannel(void);
void paintEditPaletteSetupColor(void);
void paintEditPalettePrevColor(void);
void paintEditPaletteNextColor(void);
void paintEditPaletteConfirm(void);


// Save menu helpers
void paintSaveModePrevItem(void);
void paintSaveModeNextItem(void);
void paintSaveModePrevOption(void);
void paintSaveModeNextOption(void);

// Setup/cleanup functions
void paintDrawScreenSetup(display_t* disp);
void paintDrawScreenCleanup(void);
void paintTutorialSetup(display_t* disp);
void paintTutorialPostSetup(void);
void paintTutorialCleanup(void);
void paintTutorialOnEvent(void);
bool paintTutorialCheckTrigger(const paintHelpTrigger_t* trigger);

void paintPositionDrawCanvas(void);

void paintHandleDpad(uint16_t state);
void paintFreeUndos(void);
void paintStoreUndo(paintCanvas_t* canvas);
bool paintMaybeSacrificeUndoForHeap(void);
bool paintCanUndo(void);
bool paintCanRedo(void);
void paintApplyUndo(paintCanvas_t* canvas);
void paintUndo(paintCanvas_t* canvas);
void paintRedo(paintCanvas_t* canvas);
void paintDoTool(uint16_t x, uint16_t y, paletteColor_t col);
void paintSwapFgBgColors(void);
void paintEnterSelectMode(void);
void paintExitSelectMode(void);
void paintUpdateRecents(uint8_t selectedIndex);
void paintUpdateLeds(void);
void paintDrawPickPoints(void);
void paintHidePickPoints(void);

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
