//==============================================================================
// Includes
//==============================================================================

#include "sudoku_game.h"
#include "sudoku_data.h"
#include "sudoku_ui.h"

//==============================================================================
// Functions
//==============================================================================

bool initSudokuGame(sudokuGrid_t* game, int size, int base, sudokuMode_t mode)
{
    if (size < base)
    {
        ESP_LOGE("Swadgedoku", "%dx%d Grid not large enough for base %d", size, size, base);
        return false;
    }

    // Allocate all the things
    int totalSquares = size * size;

    uint8_t* grid = heap_caps_calloc(totalSquares, sizeof(uint8_t), MALLOC_CAP_8BIT);

    if (!grid)
    {
        return false;
    }

    sudokuFlag_t* flags = heap_caps_calloc(totalSquares, sizeof(sudokuFlag_t), MALLOC_CAP_8BIT);
    if (!flags)
    {
        heap_caps_free(grid);
        return false;
    }

    uint16_t* notes = heap_caps_calloc(totalSquares, sizeof(uint16_t), MALLOC_CAP_8BIT);
    if (!notes)
    {
        heap_caps_free(flags);
        heap_caps_free(grid);
        return false;
    }

    uint8_t* boxMap = heap_caps_calloc(totalSquares, sizeof(uint8_t), MALLOC_CAP_8BIT);
    if (!boxMap)
    {
        heap_caps_free(notes);
        heap_caps_free(flags);
        heap_caps_free(grid);
        return false;
    }

    game->grid   = grid;
    game->flags  = flags;
    game->notes  = notes;
    game->boxMap = boxMap;

    game->mode = mode;
    game->size = size;
    game->base = base;

    return true;
}

void deinitSudokuGame(sudokuGrid_t* game)
{
    if (game->boxMap != NULL)
    {
        heap_caps_free(game->boxMap);
        game->boxMap = NULL;
    }

    if (game->flags != NULL)
    {
        heap_caps_free(game->flags);
        game->flags = NULL;
    }

    if (game->notes != NULL)
    {
        heap_caps_free(game->notes);
        game->notes = NULL;
    }

    if (game->grid != NULL)
    {
        heap_caps_free(game->grid);
        game->grid = NULL;
    }

    game->mode = SM_CLASSIC;
    game->base = 0;
    game->size = 0;
}

