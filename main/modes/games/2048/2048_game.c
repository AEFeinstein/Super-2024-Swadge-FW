/**
 * @file 2048_game.c
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief Core of 2048 mode
 * @version 1.0.0
 * @date 2024-09-17
 *
 * @copyright Copyright (c) 2024
 *
 */

//==============================================================================
// Includes
//==============================================================================

#include "2048_game.h"

//==============================================================================
// Defines
//==============================================================================

// Pixel counts
#define T48_CELL_SIZE   50
#define T48_LINE_WEIGHT 4
#define T48_SIDE_MARGIN 30
#define T48_TOP_MARGIN  20

//==============================================================================
// Function Prototypes
//==============================================================================

// Game
static bool t48_setRandomCell(t48_t* t48, int32_t value);
static bool t48_slideTiles(t48_t* t48, buttonBit_t direction);
static bool t48_checkWin(t48_t* t48);
static bool t48_checkOver(t48_t* t48);

// Drawing
static bool t48_drawCellTiles(t48_t* t48, int32_t x, int32_t y, uint32_t elapsedUs);
static void t48_initSparkles(t48_t* t48, int32_t x, int32_t y, wsg_t* spr);
static bool t48_drawSparkles(t48cell_t* cell, uint32_t elapsedUs);
static void t48_drawNewTile(t48_t* t48, uint32_t elapsedUs);

// Helpers
static int32_t t48_horz_offset(int32_t col);
static int32_t t48_vert_offset(int32_t row);
static void FisherYatesShuffle(int32_t* array, int32_t size);

// LEDs
static void t48_lightLEDs(t48_t* t48, bool tileMoved, buttonBit_t direction);
static led_t t48_getLEDColor(t48_t* t48);

//==============================================================================
// Const Variables
//==============================================================================

static const char paused[]  = "Paused!";
static const char pausedA[] = "Press A to continue playing";
static const char pausedB[] = "Press B to abandon game";

static const int32_t tileIndices[] = {
    0, 8, 2, 12, 5, 9, 14, 10, 7, 15, 1, 3, 6, 13, 4, 11, 0, 8, 0, 0, 0, 0, 0, 0, 0,
};

static const int32_t sparkleIndices[] = {
    3, 3, 4, 1, 6, 0, 7, 0, 3, 4, 4, 7, 5, 6, 6, 2, 2, 1, 3, 3, 3, 3, 3, 3, 3,
};

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initialize the game state
 *
 * @param t48 The game data to initialize
 */
void t48_gameInit(t48_t* t48)
{
    // Clear the board
    memset(t48->board, 0, sizeof(t48->board));

    // Reset the score
    t48->score = 0;

    // Test code
    /* int test = 2;
    for (int32_t i = 0; i < 15; i++)
    {
        t48->board[i / 4][i % 4].value               = test;
        t48->board[i / 4][i % 4].drawnTiles[0].value = test;
        test = test << 1;
    } */

    /* t48->board[0][1].value               = 1024;
    t48->board[0][2].value               = 1024;
    t48->board[0][2].drawnTiles[0].value = 1024;
    t48->board[0][1].drawnTiles[0].value = 1024; */

    // Set two cells randomly
    for (int32_t i = 0; i < 2; i++)
    {
        t48_setRandomCell(t48, 2);
    }

    // Accept input
    t48->acceptGameInput = true;
}

/**
 * @brief Run the main game loop for 2048. This animates and draws the board and handles game logic
 *
 * @param t48 The game data to loop
 * @param elapsedUs The time since this was last called, for animation
 */
