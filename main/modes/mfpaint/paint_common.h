#ifndef _PAINT_COMMON_H_
#define _PAINT_COMMON_H_

#include <stddef.h>

#include "esp_log.h"
#include "hdw-tft.h"
#include "swadge2024.h"
#include "menu.h"
#include "wheel_menu.h"
#include "dialogBox.h"
#include "paint_browser.h"
#include "p2pConnection.h"
#include "linked_list.h"
#include "geometry.h"
#include "touchUtils.h"

#include "px_stack.h"
#include "paint_type.h"
#include "paint_brush.h"

#define PAINT_LOGV(...) ESP_LOGV("Paint", __VA_ARGS__)
#define PAINT_LOGD(...) ESP_LOGD("Paint", __VA_ARGS__)
#define PAINT_LOGI(...) ESP_LOGI("Paint", __VA_ARGS__)
#define PAINT_LOGW(...) ESP_LOGW("Paint", __VA_ARGS__)
#define PAINT_LOGE(...) ESP_LOGE("Paint", __VA_ARGS__)

#define PAINT_DIE(msg, ...)                                               \
    do                                                                    \
    {                                                                     \
        PAINT_LOGE(msg, __VA_ARGS__);                                     \
        char customMsg[128];                                              \
        snprintf(customMsg, sizeof(customMsg), msg, __VA_ARGS__);         \
        if (paintState->dialogCustomDetail)                               \
        {                                                                 \
            free(paintState->dialogCustomDetail);                         \
        }                                                                 \
        paintState->dialogCustomDetail = malloc(strlen(customMsg) + 1);   \
        strcpy(paintState->dialogCustomDetail, customMsg);                \
        paintState->dialogMessageDetail = paintState->dialogCustomDetail; \
        paintState->showDialogBox       = true;                           \
        paintState->fatalError          = true;                           \
        paintState->dialogBox->icon     = &paintState->dialogErrorWsg;    \
        paintState->dialogMessageTitle  = dialogErrorTitleStr;            \
        paintSetupDialog(DIALOG_ERROR);                                   \
    } while (0)

//////// Data Constants

#define PAINT_SHARE_PX_PACKET_LEN (P2P_MAX_DATA_LEN - 3 - 11)
#define PAINT_SHARE_PX_PER_PACKET PAINT_SHARE_PX_PACKET_LEN * 2

//////// Draw Screen Layout Constants and Colors

#define PAINT_DEFAULT_CANVAS_WIDTH  70
#define PAINT_DEFAULT_CANVAS_HEIGHT 60

// Keep at least 3px free above and below the toolbar text
#define PAINT_TOOLBAR_TEXT_PADDING_Y 3

#define PAINT_TOOLBAR_FONT "ibm_vga8.font"
// #define PAINT_SHARE_TOOLBAR_FONT "radiostars.font"
#define PAINT_SHARE_TOOLBAR_FONT "ibm_vga8.font"
// #define PAINT_SAVE_MENU_FONT "radiostars.font"
#define PAINT_SAVE_MENU_FONT "ibm_vga8.font"
// #define PAINT_SMALL_FONT "tom_thumb.font"
#define PAINT_SMALL_FONT "ibm_vga8.font"

#define PAINT_TOOLBAR_BG c444

// Dimensions of the color boxes in the palette
#define PAINT_COLORBOX_W 9
#define PAINT_COLORBOX_H 9

// Spacing between the tool icons and the size, and the size and pick point counts
#define TOOL_INFO_TEXT_MARGIN_Y 6

#define PAINT_COLORBOX_SHADOW_TOP    c000
#define PAINT_COLORBOX_SHADOW_BOTTOM c111

// The screen's corner radius in pixels
#define TFT_CORNER_RADIUS 40

// Vertical margin between each color box
#define PAINT_COLORBOX_MARGIN_TOP 2
// Minimum margin to the left and right of each color box
#define PAINT_COLORBOX_MARGIN_X 4

// X and Y position of the active color boxes (foreground/background color)
#define PAINT_ACTIVE_COLOR_X (TFT_CORNER_RADIUS / 2 + PAINT_COLORBOX_MARGIN_X)
#define PAINT_ACTIVE_COLOR_Y (TFT_HEIGHT - PAINT_COLORBOX_H * 2 - PAINT_COLORBOX_MARGIN_TOP)

// Color picker stuff
#define PAINT_COLOR_PICKER_MIN_BAR_H 6
#define PAINT_COLOR_PICKER_BAR_W     6

//////// Help layout stuff

// Number of lines of text to make room for below the canvas
#define PAINT_HELP_TEXT_LINES 4

//////// Macros

// Calculates previous and next items with wraparound
#define PREV_WRAP(i, count) ((i) == 0 ? (count)-1 : (i - 1))
#define NEXT_WRAP(i, count) ((i + 1) % count)

//////// Various Constants

#define PAINT_MAX_BRUSH_SWIPE 16

// hold button for .3s to begin repeating
#define BUTTON_REPEAT_TIME 300000

