#include "sudoku_solver.h"
#include "sudoku_game.h"
#include "sudoku_ui.h"


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
// Highlight: 0x_4 [digit] [box] [row] [col]             --- Highlight cells matching ALL params (not set to 0xFF). Also, `count` used as color

#define OP_STEP 0x0
#define OP_SET_DIGIT 0x1
#define OP_ADD_NOTE 0x2
#define OP_DEL_NOTE 0x3
#define OP_HIGHLIGHT 0x4

#define OP_MASK_OP 0x0F
#define OP_MASK_COUNT 0xF0
#define OP_SHIFT_COUNT 4

#define MAKE_OPCODE(code, count) (((code) & OP_MASK_OP) | (((count) << OP_SHIFT_COUNT) & OP_MASK_COUNT))
#define GET_LENGTH(buf) (((buf)[0] << 24) | ((buf)[1] << 16) | ((buf)[2] << 8) | (buf)[3])
#define SET_LENGTH(buf, length) do { buf[0] = ((length) >> 24) & 0xFF; buf[1] = ((length) >> 16) & 0xFF; buf[2] = ((length) >> 8) & 0xFF; buf[3] = (length) & 0xFF; } while (0)

typedef enum
{
    SCB_CELL,
    SCB_REGION,
} solverCbType_t;

typedef bool (*searchCellCb)(solverCache_t* state, const sudokuGrid_t* board, int pos);
typedef bool (*searchRegionCb)(solverCache_t* state, const sudokuGrid_t* board, sudokuRegionType_t regionType, int region);
typedef bool (*eliminateCb)(solverCache_t* state, const sudokuGrid_t* board);

typedef struct
{
    solverCbType_t type;
    union
    {
        searchCellCb cell;
        searchRegionCb region;
    };
} solverCallback_t;

// new method for this...
// - look for 'last-empty-cell' (row/col/box notes have popcount() == base-1)
// - look for 'only-possibility' (cell notes have popcount() == 1)
// - look for ''

static bool findLastEmptyCell(solverCache_t* cache, const sudokuGrid_t* board, sudokuRegionType_t type, int region);
static bool findOnlyPossibility(solverCache_t* cache, const sudokuGrid_t* board, int pos);

static bool eliminatePointers(solverCache_t* cache, const sudokuGrid_t* board);
static bool eliminateNakedPairs(solverCache_t* cache, const sudokuGrid_t* board);
static bool eliminateHiddenPairs(solverCache_t* cache, const sudokuGrid_t* board);
static bool eliminateNakedTriples(solverCache_t* cache, const sudokuGrid_t* board);
static bool eliminateHiddenTriples(solverCache_t* cache, const sudokuGrid_t* board);

static bool eliminateShadows(sudokuMoveDesc_t* desc, uint8_t* hintBuf, size_t maxlen, uint16_t* notes, const sudokuGrid_t* board);
static int eliminatePairsTriplesEtc(uint16_t* notes, uint16_t* digits, uint8_t* pos0, uint8_t* pos1, uint8_t* pos2, const sudokuGrid_t* board);
static void applyCellElimination(const sudokuGrid_t* board, uint16_t* notes, int pos, uint16_t mask, int elimCount, const uint8_t* eliminations);
static void addElimination(sudokuMoveDesc_t* desc, uint8_t* hintBuf, size_t maxlen, int cellCount, uint16_t digits, sudokuRegionType_t regionType, int regionNum, uint8_t* cellPos);

static uint8_t getMemberDigit(const solverCache_t* cache, const sudokuGrid_t* board, sudokuRegionType_t type, int region, int member);
static uint16_t getMemberNotes(const solverCache_t* cache, const sudokuGrid_t* board, sudokuRegionType_t type, int region, int member);

bool hintBufNextStep(uint8_t* buf, size_t maxlen, sudokuTechniqueType_t id);
bool hintBufSetDigit(uint8_t* buf, size_t maxlen, uint8_t digit, uint8_t pos);
bool hintBufSetMultiDigit(uint8_t* buf, size_t maxlen, uint8_t digit, int count, const uint8_t* pos);
bool hintBufAddNote(uint8_t* buf, size_t maxlen, uint16_t notes, uint8_t pos);
bool hintBufDelNote(uint8_t* buf, size_t maxlen, uint16_t notes, uint8_t pos);
bool hintBufSetMultiNote(uint8_t* buf, size_t maxlen, bool add, uint16_t notes, int count, const uint8_t* pos);
bool hintBufAddHighlight(uint8_t* buf, size_t maxlen, int digit, int box, int row, int col);