bool setupSudokuGame(sudokuGrid_t* game, sudokuMode_t mode, int base, int size)
{
    switch (mode)
    {
        case SM_CLASSIC:
        case SM_JIGSAW:
        {
            if (size < base)
            {
                ESP_LOGE("Swadgedoku", "%dx%d Grid not large enough for base %d", size, size, base);
                return false;
            }

            // Allocate all the things
            int totalSquares = size * size;

            uint8_t* grid = heap_caps_calloc(totalSquares, sizeof(uint8_t), MALLOC_CAP_8BIT);

            if (!grid)
            {
                return false;
            }

            sudokuFlag_t* flags = heap_caps_calloc(totalSquares, sizeof(sudokuFlag_t), MALLOC_CAP_8BIT);
            if (!flags)
            {
                heap_caps_free(grid);
                return false;
            }

            uint16_t* notes = heap_caps_calloc(totalSquares, sizeof(uint16_t), MALLOC_CAP_8BIT);
            if (!notes)
            {
                heap_caps_free(flags);
                heap_caps_free(grid);
                return false;
            }

            uint8_t* boxMap = heap_caps_calloc(totalSquares, sizeof(uint8_t), MALLOC_CAP_8BIT);
            if (!boxMap)
            {
                heap_caps_free(notes);
                heap_caps_free(flags);
                heap_caps_free(grid);
                return false;
            }

            game->grid   = grid;
            game->flags  = flags;
            game->notes  = notes;
            game->boxMap = boxMap;

            game->mode = mode;
            game->size = size;
            game->base = base;

            // Now, define the boxes!

            // First, check if the base is a square.
            // It's much easier if it is...

            int baseRoot = 0;
            switch (base)
            {
                case 1:
                    baseRoot = 1;
                    break;
                case 4:
                    baseRoot = 2;
                    break;
                case 9:
                    baseRoot = 3;
                    break;
                case 16:
                    baseRoot = 4;
                    break;
                default:
                    break;
            }

            if (mode != SM_JIGSAW && baseRoot != 0)
            {
                // Setup square boxes!
                for (int box = 0; box < base; box++)
                {
                    for (int n = 0; n < base; n++)
                    {
                        int x                      = (box % baseRoot) * baseRoot + (n % baseRoot);
                        int y                      = (box / baseRoot) * baseRoot + (n / baseRoot);
                        game->boxMap[y * size + x] = box;
                    }
                }
            }
            else
            {
                uint8_t boxSquareCounts[game->base];
                uint16_t assignedSquareCount = 0;

                // uint8_t adjacencyCount[game->base][game->base];

                uint8_t boxXs[game->base][game->base];
                uint8_t boxYs[game->base][game->base];
                uint8_t boxCounts[game->base];

                memset(boxSquareCounts, 0, game->base * sizeof(uint8_t));
                // memset(adjacencyCount, 0, game->base * game->base * sizeof(uint8_t));
                memset(boxXs, 0, game->base * game->base * sizeof(uint8_t));
                memset(boxYs, 0, game->base * game->base * sizeof(uint8_t));
                memset(boxCounts, 0, game->base * sizeof(uint8_t));

                // Ok, here's how we're going to set up the non-square boxes:
                // - Move in a spiral, always assigning each square to a box as we encounter it
                // - This generates a spiral box pattern that is guaranteed to be contiguous, but is very boring
                // - Make random permutations to the box assignments such that they always remain valid

                // First clear all the boxes from the map for reasons
                for (int n = 0; n < game->size * game->size; n++)
                {
                    game->boxMap[n] = BOX_NONE;
                }

                int curBox = 0;

                // Move in a clockwise spiral from the top left around the board
                // 0123 right down left up
                int dir  = 0;
                int minX = 0;
                int minY = 0;
                int maxX = game->size - 1;
                int maxY = game->size - 1;
                for (int x = 0, y = 0;;)
                {
                    game->boxMap[y * size + x]         = curBox;
                    boxXs[curBox][boxCounts[curBox]]   = x;
                    boxYs[curBox][boxCounts[curBox]++] = y;

                    // Advance to the next box if we've assigned all its squares
                    if (0 == (++assignedSquareCount % game->base))
                    {
                        curBox++;
                    }

                    ////////////////////////////////////////////////////////
                    // The rest of the loop is just for the spiral pattern
                    ////////////////////////////////////////////////////////

                    bool changeDir = false;
                    switch (dir)
                    {
                        case 0:
                        {
                            // right
                            if (x == maxX)
                            {
                                minY++;
                                y++;
                                changeDir = true;
                            }
                            else
                            {
                                x++;
                            }
                            break;
                        }
                        case 1:
                        {
                            // down
                            if (y == maxY)
                            {
                                maxX--;
                                x--;
                                changeDir = true;
                            }
                            else
                            {
                                y++;
                            }
                            break;
                        }
                        case 2:
                        {
                            // left
                            if (x == minX)
                            {
                                maxY--;
                                y--;
                                changeDir = true;
                            }
                            else
                            {
                                x--;
                            }
                            break;
                        }
                        case 3:
                        {
                            // up
                            if (y == minY)
                            {
                                minX++;
                                x++;
                                changeDir = true;
                            }
                            else
                            {
                                y--;
                            }
                            break;
                        }
                    }

                    if (changeDir)
                    {
                        dir = (dir + 1) % 4;
                        if (minX > maxX || minY > maxY)
                        {
                            // We've reached the center and are trying to go beyond, exit the loop
                            break;
                        }
                    }
                }

                if (assignedSquareCount != game->base * game->base)
                {
                    ESP_LOGE("Swadgedoku", "Could not generated boxes for game of base %d", base);
                }

                int mutCount = 1; // esp_random() % (game->base * game->base / 4);
                for (int mut = 0; mut < mutCount; mut++)
                {
                    // Start randomly permuting the squares
                    uint8_t boxA = (esp_random() % game->base);
                    uint8_t boxB = boxA;

                    uint16_t touches[game->base];

                    int adjacentCount = 0;

                    int aIdxSel = -1;
                    int bIdxSel = -1;
                    do
                    {
                        boxB = esp_random() % (game->base - 1);
                        if (boxB >= boxA)
                        {
                            boxB++;
                        }

                        adjacentCount
                            = boxGetAdjacentSquares(touches, game, boxXs[boxA], boxYs[boxA], boxXs[boxB], boxYs[boxB]);
                    } while (adjacentCount < 2);

                    // Now select two indices
                    int selOne = esp_random() % adjacentCount;
                    int selTwo = esp_random() % (adjacentCount - 1);
                    if (selTwo >= selOne)
                        selTwo++;

                    ESP_LOGI("Swadgedoku", "We will swap touching squares #%d and #%d from box %" PRIu8 " and %" PRIu8,
                             selOne, selTwo, boxA, boxB);

                    if (aIdxSel >= 0 && bIdxSel >= 0)
                    {
                        // bool whichSwap = !(esp_random() % 2);
                        uint8_t* ax = &boxXs[boxA][aIdxSel];
                        uint8_t* ay = &boxYs[boxA][aIdxSel];
                        uint8_t* bx = &boxXs[boxB][bIdxSel];
                        uint8_t* by = &boxYs[boxB][bIdxSel];
                        // They touch!?
                        // Swap the actual box assignments
                        game->boxMap[game->size * *ay + *ax] = boxB;
                        game->boxMap[game->size * *by + *bx] = boxA;

                        ESP_LOGI("Swadgedoku",
                                 "Swapping [%" PRIu8 ",%" PRIu8 "] and [%" PRIu8 ",%" PRIu8 "] boxes %" PRIu8
                                 " -> %" PRIu8,
                                 *ax, *ay, *bx, *by, boxA, boxB);

                        // Swap the box mapping coordinates also
                        uint8_t tmp = *ax;
                        *ax         = *bx;
                        *bx         = tmp;

                        tmp = *ay;
                        *ay = *by;
                        *by = tmp;
                    }
                }
            }

            // Set the notes to all possible
            uint16_t allNotes = (1 << game->base) - 1;
            for (int i = 0; i < game->size * game->size; i++)
            {
                if (!((SF_VOID | SF_LOCKED) & game->flags[i]))
                {
                    game->notes[i] = allNotes;
                }
            }

            return true;
        }

        case SM_X_GRID:
        {
            return false;
        }

        default:
        {
            return false;
        }
    }
}

