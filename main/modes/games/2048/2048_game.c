#include "hdw-tft.h"
#include "2048_game.h"

// Pixel counts
#define T48_CELL_SIZE   50
#define T48_LINE_WEIGHT 4
#define T48_SIDE_MARGIN 30
#define T48_TOP_MARGIN  20

static int32_t t48_horz_offset(int32_t col);
static int32_t t48_vert_offset(int32_t row);
static bool t48_drawTile(t48_t* t48, int32_t x, int32_t y, uint32_t elapsedUs);

static void t48_slideTiles(t48_t* t48, buttonBit_t direction);
static bool t48_setRandomTile(t48_t* t48, int32_t value);

/**
 * @brief TODO
 *
 * @param t48
 */
void t48_gameInit(t48_t* t48)
{
    // Clear the board
    memset(t48->board, 0, sizeof(t48->board));

    // Set two cells randomly
    // for (int32_t i = 0; i < 2; i++)
    // {
    //     t48_setRandomTile(t48, 2);
    // }

    t48->board[0][0].value               = 2;
    t48->board[0][0].drawnTiles[0].value = 2;

    t48->board[0][2].value               = 2;
    t48->board[0][2].drawnTiles[0].value = 2;

    // Accept input
    t48->acceptGameInput = true;
}

/**
 * @brief TODO
 *
 * @param t48
 * @param value
 * @return true
 * @return false
 */
bool t48_setRandomTile(t48_t* t48, int32_t value)
{
    int32_t emptyCells[T48_GRID_SIZE * T48_GRID_SIZE];
    int32_t numEmptyCells = 0;

    // Get a list of empty cells
    for (int32_t id = 0; id < T48_GRID_SIZE * T48_GRID_SIZE; id++)
    {
        t48cell_t* cell = &t48->board[id / T48_GRID_SIZE][id % T48_GRID_SIZE];
        if (0 == cell->value)
        {
            emptyCells[numEmptyCells++] = id;
        }
    }

    if (0 == numEmptyCells)
    {
        return false;
    }

    int32_t id                = emptyCells[esp_random() % numEmptyCells];
    t48cell_t* cell           = &t48->board[id / T48_GRID_SIZE][id % T48_GRID_SIZE];
    cell->value               = value;
    cell->drawnTiles[0].value = value;
    cell->drawnTiles[1].value = 0;
    return true;
}

/**
 * @brief TODO
 *
 * @param t48
 * @param elapsedUs
 */
void t48_gameDraw(t48_t* t48, int32_t elapsedUs)
{
    // Blank
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);

    // Draw grid lines
    for (uint8_t i = 0; i < T48_GRID_SIZE + 1; i++)
    {
        int16_t left = i * (T48_CELL_SIZE + T48_LINE_WEIGHT);
        fillDisplayArea(T48_SIDE_MARGIN + left,                   //
                        T48_TOP_MARGIN,                           //
                        T48_SIDE_MARGIN + left + T48_LINE_WEIGHT, //
                        TFT_HEIGHT,                               //
                        c111);

        int16_t top = i * (T48_CELL_SIZE + T48_LINE_WEIGHT);
        fillDisplayArea(T48_SIDE_MARGIN,                        //
                        top + T48_TOP_MARGIN,                   //
                        TFT_WIDTH - T48_SIDE_MARGIN,            //
                        top + T48_TOP_MARGIN + T48_LINE_WEIGHT, //
                        c111);
    }

    // Score
    static char textBuffer[32];
    snprintf(textBuffer, sizeof(textBuffer) - 1, "Score: %" PRIu32, t48->score);
    drawText(&t48->font, c555, textBuffer, T48_SIDE_MARGIN, 4);

    bool animationInProgress = false;

    // Draw Tiles
    for (int32_t x = 0; x < T48_GRID_SIZE; x++)
    {
        for (int32_t y = 0; y < T48_GRID_SIZE; y++)
        {
            // Draw the tile
            if (t48_drawTile(t48, x, y, elapsedUs))
            {
                animationInProgress = true;
            }
        }
    }

    // When the animation is done
    if (!t48->acceptGameInput && !animationInProgress)
    {
        // Spawn a random tile, either a 2 or a 4
        t48_setRandomTile(t48, (esp_random() % 4 == 0) ? 4 : 2);

        // Set the drawn values to actual values
        for (int32_t x = 0; x < T48_GRID_SIZE; x++)
        {
            for (int32_t y = 0; y < T48_GRID_SIZE; y++)
            {
                t48cell_t* cell = &t48->board[x][y];
                memset(cell->drawnTiles, 0, sizeof(cell->drawnTiles));
                cell->drawnTiles[0].value = cell->value;
            }
        }

        // Accept input again
        t48->acceptGameInput = true;
    }

    // Draw sparkles
    // for (int i = 0; i < T48_MAX_SPARKLES; i++)
    // {
    //     if (t48->sparks[i].y >= (TFT_HEIGHT + T48_SPARKLE_SIZE))
    //     {
    //         t48->sparks[i].active = false;
    //     }
    //     if (t48->sparks[i].active)
    //     {
    //         t48->sparks[i].ySpd += 1;
    //         t48->sparks[i].y += t48->sparks[i].ySpd;
    //         t48->sparks[i].x += t48->sparks[i].xSpd;
    //         drawWsgSimple(t48->sparks[i].img, t48->sparks[i].x, t48->sparks[i].y);
    //     }
    // }
}

