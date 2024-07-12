/**
 * @file mode_2048.c
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief A game of 2048 for 2024-2025 Swadge hardware
 * @version 1.1.3
 * @date 2024-06-28
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "mode_2048.h"

//==============================================================================
// Defines
//==============================================================================

// Swadge
#define T48_US_PER_FRAME 16667

// Game
#define T48_GRID_SIZE  4
#define T48_BOARD_SIZE 16

// Graphics
#define T48_CELL_SIZE   50
#define T48_LINE_WEIGHT 4
#define T48_SIDE_MARGIN 30
#define T48_TOP_MARGIN  20
#define T48_TILE_COUNT  16
#define T48_MAX_MERGES  8
#define T48_MAX_SEQ     16

// High score
#define T48_HS_COUNT  5
#define T48_HS_KEYLEN 14

// LEDs
#define T48_LED_TIMER 32

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    GAME,      // Game running
    WIN,       // Display a win
    GAMEOVER,  // Display final score and prompt a restart
    GAMESTART, // Splash screen after load
    CONFIRM,   // Confirm player wants top abandon game
    WRITE,     // Allows player to write their initials for high score
} t48DisplayState_t;

typedef enum
{
    UP,
    DOWN,
    LEFT,
    RIGHT,
    ALL
} t48Direction_t;

typedef enum
{
    STATIC,
    MOVED,
    MERGED,
    NEW
} t48CellStateEnum_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    wsg_t image;
    int16_t pos[2];
    uint8_t spd;
    int8_t dir;
} t48FallingBlock_t;

typedef struct
{
    int8_t x;
    int8_t y;
} t48CellCoors_t;

typedef struct
{
    wsg_t image;
    t48CellCoors_t grid;
    int8_t gridEnd;
    int8_t speed;
    uint8_t sequence;
    bool horizontal;
    int32_t value;
} t48SlidingTile_t;

typedef struct
{
    t48CellCoors_t incoming;
    t48CellCoors_t end;
    t48CellStateEnum_t state;
    int32_t value;
} t48CellState_t;

typedef struct
{
    // Assets
    font_t font;
    font_t titleFont;
    font_t titleFontOutline;
    wsg_t tiles[T48_TILE_COUNT];
    midiFile_t bgm;
    midiFile_t click;

    // Game state
    int32_t boardArr[T48_GRID_SIZE][T48_GRID_SIZE]; // Row, Col
    int32_t score;
    int32_t highScore[T48_HS_COUNT];
    bool newHS;

    // Display
    char scoreStr[16];
    bool alreadyWon;
    char playerInitials[4];
    char hsInitials[T48_HS_COUNT][4];
    bool textEntryDone;
    t48DisplayState_t ds;
    uint8_t hue;
    t48SlidingTile_t slidingTiles[8]; // Max amount of sliding tiles
    t48CellState_t cellState[T48_BOARD_SIZE];
    int8_t globalAnim;

    // LEDs
    led_t leds[CONFIG_NUM_LEDS];

    // Audio
    bool bgmIsPlaying;

    // Start screen
    t48FallingBlock_t fb[T48_TILE_COUNT];
    bool startScrInitialized;
    int16_t timer;
} t48_t;

//==============================================================================
// Function Prototypes
//==============================================================================

// Swadge functions

/**
 * @brief Mode setup
 *
 */
static void t48EnterMode(void);

/**
 * @brief Mode teardown
 *
 */
static void t48ExitMode(void);

/**
 * @brief Main loop of the code
 *
 * @param elapsedUs
 */
static void t48MainLoop(int64_t elapsedUs);

// Game functions

/**
 * @brief Sets an empty cell to 2 or 4 on 50/50 basis
 *
 * @return int8_t Cell used
 */
static int8_t t48SetRandCell(void);

/**
 * @brief Updates the LEDs based on a direction and adds a new tile if valid
 *
 * @param wasUpdated If the board has changed to a new config
 * @param dir Direction used to update board
 */
static void t48BoardUpdate(bool wasUpdated, t48Direction_t dir);

/**
 * @brief Merge a single row or column. Cells merged from CELL_SIZE-1 -> 0.
 *
 * @param slice Slice to merge
 * @param updated If board was already updated
 * @return true If a merge occurred
 * @return false if no merge occurred
 */
static bool t48MergeSlice(uint32_t* slice, bool updated);

/**
 * @brief Slide blocks down if possible
 *
 */
static void t48SlideDown(void);

/**
 * @brief Slide blocks up if possible
 *
 */
static void t48SlideUp(void);

/**
 * @brief Slide blocks right if possible
 *
 */
static void t48SlideRight(void);

/**
 * @brief Slide blocks left if possible
 *
 */
static void t48SlideLeft(void);

// Game state

/**
 * @brief Initializes a game
 *
 */
static void t48StartGame(void);

/**
 * @brief Checks if the player has reached 2048 for the first time
 *
 * @return true     If this is the first time 2048 has been hit
 * @return false    Otherwise
 */
static bool t48CheckWin(void);

/**
 * @brief Checks if the game can no longer be played
 *
 * @return true     Display final score and reset
 * @return false    Continue playing
 */
static bool t48CheckOver(void);

/**
 * @brief Sorts the scores and swaps them into the correct place
 *
 */
static void t48SortHighScores(void);

// Visuals

/**
 * @brief Get the Color for a specific value
 *
 * @param val Value that requires a color for its square
 * @return uint8_t Color of the value's square
 */
