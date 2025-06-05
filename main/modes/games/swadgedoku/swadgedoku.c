//==============================================================================
// Includes
//==============================================================================

#include "swadgedoku.h"

#include "menu.h"
#include "wheel_menu.h"

//==============================================================================
// Defines
//==============================================================================

// There are exactly two unique binary sudoku games... And we have both!
#define SUDOKU_MIN_BASE 2
#define SUDOKU_MAX_BASE 16

/// @brief Value for when a square does not map to any box, such as a void square
#define BOX_NONE UINT8_MAX

#define SUDOKU_PUZ_MIN SUDOKU_PUZ_000_BSP
#define SUDOKU_PUZ_MAX SUDOKU_PUZ_002_BSP

// SUBPOS_* defines used for positioning overlays within the sudoku board
#define BOX_SIZE_SUBPOS 13

/*
 * @brief Computes a subposition value for the given x and y, in 13ths-of-a-square
 *
 */
// #define SUBPOS_XY(x, y) (((y) * BOX_SIZE_SUBPOS) + (x))

#define ONE_SECOND_IN_US (1000000)

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    SWADGEDOKU_MAIN_MENU = 0,
    SWADGEDOKU_GAME      = 1,
    SWADGEDOKU_WIN       = 2,
    SWADGEDOKU_PAUSE     = 3,
} sudokuScreen_t;

typedef enum
{
    /// @brief Your classic everyday sudoku with a square grid and boxes
    SM_CLASSIC = 0,
    /// @brief Same rules as classic, but with irregularly shaped boxes
    SM_JIGSAW,
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
    SD_BEGINNER = 0,
    SD_EASY     = 1,
    SD_MEDIUM   = 2,
    SD_HARD     = 3,
    SD_EXPERT   = 4,
    SD_HARDEST  = 5,
} sudokuDifficulty_t;

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

typedef enum
{
    SUBPOS_CENTER = (6 * 13 + 6),
    SUBPOS_NW     = (0),
    SUBPOS_NNW    = (3),
    SUBPOS_N      = (6),
    SUBPOS_NNE    = (9),
    SUBPOS_NE     = (12),
    SUBPOS_WNW    = (3 * 16),
    SUBPOS_W      = (6 * 16),
    SUBPOS_WSW    = (9 * 16),
    SUBPOS_SW     = (12 * 13),
    SUBPOS_SSW    = (12 * 13 + 3),
    SUBPOS_S      = (12 * 13 + 6),
    SUBPOS_SSE    = (12 * 13 + 9),
    SUBPOS_SE     = (12 * 13 + 12),
    SUBPOS_ENE    = (3 * 13 * 12),
    SUBPOS_E      = (6 * 13 + 12),
    SUBPOS_ESE    = (9 * 13 + 12),
} sudokuSubpos_t;

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
    /// Can be anywhere from 2 to 16, though who knows if I'll do non-square bases with irregular boxes (like killer
    /// sudoku?)
    uint8_t base;

    /// @brief The box configuration for this grid
    // sudokuBox_t* boxes;

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

    union
    {
        rectangle_t rectangle;
        circle_t circle;
        line_t line;
        arrow_t arrow;

        struct
        {
            vec_t pos;
            const char* val;
            bool center;
        } text;
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

    /// @brief The last (or next) level to play, the default to select
    int lastLevel;

    /// @brief The maximum unlocked level
    int maxLevel;

    int randState;

    sudokuPlayer_t player;

    menu_t* menu;
    menuManiaRenderer_t* menuRenderer;

    menu_t* emptyMenu;
    char emptyMenuTitle[32];

    menuItem_t* customModeMenuItem;
    menuItem_t* customSizeMenuItem;
    menuItem_t* customDifficultyMenuItem;

    bool jigsawForced;
    sudokuMode_t prevMode;
    bool squareForced;
    int prevSize;

    // Difficulty of the current game, or -1 if N/A
    int currentDifficulty;
    int currentMode;

    menu_t* numberWheel;
    wheelMenuRenderer_t* numberWheelRenderer;
    rectangle_t wheelMenuTextBox;
    bool touchState;

    menu_t* pauseMenu;
} swadgedoku_t;

typedef struct
{
    int timeLimit;
    int difficulty;
    int mode;
} swadgedokuTrophyTrigger_t;

//==============================================================================
// Function Declarations
//==============================================================================

static void swadgedokuEnterMode(void);
static void swadgedokuExitMode(void);
static void swadgedokuMainLoop(int64_t elapsedUs);
static void swadgedokuBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);

static void swadgedokuSetupMenu(void);
static void swadgedokuMainMenuCb(const char* label, bool selected, uint32_t value);
static void swadgedokuPauseMenuCb(const char* label, bool selected, uint32_t value);
static void swadgedokuSetupNumberWheel(int base, uint16_t disableMask);
static void numberWheelCb(const char* label, bool selected, uint32_t value);

bool loadSudokuData(const uint8_t* data, size_t length, sudokuGrid_t* game);
size_t writeSudokuData(uint8_t* data, const sudokuGrid_t* game);
size_t getSudokuSaveSize(const sudokuGrid_t* game, int* boxFmt, int* gridFmt, int* noteFmt, int* flagFmt);

bool initSudokuGame(sudokuGrid_t* game, int size, int base, sudokuMode_t mode);
void deinitSudokuGame(sudokuGrid_t* game);
bool setupSudokuGame(sudokuGrid_t* game, sudokuMode_t mode, int base, int size);
void setupSudokuPlayer(sudokuPlayer_t* player, const sudokuGrid_t* game);
void sudokuReevaluatePeers(uint16_t* notes, const sudokuGrid_t* game, int row, int col, int flags);
void sudokuGetNotes(uint16_t* notes, const sudokuGrid_t* game, int flags);
void getOverlayPos(int32_t* x, int32_t* y, int r, int c, sudokuSubpos_t subpos);
void getRealOverlayPos(int16_t* x, int16_t* y, int gridX, int gridY, int squareSize, int xSubPos, int ySubPos);
void addCrosshairOverlay(sudokuOverlay_t* overlay, int r, int c, int gridSize, bool drawH, bool drawV,
                         sudokuShapeTag_t tag);
void sudokuAnnotate(sudokuOverlay_t* overlay, const sudokuPlayer_t* player, const sudokuGrid_t* game);
int swadgedokuCheckWin(const sudokuGrid_t* game);
void swadgedokuCheckTrophyTriggers(void);
void swadgedokuGameButton(buttonEvt_t evt);
void swadgedokuDrawGame(const sudokuGrid_t* game, const uint16_t* notes, const sudokuOverlay_t* overlay,
                        const sudokuTheme_t* theme);