void t48_gameLoop(t48_t* t48, int32_t elapsedUs)
{
    if (t48->paused)
    {
        // Draw pause screen
        fillDisplayArea(64, 75, TFT_WIDTH - 64, 100, c100);
        drawText(&t48->titleFont, c555, paused, (TFT_WIDTH - textWidth(&t48->titleFont, paused)) / 2, 80);
        fillDisplayArea(32, 110, TFT_WIDTH - 32, 130, c100);
        drawText(&t48->font, c555, pausedA, (TFT_WIDTH - textWidth(&t48->font, pausedA)) / 2, 115);
        fillDisplayArea(32, 135, TFT_WIDTH - 32, 155, c100);
        drawText(&t48->font, c555, pausedB, (TFT_WIDTH - textWidth(&t48->font, pausedB)) / 2, 140);
        return; // Bail instead of drawing the rest of the game
    }

    // Blank the display
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);

    // Draw grid lines first
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

    // Draw Score second
    static char textBuffer[32];
    snprintf(textBuffer, sizeof(textBuffer) - 1, "Score: %" PRIu32, t48->score);
    drawText(&t48->font, c555, textBuffer, T48_SIDE_MARGIN, 4);

    // Check if anything is animating
    bool animationInProgress = false;

    // Draw new tile third
    if (t48->nTile.active)
    {
        // t48_drawNewTile(t48, elapsedUs);
        t48_drawNewTile(t48, elapsedUs);
    }

    // Draw Tiles fourth
    for (int32_t x = 0; x < T48_GRID_SIZE; x++)
    {
        for (int32_t y = 0; y < T48_GRID_SIZE; y++)
        {
            // Get a reference to the cell
            t48cell_t* cell = &t48->board[x][y];

            // If tile just spawned, don't draw until done animating
            if (t48->nTile.active && (t48->nTile.pos.x == x && t48->nTile.pos.y == y))
            {
                continue;
            }

            // Draw the tile(s) for this cell
            if (t48_drawCellTiles(t48, x, y, elapsedUs))
            {
                animationInProgress = true;
            }
            else if (cell->drawnTiles[1].value)
            {
                // Movement is done
                if (cell->drawnTiles[0].value)
                {
                    // If this is a real loop, not after a warp
                    if (0 != elapsedUs)
                    {
                        // There are two values that need to get merged. Init some sparkles
                        t48_initSparkles(t48, x, y,
                                         &t48->sparkleSprites[sparkleIndices[31 - __builtin_clz(cell->value)]]);
                    }
                    // Tally score
                    t48->score += cell->value;
                }

                // Set the drawn tiles to only draw one, with no offset
                memset(cell->drawnTiles, 0, sizeof(cell->drawnTiles));
                cell->drawnTiles[0].value = cell->value;
            }
        }
    }

    // Draw sparkles fifth
    for (int32_t x = 0; x < T48_GRID_SIZE; x++)
    {
        for (int32_t y = 0; y < T48_GRID_SIZE; y++)
        {
            // Draw sparkles for this cell
            if (t48_drawSparkles(&t48->board[x][y], elapsedUs))
            {
                animationInProgress = true;
            }
        }
    }

    // When the animation is done
    if (!t48->acceptGameInput && !animationInProgress)
    {
        // Check if game has been won
        if (!t48->alreadyWon && t48_checkWin(t48))
        {
            t48->state      = T48_WIN_SCREEN;
            t48->alreadyWon = true;
        }

        // Check for loss condition (no valid moves)
        if (t48_checkOver(t48))
        {
            t48->state = T48_END_SCREEN;
        }

        // Accept input again
        t48->acceptGameInput = true;
    }
}

/**
 * @brief Process game input
 *
 * @param t48 The game state
 * @param button The button which was pressed
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
            // If the game is paused
            if (t48->paused)
            {
                break;
            }

            // If input isn't being accepted
            if (!t48->acceptGameInput)
            {
                // Warp everything to final destination
                for (int32_t x = 0; x < T48_GRID_SIZE; x++)
                {
                    for (int32_t y = 0; y < T48_GRID_SIZE; y++)
                    {
                        // Warp all drawn tiles
                        for (int32_t t = 0; t < T48_TILES_PER_CELL; t++)
                        {
                            t48->board[x][y].drawnTiles[t].xOffset = 0;
                            t48->board[x][y].drawnTiles[t].yOffset = 0;
                        }

                        // Kill all sparkles
                        for (int32_t t = 0; t < T48_SPARKLES_PER_CELL; t++)
                        {
                            t48->board[x][y].sparkles[t].active = false;
                        }
                    }
                }
                // Run the game loop once to update state based on the warped destination
                t48_gameLoop(t48, 0);
            }

            // Start sliding tiles
            if (t48_slideTiles(t48, button))
            {
                // Spawn a random tile, 10% chance of 4, 90% chance of 2
                // See https://play2048.co/index.js, addRandomTile()
                t48_setRandomCell(t48, (esp_random() % 10 == 0) ? 4 : 2);
                // Don't accept input until the slide is done
                t48->acceptGameInput = false;
                // Play a click
                soundPlaySfx(&t48->click, MIDI_SFX);
            }
            break;
        }
        case PB_A:
        {
            t48->paused = false;
            break;
        }
        case PB_B:
        {
            if (t48->paused)
            {
                t48->state  = T48_START_SCREEN;
                t48->paused = false;
            }
            break;
        }
        case PB_START:
        {
            t48->paused = !t48->paused;
        }
        case PB_SELECT:
        default:
        {
            break;
        }
    }
}

/**
 * @brief Set a random cell to the given value
 *
 * @param t48 The game data to set a cell in
 * @param value The value to set the cell to
 * @return true if the value was set, false if there are no empty cells
 */
