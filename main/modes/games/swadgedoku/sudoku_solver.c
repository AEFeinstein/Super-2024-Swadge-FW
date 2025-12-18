#include "sudoku_solver.h"
#include "sudoku_game.h"
#include "sudoku_ui.h"

#include <stddef.h>

// "Hint Buffer" tools
// A hint buffer is just a simple bytecode stream describing sudoku actions
// This makes it much easier to deal with move sequences that might require
// several rounds of elimination before a digit is actually added, or for
// techniques (such as guess-and-check) that might solve multiple digits all at once

// The buffer always starts with 4 bytes, which represent the length of the buffer in big-endian
// (excluding the length field itself)
// Then, it is a stream of step descriptors which always start with an opcode.
// The opcode is a single byte with
//    - the high nibble containing a 'count' field, and
//    - the low nibble containing an opcode for the step descriptor type
// If the count field is used, the step may have additional data
// Opcode:   [CCCCNNNN] C: count (4 bits); N: opcode (4 bits, 0-15)
// Mark step: 0x_0 [type]  --- count is ignored
// Set digit: 0x_1 [digit] [pos * count]
// Add notes: 0x_2 [notes-msb] [notes-lsb] [pos * count]
// Del notes: 0x_3 [notes-msb] [notes-lsb] [pos * count]
// Highlight: 0x_4 [digit] [box] [row] [col]             --- Highlight cells matching ALL params (not set to 0xFF).
// Also, `count` used as color

#define OP_STEP      0x0
#define OP_SET_DIGIT 0x1
#define OP_ADD_NOTE  0x2
#define OP_DEL_NOTE  0x3
#define OP_HIGHLIGHT 0x4

#define OP_MASK_OP     0x0F
#define OP_MASK_COUNT  0xF0
#define OP_SHIFT_COUNT 4

#define MAKE_OPCODE(code, count) (((code) & OP_MASK_OP) | (((count) << OP_SHIFT_COUNT) & OP_MASK_COUNT))
#define GET_LENGTH(buf)          (((buf)[0] << 24) | ((buf)[1] << 16) | ((buf)[2] << 8) | (buf)[3])
#define SET_LENGTH(buf, length)           \
    do                                    \
    {                                     \
        buf[0] = ((length) >> 24) & 0xFF; \
        buf[1] = ((length) >> 16) & 0xFF; \
        buf[2] = ((length) >> 8) & 0xFF;  \
        buf[3] = (length) & 0xFF;         \
    } while (0)

typedef enum
{
    SCB_CELL,
    SCB_REGION,
} solverCbType_t;

typedef bool (*searchCellCb)(solverCache_t* state, const sudokuGrid_t* board, int pos);
typedef bool (*searchRegionCb)(solverCache_t* state, const sudokuGrid_t* board, sudokuRegionType_t regionType,
                               int region);
typedef bool (*eliminateCb)(solverCache_t* state, const sudokuGrid_t* board);

typedef struct
{
    solverCbType_t type;
    union
    {
        searchCellCb cell;
        searchRegionCb region;
    };
    const char* name;
} solverCallback_t;

typedef struct
{
    sudokuTechniqueType_t technique;
    const char* format;
} techniqueDesc_t;

typedef struct
{
    eliminateCb func;
    const char* name;
} eliminateCallback_t;

typedef struct
{
    uint8_t type;
    union
    {
        struct
        {
            sudokuTechniqueType_t technique;
        } step;

        struct
        {
            uint8_t digit;
            uint8_t positionCount;
            uint8_t positions[16];
        } setDigit;

        struct
        {
            uint16_t notes;
            uint8_t positionCount;
            uint8_t positions[16];
        } note;

        struct
        {
            uint8_t digit;
            int box;
            int row;
            int col;
            uint8_t color;
        } highlight;
    };
} hintOperation_t;

static bool findLastEmptyCell(solverCache_t* cache, const sudokuGrid_t* board, sudokuRegionType_t type, int region);
static bool findOnlyPossibility(solverCache_t* cache, const sudokuGrid_t* board, int pos);
static bool findHiddenSingle(solverCache_t* cache, const sudokuGrid_t* board, sudokuRegionType_t type, int region);

static bool eliminatePointers(solverCache_t* cache, const sudokuGrid_t* board);
static bool eliminateNakedPairs(solverCache_t* cache, const sudokuGrid_t* board);
static bool eliminateHiddenPairs(solverCache_t* cache, const sudokuGrid_t* board);
static bool eliminateNakedTriples(solverCache_t* cache, const sudokuGrid_t* board);
static bool eliminateHiddenTriples(solverCache_t* cache, const sudokuGrid_t* board);

static bool eliminateShadows(sudokuMoveDesc_t* desc, uint8_t* hintBuf, size_t maxlen, uint16_t* notes,
                             const sudokuGrid_t* board);
static int eliminatePairsTriplesEtc(uint16_t* notes, uint16_t* digits, uint8_t* pos0, uint8_t* pos1, uint8_t* pos2,
                                    const sudokuGrid_t* board);
static void applyCellElimination(const sudokuGrid_t* board, uint16_t* notes, int pos, uint16_t mask, int elimCount,
                                 const uint8_t* eliminations);
static void addElimination(sudokuMoveDesc_t* desc, uint8_t* hintBuf, size_t maxlen, int cellCount, uint16_t digits,
                           sudokuRegionType_t regionType, int regionNum, uint8_t* cellPos);

static uint8_t getMemberPos(const solverCache_t* cache, const sudokuGrid_t* board, sudokuRegionType_t type, int region,
                            int member);
static uint8_t getMemberDigit(const solverCache_t* cache, const sudokuGrid_t* board, sudokuRegionType_t type,
                              int region, int member);
static uint16_t getMemberNotes(const solverCache_t* cache, const sudokuGrid_t* board, sudokuRegionType_t type,
                               int region, int member);
static uint16_t getRegionNotes(const solverCache_t* cache, const sudokuGrid_t* board, sudokuRegionType_t type,
                               int region);
static uint8_t getRegionSize(const solverCache_t* cache, const sudokuGrid_t* board, sudokuRegionType_t type);

bool hintBufNextStep(uint8_t* buf, size_t maxlen, sudokuTechniqueType_t id);
bool hintBufSetDigit(uint8_t* buf, size_t maxlen, uint8_t digit, uint8_t pos);
bool hintBufSetMultiDigit(uint8_t* buf, size_t maxlen, uint8_t digit, int count, const uint8_t* pos);
bool hintBufAddNote(uint8_t* buf, size_t maxlen, uint16_t notes, uint8_t pos);
bool hintBufDelNote(uint8_t* buf, size_t maxlen, uint16_t notes, uint8_t pos);
bool hintBufSetMultiNote(uint8_t* buf, size_t maxlen, bool add, uint16_t notes, int count, const uint8_t* pos);
bool hintBufAddHighlight(uint8_t* buf, size_t maxlen, int digit, int box, int row, int col);
bool hintBufHighlightRegion(uint8_t* buf, size_t maxlen, sudokuRegionType_t type, int region);
size_t hintBufRead(hintOperation_t* op, const uint8_t* hint, size_t hintlen, size_t offset);

static const char* const aOrAnTable[] = {
    "",  // a 0
    "",  // a 1
    "",  // a 2
    "",  // a 3
    "",  // a 4
    "",  // a 5
    "",  // a 6
    "",  // a 7
    "n", // an 8
    "",  // a 9
    "n", // an A
    "",  // a B
    "",  // a C
    "",  // a D
    "n", // an E
    "n", // an F
};