int swadgedokuRand(int* seed);
int boxGetAdjacentSquares(uint16_t* indices, const sudokuGrid_t* game, uint8_t* aXs, uint8_t* aYs, uint8_t* bXs,
                          uint8_t* bYs);
bool squaresTouch(uint8_t ax, uint8_t ay, uint8_t bx, uint8_t by);

bool setDigit(sudokuGrid_t* game, uint8_t number, uint8_t x, uint8_t y);
void clearOverlayOpts(sudokuOverlay_t* overlay, const sudokuGrid_t* game, sudokuOverlayOpt_t optMask);

//==============================================================================
// Const data
//==============================================================================

static const char swadgedokuModeName[] = "Swadgedoku";
// Main menu
static const char menuItemContinue[]    = "Continue";
static const char menuItemLevelSelect[] = "Select Puzzle";
static const char menuItemPlaySudoku[]  = "Classic";
static const char menuItemPlayJigsaw[]  = "Jigsaw";
static const char menuItemPlayCustom[]  = "Infinite";
static const char menuItemStartCustom[] = "Start";
static const char menuItemCustomMode[]  = "Mode: ";
static const char menuItemCustomSize[]  = "Size: ";
static const char menuItemDifficulty[]  = "Rating: ";
static const char menuItemSettings[]    = "Settings";
// Pause menu
static const char strPaused[]             = "Paused";
static const char menuItemResume[]        = "Resume";
static const char menuItemExitPuzzle[]    = "Save and Exit";
static const char menuItemResetPuzzle[]   = "Reset Puzzle";
static const char menuItemAbandonPuzzle[] = "Abandon Puzzle";
// Number wheel title
static const char strSelectDigit[] = "Select Digit";
static const char strYouWin[]      = "You Win!";

static const int32_t menuOptValsCustomSize[] = {
    2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
};

static const char* const menuOptLabelsCustomSize[] = {
    "2x2",   "3x3",   "4x4",   "5x5",   "6x6",   "7x7",   "8x8",   "9x9",
    "10x10", "11x11", "12x12", "13x13", "14x14", "15x15", "16x16",
};

static const int32_t menuOptValsCustomMode[] = {
    SM_CLASSIC,
    SM_JIGSAW,
    SM_X_GRID,
};

static const char* const menuOptLabelsCustomMode[] = {
    "Classic",
    "Jigsaw",
    "X",
};

static const int32_t menuOptValsDifficulty[] = {
    SD_BEGINNER, SD_EASY, SD_MEDIUM, SD_HARD, SD_EXPERT, SD_HARDEST,
};

static const char* const menuOptLabelsDifficulty[] = {
    "Beginner", "Easy", "Medium", "Hard", "Expert", "Pain",
};

static const char* digitLabels[] = {
    "1",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",
    "8",
    "9",
    "A",
    "B",
    "C",
    "D",
    "E"
    "F",
    "0",
};

static const char settingKeyLastLevel[] = "sdku_lastlevel";
static const char settingKeyMaxLevel[]  = "sdku_maxlevel";
static const settingParam_t settingLevelSelectBounds
    = {.min = 1, .max = (SUDOKU_PUZ_MAX - SUDOKU_PUZ_MIN) + 1, .def = 1, .key = settingKeyLastLevel};

static const char settingKeyProgress[] = "sdku_progress";

// static const char settingKeyCustomSettings[] = "sdku_custom";
static const settingParam_t settingCustomSizeBounds
    = {.min = SUDOKU_MIN_BASE, .max = SUDOKU_MAX_BASE, .def = 9, .key = ""};

static const settingParam_t settingCustomModeBounds = {.min = 0, .max = 2, .def = 0, .key = ""};
static const settingParam_t settingDifficultyBounds = {.min = SD_BEGINNER, .max = SD_HARDEST, .def = 0, .key = ""};

static const sudokuTheme_t lightTheme = {.bgColor        = c333,
                                         .fillColor      = c555,
                                         .voidColor      = c333,
                                         .borderColor    = c000,
                                         .gridColor      = c222,
                                         .boxBorderColor = c000,
                                         .inkColor       = c000,
                                         .pencilColor    = c222,
                                         .uiTextColor    = c000,
                                         .cursorColor    = c505,
                                         .cursorShape    = OVERLAY_CIRCLE};

//==============================================================================
// Trophy Data
//==============================================================================
const swadgedokuTrophyTrigger_t anyPuzzleTrigger = {
    .difficulty = -1,
    .timeLimit  = -1,
    .mode       = -1,
};

const swadgedokuTrophyTrigger_t tenMinsTrigger = {
    .difficulty = -1,
    .timeLimit  = 600,
    .mode       = -1,
};

const swadgedokuTrophyTrigger_t fiveMinsTrigger = {
    .difficulty = -1,
    .timeLimit  = 300,
    .mode       = -1,
};

const trophyData_t swadgedokuTrophies[] = {
    {
        .title       = "It's a numbers game",
        .description = "Solve any Sudoku puzzle",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_EASY,
        .maxVal      = 1,
        .identifier  = &anyPuzzleTrigger,
    },
    {
        .title       = "Fast fingers",
        .description = "Solve a Sudoku in under 10 minutes",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_MEDIUM,
        .maxVal      = 1,
        .identifier  = &tenMinsTrigger,
    },
    {
        .title       = "Even faster fingers",
        .description = "Solve a Sudoku in under 5 minutes",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_HARD,
        .maxVal      = 1,
        .identifier  = &fiveMinsTrigger,
    },
};

// Individual mode settings
trophySettings_t swadgedokuTrophySettings = {
    .drawFromBottom   = false,
    .staticDurationUs = DRAW_STATIC_US * 2,
    .slideDurationUs  = DRAW_SLIDE_US,
};

// This is passed to the swadgeMode_t
trophyDataList_t swadgedokuTrophyData
    = {.settings = &swadgedokuTrophySettings, .list = swadgedokuTrophies, .length = ARRAY_SIZE(swadgedokuTrophies)};

// Aliases for trophies
const trophyData_t* trophySolveAny = &swadgedokuTrophies[0];
const trophyData_t* trophyTenMins  = &swadgedokuTrophies[1];
const trophyData_t* trophyFiveMins = &swadgedokuTrophies[2];

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
    .trophyData               = &swadgedokuTrophyData,
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

    if (!readNvs32(settingKeyMaxLevel, &sd->maxLevel))
    {
        sd->maxLevel = 1;
    }

    if (!readNvs32(settingKeyLastLevel, &sd->lastLevel))
    {
        sd->lastLevel = sd->maxLevel;
    }

    sd->menuRenderer = initMenuManiaRenderer(NULL, NULL, NULL);
    sd->emptyMenu    = initMenu(sd->emptyMenuTitle, NULL);

    sd->pauseMenu = initMenu(strPaused, swadgedokuPauseMenuCb);

    addSingleItemToMenu(sd->pauseMenu, menuItemResume);
    addSingleItemToMenu(sd->pauseMenu, menuItemResetPuzzle);
    addSingleItemToMenu(sd->pauseMenu, menuItemAbandonPuzzle);
    addSingleItemToMenu(sd->pauseMenu, menuItemExitPuzzle);

    swadgedokuSetupMenu();
}

