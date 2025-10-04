#include "ccmgSew.h"
#include "cosCrunch.h"

static void ccmgSewInitMicrogame(void);
static void ccmgSewDestroyMicrogame(void);
static void ccmgSewMainLoop(int64_t elapsedUs, uint64_t timeRemainingUs, float timeScale, cosCrunchMicrogameState state,
                            buttonEvt_t buttonEvts[], uint8_t buttonEvtCount);
static void ccmgSewDrawThreadBezier(int16_t startX, int16_t startY, int16_t endX, int16_t endY);

#define STITCH_Y           119
#define BUTTON_PRESS_COUNT 10

static const char ccmgSewVerb[]       = "Sew";
static const char ccmgSewSuccessMsg[] = "Nice stitches";
static const char ccmgSewFailureMsg[] = "Get the seam ripper";
const cosCrunchMicrogame_t ccmgSew    = {
       .verb                     = ccmgSewVerb,
       .successMsg               = ccmgSewSuccessMsg,
       .failureMsg               = ccmgSewFailureMsg,
       .timeoutUs                = 5000000,
       .fnInitMicrogame          = ccmgSewInitMicrogame,
       .fnDestroyMicrogame       = ccmgSewDestroyMicrogame,
       .fnMainLoop               = ccmgSewMainLoop,
       .fnBackgroundDrawCallback = NULL,
       .fnMicrogameTimeout       = NULL,
};

typedef struct
{
    buttonBit_t buttonOrder[BUTTON_PRESS_COUNT];
    vec_t needlePos[BUTTON_PRESS_COUNT];
} stitchType_t;

const stitchType_t stitchTypeStraight = {
    .buttonOrder = {PB_DOWN, PB_RIGHT, PB_UP, PB_RIGHT, PB_DOWN, PB_RIGHT, PB_UP, PB_RIGHT, PB_DOWN, PB_RIGHT},
    .needlePos = {{30, 0}, {40, 0}, {80, 0}, {90, -50}, {130, 0}, {140, 0}, {180, 0}, {190, -50}, {230, 0}, {240, 0}}};

const stitchType_t stitchTypeBackstitch = {
    .buttonOrder = {PB_DOWN, PB_RIGHT, PB_UP, PB_LEFT, PB_DOWN, PB_RIGHT, PB_UP, PB_LEFT, PB_DOWN, PB_RIGHT},
    .needlePos = {{50, 0}, {80, 0}, {135, 0}, {85, -30}, {56, 0}, {115, 0}, {230, 0}, {170, -30}, {141, 0}, {240, 0}}};

typedef struct
{
    struct
    {
        wsg_t fabric;
        wsg_t fabricEdge;
        wsg_t needleEye;
        wsg_t needlePoint;
        wsg_t threadKnot;
        wsg_t canvas;
    } wsg;

    const tintColor_t* tintColor;
    wsgPalette_t tintPalette;

    uint8_t currentStep;
    vec_t threadBezier[4];
    buttonBit_t pressedButton;

    const stitchType_t* stitchType;
} ccmgSew_t;
ccmgSew_t* ccmgsew = NULL;

static void ccmgSewInitMicrogame(void)
{
    ccmgsew = heap_caps_calloc(1, sizeof(ccmgSew_t), MALLOC_CAP_8BIT);

    loadWsg(CC_FABRIC_WSG, &ccmgsew->wsg.fabric, false);
    loadWsg(CC_FABRIC_EDGE_WSG, &ccmgsew->wsg.fabricEdge, false);
    loadWsg(CC_SEWING_NEEDLE_EYE_WSG, &ccmgsew->wsg.needleEye, false);
    loadWsg(CC_SEWING_NEEDLE_POINT_WSG, &ccmgsew->wsg.needlePoint, false);
    loadWsg(CC_THREAD_KNOT_WSG, &ccmgsew->wsg.threadKnot, false);

    ccmgsew->wsg.canvas.w  = MAX(ccmgsew->wsg.threadKnot.w, ccmgsew->wsg.threadKnot.h);
    ccmgsew->wsg.canvas.h  = ccmgsew->wsg.canvas.w;
    ccmgsew->wsg.canvas.px = (paletteColor_t*)heap_caps_malloc_tag(
        sizeof(paletteColor_t) * ccmgsew->wsg.canvas.w * ccmgsew->wsg.canvas.h, MALLOC_CAP_8BIT, "wsg");
    for (uint32_t i = 0; i < ccmgsew->wsg.canvas.w * ccmgsew->wsg.canvas.h; i++)
    {
        ccmgsew->wsg.canvas.px[i] = cTransparent;
    }

    ccmgsew->tintColor = cosCrunchMicrogameGetTintColor();
    wsgPaletteReset(&ccmgsew->tintPalette);
    tintPalette(&ccmgsew->tintPalette, ccmgsew->tintColor);

    // Pre-tint large images so we can memcpy
    drawToCanvasTint(ccmgsew->wsg.fabric, ccmgsew->wsg.fabric, 0, 0, 0, ccmgsew->tintColor);
    drawToCanvasTint(ccmgsew->wsg.fabricEdge, ccmgsew->wsg.fabricEdge, 0, 0, 0, ccmgsew->tintColor);

    if (esp_random() % 2 == 0)
    {
        ccmgsew->stitchType = &stitchTypeStraight;
    }
    else
    {
        ccmgsew->stitchType = &stitchTypeBackstitch;
    }
}