// 10 seconds to go to gallery screensaver
#define PAINT_SCREENSAVER_TIMEOUT 10000000

#define BLINK_TIME_ON  500000
#define BLINK_TIME_OFF 200000

/// @brief Struct encapsulating a cursor on the screen
typedef struct
{
    /// @brief The sprite for drawing the cursor
    const wsg_t* sprite;

    /// @brief The position of the top-left corner of the sprite, relative to the cursor position
    int8_t spriteOffsetX, spriteOffsetY;

    /// @brief The canvas X and Y coordinates of the cursor
    int16_t x, y;
} paintCursor_t;

/// @brief Struct encapsulating all info for a single player
typedef struct
{
    /// @brief Pointer to the player's selected brush definition
    const brush_t* brushDef;

    /// @brief The brush width or variant, depending on the brush definition
    uint8_t brushWidth;

    /// @brief A stack containing the points for the current pending draw action
    pxStack_t pickPoints;

    /// @brief The player's cursor information
    paintCursor_t cursor;

    /// @brief The player's selected foreground and background colors
    paletteColor_t fgColor, bgColor;
} paintArtist_t;

typedef enum
{
    DIALOG_CONFIRM_UNSAVED_CLEAR,
    DIALOG_CONFIRM_UNSAVED_LOAD,
    DIALOG_CONFIRM_UNSAVED_EXIT,
    DIALOG_CONFIRM_OVERWRITE,
    DIALOG_CONFIRM_DELETE,
    DIALOG_ERROR,
    DIALOG_ERROR_LOAD,
    DIALOG_ERROR_NONFATAL,
    DIALOG_MESSAGE,
} paintDialog_t;

typedef struct
{
    led_t leds[CONFIG_NUM_LEDS];

    paintCanvas_t canvas;

    // Margins that define the space the canvas may be placed within.
    uint16_t marginTop, marginLeft, marginBottom, marginRight;

    // Font for drawing tool info (width, pick points)
    font_t toolbarFont;
    // Font for drawing save / load / clear / exit menu
    font_t saveMenuFont;
    // Small font for small things (text above color picker gradient bars)
    font_t smallFont;

    // All shared state for 1 or 2 players
    paintArtist_t artist[2];

    // The generated cursor sprite
    wsg_t cursorWsg;

    // The "brush size" indicator sprite
    wsg_t brushSizeWsg;

    // The "picks remaining" sprite
    wsg_t picksWsg;

    // The 9x9 arrow
    wsg_t smallArrowWsg;

    // The 12x12 arrow
    wsg_t bigArrowWsg;

    // Icon to indicate free slot
    wsg_t newfileWsg;

    // Icon to indicate used slot
    wsg_t overwriteWsg;

    //////// Local-only UI state

    // Which mode will be used to interpret button presses
    paintButtonMode_t buttonMode;

    // Whether or not A is currently held
    bool aHeld;

    // flag so that an a press shorter than 1 frame always gets handled
    bool aPress;

    // When true, this is the initial D-pad button down.
    // If set, the cursor will move by one pixel and then it will be cleared.
    // The cursor will not move again until a D-pad button has been held for BUTTON_REPEAT_TIME microseconds
    bool firstMove;

    // So we don't miss a button press that happens between frames
    uint16_t unhandledButtons;

    // The time a D-pad button has been held down for, in microseconds
    int64_t btnHoldTime;

    // The number of canvas pixels to move the cursor this frame
    int8_t moveX, moveY;

    // The index of the currently selected color, while SELECT is held or in EDIT_PALETTE mode
    uint8_t paletteSelect;

    // Pointer to the selected color channel to edit (R, G, or B)
    uint8_t* editPaletteCur;

    // The separate values for the color channels
    uint8_t editPaletteR, editPaletteG, editPaletteB;

    // The color selected
    paletteColor_t newColor;

    // Used for timing blinks
    int64_t blinkTimer;
    bool blinkOn;

    // The brush width
    uint8_t startBrushWidth;

    //////// Save data flags

    // True if the canvas has been modified since last save
    bool unsaved;

    // The name of the currently opened slot
    char slotKey[17];

    //// Save Menu Flags

    // The save/load slot selected from the picker
    char selectedSlotKey[17];

    //////// Rendering flags

    // If set, the canvas will be cleared and the screen will be redrawn. Set on startup.
    bool clearScreen;

    //////// Undo Data

    // The linked list of undo data
    list_t undoList;

    // After an undo is performed, this points to the action that was undone.
    // This allows redo to work. If the image is edited, this and all following items are removed.
    node_t* undoHead;

    // The menu for the tool wheel
    menu_t* toolWheel;

    // The sub-menu for the edit palette
    menu_t* editPaletteWheel;

    // So we can update the brush size item options easily
    menuItem_t* toolWheelBrushSizeItem;

    // A box to center the selected tool wheel item label at
    rectangle_t toolWheelLabelBox;

    // So we can update the color item options easily
    menuItem_t* toolWheelColorItem;

    // Labels for each color, so there's something to display on the tool wheel
    // 8 chars is enough for #00AABB\0
    char colorNames[PAINT_MAX_COLORS][9];

    // The renderer for the tool wheel menu
    wheelMenuRenderer_t* toolWheelRenderer;

    //// Icons for various tool wheel things

    wsg_t wheelSizeWsg;
    wsg_t wheelBrushWsg;
    wsg_t wheelColorWsg;
    wsg_t wheelSettingsWsg;
    wsg_t wheelUndoWsg;
    wsg_t wheelRedoWsg;
    wsg_t wheelUndoGrayWsg;
    wsg_t wheelRedoGrayWsg;
    wsg_t wheelSaveWsg;
    wsg_t wheelOpenWsg;
    wsg_t wheelNewWsg;
    wsg_t wheelExitWsg;
    wsg_t wheelPaletteWsg;

    //////// Dialog Box

    /// @brief Icon to use in the dialog box for an error message
    wsg_t dialogErrorWsg;

    /// @brief Icon to use in the dialog box for an informational message
    wsg_t dialogInfoWsg;

    /// @brief Whether or not to show a dialog
    bool showDialogBox;

    /// @brief Which dialog to show
    paintDialog_t dialog;

    /// @brief The actual dialog
    dialogBox_t* dialogBox;

    /// @brief The title of the dialog for `DIALOG_MESSAGE`
    const char* dialogMessageTitle;

    /// @brief The detail message of the dialog for `DIALOG_MESSAGE`
    const char* dialogMessageDetail;

    /// @brief If dialogMessageDetail should be freed once done, it will be stored here
    char* dialogCustomDetail;

    /// @brief True if a fatal error occurred, and we should only show the error dialog.
    bool fatalError;

    //////// File Browser
    bool showBrowser;

    bool browserSave;

    imageBrowser_t browser;

    bool exiting;
} paintDraw_t;

