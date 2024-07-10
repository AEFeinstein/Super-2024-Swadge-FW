/**
 * @file mode_2048.c
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief A game of 2048 for 2024-2025 Swadge hardware
 * @version 1.1.2
 * @date 2024-06-28
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "mode_2048.h"

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
 * @return int Cell used
 */
static int t48SetRandCell(void);

static void t48BoardUpdate(bool wasUpdated, Direction_t dir);

/**
 * @brief Merge a single row or column. Cells merged from CELL_SIZE-1 -> 0.
 *
 * @param slice Slice to merge
 * @param updated If board was already updated
 * @return true If a merge occured
 * @return false if no merge occured
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
 * @brief Checks if the player has reached 2048 for tyhee first time
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
 * @param val       Value that requires a color for its square
 * @return uint8_t  Color of the value's square
 */
static uint8_t getColor(uint32_t val);

/**
 * @brief Draws the grid, score, and tiles
 *
 */
static void t48Draw(void);

/**
 * @brief Shows the title upon booting into the mode
 *
 * @param color Color of the title text
 */
static void t48StartScreen(uint8_t color);

/**
 * @brief Draws the final score and prompts the player to play again
 *
 * @param score The final value of the game
 */
static void t48DrawGameOverScreen(int64_t score);

/**
 * @brief Draw the win screen. It doesn't do anythiung else.
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
static void t48SetRGB(uint8_t idx, Color_t color);

/**
 * @brief Illuminate appropriate LEDs based on an indicated direction
 *
 * @param dir   Direction to illuminate LEDs
 * @param color Color to set the LEDs to
 */
static void t48LightLEDs(Direction_t dir, Color_t color);

/**
 * @brief Based on the highest block value, set the LED color
 *
 * @return Color_t Color object send to the LEDs
 */
static Color_t t48GetLEDColors(void);

/**
 * @brief Get a random bright color for the LEDs
 *
 * @return Color_t Color to send to LEDs
 */
static Color_t t48RandColor(void);

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

const char highScoreKey[HS_COUNT][HS_KEYLEN]
    = {"t48HighScore0", "t48HighScore1", "t48HighScore2", "t48HighScore3", "t48HighScore4"};
const char highScoreInitialsKey[HS_COUNT][HS_KEYLEN] = {
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
static led_t leds[CONFIG_NUM_LEDS] = {0};

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
    for (int8_t i = 0; i < HS_COUNT; i++)
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
    t48StartGame(); // First run only adds one block... so just run it twice!
}

static void t48ExitMode(void)
{
    freeFont(&t48->titleFontOutline);
    freeFont(&t48->titleFont);
    freeFont(&t48->font);

    for (uint8_t i = 0; i < TILE_COUNT; i++)
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
                for (int8_t i = 0; i < HS_COUNT; i++)
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

static int t48SetRandCell()
{
    int8_t cell = -1;
    while (t48->boardArr[cell / GRID_SIZE][cell % GRID_SIZE] != 0)
    {
        cell = esp_random() % BOARD_SIZE;
    }
    int8_t rand = esp_random() % 10;
    if (rand == 0)
    {
        t48->boardArr[cell / GRID_SIZE][cell % GRID_SIZE] = 4; // 10%
    }
    else
    {
        t48->boardArr[cell / GRID_SIZE][cell % GRID_SIZE] = 2; // 90%
    }

    return cell;
}

static void t48BoardUpdate(bool wasUpdated, Direction_t dir)
{
    if (wasUpdated)
    {
        t48SetRandCell();
        Color_t col = t48GetLEDColors();
        t48LightLEDs(dir, col);
    }
    else
    {
        Color_t col = {.r = 200, .g = 200, .b = 200};
        t48LightLEDs(ALL, col);
    }
}

static bool t48MergeSlice(uint32_t slice[], bool updated)
{
    for (uint8_t i = 0; i < GRID_SIZE - 1; i++)
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
            for (uint8_t j = i + 1; j < GRID_SIZE; j++)
            {
                slice[j] = slice[j + 1];
            }
            // Add a 0 to end if merged
            slice[GRID_SIZE - 1] = 0;
        }
    }
    return updated;
}