static bool t48_setRandomCell(t48_t* t48, int32_t value)
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

    // Cant set a random cell if there are no empty spots
    if (0 == numEmptyCells)
    {
        return false;
    }

    // Fill in one of the random cells
    int32_t id                = emptyCells[esp_random() % numEmptyCells];
    t48cell_t* cell           = &t48->board[id / T48_GRID_SIZE][id % T48_GRID_SIZE];
    cell->value               = value;
    cell->drawnTiles[0].value = value;
    cell->drawnTiles[1].value = 0;

    // Set newTile for animation
    vec_t newPos         = {.x = id / T48_GRID_SIZE, .y = id % T48_GRID_SIZE};
    t48->nTile.pos       = newPos;
    t48->nTile.active    = true;
    t48->nTile.spawnTime = 0;
    t48->nTile.sequence  = 0;
    return true;
}

/**
 * @brief Slide the tiles in the game, setting up animations and such
 *
 * @param t48 The game state to slide
 * @param direction The direction to slide
 * @return true if something moved, false if nothing moved
 */
static bool t48_slideTiles(t48_t* t48, buttonBit_t direction)
{
    // Check if any tile moved
    bool tileMoved = false;

    // For each row or column
    for (int32_t outer = 0; outer < T48_GRID_SIZE; outer++)
    {
        // Make a slice depending on the slide direction. This is either a row or a column, forwards or backwards
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
                    return false;
                }
            }
        }

        // Clear out merged flags
        for (int32_t x = 0; x < T48_GRID_SIZE; x++)
        {
            for (int32_t y = 0; y < T48_GRID_SIZE; y++)
            {
                t48->board[x][y].merged = false;
            }
        }

        // Now slide the slice

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
                else if (!slice[dest]->merged && (slice[src]->value == slice[dest]->value))
                {
                    // Slide and merge here
                    slice[dest]->merged = true;
                    validDest           = dest;
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
                // At least one tile moved
                tileMoved = true;

                // Set up pixel offset for this tile to animate the slide
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
                        return false;
                    }
                }

                // Draw the pre-merge values before the slide animation finishes
                // Up to two tiles can be sliding into a single cell!
                // Find an empty slot in this cell to store the tile animation state
                for (int t = 0; t < T48_TILES_PER_CELL; t++)
                {
                    t48drawnTile_t* tile = &slice[validDest]->drawnTiles[t];
                    if (0 == tile->value)
                    {
                        // Store tile animation state in the destination
                        tile->value   = slice[src]->value;
                        tile->xOffset = xOffset;
                        tile->yOffset = yOffset;
                        break;
                    }
                }
                // Clear tile animation state in the source
                memset(slice[src]->drawnTiles, 0, sizeof(slice[src]->drawnTiles));

                // Move the underlying value (actual game state)
                slice[validDest]->value += slice[src]->value;
                slice[src]->value = 0;
            }
        }
    }

    // Light LEDs
    t48_lightLEDs(t48, tileMoved, direction);

    return tileMoved;
}

/**
 * @brief Checks if any of the cells have reached 2048
 *
 * @param t48 Game data
 * @return true if a cell is 2048
 * @return false otherwise
 */
static bool t48_checkWin(t48_t* t48)
{
    bool won = false;
    for (int32_t id = 0; id < T48_GRID_SIZE * T48_GRID_SIZE; id++)
    {
        if (t48->board[id / T48_GRID_SIZE][id % T48_GRID_SIZE].value == 2048)
        {
            won = true;
        }
    }
    return won;
}

/**
 * @brief Checks if any valid moves are left
 *
 * @param t48 game data
 * @return true if the game no longer has viable moves
 * @return false otherwise
 */
