/**
 * @file mode_2048.c
 * @author your name (you@domain.com)
 * @brief 
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

static void t48EnterMode(void)
{
    setFrameRateUs(T48_US_PER_FRAME);
    t48 = calloc(sizeof(t48_t), 1);
    loadFont("ibm_vga8.font", &t48->font, false);

    t48->score = 4326543;
    for (int i = 0; i < 4; i++){
        for (int j = 0; j < 4; j++){
            t48->boardArr[i][j] = pow(2, i*4 + j);
        }
    }
}

static void t48ExitMode(void)
{
    freeFont(&t48->font);
    free(t48);
}

static void t48MainLoop(int64_t elapsedUs)
{
    // Input
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        t48->presses += 1;
    }

    // Draw
    t48Draw();
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
    for (int i = 0; i < 4; i++){
        for (int j = 0; j < 4; j++){
            // Grab the current value of the cell
            uint32_t val = t48->boardArr[i][j];
            // Bail if 0
            if (val == 0) {continue;}
            // Grab the offest bease on cell
            uint16_t y_cell_offset = i * (T48_CELL_SIZE + T48_LINE_WEIGHT);
            uint16_t x_cell_offset = j * (T48_CELL_SIZE + T48_LINE_WEIGHT);
            // Convert int to char
            snprintf(buffer, sizeof(buffer)-1, "%" PRIu32, val);
            // Get the center of the text
            uint16_t text_center = (textWidth(&t48->font, buffer))/2;
            // Get associated color
            uint8_t col = getColor(val);
            fillDisplayArea(side_offset + x_cell_offset, top_offset + y_cell_offset, 
                            side_offset + x_cell_offset + T48_CELL_SIZE, 
                            top_offset + y_cell_offset + T48_CELL_SIZE, col);
            // Draw the text
            drawText(&t48->font, c555, buffer, 
            side_offset + x_cell_offset - text_center + T48_CELL_SIZE/2, 
            top_offset + y_cell_offset - 4 + T48_CELL_SIZE/2);
        }
    }
}

static uint8_t getColor(uint32_t val){
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