/**
 * @brief TODO
 *
 * @param col
 * @return int32_t
 */
static int32_t t48_horz_offset(int32_t col)
{
    return col * (T48_CELL_SIZE + T48_LINE_WEIGHT) + T48_SIDE_MARGIN + T48_LINE_WEIGHT;
}

/**
 * @brief TODO
 *
 * @param row
 * @return int32_t
 */
static int32_t t48_vert_offset(int32_t row)
{
    return row * (T48_CELL_SIZE + T48_LINE_WEIGHT) + T48_TOP_MARGIN + T48_LINE_WEIGHT;
}

/**
 * @brief TODO
 *
 * @param t48
 * @param x
 * @param y
 * @param elapsedUs
 */
static bool t48_drawTile(t48_t* t48, int32_t x, int32_t y, uint32_t elapsedUs)
{
    // Get a reference to the cell
    t48cell_t* cell = &t48->board[x][y];

    bool animationInProgress = false;
    // For each tile to draw on this cell (may be multiple in motion)
    for (int t = 0; t < T48_TILES_PER_CELL; t++)
    {
        // Get a reference to the tile
        t48drawnTile_t* tile = &cell->drawnTiles[t];

        // If there is something to draw
        if (tile->value)
        {
            // Move tile towards target X
            if (tile->xOffset < tile->xOffsetTarget)
            {
                tile->xOffset += 2;
                animationInProgress = true;
            }
            else if (tile->xOffset > tile->xOffsetTarget)
            {
                tile->xOffset -= 2;
                animationInProgress = true;
            }

            // Move tile towards target Y
            if (tile->yOffset < tile->yOffsetTarget)
            {
                tile->yOffset += 2;
                animationInProgress = true;
            }
            else if (tile->yOffset > tile->yOffsetTarget)
            {
                tile->yOffset -= 2;
                animationInProgress = true;
            }

            // Sprite
            uint16_t x_cell_offset = t48_horz_offset(x) + tile->xOffset;
            uint16_t y_cell_offset = t48_vert_offset(y) + tile->yOffset;
            wsg_t* tileWsg         = &t48->tiles[31 - __builtin_clz(tile->value)];
            drawWsgSimple(tileWsg, x_cell_offset, y_cell_offset);

            // Text
            static char buffer[16];
            snprintf(buffer, sizeof(buffer) - 1, "%" PRIu32, tile->value);
            uint16_t text_center = (textWidth(&t48->font, buffer)) / 2;
            drawText(&t48->font, c555, buffer,                        //
                     x_cell_offset - text_center + T48_CELL_SIZE / 2, //
                     y_cell_offset - 4 + T48_CELL_SIZE / 2);
        }
    }
    return animationInProgress;
}