static const solverCallback_t findOrder[] = {
    {
        .type   = SCB_REGION,
        .region = findLastEmptyCell,
        .name   = "Last Empty Cell",
    },
    {
        .type = SCB_CELL,
        .cell = findOnlyPossibility,
        .name = "Only Possibiliy",
    },
    {
        .type   = SCB_REGION,
        .region = findHiddenSingle,
        .name   = "Hidden Single",
    },
};

static const eliminateCallback_t eliminateOrder[] = {
    {
        .func = eliminatePointers,
        .name = "Pointers",
    },
    {
        .func = eliminateNakedPairs,
        .name = "Naked Pairs",
    },
    {
        .func = eliminateHiddenPairs,
        .name = "Hidden Pairs",
    },
    {
        .func = eliminateNakedTriples,
        .name = "Naked Triples",
    },
    {
        .func = eliminateHiddenTriples,
        .name = "Hidden Triples",
    },
};

// Arguments:
// %1$s: Digit
// %2$d: Row
// %3$d: Column
// %4$s: aOrAnTable[sd->hint.digit] ('' or 'n')
// %5$s: "row"/"column"/"box" (region type name)
// %6$d: row/column/box id
static const techniqueDesc_t techniqueDescriptions[] = {
    {.technique = SINGLE, .format = "This is the only valid position for a%4$s %1$d within the %5$s"},
    {.technique = ONLY_POSSIBLE, .format = "%1$d is the only possible digit in this cell"},
    {
        .technique = HIDDEN_SINGLE,
        .format    = "Hidden Single",
    },
    {
        .technique = NAKED_PAIR,
        .format    = "Naked Pair",
    },
    {
        .technique = HIDDEN_PAIR,
        .format    = "Hidden Pair",
    },
    {
        .technique = NAKED_TRIPLE,
        .format    = "Naked Triple",
    },
    {
        .technique = HIDDEN_TRIPLE,
        .format    = "Hidden Triple",
    },
    {
        .technique = X_WING,
        .format    = "X-wing",
    },
    {
        .technique = XY_WING,
        .format    = "XY-Wing",
    },
    {
        .technique = GUESS,
        .format    = "Guess",
    },
    {
        .technique = KNOWN_SOLUTION,
        .format    = "Solution",
    },
    {
        .technique = FOUND_MISTAKE,
        .format    = "This cell has a mistake!",
    },
};

/**
 * @brief Determine the next simplest move needed to solve the puzzle
 *
 * @param cache A pointer to a solver cache struct which holds data needed for various solver functions
 * @param overlay A pointer to a grid-sized overlay array, where hint overlays will be added if non-NULL
 * @param notes A pointer to a grid-sized notes array, where hint notes will be added if non-NULL
 * @param board A pointer to the game board to be solved
 * @return true when a move was found and written to desc
 * @return false when no move was found because the board is in an illegal or unsolvable state
 */
bool sudokuNextMove(solverCache_t* cache, const sudokuGrid_t* board)
{
    size_t boardSize = board->size * board->size;

    sudokuGetNotes(cache->notes, board, 0);
    sudokuGetIndividualNotes(cache->rowNotes, cache->colNotes, cache->boxNotes, board, 0);

    if (cache->solution)
    {
        for (int pos = 0; pos < boardSize; pos++)
        {
            if (board->grid[pos] && board->grid[pos] != cache->solution[pos])
            {
                // Empty cell found
                hintBufNextStep(cache->hintBuf, cache->hintbufLen, FOUND_MISTAKE);
                hintBufSetDigit(cache->hintBuf, cache->hintbufLen, 0, pos);
                hintBufAddHighlight(cache->hintBuf, cache->hintbufLen, -1, -1, pos / cache->size, pos % cache->size);
                return true;
            }
        }
    }

    bool giveUp = false;
    while (!giveUp)
    {
        solverCallback_t cb = findOrder[cache->searchIdx];
        ESP_LOGI("Sudoku", "Searching for %s...", cb.name);
        if (cb.type == SCB_CELL)
        {
            for (int pos = 0; pos < boardSize; pos++)
            {
                if (cb.cell(cache, board, pos))
                {
                    ESP_LOGI("Sudoku", "Found at cell %d", pos);
                    return true;
                }
            }
        }
        else if (cb.type == SCB_REGION)
        {
            for (int box = 0; box < board->base; box++)
            {
                if (cb.region(cache, board, REGION_BOX, box))
                {
                    ESP_LOGI("Sudoku", "Found at box %d", box);
                    return true;
                }
            }

            for (int row = 0; row < board->size; row++)
            {
                if (cb.region(cache, board, REGION_ROW, row))
                {
                    ESP_LOGI("Sudoku", "Found at row %d", row);
                    return true;
                }
            }

            for (int col = 0; col < board->size; col++)
            {
                if (cb.region(cache, board, REGION_COLUMN, col))
                {
                    ESP_LOGI("Sudoku", "Found at col %d", col);
                    return true;
                }
            }
        }

        if (++cache->searchIdx >= ARRAY_SIZE(findOrder))
        {
            ESP_LOGI("Sudoku", "Nothing found! Trying eliminations");
            cache->searchIdx = 0;
            while (true)
            {
                eliminateCallback_t elimCb = eliminateOrder[cache->elimIdx];
                ESP_LOGI("Sudoku", "Eliminating %s...", elimCb.name);
                if (elimCb.func(cache, board))
                {
                    cache->elimIdx = 0;
                    ESP_LOGI("Sudoku", "Success! Restarting search");
                    break;
                }
                else
                {
                    if (++cache->elimIdx >= ARRAY_SIZE(eliminateOrder))
                    {
                        ESP_LOGI("Sudoku", "Last elimination failed, giving up!");
                        cache->elimIdx = 0;
                        giveUp         = true;
                        break;
                    }
                }
            }
        }
    }

    // Lastly, check for the solution and offer one.
    if (cache->solution)
    {
        ESP_LOGI("Sudoku", "No hint generated, falling back to known solution");
        for (int pos = 0; pos < boardSize; pos++)
        {
            if (!board->grid[pos])
            {
                // Empty cell found
                hintBufNextStep(cache->hintBuf, cache->hintbufLen, KNOWN_SOLUTION);
                hintBufSetDigit(cache->hintBuf, cache->hintbufLen, cache->solution[pos], pos);
                hintBufAddHighlight(cache->hintBuf, cache->hintbufLen, -1, -1, pos / cache->size, pos % cache->size);
                return true;
            }
        }
    }
    else
    {
        ESP_LOGI("Sudoku", "No hint generated and no solution given!");
    }

    return false;
}