void setupSudokuPlayer(sudokuPlayer_t* player, const sudokuGrid_t* game)
{
    if (player->notes != NULL)
    {
        heap_caps_free(player->notes);
        player->notes = NULL;
    }

    // TODO: Have an init/deinit function for overlays
    if (player->overlay.gridOpts != NULL)
    {
        heap_caps_free(player->overlay.gridOpts);
        player->overlay.gridOpts = NULL;
    }

    player->cursorShape = NULL;

    sudokuOverlayShape_t* shape = NULL;
    while (NULL != (shape = pop(&player->overlay.shapes)))
    {
        heap_caps_free(shape);
    }

    player->notes            = heap_caps_calloc(game->size * game->size, sizeof(uint16_t), MALLOC_CAP_8BIT);
    player->overlay.gridOpts = heap_caps_calloc(game->size * game->size, sizeof(sudokuOverlayOpt_t), MALLOC_CAP_8BIT);

    player->cursorShape = heap_caps_calloc(1, sizeof(sudokuOverlayShape_t), MALLOC_CAP_8BIT);

    if (player->cursorShape)
    {
        player->cursorShape->tag   = ST_CURSOR;
        player->cursorShape->color = c505;
        /*player->cursorShape->type          = OVERLAY_CIRCLE;
        player->cursorShape->circle.pos.x  = player->curX;
        player->cursorShape->circle.pos.y  = player->curY;
        player->cursorShape->circle.radius = 1;*/
        player->cursorShape->type = OVERLAY_RECT;
        getOverlayPos(&player->cursorShape->rectangle.pos.x, &player->cursorShape->rectangle.pos.y, player->curY,
                      player->curX, SUBPOS_NW);
        player->cursorShape->rectangle.width  = BOX_SIZE_SUBPOS;
        player->cursorShape->rectangle.height = BOX_SIZE_SUBPOS;
        push(&player->overlay.shapes, player->cursorShape);
    }
}

