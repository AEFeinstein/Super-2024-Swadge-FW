#pragma once

//==============================================================================
// Includes
//==============================================================================

#include "sudoku_data.h"

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    SUDOKU_INVALID    = -1,
    SUDOKU_INCOMPLETE = 0,
    SUDOKU_COMPLETE   = 1,
} sudokuWinState_t;

//==============================================================================
// Function Declarations
//==============================================================================

bool initSudokuGame(sudokuGrid_t* game, int size, int base, sudokuMode_t mode);
void deinitSudokuGame(sudokuGrid_t* game);
bool setupSudokuGame(sudokuGrid_t* game, sudokuMode_t mode, int base, int size);
void setupSudokuPlayer(sudokuPlayer_t* player, const sudokuGrid_t* game);
void sudokuReevaluatePeers(uint16_t* notes, const sudokuGrid_t* game, int row, int col, int flags);
void sudokuGetNotes(uint16_t* notes, const sudokuGrid_t* game, int flags);
void sudokuAnnotate(sudokuOverlay_t* overlay, const sudokuPlayer_t* player, const sudokuGrid_t* game);
sudokuWinState_t swadgedokuCheckWin(const sudokuGrid_t* game);

int swadgedokuRand(int* seed);
int boxGetAdjacentSquares(uint16_t* indices, const sudokuGrid_t* game, uint8_t* aXs, uint8_t* aYs, uint8_t* bXs,
                          uint8_t* bYs);
bool squaresTouch(uint8_t ax, uint8_t ay, uint8_t bx, uint8_t by);

bool setDigit(sudokuGrid_t* game, uint8_t number, uint8_t x, uint8_t y);
void clearOverlayOpts(sudokuOverlay_t* overlay, const sudokuGrid_t* game, sudokuOverlayOpt_t optMask);