static uint8_t getTileSprIndex(uint32_t val);

/**
 * @brief Draws the grid, score, and tiles
 *
 */
static void t48Draw(void);

static void t48DrawTileOnGrid(wsg_t* img, int8_t row, int8_t col, int16_t xOff, int16_t yOff, int32_t val);

static void t48ResetCellState(void);

static void t48SetCellState(int8_t idx, t48CellStateEnum_t st, int8_t startX, int8_t startY, int8_t endX, int8_t endY,
                            int32_t value);

static void t48ConvertCellState(void);

static void t48DrawCellState(void);

static void t48ResetSlideAnim(void);

static void t48SetSlidingTile(int8_t idx, t48CellCoors_t start, t48CellCoors_t end, int32_t value);

static void t48DrawSlidingTiles(void);

/**
 * @brief Shows the title upon booting into the mode
 *
 * @param color Color of the title text
 */
static void t48StartScreen(paletteColor_t color);

/**
 * @brief Draws the final score and prompts the player to play again
 *
 * @param score The final value of the game
 */
static void t48DrawGameOverScreen(int64_t score);

/**
 * @brief Draw the win screen. It doesn't do anything else.
 *
 */
static void t48DrawWinScreen(void);

/**
 * @brief Draw the screen asking to confirm they want to quit
 *
 */
static void t48DrawConfirm(void);

// LEDs

/**
 * @brief Automatically dims LEDs every frame
 *
 */
static void t48DimLEDs(void);

/**
 * @brief Sets an LED to a color
 *
 * @param idx   Index of the LED
 * @param color Color to set LED
 */
static void t48SetRGB(uint8_t idx, led_t color);

/**
 * @brief Illuminate appropriate LEDs based on an indicated direction
 *
 * @param dir   Direction to illuminate LEDs
 * @param color Color to set the LEDs to
 */
static void t48LightLEDs(t48Direction_t dir, led_t color);

/**
 * @brief Based on the highest block value, set the LED color
 *
 * @return led_t Color object send to the LEDs
 */
static led_t t48GetLEDColors(void);

/**
 * @brief Get a random bright color for the LEDs
 *
 * @return led_t Color to send to LEDs
 */
static led_t t48RandColor(void);

/**
 * @brief Run an random LED program
 *
 */
static void t48RandLEDs(void);

/**
 * @brief Returns values in hue sequence
 *
 * @return paletteColor_t Color to use
 */
static paletteColor_t t48Rainbow(void);

// Audio
/**
 * @brief Restart audio when song ends
 *
 */
static void t48BgmCb(void);

//==============================================================================
// Variables
//==============================================================================

const char modeName[]   = "2048";
const char pressKey[]   = "Press any key to play";
const char pressAB[]    = "Press A or B to reset the game";
const char youWin[]     = "You got 2048!";
const char continueAB[] = "Press A or B to continue";
const char highScore[]  = "You got a high score!";
const char paused[]     = "Paused!";
const char pausedA[]    = "Press A to continue playing";
const char pausedB[]    = "Press B to abandon game";

const char highScoreKey[T48_HS_COUNT][T48_HS_KEYLEN]
    = {"t48HighScore0", "t48HighScore1", "t48HighScore2", "t48HighScore3", "t48HighScore4"};
const char highScoreInitialsKey[T48_HS_COUNT][T48_HS_KEYLEN] = {
    "t48HSInitial0", "t48HSInitial1", "t48HSInitial2", "t48HSInitial3", "t48HSInitial4",
};

swadgeMode_t t48Mode = {
    .modeName                 = modeName,
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = false,
    .usesAccelerometer        = false,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = t48EnterMode,
    .fnExitMode               = t48ExitMode,
    .fnMainLoop               = t48MainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = NULL,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL,
};

t48_t* t48;

//==============================================================================
// Functions
//==============================================================================

// Swadge functions

