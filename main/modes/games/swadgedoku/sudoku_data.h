#pragma once

//==============================================================================
// Includes
//==============================================================================

#include "swadge2024.h"

//==============================================================================
// Defines
//==============================================================================

// There are exactly two unique binary sudoku games... And we have both!
#define SUDOKU_MIN_BASE 2
#define SUDOKU_MAX_BASE 16

/// @brief Value for when a square does not map to any box, such as a void square
#define BOX_NONE UINT8_MAX

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    /// @brief Your classic everyday sudoku with a square grid and boxes
    SM_CLASSIC = 0,
    /// @brief Same rules as classic, but with irregularly shaped boxes
    SM_JIGSAW,
    /// @brief Five games of sudoku in an X, with each corner game sharing a box with the central game
    SM_X_GRID,
} sudokuMode_t;

typedef enum
{
    //< This square has no flags set
    SF_NONE = 0,
    //< Square is locked and unchangeable as part of the puzzle
    SF_LOCKED = 1,
    //< Square is not considered part of the playable area, for irregular puzzles
    SF_VOID = 2,
} sudokuFlag_t;

typedef enum
{
    /// @brief No overlays at all
    OVERLAY_NONE = 0,
    /// @brief Highlight this square with the row color
    OVERLAY_HIGHLIGHT_ROW = 1,
    /// @brief Highlight this square with the column color
    OVERLAY_HIGHLIGHT_COL = 2,
    /// @brief Highlight this square with the box color
    OVERLAY_HIGHLIGHT_BOX = 4,
    /// @brief Highlight this square with color A
    OVERLAY_HIGHLIGHT_A = 8,
    /// @brief Highlight this square with color B
    OVERLAY_HIGHLIGHT_B = 16,
    /// @brief Highlight this square with color C
    OVERLAY_HIGHLIGHT_C = 32,
    /// @brief Highlight this square with color D
    OVERLAY_HIGHLIGHT_D = 64,
    /// @brief Mark this square as having an error
    OVERLAY_ERROR = 128,
    /// @brief Cross out this square as 'not possible'
    OVERLAY_CROSS_OUT = 256,
    /// @brief Mark this square as a 'maybe' or 'unknown'
    OVERLAY_QUESTION = 512,
    /// @brief Mark this square as
    OVERLAY_CHECK = 1024,
    /// @brief Overlays the computed notes onto the square
    OVERLAY_NOTES = 2048,
    /// @brief Prevents the digit or notes from being drawn
    OVERLAY_SKIP = 4096,
    /// @brief Mask for all overlay options
    OVERLAY_ALL = 0xFFFF,
} sudokuOverlayOpt_t;

typedef enum
{
    /// @brief This shape is a cursor and shouldn't be removed
    ST_CURSOR = 1,
    /// @brief This is a temporary annotation added by sudokuAnnotate()
    ST_ANNOTATE = 2,
    /// @brief This is a temporary annotation added for a hint
    ST_HINT = 4,
    /// @brief Mask for all shape tags
    ST_ALL = 0xFF,
} sudokuShapeTag_t;

typedef enum
{
    OVERLAY_RECT,
    OVERLAY_CIRCLE,
    OVERLAY_LINE,
    OVERLAY_ARROW,
    OVERLAY_TEXT,
    OVERLAY_DIGIT,
    OVERLAY_NOTES_SHAPE,
} sudokuOverlayShapeType_t;

typedef enum
{
    SSB_WRITE_ON_SELECT         = 1,
    SSB_AUTO_ANNOTATE           = 2,
    SSB_HIGHLIGHT_POSSIBILITIES = 4,
    SSB_HIGHLIGHT_ONLY_OPTIONS  = 8,
} sudokuSettingBit_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    sudokuShapeTag_t tag;
    paletteColor_t color;
    sudokuOverlayShapeType_t type;

    union
    {
        rectangle_t rectangle;
        circle_t circle;
        line_t line;
        arrow_t arrow;

        struct
        {
            vec_t pos;
            const char* val;
            bool center;
        } text;

        struct
        {
            vec_t pos;
            uint8_t digit;
        } digit;

        struct
        {
            vec_t pos;
            uint16_t notes;
        } notes;
    };
} sudokuOverlayShape_t;

/// @brief Holds information for drawing all sorts of overlay data
typedef struct
{
    /// @brief A bitmask of options for each grid in the game
    sudokuOverlayOpt_t* gridOpts;

    /// @brief A list of dynamic shapes to draw on top of the grid
    list_t shapes;
} sudokuOverlay_t;

typedef struct
{
    //< Cursor position
    uint8_t curX, curY;

    /// @brief Digit selected to be entered
    uint8_t selectedDigit;

    /// @brief Whether the player is taking notes or setting digits
    bool noteTaking;

    /// @brief A grid-sized array of notes for each cell
    uint16_t* notes;

    /// @brief A set of overlay data to show this player
    sudokuOverlay_t overlay;

    /// @brief If the player is playing, this shape refers to the cursor
    sudokuOverlayShape_t* cursorShape;

    /// @brief Some sort of score for the player, TBD how it's calculated
    int32_t score;
} sudokuPlayer_t;

typedef struct
{
    /// @brief The Sudoku rules used for this board
    sudokuMode_t mode;

    /// @brief The size of the grid. Could be larger
    uint8_t size;

    /// @brief The number of digits per line/box, and the number of boxes.
    /// Can be anywhere from 2 to 16, though who knows if I'll do non-square bases with irregular boxes (like killer
    /// sudoku?)
    uint8_t base;

    /// @brief The box configuration for this grid
    // sudokuBox_t* boxes;

    /// @brief A grid-sized array of actual filled numbers; 0 used for empty squares
    uint8_t* grid;

    /// @brief A grid-sized array of the 'notes' layer
    /// Each element is a bitmask representing which numbers are still valid for that square
    uint16_t* notes;

    /// @brief A grid-sized array mapping each square into a particular box index.
    /// Elements with value BOX_NONE are not part of any box.
    uint8_t* boxMap;

    /// @brief A grid-sized array of the 'flags' layer which has extra options for squares
    /// For example, it marks which squares are unchangeable as part of the puzzle
    sudokuFlag_t* flags;
} sudokuGrid_t;

typedef struct
{
    /// @brief Write the selected digit to the cursor position as soon as it is picked
    bool writeOnSelect;

    /// @brief Automatically fill out all annotations for the player on the grid
    bool autoAnnotate;

    /// @brief Highlights all places where the selected digit can go
    bool highlightPossibilities;

    /// @brief Highlights places where the selected digit is the only possibility
    bool highlightOnlyOptions;
} sudokuSettings_t;

//==============================================================================
// Function Declarations
//==============================================================================

bool loadSudokuData(const uint8_t* data, size_t length, sudokuGrid_t* game);
size_t writeSudokuData(uint8_t* data, const sudokuGrid_t* game);
size_t getSudokuSaveSize(const sudokuGrid_t* game, int* boxFmt, int* gridFmt, int* noteFmt, int* flagFmt);

void swadgedokuLoadSettings(sudokuSettings_t* settings);
void swadgedokuSaveSettings(const sudokuSettings_t* settings);