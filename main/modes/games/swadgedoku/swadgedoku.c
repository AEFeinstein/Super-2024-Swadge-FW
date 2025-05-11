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

typedef struct
{
    enum {
        OVERLAY_RECT,
        OVERLAY_CIRCLE,
        OVERLAY_LINE,
        OVERLAY_ARROW,
        OVERLAY_TEXT,
    } type;

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
void swadgedokuDrawGame(const sudokuGrid_t* game, const uint16_t* notes, const sudokuOverlay_t* overlay, const paletteColor_t borderCol, paletteColor_t gridColor, paletteColor_t bgColor);

//==============================================================================
// Const data
//==============================================================================

static const char swadgedokuModeName[] = "Swadgedoku";
static const char menuItemPlaySudoku[] = "Play Sudoku";

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
            swadgedokuDrawGame(&sd->game, sd->player.notes, &sd->player.overlay, c000, c222, c555);
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
            }
            setupSudokuPlayer(&sd->player, &sd->game);
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
            // Basically, this is the size of the square boxes
            // And also the number of boxes per row
            int baseRoot = 0;

            switch (base)
            {
                case 4:
                {
                    baseRoot = 2;
                    break;
                }
                case 9:
                {
                    baseRoot = 3;
                    break;
                }
                case 16:
                {
                    baseRoot = 4;
                    break;
                }
                // OK, cool
                break;
                
                default:
                ESP_LOGE("Swadgedoku", "Non-square base %d not supported by classic sudoku", base);
                return false;
            }

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

    sudokuOverlayShape_t* shape = NULL;
    while (NULL != (shape = pop(&player->overlay.shapes)))
    {
        free(shape);
    }

    player->notes = calloc(game->size * game->size, sizeof(uint16_t));
    player->overlay.gridOpts = calloc(game->size * game->size, sizeof(sudokuOverlayOpt_t));
}

void swadgedokuDrawGame(const sudokuGrid_t* game, const uint16_t* notes, const sudokuOverlay_t* overlay, paletteColor_t borderCol, paletteColor_t gridColor, paletteColor_t bgColor)
{
    // Max size of individual square
    int maxSquareSize = (TFT_HEIGHT - 1) / game->size;
    // Total size of the grid (add 1px for border)
    int gridSize = game->size * maxSquareSize;

    // Take off 1px for border and 2px for 1px of padding on each side
    int squareInterior = maxSquareSize - 3;

    // Center the grid vertically
    int gridY = (TFT_HEIGHT - gridSize) / 2;
    
    // Align the grid to the left to leave some space to the right for UI
    int gridX = (TFT_WIDTH - gridSize) / 2;

    paletteColor_t voidColor = c333;

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
    drawRect(gridX, gridY, gridX + gridSize + 1, gridY + gridSize + 1, borderCol);

    // Draw lines between the columns
    for (int col = 1; col < game->base; col++)
    {
        drawLineFast(gridX + col * maxSquareSize, gridY + 1, gridX + col * maxSquareSize, gridY + gridSize - 1, gridColor);
    }

    // Draw lines between the rows
    for (int row = 1; row < game->base; row++)
    {
        drawLineFast(gridX + 1, gridY + row * maxSquareSize, gridX + gridSize - 1, gridY + row * maxSquareSize, gridColor);
    }

    // Draw extra borders around the boxes and fill in the background
    for (int r = 0; r < game->size; r++)
    {
        for (int c = 0; c < game->size; c++)
        {
            int x = gridX + c * maxSquareSize;
            int y = gridY + r * maxSquareSize;

            paletteColor_t fillColor = bgColor;

            sudokuOverlayOpt_t opts = OVERLAY_NONE;
            sudokuFlag_t flags = game->flags[r * game->size + c];

            if (overlay)
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

                    fillColor = r * 36 + g * 6 + b;
                }
            }

            fillDisplayArea(x + 1, y + 1, x + maxSquareSize, y + maxSquareSize, bgColor);

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
                drawLineFast(x + 1, y + 1, x + maxSquareSize - 1, y + 1, borderCol);
            }
            // east
            if (c == (game->size - 1) || game->boxMap[r * game->size + c + 1] != thisBox)
            {
                // Draw east border
                drawLineFast(x + maxSquareSize - 1, y + 1, x + maxSquareSize - 1, y + maxSquareSize - 1, borderCol);
            }
            // south
            if (r == (game->size - 1) || game->boxMap[(r+1) * game->size + c] != thisBox)
            {
                // Draw south border
                drawLineFast(x + 1, y + maxSquareSize - 1, x + maxSquareSize - 1, y + maxSquareSize - 1, borderCol);
            }
            // west
            if (c == 0 || game->boxMap[r * game->size + c - 1] != thisBox)
            {
                // Draw west border
                drawLineFast(x + 1, y + 1, x + 1, y + maxSquareSize - 1, borderCol);
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

                        drawText(&sd->noteFont, c000, buf, noteX, noteY);
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
                
                paletteColor_t color = (flags & SF_LOCKED) ? c000 : c444;
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
                break;

                case OVERLAY_CIRCLE:
                break;

                case OVERLAY_LINE:
                break;

                case OVERLAY_ARROW:
                break;

                case OVERLAY_TEXT:
                break;
            }
        }
    }
}