static void t48EnterMode(void)
{
    // Init Mode & resources
    setFrameRateUs(T48_US_PER_FRAME);
    t48 = calloc(sizeof(t48_t), 1);

    // Load fonts
    loadFont("ibm_vga8.font", &t48->font, false);
    loadFont("sonic.font", &t48->titleFont, false);
    makeOutlineFont(&t48->titleFont, &t48->titleFontOutline, false);

    // Load images
    loadWsg("Tile-Blue-Diamond.wsg", &t48->tiles[0], true);
    loadWsg("Tile-Blue-Square.wsg", &t48->tiles[1], true);
    loadWsg("Tile-Cyan-Legs.wsg", &t48->tiles[2], true);
    loadWsg("Tile-Green-Diamond.wsg", &t48->tiles[3], true);
    loadWsg("Tile-Green-Octo.wsg", &t48->tiles[4], true);
    loadWsg("Tile-Green-Square.wsg", &t48->tiles[5], true);
    loadWsg("Tile-Mauve-Legs.wsg", &t48->tiles[6], true);
    loadWsg("Tile-Orange-Legs.wsg", &t48->tiles[7], true);
    loadWsg("Tile-Pink-Diamond.wsg", &t48->tiles[8], true);
    loadWsg("Tile-Pink-Octo.wsg", &t48->tiles[9], true);
    loadWsg("Tile-Pink-Square.wsg", &t48->tiles[10], true);
    loadWsg("Tile-Purple-Legs.wsg", &t48->tiles[11], true);
    loadWsg("Tile-Red-Octo.wsg", &t48->tiles[12], true);
    loadWsg("Tile-Red-Square.wsg", &t48->tiles[13], true);
    loadWsg("Tile-Yellow-Diamond.wsg", &t48->tiles[14], true);
    loadWsg("Tile-Yellow-Octo.wsg", &t48->tiles[15], true);
    // TODO: Add new sprites

    // Load sounds
    loadMidiFile("Follinesque.mid", &t48->bgm, true);
    loadMidiFile("sndBounce.mid", &t48->click, true);

    // Init Text Entry
    textEntryInit(&t48->font, 4, t48->playerInitials);
    textEntrySetBGColor(c001);
    textEntrySetEmphasisColor(c500);
    textEntrySetNewCapsStyle(true);
    textEntrySetNewEnterStyle(true);

    // Init Game
    for (int8_t i = 0; i < T48_HS_COUNT; i++)
    {
        if (!readNvs32(highScoreKey[i], &t48->highScore[i]))
        {
            switch (i)
            {
                case 0:
                    t48->highScore[i] = 100000;
                    break;
                case 1:
                    t48->highScore[i] = 50000;
                    break;
                case 2:
                    t48->highScore[i] = 25000;
                    break;
                case 3:
                    t48->highScore[i] = 10000;
                    break;
                case 4:
                    t48->highScore[i] = 5000;
                    break;
            }
            writeNvs32(highScoreKey[i], t48->highScore[i]);
        }
        size_t len = 4;
        if (!readNvsBlob(highScoreInitialsKey[i], &t48->hsInitials[i], &len))
        {
            static char buff[5];
            switch (i)
            {
                case 0:
                    strcpy(buff, "JW");
                    break;
                case 1:
                    strcpy(buff, "Pan");
                    break;
                case 2:
                    strcpy(buff, "Pix");
                    break;
                case 3:
                    strcpy(buff, "Poe");
                    break;
                case 4:
                    strcpy(buff, "DRG");
                    break;
            }
            strcpy(t48->hsInitials[i], buff);
            writeNvsBlob(highScoreInitialsKey[i], &t48->hsInitials[i], len);
        }
    }
    t48->ds = GAMESTART;
}

static void t48ExitMode(void)
{
    freeFont(&t48->titleFontOutline);
    freeFont(&t48->titleFont);
    freeFont(&t48->font);

    // TODO: Free new sprites
    for (uint8_t i = 0; i < T48_TILE_COUNT; i++)
    {
        freeWsg(&t48->tiles[i]);
    }

    soundStop(true);
    unloadMidiFile(&t48->bgm);
    unloadMidiFile(&t48->click);

    free(t48);
}

static void t48MainLoop(int64_t elapsedUs)
{
    t48DimLEDs();
    if (!t48->bgmIsPlaying)
    {
        soundPlayBgmCb(&t48->bgm, MIDI_BGM, t48BgmCb);
        t48->bgmIsPlaying = true;
    }
    buttonEvt_t evt;
    switch (t48->ds)
    {
        case GAMESTART:
            // Check any button is pressed
            while (checkButtonQueueWrapper(&evt))
            {
                if (evt.down)
                {
                    soundPlaySfx(&t48->click, MIDI_SFX);
                    t48StartGame();
                    t48->ds = GAME;
                    for (int i = 0; i < T48_BOARD_SIZE; i++)
                    {
                        t48->boardArr[i / T48_GRID_SIZE][i % T48_GRID_SIZE] = 2;
                    }
                }
            }
            // Draw
            t48StartScreen(t48Rainbow());
            break;
        case GAME:
            // Input
            while (checkButtonQueueWrapper(&evt))
            {
                // Move blocks down, up, right or left
                if (evt.down && evt.button & PB_DOWN)
                {
                    soundPlaySfx(&t48->click, MIDI_SFX);
                    t48SlideDown();
                }
                else if (evt.down && evt.button & PB_UP)
                {
                    soundPlaySfx(&t48->click, MIDI_SFX);
                    t48SlideUp();
                }
                else if (evt.down && evt.button & PB_LEFT)
                {
                    soundPlaySfx(&t48->click, MIDI_SFX);
                    t48SlideLeft();
                }
                else if (evt.down && evt.button & PB_RIGHT)
                {
                    soundPlaySfx(&t48->click, MIDI_SFX);
                    t48SlideRight();
                }
                // Restart game if you hit start
                else if (evt.down && evt.button & PB_START)
                {
                    soundPlaySfx(&t48->click, MIDI_SFX);
                    t48->ds = CONFIRM;
                }
            }
            // Check game is done or "done"
            if (t48CheckWin())
            {
                t48->ds = WIN;
            }
            if (t48CheckOver())
            {
                for (int8_t i = 0; i < T48_HS_COUNT; i++)
                {
                    if (t48->highScore[i] <= t48->score)
                    {
                        t48->newHS = true;
                    }
                }
                if (t48->newHS)
                {
                    t48->ds = WRITE;
                }
                else
                {
                    t48->ds = GAMEOVER;
                }
            }
            // Draw
            t48Draw();
            break;
        case GAMEOVER:
            // Check any button is pressed
            while (checkButtonQueueWrapper(&evt))
            {
                if (evt.down && (evt.button & PB_A || evt.button & PB_B))
                {
                    soundPlaySfx(&t48->click, MIDI_SFX);
                    t48->ds = GAMESTART;
                }
            }
            // Draw
            t48DrawGameOverScreen(t48->score);
            break;
        case WIN:
            // Check any button is pressed
            while (checkButtonQueueWrapper(&evt))
            {
                if (evt.down && (evt.button & PB_A || evt.button & PB_B))
                {
                    soundPlaySfx(&t48->click, MIDI_SFX);
                    t48->ds = GAME;
                }
            }
            // Draw
            t48DrawWinScreen();
            break;
        case CONFIRM:
            // Input
            while (checkButtonQueueWrapper(&evt))
            {
                if (evt.down && evt.button & PB_B)
                {
                    t48->ds = GAMESTART;
                }
                else if (evt.down && evt.button & PB_A)
                {
                    t48->ds = GAME;
                }
            }
            t48DrawConfirm();
            break;
        case WRITE:
            while (checkButtonQueueWrapper(&evt))
            {
                t48->textEntryDone = !textEntryInput(evt.down, evt.button);
            }
            if (t48->textEntryDone)
            {
                t48SortHighScores();
                textEntrySoftReset();
                t48->ds = GAMEOVER;
            }
            textEntryDraw(elapsedUs);
            break;
        default:
            break;
    }
}

