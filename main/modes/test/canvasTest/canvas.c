//==============================================================================
// Includes
//==============================================================================

#include "canvas.h"
#include "wsgCanvas.h"
#include "esp_random.h"
#include "esp_heap_caps.h"

//==============================================================================
// Consts
//==============================================================================

static const char canvasMode[] = "CanvasTest";
static const char* const strings[]
    = {"Press 'A' to change color", "Press arrows to spawn arrows", "Press 'B' to show/hide examples"};

//==============================================================================
// Function declarations
//==============================================================================

static void stEnterMode(void);
static void stExitMode(void);
static void stMainLoop(int64_t elapsedUs);

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    wsg_t canvas;
    wsg_t donut;
    wsg_t donut2;
    bool displayDonut;
    wsgPalette_t pal;
    wsgPalette_t pal2;
    paletteColor_t currColor;
} stData_t;

//==============================================================================
// Variables
//==============================================================================

swadgeMode_t canvasTestMode = {
    .modeName          = canvasMode,
    .wifiMode          = NO_WIFI,
    .overrideUsb       = false,
    .usesAccelerometer = false,
    .usesThermometer   = false,
    .overrideSelectBtn = false,
    .fnEnterMode       = stEnterMode,
    .fnExitMode        = stExitMode,
    .fnMainLoop        = stMainLoop,
};

static stData_t* st;

//==============================================================================
// Functions
//==============================================================================

static void stEnterMode(void)
{
    // Init
    st = heap_caps_calloc(sizeof(stData_t), 1, MALLOC_CAP_8BIT);

    // Main canvas
    canvasBlankInit(&st->canvas, TFT_WIDTH, TFT_HEIGHT, c333, true);
    wsgPaletteReset(&st->pal);
    st->currColor = esp_random() % cTransparent;
    wsgPaletteSet(&st->pal, c555, st->currColor);

    // Secondary canvas
    wsgPaletteReset(&st->pal2);
    wsgPaletteSet(&st->pal2, cTransparent, c050);
    canvasBlankInit(&st->donut, 32, 32, c333, true);
    canvasDrawSimple(&st->donut, KID_0_WSG, -16, -16);
    canvasDraw(&st->donut, KID_0_WSG, 16, -16, false, false, 45);
    canvasDrawSimplePal(&st->donut, KID_0_WSG, -16, 16, st->pal2);
    canvasDraw(&st->donut, KID_0_WSG, 16, 16, true, false, 0);

    // Tertiary canvas
    canvasBlankInit(&st->donut2, 32, 32, cTransparent, true);
    canvasDraw(&st->donut2, KID_1_WSG, 0, 0, false, true, 235);
}

static void stExitMode(void)
{
    canvasFree(&st->canvas);
}

static void stMainLoop(int64_t elapsedUs)
{
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        if (evt.down)
        {
            if (evt.button & PB_A)
            {
                st->currColor = esp_random() % cTransparent;
            }
            else if (evt.button & PB_B)
            {
                st->displayDonut = !st->displayDonut;
            }
            else if (evt.button & PB_UP)
            {
                wsgPaletteSet(&st->pal, c555, st->currColor);
                canvasDrawSimplePal(&st->canvas, ARROW_19_WSG, esp_random() % (TFT_WIDTH - 19),
                                  esp_random() % (TFT_HEIGHT - 19), st->pal);
            }
            else if (evt.button & PB_DOWN)
            {
                wsgPaletteSet(&st->pal, c555, st->currColor);
                canvasDrawPal(&st->canvas, ARROW_19_WSG, esp_random() % (TFT_WIDTH - 19),
                                      esp_random() % (TFT_HEIGHT - 19), false, true, 0, st->pal);
            }
            else if (evt.button & PB_RIGHT)
            {
                wsgPaletteSet(&st->pal, c555, st->currColor);
                canvasDrawPal(&st->canvas, ARROW_19_WSG, esp_random() % (TFT_WIDTH - 19),
                                    esp_random() % (TFT_HEIGHT - 19), false, false, 90, st->pal);
            }
            else if (evt.button & PB_LEFT)
            {
                wsgPaletteSet(&st->pal, c555, st->currColor);
                canvasDrawPal(&st->canvas, ARROW_19_WSG, esp_random() % (TFT_WIDTH - 19),
                                    esp_random() % (TFT_HEIGHT - 19), false, true, 90, st->pal);
            }
        }
    }

    // Draw
    drawWsgSimple(&st->canvas, 0, 0);
    drawText(getSysFont(), c000, strings[0], 16, 8);
    drawText(getSysFont(), c000, strings[1], 16, 24);
    drawText(getSysFont(), c000, strings[2], 16, 40);
    drawRect(0, 0, TFT_WIDTH, TFT_HEIGHT, st->currColor);
    drawRect(1, 1, TFT_WIDTH - 1, TFT_HEIGHT - 1, st->currColor);
    if (st->displayDonut)
    {
        drawWsgSimpleScaled(&st->donut, 142, 72, 3, 3);
        drawRect(141, 72, 141 + 98, 71 + 98, c550);
        drawWsgSimpleScaled(&st->donut2, 42, 72, 3, 3);
        drawRect(41, 72, 41 + 98, 71 + 98, c550);
    }
    // TODO: Show allocated memory size (heap/SPI) to show that they're not increasing
}