void sudokuReevaluatePeers(uint16_t* notes, const sudokuGrid_t* game, int row, int col, int flags)
{
    uint16_t rowNotes[game->size];
    uint16_t colNotes[game->size];
    uint16_t boxNotes[game->base];

    // List of square coordinates for peers in the source box
    uint8_t sourceBoxRows[game->base];
    uint8_t sourceBoxCols[game->base];
    int boxCount = 0;

    // This means 'all values are possible in this row/cell'
    // We'll whittle it down from there
    const uint16_t allNotes = (1 << game->base) - 1;

    // Initialize
    for (int n = 0; n < game->size; n++)
    {
        rowNotes[n] = allNotes;
        colNotes[n] = allNotes;
    }

    for (int n = 0; n < game->base; n++)
    {
        boxNotes[n] = allNotes;
    }

    uint16_t sourceBox = game->boxMap[row * game->size + col];

    // First pass: construct row/column possibilities
    for (int r = 0; r < game->size; r++)
    {
        for (int c = 0; c < game->size; c++)
        {
            uint16_t box   = game->boxMap[r * game->size + c];
            uint16_t digit = game->grid[r * game->size + c];

            if (digit != 0)
            {
                uint16_t digitUnmask = ~(1 << (digit - 1));

                rowNotes[r] &= digitUnmask;
                colNotes[c] &= digitUnmask;

                if (BOX_NONE != box && box < game->base)
                {
                    boxNotes[box] &= digitUnmask;
                }
            }

            // We only care about the source box squares that are NOT already part of the row/col
            // So skip anything on the same row or column as the source square
            if (box == sourceBox && r != row && c != col)
            {
                sourceBoxRows[boxCount]   = r;
                sourceBoxCols[boxCount++] = c;
            }
        }
    }

    // Second pass: apply changes to notes
    // We don't need to do a full grid sweep though, just the peers (as the function name suggests)

    // First, just the column -- this will include the target square itself
    for (int r = 0; r < game->size; r++)
    {
        uint16_t box                = game->boxMap[r * game->size + col];
        uint16_t boxNote            = (box < game->base) ? boxNotes[box] : allNotes;
        notes[r * game->size + col] = (rowNotes[r] & colNotes[col] & boxNote);
        if (!game->grid[r * game->size + col] && !notes[r * game->size + col])
        {
            ESP_LOGW("Swadgedoku", "Cell r=%d, c=%d has no valid entries!", r, col);
        }
    }

    // Next, just the row, skipping the square we already did (the target square)
    for (int c = 0; c < game->size; c++)
    {
        // Skip the source column, it's already done by the previous loop
        if (c == col)
        {
            continue;
        }

        uint16_t box                = game->boxMap[row * game->size + c];
        uint16_t boxNote            = (box < game->base) ? boxNotes[box] : allNotes;
        notes[row * game->size + c] = (rowNotes[row] & colNotes[c] & boxNote);
        if (!game->grid[row * game->size + c] && !notes[row * game->size + c])
        {
            ESP_LOGW("Swadgedoku", "Cell r=%d, c=%d has no valid entries!", row, c);
        }
    }

    // Maybe redundant check since nothing should get added to sourceBox{rows,cols} but idk
    if (BOX_NONE != sourceBox)
    {
        uint16_t sourceBoxNote = boxNotes[sourceBox];
        for (int n = 0; n < boxCount; n++)
        {
            int r = sourceBoxRows[n];
            int c = sourceBoxCols[n];

            ESP_LOGD("Swadgedoku", "Box[r=%d][c=%d] == sourceBox (%" PRIu16 ")", r, c, sourceBox);
            notes[r * game->size + c] = (rowNotes[r] & colNotes[c] & sourceBoxNote);
            if (!game->grid[r * game->size + c] && !notes[r * game->size + c])
            {
                ESP_LOGW("Swadgedoku", "Cell r=%d, c=%d has no valid entries!", r, c);
            }
        }
    }
}