static const solverCallback_t findOrder[] =
{
    { .type = SCB_CELL, .region = findLastEmptyCell },
    { .type = SCB_REGION, .cell = findOnlyPossibility },
};

static const eliminateCb eliminateOrder[] =
{
    eliminatePointers,
    eliminateNakedPairs,
    eliminateHiddenPairs,
    eliminateNakedTriples,
    eliminateHiddenTriples,
};

bool sudokuNextMove2(solverCache_t* cache, sudokuOverlay_t* overlay, const sudokuGrid_t* board)
{
    size_t boardSize = board->size * board->size;

    bool giveUp = false;
    while (!giveUp)
    {
        solverCallback_t cb = findOrder[cache->searchIdx];
        if (cb.type == SCB_CELL)
        {
            for (int pos = 0; pos < boardSize; pos++)
            {
                if (cb.cell(cache, board, pos))
                {
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
                    return true;
                }
            }

            for (int row = 0; row < board->size; row++)
            {
                if (cb.region(cache, board, REGION_ROW, row))
                {
                    return true;
                }
            }

            for (int col = 0; col < board->size; col++)
            {
                if (cb.region(cache, board, REGION_COLUMN, col))
                {
                    return true;
                }
            }
        }

        if (++cache->searchIdx >= ARRAY_SIZE(findOrder))
        {
            cache->searchIdx = 0;
            while (true)
            {
                eliminateCb elimCb = eliminateOrder[cache->elimIdx];
                if (elimCb(cache, board))
                {
                    cache->elimIdx = 0;
                    break;
                }
                else
                {
                    if (++cache->elimIdx >= ARRAY_SIZE(eliminateOrder))
                    {
                        cache->elimIdx = 0;
                        giveUp = true;
                        break;
                    }
                }
            }
        }
    }

    return false;
}

static bool eliminateShadows(sudokuMoveDesc_t* desc, uint8_t* hintBuf, size_t maxlen, uint16_t* notes, const sudokuGrid_t* board)
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