// Game functions

static int8_t t48SetRandCell()
{
    int8_t cell;
    do
    {
        cell = esp_random() % T48_BOARD_SIZE;
    } while (t48->boardArr[cell / T48_GRID_SIZE][cell % T48_GRID_SIZE] != 0);
    int8_t rand = esp_random() % 10;
    if (rand == 0)
    {
        t48->boardArr[cell / T48_GRID_SIZE][cell % T48_GRID_SIZE] = 4; // 10%
    }
    else
    {
        t48->boardArr[cell / T48_GRID_SIZE][cell % T48_GRID_SIZE] = 2; // 90%
    }
    t48->cellState[cell].state = STATIC;
    return cell;
}

static void t48BoardUpdate(bool wasUpdated, t48Direction_t dir)
{
    if (wasUpdated)
    {
        t48SetRandCell();
        led_t col = t48GetLEDColors();
        t48LightLEDs(dir, col);
        t48ConvertCellState();
    }
    else
    {
        led_t col = {.r = 200, .g = 200, .b = 200};
        t48LightLEDs(ALL, col);
    }
}

// TODO: Change merged cells to Affect animations
static bool t48MergeSlice(uint32_t slice[], bool updated)
{
    for (uint8_t i = 0; i < T48_GRID_SIZE - 1; i++)
    {
        // Merge
        if (slice[i] == slice[i + 1])
        {
            if (slice[i] == 0)
            {
                continue;
            }
            updated = true;
            slice[i] *= 2;
            t48->score += slice[i];
            // Move if merged
            for (uint8_t j = i + 1; j < T48_GRID_SIZE - 1; j++)
            {
                slice[j] = slice[j + 1];
            }
            // Add a 0 to end if merged
            slice[T48_GRID_SIZE - 1] = 0;
        }
    }
    return updated;
}

// NOTE: All these seem to be backwards.
// All other systems use row, column, but these seem to require column, row which is mirrors the movements along the
// top left to bottom right diagonal.
// I have left the odd behavior here as it's simple to remap keys to different functions rather than compensate in the
// visual code.

static void t48SlideDown()
{
    t48ResetCellState();
    bool updated = false;
    int8_t idx   = 0;
    for (uint8_t row = 0; row < T48_GRID_SIZE; row++)
    {
        uint32_t slice[T48_GRID_SIZE] = {0};
        for (int8_t col = T48_GRID_SIZE - 1, i = 0; col >= 0; col--)
        {
            if (t48->boardArr[row][col] != 0)
            {
                t48SetCellState(idx++, MOVED, row, col, row, T48_GRID_SIZE - 1 - i, t48->boardArr[row][col]);
                slice[i++] = t48->boardArr[row][col];
                if (col != (T48_GRID_SIZE - i))
                {
                    updated = true;
                }
            }
        }
        updated = t48MergeSlice(slice, updated);
        for (int8_t col = T48_GRID_SIZE - 1, i = 0; col >= 0; col--)
        {
            t48->boardArr[row][col] = slice[i++];
        }
    }
    t48BoardUpdate(updated, DOWN);
}

static void t48SlideUp()
{
    t48ResetCellState();
    bool updated = false;
    int8_t idx   = 0;
    for (uint8_t row = 0; row < T48_GRID_SIZE; row++)
    {
        uint32_t slice[T48_GRID_SIZE] = {0};
        for (int8_t col = 0, i = 0; col <= T48_GRID_SIZE - 1; col++)
        {
            if (t48->boardArr[row][col] != 0)
            {
                if (col != i)
                {
                    updated = true;
                }
                t48SetCellState(idx++, MOVED, row, col, row, i, t48->boardArr[row][col]);
                slice[i++] = t48->boardArr[row][col];
            }
        }
        updated = t48MergeSlice(slice, updated);
        for (int8_t col = 0, i = 0; col <= T48_GRID_SIZE - 1; col++)
        {
            t48->boardArr[row][col] = slice[i++];
        }
    }
    t48BoardUpdate(updated, UP);
}

