#pragma once

#include "sudoku_data.h"

typedef struct
{
} sudokuMoveHighlight_t;

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
} sudokuMoveDesc_t;

bool sudokuNextMove(sudokuMoveDesc_t* desc, sudokuOverlay_t* overlay, const sudokuGrid_t* board);
