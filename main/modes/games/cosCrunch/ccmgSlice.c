#include "ccmgSlice.h"
#include "cosCrunch.h"

static void ccmgSliceInitMicrogame(void);
static void ccmgSliceDestroyMicrogame(void);
static void ccmgSliceMainLoop(int64_t elapsedUs, uint64_t timeRemainingUs, cosCrunchMicrogameState state,
                              buttonEvt_t buttonEvts[], uint8_t buttonEvtCount);
static int16_t ccmgSliceGetYIntersect(int16_t angle, int16_t width);

#define KNIFE_Y_SPEED_PX_PER_US  (50.f / 1000000)
#define KNIFE_X_SPEED_DEG_PER_US (25.f / 1000000)
#define KNIFE_X_OFFSET           20

#define TARGET_AREA_CENTER_X      (TFT_WIDTH / 2)
#define TARGET_AREA_CENTER_Y      140
#define TARGET_AREA_CENTER_JITTER 50
#define TARGET_AREA_WIDTH         35
#define TARGET_AREA_HEIGHT        100
#define TARGET_AREA_WH_JITTER     20
#define TARGET_LINE_Y_MIN         30
#define TARGET_LINE_Y_MAX         60
#define TARGET_ANGLE_MAX          35

#define TOTAL_ERROR_THRESHOLD 5

#define END_ANIM_FADE_LENGTH_US        100000
#define END_ANIM_SLASH_START_TIME_US   800000
#define END_ANIM_SLASH_STOP_TIME_US    1000000
#define END_ANIM_SLASH_FRAME_LENGTH_US 150000
#define END_ANIM_FADE_END_TIME_US      1600000
#define END_ANIM_ELLIPSE_WIDTH         45
#define END_ANIM_ELLIPSE_HEIGHT        10

static const char ccmgSliceVerb[]    = "Slice";
const cosCrunchMicrogame_t ccmgSlice = {
    .verb                     = ccmgSliceVerb,
    .successMsg               = NULL,
    .failureMsg               = NULL,
    .timeoutUs                = 3500000,
    .resultDisplayTimeUs      = 2600000,
    .fnInitMicrogame          = ccmgSliceInitMicrogame,
    .fnDestroyMicrogame       = ccmgSliceDestroyMicrogame,
    .fnMainLoop               = ccmgSliceMainLoop,
    .fnBackgroundDrawCallback = NULL,
    .fnMicrogameTimeout       = NULL,
};

typedef struct
{
    struct
    {
        wsg_t knifeTop;
        wsg_t knifeBottom;
        wsg_t slash;
        wsg_t slashUnderlay;
        wsg_t slashOverlay;
        wsg_t canvas;
    } wsg;
    paletteColor_t canvasPixels[TFT_WIDTH * TFT_HEIGHT];

    const tintColor_t* tintColor;
    wsgPalette_t tintPalette;

    uint16_t buttonState;

    rectangle_t targetArea;
    int16_t targetAngle;
    uint16_t targetLineY;

    int16_t knifeX;
    float knifeBottomY;
    float knifeAngle;

    uint64_t endElapsedUs;
    uint64_t oldEndElapsedUs;
    bool endFadeComplete;
} ccmgSlice_t;
ccmgSlice_t* ccmgsl = NULL;

