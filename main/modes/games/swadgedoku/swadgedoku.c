//==============================================================================
// Includes
//==============================================================================

#include "swadgedoku.h"

#include "menu.h"

//==============================================================================
// Defines
//==============================================================================

// There are exactly two unique binary sudoku games... And we have both!
#define SUDOKU_MIN_BASE 2
#define SUDOKU_MAX_BASE 16

/// @brief Value for when a square does not map to any box, such as a void square
#define BOX_NONE UINT8_MAX

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    SWADGEDOKU_MAIN_MENU = 0,
    SWADGEDOKU_GAME = 1,
} sudokuScreen_t;

typedef enum
{
    /// @brief Your classic everyday sudoku with a square grid and boxes
    SM_CLASSIC = 0,
    /// @brief Five games of sudoku in an X, with each corner game sharing a box with the central game
    SM_X_GRID,
} sudokuMode_t;

typedef enum
{
    //< This square has no flags set
    SF_NONE = 0,
    //< Square is locked and unchangeable as part of the puzzle
    SF_LOCKED = 1,
    //< Square is not considered part of the playable area, for irregular puzzles
    SF_VOID = 2,
} sudokuFlag_t;

typedef enum
{
    /// @brief No overlays at all
    OVERLAY_NONE = 0,
    /// @brief Highlight this square with the row color
    OVERLAY_HIGHLIGHT_ROW = 1,
    /// @brief Highlight this square with the column color
    OVERLAY_HIGHLIGHT_COL = 2,
    /// @brief Highlight this square with the box color
    OVERLAY_HIGHLIGHT_BOX = 4,
    /// @brief Highlight this square with color A
    OVERLAY_HIGHLIGHT_A = 8,
    /// @brief Highlight this square with color B
    OVERLAY_HIGHLIGHT_B = 16,
    /// @brief Highlight this square with color C
    OVERLAY_HIGHLIGHT_C = 32,
    /// @brief Highlight this square with color D
    OVERLAY_HIGHLIGHT_D = 64,
    /// @brief Mark this square as having an error
    OVERLAY_ERROR = 128,
    /// @brief Cross out this square as 'not possible'
    OVERLAY_CROSS_OUT = 256,
    /// @brief Mark this square as a 'maybe' or 'unknown'
    OVERLAY_QUESTION = 512,
    /// @brief Mark this square as 
    OVERLAY_CHECK = 1024,
    /// @brief Overlays the computed notes onto the square
    OVERLAY_NOTES = 2048,
} sudokuOverlayOpt_t;

typedef enum
{
    /// @brief This shape is a cursor and shouldn't be removed
    ST_CURSOR,
    /// @brief This is a temporary annotation added by sudokuAnnotate()
    ST_ANNOTATE,
} sudokuShapeTag_t;

typedef enum
{
    OVERLAY_RECT,
    OVERLAY_CIRCLE,
    OVERLAY_LINE,
    OVERLAY_ARROW,
    OVERLAY_TEXT,
} sudokuOverlayShapeType_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    /// @brief The Sudoku rules used for this board
    sudokuMode_t mode;

    /// @brief The size of the grid. Could be larger
    uint8_t size;

    /// @brief The number of digits per line/box, and the number of boxes.
    /// Can be anywhere from 2 to 16, though who knows if I'll do non-square bases with irregular boxes (like killer sudoku?)
    uint8_t base;

    /// @brief The box configuration for this grid
    //sudokuBox_t* boxes;

    /// @brief A grid-sized array of actual filled numbers; 0 used for empty squares
    uint8_t* grid;

    /// @brief A grid-sized array of the 'notes' layer
    /// Each element is a bitmask representing which numbers are still valid for that square
    uint16_t* notes;

    /// @brief A grid-sized array mapping each square into a particular box index.
    /// Elements with value BOX_NONE are not part of any box.
    uint8_t* boxMap;

    /// @brief A grid-sized array of the 'flags' layer which has extra options for squares
    /// For example, it marks which squares are unchangeable as part of the puzzle
    sudokuFlag_t* flags;
} sudokuGrid_t;

// grid functions: setValue(x, y, val); eraseValue(x, y, val); setNotes()

