#pragma once

//==============================================================================
// Includes
//==============================================================================

#include "sudoku_data.h"

//==============================================================================
// Defines
//==============================================================================

// SUBPOS_* defines used for positioning overlays within the sudoku board
#define BOX_SIZE_SUBPOS 13

/*
 * @brief Computes a subposition value for the given x and y, in 13ths-of-a-square
 *
 */
#define SUBPOS_XY(x, y) (((y) * BOX_SIZE_SUBPOS) + (x))

#define GRID_MARGIN 1

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    SUBPOS_CENTER = (6 * 13 + 6),
    SUBPOS_NW     = (0),
    SUBPOS_NNW    = (3),
    SUBPOS_N      = (6),
    SUBPOS_NNE    = (9),
    SUBPOS_NE     = (12),
    SUBPOS_WNW    = (3 * 16),
    SUBPOS_W      = (6 * 16),
    SUBPOS_WSW    = (9 * 16),
    SUBPOS_SW     = (12 * 13),
    SUBPOS_SSW    = (12 * 13 + 3),
    SUBPOS_S      = (12 * 13 + 6),
    SUBPOS_SSE    = (12 * 13 + 9),
    SUBPOS_SE     = (12 * 13 + 12),
    SUBPOS_ENE    = (3 * 13 * 12),
    SUBPOS_E      = (6 * 13 + 12),
    SUBPOS_ESE    = (9 * 13 + 12),
} sudokuSubpos_t;

//==============================================================================
// Structs
//==============================================================================

/// @brief Holds data needed to draw the UI
typedef struct
{
    /// @brief Large font for filled-out numbers
    font_t gridFont;
    /// @brief Tiny font for the notes grid
    font_t noteFont;
    /// @brief Font for UI text
    font_t uiFont;

    /// @brief Icon used for showing input is in note taking mode
    wsg_t noteTakingIcon;
} sudokuDrawContext_t;

/// @brief Holds all the colors for the display theme
typedef struct
{
    /// @brief The background color outside of the grid and behind the UI
    paletteColor_t bgColor;

    /// @brief The fill color of the individual squares without any overlays
    paletteColor_t fillColor;

    /// @brief The fill color of any non-playable squares (for irregular grids)
    paletteColor_t voidColor;

    /// @brief The line color of the border around the entire grid
    paletteColor_t borderColor;

    /// @brief The line color of the lines separating rows and columns
    paletteColor_t gridColor;

    /// @brief The line color of the additional border separating boxes
    paletteColor_t boxBorderColor;

    /// @brief The text color of pre-filled numbers and notes
    paletteColor_t inkColor;

    /// @brief The text color of user-filled numbers
    paletteColor_t pencilColor;

    /// @brief The text color for the UI, not the grid
    paletteColor_t uiTextColor;

    /// @brief The color of the player's cursor
    paletteColor_t cursorColor;

    /// @brief The shape of the player's cursor
    sudokuOverlayShapeType_t cursorShape;
} sudokuTheme_t;

//==============================================================================
// Function Declarations
//==============================================================================

int swadgedokuGetSquareSize(const sudokuGrid_t* game);
void swadgedokuGetGridPos(int* gridX, int* gridY, const sudokuGrid_t* game);
void swadgedokuDrawGame(const sudokuGrid_t* game, const uint16_t* notes, const sudokuOverlay_t* overlay,
                        const sudokuTheme_t* theme, const sudokuDrawContext_t* context,
                        sudokuShapeTag_t tagMask, sudokuOverlayOpt_t overlayMask);
void getOverlayPos(int32_t* x, int32_t* y, int r, int c, sudokuSubpos_t subpos);
void getRealOverlayPos(int16_t* x, int16_t* y, int gridX, int gridY, int squareSize, int xSubPos, int ySubPos);
void addCrosshairOverlay(sudokuOverlay_t* overlay, int r, int c, int gridSize, bool drawH, bool drawV,
                         sudokuShapeTag_t tag);