static bool t48_checkOver(t48_t* t48)
{
    // Check if any cells are open
    for (int32_t id = 0; id < T48_GRID_SIZE * T48_GRID_SIZE; id++)
    {
        if (t48->board[id / T48_GRID_SIZE][id % T48_GRID_SIZE].value == 0)
        {
            return false;
        }
    }
    // Check if any two consecutive block match vertically
    for (uint8_t row = 0; row < T48_GRID_SIZE; row++)
    {
        for (uint8_t col = 0; col < T48_GRID_SIZE - 1; col++)
        { // -1 to account for comparison
            if (t48->board[row][col].value == t48->board[row][col + 1].value)
            {
                return false;
            }
        }
    }
    // Check if any two consecutive block match horizontally
    for (uint8_t row = 0; row < T48_GRID_SIZE - 1; row++)
    { // -1 to account for comparison
        for (uint8_t col = 0; col < T48_GRID_SIZE - 1; col++)
        {
            if (t48->board[row][col].value == t48->board[row + 1][col].value)
            {
                return false;
            }
        }
    }
    // Game is over
    return true;
}

/**
 * @brief Draw all of a cell's tiles
 *
 * @param t48 The game state to draw tiles from
 * @param x The X index of the cell on the board
 * @param y The Y index of the cell on the board
 * @param elapsedUs The time since this was last called, for animation
 * @return true if animation is in progress, false if it isn't
 */
static bool t48_drawCellTiles(t48_t* t48, int32_t x, int32_t y, uint32_t elapsedUs)
{
    // Get a reference to the cell
    t48cell_t* cell = &t48->board[x][y];

    // Check if animation is in progress
    bool animationInProgress = false;

    // For each tile to draw on this cell (may be multiple in motion)
    for (int t = 0; t < T48_TILES_PER_CELL; t++)
    {
        // Get a reference to the tile
        t48drawnTile_t* tile = &cell->drawnTiles[t];

        // If there is something to draw
        if (tile->value)
        {
            // TODO make this animation microsecond based

            int32_t pxPerFrame = 8;

            // Move tile towards target X
            if (tile->xOffset <= -pxPerFrame)
            {
                tile->xOffset += pxPerFrame;
                animationInProgress = true;
            }
            else if (tile->xOffset >= pxPerFrame)
            {
                tile->xOffset -= pxPerFrame;
                animationInProgress = true;
            }
            else if (tile->xOffset)
            {
                tile->xOffset       = 0;
                animationInProgress = true;
            }

            // Move tile towards target Y
            if (tile->yOffset <= -pxPerFrame)
            {
                tile->yOffset += pxPerFrame;
                animationInProgress = true;
            }
            else if (tile->yOffset >= pxPerFrame)
            {
                tile->yOffset -= pxPerFrame;
                animationInProgress = true;
            }
            else if (tile->yOffset)
            {
                tile->yOffset       = 0;
                animationInProgress = true;
            }

            // Draw the sprite first
            uint16_t x_offset = t48_horz_offset(x) + tile->xOffset;
            uint16_t y_offset = t48_vert_offset(y) + tile->yOffset;
            wsg_t* tileWsg    = &t48->tiles[tileIndices[31 - __builtin_clz(tile->value)]];
            drawWsgSimple(tileWsg, x_offset, y_offset);

            // Draw the text on top
            static char buffer[16];
            snprintf(buffer, sizeof(buffer) - 1, "%" PRIu32, tile->value);
            uint16_t tWidth = textWidth(&t48->font, buffer);
            drawText(&t48->font, c555, buffer,                //
                     x_offset + (T48_CELL_SIZE - tWidth) / 2, //
                     y_offset + (T48_CELL_SIZE - t48->font.height) / 2);
        }
    }
    return animationInProgress;
}

/**
 * @brief Draws a flash of light to indicate teh new tile's spawn location
 *
 * @param t48 Game Data
 * @param elapsedUs Time since last frame
 */