static void swadgedokuExitMode(void)
{
    deinitSudokuGame(&sd->game);

    void* val = NULL;
    while (NULL != (val = pop(&sd->player.overlay.shapes)))
    {
        free(val);
    }

    free(sd->player.notes);
    free(sd->player.overlay.gridOpts);

    deinitWheelMenu(sd->numberWheelRenderer);
    deinitMenu(sd->numberWheel);

    deinitMenuManiaRenderer(sd->menuRenderer);
    deinitMenu(sd->menu);
    deinitMenu(sd->emptyMenu);
    deinitMenu(sd->pauseMenu);
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
            int32_t phi, r, intensity;
            if (getTouchJoystick(&phi, &r, &intensity))
            {
                wheelMenuTouch(sd->numberWheel, sd->numberWheelRenderer, phi, r);
                sd->touchState = true;
            }
            else if (sd->touchState)
            {
                wheelMenuTouchRelease(sd->numberWheel, sd->numberWheelRenderer);
                sd->touchState = false;
            }

            // Now that touches have been handled, check if the wheel is active
            bool inWheelMenu = wheelMenuActive(sd->numberWheel, sd->numberWheelRenderer);

            if (!inWheelMenu)
            {
                sd->playTimer += elapsedUs;
            }
            // TODO I started to put an else here but I forgot what was supposed to go in it

            buttonEvt_t evt = {0};
            while (checkButtonQueueWrapper(&evt))
            {
                if (inWheelMenu)
                {
                    wheelMenuButton(sd->numberWheel, sd->numberWheelRenderer, &evt);
                }
                else
                {
                    swadgedokuGameButton(evt);
                }
            }

            swadgedokuDrawGame(&sd->game, sd->game.notes, &sd->player.overlay, &lightTheme);

            if (sd->player.selectedDigit)
            {
                char curDigitStr[16];
                snprintf(curDigitStr, sizeof(curDigitStr), "%" PRIX8, sd->player.selectedDigit);
                int textW = textWidth(&sd->gridFont, curDigitStr);

                drawText(&sd->gridFont, lightTheme.uiTextColor, curDigitStr, TFT_WIDTH - 5 - textW,
                         (TFT_HEIGHT - sd->gridFont.height) / 2);
            }

            if (inWheelMenu)
            {
                shadeDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, 2, c111);
                drawRoundedRect(sd->wheelMenuTextBox.pos.x - 2, sd->wheelMenuTextBox.pos.y - 2,
                                sd->wheelMenuTextBox.pos.x + sd->wheelMenuTextBox.width + 2,
                                sd->wheelMenuTextBox.pos.y + sd->wheelMenuTextBox.height + 2, 5, c555, c000);
                drawWheelMenu(sd->numberWheel, sd->numberWheelRenderer, elapsedUs);
            }
            break;
        }

        case SWADGEDOKU_WIN:
        {
            strncpy(sd->emptyMenuTitle, strYouWin, sizeof(sd->emptyMenuTitle));
            drawMenuMania(sd->emptyMenu, sd->menuRenderer, elapsedUs);

            char playTime[64];
            int totalTime = sd->playTimer / ONE_SECOND_IN_US;

            int hours = totalTime / 3600;
            int mins  = (totalTime % 3600) / 60;
            int secs  = totalTime % 60;
            if (hours > 0)
            {
                snprintf(playTime, sizeof(playTime), "%d:%02d:%02d", hours, mins, secs);
            }
            else
            {
                snprintf(playTime, sizeof(playTime), "%02d:%02d", mins, secs);
            }

            const char* strCompletionTime = "Completed in";
            drawText(&sd->uiFont, c000, strCompletionTime, (TFT_WIDTH - textWidth(&sd->uiFont, strCompletionTime)) / 2,
                     TFT_HEIGHT / 2 - sd->uiFont.height - 1);
            drawText(&sd->uiFont, c000, playTime, (TFT_WIDTH - textWidth(&sd->uiFont, playTime)) / 2, TFT_HEIGHT / 2);

            buttonEvt_t evt = {0};
            if (checkButtonQueueWrapper(&evt) && evt.down)
            {
                // Reset the menu, return to main menu
                swadgedokuSetupMenu();
                sd->screen = SWADGEDOKU_MAIN_MENU;
            }
            break;
        }

        case SWADGEDOKU_PAUSE:
        {
            buttonEvt_t evt = {0};
            while (checkButtonQueueWrapper(&evt))
            {
                if (evt.down && (evt.button == PB_B || evt.button == PB_START))
                {
                    sd->screen = SWADGEDOKU_GAME;
                    break;
                }

                sd->pauseMenu = menuButton(sd->pauseMenu, evt);
            }

            drawMenuMania(sd->pauseMenu, sd->menuRenderer, elapsedUs);
            break;
        }
    }
}

static void swadgedokuBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
}

static void swadgedokuSetupMenu(void)
{
    //// Main Menu
    bool hasProgress  = false;
    size_t progLength = 0;
    if (readNvsBlob(settingKeyProgress, NULL, &progLength))
    {
        hasProgress = true;
    }

    if (sd->menu != NULL)
    {
        deinitMenu(sd->menu);
        sd->menu = NULL;
    }

    sd->menu = initMenu(swadgedokuModeName, swadgedokuMainMenuCb);

    if (hasProgress)
    {
        addSingleItemToMenu(sd->menu, menuItemContinue);
    }
    ESP_LOGE("Swadgedoku", "Last level is %d", sd->lastLevel);
    sd->menu = startSubMenu(sd->menu, menuItemPlaySudoku);
    addSettingsItemToMenu(sd->menu, menuItemLevelSelect, &settingLevelSelectBounds, sd->lastLevel);
    sd->menu = endSubMenu(sd->menu);

    addSingleItemToMenu(sd->menu, menuItemPlayJigsaw);

    sd->menu = startSubMenu(sd->menu, menuItemPlayCustom);
    addSingleItemToMenu(sd->menu, menuItemStartCustom);
    sd->customModeMenuItem
        = addSettingsOptionsItemToMenu(sd->menu, menuItemCustomMode, menuOptLabelsCustomMode, menuOptValsCustomMode,
                                       ARRAY_SIZE(menuOptValsCustomMode), &settingCustomModeBounds, 0);
    sd->customSizeMenuItem
        = addSettingsOptionsItemToMenu(sd->menu, menuItemCustomSize, menuOptLabelsCustomSize, menuOptValsCustomSize,
                                       ARRAY_SIZE(menuOptValsCustomSize), &settingCustomSizeBounds, 9);
    sd->customDifficultyMenuItem
        = addSettingsOptionsItemToMenu(sd->menu, menuItemDifficulty, menuOptLabelsDifficulty, menuOptValsDifficulty,
                                       ARRAY_SIZE(menuOptValsDifficulty), &settingDifficultyBounds, 0);

    sd->menu = endSubMenu(sd->menu);

    sd->menu = startSubMenu(sd->menu, menuItemSettings);
    sd->menu = endSubMenu(sd->menu);
}

