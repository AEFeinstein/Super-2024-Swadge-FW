#include "sudoku_solver.h"
#include "sudoku_game.h"
#include "sudoku_ui.h"

static void eliminateShadows(uint16_t* notes, const sudokuGrid_t* board)
{
    // TODO
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
    uint16_t rowNotes[board->size];
    uint16_t colNotes[board->size];
    uint16_t boxNotes[board->base];
    int overlayBox = -1;

    // 0. Copy the board notes
    memcpy(notes, board->notes, sizeof(uint16_t) * boardLen);

    // 0.5 Get the individual row/col/box notes we can use later
    sudokuGetIndividualNotes(rowNotes, colNotes, boxNotes, board, 0);

    for (int stage = 0; stage < 1; stage++)
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
                    ESP_LOGI("Solver", "No %" PRIu8 " in box %d yet", digit, box);
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
                        // Done! We found exactly one valid position for this digit
                        desc->pos = pos;
                        desc->digit = digit;
                        desc->message = "This is the only valid position for a %1$d within the box";
                        overlayBox = box;
                        goto do_overlay;
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
                        if (0 == board->grid[curPos] && (notes[n] & bit))
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
                        desc->pos = pos;
                        desc->digit = digit;
                        desc->message = "This is the only valid position for a %1$d within row %2$d";
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
                        if (0 == board->grid[curPos] && (notes[n] & bit))
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
                        desc->pos = pos;
                        desc->digit = digit;
                        desc->message = "This is the only valid position for a %1$d within column %3$d";
                        goto do_overlay;
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
                desc->pos = n;
                desc->digit = __builtin_ctz(notes[n]) + 1;
                desc->message = "A %1$d is the only possible digit in this square";
                goto do_overlay;
            }
        }

        switch (stage)
        {
            case 0:
            {
                // TODO:
                // 3. Take into account 'shadow' effects to eliminate possibilities

                // 4. Repeat step 1
                // 5. Repeat step 2
                break;
            }

            case 1:
            {
                // 6. Check for naked pairs
                // 7. Check for naked triples
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
        getOverlayPos(&shape->circle.pos.x, &shape->circle.pos.y, r, c, SUBPOS_CENTER);
        push(&overlay->shapes, shape);

        overlay->gridOpts[desc->pos] |= OVERLAY_SKIP;
    }

    return true;
}