static void t48_drawNewTile(t48_t* t48, uint32_t elapsedUs)
{
    // Add US
    t48->nTile.spawnTime += elapsedUs;

    if (t48->nTile.spawnTime >= T48_NEW_SPARKLE_SEQ)
    {
        t48->nTile.sequence++;
        t48->nTile.spawnTime -= T48_NEW_SPARKLE_SEQ;
    }

    switch (t48->nTile.sequence)
    {
        case 0:
        case 10:
        {
            drawWsgSimple(&t48->newSparkles[0],              //
                          t48_horz_offset(t48->nTile.pos.x), //
                          t48_vert_offset(t48->nTile.pos.y));
            break;
        }
        case 1:
        case 9:
        {
            drawWsgSimple(&t48->newSparkles[1],              //
                          t48_horz_offset(t48->nTile.pos.x), //
                          t48_vert_offset(t48->nTile.pos.y));
            break;
        }
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        {
            drawWsgSimple(&t48->newSparkles[2],              //
                          t48_horz_offset(t48->nTile.pos.x), //
                          t48_vert_offset(t48->nTile.pos.y));
            break;
        }
        default:
        {
            // If no longer inside animation sequence, deactivate
            t48->nTile.active = false;
            break;
        }
    }
}

/**
 * @brief Initialize sparkles for a cell
 *
 * @param t48 The game data to initialize sparkles in
 * @param x The X index of the cell on the board
 * @param y The Y index of the cell on the board
 * @param spr The sprite to use as a sparkle
 */
static void t48_initSparkles(t48_t* t48, int32_t x, int32_t y, wsg_t* spr)
{
    // If there are any active sparkles already, return
    for (int32_t sIdx = 0; sIdx < T48_SPARKLES_PER_CELL; sIdx++)
    {
        t48Sparkle_t* sparkle = &t48->board[x][y].sparkles[sIdx];
        if (sparkle->active)
        {
            return;
        }
    }

    // Create an array of possible directions. This ensures no random duplicates.
    int32_t directions[T48_SPARKLES_PER_CELL * 2];
    for (int i = 0; i < ARRAY_SIZE(directions); i++)
    {
        directions[i] = i - (ARRAY_SIZE(directions) / 2);
    }
    // Shuffle the directions
    FisherYatesShuffle(directions, ARRAY_SIZE(directions));

    // For each sparkle
    for (int32_t sIdx = 0; sIdx < T48_SPARKLES_PER_CELL; sIdx++)
    {
        // Get a reference
        t48Sparkle_t* sparkle = &t48->board[x][y].sparkles[sIdx];

        // Set speed
        sparkle->xSpd = directions[sIdx];
        sparkle->ySpd = -8 - (esp_random() % 8);

        // Convert cell coords to pixel space
        sparkle->x = t48_horz_offset(x) + T48_CELL_SIZE / 2;
        sparkle->y = t48_vert_offset(y) + T48_CELL_SIZE / 2;

        // Set image
        sparkle->img = spr;

        // set active
        sparkle->active = true;
    }
}

/**
 * @brief Draw a cell's sparkles
 *
 * @param cell The cell to draw sparkles for
 * @param elapsedUs The time since this was last called, for animation
 * @return true if animation is in progress, false if it isn't
 */
static bool t48_drawSparkles(t48cell_t* cell, uint32_t elapsedUs)
{
    bool animating = false;

    // Draw all this cell's sparkles
    for (int32_t sIdx = 0; sIdx < T48_SPARKLES_PER_CELL; sIdx++)
    {
        // Get a reference to the sparkle
        t48Sparkle_t* sparkle = &cell->sparkles[sIdx];

        // If the sparkle is active
        if (sparkle->active)
        {
            // Deactivate it if it's offscreen
            if ((sparkle->y >= TFT_HEIGHT) || (sparkle->x + sparkle->img->w < 0) || (sparkle->x >= TFT_WIDTH))
            {
                sparkle->active = false;
            }
            else
            {
                // Otherwise move and draw it
                // TODO adjust animation to be microsecond-based
                sparkle->ySpd += 1;
                sparkle->y += sparkle->ySpd;
                sparkle->x += sparkle->xSpd;
                drawWsgSimple(sparkle->img, sparkle->x, sparkle->y);
                animating = true;
            }
        }
    }
    return animating;
}

/**
 * @brief Get the horizontal pixel offset for a column on the board
 *
 * @param col The column index
 * @return The pixel offset for this column
 */
static int32_t t48_horz_offset(int32_t col)
{
    return col * (T48_CELL_SIZE + T48_LINE_WEIGHT) + T48_SIDE_MARGIN + T48_LINE_WEIGHT;
}