static void swadgedokuSetupNumberWheel(int base, uint16_t disableMask)
{
    //// Number Wheel
    if (sd->numberWheelRenderer != NULL)
    {
        deinitWheelMenu(sd->numberWheelRenderer);
        sd->numberWheelRenderer = NULL;
    }

    if (sd->numberWheel != NULL)
    {
        deinitMenu(sd->numberWheel);
        sd->numberWheel = NULL;
    }

    sd->wheelMenuTextBox.pos.x  = 24;
    sd->wheelMenuTextBox.pos.y  = TFT_HEIGHT - sd->uiFont.height - 3;
    sd->wheelMenuTextBox.width  = TFT_WIDTH - 48;
    sd->wheelMenuTextBox.height = sd->uiFont.height;

    sd->numberWheelRenderer = initWheelMenu(&sd->uiFont, 90, &sd->wheelMenuTextBox);
    sd->numberWheel         = initMenu(strSelectDigit, numberWheelCb);

    for (int i = 0; i < base && i < ARRAY_SIZE(digitLabels); i++)
    {
        uint8_t pos = (base - i) % base;
        addSingleItemToMenu(sd->numberWheel, digitLabels[i]);
        wheelMenuSetItemInfo(sd->numberWheelRenderer, digitLabels[i], NULL, pos, NO_SCROLL);
        wheelMenuSetItemTextIcon(sd->numberWheelRenderer, digitLabels[i], digitLabels[i]);

        if (disableMask & (1 << i))
        {
            wheelMenuSetItemColor(sd->numberWheelRenderer, digitLabels[i], c333, c444);
        }
    }
}

static void swadgedokuMainMenuCb(const char* label, bool selected, uint32_t value)
{
    if (selected)
    {
        if (menuItemContinue == label)
        {
            size_t progLength = 0;
            if (readNvsBlob(settingKeyProgress, NULL, &progLength))
            {
                uint8_t buffer[progLength];
                if (readNvsBlob(settingKeyProgress, buffer, &progLength))
                {
                    if (loadSudokuData(buffer, progLength, &sd->game))
                    {
                        sd->playTimer = 0;
                        setupSudokuPlayer(&sd->player, &sd->game);
                        sudokuGetNotes(sd->game.notes, &sd->game, 0);
                        swadgedokuSetupNumberWheel(sd->game.base, 0);
                        sd->screen = SWADGEDOKU_GAME;
                    }
                    else
                    {
                        ESP_LOGE("Swadgedoku", "Can't load saved game: bad data");
                    }
                }
            }
        }
        else if (menuItemLevelSelect == label)
        {
            /*if (!setupSudokuGame(&sd->game, SM_CLASSIC, 9, 9))
            {
                ESP_LOGE("Swadgedoku", "Couldn't setup game???");
                return;
            }*/
            cnfsFileIdx_t file = value + SUDOKU_PUZ_000_BSP;
            if (file > SUDOKU_PUZ_MAX)
            {
                ESP_LOGE("Swadgedoku", "Tried to load illegal game file ");
                return;
            }
            size_t fileLen;
            const uint8_t* sudokuData = cnfsGetFile(file, &fileLen);
            if (!loadSudokuData(sudokuData, fileLen, &sd->game))
            {
                ESP_LOGE("Swadgedoku", "Couldn't load game file");
            }

            sd->playTimer = 0;
            setupSudokuPlayer(&sd->player, &sd->game);
            // sd->game.grid[0] = 9;
            // sd->game.flags[0] = SF_LOCKED;

            sudokuGetNotes(sd->game.notes, &sd->game, 0);

            swadgedokuSetupNumberWheel(sd->game.base, 0);

            // sd->player.notes[1] = 127;
            // sd->game.flags[8] = SF_VOID;
            // sd->game.grid[2] = 8;
            // setDigit(&sd->game, 8, 2, 0);
            sd->screen = SWADGEDOKU_GAME;
        }
        else if (menuItemStartCustom == label)
        {
            if (!setupSudokuGame(&sd->game, sd->customModeMenuItem->currentSetting,
                                 sd->customSizeMenuItem->currentSetting, sd->customSizeMenuItem->currentSetting))
            {
                ESP_LOGE("Swadgedoku", "Couldn't setup custom game???");
                return;
            }

            sd->currentMode       = sd->customModeMenuItem->currentSetting;
            sd->currentDifficulty = sd->customDifficultyMenuItem->currentSetting;

            sd->playTimer = 0;
            setupSudokuPlayer(&sd->player, &sd->game);
            sudokuGetNotes(sd->game.notes, &sd->game, 0);
            swadgedokuSetupNumberWheel(sd->game.base, 0);
            sd->screen = SWADGEDOKU_GAME;
        }
        else if (menuItemPlayJigsaw == label)
        {
            if (!setupSudokuGame(&sd->game, SM_JIGSAW, 9, 9))
            {
                return;
            }

            sd->playTimer = 0;
            setupSudokuPlayer(&sd->player, &sd->game);
            sudokuGetNotes(sd->game.notes, &sd->game, 0);
            swadgedokuSetupNumberWheel(sd->game.base, 0);
            sd->screen = SWADGEDOKU_GAME;
        }
    }
    else
    {
        if (menuItemLevelSelect == label)
        {
            sd->lastLevel = value;
        }
        else if (menuItemCustomSize == label)
        {
            // If the selected size is non-square, automatically switch to jigsaw mode
            bool forceJigsaw = false;
            switch (value)
            {
                case 1:
                case 4:
                case 9:
                case 16:
                    break;

                default:
                    forceJigsaw = true;
                    break;
            }
            if (forceJigsaw && sd->customModeMenuItem->currentOpt != SM_JIGSAW)
            {
                sd->prevMode                           = sd->customModeMenuItem->currentOpt;
                sd->customModeMenuItem->currentOpt     = SM_JIGSAW;
                sd->customModeMenuItem->currentSetting = SM_JIGSAW;
                sd->jigsawForced                       = true;
            }
            else if (!forceJigsaw && sd->customModeMenuItem->currentOpt == SM_JIGSAW && sd->jigsawForced)
            {
                // And when they switch away from a jigsaw-only size, return to the previous mode
                sd->customModeMenuItem->currentSetting = sd->prevMode;
                sd->customModeMenuItem->currentOpt     = sd->prevMode;
                sd->jigsawForced                       = false;
            }
        }
        else if (menuItemCustomMode == label)
        {
            // If the selected mode is changed to classic, switch away from incompatible non-square sizes

            bool forceSquare = false;
            switch (sd->customSizeMenuItem->currentSetting)
            {
                case 1:
                case 4:
                case 9:
                case 16:
                    break;

                default:
                    forceSquare = true;
                    break;
            }
            if (forceSquare && value == SM_CLASSIC)
            {
                sd->prevSize                           = sd->customSizeMenuItem->currentSetting;
                sd->customSizeMenuItem->currentSetting = 9;
                sd->customSizeMenuItem->currentOpt     = 7;
                sd->squareForced                       = true;
            }
            else if (!forceSquare && value != SM_CLASSIC && sd->squareForced)
            {
                // And when they switch away from the incompatible size, restore the previous selection
                sd->customSizeMenuItem->currentSetting = sd->prevSize;
                sd->customSizeMenuItem->currentOpt     = sd->prevSize - 2;
                sd->squareForced                       = false;
            }
        }
    }
}