static void ccmgSewDestroyMicrogame(void)
{
    if (ccmgsew->currentStep < BUTTON_PRESS_COUNT)
    {
        uint16_t rotation = esp_random() % 360;
        drawToCanvasTint(ccmgsew->wsg.canvas, ccmgsew->wsg.threadKnot,
                         (ccmgsew->wsg.canvas.w - ccmgsew->wsg.threadKnot.w) / 2,
                         (ccmgsew->wsg.canvas.h - ccmgsew->wsg.threadKnot.h) / 2, rotation, ccmgsew->tintColor);
        cosCrunchMicrogamePersistSplatter(ccmgsew->wsg.canvas, esp_random() % (TFT_WIDTH - ccmgsew->wsg.canvas.w),
                                          esp_random() % (TFT_HEIGHT - ccmgsew->wsg.canvas.h - 40));
    }

    freeWsg(&ccmgsew->wsg.fabric);
    freeWsg(&ccmgsew->wsg.fabricEdge);
    freeWsg(&ccmgsew->wsg.needleEye);
    freeWsg(&ccmgsew->wsg.needlePoint);
    freeWsg(&ccmgsew->wsg.threadKnot);
    freeWsg(&ccmgsew->wsg.canvas);

    heap_caps_free(ccmgsew);
}