static bool eliminateShadows(sudokuMoveDesc_t* desc, uint8_t* hintBuf, size_t maxlen, uint16_t* notes,
                             const sudokuGrid_t* board)
{
    // TODO
    // Ok here's the strategy:
    // - One digit at a time,
    for (int digit = 1; digit < board->base; digit++)
    {
        uint16_t bit = 1 << (digit - 1);

        // - Keep track, per-box, of the row and column (separately) of each possibility
        int possibleRows[board->base];
        int possibleCols[board->base];

        // -1 here will mean "not yet set"
        // -2 will mean "multiple matches"
        for (int i = 0; i < board->base; i++)
        {
            possibleRows[i] = -1;
            possibleCols[i] = -1;
        }

        // - Go through all cells
        for (int n = 0; n < board->size * board->size; n++)
        {
            int row = n / board->size;
            int col = n % board->size;

            // - Once set, if the row or column differs from the previous, then it's a no-go
            int box = board->boxMap[n];
            if (box != BOX_NONE)
            {
                if (notes[n] & bit)
                {
                    // This digit is a possibility! Write that down.
                    if (possibleRows[box] == -1)
                    {
                        // Not found in any row yet, save the first one
                        possibleRows[box] = row;
                    }
                    else if (possibleRows[box] != row)
                    {
                        // Already found in a different row
                        // Set to -2 aka "multiple matches"
                        possibleRows[box] = -2;
                    }

                    // And do the same for columns
                    if (possibleCols[box] == -1)
                    {
                        possibleCols[box] = col;
                    }
                    else if (possibleCols[box] != col)
                    {
                        possibleCols[box] = -2;
                    }
                }
            }
        }

        // - At the end of the cell loop, eliminate matching possibilities from their row/column
        for (int box = 0; box < board->base; box++)
        {
            if (possibleRows[box] >= 0)
            {
                bool eliminated = false;
                uint8_t cellPos[16];
                int cellCount = 0;

                // eliminate the digit from all spots in the row
                for (int c = 0; c < board->size; c++)
                {
                    int pos = possibleRows[box] * board->size + c;
                    if (board->boxMap[pos] != box)
                    {
                        if ((notes[pos] & bit))
                        {
                            // we can eliminate the possibility
                            notes[pos] &= ~bit;
                            eliminated = true;
                        }
                    }
                    else
                    {
                        cellPos[cellCount++] = pos;
                    }
                }

                if (eliminated)
                {
                    addElimination(desc, hintBuf, maxlen, cellCount, bit, REGION_ROW, possibleRows[box], cellPos);
                    return true;
                }
            }

            if (possibleCols[box] >= 0)
            {
                bool eliminated = false;
                uint8_t cellPos[16];
                int cellCount = 0;

                // eliminate the digit from all spots in the row
                for (int r = 0; r < board->size; r++)
                {
                    int pos = r * board->size + possibleCols[box];
                    if (board->boxMap[pos] != box)
                    {
                        if ((notes[pos] & bit))
                        {
                            notes[pos] &= ~bit;
                            eliminated = true;
                        }
                    }
                    else
                    {
                        cellPos[cellCount++] = pos;
                    }
                }

                if (eliminated)
                {
                    addElimination(desc, hintBuf, maxlen, cellCount, bit, REGION_COLUMN, possibleCols[box], cellPos);
                    return true;
                }
            }
        }
    }

    return false;
}

static int eliminatePairsTriplesEtc(uint16_t* notes, uint16_t* digits, uint8_t* pos0, uint8_t* pos1, uint8_t* pos2,
                                    const sudokuGrid_t* board)
{
    // TODO: more than one pass (for triples)
    for (int pass = 0; pass < 1; pass++)
    {
        // Eliminate ONE naked pair/triple/etc
        for (int a = 1; a <= board->base; a++)
        {
            for (int b = a + 1; b <= board->base; b++)
            {
                uint16_t bitA = 1 << (a - 1);
                uint16_t bitB = 1 << (b - 1);

                int rFirstPos[board->size];
                int rSecondPos[board->size];
                int cFirstPos[board->size];
                int cSecondPos[board->size];
                int boxFirstPos[board->base];
                int boxSecondPos[board->base];

                for (int i = 0; i < board->size; i++)
                {
                    rFirstPos[i]  = -1;
                    rSecondPos[i] = -1;
                    cFirstPos[i]  = -1;
                    cSecondPos[i] = -1;
                }

                for (int i = 0; i < board->base; i++)
                {
                    boxFirstPos[i]  = -1;
                    boxSecondPos[i] = -1;
                }

                // start with the pairs
                if (pass == 0)
                {
                    // okay so how do we actually detect a pair
                    // it's any two digits which, within a given row/col within a box,
                    //     are only found in those two boxes,
                    //     meaning we can eliminate all other possibilities in the row/box
                    for (int pos = 0; pos < board->size * board->size; pos++)
                    {
                        int row = pos / board->size;
                        int col = pos % board->size;
                        int box = board->boxMap[pos];

                        if (board->grid[pos] != 0)
                        {
                            continue;
                        }

                        if ((notes[pos] & (bitA | bitB)) == (bitA | bitB))
                        {
                            if (rFirstPos[row] == -1)
                            {
                                rFirstPos[row] = pos;
                            }
                            else if (rSecondPos[row] == -1)
                            {
                                rSecondPos[row] = pos;
                            }
                            else
                            {
                                // found more than two, no dice
                                rFirstPos[row]  = -2;
                                rSecondPos[row] = -2;
                            }

                            if (cFirstPos[col] == -1)
                            {
                                cFirstPos[col] = pos;
                            }
                            else if (cSecondPos[col] == -1)
                            {
                                cSecondPos[col] = pos;
                            }
                            else
                            {
                                // nope
                                cFirstPos[col]  = -2;
                                cSecondPos[col] = -2;
                            }

                            if (box != BOX_NONE)
                            {
                                if (boxFirstPos[box] == -1)
                                {
                                    boxFirstPos[box] = pos;
                                }
                                else if (boxSecondPos[box] == -1)
                                {
                                    boxSecondPos[box] = pos;
                                }
                                else
                                {
                                    // nope
                                    boxFirstPos[box]  = -2;
                                    boxSecondPos[box] = -2;
                                }
                            }
                        }
                        else if ((notes[pos] & bitA) || (notes[pos] & bitB))
                        {
                            // only one of the bits was set, won't work!
                            rFirstPos[row]  = -2;
                            rSecondPos[row] = -2;
                            cFirstPos[col]  = -2;
                            cSecondPos[col] = -2;

                            if (box != BOX_NONE)
                            {
                                boxFirstPos[box]  = -2;
                                boxSecondPos[box] = -2;
                            }
                        }
                    }
                }
                else
                {
                    // no pairs found, second pass goes with triples
                    // for (int c = b + 1; c <= board->base; c++)
                    {
                        // TODO
                    }
                }

                for (int i = 0; i < board->size; i++)
                {
                    if (rFirstPos[i] >= 0 && rSecondPos[i] >= 0)
                    {
                        bool eliminated = false;

                        // Eliminate across row and return
                        for (int pos = i * board->size; pos < (i + 1) * board->size; pos++)
                        {
                            if (pos != rFirstPos[i] && pos != rSecondPos[i])
                            {
                                if (!board->grid[pos] && (notes[pos] & (bitA | bitB)))
                                {
                                    notes[pos] &= ~(bitA | bitB);
                                    eliminated = true;
                                }
                            }
                            else if (notes[pos] & ~(bitA | bitB))
                            {
                                // Instead of eliminating the digits here, we eliminate everything else
                                notes[pos] &= (bitA | bitB);
                                eliminated = true;
                            }
                        }
                        if (eliminated)
                        {
                            ESP_LOGI("Solver", "Eliminated pairs in row %d at %d and %d (%d/%d)", i, rFirstPos[i],
                                     rSecondPos[i], a, b);
                            *pos0   = rFirstPos[i];
                            *pos1   = rSecondPos[i];
                            *digits = (bitA | bitB);
                            return 2;
                        }
                    }
                    else if (cFirstPos[i] >= 0 && cSecondPos[i] >= 0)
                    {
                        bool eliminated = false;

                        // Eliminate across col and return
                        for (int pos = i; pos < board->size * (board->size - 1) + i; pos += board->size)
                        {
                            if (pos != cFirstPos[i] && pos != cSecondPos[i])
                            {
                                if (!board->grid[pos] && (notes[pos] & (bitA | bitB)))
                                {
                                    notes[pos] &= ~(bitA | bitB);
                                    eliminated = true;
                                }
                            }
                            else if (notes[pos] & ~(bitA | bitB))
                            {
                                // Instead of eliminating the digits here, we eliminate everything else
                                notes[pos] &= (bitA | bitB);
                                eliminated = true;
                            }
                        }

                        if (eliminated)
                        {
                            ESP_LOGI("Solver", "Eliminated pairs in col %d at %d and %d (%d/%d)", i, cFirstPos[i],
                                     cSecondPos[i], a, b);
                            *pos0   = cFirstPos[i];
                            *pos1   = cSecondPos[i];
                            *digits = (bitA | bitB);
                            return 2;
                        }
                    }
                }

                for (int i = 0; i < board->base; i++)
                {
                    if (boxFirstPos[i] >= 0 && boxSecondPos[i] >= 0)
                    {
                        bool eliminated = false;

                        // Eliminate across box
                        for (int pos = 0; pos < board->size * board->size; pos++)
                        {
                            if (board->boxMap[pos] == i)
                            {
                                if (pos != boxFirstPos[i] && pos != boxSecondPos[i])
                                {
                                    if (!board->grid[pos] && (notes[pos] & (bitA | bitB)))
                                    {
                                        notes[pos] &= ~(bitA | bitB);
                                        eliminated = true;
                                    }
                                }
                                else if (notes[pos] & ~(bitA | bitB))
                                {
                                    // Instead of eliminating the digits here, we eliminate everything else
                                    notes[pos] &= (bitA | bitB);
                                    eliminated = true;
                                }
                            }
                        }

                        if (eliminated)
                        {
                            ESP_LOGI("Solver", "Eliminated pairs in box %d at %d and %d (%d/%d)", i, boxFirstPos[i],
                                     boxSecondPos[i], a, b);
                            *pos0   = boxFirstPos[i];
                            *pos1   = boxSecondPos[i];
                            *digits = (bitA | bitB);
                            return 2;
                        }
                    }
                }
            }
        }
    }

    // return 0, not found
    return 0;
}