static void swadgedokuPauseMenuCb(const char* label, bool selected, uint32_t value)
{
    if (selected)
    {
        ESP_LOGE("Swadgedoku", "Pause menu %s selected", label);
        if (menuItemResume == label)
        {
            sd->screen = SWADGEDOKU_GAME;
        }
        else if (menuItemResetPuzzle == label)
        {
            for (int n = 0; n < sd->game.size * sd->game.size; n++)
            {
                // Just clear all non-player-set digits
                if (!(sd->game.flags[n] & (SF_LOCKED | SF_VOID)))
                {
                    sd->game.grid[n] = 0;
                }
            }
            sd->playTimer = 0;
            sudokuGetNotes(sd->game.notes, &sd->game, 0);
        }
        else if (menuItemAbandonPuzzle == label)
        {
            swadgedokuSetupMenu();
            sd->screen = SWADGEDOKU_MAIN_MENU;
        }
        else if (menuItemExitPuzzle == label)
        {
            size_t size = getSudokuSaveSize(&sd->game, NULL, NULL, NULL, NULL);
            ESP_LOGE("Swadgedoku", "Writing %d bytes of swadgedoku save to NVS", (int)size);
            uint8_t data[size];
            // Write the data to the buffer
            writeSudokuData(data, &sd->game);

            // Write the buffer to NVS
            if (writeNvsBlob(settingKeyProgress, data, size))
            {
                ESP_LOGI("Swadgedoku", "Saved progress to NVS!");
                swadgedokuSetupMenu();
                sd->screen = SWADGEDOKU_MAIN_MENU;
            }
            else
            {
                ESP_LOGE("Swadgedoku", "Could not save progress to NVS!!!");
            }
        }
    }
}

static void numberWheelCb(const char* label, bool selected, uint32_t value)
{
    ESP_LOGE("Swadgedoku", "Number '%s' %s", label, selected ? "Selected" : "Scrolled");

    if (selected)
    {
        for (int i = 0; i < ARRAY_SIZE(digitLabels); i++)
        {
            if (label == digitLabels[i])
            {
                sd->player.selectedDigit = i + 1;
            }
        }
    }
}