static void t48SlideRight()
{
    t48ResetCellState();
    bool updated = false;
    int8_t idx   = 0;
    for (uint8_t col = 0; col < T48_GRID_SIZE; col++)
    {
        uint32_t slice[T48_GRID_SIZE] = {0};
        for (int8_t row = T48_GRID_SIZE - 1, i = 0; row >= 0; row--)
        {
            if (t48->boardArr[row][col] != 0)
            {
                t48SetCellState(idx++, MOVED, row, col, T48_GRID_SIZE - 1 - i, col, t48->boardArr[row][col]);
                slice[i++] = t48->boardArr[row][col];
                if (row != (T48_GRID_SIZE - i))
                {
                    updated = true;
                }
            }
        }
        updated = t48MergeSlice(slice, updated);
        for (int8_t row = T48_GRID_SIZE - 1, i = 0; row >= 0; row--)
        {
            t48->boardArr[row][col] = slice[i++];
        }
    }
    t48BoardUpdate(updated, RIGHT);
}

static void t48SlideLeft()
{
    t48ResetCellState();
    bool updated = false;
    int8_t idx   = 0;
    for (uint8_t col = 0; col < T48_GRID_SIZE; col++)
    {
        uint32_t slice[T48_GRID_SIZE] = {0};
        for (int8_t row = 0, i = 0; row <= T48_GRID_SIZE - 1; row++)
        {
            if (t48->boardArr[row][col] != 0)
            {
                if (col != i)
                {
                    updated = true;
                }
                t48SetCellState(idx++, MOVED, row, col, i, col, t48->boardArr[row][col]);
                slice[i++] = t48->boardArr[row][col];
            }
        }
        updated = t48MergeSlice(slice, updated);
        for (int8_t row = 0, i = 0; row <= T48_GRID_SIZE - 1; row++)
        {
            t48->boardArr[row][col] = slice[i++];
        }
    }
    t48BoardUpdate(updated, LEFT);
}

// Game state

static void t48StartGame()
{
    // clear the board
    for (uint8_t i = 0; i < T48_BOARD_SIZE; i++)
    {
        t48->boardArr[i / T48_GRID_SIZE][i % T48_GRID_SIZE] = 0;
    }
    t48ResetCellState();
    t48->alreadyWon = false;
    t48->score      = 0;
    // Get random places to start
    t48SetRandCell();
    t48SetRandCell();
}

static bool t48CheckWin()
{
    if (t48->alreadyWon)
    {
        return false;
    }
    for (uint8_t i = 0; i < T48_BOARD_SIZE; i++)
    {
        if (t48->boardArr[i / T48_GRID_SIZE][i % T48_GRID_SIZE] == 2048)
        {
            t48->alreadyWon = true;
            return true;
        }
    }
    return false;
}

static bool t48CheckOver()
{
    // Check if any cells are open
    for (uint8_t i = 0; i < T48_BOARD_SIZE; i++)
    {
        if (t48->boardArr[i / T48_GRID_SIZE][i % T48_GRID_SIZE] == 0)
        {
            return false;
        }
    }
    // Check if any two consecutive block match vertically
    for (uint8_t row = 0; row < T48_GRID_SIZE; row++)
    {
        for (uint8_t col = 0; col < T48_GRID_SIZE - 1; col++)
        { // -1 to account for comparison
            if (t48->boardArr[row][col] == t48->boardArr[row + 1][col])
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
            if (t48->boardArr[row][col] == t48->boardArr[row][col + 1])
            {
                return false;
            }
        }
    }
    // Game is over
    return true;
}

static void t48SortHighScores()
{
    // 5th place needs to compare to the score
    if (t48->highScore[T48_HS_COUNT - 1] < t48->score)
    {
        t48->highScore[T48_HS_COUNT - 1] = t48->score;
        strcpy(t48->hsInitials[T48_HS_COUNT - 1], t48->playerInitials);
    }
    else
    {
        // Scores *should* be sorted already. Save cycles.
        return;
    }
    for (int8_t i = T48_HS_COUNT - 2; i >= 0; i--)
    {
        if (t48->highScore[i] < t48->highScore[i + 1])
        {
            // Swap
            int32_t swap          = t48->highScore[i];
            t48->highScore[i]     = t48->highScore[i + 1];
            t48->highScore[i + 1] = swap;
            char swapI[4];
            strcpy(swapI, t48->hsInitials[i]);
            strcpy(t48->hsInitials[i], t48->hsInitials[i + 1]);
            strcpy(t48->hsInitials[i + 1], swapI);
        }
    }
    // Save out the new scores
    for (int8_t i = 0; i < T48_HS_COUNT; i++)
    {
        writeNvs32(highScoreKey[i], t48->highScore[i]);
        writeNvsBlob(highScoreInitialsKey[i], &t48->hsInitials[i], 4);
    }
}

// Visuals

static uint8_t getTileSprIndex(uint32_t val)
{
    switch (val)
    {
        case 2:
            return 5;
        case 4:
            return 8;
        case 8:
            return 2;
        case 16:
            return 12;
        case 32:
            return 0;
        case 64:
            return 15;
        case 128:
            return 1;
        case 256:
            return 7;
        case 512:
            return 10;
        case 1024:
            return 9;
        case 2048:
            return 14;
        case 4096:
            return 11;
        case 8192:
            return 6;
        case 16384:
            return 13;
        case 32768:
            return 4;
        case 65535:
            return 3;
        case 131072:
            return 0; // Probably never seen
        default:
            return 0;
    }
}

