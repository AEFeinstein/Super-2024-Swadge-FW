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
    SINGLE = 1, // only empty square left in row/col/box
    ONLY_POSSIBLE = 2, // only possibility in the square
    HIDDEN_SINGLE = 3,
    NAKED_PAIR = 4, // eliminate possibilities
    HIDDEN_PAIR = 5,
    NAKED_TRIPLE = 6,
    HIDDEN_TRIPLE = 7,
    X_WING = 8,
    XY_WING = 9,
    GUESS = 10,
    NOTE_ELIMINATION = 11,
    KNOWN_SOLUTION = 12, // use the known solution instead of calculating it
    TECHNIQUE_TYPE_LAST,
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

    uint8_t hintBuf[1024];
} sudokuMoveDesc_t;

typedef struct
{
    uint8_t* hintBuf;
    size_t hintbufLen;

    uint16_t* notes;
    uint16_t* boxNotes;
    uint16_t* rowNotes;
    uint16_t* colNotes;

    uint8_t* boxMap;

    const uint8_t* solution;

    int size;
    int base;

    int searchIdx;
    int elimIdx;
    int digit;
    int pos;
} solverCache_t;

bool sudokuNextMove2(solverCache_t* cache, const sudokuGrid_t* board);
bool sudokuNextMove(sudokuMoveDesc_t* desc, sudokuOverlay_t* overlay, const sudokuGrid_t* board);
void sudokuApplyMove(sudokuGrid_t* board, const sudokuMoveDesc_t* desc);
void hintToOverlay(sudokuOverlay_t* overlay, const sudokuGrid_t* game, int stepNum, const uint8_t* hint, size_t n);

bool initSolverCache(solverCache_t* cache, int size, int base);
void resetSolverCache(solverCache_t* cache, int size, int base);
void deinitSolverCache(solverCache_t* cache);
void makeBoxMapp(solverCache_t* cache, const sudokuGrid_t* board);

void writeStepDescription(char* buf, size_t n, const uint8_t* hintBuf, size_t hintbufLen, int step);
void applyHint(sudokuGrid_t* game, uint16_t* notes, const uint8_t* hintbuf, size_t hintbufLen);
void hintBufDebug(const uint8_t* hint, size_t hintbufLen);