static void applyCellElimination(const sudokuGrid_t* board, uint16_t* notes, int pos, uint16_t mask, int elimCount,
                                 const uint8_t* eliminations)
{
    // First check if this is one of the eliminated values
    bool isTarget = false;
    for (int i = 0; i < elimCount; i++)
    {
        if (eliminations[i] == pos)
        {
            isTarget = true;
            break;
        }
    }

    if (board->grid[pos] == 0)
    {
        if (isTarget)
        {
            // Remove other possibilities from this note
            notes[pos] &= mask;
        }
        else
        {
            notes[pos] &= ~mask;
        }
    }
}

static void addElimination(sudokuMoveDesc_t* desc, uint8_t* hintBuf, size_t maxlen, int cellCount, uint16_t digits,
                           sudokuRegionType_t regionType, int regionNum, uint8_t* cellPos)
{
    hintBufNextStep(hintBuf, maxlen, NOTE_ELIMINATION);
    hintBufSetMultiNote(hintBuf, maxlen, false, digits, cellCount, cellPos);

    switch (regionType)
    {
        case REGION_NONE:
        {
            break;
        }
        case REGION_BOX:
        {
            hintBufAddHighlight(hintBuf, maxlen, -1, regionNum, -1, -1);
            break;
        }
        case REGION_ROW:
        {
            hintBufAddHighlight(hintBuf, maxlen, -1, -1, regionNum, -1);
            break;
        }
        case REGION_COLUMN:
        {
            hintBufAddHighlight(hintBuf, maxlen, -1, -1, -1, regionNum);
            break;
        }
    }

    /*if (desc->stepCount < MAX_ELIMINATION_STEPS)
    {
        sudokuMoveElimination_t* elim = &desc->eliminationSteps[desc->stepCount];
        elim->eliminationCount = cellCount;
        elim->eliminationMask = digits;
        elim->eliminationRegionType = regionType;
        elim->eliminationRegionNum = regionNum;
        for (int i = 0; i < cellCount; i++)
        {
            elim->eliminations[i] = cellPos[i];
        }

        desc->stepCount++;
    }*/
}

void sudokuApplyMove(sudokuGrid_t* board, const sudokuMoveDesc_t* desc)
{
    if (desc->digit > 0)
    {
        setDigit(board, desc->digit, desc->pos % board->size, desc->pos / board->size);
    }

    for (int i = 0; i < desc->stepCount; i++)
    {
        const sudokuMoveElimination_t* step = &desc->eliminationSteps[i];
        if (step->eliminationCount > 0)
        {
            // Apply the mask to the pair/triple
            for (int j = 0; j < step->eliminationCount; j++)
            {
                board->notes[step->eliminations[j]] &= step->eliminationMask;
            }

            // Apply the mask to the
            if (step->eliminationRegionType == REGION_BOX)
            {
                for (int pos = 0; pos < board->size * board->size; pos++)
                {
                    if (board->boxMap[pos] == step->eliminationRegionNum)
                    {
                        applyCellElimination(board, board->notes, pos, step->eliminationMask, step->eliminationCount,
                                             step->eliminations);
                    }
                }
            }
            else if (step->eliminationRegionType == REGION_ROW)
            {
                for (int col = 0; col < board->size; col++)
                {
                    int pos = step->eliminationRegionNum * board->size + col;

                    applyCellElimination(board, board->notes, pos, step->eliminationMask, step->eliminationCount,
                                         step->eliminations);
                }
            }
            else if (step->eliminationRegionType == REGION_COLUMN)
            {
                for (int row = 0; row < board->size; row++)
                {
                    int pos = row * board->size + step->eliminationRegionNum;

                    applyCellElimination(board, board->notes, pos, step->eliminationMask, step->eliminationCount,
                                         step->eliminations);
                }
            }
        }
    }
}