/// @brief Holds all the colors for the display theme
typedef struct
{
    /// @brief The background color outside of the grid and behind the UI
    paletteColor_t bgColor;

    /// @brief The fill color of the individual squares without any overlays
    paletteColor_t fillColor;
    
    /// @brief The fill color of any non-playable squares (for irregular grids)
    paletteColor_t voidColor;

    /// @brief The line color of the border around the entire grid
    paletteColor_t borderColor;

    /// @brief The line color of the lines separating rows and columns
    paletteColor_t gridColor;

    /// @brief The line color of the additional border separating boxes
    paletteColor_t boxBorderColor;
    
    /// @brief The text color of pre-filled numbers and notes
    paletteColor_t inkColor;

    /// @brief The text color of user-filled numbers
    paletteColor_t pencilColor;
    
    /// @brief The text color for the UI, not the grid
    paletteColor_t uiTextColor;

    /// @brief The color of the player's cursor
    paletteColor_t cursorColor;

    /// @brief The shape of the player's cursor
    sudokuOverlayShapeType_t cursorShape;
} sudokuTheme_t;

typedef struct
{
    sudokuShapeTag_t tag;
    paletteColor_t color;
    sudokuOverlayShapeType_t type;

    union {
        rectangle_t rectangle;
        circle_t circle;
        line_t line;
        arrow_t arrow;
        const char* text;
    };
} sudokuOverlayShape_t;

/// @brief Holds information for drawing all sorts of overlay data
typedef struct
{
    /// @brief A bitmask of options for each grid in the game
    sudokuOverlayOpt_t* gridOpts;

    /// @brief A list of dynamic shapes to draw on top of the grid
    list_t shapes;
} sudokuOverlay_t;

typedef struct
{
    //< Cursor position
    uint8_t curX, curY;

    /// @brief Digit selected to be entered
    uint8_t selectedDigit;

    /// @brief Whether the player is taking notes or setting digits
    bool noteTaking;

    /// @brief A grid-sized array of notes for each cell
    uint16_t* notes;

    /// @brief A set of overlay data to show this player
    sudokuOverlay_t overlay;

    /// @brief If the player is playing, this shape refers to the cursor
    sudokuOverlayShape_t* cursorShape;

    /// @brief Some sort of score for the player, TBD how it's calculated
    int32_t score;
} sudokuPlayer_t;

typedef struct
{
    /// @brief Large font for filled-out numbers
    font_t gridFont;

    /// @brief Tiny font for the notes grid
    font_t noteFont;

    /// @brief Font for UI text
    font_t uiFont;

    sudokuScreen_t screen;
    sudokuGrid_t game;
    int64_t playTimer;

    sudokuPlayer_t player;

    menu_t* menu;
    menuManiaRenderer_t* menuRenderer;
} swadgedoku_t;

//==============================================================================
// Function Declarations
//==============================================================================

static void swadgedokuEnterMode(void);
static void swadgedokuExitMode(void);
static void swadgedokuMainLoop(int64_t elapsedUs);
static void swadgedokuBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);

static void swadgedokuMainMenuCb(const char* label, bool selected, uint32_t value);

void deinitSudokuGame(sudokuGrid_t* game);
bool setupSudokuGame(sudokuGrid_t* game, sudokuMode_t mode, int base, int size);
void setupSudokuPlayer(sudokuPlayer_t* player, const sudokuGrid_t* game);
void sudokuReevaluatePeers(uint16_t* notes, const sudokuGrid_t* game, int row, int col, int flags);
void sudokuGetNotes(uint16_t* notes, const sudokuGrid_t* game, int flags);
void swadgedokuGameButton(buttonEvt_t evt);
void swadgedokuDrawGame(const sudokuGrid_t* game, const uint16_t* notes, const sudokuOverlay_t* overlay, const sudokuTheme_t* theme);

bool setDigit(sudokuGrid_t* game, uint8_t number, uint8_t x, uint8_t y);


//==============================================================================
// Const data
//==============================================================================

static const char swadgedokuModeName[] = "Swadgedoku";
static const char menuItemPlaySudoku[] = "Play Sudoku";