/**
 * @brief Get the horizontal pixel offset for a row on the board
 *
 * @param row The row index
 * @return The pixel offset for this row
 */
static int32_t t48_vert_offset(int32_t row)
{
    return row * (T48_CELL_SIZE + T48_LINE_WEIGHT) + T48_TOP_MARGIN + T48_LINE_WEIGHT;
}

/**
 * @brief Shuffle the items in an array
 *
 * See https://en.wikipedia.org/wiki/Fisher%E2%80%93Yates_shuffle
 *
 * @param array The array to shuffle
 * @param size The number of elements in the array to shuffle
 */
static void FisherYatesShuffle(int32_t* array, int32_t size)
{
    int32_t n, k, temp;

    // Iterate through the array in reverse order
    for (n = size - 1; n > 0; n--)
    {
        // Generate a random index 'k' between 0 and n (inclusive)
        k = esp_random() % (n + 1);

        // Swap the elements at indices 'n' and 'k'
        temp     = array[n];
        array[n] = array[k];
        array[k] = temp;
    }
}

/**
 * @brief Illuminates the LEDs in the directions that the player pressed
 *
 * @param t48 Game data
 * @param tileMoved If the tile moved or bounced
 * @param direction Direction tiles moved in
 */
static void t48_lightLEDs(t48_t* t48, bool tileMoved, buttonBit_t direction)
{
    if (tileMoved)
    {
        // Illuminate LEDS based on direction and value
        led_t led = t48_getLEDColor(t48);
        switch (direction)
        {
            case PB_LEFT:
            {
                // LED 3 and 4
                t48->leds[3] = led;
                t48->leds[4] = led;
                break;
            }
            case PB_RIGHT:
            {
                // LED 0 and 1
                t48->leds[0] = led;
                t48->leds[1] = led;
                break;
            }
            case PB_UP:
            {
                // LEDs 1, 2, 3
                t48->leds[1] = led;
                t48->leds[2] = led;
                t48->leds[3] = led;
                break;
            }
            case PB_DOWN:
            {
                // LEDs 4 and 0
                t48->leds[0] = led;
                t48->leds[4] = led;
                break;
            }
            default:
            {
                break;
            }
        }
    }
    else
    {
        for (int32_t i = 0; i < CONFIG_NUM_LEDS; i++)
        {
            led_t led    = {.r = 128, .b = 128, .g = 128};
            t48->leds[i] = led;
        }
    }
}

/**
 * @brief Gets the correct LED color
 *
 * @param t48 Game data
 * @return led_t The led object containing the colors to lo0ad into the LEDs
 */
static led_t t48_getLEDColor(t48_t* t48)
{
    // Get the largest value available
    int32_t value = 0;
    for (int32_t i = 0; i < T48_GRID_SIZE * T48_GRID_SIZE; i++)
    {
        if (t48->board[i / T48_GRID_SIZE][i % T48_GRID_SIZE].value > value)
        {
            value = t48->board[i / T48_GRID_SIZE][i % T48_GRID_SIZE].value;
        }
    }
    led_t col = {0};
    switch (value)
    {
        case 4:
        {
            // Cyan
            col.g = 255;
            col.b = 255;
            return col;
        }
        case 8:
        case 8192:
        {
            // Red
            col.r = 255;
            return col;
        }
        case 16:
        case 2048:
        case 16384:
        {
            // Green
            col.g = 128;
            return col;
        }
        case 2:
        case 32:
        case 128:
        case 131072:
        {
            // Pink
            col.r = 200;
            col.g = 150;
            col.b = 150;
            return col;
        }
        case 64:
        case 512:
        {
            // Yellow
            col.r = 128;
            col.g = 128;
            return col;
        }
        case 256:
        {
            // Orange
            col.r = 255;
            col.g = 165;
            return col;
        }
        case 1024:
        case 65536:
        {
            // Blue
            col.b = 255;
            return col;
        }
        case 4096:
        {
            // Dark Pink
            col.r = 255;
            col.g = 64;
            col.b = 64;
            return col;
        }
        case 32768:
        {
            // Purple
            col.r = 200;
            col.b = 200;
            return col;
        }
        default:
            col.r = 255;
            col.g = 128;
            col.b = 128;
            return col;
    }
}