static void t48Draw()
{
    // Blank
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);

    // Draw grid lines
    for (uint8_t i = 0; i < T48_GRID_SIZE + 1; i++)
    {
        int16_t left = i * (T48_CELL_SIZE + T48_LINE_WEIGHT);
        fillDisplayArea(T48_SIDE_MARGIN + left, T48_TOP_MARGIN, T48_SIDE_MARGIN + left + T48_LINE_WEIGHT, TFT_HEIGHT,
                        c111);
        int16_t top = i * (T48_CELL_SIZE + T48_LINE_WEIGHT);
        fillDisplayArea(T48_SIDE_MARGIN, top + T48_TOP_MARGIN, TFT_WIDTH - T48_SIDE_MARGIN,
                        top + T48_TOP_MARGIN + T48_LINE_WEIGHT, c111);
    }

    // Score
    static char textBuffer[32];
    snprintf(textBuffer, sizeof(textBuffer) - 1, "Score: %" PRIu32, t48->score);
    strcpy(t48->scoreStr, textBuffer);
    drawText(&t48->font, c555, t48->scoreStr, T48_SIDE_MARGIN, 4);

    // Draw Tiles
    t48DrawCellState();
}

static void t48DrawTileOnGrid(wsg_t* img, int8_t row, int8_t col, int16_t xOff, int16_t yOff, int32_t val)
{
    // Bail if 0
    if (val == 0)
        return;
    // Sprite
    uint16_t x_cell_offset = row * (T48_CELL_SIZE + T48_LINE_WEIGHT) + T48_SIDE_MARGIN + T48_LINE_WEIGHT + xOff;
    uint16_t y_cell_offset = col * (T48_CELL_SIZE + T48_LINE_WEIGHT) + T48_TOP_MARGIN + T48_LINE_WEIGHT + yOff;
    drawWsgSimple(img, x_cell_offset, y_cell_offset);
    // Text
    static char buffer[16];
    snprintf(buffer, sizeof(buffer) - 1, "%" PRIu32, val);
    uint16_t text_center = (textWidth(&t48->font, buffer)) / 2;
    drawText(&t48->font, c555, buffer, x_cell_offset - text_center + T48_CELL_SIZE / 2,
             y_cell_offset - 4 + T48_CELL_SIZE / 2);
}

static void t48ResetCellState()
{
    for (int8_t i = 0; i < T48_BOARD_SIZE; i++)
    {
        t48->cellState[i].incoming.x = 0;
        t48->cellState[i].incoming.y = 0;
        t48->cellState[i].end.x      = 0;
        t48->cellState[i].end.y      = 0;
        t48->cellState[i].state      = STATIC;
        t48->cellState[i].value      = 0;
    }
    t48ResetSlideAnim();
}

static void t48SetCellState(int8_t idx, t48CellStateEnum_t st, int8_t startX, int8_t startY, int8_t endX, int8_t endY,
                            int32_t value)
{
    t48->cellState[idx].incoming.x = startX;
    t48->cellState[idx].incoming.y = startY;
    t48->cellState[idx].end.x      = endX;
    t48->cellState[idx].end.y      = endY;
    t48->cellState[idx].state      = st;
    t48->cellState[idx].value      = value;
}

static void t48ConvertCellState()
{
    int8_t idx      = 0;
    t48->globalAnim = 0;
    for (int8_t i = 0; i < T48_BOARD_SIZE; i++)
    {
        switch (t48->cellState[i].state)
        {
            case MOVED:
                t48SetSlidingTile(idx++, t48->cellState[i].incoming, t48->cellState[i].end, t48->cellState[i].value);
                break;
            case MERGED:
                break;
            case NEW:
                break;
            default:
                break;
        }
    }
}

static void t48DrawCellState()
{
    if (t48->globalAnim <= T48_MAX_SEQ)
    {
        t48->globalAnim++;
    }
    else
    {
        t48ResetCellState();
    }
    for (int8_t i = 0; i < T48_BOARD_SIZE; i++)
    {
        switch (t48->cellState[i].state)
        {
            case STATIC:
                if (t48->globalAnim > T48_MAX_SEQ)
                {
                    int8_t row = i / T48_GRID_SIZE;
                    int8_t col = i % T48_GRID_SIZE;
                    t48DrawTileOnGrid(&t48->tiles[getTileSprIndex(t48->boardArr[row][col])], row, col, 0, 0,
                                      t48->boardArr[row][col]); // FIXME: Convert to CellState vars
                }
                break;
            case MERGED:
                break;
            case NEW:
                break;
            default:
                break;
        }
    }
    t48DrawSlidingTiles();
}

static void t48ResetSlideAnim()
{
    for (int8_t i = 0; i < T48_MAX_MERGES; i++)
    {
        t48->slidingTiles[i].speed    = 0;
        t48->slidingTiles[i].value    = 0;
        t48->slidingTiles[i].sequence = 0;
    }
}

static void t48SetSlidingTile(int8_t idx, t48CellCoors_t start, t48CellCoors_t end, int32_t value)
{
    t48->slidingTiles[idx].value  = value;
    t48->slidingTiles[idx].grid.x = start.x;
    t48->slidingTiles[idx].grid.y = start.y;
    t48->slidingTiles[idx].speed
        = (end.x - start.x + end.y - start.y) * (T48_CELL_SIZE + T48_LINE_WEIGHT) / T48_MAX_SEQ;
    t48->slidingTiles[idx].horizontal = (end.x != start.x);
}

static void t48DrawSlidingTiles()
{
    for (int8_t idx = 0; idx < T48_MAX_MERGES; idx++)
    {
        if (t48->slidingTiles[idx].value != 0)
        {
            int16_t xVal = 0;
            int16_t yVal = 0;
            if (t48->slidingTiles[idx].horizontal)
            {
                xVal = t48->globalAnim * t48->slidingTiles[idx].speed;
            }
            else
            {
                yVal = t48->globalAnim * t48->slidingTiles[idx].speed;
            }
            t48DrawTileOnGrid(&t48->tiles[getTileSprIndex(t48->slidingTiles[idx].value)], t48->slidingTiles[idx].grid.x,
                              t48->slidingTiles[idx].grid.y, xVal, yVal, t48->slidingTiles[idx].value);
        }
    }
}