static void ccmgSewMainLoop(int64_t elapsedUs, uint64_t timeRemainingUs, float timeScale, cosCrunchMicrogameState state,
                            buttonEvt_t buttonEvts[], uint8_t buttonEvtCount)
{
    if (state == CC_MG_PLAYING)
    {
        buttonBit_t pressedButton = 0;
        for (uint8_t i = 0; i < buttonEvtCount; i++)
        {
            if (buttonEvts[i].down
                && (buttonEvts[i].button == PB_UP || buttonEvts[i].button == PB_DOWN || buttonEvts[i].button == PB_LEFT
                    || buttonEvts[i].button == PB_RIGHT))
            {
                pressedButton = buttonEvts[i].button;
            }
        }
        if (pressedButton == ccmgsew->stitchType->buttonOrder[ccmgsew->currentStep])
        {
            ccmgsew->currentStep++;
            if (ccmgsew->currentStep == BUTTON_PRESS_COUNT)
            {
                cosCrunchMicrogameResult(true);
            }
        }
    }

    bool eyeUnderFabric   = false;
    bool pointUnderFabric = false;
    if (ccmgsew->currentStep < BUTTON_PRESS_COUNT)
    {
        if (ccmgsew->stitchType->buttonOrder[ccmgsew->currentStep] == PB_UP)
        {
            eyeUnderFabric   = true;
            pointUnderFabric = false;
        }
        else if (ccmgsew->stitchType->buttonOrder[ccmgsew->currentStep] == PB_DOWN)
        {
            eyeUnderFabric   = false;
            pointUnderFabric = true;
        }
        else
        {
            for (int i = ccmgsew->currentStep; i >= 0; i--)
            {
                buttonBit_t btn = ccmgsew->stitchType->buttonOrder[i];
                if (btn == PB_UP)
                {
                    eyeUnderFabric   = false;
                    pointUnderFabric = false;
                    break;
                }
                if (btn == PB_DOWN)
                {
                    eyeUnderFabric   = true;
                    pointUnderFabric = true;
                    break;
                }
            }
        }
    }

    bool mirrorLR;
    int32_t rotateDeg;
    int16_t needlePointXOffset, needlePointYOffset;
    int16_t needleEyeXOffset, needleEyeYOffset;
    int16_t threadEyeXOffset, threadEyeYOffset;
    int16_t threadStartX, threadStartY;

    switch (ccmgsew->stitchType->buttonOrder[ccmgsew->currentStep])
    {
        case PB_LEFT:
            mirrorLR           = false;
            rotateDeg          = 90;
            needlePointXOffset = 0;
            needlePointYOffset = 0;
            needleEyeXOffset   = ccmgsew->wsg.needleEye.h;
            needleEyeYOffset   = 0;
            threadEyeXOffset   = needleEyeXOffset + 8;
            threadEyeYOffset   = needleEyeYOffset + 12;
            break;
        case PB_RIGHT:
            mirrorLR           = true;
            rotateDeg          = 270;
            needlePointXOffset = 30;
            needlePointYOffset = 20;
            needleEyeXOffset   = needlePointXOffset - ccmgsew->wsg.needleEye.h;
            needleEyeYOffset   = needlePointYOffset;
            threadEyeXOffset   = needleEyeXOffset - 4;
            threadEyeYOffset   = needleEyeYOffset + 12;
            break;
        case PB_UP:
            mirrorLR           = true;
            rotateDeg          = 180;
            needlePointXOffset = 0;
            needlePointYOffset = -ccmgsew->wsg.needleEye.h;
            needleEyeXOffset   = 0;
            needleEyeYOffset   = needlePointYOffset + ccmgsew->wsg.needleEye.h;
            threadEyeXOffset   = needleEyeXOffset + 2;
            threadEyeYOffset   = needleEyeYOffset + 18;
            break;
        case PB_DOWN:
        default:
            mirrorLR           = false;
            rotateDeg          = 0;
            needlePointXOffset = 0;
            needlePointYOffset = 0;
            needleEyeXOffset   = 0;
            needleEyeYOffset   = -ccmgsew->wsg.needleEye.h;
            threadEyeXOffset   = needleEyeXOffset + 2;
            threadEyeYOffset   = needleEyeYOffset + 6;
            break;
    }

    if (ccmgsew->currentStep == 0)
    {
        threadStartX = 0;
        threadStartY = STITCH_Y - 50;
    }
    else
    {
        uint8_t lastStep = ccmgsew->currentStep - 1;
        while (lastStep > 0 && ccmgsew->stitchType->buttonOrder[lastStep] != PB_UP
               && ccmgsew->stitchType->buttonOrder[lastStep] != PB_DOWN)
        {
            lastStep--;
        }
        threadStartX = ccmgsew->stitchType->needlePos[lastStep].x;
        threadStartY = STITCH_Y;
    }

    if (ccmgsew->currentStep < BUTTON_PRESS_COUNT)
    {
        if (eyeUnderFabric)
        {
            drawWsg(&ccmgsew->wsg.needleEye, ccmgsew->stitchType->needlePos[ccmgsew->currentStep].x + needleEyeXOffset,
                    STITCH_Y + ccmgsew->stitchType->needlePos[ccmgsew->currentStep].y + needleEyeYOffset, mirrorLR,
                    false, rotateDeg);
            ccmgSewDrawThreadBezier(
                threadStartX, threadStartY, ccmgsew->stitchType->needlePos[ccmgsew->currentStep].x + threadEyeXOffset,
                STITCH_Y + ccmgsew->stitchType->needlePos[ccmgsew->currentStep].y + threadEyeYOffset);
        }
        if (pointUnderFabric)
        {
            drawWsg(&ccmgsew->wsg.needlePoint,
                    ccmgsew->stitchType->needlePos[ccmgsew->currentStep].x + needlePointXOffset,
                    STITCH_Y + ccmgsew->stitchType->needlePos[ccmgsew->currentStep].y + needlePointYOffset, mirrorLR,
                    false, rotateDeg);
        }
    }

    memcpy(&getPxTftFramebuffer()[0], &ccmgsew->wsg.fabric.px[0],
           ccmgsew->wsg.fabric.w * ccmgsew->wsg.fabric.h * sizeof(paletteColor_t));
    drawWsgSimple(&ccmgsew->wsg.fabricEdge, 0, ccmgsew->wsg.fabric.h);

    if (ccmgsew->currentStep < BUTTON_PRESS_COUNT)
    {
        if (!eyeUnderFabric)
        {
            drawWsg(&ccmgsew->wsg.needleEye, ccmgsew->stitchType->needlePos[ccmgsew->currentStep].x + needleEyeXOffset,
                    STITCH_Y + ccmgsew->stitchType->needlePos[ccmgsew->currentStep].y + needleEyeYOffset, mirrorLR,
                    false, rotateDeg);
            ccmgSewDrawThreadBezier(
                threadStartX, threadStartY, ccmgsew->stitchType->needlePos[ccmgsew->currentStep].x + threadEyeXOffset,
                STITCH_Y + ccmgsew->stitchType->needlePos[ccmgsew->currentStep].y + threadEyeYOffset);
        }
        if (!pointUnderFabric)
        {
            drawWsg(&ccmgsew->wsg.needlePoint,
                    ccmgsew->stitchType->needlePos[ccmgsew->currentStep].x + needlePointXOffset,
                    STITCH_Y + ccmgsew->stitchType->needlePos[ccmgsew->currentStep].y + needlePointYOffset, mirrorLR,
                    false, rotateDeg);
        }
    }

    // Draw completed stitches
    for (uint8_t step = 0; step < ccmgsew->currentStep; step++)
    {
        if (ccmgsew->stitchType->buttonOrder[step] != PB_DOWN)
        {
            continue;
        }

        uint8_t lastStep = MAX(step - 1, 0);
        while (lastStep > 0 && ccmgsew->stitchType->buttonOrder[lastStep] != PB_UP)
        {
            lastStep--;
        }

        int16_t x1, x2;
        if (lastStep == 0)
        {
            x1 = 0;
        }
        else
        {
            x1 = ccmgsew->stitchType->needlePos[lastStep].x;
        }
        if (step == BUTTON_PRESS_COUNT)
        {
            x2 = TFT_WIDTH;
        }
        else
        {
            x2 = ccmgsew->stitchType->needlePos[step].x;
        }

        int16_t minX = MIN(x1, x2);
        int16_t maxX = MAX(x1, x2);
        drawLineFast(minX, STITCH_Y, maxX, STITCH_Y, ccmgsew->tintColor->lowlight);
        setPxTft(minX - 1, STITCH_Y + 1, ccmgsew->tintColor->lowlight);
        setPxTft(maxX + 1, STITCH_Y + 1, ccmgsew->tintColor->lowlight);
    }
}

