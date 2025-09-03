#include "canvas.h"
#include "wsgCanvas.h"

static const char canvasMode[] = "CanvasTest";

static void stEnterMode(void);
static void stExitMode(void);
static void stMainLoop(int64_t elapsedUs);

typedef struct
{
    wsg_t canvas;
    wsgPalette_t pal;
} stData_t;

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

static void stEnterMode(void)
{
    st = heap_caps_calloc(sizeof(stData_t), 1, MALLOC_CAP_8BIT);
    canvasBlankInit(&st->canvas, 32, 32, c555, true);
    canvasDraw(&st->canvas, KID_0_WSG, -16, -16);
    canvasDraw(&st->canvas, KID_0_WSG, 16, 16);
    canvasDraw(&st->canvas, KID_0_WSG, 16, -16);
    wsgPaletteReset(&st->pal);
    wsgPaletteSet(&st->pal, c555, c050);
    wsgPaletteSet(&st->pal, cTransparent, c000);
    canvasDrawPalette(&st->canvas, KID_0_WSG, -16, 16, st->pal);
}

static void stExitMode(void)
{
    freeWsg(&st->canvas);
}

static void stMainLoop(int64_t elapsedUs)
{
    drawWsgSimpleScaled(&st->canvas, 32, 32, 3, 3);
}