bool loadSudokuData(const uint8_t* data, size_t length, sudokuGrid_t* game)
{
    if (!length || !data)
    {
        return false;
    }

    // Clear the sudoku game
    deinitSudokuGame(game);

    const uint8_t* cur = data;

    // 8 bytes of header
    // + (boxFormat) ? ((size * size + 1) / 2) : 0
    // + (gridFormat) ? (size * size) : (1 + (numCount + 1) / 2 * 3)
    // + (noteFormat) ? (size * size * 2) : (1 + noteCount * 3)
    // + (flagFormat) ? (size * size) : (1 + flagCount * 2)
    // Version 0 only for now
    uint8_t version = *cur++;

    // Mode:
    // - 0: Regular
    // - 1: Jigsaw, 2: X?
    sudokuMode_t mode = (sudokuMode_t)*cur++;

    // Grid size: grid size in each dimension
    uint8_t size = *cur++;

    // Base: total number of numbers/rows/boxes
    uint8_t base = *cur++;

    // Box data format:
    // - 0: Predefined (square)
    // - 1: Individual (full grid)
    uint8_t boxFormat = *cur++;

    // Grid format:
    // 0: Mapped (smaller for sparse puzzles)
    // 1: Individual (full grid)
    uint8_t gridFormat = *cur++;

    // Notes format:
    // - 0: Mapped
    // - 1: Individual (full grid)
    uint8_t noteFormat = *cur++;

    // Flags format:
    // - 0: Mapped
    // - 1: Individual (full grid)
    uint8_t flagsFormat = *cur++;

    ESP_LOGD("Swadgedoku", "Loading sudoku puzzle");
    ESP_LOGD("Swadgedoku", "- Mode: %" PRIu8, mode);
    ESP_LOGD("Swadgedoku", "- Size: %" PRIu8 "x%" PRIu8, size, size);
    ESP_LOGD("Swadgedoku", "- Base: %" PRIu8, base);
    ESP_LOGD("Swadgedoku", "- Format (Box/Grid/Note/Flags): %" PRIu8 "/%" PRIu8 "/%" PRIu8 "/%" PRIu8, boxFormat,
             gridFormat, noteFormat, flagsFormat);

    if (version != 0)
    {
        ESP_LOGE("Swadgedoku", "Cannot load unsupported version %" PRIu8, version);
        return false;
    }

    if (!initSudokuGame(game, size, base, mode))
    {
        return false;
    }

    switch (boxFormat)
    {
        case 0:
        {
            // Standard
            // Boxes are boring squares
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

            if (baseRoot == 0)
            {
                ESP_LOGE("Swadgedoku", "Invalid 'standard' box configuration for non-square base %" PRIu8, base);
                deinitSudokuGame(game);
                return false;
            }

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

            break;
        }

        case 1:
        {
            // Individual
            // Boxes are defined in a <size>X<size> map (packed slightly)
            const uint8_t* boxMapStart = cur;
            // We can store two boxes per byte
            // Since boxes can never be larger than 16
            const uint8_t* boxMapEnd = cur + (size * size + 1) / 2;

            while (cur < boxMapEnd)
            {
                int n = (cur - boxMapStart) * 2;

                // First box in the pair is stored in the most-significant nibble
                game->boxMap[n] = (*cur >> 4) & 0x0F;

                // Make sure we don't have an empty LSB here
                if (n + 1 < size * size)
                {
                    // Second box in the pair is stored in the least-significant nibble
                    game->boxMap[n + 1] = *cur & 0x0F;
                }

                cur++;
            }
            break;
        }

        default:
        {
            ESP_LOGE("Swagedoku", "Invalid box format %" PRIu8, boxFormat);
            return false;
        }
    }

    switch (gridFormat)
    {
        case 0:
        {
            // Mapped
            // mapLength is the number of VALUES used by the map
            uint8_t mapLength = *cur++;
            // Convert mapLength to the total number of bytes
            const uint8_t* mapEnd = cur + (mapLength + 1) / 2 * 3;

            int n = 0;

            while (cur < mapEnd)
            {
                // Map format:
                // Byte 0 is location, with row in the most-significant nibble
                // and column in the least-significant nibble
                // Byte 1 is a second location
                // Byte 2 is two values. The most-significant nibble is the value at the location in byte 0
                // The least-significant nibble is the value at the location in byte 1
                uint8_t loc0 = *cur++;
                uint8_t loc1 = *cur++;
                uint8_t vals = *cur++;

                game->grid[((loc0 >> 4) & 0x0F) * game->size + (loc0 & 0x0F)] = ((vals >> 4) & 0x0F) + 1;
                n++;

                if (n < mapLength)
                {
                    game->grid[((loc1 >> 4) & 0x0F) * game->size + (loc1 & 0x0F)] = ((vals & 0x0F)) + 1;
                    n++;
                }
            }
            break;
        }

        case 1:
        {
            // Full grid
            // We'd like to pack the grid a bit more but it won't fit
            // We need to be able to store a 0 (no digit) as well as the digit
            // So, we need a full byte, with 0 and 1-16 -- technically 5 bytes
            // 3 wasted bits, unless we use it to store pen/not-pen status? no...
            const uint8_t* gridStart = cur;
            const uint8_t* gridEnd   = cur + game->size * game->size;

            while (cur < gridEnd)
            {
                game->grid[cur - gridStart] = *cur;
                cur++;
            }

            break;
        }
    }

    switch (noteFormat)
    {
        case 0:
        {
            // Mapped
            // It's 3 bytes per square here!!!
            // 1 byte for the location and 2 for the actual notes
            uint8_t mapLength       = *cur++;
            const uint8_t* notesEnd = cur + 3 * mapLength;

            while (cur < notesEnd)
            {
                uint8_t pos = *cur++;
                // MSB first
                uint16_t note = (*cur++) << 8;
                // LSB next
                note |= *cur++;
                game->notes[((pos >> 4) & 0x0F) * game->size + (pos & 0x0F)] = note;
            }
            break;
        }

        case 1:
        {
            const uint8_t* notesStart = cur;
            const uint8_t* notesEnd   = cur + 2 * game->size * game->size;

            while (cur < notesEnd)
            {
                uint16_t note = (*cur++) << 8;
                note |= *cur++;

                game->notes[(cur - notesStart - 2) / 2] = note;
            }
            break;
        }
    }

    switch (flagsFormat)
    {
        case 0:
        {
            // Mapped
            // It's 2 bytes per square so we can have up to 8 flags
            uint8_t mapLength     = *cur++;
            const uint8_t* mapEnd = cur + 2 * mapLength;

            while (cur < mapEnd)
            {
                uint8_t pos  = *cur++;
                uint8_t flag = *cur++;

                game->flags[((pos >> 4) & 0x0F) * game->size + (pos & 0x0F)] = (sudokuFlag_t)flag;
            }
            break;
        }
        case 1:
        {
            // Individual
            const uint8_t* flagsStart = cur;
            const uint8_t* flagsEnd   = cur + game->size * game->size;

            while (cur < flagsEnd)
            {
                game->flags[cur - flagsStart] = (sudokuFlag_t)*cur;
                cur++;
            }
            break;
        }
    }

    return true;
}

size_t writeSudokuData(uint8_t* data, const sudokuGrid_t* game)
{
    int boxFmt, gridFmt, noteFmt, flagFmt;
    getSudokuSaveSize(game, &boxFmt, &gridFmt, &noteFmt, &flagFmt);

    uint8_t* out = data;

    // write header
    // version
    *out++ = 0;
    // game mode
    *out++ = (uint8_t)game->mode;
    // grid size
    *out++ = game->size;
    // base
    *out++ = game->base;
    // box format
    *out++ = boxFmt;
    // grid format
    *out++ = gridFmt;
    // note format
    *out++ = noteFmt;
    // flags format
    *out++ = flagFmt;

    switch (boxFmt)
    {
        case 0:
            // Write no data
            break;

        case 1:
        {
            for (int n = 0; n < game->size * game->size; n += 2)
            {
                uint8_t boxPair = (game->boxMap[n] & 0x0F) << 4;

                if (n + 1 < game->size * game->size)
                {
                    boxPair |= (game->boxMap[n + 1] & 0x0F);
                }

                *out++ = boxPair;
            }
            break;
        }
    }

    switch (gridFmt)
    {
        case 0:
        {
            // Mapped
            int count = 0;
            for (int n = 0; n < game->size * game->size; n++)
            {
                if (game->grid[n] != 0)
                {
                    count++;
                }
            }

            *out++ = (count & 0xFF);

            // Holds the nibble for the cell value
            uint8_t buf  = 0;
            bool pending = false;

            for (int n = 0; n < game->size * game->size; n++)
            {
                if (game->grid[n] != 0)
                {
                    int r = n / game->size;
                    int c = n % game->size;

                    if (pending)
                    {
                        // Write second location of pair
                        *out++ = ((r & 0x0F) << 4) | (c & 0x0F);
                        buf |= (game->grid[n] - 1);
                        // Write accompanying numerical values
                        *out++  = buf;
                        pending = false;
                    }
                    else
                    {
                        // Write first location of pair
                        *out++  = ((r & 0x0F) << 4) | (c & 0x0F);
                        buf     = (game->grid[n] - 1) << 4;
                        pending = true;
                    }
                }
            }

            // Write any remaining value
            if (pending)
            {
                // Write an empty location
                *out++ = 0;

                // Write the pending value
                *out++ = buf;
            }
            break;
        }

        case 1:
        {
            // Individual
            for (int n = 0; n < game->size * game->size; n++)
            {
                *out++ = game->grid[n];
            }
            break;
        }
    }

    switch (noteFmt)
    {
        case 0:
        {
            // Mapped

            int count = 0;
            for (int n = 0; n < game->size * game->size; n++)
            {
                if (game->notes[n] != 0)
                {
                    count++;
                }
            }

            *out++ = (count & 0xFF);

            for (int n = 0; n < game->size * game->size; n++)
            {
                if (game->notes[n] != 0)
                {
                    int r  = n / game->size;
                    int c  = n % game->size;
                    *out++ = ((r & 0x0F) << 4) | (c & 0x0F);
                    *out++ = (game->notes[n] >> 8) & 0xFF;
                    *out++ = (game->notes[n]) & 0xFF;
                }
            }

            break;
        }

        case 1:
        {
            // Individual
            for (int n = 0; n < game->size * game->size; n++)
            {
                *out++ = (game->notes[n] >> 8) & 0xFF;
                *out++ = game->notes[n] & 0xFF;
            }
            break;
        }
    }

    switch (flagFmt)
    {
        case 0:
        {
            // Mapped
            int count = 0;
            for (int n = 0; n < game->size * game->size; n++)
            {
                if (game->flags[n] != SF_NONE)
                {
                    count++;
                }
            }

            *out++ = (count & 0xFF);
            for (int n = 0; n < game->size * game->size; n++)
            {
                if (game->flags[n] != SF_NONE)
                {
                    int r  = n / game->size;
                    int c  = n % game->size;
                    *out++ = ((r & 0x0F) << 4) | (c & 0x0F);
                    *out++ = (uint8_t)game->flags[n];
                }
            }
            break;
        }

        case 1:
        {
            // Individual
            for (int n = 0; n < game->size * game->size; n++)
            {
                *out++ = (uint8_t)game->flags[n];
            }
            break;
        }
    }

    return out - data;
}