static void ccmgSewDrawThreadBezier(int16_t startX, int16_t startY, int16_t endX, int16_t endY)
{
    if (ccmgsew->threadBezier[0].x != startX || ccmgsew->threadBezier[3].x != endX
        || ccmgsew->threadBezier[0].y != startY || ccmgsew->threadBezier[3].y != endY)
    {
        ccmgsew->threadBezier[0].x = startX;
        ccmgsew->threadBezier[0].y = startY;
        ccmgsew->threadBezier[3].x = endX;
        ccmgsew->threadBezier[3].y = endY;

        int16_t minX = MIN(ccmgsew->threadBezier[0].x, ccmgsew->threadBezier[3].x) - 40;
        int16_t maxX = MAX(ccmgsew->threadBezier[0].x, ccmgsew->threadBezier[3].x) + 40;
        int16_t minY = MIN(ccmgsew->threadBezier[0].y, ccmgsew->threadBezier[3].y) - 40;
        int16_t maxY = MAX(ccmgsew->threadBezier[0].y, ccmgsew->threadBezier[3].y) + 40;
        for (int i = 1; i < 3; i++)
        {
            ccmgsew->threadBezier[i].x = esp_random() % ABS(maxX - minX) + minX;
            ccmgsew->threadBezier[i].y = esp_random() % ABS(maxY - minY) + minY;
        }
    }

    drawCubicBezier(ccmgsew->threadBezier[0].x, ccmgsew->threadBezier[0].y, ccmgsew->threadBezier[1].x,
                    ccmgsew->threadBezier[1].y, ccmgsew->threadBezier[2].x, ccmgsew->threadBezier[2].y,
                    ccmgsew->threadBezier[3].x, ccmgsew->threadBezier[3].y, ccmgsew->tintColor->lowlight);
}