static int eliminatePairsTriplesEtc(uint16_t* notes, uint16_t* digits, uint8_t* pos0, uint8_t* pos1, uint8_t* pos2, const sudokuGrid_t* board)
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
                    rFirstPos[i] = -1;
                    rSecondPos[i] = -1;
                    cFirstPos[i] = -1;
                    cSecondPos[i] = -1;
                }

                for (int i = 0; i < board->base; i++)
                {
                    boxFirstPos[i] = -1;
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
                                rFirstPos[row] = -2;
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
                                cFirstPos[col] = -2;
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
                                    boxFirstPos[box] = -2;
                                    boxSecondPos[box] = -2;
                                }
                            }
                        }
                        else if ((notes[pos] & bitA) || (notes[pos] & bitB))
                        {
                            // only one of the bits was set, won't work!
                            rFirstPos[row] = -2;
                            rSecondPos[row] = -2;
                            cFirstPos[col] = -2;
                            cSecondPos[col] = -2;

                            if (box != BOX_NONE)
                            {
                                boxFirstPos[box] = -2;
                                boxSecondPos[box] = -2;
                            }
                        }
                    }
                }
                else
                {
                    // no pairs found, second pass goes with triples
                    //for (int c = b + 1; c <= board->base; c++)
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
                            ESP_LOGI("Solver", "Eliminated pairs in row %d at %d and %d (%d/%d)", i, rFirstPos[i], rSecondPos[i], a, b);
                            *pos0 = rFirstPos[i];
                            *pos1 = rSecondPos[i];
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
                            ESP_LOGI("Solver", "Eliminated pairs in col %d at %d and %d (%d/%d)", i, cFirstPos[i], cSecondPos[i], a, b);
                            *pos0 = cFirstPos[i];
                            *pos1 = cSecondPos[i];
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
                            ESP_LOGI("Solver", "Eliminated pairs in box %d at %d and %d (%d/%d)", i, boxFirstPos[i], boxSecondPos[i], a, b);
                            *pos0 = boxFirstPos[i];
                            *pos1 = boxSecondPos[i];
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

static void applyCellElimination(const sudokuGrid_t* board, uint16_t* notes, int pos, uint16_t mask, int elimCount, const uint8_t* eliminations)
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

static void addElimination(sudokuMoveDesc_t* desc, uint8_t* hintBuf, size_t maxlen, int cellCount, uint16_t digits, sudokuRegionType_t regionType, int regionNum, uint8_t* cellPos)
{
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

/**
 * @brief Determine the next simplest move needed to solve the puzzle
 *
 * @param desc A pointer to a move description struct, which will be updated with the move's location
 * @param overlay A pointer to a grid-sized overlay array, where hint overlays will be added if non-NULL
 * @param notes A pointer to a grid-sized notes array, where hint notes will be added if non-NULL
 * @param board A pointer to the game board to be solved
 * @return true when a move was found and written to desc
 * @return false when no move was found because the board is in an illegal or unsolvable state
 */
bool sudokuNextMove(sudokuMoveDesc_t* desc, sudokuOverlay_t* overlay, const sudokuGrid_t* board)
{
    const size_t boardLen = board->size * board->size;
    uint16_t notes[boardLen];
    uint16_t prevNotes[boardLen];
    uint16_t rowNotes[board->size];
    uint16_t colNotes[board->size];
    uint16_t boxNotes[board->base];
    int overlayBox = -1;
    int overlayRow = -1;
    int overlayCol = -1;

    uint16_t eliminationDigits = 0;
    uint8_t eliminationCount = 0;
    uint8_t eliminationPos[3] = {0};

    uint8_t hintBuf[1024];
    size_t hintBufLen = sizeof(hintBuf);
    SET_LENGTH(hintBuf, 0);

#define ADD_ELIMINATIONS(regionType, regionNum) addElimination(desc, hintBuf, hintBufLen, eliminationCount, eliminationDigits, regionType, regionNum, eliminationPos)

    // 0. Copy the board notes
    memcpy(notes, board->notes, sizeof(uint16_t) * boardLen);
    // Also save a copy
    memcpy(prevNotes, notes, sizeof(uint16_t) * boardLen);

    // 0.5 Get the individual row/col/box notes we can use later
    sudokuGetIndividualNotes(rowNotes, colNotes, boxNotes, board, 0);

    bool giveUp = false;
    int stage = 0;
    while (!giveUp)
    {
        // (In order of digits)
        for (uint8_t digit = 1; digit <= board->base; digit++)
        {
            uint16_t bit = (1 << (digit - 1));

            // 1. Check for "only place this digit fits" within:
            //    a. Boxes
            // TODO this loop is less efficient, flip it so we check each cell once and do all the box calculation later
            for (int box = 0; box < board->base; box++)
            {
                // Only check boxes that don't already have this digit in them
                if (boxNotes[box] & bit)
                {
                    // Number of valid spots we've found so far
                    int count = 0;

                    // The position we found the last valid spot in
                    uint16_t pos = 0;

                    // This digit _could_ go in this box!
                    // So let's check how many places it could
                    for (int n = 0; n < boardLen; n++)
                    {
                        if (board->boxMap[n] == box && 0 == board->grid[n] && (notes[n] & bit))
                        {
                            // This is the box we are interested in, and this digit hasn't already been placed here
                            if (++count > 1)
                            {
                                // We've found a second possibility, skip
                                break;
                            }

                            // Save the position
                            pos = n;
                        }
                    }

                    if (count == 1)
                    {
                        hintBufSetDigit(hintBuf, hintBufLen, digit, pos);
                        hintBufAddHighlight(hintBuf, hintBufLen, digit, box, -1, -1);
                        hintToOverlay(overlay, board, -1, hintBuf, hintBufLen);
                        return true;

                        // Done! We found exactly one valid position for this digit
                        /*desc->pos = pos;
                        desc->digit = digit;
                        desc->message = "This is the only valid position for a%4$s %1$d within the box";
                        desc->detail = NULL;
                        //ADD_ELIMINATIONS(REGION_BOX, box);
                        overlayBox = box;
                        goto do_overlay;*/
                    }

                    // No valid position found, continue to the next box
                }
            }
        }

        for (uint8_t digit = 1; digit <= board->base; digit++)
        {
            uint16_t bit = (1 << (digit - 1));
            //    b. Rows
            for (int row = 0; row < board->size; row++)
            {
                if (rowNotes[row] & bit)
                {
                    int count = 0;
                    uint16_t pos = 0;

                    // Iterate over the row members
                    for (int n = 0; n < board->size; n++)
                    {
                        uint16_t curPos = row * board->size + n;
                        if (0 == board->grid[curPos] && (notes[curPos] & bit))
                        {
                            if (++count > 1)
                            {
                                break;
                            }

                            pos = curPos;
                        }
                    }

                    if (count == 1)
                    {
                        hintBufSetDigit(hintBuf, hintBufLen, digit, pos);
                        hintBufAddHighlight(hintBuf, hintBufLen, digit, -1, row, -1);
                        hintToOverlay(overlay, board, -1, hintBuf, hintBufLen);
                        return true;

                        desc->pos = pos;
                        desc->digit = digit;
                        desc->message = "This is the only valid position for a%4$s %1$d within row %2$d";
                        //ADD_ELIMINATIONS(REGION_ROW, row);
                        overlayRow = pos / board->size;
                        goto do_overlay;
                    }
                }
            }
        }

        for (uint8_t digit = 1; digit <= board->base; digit++)
        {
            uint16_t bit = (1 << (digit - 1));
            //    c. Columns
            for (int col = 0; col < board->size; col++)
            {
                if (colNotes[col] & bit)
                {
                    int count = 0;
                    uint16_t pos = 0;

                    // Iterate over the row members
                    for (int n = 0; n < board->size; n++)
                    {
                        uint16_t curPos = n * board->size + col;
                        if (0 == board->grid[curPos] && (notes[curPos] & bit))
                        {
                            // No digit in this cell, and the current digit is a possibility here
                            if (++count > 1)
                            {
                                break;
                            }

                            pos = curPos;
                        }
                    }

                    if (count == 1)
                    {
                        hintBufSetDigit(hintBuf, hintBufLen, digit, pos);
                        hintBufAddHighlight(hintBuf, hintBufLen, digit, -1, -1, col);
                        hintToOverlay(overlay, board, -1, hintBuf, hintBufLen);
                        return true;

                        /*desc->pos = pos;
                        desc->digit = digit;
                        desc->message = "This is the only valid position for a%4$s %1$d within column %3$d";
                        //ADD_ELIMINATIONS(REGION_COLUMN, col);
                        overlayCol = pos % board->size;
                        goto do_overlay;*/
                    }
                }
            }
        }

        // Now, we don't really care about which digit it is now, so...
        // 2. Check for "only this digit fits here" within the entire board
        for (int n = 0; n < boardLen; n++)
        {
            if (0 == board->grid[n] && notes[n] && 1 == __builtin_popcount(notes[n]))
            {
                // only one possibility!
                uint8_t digit = __builtin_ctz(notes[n]) + 1;
                hintBufSetDigit(hintBuf, hintBufLen, digit, n);
                hintBufAddHighlight(hintBuf, hintBufLen, -1, -1, n / board->size, n % board->size);
                hintToOverlay(overlay, board, -1, hintBuf, hintBufLen);
                return true;

                desc->message = "A%4$s %1$d is the only possible digit in this square";
                //ADD_ELIMINATIONS(REGION_NONE, 0);
                overlayRow = n / board->size;
                overlayCol = n % board->size;
                goto do_overlay;
            }
        }

        switch (stage)
        {
            case 0:
            {
                // TODO:
                // 3. Take into account 'shadow' effects to eliminate possibilities
                // Save the previous notes
                ESP_LOGI("Solver", "Didn't find any obvious moves, eliminating pointers...");
                memcpy(prevNotes, notes, sizeof(uint16_t) * boardLen);
                if (!eliminateShadows(desc, hintBuf, hintBufLen, notes, board))
                {
                    // No more shadows to eliminate, try the next stage
                    ESP_LOGI("Solver", "Couldn't eliminate any pointers, next stage...");
                    stage++;
                }

                // 4. Repeat step 1
                // 5. Repeat step 2
                break;
            }

            case 1:
            {
                memcpy(prevNotes, notes, sizeof(uint16_t) * boardLen);

                // TODO: running this a few times messes stuff up
                eliminationCount = eliminatePairsTriplesEtc(notes, &eliminationDigits, &eliminationPos[0], &eliminationPos[1], &eliminationPos[2], board);
                if (0 != eliminationCount)
                {
                    ESP_LOGI("Solver", "Eliminated a %s", (eliminationCount == 2) ? "pair" : "triple");
                    // Go back to the start now that we've eliminated something
                    stage = 0;
                    continue;
                }
                else
                {
                    ESP_LOGI("Solver", "eliminate pairs returned 0, giving up");
                    giveUp = true;
                }
                // 6. Check for naked pairs
                // 7. Check for naked triples
                break;
            }
        }
    }
    // N. Uhhh eventually do guess-and-check or whatever the fancy sudoku name for it is, that's gonna suck

    return false;

do_overlay:
    if (overlay)
    {
        sudokuOverlayShape_t* shape;
        if (overlayBox >= 0)
        {
            // Draw a colored border around this box
            for (int row = 0; row < board->size; row++)
            {
                for (int col = 0; col < board->size; col++)
                {
                    // The box of the square we're looking at
                    uint16_t thisBox = board->boxMap[row * board->size + col];

                    if (thisBox != overlayBox)
                    {
                        continue;
                    }

                    // north
                    if (row == 0 || board->boxMap[(row - 1) * board->size + col] != thisBox)
                    {
                        // Draw north border
                        shape = heap_caps_malloc(sizeof(sudokuOverlayShape_t), MALLOC_CAP_8BIT);
                        shape->type = OVERLAY_LINE;
                        shape->color = c050;
                        shape->tag = ST_HINT;
                        getOverlayPos(&shape->line.p1.x, &shape->line.p1.y, row, col, SUBPOS_NW);
                        getOverlayPos(&shape->line.p2.x, &shape->line.p2.y, row, col, SUBPOS_NE);
                        push(&overlay->shapes, shape);
                    }
                    // east
                    if (col == (board->size - 1) || board->boxMap[row * board->size + col + 1] != thisBox)
                    {
                        // Draw east border
                        shape = heap_caps_malloc(sizeof(sudokuOverlayShape_t), MALLOC_CAP_8BIT);
                        shape->type = OVERLAY_LINE;
                        shape->color = c050;
                        shape->tag = ST_HINT;
                        getOverlayPos(&shape->line.p1.x, &shape->line.p1.y, row, col, SUBPOS_NE);
                        getOverlayPos(&shape->line.p2.x, &shape->line.p2.y, row, col, SUBPOS_SE);
                        push(&overlay->shapes, shape);
                    }
                    // south
                    if (row == (board->size - 1) || board->boxMap[(row + 1) * board->size + col] != thisBox)
                    {
                        // Draw south border
                        shape = heap_caps_malloc(sizeof(sudokuOverlayShape_t), MALLOC_CAP_8BIT);
                        shape->type = OVERLAY_LINE;
                        shape->color = c050;
                        shape->tag = ST_HINT;
                        getOverlayPos(&shape->line.p1.x, &shape->line.p1.y, row, col, SUBPOS_SW);
                        getOverlayPos(&shape->line.p2.x, &shape->line.p2.y, row, col, SUBPOS_SE);
                        push(&overlay->shapes, shape);
                    }
                    // west
                    if (col == 0 || board->boxMap[row * board->size + col - 1] != thisBox)
                    {
                        // Draw west border
                        shape = heap_caps_malloc(sizeof(sudokuOverlayShape_t), MALLOC_CAP_8BIT);
                        shape->type = OVERLAY_LINE;
                        shape->color = c050;
                        shape->tag = ST_HINT;
                        getOverlayPos(&shape->line.p1.x, &shape->line.p1.y, row, col, SUBPOS_NW);
                        getOverlayPos(&shape->line.p2.x, &shape->line.p2.y, row, col, SUBPOS_SW);
                        push(&overlay->shapes, shape);
                    }
                }
            }
        }

        if (overlayRow >= 0 && overlayCol >= 0)
        {
            // box a single square
            shape = heap_caps_malloc(sizeof(sudokuOverlayShape_t), MALLOC_CAP_8BIT);
            shape->type = OVERLAY_RECT;
            shape->color = c050;
            shape->tag = ST_HINT;
            getOverlayPos(&shape->rectangle.pos.x, &shape->rectangle.pos.y, overlayRow, overlayCol, SUBPOS_NW);
            getOverlayPos(&shape->rectangle.width, &shape->rectangle.height, 1, 1, SUBPOS_NW);
            push(&overlay->shapes, shape);
        }
        else if (overlayRow >= 0)
        {
            shape = heap_caps_malloc(sizeof(sudokuOverlayShape_t), MALLOC_CAP_8BIT);
            shape->type = OVERLAY_RECT;
            shape->color = c050;
            shape->tag = ST_HINT;
            getOverlayPos(&shape->rectangle.pos.x, &shape->rectangle.pos.y, overlayRow, 0, SUBPOS_NW);
            getOverlayPos(&shape->rectangle.width, &shape->rectangle.height, 1, board->size, SUBPOS_NW);
            push(&overlay->shapes, shape);
        }
        else if (overlayCol >= 0)
        {
            shape = heap_caps_malloc(sizeof(sudokuOverlayShape_t), MALLOC_CAP_8BIT);
            shape->type = OVERLAY_RECT;
            shape->color = c050;
            shape->tag = ST_HINT;
            getOverlayPos(&shape->rectangle.pos.x, &shape->rectangle.pos.y, 0, overlayCol, SUBPOS_NW);
            getOverlayPos(&shape->rectangle.width, &shape->rectangle.height, board->size, 1, SUBPOS_NW);
            push(&overlay->shapes, shape);
        }

        int r = desc->pos / board->size;
        int c = desc->pos % board->size;

        shape = heap_caps_malloc(sizeof(sudokuOverlayShape_t), MALLOC_CAP_8BIT);
        shape->type = OVERLAY_CIRCLE;
        shape->color = c500;
        shape->tag = ST_HINT;
        shape->circle.radius = BOX_SIZE_SUBPOS / 2;
        getOverlayPos(&shape->circle.pos.x, &shape->circle.pos.y, r, c, SUBPOS_CENTER);
        push(&overlay->shapes, shape);

        shape = heap_caps_malloc(sizeof(sudokuOverlayShape_t), MALLOC_CAP_8BIT);
        shape->type = OVERLAY_DIGIT;
        shape->color = c005;
        shape->tag = ST_HINT;
        shape->digit.digit = desc->digit;
        shape->digit.pos.x = c * BOX_SIZE_SUBPOS;
        shape->digit.pos.y = r * BOX_SIZE_SUBPOS;
        getOverlayPos(&shape->digit.pos.x, &shape->digit.pos.y, r, c, SUBPOS_CENTER);
        push(&overlay->shapes, shape);

        for (int stepNum = 0; stepNum < desc->stepCount; stepNum++)
        {
            const sudokuMoveElimination_t* elim = &desc->eliminationSteps[stepNum];
            for (int elimNum = 0; elimNum < elim->eliminationCount; elimNum++)
            {
                shape = heap_caps_malloc(sizeof(sudokuOverlayShape_t), MALLOC_CAP_8BIT);
                shape->type = OVERLAY_NOTES_SHAPE;
                shape->color = c500;
                shape->tag = ST_HINT;
                int pos = elim->eliminations[elimNum];
                int elimR = pos / board->size;
                int elimC = pos % board->size;
                getOverlayPos(&shape->digit.pos.x, &shape->digit.pos.y, elimR, elimC, SUBPOS_CENTER);
                push(&overlay->shapes, shape);
            }
        }

        overlay->gridOpts[desc->pos] |= OVERLAY_SKIP;
    }

    return true;
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
                        applyCellElimination(board, board->notes, pos, step->eliminationMask, step->eliminationCount, step->eliminations);
                    }
                }
            }
            else if (step->eliminationRegionType == REGION_ROW)
            {
                for (int col = 0; col < board->size; col++)
                {
                    int pos = step->eliminationRegionNum * board->size + col;

                    applyCellElimination(board, board->notes, pos, step->eliminationMask, step->eliminationCount, step->eliminations);
                }
            }
            else if (step->eliminationRegionType == REGION_COLUMN)
            {
                for (int row = 0; row < board->size; row++)
                {
                    int pos = row * board->size + step->eliminationRegionNum;

                    applyCellElimination(board, board->notes, pos, step->eliminationMask, step->eliminationCount, step->eliminations);
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
        uint8_t op = header & OP_MASK_OP;
        uint8_t count = (header & OP_MASK_COUNT) >> OP_SHIFT_COUNT;

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
                        uint8_t r = pos / boardSize;
                        uint8_t c = pos % boardSize;

                        shape = heap_caps_malloc(sizeof(sudokuOverlayShape_t), MALLOC_CAP_8BIT);
                        shape->type = OVERLAY_DIGIT;
                        shape->color = c005;
                        shape->tag = ST_HINT;
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
                        shape = heap_caps_malloc(sizeof(sudokuOverlayShape_t), MALLOC_CAP_8BIT);
                        shape->type = OVERLAY_NOTES_SHAPE;
                        shape->color = color;
                        shape->tag = ST_HINT;
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
                uint8_t targetBox = hint[idx++];
                uint8_t targetRow = hint[idx++];
                uint8_t targetCol = hint[idx++];

                bool useDigit = (targetDigit != 0 && targetDigit <= game->base);
                bool useBox = (targetBox < game->base);
                bool useRow = (targetRow < game->size);
                bool useCol = (targetCol < game->size);

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
                    shape = heap_caps_malloc(sizeof(sudokuOverlayShape_t), MALLOC_CAP_8BIT);
                    shape->type = OVERLAY_RECT;
                    shape->color = c050;
                    shape->tag = ST_HINT;
                    getOverlayPos(&shape->rectangle.pos.x, &shape->rectangle.pos.y, targetRow, 0, SUBPOS_NW);
                    getOverlayPos(&shape->rectangle.width, &shape->rectangle.height, 1, game->size, SUBPOS_NW);
                    push(&overlay->shapes, shape);
                    break;
                }
                else if (useCol && !useDigit && !useBox && !useRow)
                {
                    // box the entire column
                    shape = heap_caps_malloc(sizeof(sudokuOverlayShape_t), MALLOC_CAP_8BIT);
                    shape->type = OVERLAY_RECT;
                    shape->color = c050;
                    shape->tag = ST_HINT;
                    getOverlayPos(&shape->rectangle.pos.x, &shape->rectangle.pos.y, 0, targetCol, SUBPOS_NW);
                    getOverlayPos(&shape->rectangle.width, &shape->rectangle.height, game->size, 1, SUBPOS_NW);
                    push(&overlay->shapes, shape);
                    break;
                }

                for (int pos = 0; pos < boardSize * boardSize; pos++)
                {
                    int row = pos / boardSize;
                    int col = pos % boardSize;
                    int box = game->boxMap[pos];
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
                    shape = heap_caps_malloc(sizeof(sudokuOverlayShape_t), MALLOC_CAP_8BIT);
                    shape->type = OVERLAY_RECT;
                    shape->color = c050;
                    shape->tag = ST_HINT;
                    getOverlayPos(&shape->rectangle.pos.x, &shape->rectangle.pos.y, row, col, SUBPOS_NW);
                    getOverlayPos(&shape->rectangle.width, &shape->rectangle.height, 1, 1, SUBPOS_NW);
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
    if (len + 2 > maxlen)
    {
        return false;
    }

    buf[len++] = OP_STEP;
    buf[len++] = (uint8_t)id;

    SET_LENGTH(buf, len);

    return true;
}

bool hintBufSetDigit(uint8_t* buf, size_t maxlen, uint8_t digit, uint8_t pos)
{
    if (maxlen < 4)
    {
        return false;
    }

    uint32_t len = GET_LENGTH(buf);
    if (len + 3 > maxlen)
    {
        return false;
    }

    buf[len++] = MAKE_OPCODE(OP_SET_DIGIT, 0);
    buf[len++] = digit;
    buf[len++] = pos;

    SET_LENGTH(buf, len);

    return true;
}

bool hintBufSetMultiDigit(uint8_t* buf, size_t maxlen, uint8_t digit, int count, const uint8_t* pos)
{
    if (maxlen < 4 || count < 1)
    {
        return false;
    }

    uint32_t len = GET_LENGTH(buf);
    if (len + 2 + count > maxlen)
    {
        return false;
    }

    buf[len++] = MAKE_OPCODE(OP_SET_DIGIT, count - 1);
    buf[len++] = digit;

    for (int  n = 0; n < count; n++)
    {
        buf[len++] = pos[n];
    }

    SET_LENGTH(buf, len);

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
    if (len + 3 + count > maxlen)
    {
        return false;
    }

    buf[len++] = MAKE_OPCODE(add ? OP_ADD_NOTE : OP_DEL_NOTE, count - 1);
    buf[len++] = (notes >> 8) & 0xFF;
    buf[len++] = notes & 0xFF;

    for (int n = 0; n < count; n++)
    {
        buf[len++] = pos[n];
    }

    SET_LENGTH(buf, len);

    return true;
}

bool hintBufAddHighlight(uint8_t* buf, size_t maxlen, int digit, int box, int row, int col)
{
    if (maxlen < 4)
    {
        return false;
    }

    uint32_t len = GET_LENGTH(buf);
    if (len + 5 > maxlen)
    {
        return false;
    }

    uint8_t digitVal = (digit <= 0) ? 255u : digit;
    uint8_t boxVal = (box < 0) ? 255u : box;
    uint8_t rowVal = (row < 0) ? 255u : row;
    uint8_t colVal = (col < 0) ? 255u : col;

    buf[len++] = MAKE_OPCODE(OP_HIGHLIGHT, 0);
    buf[len++] = digitVal;
    buf[len++] = boxVal;
    buf[len++] = rowVal;
    buf[len++] = colVal;

    SET_LENGTH(buf, len);

    return true;
}

bool initSolverCache(solverCache_t* cache, int size, int base)
{
    const size_t defaultLen = 256;
    uint16_t* notesAlloc = heap_caps_calloc(size * size + base + size * 2, sizeof(uint16_t), MALLOC_CAP_8BIT);
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

    cache->hintBuf = hintAlloc;
    cache->hintbufLen = defaultLen;

    cache->notes = notesAlloc;
    cache->boxNotes = notesAlloc + (size * size);
    cache->rowNotes = notesAlloc + (size * size) + base;
    cache->colNotes = notesAlloc + (size * size) + base + size;

    cache->boxMap = boxMapAlloc;

    cache->size = size;
    cache->base = base;

    cache->searchIdx = 0;
    cache->elimIdx = 0;
    cache->digit = 1;
    cache->pos = 0;

    return true;
}

void resetSolverCache(solverCache_t* cache)
{
    memset(cache->hintBuf, 0, cache->hintbufLen);
    memset(cache->notes, 0, sizeof(uint16_t) * cache->size * cache->size);
    memset(cache->boxNotes, 0, sizeof(uint16_t) * cache->base);
    memset(cache->rowNotes, 0, sizeof(uint16_t) * cache->size);
    memset(cache->colNotes, 0, sizeof(uint16_t) * cache->size);
    memset(cache->boxMap, 0, cache->base * cache->base);

    cache->searchIdx = 0;
    cache->elimIdx = 0;
    cache->digit = 1;
    cache->pos = 0;
}

void deinitSolverCache(solverCache_t* cache)
{
    if (cache->notes)
    {
        free(cache->notes);
        cache->notes = NULL;
        cache->boxNotes = NULL;
        cache->rowNotes = NULL;
        cache->colNotes = NULL;
    }

    if (cache->hintBuf)
    {
        free(cache->hintBuf);
        cache->hintBuf = NULL;
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

static uint8_t getMemberPos(const solverCache_t* cache, const sudokuGrid_t* board, sudokuRegionType_t type, int region, int member)
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
    }
}

static uint8_t getMemberDigit(const solverCache_t* cache, const sudokuGrid_t* board, sudokuRegionType_t type, int region, int member)
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
    }

    return 0;
}

