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

const char modeName[] = "1234";

const colorPair_t colors[18] = {
    {.val = 0, .color = c222},
    {.val = 2, .color = c322},
    {.val = 4, .color = c232},
    {.val = 8, .color = c223},
    {.val = 16, .color = c332},
    {.val = 32, .color = c323},
    {.val = 64, .color = c233},
    {.val = 128, .color = c333},
    {.val = 256, .color = c433},
    {.val = 512, .color = c343},
    {.val = 1024, .color = c334},
    {.val = 2048, .color = c433},
    {.val = 4096, .color = c434},
    {.val = 8192, .color = c344},
    {.val = 16384, .color = c444},
    {.val = 32768, .color = c544},
    {.val = 65536, .color = c454},
    {.val = 131072, .color = c445},
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

static void t48EnterMode(void)
{
    setFrameRateUs(T48_US_PER_FRAME);
    t48 = calloc(sizeof(t48_t), 1);
    loadFont("sonic.font", &t48->font, false);

    t48->score = 1234;
}

static void t48ExitMode(void)
{
    freeFont(&t48->font);
    free(t48);
}

static void t48MainLoop(int64_t elapsedUs)
{
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        t48->presses += 1;
    }
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
    snprintf(textBuffer, sizeof(textBuffer)-1, "LOL %" PRIu32, t48->score);
    strcpy(t48->scoreStr, textBuffer);
    drawText(&t48->font, c555, modeName, 20, 26);
}

static void t48BGCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum){}