void hintToOverlay(sudokuOverlay_t* overlay, const sudokuGrid_t* game, int stepNum, const uint8_t* hint, size_t n)
{
    size_t boardSize = game->size * game->size;

    // Skip the length header
    uint32_t idx = 4;
    uint32_t len = GET_LENGTH(hint);

    sudokuOverlayShape_t* shape;

    int curStep = -1;

    while (idx < n && idx < len + 4)
    {
        uint8_t header = hint[idx++];
        uint8_t op     = header & OP_MASK_OP;
        uint8_t count  = (header & OP_MASK_COUNT) >> OP_SHIFT_COUNT;

        switch (op)
        {
            case OP_STEP:
            {
                if (curStep == -1)
                {
                    curStep = 0;
                }
                else
                {
                    curStep++;
                }

                sudokuTechniqueType_t type = (sudokuTechniqueType_t)hint[idx++];
                break;
            }

            case OP_SET_DIGIT:
            {
                uint8_t digit = hint[idx++];
                for (int digitNum = 0; digitNum < count + 1; digitNum++)
                {
                    uint8_t pos = hint[idx++];

                    if (stepNum == -1 || curStep == stepNum)
                    {
                        uint8_t r = pos / game->size;
                        uint8_t c = pos % game->size;

                        shape              = heap_caps_malloc(sizeof(sudokuOverlayShape_t), MALLOC_CAP_8BIT);
                        shape->type        = OVERLAY_DIGIT;
                        shape->color       = c005;
                        shape->tag         = ST_HINT;
                        shape->digit.digit = digit;
                        shape->digit.pos.x = c * BOX_SIZE_SUBPOS;
                        shape->digit.pos.y = r * BOX_SIZE_SUBPOS;
                        getOverlayPos(&shape->digit.pos.x, &shape->digit.pos.y, r, c, SUBPOS_CENTER);
                        push(&overlay->shapes, shape);

                        overlay->gridOpts[pos] |= OVERLAY_SKIP;
                    }
                }
                break;
            }

            case OP_ADD_NOTE:
            case OP_DEL_NOTE:
            {
                uint16_t notes = hint[idx++];
                notes <<= 8;
                notes |= hint[idx++];
                paletteColor_t color = (op == OP_ADD_NOTE) ? c005 : c500;

                for (int posIdx = 0; posIdx < count + 1 && idx < n; posIdx++)
                {
                    int pos = hint[idx++];

                    if (stepNum == -1 || curStep == stepNum)
                    {
                        shape              = heap_caps_malloc(sizeof(sudokuOverlayShape_t), MALLOC_CAP_8BIT);
                        shape->type        = OVERLAY_NOTES_SHAPE;
                        shape->color       = color;
                        shape->tag         = ST_HINT;
                        shape->notes.notes = notes;

                        int elimR = pos / game->size;
                        int elimC = pos % game->size;
                        getOverlayPos(&shape->notes.pos.x, &shape->notes.pos.y, elimR, elimC, SUBPOS_CENTER);
                        push(&overlay->shapes, shape);

                        overlay->gridOpts[pos] |= OVERLAY_SKIP;
                    }
                }
                break;
            }

            case OP_HIGHLIGHT:
            {
                uint8_t targetDigit = hint[idx++];
                uint8_t targetBox   = hint[idx++];
                uint8_t targetRow   = hint[idx++];
                uint8_t targetCol   = hint[idx++];

                bool useDigit = (targetDigit != 0 && targetDigit <= game->base);
                bool useBox   = (targetBox < game->base);
                bool useRow   = (targetRow < game->size);
                bool useCol   = (targetCol < game->size);

                ESP_LOGI("Solver", "Digit / Box / Row / Col: %s / %s / %s / %s", useDigit ? "yes" : "no",
                         useBox ? "yes" : "no", useRow ? "yes" : "no", useCol ? "yes" : "no");

                if (stepNum != -1 && curStep != stepNum)
                {
                    break;
                }

                if (useBox && !useDigit && !useRow && !useCol)
                {
                    // box the entire... box
                    addBoxHighlight(overlay, game, targetBox);
                    break;
                }
                else if (useRow && !useDigit && !useBox && !useCol)
                {
                    // box the entire row
                    shape        = heap_caps_malloc(sizeof(sudokuOverlayShape_t), MALLOC_CAP_8BIT);
                    shape->type  = OVERLAY_RECT;
                    shape->color = c050;
                    shape->tag   = ST_HINT;
                    getOverlayPos(&shape->rectangle.pos.x, &shape->rectangle.pos.y, targetRow, 0, SUBPOS_NW);
                    getOverlayPos(&shape->rectangle.width, &shape->rectangle.height, 1, game->size, SUBPOS_NW);
                    push(&overlay->shapes, shape);
                    break;
                }
                else if (useCol && !useDigit && !useBox && !useRow)
                {
                    // box the entire column
                    shape        = heap_caps_malloc(sizeof(sudokuOverlayShape_t), MALLOC_CAP_8BIT);
                    shape->type  = OVERLAY_RECT;
                    shape->color = c050;
                    shape->tag   = ST_HINT;
                    getOverlayPos(&shape->rectangle.pos.x, &shape->rectangle.pos.y, 0, targetCol, SUBPOS_NW);
                    getOverlayPos(&shape->rectangle.width, &shape->rectangle.height, game->size, 1, SUBPOS_NW);
                    push(&overlay->shapes, shape);
                    break;
                }

                for (int pos = 0; pos < boardSize; pos++)
                {
                    int row   = pos / game->size;
                    int col   = pos % game->size;
                    int box   = game->boxMap[pos];
                    int digit = game->grid[pos];

                    if (useDigit && digit != targetDigit)
                    {
                        continue;
                    }

                    if (useBox && box != targetBox)
                    {
                        continue;
                    }

                    if (useRow && row != targetRow)
                    {
                        continue;
                    }

                    if (useCol && col != targetCol)
                    {
                        continue;
                    }

                    // box a single square
                    shape        = heap_caps_malloc(sizeof(sudokuOverlayShape_t), MALLOC_CAP_8BIT);
                    shape->type  = OVERLAY_CIRCLE;
                    shape->color = c035;
                    shape->tag   = ST_HINT;
                    getOverlayPos(&shape->circle.pos.x, &shape->circle.pos.y, row, col, SUBPOS_CENTER);
                    shape->circle.radius = BOX_SIZE_SUBPOS * 7 / 10;
                    push(&overlay->shapes, shape);
                }
                break;
            }
        }
    }
}

bool hintBufNextStep(uint8_t* buf, size_t maxlen, sudokuTechniqueType_t id)
{
    if (maxlen < 4)
    {
        return false;
    }

    uint32_t len = GET_LENGTH(buf);
    if (len + 4 + 2 > maxlen)
    {
        return false;
    }

    uint32_t off = len + 4;
    buf[off++]   = OP_STEP;
    buf[off++]   = (uint8_t)id;

    SET_LENGTH(buf, off - 4);

    return true;
}

bool hintBufSetDigit(uint8_t* buf, size_t maxlen, uint8_t digit, uint8_t pos)
{
    if (maxlen < 4)
    {
        return false;
    }

    uint32_t len = GET_LENGTH(buf);
    if (len + 4 + 3 > maxlen)
    {
        return false;
    }

    uint32_t off = len + 4;
    buf[off++]   = MAKE_OPCODE(OP_SET_DIGIT, 0);
    buf[off++]   = digit;
    buf[off++]   = pos;

    SET_LENGTH(buf, off - 4);

    return true;
}

bool hintBufSetMultiDigit(uint8_t* buf, size_t maxlen, uint8_t digit, int count, const uint8_t* pos)
{
    if (maxlen < 4 || count < 1)
    {
        return false;
    }

    uint32_t len = GET_LENGTH(buf);
    if (len + 4 + 2 + count > maxlen)
    {
        return false;
    }

    uint32_t off = len + 4;

    buf[off++] = MAKE_OPCODE(OP_SET_DIGIT, count - 1);
    buf[off++] = digit;

    for (int n = 0; n < count; n++)
    {
        buf[off++] = pos[n];
    }

    SET_LENGTH(buf, off - 4);

    return true;
}

bool hintBufAddNote(uint8_t* buf, size_t maxlen, uint16_t notes, uint8_t pos)
{
    return hintBufSetMultiNote(buf, maxlen, true, notes, 1, &pos);
}

bool hintBufDelNote(uint8_t* buf, size_t maxlen, uint16_t notes, uint8_t pos)
{
    return hintBufSetMultiNote(buf, maxlen, false, notes, 1, &pos);
}

