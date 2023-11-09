#ifndef _PAINT_TYPE_H_
#define _PAINT_TYPE_H_

#include <stdint.h>

#include "palette.h"

// The number of colors in the palette and the max number of colors an image can be saved with
#define PAINT_MAX_COLORS 16

typedef paletteColor_t (*colorMapFn_t)(paletteColor_t col);

/// @brief Defines each separate screen in the paint mode.
typedef enum
{
    // Top menu
    PAINT_MENU,
    // Control instructions
    PAINT_HELP,
    // Drawing mode
    PAINT_DRAW,
    // Select and view/edit saved drawings
    PAINT_GALLERY,
    // Share a drawing via ESPNOW
    PAINT_SHARE,
    // Receive a shared drawing over ESPNOW
    PAINT_RECEIVE,
} paintScreen_t;

/// @brief Represents the coordinates of a single pixel or point
typedef struct
{
    uint16_t x, y;
} point_t;

typedef enum
{
    BTN_MODE_DRAW,
    BTN_MODE_SELECT,
    BTN_MODE_PALETTE,
} paintButtonMode_t;

typedef enum
{
    PALETTE_R,
    PALETTE_G,
    PALETTE_B,
} paintEditPalette_t;

typedef enum
{
    HIDDEN,
    UNDO,
    REDO,
    PICK_SLOT_SAVE,
    PICK_SLOT_LOAD,
    CONFIRM_OVERWRITE,
    CONFIRM_UNSAVED,
    EDIT_PALETTE,
    COLOR_PICKER,
    CLEAR,
    CONFIRM_CLEAR,
    EXIT,
    CONFIRM_EXIT,
} paintSaveMenu_t;

// For tracking the state of the sharing / receiving process
typedef enum
{
    /////// Sender States

    // Sender is selecting slot to be shared (initial sender state)
    SHARE_SEND_SELECT_SLOT,

    // Sender is waiting for connection
    SHARE_SEND_WAIT_FOR_CONN,

    // Sender is sending canvas metadata
    SHARE_SEND_CANVAS_DATA,

    // Sender sent canvas data and is waiting for ack from receiver
    SHARE_SEND_WAIT_CANVAS_DATA_ACK,

    // Wait for the receiver to finish processing pixel data and request some more
    SHARE_SEND_WAIT_FOR_PIXEL_REQUEST,

    // Sender got canvas data ack, is now sending pixel data packets
    SHARE_SEND_PIXEL_DATA,

    // Sender sent pixel data, is waiting for ack
    SHARE_SEND_WAIT_PIXEL_DATA_ACK,

    // All done!
    SHARE_SEND_COMPLETE,

    //////// Receiver States

    // Receiver is waiting for connection (initial receiver state)
    SHARE_RECV_WAIT_FOR_CONN,

    // Receiver is waiting for canvas metadata
    SHARE_RECV_WAIT_CANVAS_DATA,

    // Receiver is receiving pixel data
    SHARE_RECV_PIXEL_DATA,

    // Receiver has all pixel data and must pick save slot
    SHARE_RECV_SELECT_SLOT,
} paintShareState_t;

/// @brief Definition for a paintable screen region
typedef struct
{
    // The X and Y offset of the canvas's top-left pixel
    uint16_t x, y;

    // The canvas's width and height, in "canvas pixels"
    uint16_t w, h;

    // The X and Y scale of the canvas. Each "canvas pixel" will be drawn as [xScale x yScale]
    uint8_t xScale, yScale;

    paletteColor_t palette[PAINT_MAX_COLORS];

    // Whether this canvas is backed off-screen in a buffer
    bool buffered;
    // The buffer, if this canvas has one
    uint8_t* buffer;

} paintCanvas_t;

typedef struct
{
    paletteColor_t palette[PAINT_MAX_COLORS];
    uint8_t fgIdx, bgIdx;
    uint8_t* px;
} paintUndo_t;

#endif