static uint16_t getMemberNotes(const solverCache_t* cache, const sudokuGrid_t* board, sudokuRegionType_t type, int region, int member)
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
    }

    return 0;
}

static bool findLastEmptyCell(solverCache_t* cache, const sudokuGrid_t* board, sudokuRegionType_t type, int region)
{
    uint16_t* notes = NULL;
    uint8_t noteCount = 0;
    switch (type)
    {
        case REGION_BOX:
        {
            notes = cache->boxNotes;
            noteCount = cache->base;
            break;
        }

        case REGION_ROW:
        {
            notes = cache->rowNotes;
            noteCount = cache->size;
            break;
        }

        case REGION_COLUMN:
        {
            notes = cache->colNotes;
            noteCount = cache->size;
            break;
        }

        default:
        {
            return false;
        }
    }

    for (int n = 0; n < noteCount; n++)
    {
        // Only a single digit is possible in this region
        if (__builtin_popcount(notes[n]) == 1)
        {
            int missingDigit = __builtin_ctz(notes[n]) + 1;
            for (int member = 0; member < noteCount; member++)
            {
                if (0 == getMemberDigit(cache, board, type, region, member))
                {
                    uint8_t pos = getMemberPos(cache, board, type, region, member);
                    hintBufNextStep(cache->hintBuf, cache->hintbufLen, SINGLE);
                    hintBufSetDigit(cache->hintBuf, cache->hintbufLen, missingDigit, pos);
                    hintBufAddHighlight(cache->hintBuf, cache->hintbufLen, -1, -1, pos / cache->size, pos % cache->size);
                    return true;
                }
            }
        }
    }

    return false;
}

static bool findOnlyPossibility(solverCache_t* cache, const sudokuGrid_t* board, int pos)
{
    if (__builtin_popcount(board->notes[pos]) == 1)
    {
        int digit = __builtin_ctz(notes[n]) + 1;
        hintBufNextStep(cache->hintBuf, cache->hintbufLen, ONLY_POSSIBLE);
        hintBufSetDigit(cache->hintBuf, cache->hintbufLen, digit, pos);
        hintBufAddHighlight(cache->hintBuf, cache->hintbufLen, -1, -1, pos / cache->size, pos % cache->size);

        return true;
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