static void t48StartScreen(paletteColor_t color)
{
    // Blank
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);
    // Draw random blocks
    if (!t48->startScrInitialized)
    {
        // Set random x and y coordinates for all blocks
        for (uint8_t i = 0; i < T48_TILE_COUNT; i++)
        {
            t48->fb[i].image  = t48->tiles[i];
            t48->fb[i].pos[0] = (esp_random() % (TFT_WIDTH + (2 * T48_CELL_SIZE))) - T48_CELL_SIZE;
            t48->fb[i].pos[1] = (esp_random() % (TFT_HEIGHT + (2 * T48_CELL_SIZE))) - T48_CELL_SIZE;
            t48->fb[i].spd    = (esp_random() % 2) + 1;
            t48->fb[i].dir    = (esp_random() % 3) - 1; // -1 for left, 0 for down, 1 for right
        }
        t48->startScrInitialized = true;
    }
    for (uint8_t i = 0; i < T48_TILE_COUNT; i++)
    {
        // Move block
        t48->fb[i].pos[1] += t48->fb[i].spd;
        t48->fb[i].pos[0] += t48->fb[i].spd * t48->fb[i].dir;
        // Wrap block if outside visual area
        if (t48->fb[i].pos[0] >= TFT_WIDTH + T48_CELL_SIZE)
        { // Wraps from right to left
            t48->fb[i].pos[0] = -T48_CELL_SIZE;
        }
        if (t48->fb[i].pos[0] <= -T48_CELL_SIZE)
        { // Wraps from left to right
            t48->fb[i].pos[0] = TFT_WIDTH + T48_CELL_SIZE;
        }
        if (t48->fb[i].pos[1] >= TFT_HEIGHT + T48_CELL_SIZE)
        { // Wraps from bottom ot top
            t48->fb[i].pos[1] = -T48_CELL_SIZE;
            t48->fb[i].pos[0] = (esp_random() % (TFT_WIDTH - T48_CELL_SIZE));
        }
        // Draw block
        drawWsgSimple(&t48->fb[i].image, t48->fb[i].pos[0], t48->fb[i].pos[1]);
    }
    // Do LEDs
    t48RandLEDs();
    // Title
    drawText(&t48->titleFont, color, modeName, (TFT_WIDTH - textWidth(&t48->titleFont, modeName)) / 2,
             TFT_HEIGHT / 2 - 12);
    drawText(&t48->titleFontOutline, c555, modeName, (TFT_WIDTH - textWidth(&t48->titleFont, modeName)) / 2,
             TFT_HEIGHT / 2 - 12);
    // Draw current High Score
    static char textBuffer[20];
    snprintf(textBuffer, sizeof(textBuffer) - 1, "High score: %" PRIu32, t48->highScore[0]);
    drawText(&t48->font, c444, textBuffer, (TFT_WIDTH - textWidth(&t48->font, textBuffer)) / 2, TFT_HEIGHT - 32);
    // Press any key...
    drawText(&t48->font, c555, pressKey, (TFT_WIDTH - textWidth(&t48->font, pressKey)) / 2, TFT_HEIGHT - 64);
}

static void t48DrawGameOverScreen(int64_t score)
{
    // Blank
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);

    // Display final score
    static char textBuffer[32];
    snprintf(textBuffer, sizeof(textBuffer) - 1, "Final score: %" PRIu64, score);
    drawText(&t48->titleFont, c550, textBuffer, 16, 48);
    for (int8_t i = 0; i < T48_HS_COUNT; i++)
    {
        static char initBuff[20];
        static paletteColor_t color;
        if (score == t48->highScore[i])
        {
            int16_t x = 16;
            int16_t y = 80;
            drawTextWordWrap(&t48->titleFont, t48Rainbow(), highScore, &x, &y, TFT_WIDTH - 16, y + 120);
            color = c500;
        }
        else
        {
            color = c444;
        }
        snprintf(initBuff, sizeof(initBuff) - 1, "%d: %d - ", i + 1, (int)t48->highScore[i]);
        strcat(initBuff, t48->hsInitials[i]);
        drawText(&t48->font, color, initBuff, 16, TFT_HEIGHT - (98 - (16 * i)));
    }
    // Display AB text
    drawText(&t48->font, c444, pressAB, 18, TFT_HEIGHT - 16);
    // LEDs!
    t48RandLEDs();
}

static void t48DrawWinScreen(void)
{
    // Blank
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);
    // Title
    drawText(&t48->titleFont, c055, youWin, (TFT_WIDTH - textWidth(&t48->titleFont, youWin)) / 2, 48);
    // Press any key...
    drawText(&t48->font, c555, continueAB, (TFT_WIDTH - textWidth(&t48->font, continueAB)) / 2, TFT_HEIGHT - 64);
    // LEDs!
    t48RandLEDs();
}