void sudokuGetIndividualNotes(uint16_t* rowNotes, uint16_t* colNotes, uint16_t* boxNotes, const sudokuGrid_t* game, int flags)
{
    // This means 'all values are possible in this row/cell'
    const uint16_t allNotes = (1 << game->base) - 1;

    // Initialize
    for (int n = 0; n < game->size; n++)
    {
        rowNotes[n] = allNotes;
        colNotes[n] = allNotes;
    }

    for (int n = 0; n < game->base; n++)
    {
        boxNotes[n] = allNotes;
    }

    //
    for (int row = 0; row < game->size; row++)
    {
        for (int col = 0; col < game->size; col++)
        {
            uint8_t box   = game->boxMap[row * game->size + col];
            uint8_t digit = game->grid[row * game->size + col];

            if (digit != 0)
            {
                uint16_t digitUnmask = ~(1 << (digit - 1));
                rowNotes[row] &= digitUnmask;
                colNotes[col] &= digitUnmask;

                if (BOX_NONE != box && box < game->base)
                {
                    boxNotes[box] &= digitUnmask;
                }
            }
        }
    }
}

void sudokuGetNotes(uint16_t* notes, const sudokuGrid_t* game, int flags)
{
    // Means all digits are possible
    const uint16_t allNotes = (1 << game->base) - 1;

    // Summaries of the possibilities for each row, box, and column
    // We will construct these from
    uint16_t rowNotes[game->size];
    uint16_t colNotes[game->size];
    uint16_t boxNotes[game->base];

    sudokuGetIndividualNotes(rowNotes, colNotes, boxNotes, game, flags);

    for (int row = 0; row < game->size; row++)
    {
        for (int col = 0; col < game->size; col++)
        {
            uint8_t box   = game->boxMap[row * game->size + col];
            uint8_t digit = game->grid[row * game->size + col];

            if (digit == 0)
            {
                uint16_t boxNote              = (box < game->base) ? boxNotes[box] : allNotes;
                notes[row * game->size + col] = (rowNotes[row] & colNotes[col] & boxNote);
            }
            else
            {
                notes[row * game->size + col] = 0;
            }
        }
    }
}

/**
 * @brief Uses the game grid and notes to place annotations on the board.
 *
 * This is responsible for deciding where to draw lines, how to highlight
 * cells, and any other automatically added things.
 *
 * Assumes that game->notes is up-to-date!
 *
 * @param overlay The overlay to update
 * @param player The player to use for preferences and selected digit or cursor position
 * @param game The game to read digits and notes from
 * @param settings Global sudoku settings to control which annotations are set and how
 */