typedef struct
{
    paintCanvas_t canvas;

    font_t toolbarFont;
    wsg_t arrowWsg;

    // The save slot being displayed / shared
    uint8_t shareSaveSlot;
    char shareSaveSlotKey[16];

    paintShareState_t shareState;

    imageBrowser_t browser;
    bool browserVisible;

    dialogBox_t* dialog;
    size_t dataOffset;

    bool shareAcked;
    bool connectionStarted;

    // The version of the protocol to use
    // 0 is the original
    // 1 is the next one, only difference is packet length and
    uint8_t version;
    bool versionSent;

    // For the sender, the sequence number of the current packet being sent / waiting for ack
    uint16_t shareSeqNum;

    uint8_t sharePacket[P2P_MAX_DATA_LEN];
    uint8_t sharePacketLen;

    uint8_t sharePaletteMap[256];

    // Set to true when a new packet has been written to sharePacket, either to be sent or to be handled
    bool shareNewPacket;

    // Flag for updating the screen
    bool shareUpdateScreen;

    // TODO rename this so it's not the same as the global one
    bool useCable;
    p2pInfo p2pInfo;

    // Time for the progress bar timer
    int64_t shareTime;
    int64_t timeSincePacket;

    // True if we are the sender, false if not
    bool isSender;

    // True if the screen should be cleared on the next loop
    bool clearScreen;
} paintShare_t;

typedef enum
{
    GALLERY_INFO_SPEED      = 0x01,
    GALLERY_INFO_BRIGHTNESS = 0x02,
    GALLERY_INFO_DANCE      = 0x04,
    GALLERY_INFO_NEXT       = 0x08,
    GALLERY_INFO_EXIT       = 0x10,
    GALLERY_INFO_VIEW       = 0x20,
    GALLERY_INFO_CONTROLS   = 0xFF,
} paintGalleryInfoView_t;

typedef struct
{
    font_t infoFont;
    wsg_t arrow;
    wsg_t aWsg;
    wsg_t bWsg;
    wsg_t pauseWsg;
    wsg_t spinWsg;

    // TODO rename these to better things now that they're in their own struct

    // Last timestamp of gallery transition
    int64_t galleryTime;

    // Amount of time between each transition, or 0 for disabled
    int64_t gallerySpeed;
    int32_t gallerySpeedIndex;

    // portableDance_t* portableDances;

    // Reaining time that info text will be shown
    int64_t infoTimeRemaining;

    // Current image used in gallery
    char gallerySlotKey[16];

    imageBrowser_t browser;

    bool showUi;
    bool screensaverMode;
    paintScreen_t returnScreen;
    paintGalleryInfoView_t infoView;

    int startBrightness;
    touchSpinState_t spinState;

    uint8_t galleryScale;
} paintGallery_t;

typedef struct
{
    //////// General app data

    // Main Menu Font
    font_t menuFont;
    // Main Menu
    menu_t* menu;
    menuLogbookRenderer_t* menuRenderer;

    uint8_t eraseSlot;

    bool eraseDataSelected, eraseDataConfirm;

    int64_t idleTimer;
    bool enableScreensaver;

    // The screen within paint that the user is in
    paintScreen_t screen;
} paintMainMenu_t;

extern paintMainMenu_t* paintMenu;

#endif