static void t48SlideDown()
{
    bool updated = false;
    for (uint8_t col = 0; col < GRID_SIZE; col++)
    {
        // Create a slice to merge
        uint32_t slice[GRID_SIZE] = {0};
        // Load only cells with value into slice in the order:
        // Bottom -> Top
        for (int8_t row = GRID_SIZE - 1, i = 0; row >= 0; row--)
        {
            // Only copy over values > 0, automatically moving all non-zeroes
            if (t48->boardArr[row][col] != 0)
            {
                slice[i++] = t48->boardArr[row][col];
                if (row != (GRID_SIZE - i))
                {
                    // If these go out of sync, board has updated
                    updated = true;
                }
            }
        }
        // Merge. If merge happens, update
        updated = t48MergeSlice(slice, updated);
        // Copy modified slice back into board array
        for (int8_t row = GRID_SIZE - 1, i = 0; row >= 0; row--)
        {
            t48->boardArr[row][col] = slice[i++];
        }
    }
    // If a board updated, add a new cell
    t48BoardUpdate(updated, DOWN);
}

static void t48SlideUp()
{
    bool updated = false;
    for (uint8_t col = 0; col < GRID_SIZE; col++)
    {
        // Create a slice to merge
        uint32_t slice[GRID_SIZE] = {0};
        // Load only cells with value into slice in the order:
        // Top -> Bottom
        for (int8_t row = 0, i = 0; row <= GRID_SIZE - 1; row++)
        {
            // Only copy over values > 0, automatically moving all non-zeroes
            if (t48->boardArr[row][col] != 0)
            {
                if (row != i)
                {
                    // If these go out of sync, board has updated
                    updated = true;
                }
                slice[i++] = t48->boardArr[row][col];
            }
        }
        // Merge. If merge happens, update
        updated = t48MergeSlice(slice, updated);
        // Copy modified slice back into board array
        for (int8_t row = 0, i = 0; row <= GRID_SIZE - 1; row++)
        {
            t48->boardArr[row][col] = slice[i++];
        }
    }
    // If a board updated, add a new cell
    t48BoardUpdate(updated, UP);
}

static void t48SlideRight()
{
    bool updated = false;
    for (uint8_t row = 0; row < GRID_SIZE; row++)
    {
        // Create a slice to merge
        uint32_t slice[GRID_SIZE] = {0};
        // Load only cells with value into slice in the order:
        // Right -> Left
        for (int8_t col = GRID_SIZE - 1, i = 0; col >= 0; col--)
        {
            // Only copy over values > 0, automatically moving all non-zeroes
            if (t48->boardArr[row][col] != 0)
            {
                slice[i++] = t48->boardArr[row][col];
                if (col != (GRID_SIZE - i))
                {
                    // If these go out of sync, board has updated
                    updated = true;
                }
            }
        }
        // Merge. If merge happens, update
        updated = t48MergeSlice(slice, updated);
        // Copy modified slice back into board array
        for (int8_t col = GRID_SIZE - 1, i = 0; col >= 0; col--)
        {
            t48->boardArr[row][col] = slice[i++];
        }
    }
    // If a board updated, add a new cell
    t48BoardUpdate(updated, RIGHT);
}

static void t48SlideLeft()
{
    bool updated = false;
    for (uint8_t row = 0; row < GRID_SIZE; row++)
    {
        // Create a slice to merge
        uint32_t slice[GRID_SIZE] = {0};
        // Load only cells with value into slice in the order:
        // Left -> Right
        for (int8_t col = 0, i = 0; col <= GRID_SIZE - 1; col++)
        {
            // Only copy over values > 0, automatically moving all non-zeroes
            if (t48->boardArr[row][col] != 0)
            {
                if (col != i)
                {
                    // If these go out of sync, board has updated
                    updated = true;
                }
                slice[i++] = t48->boardArr[row][col];
            }
        }
        // Merge. If merge happens, update
        updated = t48MergeSlice(slice, updated);
        // Copy modified slice back into board array
        for (int8_t col = 0, i = 0; col <= GRID_SIZE - 1; col++)
        {
            t48->boardArr[row][col] = slice[i++];
        }
    }
    // If a board updated, add a new cell
    t48BoardUpdate(updated, LEFT);
}

// Game state