void sudokuAnnotate(sudokuOverlay_t* overlay, const sudokuPlayer_t* player, const sudokuGrid_t* game,
                    const sudokuSettings_t* settings)
{
    // 1. Remove all the existing annotations placed by us
    node_t* next = NULL;
    for (node_t* node = overlay->shapes.first; node != NULL; node = next)
    {
        sudokuOverlayShape_t* shape = (sudokuOverlayShape_t*)node->val;
        // Save the next node pointer in case we remove the current one
        next = node->next;

        if (shape->tag == ST_ANNOTATE)
        {
            removeEntry(&overlay->shapes, node);
            heap_caps_free(shape);
        }
    }

    const int curRow      = player->curY;
    const int curCol      = player->curX;
    const int curBox      = game->boxMap[curRow * game->size + curCol];
    const int cursorDigit = game->grid[curRow * game->size + curCol];

    // This might need to be configurable... so let's make a big bunch of bools
    // Gently highlighting the selected row/col/box
    bool highlightCurRow = false;
    bool highlightCurCol = false;
    bool highlightCurBox = false;

    // Highlight the digit underneath the cursor, if any
    bool highlightCursorDigit = true;
    // Highlight the digit to be entered by the player with A
    bool highlightSelectedDigit = false;

    // Highlight the first-order possibilities for the digit under the cursor
    bool highlightCursorDigitLocations = true;
    // Highlight the first-order possibilities for the digit to be entered
    bool highlightSelectedDigitLocations = false;

    bool hLineThroughCursorDigits = false;
    bool vLineThroughCursorDigits = false;

    bool hLineThroughSelectedDigits = false;
    bool vLineThroughSelectedDigits = false;

    const sudokuOverlayOpt_t keepOverlay = 0;

    // Just make this oversized sometimes so it can be initialized, I don't wanna do 6 memsets
    uint16_t rowMasks[SUDOKU_MAX_BASE] = {0};
    uint16_t colMasks[SUDOKU_MAX_BASE] = {0};
    uint16_t boxMasks[SUDOKU_MAX_BASE] = {0};

    uint16_t rowErrs[SUDOKU_MAX_BASE] = {0};
    uint16_t colErrs[SUDOKU_MAX_BASE] = {0};
    uint16_t boxErrs[SUDOKU_MAX_BASE] = {0};

    // Get all row/col/box masks to check duplicates
    for (int n = 0; n < game->size * game->size; n++)
    {
        if (!(game->flags[n] & SF_VOID))
        {
            const int digit = game->grid[n];
            if (digit != 0)
            {
                const int r         = n / game->size;
                const int c         = n % game->size;
                const int box       = game->boxMap[n];
                const uint16_t bits = (1 << (digit - 1));

                if (rowMasks[r] & bits)
                {
                    rowErrs[r] |= bits;
                }

                if (colMasks[c] & bits)
                {
                    colErrs[c] |= bits;
                }

                rowMasks[r] |= bits;
                colMasks[c] |= bits;
                if (box < SUDOKU_MAX_BASE)
                {
                    if (boxMasks[box] & bits)
                    {
                        boxErrs[box] |= bits;
                    }
                    boxMasks[box] |= bits;
                }
            }
        }
    }

    // Do all the annotations now
    for (int n = 0; n < game->size * game->size; n++)
    {
        overlay->gridOpts[n] &= keepOverlay;

        if (!(game->flags[n] & SF_VOID))
        {
            const int r   = n / game->size;
            const int c   = n % game->size;
            const int box = game->boxMap[n];
            int digit     = game->grid[n];

            if (r == curRow)
            {
                if (highlightCurRow)
                {
                    overlay->gridOpts[n] |= OVERLAY_HIGHLIGHT_ROW;
                }
            }

            if (c == curCol)
            {
                if (highlightCurCol)
                {
                    overlay->gridOpts[n] |= OVERLAY_HIGHLIGHT_COL;
                }
            }

            if (box != BOX_NONE && box == curBox)
            {
                if (highlightCurBox)
                {
                    overlay->gridOpts[n] |= OVERLAY_HIGHLIGHT_BOX;
                }
            }

            if (r == curRow && c == curCol)
            {
                // idk? don't really care
            }

            if (highlightCursorDigit && cursorDigit && cursorDigit == digit)
            {
                overlay->gridOpts[n] |= OVERLAY_HIGHLIGHT_A;
            }
            else if (highlightSelectedDigit && player->selectedDigit && player->selectedDigit == digit)
            {
                overlay->gridOpts[n] |= OVERLAY_HIGHLIGHT_B;
            }

            uint16_t selBits    = player->selectedDigit ? (1 << (player->selectedDigit - 1)) : 0;
            uint16_t cursorBits = cursorDigit ? (1 << (cursorDigit - 1)) : 0;

            if (!digit && highlightCursorDigitLocations && cursorDigit && (game->notes[n] & cursorBits))
            {
                if (game->notes[n] == cursorBits && settings->highlightOnlyOptions)
                {
                    overlay->gridOpts[n] |= OVERLAY_CHECK;
                }
                else if (settings->highlightPossibilities)
                {
                    overlay->gridOpts[n] |= OVERLAY_QUESTION;
                }
            }
            else if (!digit && highlightSelectedDigitLocations && player->selectedDigit && (game->notes[n] & selBits))
            {
                // Use a ? if this is the only possibility
                if (game->notes[n] == selBits && settings->highlightOnlyOptions)
                {
                    overlay->gridOpts[n] |= OVERLAY_CHECK;
                }
                else if (settings->highlightPossibilities)
                {
                    overlay->gridOpts[n] |= OVERLAY_QUESTION;
                }
            }

            if (cursorDigit && cursorDigit == digit)
            {
                if (hLineThroughCursorDigits || vLineThroughCursorDigits)
                {
                    addCrosshairOverlay(overlay, r, c, game->size, hLineThroughCursorDigits, vLineThroughCursorDigits,
                                        ST_ANNOTATE);
                }
            }

            if (player->selectedDigit && player->selectedDigit == digit)
            {
                if (hLineThroughSelectedDigits || vLineThroughSelectedDigits)
                {
                    addCrosshairOverlay(overlay, r, c, game->size, hLineThroughSelectedDigits,
                                        vLineThroughSelectedDigits, ST_ANNOTATE);
                }
            }

            // Error checking
            if (digit != 0)
            {
                // So we know which digits have errors (duplicates) in any given
                // row, column, or box. Now we just highlight _all_ those cells!
                uint16_t bits = (1 << (digit - 1));
                if ((rowErrs[r] & bits) || (colErrs[c] & bits) || (box < SUDOKU_MAX_BASE && (boxErrs[box] & bits)))
                {
                    overlay->gridOpts[n] |= OVERLAY_ERROR;
                }
            }
        }
    }
}

