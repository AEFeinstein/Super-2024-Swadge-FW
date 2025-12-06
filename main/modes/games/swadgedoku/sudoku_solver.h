#pragma once

#include <stdint.h>
#include "sudoku_data.h"

#define MAX_ELIMINATION_STEPS 16

typedef enum
{
    REGION_NONE,
    REGION_BOX,
    REGION_ROW,
    REGION_COLUMN,
} sudokuRegionType_t;

typedef enum
{
    SINGLE, // only empty square left in row/col/box
    ONLY_POSSIBLE, // only possibility in the square
    NAKED_PAIR, // eliminate possibilities
    HIDDEN_PAIR,
    NAKED_TRIPLE,
    HIDDEN_TRIPLE,
    X_WING,
    XY_WING,
    GUESS,
} sudokuTechniqueType_t;

typedef struct
{
} sudokuMoveHighlight_t;

typedef struct
{
    /// @brief The number of elimination squares
    uint8_t eliminationCount;

    /// @brief The digits that were eliminated
    uint16_t eliminationMask;

    /// @brief The positions that were eliminated
    uint8_t eliminations[3];

    /// @brief Whether the elimination is by row, column, or box
    sudokuRegionType_t eliminationRegionType;

    /// @brief The row, column, or box the elimination occurs in
    uint8_t eliminationRegionNum;
} sudokuMoveElimination_t;

typedef struct
{
    /// @brief The grid index to place the digit in
    uint16_t pos;

    /// @brief The digit to place
    uint8_t digit;

    /// @brief A text description of the hint accompanying this move
    char* message;

    /// @brief A detailed text description of the technique used in this move
    char* detail;

    uint8_t stepCount;
    sudokuMoveElimination_t eliminationSteps[MAX_ELIMINATION_STEPS];
} sudokuMoveDesc_t;

bool sudokuNextMove(sudokuMoveDesc_t* desc, sudokuOverlay_t* overlay, const sudokuGrid_t* board);
void sudokuApplyMove(sudokuGrid_t* board, const sudokuMoveDesc_t* desc);