static void t48StartGame()
{
    // clear the board
    for (uint8_t i = 0; i < BOARD_SIZE; i++)
    {
        t48->boardArr[i / GRID_SIZE][i % GRID_SIZE] = 0;
    }
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
    for (uint8_t i = 0; i < BOARD_SIZE; i++)
    {
        if (t48->boardArr[i / GRID_SIZE][i % GRID_SIZE] == 2048)
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
    for (uint8_t i = 0; i < BOARD_SIZE; i++)
    {
        if (t48->boardArr[i / GRID_SIZE][i % GRID_SIZE] == 0)
        {
            return false;
        }
    }
    // Check if any two consecutive block match vertically
    for (uint8_t row = 0; row < GRID_SIZE; row++)
    {
        for (uint8_t col = 0; col < GRID_SIZE - 1; col++)
        { // -1 to account for comparison
            if (t48->boardArr[col][row] == t48->boardArr[col + 1][row])
            {
                return false;
            }
        }
    }
    // Check if any two consecutive block match horizontally
    for (uint8_t row = 0; row < GRID_SIZE - 1; row++)
    { // -1 to account for comparison
        for (uint8_t col = 0; col < GRID_SIZE - 1; col++)
        {
            if (t48->boardArr[col][row] == t48->boardArr[col][row + 1])
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
    if (t48->highScore[HS_COUNT - 1] < t48->score)
    {
        t48->highScore[HS_COUNT - 1] = t48->score;
        strcpy(t48->hsInitials[HS_COUNT - 1], t48->playerInitials);
    }
    else
    {
        // Scores *should* be sorted already. save cycles.
        return;
    }
    for (int8_t i = HS_COUNT - 2; i >= 0; i--)
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
    for (int8_t i = 0; i < HS_COUNT; i++)
    {
        writeNvs32(highScoreKey[i], t48->highScore[i]);
        writeNvsBlob(highScoreInitialsKey[i], &t48->hsInitials[i], 4);
    }
}

// Visuals

static uint8_t getColor(uint32_t val)
{
    switch (val)
    {
        case 2:
            return 5;
            break;
        case 4:
            return 8;
            break;
        case 8:
            return 2;
            break;
        case 16:
            return 12;
            break;
        case 32:
            return 0;
            break;
        case 64:
            return 15;
            break;
        case 128:
            return 1;
            break;
        case 256:
            return 7;
            break;
        case 512:
            return 10;
            break;
        case 1024:
            return 9;
            break;
        case 2048:
            return 14;
            break;
        case 4096:
            return 11;
            break;
        case 8192:
            return 6;
            break;
        case 16384:
            return 13;
            break;
        case 32768:
            return 4;
            break;
        case 65535:
            return 3;
            break;
        case 131072:
            return 0; // Probably never seen
            break;
        default:
            return 0;
            break;
    }
}

static void t48Draw()
{
    // Blank
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);

    // Draw vertrical grid line
    for (uint8_t i = 0; i < 5; i++)
    {
        int16_t left = i * (T48_CELL_SIZE + T48_LINE_WEIGHT);
        fillDisplayArea(SIDE_MARGIN + left, TOP_MARGIN, SIDE_MARGIN + left + T48_LINE_WEIGHT, TFT_HEIGHT, c111);
    }

    // Draw horizontal grid lines
    for (uint8_t i = 0; i < 5; i++)
    {
        int16_t top = i * (T48_CELL_SIZE + T48_LINE_WEIGHT);
        fillDisplayArea(SIDE_MARGIN, top + TOP_MARGIN, TFT_WIDTH - SIDE_MARGIN, top + TOP_MARGIN + T48_LINE_WEIGHT,
                        c111);
    }

    // Score
    static char textBuffer[32];
    snprintf(textBuffer, sizeof(textBuffer) - 1, "Score: %" PRIu32, t48->score);
    strcpy(t48->scoreStr, textBuffer);
    drawText(&t48->font, c555, t48->scoreStr, SIDE_MARGIN, 4);

    // Cells
    int16_t side_offset = SIDE_MARGIN + T48_LINE_WEIGHT;
    int16_t top_offset  = TOP_MARGIN + T48_LINE_WEIGHT;
    static char buffer[16];
    for (uint8_t col = 0; col < GRID_SIZE; col++)
    {
        for (uint8_t row = 0; row < GRID_SIZE; row++)
        {
            // Grab the current value of the cell
            uint32_t val = t48->boardArr[col][row];
            // Bail if 0
            if (val == 0)
            {
                continue;
            }
            // Grab the offest based on cell
            uint16_t y_cell_offset = col * (T48_CELL_SIZE + T48_LINE_WEIGHT);
            uint16_t x_cell_offset = row * (T48_CELL_SIZE + T48_LINE_WEIGHT);
            // Convert int to char
            snprintf(buffer, sizeof(buffer) - 1, "%" PRIu32, val);
            // Get the center of the text
            uint16_t text_center = (textWidth(&t48->font, buffer)) / 2;
            // Get associated color
            drawWsgSimple(&t48->tiles[getColor(val)], side_offset + x_cell_offset, top_offset + y_cell_offset);
            // Draw the text
            drawText(&t48->font, c555, buffer, side_offset + x_cell_offset - text_center + T48_CELL_SIZE / 2,
                     top_offset + y_cell_offset - 4 + T48_CELL_SIZE / 2);
        }
    }
}

static void t48StartScreen(uint8_t color)
{
    // Blank
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);
    // Draw random blocks
    if (!t48->startScrInitialized)
    {
        // Set random x and y coordinates for all blocks
        for (uint8_t i = 0; i < TILE_COUNT; i++)
        {
            t48->fb[i].image  = t48->tiles[i];
            t48->fb[i].pos[0] = (esp_random() % (TFT_WIDTH + (2 * T48_CELL_SIZE))) - T48_CELL_SIZE;
            t48->fb[i].pos[1] = (esp_random() % (TFT_HEIGHT + (2 * T48_CELL_SIZE))) - T48_CELL_SIZE;
            t48->fb[i].spd    = (esp_random() % 2) + 1;
            t48->fb[i].dir    = (esp_random() % 3) - 1; // -1 for left, 0 for down, 1 for right
        }
        t48->startScrInitialized = true;
    }
    for (uint8_t i = 0; i < TILE_COUNT; i++)
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
    for (int8_t i = 0; i < HS_COUNT; i++)
    {
        static char initBuff[20];
        static uint8_t color;
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
        if (leds[i].r < 6)
        {
            leds[i].r = 0;
        }
        else if (leds[i].r == 0)
        {
            // Do nothing
        }
        else
        {
            leds[i].r -= 6;
        }
        // Green
        if (leds[i].g < 6)
        {
            leds[i].g = 0;
        }
        else if (leds[i].g == 0)
        {
            // Do nothing
        }
        else
        {
            leds[i].g -= 6;
        }
        // Blue
        if (leds[i].b < 6)
        {
            leds[i].b = 0;
        }
        else if (leds[i].b == 0)
        {
            // Do nothing
        }
        else
        {
            leds[i].b -= 6;
        }
    }
    setLeds(leds, CONFIG_NUM_LEDS);
}

static void t48SetRGB(uint8_t idx, Color_t color)
{
    leds[idx].r = color.r;
    leds[idx].g = color.g;
    leds[idx].b = color.b;
}

static void t48LightLEDs(Direction_t dir, Color_t color)
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

static Color_t t48GetLEDColors()
{
    uint32_t maxval = 0;
    for (uint8_t i = 0; i < BOARD_SIZE; i++)
    {
        if (maxval < t48->boardArr[i / GRID_SIZE][i % GRID_SIZE])
        {
            maxval = t48->boardArr[i / GRID_SIZE][i % GRID_SIZE];
        }
    }
    Color_t col = {0};
    switch (maxval)
    {
        case 2:
            // Green
            col.g = 128;
            return col;
            break;
        case 4:
            // Pink
            col.r = 200;
            col.g = 100;
            col.b = 100;
            return col;
            break;
        case 8:
            // Cyan
            col.g = 255;
            col.b = 255;
            return col;
            break;
        case 16:
            // Red
            col.r = 255;
            return col;
            break;
        case 32:
            // Blue
            col.b = 255;
            return col;
            break;
        case 64:
            // Yellow
            col.r = 128;
            col.g = 128;
            return col;
            break;
        case 128:
            // Blue
            col.b = 255;
            return col;
            break;
        case 256:
            // Orange
            col.r = 255;
            col.g = 165;
            return col;
            break;
        case 512:
            // Dark Pink
            col.r = 255;
            col.g = 64;
            col.b = 64;
            return col;
            break;
        case 1024:
            // Pink
            col.r = 255;
            col.g = 128;
            col.b = 128;
            return col;
            break;
        case 2048:
            // Yellow
            col.r = 255;
            col.g = 255;
            return col;
            break;
        case 4096:
            // Purple
            col.r = 200;
            col.b = 200;
            return col;
            break;
        case 8192:
            // Mauve
            col.r = 255;
            col.b = 64;
            return col;
            break;
        case 16384:
            // Red
            col.r = 255;
            return col;
            break;
        case 32768:
            // Green
            col.g = 255;
            return col;
            break;
        case 65535:
            // Dark Blue
            col.b = 255;
            return col;
            break;
        default:
            col.r = 255;
            col.g = 128;
            col.b = 128;
            return col;
    }
}

static Color_t t48RandColor()
{
    Color_t col = {0};
    col.r       = 128 + (esp_random() % 127);
    col.g       = 128 + (esp_random() % 127);
    col.b       = 128 + (esp_random() % 127);
    return col;
}

static void t48RandLEDs()
{
    t48->timer -= 1;
    if (t48->timer <= 0)
    {
        t48->timer = 32;
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