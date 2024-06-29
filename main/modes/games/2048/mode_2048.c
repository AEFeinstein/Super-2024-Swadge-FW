/**
 * @file mode_2048.c
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief A game of 2048 for 2024-2025 Swadge hardware
 * @version 0.1
 * @date 2024-06-28
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include "mode_2048.h"

//==============================================================================
// Variables
//==============================================================================

const char modeName[] = "2048";

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

/**
 * @brief Mode setup
 * 
 */
static void t48EnterMode(void)
{
    setFrameRateUs(T48_US_PER_FRAME);
    t48 = calloc(sizeof(t48_t), 1);
    loadFont("ibm_vga8.font", &t48->font, false);

    t48StartGame();  // First run only adds one block... so just run it twice!
    t48StartGame();
}

/**
 * @brief Mode teardown
 * 
 */
static void t48ExitMode(void)
{
    freeFont(&t48->font);
    free(t48);
}

/**
 * @brief Main loop of the code
 * 
 * @param elapsedUs 
 */
static void t48MainLoop(int64_t elapsedUs)
{
    if(t48CheckWin()){

    }

    if (t48CheckOver()){
        // FIXME: does not actually reset?
        t48StartGame();
    }
    // Input
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        if (evt.down && evt.button & PB_A){
            t48SetRandCell();
        }
    }

    // Draw
    t48Draw();
}

/**
 * @brief Sets an empty cell to 2 or 4 on 50/50 basis
 * 
 * @return int Cell used
 */
static int t48SetRandCell()
{
    int8_t cell = -1;
    while (t48->boardArr[cell / GRID_SIZE][cell % GRID_SIZE] != 0){
        cell = esp_random() % BOARD_SIZE;
    }
    t48->boardArr[cell / GRID_SIZE][cell % GRID_SIZE] = ((esp_random() % 2) * 2) + 2;
    return cell;
}

/**
 * @brief Initializes a game
 * 
 */
static void t48StartGame()
{
    // clear the board
    for (int i = 0; i < BOARD_SIZE; i++){
        t48->boardArr[i / GRID_SIZE][i % GRID_SIZE] = 0;
    }
    t48->alreadyWon = false;
    // Get random places to start
    t48SetRandCell();
    t48SetRandCell();
}

/**
 * @brief Checks if the player has reached 2048 for tyhee first time
 * 
 * @return true     If this is the first time 2048 has been hit
 * @return false    Otherwise
 */
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

/**
 * @brief Checks if the game can no longer be played
 * 
 * @return true     Display final score and reset
 * @return false    Continue playing
 */
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

/**
 * @brief Draws the grid, score, and tiles
 * 
 */
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
            uint8_t color = getColor(val);
            fillDisplayArea(side_offset + x_cell_offset, top_offset + y_cell_offset, 
                            side_offset + x_cell_offset + T48_CELL_SIZE, 
                            top_offset + y_cell_offset + T48_CELL_SIZE, color);
            // Draw the text
            drawText(&t48->font, c555, buffer, 
            side_offset + x_cell_offset - text_center + T48_CELL_SIZE/2, 
            top_offset + y_cell_offset - 4 + T48_CELL_SIZE/2);
        }
    }
}

/**
 * @brief Get the Color for a specific value
 * 
 * @param val       Value that requires a color for its square
 * @return uint8_t  Color of the value's square
 */
static uint8_t getColor(uint32_t val)
{
    switch(val){
        case 0:
            return c552;
            break;
        case 2:
            return c441;
            break;
        case 4:
            return c505;
            break;
        case 8:
            return c515;
            break;
        case 16:
            return c525;
            break;
        case 32:
            return c535;
            break;
        case 64:
            return c005;
            break;
        case 128:
            return c115;
            break;
        case 256:
            return c225;
            break;
        case 512:
            return c335;
            break;
        case 1024:
            return c050;
            break;
        case 2048:
            return c151;
            break;
        case 4096:
            return c252;
            break;
        case 8192:
            return c353;
            break;
        case 16384:
            return c500;
            break;
        case 32768:
            return c511;
            break;
        case 65535:
            return c522;
            break;
        case 131072:
            return c533;
            break;
        default:
            return c101;
            break;
    }
}