size_t getSudokuSaveSize(const sudokuGrid_t* game, int* boxFmt, int* gridFmt, int* noteFmt, int* flagFmt)
{
    // 8 bytes of header
    // + (boxFormat) ? ((size * size + 1) / 2) : 0
    // + (gridFormat) ? (size * size) : (1 + (numCount + 1) / 2 * 3)
    // + (noteFormat) ? (size * size * 2) : (1 + noteCount * 3)
    // + (flagFormat) ? (size * size) : (1 + flagCount * 2)

    // Default to square boxes
    int boxFormat = 0;

    // Check for regular boxes
    int baseRoot = 0;
    switch (game->base)
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

    if (baseRoot != 0)
    {
        // Setup square boxes!
        for (int box = 0; box < game->base && !boxFormat; box++)
        {
            for (int n = 0; n < game->base; n++)
            {
                int x = (box % baseRoot) * baseRoot + (n % baseRoot);
                int y = (box / baseRoot) * baseRoot + (n / baseRoot);

                if (game->boxMap[y * game->size + x] != box)
                {
                    // We can't use square boxes, stop checking
                    boxFormat = 1;
                    break;
                }
            }
        }
    }
    else
    {
        boxFormat = 1;
    }

    int numCount  = 0;
    int noteCount = 0;
    int flagCount = 0;

    // Count number of elements we'd need to save
    for (int n = 0; n < game->size * game->size; n++)
    {
        if (game->grid[n] != 0)
        {
            numCount++;
        }

        if (game->notes[n] != 0)
        {
            noteCount++;
        }

        if (game->flags[n] != SF_NONE)
        {
            flagCount++;
        }
    }

    const int gridArea = game->size * game->size;

    // Decide which is smaller
    int gridLenF0 = (1 + (numCount + 1) / 2 * 3);
    int gridLenF1 = game->size * game->size;

    int noteLenF0 = (1 + noteCount * 3);
    int noteLenF1 = (game->size * game->size * 2);

    int flagLenF0 = (1 + flagCount * 2);
    int flagLenF1 = (game->size * game->size);

    int gridFormat = (gridLenF0 < gridLenF1) ? 0 : 1;
    int noteFormat = (noteLenF0 < noteLenF1) ? 0 : 1;
    int flagFormat = (flagLenF0 < flagLenF1) ? 0 : 1;

    int boxLen  = boxFormat ? ((gridArea + 1) / 2) : 0;
    int gridLen = gridFormat ? gridLenF1 : gridLenF0;
    int noteLen = noteFormat ? noteLenF1 : noteLenF0;
    int flagLen = flagFormat ? flagLenF1 : flagLenF0;

    // Now we know how big it will be!

    if (boxFmt)
    {
        *boxFmt = boxFormat;
    }

    if (gridFmt)
    {
        *gridFmt = gridFormat;
    }

    if (noteFmt)
    {
        *noteFmt = noteFormat;
    }

    if (flagFmt)
    {
        *flagFmt = flagFormat;
    }

    size_t totalLen = 8 + boxLen + gridLen + noteLen + flagLen;

    return totalLen;
}

bool initSudokuGame(sudokuGrid_t* game, int size, int base, sudokuMode_t mode)
{
    if (size < base)
    {
        ESP_LOGE("Swadgedoku", "%dx%d Grid not large enough for base %d", size, size, base);
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
        case SM_JIGSAW:
        {
            if (size < base)
            {
                ESP_LOGE("Swadgedoku", "%dx%d Grid not large enough for base %d", size, size, base);
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

    player->notes            = calloc(game->size * game->size, sizeof(uint16_t));
    player->overlay.gridOpts = calloc(game->size * game->size, sizeof(sudokuOverlayOpt_t));

    player->cursorShape = calloc(1, sizeof(sudokuOverlayShape_t));

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

            ESP_LOGI("Swadgedoku", "Box[r=%d][c=%d] == sourceBox (%" PRIu16 ")", r, c, sourceBox);
            notes[r * game->size + c] = (rowNotes[r] & colNotes[c] & sourceBoxNote);
            if (!game->grid[r * game->size + c] && !notes[r * game->size + c])
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
    sudokuOverlayShape_t* circle = malloc(sizeof(sudokuOverlayShape_t));
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
        sudokuOverlayShape_t* leftLine = malloc(sizeof(sudokuOverlayShape_t));

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
        sudokuOverlayShape_t* rightLine = malloc(sizeof(sudokuOverlayShape_t));

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
        sudokuOverlayShape_t* topLine = malloc(sizeof(sudokuOverlayShape_t));

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
        sudokuOverlayShape_t* bottomLine = malloc(sizeof(sudokuOverlayShape_t));

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
 */
void sudokuAnnotate(sudokuOverlay_t* overlay, const sudokuPlayer_t* player, const sudokuGrid_t* game)
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
            free(shape);
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

    const sudokuOverlayOpt_t keepOverlay = OVERLAY_ERROR;

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
                overlay->gridOpts[n] |= (game->notes[n] == cursorBits) ? OVERLAY_CHECK : OVERLAY_QUESTION;
            }
            else if (!digit && highlightSelectedDigitLocations && player->selectedDigit && (game->notes[n] & selBits))
            {
                // Use a ? if this is the only possibility
                overlay->gridOpts[n] |= (game->notes[n] == selBits) ? OVERLAY_CHECK : OVERLAY_QUESTION;
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
        }
    }
}

/**
 * @brief Checks whether the puzzle is valid, in error, or complete. Returns 0 if valid
 * but incomplete, 1 if complete, and -1 if incorrect
 *
 * @param game The game to check
 * @return int -1 for an invalid game, 1 for win, and 0 if valid but incomplete
 */
int swadgedokuCheckWin(const sudokuGrid_t* game)
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
                    return -1;
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
    return complete ? 1 : 0;
}