bool hintBufSetMultiNote(uint8_t* buf, size_t maxlen, bool add, uint16_t notes, int count, const uint8_t* pos)
{
    if (maxlen < 4 || count < 1)
    {
        return false;
    }

    uint32_t len = GET_LENGTH(buf);
    if (len + 4 + 3 + count > maxlen)
    {
        return false;
    }

    uint32_t off = len + 4;

    buf[off++] = MAKE_OPCODE(add ? OP_ADD_NOTE : OP_DEL_NOTE, count - 1);
    buf[off++] = (notes >> 8) & 0xFF;
    buf[off++] = notes & 0xFF;

    for (int n = 0; n < count; n++)
    {
        buf[off++] = pos[n];
    }

    SET_LENGTH(buf, off - 4);

    return true;
}

bool hintBufAddHighlight(uint8_t* buf, size_t maxlen, int digit, int box, int row, int col)
{
    if (maxlen < 4)
    {
        return false;
    }

    uint32_t len = GET_LENGTH(buf);
    if (len + 4 + 5 > maxlen)
    {
        return false;
    }

    uint32_t off = len + 4;

    uint8_t digitVal = (digit <= 0) ? 255u : digit;
    uint8_t boxVal   = (box < 0) ? 255u : box;
    uint8_t rowVal   = (row < 0) ? 255u : row;
    uint8_t colVal   = (col < 0) ? 255u : col;

    buf[off++] = MAKE_OPCODE(OP_HIGHLIGHT, 0);
    buf[off++] = digitVal;
    buf[off++] = boxVal;
    buf[off++] = rowVal;
    buf[off++] = colVal;

    SET_LENGTH(buf, off - 4);

    return true;
}

bool hintBufHighlightRegion(uint8_t* buf, size_t maxlen, sudokuRegionType_t type, int region)
{
    switch (type)
    {
        case REGION_ROW:
        {
            return hintBufAddHighlight(buf, maxlen, -1, -1, region, -1);
        }

        case REGION_COLUMN:
        {
            return hintBufAddHighlight(buf, maxlen, -1, -1, -1, region);
            break;
        }

        case REGION_BOX:
        {
            return hintBufAddHighlight(buf, maxlen, -1, region, -1, -1);
        }
    }

    return false;
}

size_t hintBufRead(hintOperation_t* op, const uint8_t* hint, size_t maxlen, size_t offset)
{
    if (!op || !hint || maxlen < 4)
    {
        ESP_LOGI("Sudoku", "Invalid hintbuf");
        return 0;
    }

    // Skip the length header
    size_t idx = offset + 4;
    size_t len = GET_LENGTH(hint);

    sudokuOverlayShape_t* shape;

    while (idx < maxlen && idx < len + 4)
    {
        uint8_t header = hint[idx++];
        uint8_t opcode = header & OP_MASK_OP;
        uint8_t count  = (header & OP_MASK_COUNT) >> OP_SHIFT_COUNT;

        switch (opcode)
        {
            case OP_STEP:
            {
                uint8_t type = hint[idx++];
                if (type >= (int)TECHNIQUE_TYPE_LAST)
                {
                    return 0;
                }

                op->type           = opcode;
                op->step.technique = (sudokuTechniqueType_t)type;

                return idx - offset - 4;
            }

            case OP_SET_DIGIT:
            {
                uint8_t positionCount = 0;
                uint8_t positions[16] = {0};

                uint8_t digit = hint[idx++];

                for (int digitNum = 0; digitNum < count + 1 && idx < maxlen && idx < len + 4; digitNum++)
                {
                    uint8_t pos                = hint[idx++];
                    positions[positionCount++] = pos;
                }

                op->type                   = opcode;
                op->setDigit.digit         = digit;
                op->setDigit.positionCount = positionCount;
                memcpy(&op->setDigit.positions, positions, sizeof(positions));

                return idx - offset - 4;
            }

            case OP_ADD_NOTE:
            case OP_DEL_NOTE:
            {
                uint16_t notes = hint[idx++];
                notes <<= 8;
                notes |= hint[idx++];

                uint8_t positionCount = 0;
                uint8_t positions[16] = {0};

                for (int posIdx = 0; posIdx < count + 1 && idx < maxlen && idx < len + 4; posIdx++)
                {
                    uint8_t pos                = hint[idx++];
                    positions[positionCount++] = pos;
                }

                op->type               = opcode;
                op->note.notes         = notes;
                op->note.positionCount = positionCount;
                memcpy(&op->setDigit.positions, positions, sizeof(positions));

                return idx - offset - 4;
            }

            case OP_HIGHLIGHT:
            {
                uint8_t targetDigit = hint[idx++];
                uint8_t targetBox   = hint[idx++];
                uint8_t targetRow   = hint[idx++];
                uint8_t targetCol   = hint[idx++];

                bool useDigit = (targetDigit != 0 && targetDigit != 255);
                bool useBox   = (targetBox != 255);
                bool useRow   = (targetRow != 255);
                bool useCol   = (targetCol != 255);

                op->type            = opcode;
                op->highlight.digit = targetDigit;
                op->highlight.box   = useBox ? targetBox : -1;
                op->highlight.row   = useRow ? targetRow : -1;
                op->highlight.col   = useCol ? targetCol : -1;
                op->highlight.color = count;

                return idx - offset - 4;
            }

            default:
            {
                return 0;
            }
        }
    }

    return 0;
}

void hintBufDebug(const uint8_t* hint, size_t hintbufLen)
{
    size_t offset = 0;
    size_t read   = 0;

    hintOperation_t opInfo = {0};

    int curStep = -1;

    while (0 != (read = hintBufRead(&opInfo, hint, hintbufLen, offset)))
    {
        offset += read;

        switch (opInfo.type)
        {
            case OP_STEP:
            {
                curStep++;
                ESP_LOGI("hintbuf", "Step %d: [NEW STEP] %d", curStep, opInfo.type);
                break;
            }

            case OP_SET_DIGIT:
            {
                char buf[128];
                const char* end = buf + sizeof(buf);
                char* cur       = buf;
                cur += snprintf(buf, sizeof(buf), "Step %d: [SETDIGIT] %2d x %2d @ [", curStep, opInfo.setDigit.digit,
                                opInfo.setDigit.positionCount);

                for (int n = 0; n < opInfo.setDigit.positionCount; n++)
                {
                    if (n != 0)
                    {
                        strncat(buf, ", ", end - cur);
                        cur += 2;
                    }

                    cur += snprintf(cur, end - cur, "%" PRIu8, opInfo.setDigit.positions[n]);
                }
                strncat(buf, "]", end - cur);
                cur++;

                ESP_LOGI("hintbuf", "%s", buf);
                break;
            }

            case OP_ADD_NOTE:
            {
                ESP_LOGI("hintbuf", "Step %d: [ADD NOTE] ...", curStep);
                break;
            }

            case OP_DEL_NOTE:
            {
                ESP_LOGI("hintbuf", "Step %d: [DEL NOTE] ...", curStep);
                break;
            }

            case OP_HIGHLIGHT:
            {
                ESP_LOGI("hintbuf", "Step %d: [HIGHLITE] ...", curStep);
                break;
            }

            default:
            {
                ESP_LOGE("Sudoku", "Unknown opcode: %" PRIu8, opInfo.type);
            }
        }
    }
}

