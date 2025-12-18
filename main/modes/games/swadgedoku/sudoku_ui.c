//==============================================================================
// Includes
//==============================================================================

#include "sudoku_ui.h"

//==============================================================================
// Functions
//==============================================================================

int swadgedokuGetSquareSize(const sudokuGrid_t* game)
{
    // Total space around the grid
    int gridMargin = GRID_MARGIN;
    return (TFT_HEIGHT - gridMargin) / game->size;
}

void swadgedokuGetGridPos(int* gridX, int* gridY, const sudokuGrid_t* game)
{
    int maxSquareSize = swadgedokuGetSquareSize(game);

    // Total size of the grid (add 1px for border)
    int gridSize = game->size * maxSquareSize;

    // Center the grid vertically
    if (gridY)
    {
        *gridY = (TFT_HEIGHT - gridSize) / 2;
    }

    // Align the grid to the left to leave some space to the right for UI
    if (gridX)
    {
        *gridX = (TFT_WIDTH - gridSize) / 2;
    }
}

void swadgedokuDrawGame(const sudokuGrid_t* game, const uint16_t* notes, const sudokuOverlay_t* overlay,
                        const sudokuTheme_t* theme, const sudokuDrawContext_t* context,
                        sudokuShapeTag_t tagMask, sudokuOverlayOpt_t overlayMask)
{
    // Max size of individual square
    int maxSquareSize = swadgedokuGetSquareSize(game);

    // Total size of the grid (add 1px for border)
    int gridSize = game->size * maxSquareSize;

    // Center the grid vertically
    int gridY = (TFT_HEIGHT - gridSize) / 2;

    // Align the grid to the left to leave some space to the right for UI
    int gridX = (TFT_WIDTH - gridSize) / 2;

    paletteColor_t voidColor = theme->voidColor;

    // Efficiently fill in the edges of the screen, not covered by the grid
    if (gridY > 0)
    {
        // Fill top
        fillDisplayArea(0, 0, TFT_WIDTH, gridY, voidColor);
    }

    if (gridY + gridSize < TFT_HEIGHT)
    {
        // Fill bottom
        fillDisplayArea(0, gridY + gridSize, TFT_WIDTH, TFT_HEIGHT, voidColor);
    }

    if (gridX > 0)
    {
        // Fill left
        fillDisplayArea(0, gridY, gridX, gridY + gridSize, voidColor);
    }

    if (gridX + gridSize < TFT_WIDTH)
    {
        // Fill right
        fillDisplayArea(gridX + gridSize, gridY, TFT_WIDTH, gridY + gridSize, voidColor);
    }

    // Draw border around the grid
    drawRect(gridX, gridY, gridX + gridSize + 1, gridY + gridSize + 1, theme->borderColor);

    // Draw lines between the columns
    for (int col = 1; col < game->base; col++)
    {
        drawLineFast(gridX + col * maxSquareSize, gridY + 1, gridX + col * maxSquareSize, gridY + gridSize - 1,
                     theme->gridColor);
    }

    // Draw lines between the rows
    for (int row = 1; row < game->base; row++)
    {
        drawLineFast(gridX + 1, gridY + row * maxSquareSize, gridX + gridSize - 1, gridY + row * maxSquareSize,
                     theme->gridColor);
    }

    // Draw extra borders around the boxes and fill in the background
    for (int r = 0; r < game->size; r++)
    {
        for (int c = 0; c < game->size; c++)
        {
            int x = gridX + c * maxSquareSize;
            int y = gridY + r * maxSquareSize;

            paletteColor_t fillColor = theme->fillColor;

            sudokuOverlayOpt_t opts = OVERLAY_NONE;
            sudokuFlag_t flags      = game->flags[r * game->size + c];

            // For some kinds of overlays, we need to skip drawing the actual digit/notes
            bool skipSquare = false;

            if (flags & SF_VOID)
            {
                fillColor = voidColor;
            }
            else if (overlay)
            {
                opts = overlay->gridOpts[r * game->size + c] & overlayMask;

                if (opts & OVERLAY_SKIP)
                {
                    skipSquare = true;
                }

                if (opts & OVERLAY_ERROR)
                {
                    fillColor = c544;
                }
                else if (opts & OVERLAY_HIGHLIGHT_A)
                {
                    // Cyan
                    fillColor = c044;
                }
                else if (opts & OVERLAY_HIGHLIGHT_B)
                {
                    // Yellow-green
                    fillColor = c450;
                }
                else if (opts & OVERLAY_HIGHLIGHT_C)
                {
                    // Orangey
                    fillColor = c530;
                }
                else if (opts & OVERLAY_HIGHLIGHT_D)
                {
                    // Purpley
                    fillColor = c503;
                }
                else
                {
                    int red = 0;
                    int grn = 0;
                    int blu = 0;

                    if (opts & OVERLAY_HIGHLIGHT_ROW)
                    {
                        // Cyan
                        grn += 2;
                        blu += 2;
                    }

                    if (opts & OVERLAY_HIGHLIGHT_COL)
                    {
                        // Blue
                        blu += 3;
                    }

                    if (opts & OVERLAY_HIGHLIGHT_BOX)
                    {
                        // Yellow
                        red += 3;
                        grn += 3;
                    }

                    if (opts & OVERLAY_ERROR)
                    {
                        red += 2;
                    }

                    if (red || grn || blu)
                    {
                        fillColor = red * 36 + grn * 6 + blu;
                    }
                }
            }

            fillDisplayArea(x + 1, y + 1, x + maxSquareSize, y + maxSquareSize, fillColor);

            // For each of the four cardinal directions,
            // that side gets an extra border if either:
            //  - That side has no neighbor cell (it's at the edge of the grid), or
            //  - That side has a neighbor cell with a different grid than this
            // This method should always draw the border properly even for non-rectangular boxes

            // The box of the square we're looking at
            uint16_t thisBox = game->boxMap[r * game->size + c];

            // north
            if (r == 0 || game->boxMap[(r - 1) * game->size + c] != thisBox)
            {
                // Draw north border
                drawLineFast(x + 1, y + 1, x + maxSquareSize - 1, y + 1, theme->borderColor);
            }
            // east
            if (c == (game->size - 1) || game->boxMap[r * game->size + c + 1] != thisBox)
            {
                // Draw east border
                drawLineFast(x + maxSquareSize - 1, y + 1, x + maxSquareSize - 1, y + maxSquareSize - 1,
                             theme->borderColor);
            }
            // south
            if (r == (game->size - 1) || game->boxMap[(r + 1) * game->size + c] != thisBox)
            {
                // Draw south border
                drawLineFast(x + 1, y + maxSquareSize - 1, x + maxSquareSize - 1, y + maxSquareSize - 1,
                             theme->borderColor);
            }
            // west
            if (c == 0 || game->boxMap[r * game->size + c - 1] != thisBox)
            {
                // Draw west border
                drawLineFast(x + 1, y + 1, x + 1, y + maxSquareSize - 1, theme->borderColor);
            }

            uint16_t squareVal = game->grid[r * game->size + c];

            // NOW! Draw the number, or the notes
            if (NULL != notes && 0 == squareVal && !skipSquare)
            {
                // Draw notes
                uint16_t squareNote = notes[r * game->size + c];
                for (int n = 0; n < game->base; n++)
                {
                    if (squareNote & (1 << n))
                    {
                        char buf[16];
                        snprintf(buf, sizeof(buf), "%X", n + 1);

                        // TODO center?
                        // int charW = textWidth(&context->noteFont, buf);

                        int baseRoot = 3;
                        switch (game->base)
                        {
                            case 1:
                                baseRoot = 1;
                                break;

                            case 2:
                            case 3:
                            case 4:
                                baseRoot = 2;
                                break;

                            case 10:
                            case 11:
                            case 12:
                            case 13:
                            case 14:
                            case 15:
                            case 16:
                                baseRoot = 4;
                                break;
                            default:
                                break;
                        }

                        int miniSquareSize = maxSquareSize / baseRoot;

                        int noteX = x + (n % baseRoot) * maxSquareSize / baseRoot
                                    + (miniSquareSize - textWidth(&context->noteFont, buf)) / 2 + 1;
                        int noteY = y + (n / baseRoot) * maxSquareSize / baseRoot
                                    + (miniSquareSize - context->noteFont.height) / 2 + 2;

                        drawText(&context->noteFont, theme->inkColor, buf, noteX, noteY);
                    }
                }
            }
            else if (0 != squareVal && !skipSquare)
            {
                // Draw number
                char buf[16];
                snprintf(buf, sizeof(buf), "%X", squareVal);

                int textX = x + (maxSquareSize - textWidth(&context->gridFont, buf)) / 2;
                int textY = y + (maxSquareSize - context->gridFont.height) / 2;

                paletteColor_t color = (flags & SF_LOCKED) ? theme->inkColor : theme->pencilColor;
                if (overlay)
                {
                    if (opts & OVERLAY_ERROR && !(game->flags[r * game->size + c] & SF_LOCKED))
                    {
                        // Color the text red in error (but not if it's a given value)
                        color = c500;
                    }
                    else if (opts & OVERLAY_CHECK)
                    {
                        color = c050;
                    }
                }

                drawText(&context->gridFont, color, buf, textX, textY);
            }

            // Check the overlay again, for more info
            if (overlay)
            {
                const char* overlayText   = NULL;
                paletteColor_t overlayCol = c000;

                if (opts & OVERLAY_NOTES)
                {
                    // TODO function-ify the notes draw-er and put it here
                }
                if (opts & OVERLAY_CHECK)
                {
                    // TODO: Draw a checkmark
                    overlayText = "+";
                    overlayCol  = c041;
                }
                if (opts & OVERLAY_QUESTION)
                {
                    overlayText = "?";
                    overlayCol  = c224;
                }
                if (opts & OVERLAY_CROSS_OUT)
                {
                    overlayText = "X";
                }

                if (overlayText)
                {
                    int textX = x + (maxSquareSize - textWidth(&context->gridFont, overlayText)) / 2;
                    int textY = y + (maxSquareSize - context->gridFont.height) / 2;
                    drawText(&context->gridFont, overlayCol, overlayText, textX, textY);
                }
            }
        }
    }

    if (overlay)
    {
        for (node_t* node = overlay->shapes.first; node != NULL; node = node->next)
        {
            const sudokuOverlayShape_t* shape = (sudokuOverlayShape_t*)node->val;

            if (!(tagMask & shape->tag))
            {
                // Skip tags not in the mask
                continue;
            }

            switch (shape->type)
            {
                case OVERLAY_RECT:
                {
                    int16_t x0, y0, x1, y1;
                    getRealOverlayPos(&x0, &y0, gridX, gridY, maxSquareSize, shape->rectangle.pos.x,
                                      shape->rectangle.pos.y);
                    getRealOverlayPos(&x1, &y1, gridX, gridY, maxSquareSize,
                                      shape->rectangle.pos.x + shape->rectangle.width,
                                      shape->rectangle.pos.y + shape->rectangle.height);
                    drawRect(x0, y0, x1, y1, shape->color);
                    // make it thicker
                    drawRect(x0 + 1, y0 + 1, x1 - 1, y1 - 1, shape->color);
                    break;
                }

                case OVERLAY_CIRCLE:
                {
                    int16_t x, y;
                    getRealOverlayPos(&x, &y, gridX, gridY, maxSquareSize, shape->circle.pos.x, shape->circle.pos.y);
                    drawCircle(x, y, shape->circle.radius * maxSquareSize / BOX_SIZE_SUBPOS, shape->color);
                    break;
                }

                case OVERLAY_LINE:
                {
                    int16_t x0, y0, x1, y1;
                    getRealOverlayPos(&x0, &y0, gridX, gridY, maxSquareSize, shape->line.p1.x, shape->line.p1.y);
                    getRealOverlayPos(&x1, &y1, gridX, gridY, maxSquareSize, shape->line.p2.x, shape->line.p2.y);
                    drawLineFast(x0, y0, x1, y1, shape->color);
                    break;
                }

                case OVERLAY_ARROW:
                {
                    int16_t x0, y0, x1, y1;

                    getRealOverlayPos(&x0, &y0, gridX, gridY, maxSquareSize, shape->arrow.tip.x, shape->arrow.tip.y);
                    getRealOverlayPos(&x1, &y1, gridX, gridY, maxSquareSize, shape->arrow.base.x, shape->arrow.base.y);

                    drawLineFast(x0, y0, x1, y1, shape->color);

                    getRealOverlayPos(&x1, &y1, gridX, gridY, maxSquareSize, shape->arrow.wing1.x,
                                      shape->arrow.wing1.y);
                    drawLineFast(x0, y0, x1, y1, shape->color);

                    getRealOverlayPos(&x1, &y1, gridX, gridY, maxSquareSize, shape->arrow.wing2.x,
                                      shape->arrow.wing2.y);
                    drawLineFast(x0, y0, x1, y1, shape->color);
                    break;
                }

                case OVERLAY_TEXT:
                {
                    int16_t x, y;

                    getRealOverlayPos(&x, &y, gridX, gridY, maxSquareSize, shape->text.pos.x, shape->text.pos.y);
                    int w = textWidth(&context->uiFont, shape->text.val);

                    if (shape->text.center)
                    {
                        x -= w / 2;
                    }

                    drawText(&context->uiFont, shape->color, shape->text.val, x, y);
                    break;
                }

                case OVERLAY_DIGIT:
                {
                    int c = shape->digit.pos.x / BOX_SIZE_SUBPOS;
                    int r = shape->digit.pos.y / BOX_SIZE_SUBPOS;

                    int x = gridX + c * maxSquareSize;
                    int y = gridY + r * maxSquareSize;

                    // Draw number
                    char buf[16];
                    if (shape->digit.digit)
                    {
                        snprintf(buf, sizeof(buf), "%X", shape->digit.digit);
                    }
                    else
                    {
                        buf[0] = 'X';
                        buf[1] = '\0';
                    }

                    int textX = x + (maxSquareSize - textWidth(&context->gridFont, buf)) / 2;
                    int textY = y + (maxSquareSize - context->gridFont.height) / 2;

                    drawText(&context->gridFont, shape->color, buf, textX, textY);
                    break;
                }

                case OVERLAY_NOTES_SHAPE:
                {
                    char buf[16];
                    uint16_t squareNote = shape->notes.notes;
                    int baseRoot = 3;
                    switch (game->base)
                    {
                        case 1:
                            baseRoot = 1;
                            break;

                        case 2:
                        case 3:
                        case 4:
                            baseRoot = 2;
                            break;

                        case 10:
                        case 11:
                        case 12:
                        case 13:
                        case 14:
                        case 15:
                        case 16:
                            baseRoot = 4;
                            break;
                        default:
                            break;
                    }
                    int miniSquareSize = maxSquareSize / baseRoot;

                    int x = gridX + game->size * maxSquareSize;
                    int y = gridY + game->size * maxSquareSize;

                    for (int n = 0; n < game->base; n++)
                    {
                        if (squareNote & (1 << n))
                        {
                            snprintf(buf, sizeof(buf), "%X", n + 1);

                            // TODO center?
                            // int charW = textWidth(&context->noteFont, buf);

                            int noteX = x + (n % baseRoot) * maxSquareSize / baseRoot
                                        + (miniSquareSize - textWidth(&context->noteFont, buf)) / 2 + 1;
                            int noteY = y + (n / baseRoot) * maxSquareSize / baseRoot
                                        + (miniSquareSize - context->noteFont.height) / 2 + 2;

                            drawText(&context->noteFont, shape->color, buf, noteX, noteY);
                        }
                    }
                    break;
                }
            }
        }
    }
}