static void ccmgSliceInitMicrogame(void)
{
    ccmgsl = heap_caps_calloc(1, sizeof(ccmgSlice_t), MALLOC_CAP_8BIT);

    loadWsg(CC_KNIFE_TOP_WSG, &ccmgsl->wsg.knifeTop, false);
    loadWsg(CC_KNIFE_BOTTOM_WSG, &ccmgsl->wsg.knifeBottom, false);
    loadWsg(CC_SLASH_WSG, &ccmgsl->wsg.slash, false);
    loadWsg(CC_SLASH_UNDERLAY_WSG, &ccmgsl->wsg.slashUnderlay, false);
    loadWsg(CC_SLASH_OVERLAY_WSG, &ccmgsl->wsg.slashOverlay, false);

    ccmgsl->wsg.canvas.w  = TFT_WIDTH;
    ccmgsl->wsg.canvas.h  = TFT_HEIGHT;
    ccmgsl->wsg.canvas.px = ccmgsl->canvasPixels;
    for (uint32_t i = 0; i < ccmgsl->wsg.canvas.w * ccmgsl->wsg.canvas.h; i++)
    {
        ccmgsl->canvasPixels[i] = cTransparent;
    }

    ccmgsl->tintColor = cosCrunchMicrogameGetTintColor();
    wsgPaletteReset(&ccmgsl->tintPalette);
    tintPalette(&ccmgsl->tintPalette, ccmgsl->tintColor);

    ccmgsl->targetArea.width  = TARGET_AREA_WIDTH + esp_random() % TARGET_AREA_WH_JITTER - TARGET_AREA_WH_JITTER / 2;
    ccmgsl->targetArea.height = TARGET_AREA_HEIGHT + esp_random() % TARGET_AREA_WH_JITTER - TARGET_AREA_WH_JITTER / 2;
    // Width and height both need to be odd so the dithered shadow doesn't look weird
    ccmgsl->targetArea.width += ccmgsl->targetArea.width % 2 - 1;
    ccmgsl->targetArea.height += ccmgsl->targetArea.height % 2 - 1;

    ccmgsl->targetArea.pos.x = TARGET_AREA_CENTER_X
                               + (esp_random() % TARGET_AREA_CENTER_JITTER - TARGET_AREA_CENTER_JITTER / 2)
                               - ccmgsl->targetArea.width / 2;
    ccmgsl->targetArea.pos.y = TARGET_AREA_CENTER_Y
                               + (esp_random() % TARGET_AREA_CENTER_JITTER - TARGET_AREA_CENTER_JITTER / 2)
                               - ccmgsl->targetArea.height / 2;

    ccmgsl->targetAngle = esp_random() % (TARGET_ANGLE_MAX * 2) - TARGET_ANGLE_MAX;
    ccmgsl->targetLineY = esp_random() % (TARGET_LINE_Y_MAX - TARGET_LINE_Y_MIN) + TARGET_LINE_Y_MIN;

    ccmgsl->knifeX       = ccmgsl->targetArea.pos.x - KNIFE_X_OFFSET;
    ccmgsl->knifeBottomY = TFT_HEIGHT / 2 - ccmgsl->wsg.knifeBottom.h;
    ccmgsl->knifeAngle   = 0;
}

static void ccmgSliceDestroyMicrogame(void)
{
    cosCrunchMicrogamePersistSplatter(ccmgsl->wsg.canvas, 0, 0);

    freeWsg(&ccmgsl->wsg.knifeTop);
    freeWsg(&ccmgsl->wsg.knifeBottom);
    freeWsg(&ccmgsl->wsg.slash);
    freeWsg(&ccmgsl->wsg.slashUnderlay);
    freeWsg(&ccmgsl->wsg.slashOverlay);

    heap_caps_free(ccmgsl);
}