void applyHint(sudokuGrid_t* game, uint16_t* notes, const uint8_t* hint, size_t hintbufLen)
{
    size_t read            = 0;
    size_t offset          = 0;
    hintOperation_t opInfo = {0};
    int curStep            = -1;

    while (0 != (read = hintBufRead(&opInfo, hint, hintbufLen, offset)))
    {
        offset += read;

        switch (opInfo.type)
        {
            case OP_STEP:
            {
                break;
            }

            case OP_SET_DIGIT:
            {
                for (int i = 0; i < opInfo.setDigit.positionCount; i++)
                {
                    setDigit(game, opInfo.setDigit.digit, opInfo.setDigit.positions[i] % game->size,
                             opInfo.setDigit.positions[i] / game->size);
                }
                break;
            }

            case OP_ADD_NOTE:
            {
                for (int i = 0; i < opInfo.note.positionCount; i++)
                {
                    game->notes[opInfo.note.positions[i]] |= opInfo.note.notes;
                    if (notes)
                    {
                        notes[opInfo.note.positions[i]] |= opInfo.note.notes;
                    }
                }
                break;
            }

            case OP_DEL_NOTE:
            {
                for (int i = 0; i < opInfo.note.positionCount; i++)
                {
                    game->notes[opInfo.note.positions[i]] &= ~opInfo.note.notes;
                    if (notes)
                    {
                        notes[opInfo.note.positions[i]] &= ~opInfo.note.notes;
                    }
                }
                break;
            }

            case OP_HIGHLIGHT:
            {
                break;
            }

            default:
            {
                ESP_LOGE("Sudoku", "Unknown opcode: %" PRIu8, opInfo.type);
            }
        }
    }
}

bool initSolverCache(solverCache_t* cache, int size, int base)
{
    const size_t defaultLen = 256;
    uint16_t* notesAlloc    = heap_caps_calloc(size * size + base + size * 2, sizeof(uint16_t), MALLOC_CAP_8BIT);
    if (!notesAlloc)
    {
        return false;
    }

    uint8_t* hintAlloc = heap_caps_malloc(defaultLen, MALLOC_CAP_8BIT);
    if (!hintAlloc)
    {
        free(notesAlloc);
        return false;
    }

    uint8_t* boxMapAlloc = heap_caps_malloc(base * base, MALLOC_CAP_8BIT);
    if (!boxMapAlloc)
    {
        free(hintAlloc);
        free(notesAlloc);
        return false;
    }

    cache->hintBuf    = hintAlloc;
    cache->hintbufLen = defaultLen;

    cache->notes    = notesAlloc;
    cache->boxNotes = notesAlloc + (size * size);
    cache->rowNotes = notesAlloc + (size * size) + base;
    cache->colNotes = notesAlloc + (size * size) + base + size;

    cache->boxMap = boxMapAlloc;

    cache->solution = NULL;

    cache->size = size;
    cache->base = base;

    cache->searchIdx = 0;
    cache->elimIdx   = 0;
    cache->digit     = 1;
    cache->pos       = 0;

    return true;
}

void resetSolverCache(solverCache_t* cache, int size, int base)
{
    if (cache->size != size || cache->base != base)
    {
        deinitSolverCache(cache);
        initSolverCache(cache, size, base);
        return;
    }

    memset(cache->hintBuf, 0, cache->hintbufLen);
    memset(cache->notes, 0, sizeof(uint16_t) * cache->size * cache->size);
    memset(cache->boxNotes, 0, sizeof(uint16_t) * cache->base);
    memset(cache->rowNotes, 0, sizeof(uint16_t) * cache->size);
    memset(cache->colNotes, 0, sizeof(uint16_t) * cache->size);
    memset(cache->boxMap, 0, cache->base * cache->base);

    cache->solution = NULL;

    cache->searchIdx = 0;
    cache->elimIdx   = 0;
    cache->digit     = 1;
    cache->pos       = 0;
}

void deinitSolverCache(solverCache_t* cache)
{
    if (cache->notes)
    {
        free(cache->notes);
        cache->notes    = NULL;
        cache->boxNotes = NULL;
        cache->rowNotes = NULL;
        cache->colNotes = NULL;
    }

    if (cache->hintBuf)
    {
        free(cache->hintBuf);
        cache->hintBuf    = NULL;
        cache->hintbufLen = 0;
    }

    if (cache->boxMap)
    {
        free(cache->boxMap);
        cache->boxMap = NULL;
    }

    cache->size = 0;
    cache->base = 0;
}

void makeBoxMap(solverCache_t* cache, const sudokuGrid_t* board)
{
    uint8_t memberCounts[board->base];
    memset(memberCounts, 0, board->base);

    for (int pos = 0; pos < board->size * board->size; pos++)
    {
        uint8_t box = board->boxMap[pos];
        if (box < board->base)
        {
            cache->boxMap[box * board->base + memberCounts[box]++] = pos;
        }
    }
}

static uint8_t getMemberPos(const solverCache_t* cache, const sudokuGrid_t* board, sudokuRegionType_t type, int region,
                            int member)
{
    switch (type)
    {
        case REGION_BOX:
        {
            return cache->boxMap[region * cache->base + member];
        }

        case REGION_ROW:
        {
            return region * cache->size + member;
        }

        case REGION_COLUMN:
        {
            return member * cache->size + region;
        }

        default:
        {
            return 0;
        }
    }

    return 0;
}

static uint8_t getMemberDigit(const solverCache_t* cache, const sudokuGrid_t* board, sudokuRegionType_t type,
                              int region, int member)
{
    switch (type)
    {
        case REGION_BOX:
        {
            return board->grid[cache->boxMap[region * cache->base + member]];
        }

        case REGION_ROW:
        {
            return board->grid[region * cache->size + member];
        }

        case REGION_COLUMN:
        {
            return board->grid[member * cache->size + region];
        }

        default:
        {
            return 0;
        }
    }

    return 0;
}

static uint16_t getMemberNotes(const solverCache_t* cache, const sudokuGrid_t* board, sudokuRegionType_t type,
                               int region, int member)
{
    switch (type)
    {
        case REGION_BOX:
        {
            return board->notes[cache->boxMap[region * cache->base + member]];
        }

        case REGION_ROW:
        {
            return board->notes[region * cache->size + member];
        }

        case REGION_COLUMN:
        {
            return board->notes[member * cache->size + region];
        }

        default:
        {
            return 0;
        }
    }

    return 0;
}

static uint16_t getRegionNotes(const solverCache_t* cache, const sudokuGrid_t* board, sudokuRegionType_t type,
                               int region)
{
    switch (type)
    {
        case REGION_BOX:
        {
            return cache->boxNotes[region];
        }

        case REGION_ROW:
        {
            return cache->rowNotes[region];
        }

        case REGION_COLUMN:
        {
            return cache->colNotes[region];
        }

        default:
        {
            return 0;
        }
    }

    return 0;
}

static uint8_t getRegionSize(const solverCache_t* cache, const sudokuGrid_t* board, sudokuRegionType_t type)
{
    return (type == REGION_BOX) ? board->base : board->size;
}