static const sudokuTheme_t lightTheme = {
    .bgColor = c333,
    .fillColor = c555,
    .voidColor = c333,
    .borderColor = c000,
    .gridColor = c222,
    .boxBorderColor = c000,
    .inkColor = c000,
    .pencilColor = c222,
    .uiTextColor = c000,
    .cursorColor = c505,
    .cursorShape = OVERLAY_CIRCLE
};

//==============================================================================
// Variables
//==============================================================================

swadgeMode_t swadgedokuMode = {
    .modeName                 = swadgedokuModeName,
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = false,
    .usesAccelerometer        = false,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = swadgedokuEnterMode,
    .fnExitMode               = swadgedokuExitMode,
    .fnMainLoop               = swadgedokuMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = swadgedokuBackgroundDrawCallback,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL,
    .fnDacCb                  = NULL,
};

static swadgedoku_t* sd = NULL;

//==============================================================================
// Functions
//==============================================================================

static void swadgedokuEnterMode(void)
{
    sd = calloc(1, sizeof(swadgedoku_t));

    sd->screen = SWADGEDOKU_MAIN_MENU;

    loadFont(RADIOSTARS_FONT, &sd->gridFont, true);
    loadFont(TINY_NUMBERS_FONT, &sd->noteFont, true);
    loadFont(SONIC_FONT, &sd->uiFont, true);

    sd->menu = initMenu(swadgedokuModeName, swadgedokuMainMenuCb);
    sd->menuRenderer = initMenuManiaRenderer(NULL, NULL, NULL);

    addSingleItemToMenu(sd->menu, menuItemPlaySudoku);
}

static void swadgedokuExitMode(void)
{
    deinitMenuManiaRenderer(sd->menuRenderer);
    deinitMenu(sd->menu);
    free(sd);
    sd = NULL;
}

static void swadgedokuMainLoop(int64_t elapsedUs)
{
    switch (sd->screen)
    {
        case SWADGEDOKU_MAIN_MENU:
        {
            buttonEvt_t evt = {0};
            while (checkButtonQueueWrapper(&evt))
            {
                sd->menu = menuButton(sd->menu, evt);
            }

            drawMenuMania(sd->menu, sd->menuRenderer, elapsedUs);
            break;
        }

        case SWADGEDOKU_GAME:
        {
            buttonEvt_t evt = {0};
            while (checkButtonQueueWrapper(&evt))
            {
                swadgedokuGameButton(evt);
            }

            swadgedokuDrawGame(&sd->game, sd->game.notes, &sd->player.overlay, &lightTheme);
            
            if (sd->player.selectedDigit)
            {
                char curDigitStr[16];
                snprintf(curDigitStr, sizeof(curDigitStr), "%" PRIX8, sd->player.selectedDigit);
                int textW = textWidth(&sd->gridFont, curDigitStr);

                drawText(&sd->gridFont, lightTheme.uiTextColor, curDigitStr, TFT_WIDTH - 5 - textW, (TFT_HEIGHT - sd->gridFont.height) / 2);
            }
            break;
        }
    }
}

static void swadgedokuBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
    
}

static void swadgedokuMainMenuCb(const char* label, bool selected, uint32_t value)
{
    if (selected)
    {
        if (menuItemPlaySudoku == label)
        {
            if (!setupSudokuGame(&sd->game, SM_CLASSIC, 9, 9))
            {
                ESP_LOGE("Swadgedoku", "Couldn't setup game???");
                return;
            }
            setupSudokuPlayer(&sd->player, &sd->game);
            sd->game.grid[0] = 9;
            sd->game.flags[0] = SF_LOCKED;

            sudokuGetNotes(sd->game.notes, &sd->game, 0);

            sd->player.notes[1] = 127;
            sd->game.flags[8] = SF_VOID;
            //sd->game.grid[2] = 8;
            setDigit(&sd->game, 8, 2, 0);
            sd->screen = SWADGEDOKU_GAME;
        }
    }
}