void swadgedokuCheckTrophyTriggers(void)
{
    for (const trophyData_t* trophy = swadgedokuTrophyData.list;
         trophy < swadgedokuTrophyData.list + swadgedokuTrophyData.length; trophy++)
    {
        if (NULL != trophy->identifier)
        {
            const swadgedokuTrophyTrigger_t* trigger = (const swadgedokuTrophyTrigger_t*)trophy->identifier;

            if (trigger->timeLimit != -1 && (sd->playTimer / ONE_SECOND_IN_US) > trigger->timeLimit)
            {
                // Over time limit for trigger, skip to next
                continue;
            }

            if (trigger->difficulty != -1 && sd->currentDifficulty < trigger->difficulty)
            {
                // Difficulty doesn't match trigger, skip to next
                continue;
            }

            if (trigger->mode != -1 && sd->currentMode != trigger->mode)
            {
                // Mode doesn't match trigger, skip to next
                continue;
            }

            // All the trigger conditions must have passed, trigger the trophy!
            trophyUpdate(*trophy, 1, true);
        }
    }
}

void swadgedokuGameButton(buttonEvt_t evt)
{
    if (evt.down)
    {
        bool moved      = false;
        bool reAnnotate = false;
        switch (evt.button)
        {
            case PB_A:
            case PB_B:
            {
                // B unsets, A sets
                int digit = (evt.button == PB_A) ? sd->player.selectedDigit : 0;

                if (!((SF_LOCKED | SF_VOID) & sd->game.flags[sd->player.curY * sd->game.size + sd->player.curX]))
                {
                    // Not locked or void, proceed setting the digit
                    if (!digit || digit == sd->game.grid[sd->player.curY * sd->game.size + sd->player.curX])
                    {
                        // Unset number
                        setDigit(&sd->game, 0, sd->player.curX, sd->player.curY);
                    }
                    else
                    {
                        // Set number
                        if (setDigit(&sd->game, digit, sd->player.curX, sd->player.curY))
                        {
                            switch (swadgedokuCheckWin(&sd->game))
                            {
                                case -1:
                                    ESP_LOGD("Swadgedoku", "Invalid sudoku board!");
                                    break;

                                case 0:
                                    ESP_LOGD("Swadgedoku", "Sudoku OK but incomplete");
                                    break;

                                case 1:
                                    swadgedokuCheckTrophyTriggers();
                                    sd->screen = SWADGEDOKU_WIN;
                                    break;
                            }
                        }
                    }

                    reAnnotate = true;
                }
                break;
            }

            case PB_SELECT:
            {
                break;
            }

            case PB_START:
            {
                sd->screen = SWADGEDOKU_PAUSE;
                break;
            }

            case PB_UP:
            {
                moved = true;
                do
                {
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
                moved = true;
                do
                {
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
                moved = true;
                do
                {
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
                moved = true;
                do
                {
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

        if (moved && sd->player.cursorShape)
        {
            switch (sd->player.cursorShape->type)
            {
                case OVERLAY_RECT:
                {
                    getOverlayPos(&sd->player.cursorShape->rectangle.pos.x, &sd->player.cursorShape->rectangle.pos.y,
                                  sd->player.curY, sd->player.curX, SUBPOS_NW);
                    break;
                }

                case OVERLAY_CIRCLE:
                {
                    getOverlayPos(&sd->player.cursorShape->circle.pos.x, &sd->player.cursorShape->circle.pos.y,
                                  sd->player.curY, sd->player.curX, SUBPOS_CENTER);
                    break;
                }

                default:
                {
                    break;
                }
            }
        }

        if (moved || reAnnotate)
        {
            sudokuAnnotate(&sd->player.overlay, &sd->player, &sd->game);
        }
    }
}

void swadgedokuDrawGame(const sudokuGrid_t* game, const uint16_t* notes, const sudokuOverlay_t* overlay,
                        const sudokuTheme_t* theme)
{
    // Total space around the grid
    int gridMargin = 1;

    // Max size of individual square
    int maxSquareSize = (TFT_HEIGHT - gridMargin) / game->size;
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
                        // int charW = textWidth(&sd->noteFont, buf);

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
                                    + (miniSquareSize - textWidth(&sd->noteFont, buf)) / 2 + 1;
                        int noteY = y + (n / baseRoot) * maxSquareSize / baseRoot
                                    + (miniSquareSize - sd->noteFont.height) / 2 + 2;

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
                if (opts & OVERLAY_ERROR)
                {
                    // Really this should just make the number red?
                    overlayText = "!";
                }

                if (overlayText)
                {
                    int textX = x + (maxSquareSize - textWidth(&sd->gridFont, overlayText)) / 2;
                    int textY = y + (maxSquareSize - sd->gridFont.height) / 2;
                    drawText(&sd->gridFont, overlayCol, overlayText, textX, textY);
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
                    int w = textWidth(&sd->uiFont, shape->text.val);

                    if (shape->text.center)
                    {
                        x -= w / 2;
                    }

                    drawText(&sd->uiFont, shape->color, shape->text.val, x, y);
                    break;
                }
            }
        }
    }
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

bool squaresTouch(uint8_t ax, uint8_t ay, uint8_t bx, uint8_t by)
{
    return ((ay == by) && ((ax + 1 == bx) || (bx + 1 == ax))) || ((ax == bx) && ((ay + 1 == by) || (by + 1 == ay)));
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