/**
 * @brief TODO
 *
 * @param t48
 * @param button
 */
void t48_gameInput(t48_t* t48, buttonBit_t button)
{
    switch (button)
    {
        case PB_UP:
        case PB_DOWN:
        case PB_LEFT:
        case PB_RIGHT:
        {
            if (t48->acceptGameInput)
            {
                // Start sliding tiles
                t48_slideTiles(t48, button);
                // Don't accept input until the slide is done
                t48->acceptGameInput = false;
            }
            break;
        }
        default:
        case PB_A:
        case PB_B:
        case PB_START:
        case PB_SELECT:
        {
            break;
        }
    }
}

/**
 * @brief TODO
 *
 * @param t48
 * @param direction
 */
void t48_slideTiles(t48_t* t48, buttonBit_t direction)
{
    // For each row or column
    for (int32_t outer = 0; outer < T48_GRID_SIZE; outer++)
    {
        // Make a slice
        t48cell_t* slice[T48_GRID_SIZE];
        for (int32_t inner = 0; inner < T48_GRID_SIZE; inner++)
        {
            switch (direction)
            {
                case PB_LEFT:
                {
                    slice[inner] = &t48->board[inner][outer];
                    break;
                }
                case PB_RIGHT:
                {
                    slice[inner] = &t48->board[T48_GRID_SIZE - inner - 1][outer];
                    break;
                }
                case PB_UP:
                {
                    slice[inner] = &t48->board[outer][inner];
                    break;
                }
                case PB_DOWN:
                {
                    slice[inner] = &t48->board[outer][T48_GRID_SIZE - inner - 1];
                    break;
                }
                default:
                {
                    return;
                }
            }
        }

        // Now slide the slice

        // Allow one merge per row
        bool sliceMerger = false;

        // Check sources to slide from front to back
        for (int32_t src = 1; src < T48_GRID_SIZE; src++)
        {
            // No tile to move, continue
            if (0 == slice[src]->value)
            {
                continue;
            }

            // Keep track of this tile's potential destination
            int32_t validDest = src;

            // Check destinations to slide to from back to front
            for (int32_t dest = src - 1; dest >= 0; dest--)
            {
                if (0 == slice[dest]->value)
                {
                    // Free to slide here, then keep checking
                    validDest = dest;
                }
                else if (!sliceMerger && (slice[src]->value == slice[dest]->value))
                {
                    // Slide and merge here
                    sliceMerger = true;
                    validDest   = dest;
                    // Merge here, so break
                    break;
                }
                else
                {
                    // Can't move further, break
                    break;
                }
            }

            // If the destination doesn't match the source
            if (validDest != src)
            {
                // Set up current board state offset
                int32_t xOffset = 0;
                int32_t yOffset = 0;
                switch (direction)
                {
                    case PB_LEFT:
                    {
                        xOffset = t48_horz_offset(src) - t48_horz_offset(validDest);
                        break;
                    }
                    case PB_RIGHT:
                    {
                        xOffset = t48_horz_offset(validDest) - t48_horz_offset(src);
                        break;
                    }
                    case PB_UP:
                    {
                        yOffset = t48_vert_offset(src) - t48_vert_offset(validDest);
                        break;
                    }
                    case PB_DOWN:
                    {
                        yOffset = t48_vert_offset(validDest) - t48_vert_offset(src);
                        break;
                    }
                    default:
                    {
                        return;
                    }
                }

                // Draw the pre-merge values before animation finishes
                // addTileToCell(slice[validDest], xOffset, yOffset, slice[src]->value);
                for (int t = 0; t < T48_TILES_PER_CELL; t++)
                {
                    t48drawnTile_t* tile = &slice[validDest]->drawnTiles[t];
                    if (0 == tile->value)
                    {
                        tile->value   = slice[src]->value;
                        tile->xOffset = xOffset;
                        tile->yOffset = yOffset;
                        break;
                    }
                }
                memset(slice[src]->drawnTiles, 0, sizeof(slice[src]->drawnTiles));

                // Move the underlying value
                slice[validDest]->value += slice[src]->value;
                slice[src]->value = 0;
            }
        }
    }
}