void deinitSudokuGame(sudokuGrid_t* game)
{
    if (game->boxMap != NULL)
    {
        free(game->boxMap);
        game->boxMap = NULL;
    }

    if (game->flags != NULL)
    {
        free(game->flags);
        game->flags = NULL;
    }

    if (game->notes != NULL)
    {
        free(game->notes);
        game->notes = NULL;
    }

    if (game->grid != NULL)
    {
        free(game->grid);
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
        {
            if (size < base)
            {
                ESP_LOGE("Swadgedoku", "%1$dx%1$d Grid not large enough for base %2$d", size, base);
                return false;
            }

            // Allocate all the things
            int totalSquares = size * size;

            uint8_t* grid = calloc(totalSquares, sizeof(uint8_t));

            if (!grid)
            {
                return false;
            }
            
            sudokuFlag_t* flags = calloc(totalSquares, sizeof(sudokuFlag_t));
            if (!flags)
            {
                free(grid);
                return false;
            }
            
            uint16_t* notes = calloc(totalSquares, sizeof(uint16_t));
            if (!notes)
            {
                free(flags);
                free(grid);
                return false;
            }

            uint8_t* boxMap = calloc(totalSquares, sizeof(uint8_t));
            if (!boxMap)
            {
                free(notes);
                free(flags);
                free(grid);
                return false;
            }

            game->grid = grid;
            game->flags = flags;
            game->notes = notes;
            game->boxMap = boxMap;

            game->mode = mode;
            game->size = size;
            game->base = base;

            // Now, define the boxes!

            // First, check if the base is a square.
            // It's much easier if it is...

            int baseRoot = 0;
            /*switch (base)
            {
                case 1: baseRoot = 1; break;
                case 4: baseRoot = 2; break;
                case 9: baseRoot = 3; break;
                case 16: baseRoot = 4; break;
                default: break;
            }*/
            
            if (baseRoot != 0)
            {
                // Setup square boxes!
                for (int box = 0; box < base; box++)
                {
                    for (int n = 0; n < base; n++)
                    {
                        int x = (box % baseRoot) * baseRoot + (n % baseRoot);
                        int y = (box / baseRoot) * baseRoot + (n / baseRoot);
                        game->boxMap[y * size + x] = box;
                    }
                }
            }
            else
            {
                uint16_t totalSquares = game->base * game->base;
                uint8_t boxSquareCounts[game->base];
                uint16_t assignedSquareCount = 0;

                uint16_t adjacencyMap[game->base];

                memset(boxSquareCounts, 0, game->base * sizeof(uint8_t));
                memset(adjacencyMap, 0, game->base * sizeof(uint16_t));

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
                int dir = 0;
                int minX = 0;
                int minY = 0;
                int maxX = game->size - 1;
                int maxY = game->size - 1;
                for (int x = 0, y = 0;;)
                {
                    game->boxMap[y * size + x] = curBox;

                    // Check for adjacent squares
                    if (x > 0)
                    {
                        uint8_t leftBox = game->boxMap[y * game->size + x - 1];

                        if (leftBox != BOX_NONE)
                        {
                            adjacencyMap[leftBox] |= (1 << curBox);
                            adjacencyMap[curBox] |= (1 << leftBox);
                        }
                    }

                    if (y > 0)
                    {
                        uint8_t topBox = game->boxMap[(y - 1) * game->size + x];
                        if (topBox != BOX_NONE)
                        {
                            adjacencyMap[topBox] |= (1 << curBox);
                            adjacencyMap[curBox] |= (1 << topBox);
                        }
                    }

                    if (x < game->size - 1)
                    {
                        uint8_t rightBox = game->boxMap[y * game->size + x + 1];
                        if (rightBox != BOX_NONE)
                        {
                            adjacencyMap[rightBox] |= (1 << curBox);
                            adjacencyMap[curBox] |= (1 << rightBox);
                        }
                    }

                    if (y < game->size - 1)
                    {
                        uint8_t bottomBox = game->boxMap[(y + 1) * game->size + x];
                        if (bottomBox != BOX_NONE)
                        {
                            adjacencyMap[bottomBox] |= (1 << curBox);
                            adjacencyMap[curBox] |= (1 << bottomBox);
                        }
                    }

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

                // Start randomly permuting the squares
                uint8_t boxA = (esp_random() % game->base);
                uint8_t boxB = (esp_random() % (game->base - 1));
                if (boxB >= boxA) { boxB++; }
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
        free(player->notes);
        player->notes = NULL;
    }

    // TODO: Have an init/deinit function for overlays
    if (player->overlay.gridOpts != NULL)
    {
        free(player->overlay.gridOpts);
        player->overlay.gridOpts = NULL;
    }

    player->cursorShape = NULL;

    sudokuOverlayShape_t* shape = NULL;
    while (NULL != (shape = pop(&player->overlay.shapes)))
    {
        free(shape);
    }

    player->notes = calloc(game->size * game->size, sizeof(uint16_t));
    player->overlay.gridOpts = calloc(game->size * game->size, sizeof(sudokuOverlayOpt_t));

    player->cursorShape = calloc(1, sizeof(sudokuOverlayShape_t));
    player->cursorShape->tag = ST_CURSOR;
    player->cursorShape->color = c505;
    //player->cursorShape->type = OVERLAY_RECT;
    player->cursorShape->type = OVERLAY_CIRCLE;
    player->cursorShape->circle.pos.x = player->curX;
    player->cursorShape->circle.pos.y = player->curY;
    player->cursorShape->circle.radius = 1;
    //player->cursorShape->rectangle.pos.x = player->curX;
    //player->cursorShape->rectangle.pos.y = player->curY;
    //player->cursorShape->rectangle.width = 1;
    //player->cursorShape->rectangle.height = 1;
    push(&player->overlay.shapes, player->cursorShape);
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
    
    // First pass: construct row/column possibiliies
    for (int r = 0; r < game->size; r++)
    {
        for (int c = 0; c < game->size; c++)
        {
            uint16_t box = game->boxMap[r * game->size + c];
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
                sourceBoxRows[boxCount] = r;
                sourceBoxCols[boxCount++] = c;
            }
        }
    }

    // Second pass: apply changes to notes
    // We don't need to do a full grid sweep though, just the peers (as the function name suggests)

    // First, just the column -- this will include the target square itself
    for (int r = 0; r < game->size; r++)
    {
        uint16_t box = game->boxMap[r * game->size + col];
        uint16_t boxNote = (box < game->base) ? boxNotes[box] : allNotes;
        notes[r * game->size + col] = (rowNotes[r] & colNotes[col] & boxNote);
        if (!notes[r * game->size + col])
        {
            ESP_LOGW("Swadgedoku", "Cell r=%d, c=%d has no valid entries!", r, col);
        }
    }

    // Next, just the row, skipping the square we already did (the target square)
    for (int c = 0; c < game->size; c++)
    {
        // Skip the source column, it's already done by the previous loop
        if (c == col) { continue; }

        uint16_t box = game->boxMap[row * game->size + c];
        uint16_t boxNote = (box < game->base) ? boxNotes[box] : allNotes;
        notes[row * game->size + c] = (rowNotes[row] & colNotes[c] & boxNote);
        if (!notes[row * game->size + c])
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

            ESP_LOGI("Swadgedoku", "Box[r=%d][c=%d] == sourceBox (%" PRIu16 ")", r, c, sourceBox);
            notes[r * game->size + c] = (rowNotes[r] & colNotes[c] & sourceBoxNote);
            if (!notes[r * game->size + c])
            {
                ESP_LOGW("Swadgedoku", "Cell r=%d, c=%d has no valid entries!", r, c);
            }
        }
    }
}

void sudokuGetNotes(uint16_t* notes, const sudokuGrid_t* game, int flags)
{
    // Summaries of the possibilities for each row, box, and column
    // We will construct these from
    uint16_t rowNotes[game->size];
    uint16_t colNotes[game->size];
    uint16_t boxNotes[game->base];

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
            uint8_t box = game->boxMap[row * game->size + col];
            uint8_t digit = game->grid[row * game->size + col];

            if (digit != 0)
            {
                uint16_t digitUnmask = ~(1 << (digit - 1));
                ESP_LOGE("Swadgedoku", "Unmasking %" PRIu8 " into %" PRIX16, digit, digitUnmask);
                rowNotes[row] &= digitUnmask;
                colNotes[col] &= digitUnmask;

                if (BOX_NONE != box && box < game->base)
                {
                    boxNotes[box] &= digitUnmask;
                }
            }
        }
    }

    for (int row = 0; row < game->size; row++)
    {
        for (int col = 0; col < game->size; col++)
        {
            uint8_t box = game->boxMap[row * game->size + col];
            uint8_t digit = game->grid[row * game->size + col];

            if (digit == 0)
            {
                uint16_t boxNote = (box < game->base) ? boxNotes[box] : allNotes;
                notes[row * game->size + col] = (rowNotes[row] & colNotes[col] & boxNote);
            }
            else
            {
                notes[row * game->size + col] = 0;
            }
        }
    }
}