static void ccmgSliceMainLoop(int64_t elapsedUs, uint64_t timeRemainingUs, cosCrunchMicrogameState state,
                              buttonEvt_t buttonEvts[], uint8_t buttonEvtCount)
{
    for (uint8_t i = 0; i < buttonEvtCount; i++)
    {
        ccmgsl->buttonState = buttonEvts[i].state;
        if (buttonEvts[i].button == PB_A && buttonEvts[i].down && state == CC_MG_PLAYING)
        {
            int16_t cutLineY1   = ccmgsl->targetArea.pos.y + ccmgsl->targetLineY;
            int16_t cutLineY2   = cutLineY1 - ccmgSliceGetYIntersect(ccmgsl->targetAngle, ccmgsl->targetArea.width);
            int16_t knifeLineY1 = ccmgsl->knifeBottomY + ccmgsl->wsg.knifeBottom.h
                                  - ccmgSliceGetYIntersect(ccmgsl->knifeAngle, KNIFE_X_OFFSET);
            int16_t knifeLineY2
                = ccmgsl->knifeBottomY + ccmgsl->wsg.knifeBottom.h
                  - ccmgSliceGetYIntersect(ccmgsl->knifeAngle, ccmgsl->targetArea.width + KNIFE_X_OFFSET);
            int16_t totalError = ABS(cutLineY1 - knifeLineY1) + ABS(cutLineY2 - knifeLineY2);
            bool success       = totalError <= TOTAL_ERROR_THRESHOLD;
            cosCrunchMicrogameResult(success);

            if (success)
            {
                // Draw paper confetti on the canvas to persist to the mat
                int16_t minX = ccmgsl->targetArea.pos.x - 15;
                int16_t maxX = ccmgsl->targetArea.pos.x + ccmgsl->targetArea.width + 15;
                int16_t minY = ccmgsl->targetArea.pos.y - 15;
                int16_t maxY = ccmgsl->targetArea.pos.y + ccmgsl->targetLineY + 15;
                for (uint8_t pxl = 0; pxl < 50; pxl++)
                {
                    int16_t x = esp_random() % (maxX - minX) + minX;
                    int16_t y = esp_random() % (maxY - minY) + minY;
                    paletteColor_t color;
                    if (esp_random() % 2 == 0)
                    {
                        color = ccmgsl->tintColor->highlight;
                    }
                    else
                    {
                        color = ccmgsl->tintColor->base;
                    }
                    ccmgsl->wsg.canvas.px[TFT_WIDTH * y + x] = color;
                }
            }
        }
    }

    if (state == CC_MG_PLAYING)
    {
        if (ccmgsl->buttonState & PB_UP)
        {
            ccmgsl->knifeBottomY -= KNIFE_Y_SPEED_PX_PER_US * elapsedUs;
        }
        else if (ccmgsl->buttonState & PB_DOWN)
        {
            ccmgsl->knifeBottomY += KNIFE_Y_SPEED_PX_PER_US * elapsedUs;
        }

        if (ccmgsl->buttonState & PB_LEFT)
        {
            ccmgsl->knifeAngle = MIN(ccmgsl->knifeAngle + KNIFE_X_SPEED_DEG_PER_US * elapsedUs, TARGET_ANGLE_MAX + 10);
        }
        else if (ccmgsl->buttonState & PB_RIGHT)
        {
            ccmgsl->knifeAngle
                = MAX(ccmgsl->knifeAngle - KNIFE_X_SPEED_DEG_PER_US * elapsedUs, -(TARGET_ANGLE_MAX + 10));
        }
    }

    int16_t targetDeltaY = ccmgSliceGetYIntersect(ccmgsl->targetAngle, ccmgsl->targetArea.width);
    int16_t targetTopY   = ccmgsl->targetArea.pos.y;
    int16_t shadowTopY   = targetTopY;
    if (state == CC_MG_CELEBRATING && ccmgsl->endFadeComplete)
    {
        int16_t triangleTopX, triangleTopY;
        if (targetDeltaY < 0)
        {
            triangleTopX = ccmgsl->targetArea.pos.x;
            triangleTopY = ccmgsl->targetArea.pos.y + ccmgsl->targetLineY;
            targetTopY   = targetTopY + ccmgsl->targetLineY - targetDeltaY;
            shadowTopY   = targetTopY + 1;
        }
        else
        {
            triangleTopX = ccmgsl->targetArea.pos.x + ccmgsl->targetArea.width - 1;
            triangleTopY = ccmgsl->targetArea.pos.y + ccmgsl->targetLineY - targetDeltaY;
            targetTopY   = targetTopY + ccmgsl->targetLineY;
            shadowTopY   = triangleTopY;
        }
        drawTriangleOutlined(ccmgsl->targetArea.pos.x, targetTopY + 1,
                             ccmgsl->targetArea.pos.x + ccmgsl->targetArea.width - 1, targetTopY + 1, triangleTopX,
                             triangleTopY + 1, ccmgsl->tintColor->base, ccmgsl->tintColor->base);
    }

    // Tint color rectangle
    drawRectFilled(ccmgsl->targetArea.pos.x, targetTopY, ccmgsl->targetArea.pos.x + ccmgsl->targetArea.width,
                   ccmgsl->targetArea.pos.y + ccmgsl->targetArea.height, ccmgsl->tintColor->base);

    // Dotted lines to make a dithered shadow
    drawLine(ccmgsl->targetArea.pos.x + 1, ccmgsl->targetArea.pos.y + ccmgsl->targetArea.height,
             ccmgsl->targetArea.pos.x + 1 + ccmgsl->targetArea.width,
             ccmgsl->targetArea.pos.y + ccmgsl->targetArea.height, c111, 1);
    drawLine(ccmgsl->targetArea.pos.x + 2, ccmgsl->targetArea.pos.y + ccmgsl->targetArea.height + 1,
             ccmgsl->targetArea.pos.x + 2 + ccmgsl->targetArea.width,
             ccmgsl->targetArea.pos.y + ccmgsl->targetArea.height + 1, c111, 1);
    drawLine(ccmgsl->targetArea.pos.x + ccmgsl->targetArea.width, shadowTopY + 1,
             ccmgsl->targetArea.pos.x + ccmgsl->targetArea.width,
             ccmgsl->targetArea.pos.y + 1 + ccmgsl->targetArea.height, c111, 1);
    drawLine(ccmgsl->targetArea.pos.x + ccmgsl->targetArea.width + 1, shadowTopY + 2,
             ccmgsl->targetArea.pos.x + ccmgsl->targetArea.width + 1,
             ccmgsl->targetArea.pos.y + 2 + ccmgsl->targetArea.height, c111, 1);

    int16_t knifeDeltaY  = ccmgSliceGetYIntersect(ccmgsl->knifeAngle, ccmgsl->targetArea.width + 2 * KNIFE_X_OFFSET);
    int16_t knifeX       = ccmgsl->knifeX;
    int16_t knifeBottomY = ccmgsl->knifeBottomY;
    int16_t knifeTopXYOffset = 0;
    if (!ccmgsl->endFadeComplete)
    {
        // Dashed line to show where the cut should go
        drawLine(ccmgsl->targetArea.pos.x, ccmgsl->targetLineY + ccmgsl->targetArea.pos.y,
                 ccmgsl->targetArea.pos.x + ccmgsl->targetArea.width - 1,
                 ccmgsl->targetLineY + ccmgsl->targetArea.pos.y - targetDeltaY, ccmgsl->tintColor->lowlight, 4);

        // Solid line showing where the knife will cut
        drawLineFast(ccmgsl->knifeX, ccmgsl->knifeBottomY + ccmgsl->wsg.knifeBottom.h,
                     ccmgsl->knifeX + ccmgsl->targetArea.width + 2 * KNIFE_X_OFFSET,
                     ccmgsl->knifeBottomY + ccmgsl->wsg.knifeBottom.h - knifeDeltaY, c111);
    }
    else if (state == CC_MG_CELEBRATING)
    {
        knifeX += ccmgsl->targetArea.width + 2 * KNIFE_X_OFFSET;
        knifeBottomY -= knifeDeltaY;
    }
    else if (state == CC_MG_DESPAIRING && ccmgsl->endElapsedUs >= 1900000)
    {
        knifeTopXYOffset = (ccmgsl->endElapsedUs - 1900000) / 100000;
    }

    drawWsgSimple(&ccmgsl->wsg.knifeBottom, knifeX, knifeBottomY);
    drawWsgSimple(&ccmgsl->wsg.knifeTop, knifeX - 1 + knifeTopXYOffset,
                  knifeBottomY - ccmgsl->wsg.knifeTop.h + (ccmgsl->wsg.knifeTop.w - 3) + knifeTopXYOffset);

    if (state == CC_MG_CELEBRATING || state == CC_MG_DESPAIRING)
    {
        uint8_t shadeLevel = ccmgsl->endElapsedUs / (END_ANIM_FADE_LENGTH_US / 5);
        if (shadeLevel <= 4 && !ccmgsl->endFadeComplete)
        {
            shadeDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, shadeLevel, c000);
        }
        else if (ccmgsl->endElapsedUs < END_ANIM_FADE_END_TIME_US)
        {
            ccmgsl->endFadeComplete = true;
            fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);

            if (ccmgsl->endElapsedUs >= END_ANIM_SLASH_START_TIME_US)
            {
                // Animate slash in a partial ellipse
                int16_t angle  = MIN(180
                                             * ((float)(ccmgsl->endElapsedUs - END_ANIM_SLASH_START_TIME_US)
                                               / (float)(END_ANIM_SLASH_STOP_TIME_US - END_ANIM_SLASH_START_TIME_US))
                                         + 100,
                                     360);
                int16_t slashX = ccmgsl->knifeX - 15 + getCos1024(angle) * END_ANIM_ELLIPSE_WIDTH / 1024;
                int16_t slashY = ccmgsl->knifeBottomY + ccmgsl->wsg.knifeBottom.h - 10
                                 - getSin1024(angle) * END_ANIM_ELLIPSE_HEIGHT / 1024;

                // Three entire frames? In THIS economy?
                if (ccmgsl->endElapsedUs >= END_ANIM_SLASH_FRAME_LENGTH_US + END_ANIM_SLASH_START_TIME_US)
                {
                    drawWsgPaletteSimple(&ccmgsl->wsg.slashUnderlay, slashX + 21, slashY - 4, &ccmgsl->tintPalette);
                }
                if (ccmgsl->endElapsedUs >= END_ANIM_SLASH_START_TIME_US)
                {
                    drawWsgSimple(&ccmgsl->wsg.slash, slashX, slashY);
                    if (ccmgsl->oldEndElapsedUs < END_ANIM_SLASH_START_TIME_US)
                    {
                        midiNoteOn(globalMidiPlayerGet(MIDI_SFX), 9, OPEN_HI_HAT, 0x7f);
                    }
                }
                if (ccmgsl->endElapsedUs >= 2 * END_ANIM_SLASH_FRAME_LENGTH_US + END_ANIM_SLASH_START_TIME_US)
                {
                    drawWsgPaletteSimple(&ccmgsl->wsg.slashOverlay, slashX + 60, slashY - 26, &ccmgsl->tintPalette);
                }
            }
        }
        else
        {
            if (state == CC_MG_CELEBRATING)
            {
                drawWsgSimple(&ccmgsl->wsg.canvas, 0, 0);
            }
        }

        ccmgsl->oldEndElapsedUs = ccmgsl->endElapsedUs;
        ccmgsl->endElapsedUs += elapsedUs;
    }
}

static int16_t ccmgSliceGetYIntersect(int16_t angle, int16_t width)
{
    while (angle < 0 || angle > 359)
    {
        angle = (angle + 360) % 360;
    }
    return getTan1024(angle) * width / 1024;
}