static void t48DrawConfirm()
{
    fillDisplayArea(64, 75, TFT_WIDTH - 64, 100, c100);
    drawText(&t48->titleFont, c555, paused, (TFT_WIDTH - textWidth(&t48->titleFont, paused)) / 2, 80);
    fillDisplayArea(32, 110, TFT_WIDTH - 32, 130, c100);
    drawText(&t48->font, c555, pausedA, (TFT_WIDTH - textWidth(&t48->font, pausedA)) / 2, 115);
    fillDisplayArea(32, 135, TFT_WIDTH - 32, 155, c100);
    drawText(&t48->font, c555, pausedB, (TFT_WIDTH - textWidth(&t48->font, pausedB)) / 2, 140);
}

// LEDs

static void t48DimLEDs()
{
    for (uint8_t i = 0; i < CONFIG_NUM_LEDS; i++)
    {
        // Red
        if (t48->leds[i].r < 6)
        {
            t48->leds[i].r = 0;
        }
        else if (t48->leds[i].r == 0)
        {
            // Do nothing
        }
        else
        {
            t48->leds[i].r -= 6;
        }
        // Green
        if (t48->leds[i].g < 6)
        {
            t48->leds[i].g = 0;
        }
        else if (t48->leds[i].g == 0)
        {
            // Do nothing
        }
        else
        {
            t48->leds[i].g -= 6;
        }
        // Blue
        if (t48->leds[i].b < 6)
        {
            t48->leds[i].b = 0;
        }
        else if (t48->leds[i].b == 0)
        {
            // Do nothing
        }
        else
        {
            t48->leds[i].b -= 6;
        }
    }
    setLeds(t48->leds, CONFIG_NUM_LEDS);
}

static void t48SetRGB(uint8_t idx, led_t color)
{
    // FIXME: See if I can make this one line
    t48->leds[idx].r = color.r;
    t48->leds[idx].g = color.g;
    t48->leds[idx].b = color.b;
}

static void t48LightLEDs(t48Direction_t dir, led_t color)
{
    switch (dir)
    {
        case DOWN:
            t48SetRGB(0, color);
            t48SetRGB(1, color);
            t48SetRGB(6, color);
            t48SetRGB(7, color);
            break;
        case UP:
            t48SetRGB(2, color);
            t48SetRGB(3, color);
            t48SetRGB(4, color);
            t48SetRGB(5, color);
            break;
        case LEFT:
            t48SetRGB(0, color);
            t48SetRGB(1, color);
            t48SetRGB(2, color);
            t48SetRGB(3, color);
            break;
        case RIGHT:
            t48SetRGB(4, color);
            t48SetRGB(5, color);
            t48SetRGB(6, color);
            t48SetRGB(7, color);
            break;
        case ALL:
            t48SetRGB(0, color);
            t48SetRGB(1, color);
            t48SetRGB(2, color);
            t48SetRGB(3, color);
            t48SetRGB(4, color);
            t48SetRGB(5, color);
            t48SetRGB(6, color);
            t48SetRGB(7, color);
            break;
    }
}

static led_t t48GetLEDColors()
{
    uint32_t maxVal = 0;
    for (uint8_t i = 0; i < T48_BOARD_SIZE; i++)
    {
        if (maxVal < t48->boardArr[i / T48_GRID_SIZE][i % T48_GRID_SIZE])
        {
            maxVal = t48->boardArr[i / T48_GRID_SIZE][i % T48_GRID_SIZE];
        }
    }
    led_t col = {0};
    switch (maxVal)
    {
        case 2:
            // Green
            col.g = 128;
            return col;
        case 4:
            // Pink
            col.r = 200;
            col.g = 150;
            col.b = 150;
            return col;
        case 8:
            // Cyan
            col.g = 255;
            col.b = 255;
            return col;
        case 16:
            // Red
            col.r = 255;
            return col;
        case 32:
            // Blue
            col.b = 255;
            return col;
        case 64:
            // Yellow
            col.r = 128;
            col.g = 128;
            return col;
        case 128:
            // Blue
            col.b = 255;
            return col;
        case 256:
            // Orange
            col.r = 255;
            col.g = 165;
            return col;
        case 512:
            // Dark Pink
            col.r = 255;
            col.g = 64;
            col.b = 64;
            return col;
        case 1024:
            // Pink
            col.r = 255;
            col.g = 128;
            col.b = 128;
            return col;
        case 2048:
            // Yellow
            col.r = 255;
            col.g = 255;
            return col;
        case 4096:
            // Purple
            col.r = 200;
            col.b = 200;
            return col;
        case 8192:
            // Mauve
            col.r = 255;
            col.b = 64;
            return col;
        case 16384:
            // Red
            col.r = 255;
            return col;
        case 32768:
            // Green
            col.g = 255;
            return col;
        case 65535:
            // Dark Blue
            col.b = 255;
            return col;
        default:
            col.r = 255;
            col.g = 128;
            col.b = 128;
            return col;
    }
}

static led_t t48RandColor()
{
    led_t col = {0};
    col.r     = 128 + (esp_random() % 127);
    col.g     = 128 + (esp_random() % 127);
    col.b     = 128 + (esp_random() % 127);
    return col;
}

static void t48RandLEDs()
{
    t48->timer -= 1;
    if (t48->timer <= 0)
    {
        t48->timer = T48_LED_TIMER;
        t48LightLEDs(esp_random() % 5, t48RandColor());
    }
}

static paletteColor_t t48Rainbow()
{
    uint8_t hue = t48->hue++;
    uint8_t sat = 255;
    uint8_t val = 255;
    return paletteHsvToHex(hue, sat, val);
}

// Audio

static void t48BgmCb()
{
    t48->bgmIsPlaying = false;
}