// Annotate the overlay with
void sudokuAnnotate(sudokuOverlay_t* overlay, const sudokuGrid_t* grid)
{
    // 1. Remove all the existing annotations
    node_t* next = NULL;
    for (node_t* node = overlay->shapes.first; node != NULL; node = next)
    {
        sudokuOverlayShape_t* shape = (sudokuOverlayShape_t*)node->val;
        // Save the next node pointer in case we remove the current one
        next = node->next;

        if (shape->tag == ST_ANNOTATE)
        {
            removeEntry(&overlay->shapes, node);
            free(shape);
        }
    }

    // TODO other stuff
}

void swadgedokuGameButton(buttonEvt_t evt)
{
    if (evt.down)
    {
        switch (evt.button)
        {
            case PB_A:
            {
                if (sd->player.selectedDigit && !((SF_LOCKED | SF_VOID) & sd->game.flags[sd->player.curY * sd->game.size + sd->player.curX]))
                {
                    // Not locked or void, proceed setting the digit
                    //sd->game.grid[sd->player.curY * sd->game.size + sd->player.curX] = sd->player.selectedDigit;
                    setDigit(&sd->game, sd->player.selectedDigit, sd->player.curX, sd->player.curY);
                }
                break;
            }

            case PB_B:
            {
                sd->player.selectedDigit = (sd->player.selectedDigit) % (sd->game.base) + 1;
                break;
            }

            case PB_SELECT:
            {
                break;
            }

            case PB_START:
            {
                break;
            }

            case PB_UP:
            {
                do {
                    if (sd->player.curY == 0)
                    {
                        sd->player.curY = sd->game.size - 1;
                    }
                    else
                    {
                        sd->player.curY--;
                    }
                } while (SF_VOID & (sd->game.flags[sd->player.curY * sd->game.size + sd->player.curX]));
                break;
            }

            case PB_DOWN:
            {
                do {
                    if (sd->player.curY >= sd->game.size - 1)
                    {
                        sd->player.curY = 0;
                    }
                    else
                    {
                        sd->player.curY++;
                    }
                } while (SF_VOID & (sd->game.flags[sd->player.curY * sd->game.size + sd->player.curX]));
                break;
            }

            case PB_LEFT:
            {
                do {
                    if (sd->player.curX == 0)
                    {
                        sd->player.curX = sd->game.size - 1;
                    }
                    else
                    {
                        sd->player.curX--;
                    }
                } while (SF_VOID & (sd->game.flags[sd->player.curY * sd->game.size + sd->player.curX]));
                break;
            }

            case PB_RIGHT:
            {
                do {
                    if (sd->player.curX >= sd->game.size - 1)
                    {
                        sd->player.curX = 0;
                    }
                    else
                    {
                        sd->player.curX++;
                    }
                } while (SF_VOID & (sd->game.flags[sd->player.curY * sd->game.size + sd->player.curX]));
                break;
            }
        }

        if (sd->player.cursorShape)
        {
            sd->player.cursorShape->rectangle.pos.x = sd->player.curX;
            sd->player.cursorShape->rectangle.pos.y = sd->player.curY;
        }
    }
}

