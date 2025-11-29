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
} sudokuMoveDesc_t;

bool sudokuNextMove(sudokuMoveDesc_t* desc, sudokuOverlay_t* overlay, const sudokuGrid_t* board);