static bool findLastEmptyCell(solverCache_t* cache, const sudokuGrid_t* board, sudokuRegionType_t type, int region)
{
    uint8_t memberCount = getRegionSize(cache, board, type);
    for (int member = 0; member < memberCount; member++)
    {
        uint16_t notes = getMemberNotes(cache, board, type, region, member);
        // Only a single digit is possible in this region
        if (notes != 0 && __builtin_popcount(notes) == 1 && 0 == getMemberDigit(cache, board, type, region, member))
        {
            ESP_LOGI("Solver", "Region has only one possibility left: %d, %" PRIb16 " (popcount=%d)\n", region, notes,
                     __builtin_popcount(notes));
            int missingDigit = __builtin_ctz(notes) + 1;
            uint8_t pos      = getMemberPos(cache, board, type, region, member);
            hintBufNextStep(cache->hintBuf, cache->hintbufLen, SINGLE);
            hintBufSetDigit(cache->hintBuf, cache->hintbufLen, missingDigit, pos);
            hintBufAddHighlight(cache->hintBuf, cache->hintbufLen, -1, -1, pos / cache->size, pos % cache->size);
            hintBufHighlightRegion(cache->hintBuf, cache->hintbufLen, type, region);
            return true;
        }
    }

    return false;
}

static bool findOnlyPossibility(solverCache_t* cache, const sudokuGrid_t* board, int pos)
{
    if (__builtin_popcount(cache->notes[pos]) == 1 && cache->notes[pos] != 0)
    {
        int digit = __builtin_ctz(cache->notes[pos]) + 1;
        hintBufNextStep(cache->hintBuf, cache->hintbufLen, ONLY_POSSIBLE);
        hintBufSetDigit(cache->hintBuf, cache->hintbufLen, digit, pos);
        hintBufAddHighlight(cache->hintBuf, cache->hintbufLen, -1, -1, pos / cache->size, pos % cache->size);

        return true;
    }

    return false;
}

static bool findHiddenSingle(solverCache_t* cache, const sudokuGrid_t* board, sudokuRegionType_t type, int region)
{
    uint16_t regionNotes = getRegionNotes(cache, board, type, region);
    uint8_t memberCount  = getRegionSize(cache, board, type);

    for (int digit = 1; digit <= board->base; digit++)
    {
        uint16_t bit = 1 << (digit - 1);
        if (bit & regionNotes)
        {
            int count      = 0;
            int onlyMember = 0;

            // this digit can go here
            for (int member = 0; member < memberCount; member++)
            {
                if (0 == getMemberDigit(cache, board, type, region, member)
                    && (bit & getMemberNotes(cache, board, type, region, member)))
                {
                    if (++count > 1)
                    {
                        break;
                    }

                    onlyMember = member;
                }
            }

            if (count == 1)
            {
                uint8_t pos = getMemberPos(cache, board, type, region, onlyMember);
                hintBufNextStep(cache->hintBuf, cache->hintbufLen, HIDDEN_SINGLE);
                hintBufSetDigit(cache->hintBuf, cache->hintbufLen, digit, pos);
                hintBufAddHighlight(cache->hintBuf, cache->hintbufLen, -1, -1, pos / cache->size, pos % cache->size);
                return true;
            }
        }
    }

    return false;
}

static bool eliminatePointers(solverCache_t* cache, const sudokuGrid_t* board)
{
    return false;
}

static bool eliminateNakedPairs(solverCache_t* cache, const sudokuGrid_t* board)
{
    return false;
}

static bool eliminateHiddenPairs(solverCache_t* cache, const sudokuGrid_t* board)
{
    return false;
}

static bool eliminateNakedTriples(solverCache_t* cache, const sudokuGrid_t* board)
{
    return false;
}

static bool eliminateHiddenTriples(solverCache_t* cache, const sudokuGrid_t* board)
{
    return false;
}

void writeStepDescription(char* buf, size_t n, const uint8_t* hint, size_t hintbufLen, int stepNum)
{
    sudokuOverlayShape_t* shape;

    int curStep = -1;

    char* out       = buf;
    const char* end = buf + n;

    sudokuTechniqueType_t stepType = TECHNIQUE_TYPE_LAST;
    int stepDigit                  = -1;
    int stepDigitRow               = -1;
    int stepDigitColumn            = -1;
    int stepDigitBox               = -1;
    int stepPos                    = -1;
    int stepRegion                 = -1;
    const char* regionTypeName     = "?";

    hintOperation_t opInfo = {0};

    size_t offset = 0;
    size_t read   = 0;

    hintBufDebug(hint, hintbufLen);

    while (0 != (read = hintBufRead(&opInfo, hint, hintbufLen, offset)))
    {
        offset += read;

        switch (opInfo.type)
        {
            case OP_STEP:
            {
                curStep++;
                ESP_LOGI("Sudoku", "Step %d", curStep);

                if (stepNum == -1 || curStep == stepNum)
                {
                    stepType = opInfo.step.technique;
                }
                break;
            }

            case OP_SET_DIGIT:
            {
                if (stepNum == -1 || curStep == stepNum)
                {
                    stepDigit = opInfo.setDigit.digit;
                    stepPos   = opInfo.setDigit.positions[0];
                }
                break;
            }

            case OP_ADD_NOTE:
            {
                if (stepNum == -1 || curStep == stepNum)
                {
                    // TODO
                }
                break;
            }

            case OP_DEL_NOTE:
            {
                if (stepNum == -1 || curStep == stepNum)
                {
                    // TODO
                }
                break;
            }

            case OP_HIGHLIGHT:
            {
                if (stepNum != -1 && curStep != stepNum)
                {
                    break;
                }

                if (opInfo.highlight.box >= 0)
                {
                    regionTypeName = "box";
                    stepDigitBox   = opInfo.highlight.box;
                    stepRegion     = opInfo.highlight.col;
                }
                else if (opInfo.highlight.row >= 0 && opInfo.highlight.col >= 0)
                {
                    regionTypeName  = "cell";
                    stepDigitRow    = opInfo.highlight.row;
                    stepDigitColumn = opInfo.highlight.col;
                    stepRegion      = opInfo.highlight.col;
                }
                else if (opInfo.highlight.row >= 0)
                {
                    regionTypeName = "row";
                    stepDigitRow   = opInfo.highlight.row;
                    stepRegion     = opInfo.highlight.row;
                }
                else if (opInfo.highlight.col >= 0)
                {
                    regionTypeName  = "column";
                    stepDigitColumn = opInfo.highlight.col;
                    stepRegion      = opInfo.highlight.col;
                }
                break;
            }

            default:
            {
                ESP_LOGE("Sudoku", "Unknown opcode: %" PRIu8, opInfo.type);
            }
        }
    }

    if (stepType != TECHNIQUE_TYPE_LAST)
    {
        const char* format = "?";
        for (int i = 0; i < ARRAY_SIZE(techniqueDescriptions); i++)
        {
            if (techniqueDescriptions[i].technique == stepType)
            {
                format = techniqueDescriptions[i].format;
                break;
            }
        }

        ESP_LOGI("Sudoku", "Format is: \"%s\"", format);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
        snprintf(buf, n, format,
                 /////////
                 stepDigit,                                     // %1$s: Digit
                 stepDigitRow + 1,                              // %2$d: Row
                 stepDigitColumn + 1,                           // %3$d: Col
                 (stepDigit >= 0) ? aOrAnTable[stepDigit] : "", // %4$s: A/An
                 regionTypeName,                                // %5$s: row/column/box
                 stepRegion + 1                                 // %6$s: row/col/box ID
        );
#pragma GCC diagnostic pop
    }
    else
    {
        ESP_LOGI("Sudoku", "No technique type set");
    }
}