void swadgedokuDrawGame(const sudokuGrid_t* game, const uint16_t* notes, const sudokuOverlay_t* overlay, const sudokuTheme_t* theme)
{
    // Total space around the grid
    int gridMargin = 1;

    // Max size of individual square
    int maxSquareSize = (TFT_HEIGHT - gridMargin) / game->size;
    // Total size of the grid (add 1px for border)
    int gridSize = game->size * maxSquareSize;

    // Take off 1px for border and 2px for 1px of padding on each side
    int squareInterior = maxSquareSize - 3;

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
        drawLineFast(gridX + col * maxSquareSize, gridY + 1, gridX + col * maxSquareSize, gridY + gridSize - 1, theme->gridColor);
    }

    // Draw lines between the rows
    for (int row = 1; row < game->base; row++)
    {
        drawLineFast(gridX + 1, gridY + row * maxSquareSize, gridX + gridSize - 1, gridY + row * maxSquareSize, theme->gridColor);
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
            sudokuFlag_t flags = game->flags[r * game->size + c];

            if (flags & SF_VOID)
            {
                fillColor = voidColor;
            }
            else if (overlay)
            {
                opts = overlay->gridOpts[r * game->size + c];

                if (opts & OVERLAY_HIGHLIGHT_A)
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
                    int r = 0;
                    int g = 0;
                    int b = 0;

                    if (opts & OVERLAY_HIGHLIGHT_ROW)
                    {
                        // Cyan
                        g += 2;
                        b += 2;
                    }

                    if (opts & OVERLAY_HIGHLIGHT_COL)
                    {
                        // Blue
                        b += 3;
                    }

                    if (opts & OVERLAY_HIGHLIGHT_BOX)
                    {
                        // Yellow
                        r += 3;
                        g += 3;
                    }

                    if (opts & OVERLAY_ERROR)
                    {
                        r += 2;
                    }

                    if (r || g || b)
                    {
                        fillColor = r * 36 + g * 6 + b;
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
            if (r == 0 || game->boxMap[(r-1) * game->size + c] != thisBox)
            {
                // Draw north border
                drawLineFast(x + 1, y + 1, x + maxSquareSize - 1, y + 1, theme->borderColor);
            }
            // east
            if (c == (game->size - 1) || game->boxMap[r * game->size + c + 1] != thisBox)
            {
                // Draw east border
                drawLineFast(x + maxSquareSize - 1, y + 1, x + maxSquareSize - 1, y + maxSquareSize - 1, theme->borderColor);
            }
            // south
            if (r == (game->size - 1) || game->boxMap[(r+1) * game->size + c] != thisBox)
            {
                // Draw south border
                drawLineFast(x + 1, y + maxSquareSize - 1, x + maxSquareSize - 1, y + maxSquareSize - 1, theme->borderColor);
            }
            // west
            if (c == 0 || game->boxMap[r * game->size + c - 1] != thisBox)
            {
                // Draw west border
                drawLineFast(x + 1, y + 1, x + 1, y + maxSquareSize - 1, theme->borderColor);
            }

            uint16_t squareVal = game->grid[r * game->size + c];

            // NOW! Draw the number, or the notes
            if (NULL != notes && 0 == squareVal)
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
                        //int charW = textWidth(&sd->noteFont, buf);

                        int baseRoot = 3;
                        switch (game->base)
                        {
                            case 1: baseRoot = 1; break;

                            case 2:
                            case 3:
                            case 4: baseRoot = 2; break;

                            case 10:
                            case 11:
                            case 12:
                            case 13:
                            case 14:
                            case 15:
                            case 16: baseRoot = 4; break;
                            default: break;
                        }

                        int miniSquareSize = maxSquareSize / baseRoot;
                        
                        int noteX = x + (n % baseRoot) * maxSquareSize / baseRoot + (miniSquareSize - textWidth(&sd->noteFont, buf)) / 2 + 1;
                        int noteY = y + (n / baseRoot) * maxSquareSize / baseRoot + (miniSquareSize - sd->noteFont.height) / 2 + 2;

                        drawText(&sd->noteFont, theme->inkColor, buf, noteX, noteY);
                    }
                }
            }
            else if (0 != squareVal)
            {
                // Draw number
                char buf[16];
                snprintf(buf, sizeof(buf), "%X", squareVal);
                
                int textX = x + (maxSquareSize - textWidth(&sd->gridFont, buf)) / 2;
                int textY = y + (maxSquareSize - sd->gridFont.height) / 2;
                
                paletteColor_t color = (flags & SF_LOCKED) ? theme->inkColor : theme->pencilColor;
                if (overlay)
                {
                    if (opts & OVERLAY_ERROR)
                    {
                        color = c500;
                    }
                    else if (opts & OVERLAY_CHECK)
                    {
                        color = c050;
                    }
                }

                drawText(&sd->gridFont, color, buf, textX, textY);
            }

            // Check the overlay again, for more info
            if (overlay)
            {
                if (opts & OVERLAY_NOTES)
                {
                    // TODO function-ify the notes draw-er and put it here
                }
                if (opts & OVERLAY_CHECK)
                {
                    // TODO: Draw a checkbox
                }
                if (opts & OVERLAY_QUESTION)
                {
                    // TODO: Draw a checkbox
                }
                if (opts & OVERLAY_CROSS_OUT)
                {
                    // TODO: Draw a big X
                }
                if (opts & OVERLAY_ERROR)
                {
                    // TODO: ??? Probably good tbh
                }
            }
        }
    }

    if (overlay)
    {
        for (node_t* node = overlay->shapes.first; node != NULL; node = node->next)
        {
            const sudokuOverlayShape_t* shape = (sudokuOverlayShape_t*)node->val;
            
            switch (shape->type)
            {
                case OVERLAY_RECT:
                drawRect(gridX + maxSquareSize * shape->rectangle.pos.x, gridY + maxSquareSize * shape->rectangle.pos.y, gridX + maxSquareSize * (shape->rectangle.pos.x + shape->rectangle.width), gridY + maxSquareSize * (shape->rectangle.pos.y + shape->rectangle.height), shape->color);
                break;

                case OVERLAY_CIRCLE:
                drawCircle(gridX + maxSquareSize * shape->circle.pos.x + maxSquareSize / 2, gridY + maxSquareSize * shape->circle.pos.y + maxSquareSize / 2, shape->circle.radius * maxSquareSize * 3 / 5, shape->color);
                break;

                case OVERLAY_LINE:
                drawLineFast(
                    gridX + maxSquareSize + shape->line.p1.x * maxSquareSize + maxSquareSize / 2,
                    gridY + maxSquareSize * shape->line.p1.y * maxSquareSize + maxSquareSize / 2,
                    gridX + maxSquareSize * shape->line.p2.x * maxSquareSize + maxSquareSize / 2,
                    gridY + maxSquareSize * shape->line.p2.y * maxSquareSize + maxSquareSize / 2,
                    shape->color
                );
                break;

                case OVERLAY_ARROW:
                {
                    drawLineFast(
                        gridX + maxSquareSize + shape->arrow.base.x * maxSquareSize + maxSquareSize / 2,
                        gridY + maxSquareSize * shape->arrow.base.y * maxSquareSize + maxSquareSize / 2,
                        gridX + maxSquareSize * shape->arrow.tip.x * maxSquareSize + maxSquareSize / 2,
                        gridY + maxSquareSize * shape->arrow.tip.y * maxSquareSize + maxSquareSize / 2,
                        shape->color
                    );
                    drawLineFast(
                        gridX + maxSquareSize + shape->arrow.wing1.x * maxSquareSize + maxSquareSize / 2,
                        gridY + maxSquareSize * shape->arrow.wing1.y * maxSquareSize + maxSquareSize / 2,
                        gridX + maxSquareSize * shape->arrow.tip.x * maxSquareSize + maxSquareSize / 2,
                        gridY + maxSquareSize * shape->arrow.tip.y * maxSquareSize + maxSquareSize / 2,
                        shape->color
                    );
                    drawLineFast(
                        gridX + maxSquareSize + shape->arrow.wing2.x * maxSquareSize + maxSquareSize / 2,
                        gridY + maxSquareSize * shape->arrow.wing2.y * maxSquareSize + maxSquareSize / 2,
                        gridX + maxSquareSize * shape->arrow.tip.x * maxSquareSize + maxSquareSize / 2,
                        gridY + maxSquareSize * shape->arrow.tip.y * maxSquareSize + maxSquareSize / 2,
                        shape->color
                    );
                    break;
                }

                case OVERLAY_TEXT:
                {
                    // TODO
                }
                break;
            }
        }
    }
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