/**
 * @brief Checks whether the puzzle is valid, in error, or complete. Returns 0 if valid
 * but incomplete, 1 if complete, and -1 if incorrect
 *
 * @param game The game to check
 * @return sudokuWinState The invalid, incomplete, or complete status of the game
 */
sudokuWinState_t swadgedokuCheckWin(const sudokuGrid_t* game)
{
    bool complete = true;

    uint16_t rowMasks[game->size];
    uint16_t colMasks[game->size];
    uint16_t boxMasks[game->base];

    memset(rowMasks, 0, sizeof(rowMasks));
    memset(colMasks, 0, sizeof(colMasks));
    memset(boxMasks, 0, sizeof(boxMasks));

    for (int n = 0; n < game->size * game->size; n++)
    {
        if (!(game->flags[n] & SF_VOID))
        {
            if (game->grid[n])
            {
                uint16_t valBits  = (1 << (game->grid[n] - 1));
                const uint8_t box = game->boxMap[n];
                const int r       = n / game->size;
                const int c       = n % game->size;

                if ((rowMasks[r] & valBits) || (colMasks[c] & valBits)
                    || (box != BOX_NONE && (boxMasks[box] & valBits)))
                {
                    // Duplicate!
                    return SUDOKU_INVALID;
                }

                rowMasks[r] |= valBits;
                colMasks[c] |= valBits;
                if (box != BOX_NONE)
                {
                    boxMasks[box] |= valBits;
                }
            }
            else
            {
                complete = false;
            }
        }
    }

    // If there was a duplicate anywhere we would have returned.
    // Now we're just checking for a full-complete.
    const uint16_t allBits = (1 << game->base) - 1;
    for (int i = 0; i < game->size; i++)
    {
        if (rowMasks[i] != allBits || colMasks[i] != allBits || (i < game->base && boxMasks[i] != allBits))
        {
            // Game is incomplete
            return 0;
        }
    }

    // No errors detected!
    return complete ? SUDOKU_COMPLETE : SUDOKU_INCOMPLETE;
}