// overlays are subdivided into 12 squares (13) so we can position stuff more clearly but still not deal with fixed
// positioning
/**
 * @brief Converts from row, column, and subposition to
 *
 * @param x
 * @param y
 * @param r
 * @param c
 * @param subpos
 */
void getOverlayPos(int32_t* x, int32_t* y, int r, int c, sudokuSubpos_t subpos)
{
    *x = c * BOX_SIZE_SUBPOS + ((int)subpos % BOX_SIZE_SUBPOS);
    *y = r * BOX_SIZE_SUBPOS + ((int)subpos / BOX_SIZE_SUBPOS);
}

/**
 * @brief Calculates the absolute position **in pixels** given a position within the grid **in sub-position
 * coordinates**
 *
 * @param x A pointer where the computed x value will be written
 * @param y A pointer where the computed y value will be written
 * @param gridSize The grid size, in pixels
 * @param gridX The X-location of the grid, in pixels
 * @param gridY The Y-location of the grid, in pixels
 * @param squareSize The size of a sudoku square, in pixels
 * @param xSubPos The X position within the grid, in 13ths of a square
 * @param ySubPos The Y position within the grid, in 13ths of a square
 */
void getRealOverlayPos(int16_t* x, int16_t* y, int gridX, int gridY, int squareSize, int xSubPos, int ySubPos)
{
    *x = gridX + (xSubPos * squareSize) / BOX_SIZE_SUBPOS;
    *y = gridY + (ySubPos * squareSize) / BOX_SIZE_SUBPOS;
}

