/**
 * @file mode_2048.c
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief A game of 2048 for 2024-2025 Swadge hardware
 * @version 1.0
 * @date 2024-06-28
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include "mode_2048.h"

//==============================================================================
// Variables
//==============================================================================

const char modeName[]   = "2048";
const char pressKey[]   = "Press any key to play";
const char pressAB[]    = "Press A or B to reset the game";
const char youWin[]     = "You got 2048!";
const char continueAB[] = "Press A or B to continue";

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
    loadWsg("Orange-Splat.wsg", &t48->tiles[16], true);

    // Init Game
    t48->ds = GAMESTART;
    t48StartGame();  // First run only adds one block... so just run it twice!
}

static void t48ExitMode(void)
{
    freeFont(&t48->titleFont);
    freeFont(&t48->font);

    for (int i = 0; i < 17; i ++){
        freeWsg(&t48->tiles[i]);
    }

    free(t48);
}

static void t48MainLoop(int64_t elapsedUs)
{
    buttonEvt_t evt;
    switch(t48->ds){
        case GAMESTART:
            // Check any button is pressed
            while (checkButtonQueueWrapper(&evt))
            {
                if (evt.down){
                    t48StartGame();
                    t48->ds = GAME;
                    for (int i = 0; i < 15; i++){
                        t48->boardArr[i / GRID_SIZE][i % GRID_SIZE] = 2 << i;
                    }
                }
            }
            // Draw
            t48StartScreen(c550);   // TODO: Make a rainbow effect
            break;
        case GAME:
            // Input
            while (checkButtonQueueWrapper(&evt))
            {
                // Move blocks down, up, right or left
                if (evt.down && evt.button & PB_DOWN){
                    t48SlideDown();
                } else if (evt.down && evt.button & PB_UP){
                    t48SlideUp();
                } else if (evt.down && evt.button & PB_LEFT){
                    t48SlideLeft();
                } else if (evt.down && evt.button & PB_RIGHT){
                    t48SlideRight();
                } 
                // Restart game if you hit start
                else if (evt.down && evt.button & PB_START){
                    t48StartGame();
                }
            }
            // Check game is done or "done"
            if(t48CheckWin()){
                t48->ds = WIN;
            }
            if (t48CheckOver()){
                t48->ds = GAMEOVER;
            }
            // Draw
            t48Draw();
            break;
        case GAMEOVER:
            // Check any button is pressed
            while (checkButtonQueueWrapper(&evt))
            {
                if (evt.down && (evt.button & PB_A || evt.button & PB_B)){
                    t48StartGame();
                    t48->ds = GAME;
                }
            }
            // Draw
            t48DrawGameOverScreen(t48->score);
            break;
        case WIN:
            // Check any button is pressed
            while (checkButtonQueueWrapper(&evt))
            {
                if (evt.down && (evt.button & PB_A || evt.button & PB_B)){
                    t48->ds = GAME;
                }
            }
            // Draw
            t48DrawWinScreen();
            break;
        default:
            break;
    }
}

// Game functions

static int t48SetRandCell()
{
    int8_t cell = -1;
    while (t48->boardArr[cell / GRID_SIZE][cell % GRID_SIZE] != 0){
        cell = esp_random() % BOARD_SIZE;
    }
    int8_t rand = esp_random() % 10;
    if (rand == 0){
        t48->boardArr[cell / GRID_SIZE][cell % GRID_SIZE] = 4;  // 10%
    } else {
        t48->boardArr[cell / GRID_SIZE][cell % GRID_SIZE] = 2;  // 90%
    }
    
    return cell;
}

static bool t48MergeSlice(int slice[], bool updated)
{
    for (int8_t i = 0; i < GRID_SIZE - 1; i++){
        // Merge
        if (slice[i] == slice[i + 1]){
            if(slice[i] == 0) {continue;}
            updated = true;
            slice[i] *= 2;
            t48->score += slice[i];
            // Move if merged
            for (int j = i + 1; j < GRID_SIZE; j++){
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
    for (int col = 0; col < GRID_SIZE; col++){
        // Create a slice to merge
        int32_t slice[GRID_SIZE] = {0};
        // Load only cells with value into slice in the order:
        // Bottom -> Top
        for (int8_t row = GRID_SIZE - 1, i = 0; row >= 0; row--){
            // Only copy over values > 0, automatically moving all non-zeroes
            if (t48->boardArr[row][col] != 0){
                slice[i++] = t48->boardArr[row][col];
                if (row != (GRID_SIZE - i)){
                    // If these go out of sync, board has updated
                    updated = true;
                }
            }
        }
        // Merge. If merge happens, update
        updated = t48MergeSlice(slice, updated);
        // Copy modified slice back into board array
        for (int8_t row = GRID_SIZE - 1, i = 0; row >= 0; row--){
            t48->boardArr[row][col] = slice[i++];
        }
    }
    // If a board updated, add a new cell
    if (updated){
        t48SetRandCell();
    }
}

static void t48SlideUp()
{
    bool updated = false;
    for (int col = 0; col < GRID_SIZE; col++){
        // Create a slice to merge
        int32_t slice[GRID_SIZE] = {0};
        // Load only cells with value into slice in the order:
        // Top -> Bottom
        for (int8_t row = 0, i = 0; row <= GRID_SIZE - 1; row++){
            // Only copy over values > 0, automatically moving all non-zeroes
            if (t48->boardArr[row][col] != 0){
                if (row != i){
                    // If these go out of sync, board has updated
                    updated = true;
                }
                slice[i++] = t48->boardArr[row][col];
            }
        }
        // Merge. If merge happens, update
        updated = t48MergeSlice(slice, updated);
        // Copy modified slice back into board array
        for (int8_t row = 0, i = 0; row <= GRID_SIZE - 1; row++){
            t48->boardArr[row][col] = slice[i++];
        }
    }
    // If a board updated, add a new cell
    if (updated){
        t48SetRandCell();
    }
}

static void t48SlideRight()
{
    bool updated = false;
    for (int row = 0; row < GRID_SIZE; row++){
        // Create a slice to merge
        int32_t slice[GRID_SIZE] = {0};
        // Load only cells with value into slice in the order:
        // Right -> Left
        for (int8_t col = GRID_SIZE - 1, i = 0; col >= 0; col--){
            // Only copy over values > 0, automatically moving all non-zeroes
            if (t48->boardArr[row][col] != 0){
                slice[i++] = t48->boardArr[row][col];
                if (col != (GRID_SIZE - i)){
                    // If these go out of sync, board has updated
                    updated = true;
                }
            }
        }
        // Merge. If merge happens, update
        updated = t48MergeSlice(slice, updated);
        // Copy modified slice back into board array
        for (int8_t col = GRID_SIZE - 1, i = 0; col >= 0; col--){
            t48->boardArr[row][col] = slice[i++];
        }
    }
    // If a board updated, add a new cell
    if (updated){
        t48SetRandCell();
    }
}

static void t48SlideLeft()
{
    bool updated = false;
    for (int row = 0; row < GRID_SIZE; row++){
        // Create a slice to merge
        int32_t slice[GRID_SIZE] = {0};
        // Load only cells with value into slice in the order:
        // Left -> Right
        for (int8_t col = 0, i = 0; col <= GRID_SIZE - 1; col++){
            // Only copy over values > 0, automatically moving all non-zeroes
            if (t48->boardArr[row][col] != 0){
                if (col != i){
                    // If these go out of sync, board has updated
                    updated = true;
                }
                slice[i++] = t48->boardArr[row][col];
            }
        }
        // Merge. If merge happens, update
        updated = t48MergeSlice(slice, updated);
        // Copy modified slice back into board array
        for (int8_t col = 0, i = 0; col <= GRID_SIZE - 1; col++){
            t48->boardArr[row][col] = slice[i++];
        }
    }
    // If a board updated, add a new cell
    if (updated){
        t48SetRandCell();
    }
}

// Game state

static void t48StartGame()
{
    // clear the board
    for (int i = 0; i < BOARD_SIZE; i++){
        t48->boardArr[i / GRID_SIZE][i % GRID_SIZE] = 0;
    }
    t48->alreadyWon = false;
    t48->score = 0;
    // Get random places to start
    t48SetRandCell();
    t48SetRandCell();
}

static bool t48CheckWin()
{
    if(t48->alreadyWon){
        return false;
    }
    for (int i = 0; i < BOARD_SIZE; i ++){
        if (t48->boardArr[i / GRID_SIZE][i % GRID_SIZE] == 2048){
            t48->alreadyWon = true;
            return true;
        }
    }
    return false;
}

static bool t48CheckOver()
{
    // Check if any cells are open
    for (int i = 0; i < BOARD_SIZE; i++){
        if(t48->boardArr[i / GRID_SIZE][i % GRID_SIZE] == 0){
            return false;
        }
    }
    // Check if any two consecutive block match vertically
    for (int row = 0; row < GRID_SIZE; row++){
        for (int col = 0; col < GRID_SIZE - 1; col++){  // -1 to account for comparison
            if (t48->boardArr[col][row] == t48->boardArr[col + 1][row]){
                return false;
            }
        }
    }
    // Check if any two consecutive block match horizontally
    for (int row = 0; row < GRID_SIZE - 1 ; row++){  // -1 to account for comparison
        for (int col = 0; col < GRID_SIZE - 1; col++){ 
            if (t48->boardArr[col][row] == t48->boardArr[col][row + 1]){
                return false;
            }
        }
    }
    // Game is over
    return true;
}

// Visuals

static uint8_t getColor(uint32_t val)
{
    switch(val){
        case 0:
            return 0;
            break;
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
            return 16;
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
    for(int i = 0; i < 5; i++){
        int16_t left = i * (T48_CELL_SIZE + T48_LINE_WEIGHT);
        fillDisplayArea(SIDE_MARGIN + left, TOP_MARGIN, SIDE_MARGIN + left + T48_LINE_WEIGHT, TFT_HEIGHT, c111);
    }
    
    // Draw horizontal grid lines
    for(int i = 0; i < 5; i++){
        int16_t top = i * (T48_CELL_SIZE + T48_LINE_WEIGHT);
        fillDisplayArea(SIDE_MARGIN, top + TOP_MARGIN, TFT_WIDTH - SIDE_MARGIN, \
        top + TOP_MARGIN + T48_LINE_WEIGHT, c111);
    }

    // Score
    static char textBuffer[32];
    snprintf(textBuffer, sizeof(textBuffer)-1, "Score: %" PRIu32, t48->score);
    strcpy(t48->scoreStr, textBuffer);
    drawText(&t48->font, c555, t48->scoreStr, SIDE_MARGIN, 4);

    // Cells
    int16_t side_offset = SIDE_MARGIN + T48_LINE_WEIGHT;
    int16_t top_offset = TOP_MARGIN + T48_LINE_WEIGHT;
    static char buffer[16];
    for (int col = 0; col < GRID_SIZE; col++){
        for (int row = 0; row < GRID_SIZE; row++){
            // Grab the current value of the cell
            uint32_t val = t48->boardArr[col][row];
            // Bail if 0
            if (val == 0) {continue;}
            // Grab the offest based on cell
            uint16_t y_cell_offset = col * (T48_CELL_SIZE + T48_LINE_WEIGHT);
            uint16_t x_cell_offset = row * (T48_CELL_SIZE + T48_LINE_WEIGHT);
            // Convert int to char
            snprintf(buffer, sizeof(buffer)-1, "%" PRIu32, val);
            // Get the center of the text
            uint16_t text_center = (textWidth(&t48->font, buffer))/2;
            // Get associated color
            drawWsgSimple(&t48->tiles[getColor(val)], side_offset + x_cell_offset, 
                          top_offset + y_cell_offset);
            // Draw the text
            drawText(&t48->font, c555, buffer, 
                     side_offset + x_cell_offset - text_center + T48_CELL_SIZE/2, 
                     top_offset + y_cell_offset - 4 + T48_CELL_SIZE/2);
        }
    }
}

static void t48StartScreen(uint8_t color)
{
    // Blank
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);
    // Title
    drawText(&t48->titleFont, color, modeName, 
             (TFT_WIDTH - textWidth(&t48->titleFont, modeName))/2, 
             TFT_HEIGHT/2 - 12);
    // Press any key...
    drawText(&t48->font, c555, pressKey, 
             (TFT_WIDTH - textWidth(&t48->font, pressKey))/2, TFT_HEIGHT - 64);
}

static void t48DrawGameOverScreen(int64_t score)
{
    // Blank
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);

    // Display final score
    static char textBuffer[32];
    snprintf(textBuffer, sizeof(textBuffer)-1, "Final score: %" PRIu64, score);
    drawText(&t48->titleFont, c550, textBuffer, 16, TFT_HEIGHT/2 - 24);
    drawText(&t48->font, c555, pressAB, 16, TFT_HEIGHT/2 + 24);
}

static void t48DrawWinScreen(void)
{
    // Blank
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);
    // Title
    drawText(&t48->titleFont, c055, youWin, 
             (TFT_WIDTH - textWidth(&t48->titleFont, youWin))/2, 48);
    // Press any key...
    drawText(&t48->font, c555, continueAB, 
             (TFT_WIDTH - textWidth(&t48->font, continueAB))/2, TFT_HEIGHT - 64);
}