int swadgedokuRand(int* seed)
{
    // Adapted from https://stackoverflow.com/a/1280765
    int val = *seed;
    val *= 0x343fd;
    val += 0x269EC3;
    *seed = val;
    return (val >> 0x10) & 0x7FFF;
}

/**
 * @brief Gets the list and count of adjacent squares in two boxes
 *
 * The 'indices' value will be constructed as follows:
 * - Each item corresponds to the square in box A at the same index
 * - The value represents a bitmask of which squares in box B touch the box A square at that index
 * - So, if square 0 of box A touches square 2 of box B, then 0 != (indices[0] & (1 << 2))
 * - And an index value of 0 means that square of box A does not touch any squares of box B
 *
 * @param[out] indices A list where the adjacent squares are designated
 * @param game
 * @param aXs
 * @param aYs
 * @param bXs
 * @param bYs
 * @return int The number of adjacent squares
 */
int boxGetAdjacentSquares(uint16_t* indices, const sudokuGrid_t* game, uint8_t* aXs, uint8_t* aYs, uint8_t* bXs,
                          uint8_t* bYs)
{
    int count = 0;

    for (int aIdx = 0; aIdx < game->base; aIdx++)
    {
        int touches   = 0;
        indices[aIdx] = 0;

        for (int bIdx = 0; bIdx < game->base; bIdx++)
        {
            uint8_t* ax = &aXs[aIdx];
            uint8_t* ay = &aYs[aIdx];
            uint8_t* bx = &bXs[bIdx];
            uint8_t* by = &bYs[bIdx];

            if (squaresTouch(*ax, *ay, *bx, *by))
            {
                touches++;
                indices[aIdx] |= (1 << bIdx);
            }
        }

        // We're only counting the number of **box A** squares that touch any box B square
        // NOT the number of times those squares touch box B squares
        // We don't care if one square touches multiple other squares here, just count each once
        if (touches)
        {
            count++;
        }
    }

    return count;
}

bool squaresTouch(uint8_t ax, uint8_t ay, uint8_t bx, uint8_t by)
{
    return ((ay == by) && ((ax + 1 == bx) || (bx + 1 == ax))) || ((ax == bx) && ((ay + 1 == by) || (by + 1 == ay)));
}

bool setDigit(sudokuGrid_t* game, uint8_t number, uint8_t x, uint8_t y)
{
    bool ok = true;
    if (x < game->size && y < game->size)
    {
        if (number <= game->base)
        {
            if (!((SF_VOID | SF_LOCKED) & game->flags[y * game->size + x]))
            {
                if (number != 0)
                {
                    game->notes[y * game->size + x] = 0;
                }
                game->grid[y * game->size + x] = number;
                sudokuReevaluatePeers(game->notes, game, y, x, 0);
            }
        }
    }
    return ok;
}

void clearOverlayOpts(sudokuOverlay_t* overlay, const sudokuGrid_t* game, sudokuOverlayOpt_t optMask)
{
    for (int n = 0; n < game->size * game->size; n++)
    {
        overlay->gridOpts[n] &= ~optMask;
    }
}