void addCrosshairOverlay(sudokuOverlay_t* overlay, int r, int c, int gridSize, bool drawH, bool drawV,
                         sudokuShapeTag_t tag)
{
    // add a circle
    sudokuOverlayShape_t* circle = heap_caps_malloc(sizeof(sudokuOverlayShape_t), MALLOC_CAP_8BIT);
    if (circle)
    {
        circle->type  = OVERLAY_CIRCLE;
        circle->color = c005;
        circle->tag   = tag;
        getOverlayPos(&circle->circle.pos.x, &circle->circle.pos.y, r, c, SUBPOS_CENTER);
        circle->circle.radius = BOX_SIZE_SUBPOS * 3 / 5;

        push(&overlay->shapes, circle);
    }

    // add a left line
    if (c > 0)
    {
        sudokuOverlayShape_t* leftLine = heap_caps_malloc(sizeof(sudokuOverlayShape_t), MALLOC_CAP_8BIT);

        if (leftLine)
        {
            leftLine->type  = OVERLAY_LINE;
            leftLine->color = c005;
            leftLine->tag   = tag;
            getOverlayPos(&leftLine->line.p1.x, &leftLine->line.p1.y, r, 0, SUBPOS_W);
            getOverlayPos(&leftLine->line.p2.x, &leftLine->line.p2.y, r, c, SUBPOS_E);

            push(&overlay->shapes, leftLine);
        }
    }

    // add a right line
    if (c < gridSize - 1)
    {
        sudokuOverlayShape_t* rightLine = heap_caps_malloc(sizeof(sudokuOverlayShape_t), MALLOC_CAP_8BIT);

        if (rightLine)
        {
            rightLine->type  = OVERLAY_LINE;
            rightLine->color = c005;
            rightLine->tag   = tag;
            getOverlayPos(&rightLine->line.p1.x, &rightLine->line.p1.y, r, c + 1, SUBPOS_E - 3);
            getOverlayPos(&rightLine->line.p2.x, &rightLine->line.p2.y, r, gridSize - 1, SUBPOS_E - 3);

            push(&overlay->shapes, rightLine);
        }
    }

    // add a top line
    if (r > 0)
    {
        sudokuOverlayShape_t* topLine = heap_caps_malloc(sizeof(sudokuOverlayShape_t), MALLOC_CAP_8BIT);

        if (topLine)
        {
            topLine->type  = OVERLAY_LINE;
            topLine->color = c005;
            topLine->tag   = tag;
            getOverlayPos(&topLine->line.p1.x, &topLine->line.p1.y, 0, c, SUBPOS_N + 3);
            getOverlayPos(&topLine->line.p2.x, &topLine->line.p2.y, r - 1, c, SUBPOS_N + 3);

            push(&overlay->shapes, topLine);
        }
    }

    // add a bottom line
    if (r < gridSize - 1)
    {
        sudokuOverlayShape_t* bottomLine = heap_caps_malloc(sizeof(sudokuOverlayShape_t), MALLOC_CAP_8BIT);

        if (bottomLine)
        {
            bottomLine->type  = OVERLAY_LINE;
            bottomLine->color = c005;
            bottomLine->tag   = tag;
            getOverlayPos(&bottomLine->line.p1.x, &bottomLine->line.p1.y, r + 1, c, SUBPOS_S - 3);
            getOverlayPos(&bottomLine->line.p2.x, &bottomLine->line.p2.y, gridSize - 1, c, SUBPOS_S - 3);

            push(&overlay->shapes, bottomLine);
        }
    }
}

void addBoxHighlight(sudokuOverlay_t* overlay, const sudokuGrid_t* board, int box)
{
    sudokuOverlayShape_t* shape;

    // Draw a colored border around this box
    for (int row = 0; row < board->size; row++)
    {
        for (int col = 0; col < board->size; col++)
        {
            // The box of the square we're looking at
            uint16_t thisBox = board->boxMap[row * board->size + col];

            if (thisBox != box)
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